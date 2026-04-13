#include "nova/semantic/semantic_analyzer.h"
#include "nova/ast/ast_node.h"
#include <algorithm>

namespace nova {

namespace {

const SceneDefNode* as_scene(const AstNode* node) {
    return dynamic_cast<const SceneDefNode*>(node);
}

const VarDefNode* as_var_def(const AstNode* node) {
    return dynamic_cast<const VarDefNode*>(node);
}

const CharDefNode* as_char_def(const AstNode* node) {
    return dynamic_cast<const CharDefNode*>(node);
}

const ItemDefNode* as_item_def(const AstNode* node) {
    return dynamic_cast<const ItemDefNode*>(node);
}

const LabelNode* as_label(const AstNode* node) {
    return dynamic_cast<const LabelNode*>(node);
}

const ThemeDefNode* as_theme_def(const AstNode* node) {
    return dynamic_cast<const ThemeDefNode*>(node);
}

const JumpNode* as_jump(const AstNode* node) {
    return dynamic_cast<const JumpNode*>(node);
}

const DialogueNode* as_dialogue(const AstNode* node) {
    return dynamic_cast<const DialogueNode*>(node);
}

const ChoiceNode* as_choice(const AstNode* node) {
    return dynamic_cast<const ChoiceNode*>(node);
}

const ChoiceOptionNode* as_choice_option(const AstNode* node) {
    return dynamic_cast<const ChoiceOptionNode*>(node);
}

const BranchNode* as_branch(const AstNode* node) {
    return dynamic_cast<const BranchNode*>(node);
}

const SetCommandNode* as_set_cmd(const AstNode* node) {
    return dynamic_cast<const SetCommandNode*>(node);
}

const FlagNode* as_flag_cmd(const AstNode* node) {
    return dynamic_cast<const FlagNode*>(node);
}

const GiveCommandNode* as_give_cmd(const AstNode* node) {
    return dynamic_cast<const GiveCommandNode*>(node);
}

const TakeCommandNode* as_take_cmd(const AstNode* node) {
    return dynamic_cast<const TakeCommandNode*>(node);
}

const CheckCommandNode* as_check_cmd(const AstNode* node) {
    return dynamic_cast<const CheckCommandNode*>(node);
}

const IdentifierNode* as_identifier(const AstNode* node) {
    return dynamic_cast<const IdentifierNode*>(node);
}

const CallExprNode* as_call_expr(const AstNode* node) {
    return dynamic_cast<const CallExprNode*>(node);
}

const BinaryExprNode* as_binary_expr(const AstNode* node) {
    return dynamic_cast<const BinaryExprNode*>(node);
}

const UnaryExprNode* as_unary_expr(const AstNode* node) {
    return dynamic_cast<const UnaryExprNode*>(node);
}

}

SemanticAnalyzer::SemanticAnalyzer() = default;

SemanticAnalysisResult SemanticAnalyzer::analyze(const ProgramNode* program) {
    m_diagnostics.clear();
    m_symbols.clear();
    
    collect_definitions(program);
    check_references(program);
    check_structures(program);
    check_unused_symbols();
    
    return SemanticAnalysisResult{
        !m_diagnostics.has_errors(),
        std::move(m_diagnostics),
        std::move(m_symbols)
    };
}

SemanticAnalysisResult SemanticAnalyzer::analyze(const AstPtr& program) {
    return analyze(dynamic_cast<const ProgramNode*>(program.get()));
}

void SemanticAnalyzer::collect_definitions(const ProgramNode* program) {
    for (const auto& stmt : program->statements()) {
        collect_from_node(stmt.get());
    }
}

void SemanticAnalyzer::define_variable_symbol(const VarDefNode* var) {
    if (!var) return;

    const std::string scope = m_symbols.current_variable_scope();
    if (m_symbols.exists_in_current_scope(var->name())) {
        m_diagnostics.error(SemanticError::DuplicateVariable,
            "duplicate variable definition: " + var->name(),
            var->location());
        return;
    }

    m_symbols.define(var->name(), SymbolKind::Variable,
                    var->location(), scope);
}

void SemanticAnalyzer::ensure_variable_symbol(const std::string& name, SourceLocation loc) {
    const std::string scope = m_symbols.current_variable_scope();
    if (m_symbols.exists(name, scope)) {
        return;
    }

    m_symbols.define(name, SymbolKind::Variable, std::move(loc), scope);
}

void SemanticAnalyzer::analyze_statement_sequence(const std::vector<AstPtr>& statements,
                                                 bool new_block_scope) {
    if (new_block_scope) {
        m_symbols.enter_block();
    }

    for (const auto& stmt : statements) {
        if (!stmt) {
            continue;
        }

        if (stmt->type() == NodeType::VarDef) {
            define_variable_symbol(as_var_def(stmt.get()));
        }

        check_node_references(stmt.get());
        check_node_structure(stmt.get());
    }

    if (new_block_scope) {
        m_symbols.leave_block();
    }
}

void SemanticAnalyzer::collect_from_node(const AstNode* node) {
    if (!node) return;
    
    switch (node->type()) {
        case NodeType::SceneDef: {
            auto scene = as_scene(node);
            if (m_symbols.exists(scene->name())) {
                m_diagnostics.error(SemanticError::DuplicateScene,
                    "duplicate scene definition: " + scene->name(),
                    scene->location());
            } else {
                m_symbols.define(scene->name(), SymbolKind::Scene, 
                                scene->location());
            }
            break;
        }
        
        case NodeType::VarDef: {
            auto var = as_var_def(node);
            define_variable_symbol(var);
            break;
        }
        
        case NodeType::CharDef: {
            auto ch = as_char_def(node);
            if (m_symbols.exists(ch->name())) {
                m_diagnostics.error(SemanticError::DuplicateCharacter,
                    "duplicate character definition: " + ch->name(),
                    ch->location());
            } else {
                m_symbols.define(ch->name(), SymbolKind::Character,
                                ch->location());
            }
            break;
        }
        
        case NodeType::ItemDef: {
            auto item = as_item_def(node);
            if (m_symbols.exists(item->name())) {
                m_diagnostics.error(SemanticError::DuplicateItem,
                    "duplicate item definition: " + item->name(),
                    item->location());
            } else {
                m_symbols.define(item->name(), SymbolKind::Item,
                                item->location());
            }
            break;
        }
        
        case NodeType::Label: {
            auto label = as_label(node);
            if (m_symbols.exists(label->name(), m_symbols.current_scene())) {
                m_diagnostics.error(SemanticError::DuplicateLabel,
                    "duplicate label in scene: " + label->name(),
                    label->location());
            } else {
                m_symbols.define(label->name(), SymbolKind::Label,
                                label->location(), m_symbols.current_scene());
            }
            break;
        }
        
        case NodeType::ThemeDef: {
            auto theme = as_theme_def(node);
            if (m_symbols.exists(theme->name())) {
                m_diagnostics.error(SemanticError::DuplicateVariable,
                    "duplicate theme: " + theme->name(),
                    theme->location());
            } else {
                m_symbols.define(theme->name(), SymbolKind::Theme,
                                theme->location());
            }
            break;
        }
        
        default:
            break;
    }
}

void SemanticAnalyzer::check_references(const ProgramNode* program) {
    for (const auto& stmt : program->statements()) {
        if (!stmt) {
            continue;
        }

        if (stmt->type() == NodeType::VarDef) {
            check_node_references(stmt.get());
            continue;
        }

        check_node_references(stmt.get());
    }
}

void SemanticAnalyzer::check_node_references(const AstNode* node) {
    if (!node) return;
    
    switch (node->type()) {
        case NodeType::SceneDef: {
            auto scene = as_scene(node);
            m_symbols.enter_scene(scene->name());
            break;
        }
        
        case NodeType::Dialogue: {
            auto dialogue = as_dialogue(node);
            check_character_ref(dialogue->speaker(), dialogue->location());
            break;
        }
        
        case NodeType::Jump: {
            auto jump = as_jump(node);
            check_jump_target(jump->target(), jump->location());
            break;
        }
        
        case NodeType::Choice: {
            auto choice = as_choice(node);
            for (const auto& opt : choice->options()) {
                auto opt_node = as_choice_option(opt.get());
                if (opt_node) {
                    if (!opt_node->target().empty()) {
                        check_jump_target(opt_node->target(), opt_node->location());
                    }
                    if (opt_node->condition()) {
                        check_expression(opt_node->condition());
                    }
                    if (!opt_node->body().empty()) {
                        analyze_statement_sequence(opt_node->body(), true);
                    }
                }
            }
            break;
        }
        
        case NodeType::Branch: {
            auto branch = as_branch(node);
            if (branch->condition()) {
                check_expression(branch->condition());
            }
            analyze_statement_sequence(branch->then_branch(), true);
            analyze_statement_sequence(branch->else_branch(), true);
            break;
        }
        
        case NodeType::SetCommand: {
            auto set = as_set_cmd(node);
            if (set->value()) {
                check_expression(set->value());
            }
            ensure_variable_symbol(set->name(), set->location());
            break;
        }
        
        case NodeType::GiveCommand: {
            auto give = as_give_cmd(node);
            check_item_ref(give->item(), give->location());
            break;
        }
        
        case NodeType::TakeCommand: {
            auto take = as_take_cmd(node);
            check_item_ref(take->item(), take->location());
            break;
        }
        
        case NodeType::CheckCommand: {
            auto check = as_check_cmd(node);
            if (check->condition()) {
                check_expression(check->condition());
            }
            analyze_statement_sequence(check->success_branch(), true);
            analyze_statement_sequence(check->failure_branch(), true);
            break;
        }
        
        case NodeType::VarDef: {
            auto var = as_var_def(node);
            if (var->init_value()) {
                check_expression(var->init_value());
            }
            break;
        }
        
        default:
            break;
    }
}

void SemanticAnalyzer::check_expression(const AstNode* expr) {
    if (!expr) return;
    
    switch (expr->type()) {
        case NodeType::Identifier: {
            auto id = as_identifier(expr);
            check_variable_ref(id->name(), id->location());
            break;
        }
        
        case NodeType::BinaryExpr: {
            auto bin = as_binary_expr(expr);
            check_expression(bin->left());
            check_expression(bin->right());
            break;
        }
        
        case NodeType::UnaryExpr: {
            auto unary = as_unary_expr(expr);
            check_expression(unary->operand());
            break;
        }
        
        case NodeType::CallExpr: {
            auto call = as_call_expr(expr);
            std::vector<AstNode*> args;
            for (const auto& arg : call->arguments()) {
                args.push_back(arg.get());
            }
            check_function_call(call->name(), args, call->location());
            break;
        }
        
        default:
            break;
    }
}

void SemanticAnalyzer::check_variable_ref(const std::string& name, SourceLocation loc) {
    const std::string scope = m_symbols.current_variable_scope();
    if (!m_symbols.exists(name, scope)) {
        m_diagnostics.error(SemanticError::UndefinedVariable,
            "undefined variable: " + name, loc);
    } else {
        m_symbols.mark_used(name, scope);
    }
}

void SemanticAnalyzer::check_jump_target(const std::string& target, SourceLocation loc) {
    if (target.empty()) return;
    
    if (target[0] == '.') {
        if (!m_symbols.exists(target.substr(1), m_symbols.current_scene())) {
            m_diagnostics.error(SemanticError::UndefinedLabel,
                "undefined label: " + target, loc);
        } else {
            m_symbols.mark_used(target.substr(1), m_symbols.current_scene());
        }
    } else {
        if (!m_symbols.exists(target)) {
            m_diagnostics.error(SemanticError::UndefinedScene,
                "undefined scene: " + target, loc);
        } else {
            m_symbols.mark_used(target);
        }
    }
}

void SemanticAnalyzer::check_character_ref(const std::string& name, SourceLocation loc) {
    if (!m_symbols.exists(name)) {
        m_diagnostics.warning(SemanticError::UndefinedCharacter,
            "undefined character (will be auto-created): " + name, loc);
    } else {
        m_symbols.mark_used(name);
    }
}

void SemanticAnalyzer::check_item_ref(const std::string& name, SourceLocation loc) {
    if (!m_symbols.exists(name)) {
        m_diagnostics.error(SemanticError::UndefinedItem,
            "undefined item: " + name, loc);
    } else {
        m_symbols.mark_used(name);
    }
}

void SemanticAnalyzer::check_function_call(const std::string& name,
                                           const std::vector<AstNode*>& args,
                                           SourceLocation loc) {
    static const std::unordered_map<std::string, size_t> builtin_functions = {
        {"has_ending", 1},
        {"has_flag", 1},
        {"has_item", 1},
        {"item_count", 1},
        {"var", 1},
        {"roll", 1},
        {"random", 2},
        {"chance", 1},
    };
    
    auto it = builtin_functions.find(name);
    if (it == builtin_functions.end()) {
        m_diagnostics.error(SemanticError::InvalidFunctionCall,
            "unknown function: " + name, loc);
        return;
    }
    
    if (args.size() != it->second) {
        m_diagnostics.error(SemanticError::InvalidFunctionCall,
            "function " + name + " expects " + std::to_string(it->second) +
            " arguments, got " + std::to_string(args.size()), loc);
        return;
    }
    
    if (name == "roll") {
        if (args.empty()) {
            return;
        }
        auto* lit = dynamic_cast<LiteralNode*>(args[0]);
        if (!lit || !lit->is_string()) {
            m_diagnostics.error(SemanticError::InvalidFunctionCall,
                "function roll expects a string dice expression", loc);
            return;
        }
        for (auto* arg : args) {
            check_expression(arg);
        }
        return;
    }

    if (name == "has_item" || name == "item_count" || name == "has_flag" || name == "has_ending" || name == "var") {
        auto* lit = dynamic_cast<LiteralNode*>(args[0]);
        auto* id = dynamic_cast<IdentifierNode*>(args[0]);
        if (!id && (!lit || !lit->is_string())) {
            m_diagnostics.error(SemanticError::InvalidFunctionCall,
                "function " + name + " expects an identifier or string argument", loc);
            return;
        }
        if (id && name == "var") {
            check_variable_ref(id->name(), id->location());
        }
        return;
    }

    for (auto* arg : args) {
        check_expression(arg);
    }
}

void SemanticAnalyzer::check_structures(const ProgramNode* program) {
    for (const auto& stmt : program->statements()) {
        check_node_structure(stmt.get());
    }
}

void SemanticAnalyzer::check_node_structure(const AstNode* node) {
    if (!node) return;
    
    switch (node->type()) {
        case NodeType::Choice: {
            auto choice = as_choice(node);
            if (choice->options().empty()) {
                m_diagnostics.error(SemanticError::EmptyChoice,
                    "choice must have at least one option", choice->location());
            }

            for (const auto& opt : choice->options()) {
                auto opt_node = as_choice_option(opt.get());
                if (!opt_node) {
                    continue;
                }

                if (opt_node->body().empty()) {
                    if (opt_node->target().empty()) {
                        m_diagnostics.error(SemanticError::MissingChoiceTarget,
                            "choice option must have a target or end with '-> target' in block-style syntax",
                            opt_node->location());
                    }
                    continue;
                }

                const auto& body = opt_node->body();
                const auto* last_stmt = body.back().get();
                if (!last_stmt || last_stmt->type() != NodeType::Jump) {
                    m_diagnostics.error(SemanticError::MissingChoiceTarget,
                        "block-style choice option must end with '-> target'",
                        opt_node->location());
                    continue;
                }

                for (size_t i = 0; i < body.size(); ++i) {
                    const auto* stmt = body[i].get();
                    const bool is_last = i + 1 == body.size();
                    const bool is_allowed_action = stmt && (stmt->type() == NodeType::SetCommand || stmt->type() == NodeType::Flag);
                    const bool is_jump = stmt && stmt->type() == NodeType::Jump;

                    if ((is_last && !is_jump) || (!is_last && !is_allowed_action)) {
                        m_diagnostics.error(SemanticError::InvalidChoiceAction,
                            "block-style choice option only allows @set/@flag before a final '-> target'",
                            stmt ? stmt->location() : opt_node->location());
                    }
                }
            }
            break;
        }
        
        case NodeType::CheckCommand: {
            auto check = as_check_cmd(node);
            if (check->success_branch().empty()) {
                m_diagnostics.warning(SemanticError::MissingSuccessBranch,
                    "@check has no success branch", check->location());
            }
            if (check->failure_branch().empty()) {
                m_diagnostics.warning(SemanticError::MissingFailBranch,
                    "@check has no fail branch", check->location());
            }
            for (const auto& stmt : check->success_branch()) {
                check_node_structure(stmt.get());
            }
            for (const auto& stmt : check->failure_branch()) {
                check_node_structure(stmt.get());
            }
            break;
        }
        
        case NodeType::Branch: {
            auto branch = as_branch(node);
            for (const auto& stmt : branch->then_branch()) {
                check_node_structure(stmt.get());
            }
            for (const auto& stmt : branch->else_branch()) {
                check_node_structure(stmt.get());
            }
            break;
        }
        
        default:
            break;
    }
}

void SemanticAnalyzer::check_unused_symbols() {
    auto unused = m_symbols.get_unused_symbols();
    for (const auto& sym : unused) {
        if (sym.kind == SymbolKind::Variable || sym.kind == SymbolKind::Item) {
            m_diagnostics.warning(SemanticError::UnusedVariable,
                "unused " + std::string(sym.kind == SymbolKind::Variable ? "variable" : "item") +
                ": " + sym.name, sym.definition_location);
        }
    }
}

} // namespace nova
