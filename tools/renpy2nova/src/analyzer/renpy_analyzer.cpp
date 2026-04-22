#include "renpy2nova/analyzer/renpy_analyzer.h"

#include <utility>
#include <vector>

namespace nova::renpy2nova {
namespace {

bool is_ignored_token(RenpyTokenType type) {
    return type == RenpyTokenType::Blank || type == RenpyTokenType::Comment;
}

bool opens_block(RenpyTokenType type) {
    return type == RenpyTokenType::Label
        || type == RenpyTokenType::Menu
        || type == RenpyTokenType::MenuChoice
        || type == RenpyTokenType::IfStatement
        || type == RenpyTokenType::ElifStatement
        || type == RenpyTokenType::ElseStatement
        || type == RenpyTokenType::PythonBlockStart
        || type == RenpyTokenType::InitPythonBlockStart
        || type == RenpyTokenType::UnsupportedConstruct;
}

RenpyNode make_node(const RenpyToken& token) {
    RenpyNode node;
    node.unsupported_kind = token.unsupported_kind;
    node.name = token.primary;
    node.value = token.secondary;
    node.line = token.line;
    node.column = token.column;
    node.indent = token.indent;

    switch (token.type) {
    case RenpyTokenType::DefineCharacter:
        node.kind = RenpyNodeKind::CharacterDefinition;
        break;
    case RenpyTokenType::DefaultStatement:
        node.kind = RenpyNodeKind::DefaultStatement;
        break;
    case RenpyTokenType::IfStatement:
        node.kind = RenpyNodeKind::If;
        break;
    case RenpyTokenType::ElifStatement:
        node.kind = RenpyNodeKind::Elif;
        break;
    case RenpyTokenType::ElseStatement:
        node.kind = RenpyNodeKind::Else;
        break;
    case RenpyTokenType::Label:
        node.kind = RenpyNodeKind::Label;
        break;
    case RenpyTokenType::Jump:
        node.kind = RenpyNodeKind::Jump;
        break;
    case RenpyTokenType::Call:
        node.kind = RenpyNodeKind::Call;
        break;
    case RenpyTokenType::Return:
        node.kind = RenpyNodeKind::Return;
        break;
    case RenpyTokenType::Say:
        node.kind = RenpyNodeKind::Say;
        break;
    case RenpyTokenType::Narrator:
        node.kind = RenpyNodeKind::Narration;
        break;
    case RenpyTokenType::Menu:
        node.kind = RenpyNodeKind::Menu;
        break;
    case RenpyTokenType::MenuChoice:
        node.kind = RenpyNodeKind::MenuChoice;
        break;
    case RenpyTokenType::Scene:
        node.kind = RenpyNodeKind::Scene;
        break;
    case RenpyTokenType::Show:
        node.kind = RenpyNodeKind::Show;
        break;
    case RenpyTokenType::Hide:
        node.kind = RenpyNodeKind::Hide;
        break;
    case RenpyTokenType::PlayMusic:
        node.kind = RenpyNodeKind::PlayMusic;
        break;
    case RenpyTokenType::PlaySound:
        node.kind = RenpyNodeKind::PlaySound;
        break;
    case RenpyTokenType::DollarStatement:
        node.kind = RenpyNodeKind::DollarStatement;
        break;
    case RenpyTokenType::PythonBlockStart:
        node.kind = RenpyNodeKind::PythonBlock;
        break;
    case RenpyTokenType::InitPythonBlockStart:
        node.kind = RenpyNodeKind::InitPythonBlock;
        break;
    case RenpyTokenType::WithStatement:
        node.kind = RenpyNodeKind::With;
        break;
    case RenpyTokenType::UnsupportedConstruct:
    case RenpyTokenType::Unknown:
        node.kind = RenpyNodeKind::Unsupported;
        if (node.unsupported_kind == RenpyUnsupportedKind::None) {
            node.unsupported_kind = RenpyUnsupportedKind::Unknown;
        }
        if (node.value.empty()) {
            node.value = token.lexeme;
        }
        break;
    case RenpyTokenType::Blank:
    case RenpyTokenType::Comment:
    case RenpyTokenType::Eof:
        node.kind = RenpyNodeKind::Unsupported;
        break;
    }

    if (token.type == RenpyTokenType::Narrator || token.type == RenpyTokenType::Menu) {
        node.value = token.primary;
        node.name.clear();
    }

    if (token.type == RenpyTokenType::MenuChoice) {
        node.value = token.primary;
        node.name = token.secondary;
    }

    return node;
}

std::vector<RenpyNode> fold_conditionals(std::vector<RenpyNode> nodes) {
    for (auto& node : nodes) {
        node.children = fold_conditionals(std::move(node.children));
        node.else_children = fold_conditionals(std::move(node.else_children));
    }

    std::vector<RenpyNode> folded;
    for (size_t index = 0; index < nodes.size(); ++index) {
        RenpyNode node = std::move(nodes[index]);
        if (node.kind != RenpyNodeKind::If) {
            folded.push_back(std::move(node));
            continue;
        }

        if (index + 1 < nodes.size() && nodes[index + 1].kind == RenpyNodeKind::Elif) {
            std::vector<RenpyNode> chain;
            while (index + 1 < nodes.size() && nodes[index + 1].kind == RenpyNodeKind::Elif) {
                RenpyNode nested = std::move(nodes[index + 1]);
                nested.kind = RenpyNodeKind::If;
                chain.push_back(std::move(nested));
                ++index;
            }

            if (!chain.empty()) {
                for (size_t chain_index = 0; chain_index + 1 < chain.size(); ++chain_index) {
                    chain[chain_index].else_children.push_back(std::move(chain[chain_index + 1]));
                }

                if (index + 1 < nodes.size() && nodes[index + 1].kind == RenpyNodeKind::Else) {
                    chain.back().else_children = std::move(nodes[index + 1].children);
                    ++index;
                }

                node.else_children.push_back(std::move(chain.front()));
            }
        } else if (index + 1 < nodes.size() && nodes[index + 1].kind == RenpyNodeKind::Else) {
            node.else_children = std::move(nodes[index + 1].children);
            ++index;
        }

        node.else_children = fold_conditionals(std::move(node.else_children));
        folded.push_back(std::move(node));
    }

    return folded;
}

std::vector<RenpyNode> parse_block(const std::vector<RenpyToken>& tokens, size_t& index, size_t base_indent) {
    std::vector<RenpyNode> nodes;

    while (index < tokens.size()) {
        const RenpyToken& token = tokens[index];
        if (token.type == RenpyTokenType::Eof) {
            break;
        }

        if (is_ignored_token(token.type)) {
            ++index;
            continue;
        }

        if (token.indent < base_indent) {
            break;
        }

        if (token.indent > base_indent) {
            break;
        }

        RenpyNode node = make_node(token);
        ++index;

        if (opens_block(token.type) && index < tokens.size()) {
            size_t child_index = index;
            while (child_index < tokens.size() && is_ignored_token(tokens[child_index].type)) {
                ++child_index;
            }

            if (child_index < tokens.size()
                && tokens[child_index].type != RenpyTokenType::Eof
                && tokens[child_index].indent > token.indent) {
                index = child_index;
                node.children = parse_block(tokens, index, tokens[child_index].indent);
            }
        }

        nodes.push_back(std::move(node));
    }

    return nodes;
}

} // namespace

ConversionResult<RenpyProject> RenpyAnalyzer::analyze(std::vector<RenpyToken> tokens) const {
    size_t index = 0;
    std::vector<RenpyNode> statements = fold_conditionals(parse_block(tokens, index, 0));
    RenpyProject project{std::move(tokens), std::move(statements)};
    return ConversionResult<RenpyProject>(std::move(project));
}

} // namespace nova::renpy2nova
