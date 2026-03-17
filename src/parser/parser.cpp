#include "nova/parser/parser.h"

namespace nova {

Parser::Parser(std::vector<Token> tokens)
    : m_tokens(std::move(tokens))
{
}

Result<AstPtr> Parser::parse() {
    auto program = std::make_unique<ProgramNode>(SourceLocation::unknown());
    
    while (check(TokenType::Newline)) {
        advance();
    }
    if (at_end()) {
        return Ok(AstPtr(program.release()));
    }
    
    if (check(TokenType::Minus)) {
        SourceLocation loc = current().location;
        advance();
        if (check(TokenType::Minus)) {
            advance();
            if (check(TokenType::Minus)) {
                advance();
                if (check(TokenType::Newline)) advance();
                
                auto front_matter = std::make_unique<FrontMatterNode>(loc);
                
                while (!at_end() && !check(TokenType::Minus)) {
                    while (check(TokenType::Newline)) advance();
                    if (at_end() || check(TokenType::Minus)) break;
                    
                    std::string key;
                    if (check(TokenType::Identifier) || check(TokenType::Text)) {
                        key = current().value;
                        advance();
                    } else {
                        break;
                    }
                    
                    if (check(TokenType::Colon)) {
                        advance();
                        std::string value = parse_property_value();
                        front_matter->add_property(std::move(key), std::move(value));
                    }
                    
                    if (check(TokenType::Newline)) advance();
                }
                
                if (check(TokenType::Minus)) {
                    advance();
                    if (check(TokenType::Minus)) {
                        advance();
                        if (check(TokenType::Minus)) {
                            advance();
                            if (check(TokenType::Newline)) advance();
                        }
                    }
                }
                
                program->add_statement(AstPtr(front_matter.release()));
            }
        }
    }
    
    while (!at_end()) {
        while (check(TokenType::Newline)) {
            advance();
        }
        if (at_end()) break;
        
        auto stmt = parse_statement();
        if (stmt.is_err()) {
            return stmt;
        }
        program->add_statement(std::move(stmt.unwrap()));
    }
    
    return Ok(AstPtr(program.release()));
}

const Token& Parser::current() const {
    static Token eof{TokenType::Eof, {}, SourceLocation::unknown()};
    if (m_pos >= m_tokens.size()) return eof;
    return m_tokens[m_pos];
}

const Token& Parser::peek() const {
    static Token eof{TokenType::Eof, {}, SourceLocation::unknown()};
    if (m_pos + 1 >= m_tokens.size()) return eof;
    return m_tokens[m_pos + 1];
}

bool Parser::at_end() const {
    return current().is(TokenType::Eof);
}

Token Parser::advance() {
    if (!at_end()) {
        return m_tokens[m_pos++];
    }
    return current();
}

bool Parser::check(TokenType type) const {
    return current().is(type);
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match_end_directive() {
    if (check(TokenType::AtSign) && peek().is(TokenType::KwEnd)) {
        advance();
        advance();
        return true;
    }
    return false;
}

Result<Token> Parser::expect(TokenType type, const std::string& message) {
    if (check(type)) {
        return Ok(advance());
    }
    return make_error<Token>(ErrorKind::ExpectedToken, message);
}

Result<AstPtr> Parser::parse_statement() {
    const auto& tok = current();
    
    switch (tok.type) {
        case TokenType::Greater:
            return parse_narrator();
            
        case TokenType::Arrow:
            return parse_jump();
            
        case TokenType::Hash:
            return parse_scene_def();
            
        case TokenType::Dot:
            return parse_label();
            
        case TokenType::Question:
            return parse_choice();
            
        case TokenType::AtSign:
            return parse_directive();
            
        case TokenType::KwIf:
            return parse_if();
            
        case TokenType::Identifier:
            if (peek().is(TokenType::Colon) || peek().is(TokenType::LeftBracket)) {
                return parse_dialogue();
            }
            return make_error<AstPtr>(ErrorKind::UnexpectedToken,
                "unexpected identifier: " + tok.value);
            
        default:
            return make_error<AstPtr>(ErrorKind::UnexpectedToken,
                "unexpected token: " + std::string(token_type_str(tok.type)));
    }
}

Result<AstPtr> Parser::parse_narrator() {
    SourceLocation loc = current().location;
    advance();  // consume '>'
    
    if (check(TokenType::Text)) {
        std::string text = current().value;
        advance();
        if (check(TokenType::Newline)) advance();
        return Ok(AstPtr(new NarratorNode(loc, std::move(text))));
    }
    
    if (check(TokenType::Newline)) {
        advance();
        return Ok(AstPtr(new NarratorNode(loc, "")));
    }
    
    return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected text after '>'");
}

Result<AstPtr> Parser::parse_dialogue() {
    SourceLocation loc = current().location;
    std::string speaker = current().value;
    advance();
    
    std::string emotion;
    if (match(TokenType::LeftBracket)) {
        if (!check(TokenType::Identifier) && !check(TokenType::Text)) {
            return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected emotion in brackets");
        }
        emotion = current().value;
        advance();
        if (!match(TokenType::RightBracket)) {
            return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected ']' after emotion");
        }
    }
    
    if (!match(TokenType::Colon)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected ':' after speaker name");
    }
    
    std::string text;
    if (check(TokenType::Text)) {
        text = current().value;
        advance();
    }
    
    if (check(TokenType::Newline)) advance();
    
    return Ok(AstPtr(new DialogueNode(loc, std::move(speaker), std::move(emotion), std::move(text))));
}

Result<AstPtr> Parser::parse_scene_def() {
    SourceLocation loc = current().location;
    advance();  // consume '#'
    
    if (!check(TokenType::Identifier)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected scene name after '#'");
    }
    std::string name = current().value;
    advance();
    
    std::string title;
    if (check(TokenType::StringLiteral)) {
        title = current().value;
        advance();
    }
    
    if (check(TokenType::Newline)) advance();
    
    return Ok(AstPtr(new SceneDefNode(loc, std::move(name), std::move(title))));
}

Result<AstPtr> Parser::parse_jump() {
    SourceLocation loc = current().location;
    advance();  // consume '->'
    
    std::string target;
    
    if (check(TokenType::Dot)) {
        advance();
        if (!check(TokenType::Identifier)) {
            return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected label name after '.'");
        }
        target = "." + current().value;
        advance();
    } else if (check(TokenType::Identifier)) {
        target = current().value;
        advance();
    } else {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected target scene name or label after '->'");
    }
    
    if (check(TokenType::Newline)) advance();
    
    return Ok(AstPtr(new JumpNode(loc, std::move(target))));
}

Result<AstPtr> Parser::parse_choice() {
    SourceLocation loc = current().location;
    advance();  // consume '?'
    
    std::string question;
    if (check(TokenType::Text)) {
        question = current().value;
        advance();
    }
    
    if (check(TokenType::Newline)) advance();
    
    auto choice = std::make_unique<ChoiceNode>(loc, std::move(question));
    
    while (check(TokenType::Minus)) {
        auto option_result = parse_choice_option();
        if (option_result.is_err()) {
            return option_result;
        }
        choice->add_option(std::move(option_result.unwrap()));
    }
    
    return Ok(AstPtr(choice.release()));
}

Result<AstPtr> Parser::parse_choice_option() {
    SourceLocation loc = current().location;
    advance();  // consume '-'
    
    if (!match(TokenType::LeftBracket)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected '[' for choice option");
    }
    
    std::string text;
    while (!at_end() && !check(TokenType::RightBracket)) {
        const auto& tok = current();
        switch (tok.type) {
            case TokenType::Text:
            case TokenType::Identifier:
            case TokenType::StringLiteral:
            case TokenType::NumberLiteral:
                if (!text.empty()) text += " ";
                text += tok.value;
                advance();
                break;
            case TokenType::LeftParen:
                text += " (";
                advance();
                break;
            case TokenType::RightParen:
                text += ")";
                advance();
                break;
            default:
                if (!text.empty()) text += " ";
                text += tok.value;
                advance();
                break;
        }
    }
    
    if (!match(TokenType::RightBracket)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected ']' after choice text");
    }
    
    if (!match(TokenType::Arrow)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected '->' after choice text");
    }
    
    std::string target;
    if (check(TokenType::Dot)) {
        advance();
        if (!check(TokenType::Identifier)) {
            return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected label name after '.'");
        }
        target = "." + current().value;
        advance();
    } else if (check(TokenType::Identifier)) {
        target = current().value;
        advance();
    } else {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected target scene name or label");
    }
    
    AstPtr condition;
    if (check(TokenType::KwIf)) {
        advance();
        auto expr_result = parse_expression();
        if (expr_result.is_err()) {
            return expr_result;
        }
        condition = std::move(expr_result.unwrap());
    }
    
    if (check(TokenType::Newline)) advance();
    
    return Ok(AstPtr(new ChoiceOptionNode(loc, std::move(text), std::move(target), std::move(condition))));
}

Result<AstPtr> Parser::parse_directive() {
    SourceLocation loc = current().location;
    advance();  // consume '@'
    
    if (!check(TokenType::Identifier)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected directive name after '@'");
    }
    
    std::string directive = current().value;
    advance();
    
    if (directive == "var") {
        return parse_var_def();
    }
    if (directive == "char") {
        return parse_char_def();
    }
    if (directive == "item") {
        return parse_item_def();
    }
    if (directive == "bg") {
        return parse_bg_command();
    }
    if (directive == "sprite") {
        return parse_sprite_command();
    }
    if (directive == "bgm") {
        return parse_bgm_command();
    }
    if (directive == "sfx") {
        return parse_sfx_command();
    }
    if (directive == "set") {
        return parse_set_command();
    }
    if (directive == "give") {
        return parse_give_command();
    }
    if (directive == "take") {
        return parse_take_command();
    }
    if (directive == "save") {
        return parse_save_command();
    }
    if (directive == "call") {
        return parse_call_command();
    }
    if (directive == "return") {
        return parse_return_command();
    }
    if (directive == "ending") {
        return parse_ending_command();
    }
    if (directive == "flag") {
        return parse_flag_command();
    }
    if (directive == "check") {
        return parse_check_command();
    }
    if (directive == "wait") {
        return parse_wait_command();
    }
    if (directive == "ui") {
        return parse_ui_command();
    }
    if (directive == "theme") {
        return parse_theme_def();
    }
    
    return make_error<AstPtr>(ErrorKind::UnexpectedToken, "unknown directive: @" + directive);
}

Result<AstPtr> Parser::parse_var_def() {
    SourceLocation loc = current().location;
    
    if (!check(TokenType::Identifier)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected variable name");
    }
    std::string name = current().value;
    advance();
    
    AstPtr init_value;
    if (match(TokenType::Equals)) {
        auto expr_result = parse_expression();
        if (expr_result.is_err()) {
            return expr_result;
        }
        init_value = std::move(expr_result.unwrap());
    }
    
    if (check(TokenType::Newline)) advance();
    
    return Ok(AstPtr(new VarDefNode(loc, std::move(name), std::move(init_value))));
}

Result<AstPtr> Parser::parse_expression() {
    return parse_comparison();
}

Result<AstPtr> Parser::parse_comparison() {
    auto left = parse_term();
    if (left.is_err()) return left;
    
    while (check(TokenType::EqualEqual) || check(TokenType::BangEqual) ||
           check(TokenType::Less) || check(TokenType::LessEqual) ||
           check(TokenType::Greater) || check(TokenType::GreaterEqual)) {
        std::string op = current().value;
        advance();
        auto right = parse_term();
        if (right.is_err()) return right;
        auto loc = left.unwrap()->location();
        auto left_expr = std::move(left.unwrap());
        auto right_expr = std::move(right.unwrap());
        left = Ok(AstPtr(new BinaryExprNode(loc, std::move(op), std::move(left_expr), std::move(right_expr))));
    }
    
    return left;
}

Result<AstPtr> Parser::parse_term() {
    auto left = parse_factor();
    if (left.is_err()) return left;
    
    while (check(TokenType::Plus) || check(TokenType::Minus)) {
        std::string op = current().value;
        advance();
        auto right = parse_factor();
        if (right.is_err()) return right;
        auto loc = left.unwrap()->location();
        auto left_expr = std::move(left.unwrap());
        auto right_expr = std::move(right.unwrap());
        left = Ok(AstPtr(new BinaryExprNode(loc, std::move(op), std::move(left_expr), std::move(right_expr))));
    }
    
    return left;
}

Result<AstPtr> Parser::parse_factor() {
    auto left = parse_unary();
    if (left.is_err()) return left;
    
    while (check(TokenType::Asterisk) || check(TokenType::Slash) || check(TokenType::Percent)) {
        std::string op = current().value;
        advance();
        auto right = parse_unary();
        if (right.is_err()) return right;
        auto loc = left.unwrap()->location();
        auto left_expr = std::move(left.unwrap());
        auto right_expr = std::move(right.unwrap());
        left = Ok(AstPtr(new BinaryExprNode(loc, std::move(op), std::move(left_expr), std::move(right_expr))));
    }
    
    return left;
}

Result<AstPtr> Parser::parse_unary() {
    if (check(TokenType::Minus) || check(TokenType::KwNot)) {
        SourceLocation loc = current().location;
        std::string op = current().value;
        if (op.empty() && check(TokenType::KwNot)) {
            op = "not";
        }
        advance();
        auto operand_result = parse_unary();
        if (operand_result.is_err()) {
            return operand_result;
        }
        return Ok(AstPtr(new UnaryExprNode(loc, std::move(op), std::move(operand_result.unwrap()))));
    }
    return parse_primary();
}

Result<AstPtr> Parser::parse_primary() {
    if (check(TokenType::NumberLiteral)) {
        double val = std::stod(current().value);
        auto node = std::make_unique<LiteralNode>(current().location, val);
        advance();
        return Ok(AstPtr(node.release()));
    }
    
    if (check(TokenType::StringLiteral)) {
        std::string val = current().value;
        auto node = std::make_unique<LiteralNode>(current().location, std::move(val));
        advance();
        return Ok(AstPtr(node.release()));
    }
    
    if (check(TokenType::KwTrue)) {
        auto node = std::make_unique<LiteralNode>(current().location, true);
        advance();
        return Ok(AstPtr(node.release()));
    }
    
    if (check(TokenType::KwFalse)) {
        auto node = std::make_unique<LiteralNode>(current().location, false);
        advance();
        return Ok(AstPtr(node.release()));
    }
    
    if (check(TokenType::Identifier)) {
        SourceLocation loc = current().location;
        std::string name = current().value;
        advance();
        
        if (match(TokenType::LeftParen)) {
            auto call = std::make_unique<CallExprNode>(loc, std::move(name));
            
            if (!check(TokenType::RightParen)) {
                auto arg_result = parse_expression();
                if (arg_result.is_err()) return arg_result;
                call->add_argument(std::move(arg_result.unwrap()));
                
                while (match(TokenType::Comma)) {
                    arg_result = parse_expression();
                    if (arg_result.is_err()) return arg_result;
                    call->add_argument(std::move(arg_result.unwrap()));
                }
            }
            
            if (!match(TokenType::RightParen)) {
                return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected ')' after function arguments");
            }
            
            return Ok(AstPtr(call.release()));
        }
        
        return Ok(AstPtr(new IdentifierNode(loc, std::move(name))));
    }
    
    if (match(TokenType::LeftParen)) {
        auto expr = parse_expression();
        if (expr.is_err()) return expr;
        if (!match(TokenType::RightParen)) {
            return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected ')' after expression");
        }
        return expr;
    }
    
    return make_error<AstPtr>(ErrorKind::UnexpectedToken, 
        "expected expression, got: " + std::string(token_type_str(current().type)));
}

Result<AstPtr> Parser::parse_if() {
    SourceLocation loc = current().location;
    advance();  // consume 'if'
    
    auto cond_result = parse_expression();
    if (cond_result.is_err()) {
        return cond_result;
    }
    
    auto branch = std::make_unique<BranchNode>(loc, std::move(cond_result.unwrap()));
    
    if (check(TokenType::Newline)) advance();
    
    while (!at_end() && !check(TokenType::KwEndif) && !check(TokenType::KwElse)) {
        while (check(TokenType::Newline)) advance();
        if (at_end() || check(TokenType::KwEndif) || check(TokenType::KwElse)) break;
        
        auto stmt = parse_statement();
        if (stmt.is_err()) {
            return stmt;
        }
        branch->add_then(std::move(stmt.unwrap()));
    }
    
    if (match(TokenType::KwElse)) {
        if (check(TokenType::Newline)) advance();
        
        while (!at_end() && !check(TokenType::KwEndif)) {
            while (check(TokenType::Newline)) advance();
            if (at_end() || check(TokenType::KwEndif)) break;
            
            auto stmt = parse_statement();
            if (stmt.is_err()) {
                return stmt;
            }
            branch->add_else(std::move(stmt.unwrap()));
        }
    }
    
    if (!match(TokenType::KwEndif)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected 'endif' to close 'if' block");
    }
    
    if (check(TokenType::Newline)) advance();
    
    return Ok(AstPtr(branch.release()));
}

Result<AstPtr> Parser::parse_char_def() {
    SourceLocation loc = current().location;
    
    if (!check(TokenType::Identifier)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected character name after '@char'");
    }
    std::string name = current().value;
    advance();
    
    auto char_def = std::make_unique<CharDefNode>(loc, std::move(name));
    
    if (check(TokenType::Newline)) advance();
    
    while (!at_end() && check(TokenType::Identifier)) {
        std::string key = current().value;
        advance();
        
        if (!match(TokenType::Colon)) {
            return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected ':' after property key");
        }
        
        std::string value = parse_property_value();
        char_def->add_property(std::move(key), std::move(value));
        
        if (check(TokenType::Newline)) advance();
    }
    
    if (!match_end_directive()) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected '@end' to close '@char' block");
    }
    
    if (check(TokenType::Newline)) advance();
    
    return Ok(AstPtr(char_def.release()));
}

Result<AstPtr> Parser::parse_item_def() {
    SourceLocation loc = current().location;
    
    if (!check(TokenType::Identifier)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected item name after '@item'");
    }
    std::string name = current().value;
    advance();
    
    auto item_def = std::make_unique<ItemDefNode>(loc, std::move(name));
    
    if (check(TokenType::Newline)) advance();
    
    while (!at_end() && check(TokenType::Identifier)) {
        std::string key = current().value;
        advance();
        
        if (!match(TokenType::Colon)) {
            return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected ':' after property key");
        }
        
        std::string value = parse_property_value();
        item_def->add_property(std::move(key), std::move(value));
        
        if (check(TokenType::Newline)) advance();
    }
    
    if (!match_end_directive()) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected '@end' to close '@item' block");
    }
    
    if (check(TokenType::Newline)) advance();
    
    return Ok(AstPtr(item_def.release()));
}

std::string Parser::parse_property_value() {
    std::string value;
    
    if (check(TokenType::StringLiteral)) {
        value = current().value;
        advance();
    } else if (check(TokenType::NumberLiteral)) {
        value = current().value;
        advance();
    } else if (check(TokenType::Text)) {
        value = current().value;
        advance();
    } else if (check(TokenType::Identifier)) {
        value = current().value;
        advance();
    }
    
    return value;
}

Result<AstPtr> Parser::parse_bg_command() {
    SourceLocation loc = current().location;
    
    if (!check(TokenType::Identifier) && !check(TokenType::StringLiteral)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected image name after '@bg'");
    }
    std::string image = current().value;
    advance();
    
    while (check(TokenType::Dot)) {
        image += ".";
        advance();
        if (check(TokenType::Identifier)) {
            image += current().value;
            advance();
        }
    }
    
    auto cmd = std::make_unique<BgCommandNode>(loc, std::move(image));
    
    while (check(TokenType::Identifier)) {
        std::string key = current().value;
        advance();
        if (match(TokenType::Colon)) {
            std::string value = parse_property_value();
            cmd->add_arg(std::move(key), std::move(value));
        }
    }
    
    if (check(TokenType::Newline)) advance();
    
    return Ok(AstPtr(cmd.release()));
}

Result<AstPtr> Parser::parse_sprite_command() {
    SourceLocation loc = current().location;
    
    if (!check(TokenType::Identifier)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected sprite name after '@sprite'");
    }
    std::string name = current().value;
    advance();
    
    auto cmd = std::make_unique<SpriteCommandNode>(loc, std::move(name));
    
    while (check(TokenType::Identifier)) {
        std::string key = current().value;
        advance();
        if (match(TokenType::Colon)) {
            std::string value = parse_property_value();
            cmd->add_arg(std::move(key), std::move(value));
        }
    }
    
    if (check(TokenType::Newline)) advance();
    
    return Ok(AstPtr(cmd.release()));
}

Result<AstPtr> Parser::parse_bgm_command() {
    SourceLocation loc = current().location;
    
    if (!check(TokenType::Identifier) && !check(TokenType::StringLiteral)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected file name after '@bgm'");
    }
    std::string file = current().value;
    advance();
    
    while (check(TokenType::Dot)) {
        file += ".";
        advance();
        if (check(TokenType::Identifier)) {
            file += current().value;
            advance();
        }
    }
    
    auto cmd = std::make_unique<BgmCommandNode>(loc, std::move(file));
    
    while (check(TokenType::Identifier)) {
        std::string key = current().value;
        advance();
        if (match(TokenType::Colon)) {
            std::string value = parse_property_value();
            cmd->add_arg(std::move(key), std::move(value));
        }
    }
    
    if (check(TokenType::Newline)) advance();
    
    return Ok(AstPtr(cmd.release()));
}

Result<AstPtr> Parser::parse_sfx_command() {
    SourceLocation loc = current().location;
    
    if (!check(TokenType::Identifier) && !check(TokenType::StringLiteral)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected file name after '@sfx'");
    }
    std::string file = current().value;
    advance();
    
    while (check(TokenType::Dot)) {
        file += ".";
        advance();
        if (check(TokenType::Identifier)) {
            file += current().value;
            advance();
        }
    }
    
    auto cmd = std::make_unique<SfxCommandNode>(loc, std::move(file));
    
    while (check(TokenType::Identifier)) {
        std::string key = current().value;
        advance();
        if (match(TokenType::Colon)) {
            std::string value = parse_property_value();
            cmd->add_arg(std::move(key), std::move(value));
        }
    }
    
    if (check(TokenType::Newline)) advance();
    
    return Ok(AstPtr(cmd.release()));
}

Result<AstPtr> Parser::parse_set_command() {
    SourceLocation loc = current().location;
    
    if (!check(TokenType::Identifier)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected variable name after '@set'");
    }
    std::string name = current().value;
    advance();
    
    if (!match(TokenType::Equals)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected '=' after variable name");
    }
    
    auto expr_result = parse_expression();
    if (expr_result.is_err()) {
        return expr_result;
    }
    
    if (check(TokenType::Newline)) advance();
    
    return Ok(AstPtr(new SetCommandNode(loc, std::move(name), std::move(expr_result.unwrap()))));
}

Result<AstPtr> Parser::parse_give_command() {
    SourceLocation loc = current().location;
    
    if (!check(TokenType::Identifier)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected item name after '@give'");
    }
    std::string item = current().value;
    advance();
    
    int count = 1;
    if (check(TokenType::NumberLiteral)) {
        count = static_cast<int>(std::stod(current().value));
        advance();
    }
    
    if (check(TokenType::Newline)) advance();
    
    return Ok(AstPtr(new GiveCommandNode(loc, std::move(item), count)));
}

Result<AstPtr> Parser::parse_take_command() {
    SourceLocation loc = current().location;
    
    if (!check(TokenType::Identifier)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected item name after '@take'");
    }
    std::string item = current().value;
    advance();
    
    int count = 1;
    if (check(TokenType::NumberLiteral)) {
        count = static_cast<int>(std::stod(current().value));
        advance();
    }
    
    if (check(TokenType::Newline)) advance();
    
    return Ok(AstPtr(new TakeCommandNode(loc, std::move(item), count)));
}

Result<AstPtr> Parser::parse_save_command() {
    SourceLocation loc = current().location;
    
    std::string label;
    if (check(TokenType::StringLiteral)) {
        label = current().value;
        advance();
    }
    
    if (check(TokenType::Newline)) advance();
    
    return Ok(AstPtr(new SaveNode(loc, std::move(label))));
}

Result<AstPtr> Parser::parse_call_command() {
    SourceLocation loc = current().location;
    
    if (!check(TokenType::Identifier)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected scene name after '@call'");
    }
    std::string target = current().value;
    advance();
    
    if (check(TokenType::Newline)) advance();
    
    return Ok(AstPtr(new CallNode(loc, std::move(target))));
}

Result<AstPtr> Parser::parse_return_command() {
    SourceLocation loc = current().location;
    
    if (check(TokenType::Newline)) advance();
    
    return Ok(AstPtr(new ReturnNode(loc)));
}

Result<AstPtr> Parser::parse_ending_command() {
    SourceLocation loc = current().location;
    
    if (!check(TokenType::Identifier)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected ending name after '@ending'");
    }
    std::string name = current().value;
    advance();
    
    if (check(TokenType::Newline)) advance();
    
    return Ok(AstPtr(new EndingNode(loc, std::move(name))));
}

Result<AstPtr> Parser::parse_flag_command() {
    SourceLocation loc = current().location;
    
    if (!check(TokenType::Identifier)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected flag name after '@flag'");
    }
    std::string name = current().value;
    advance();
    
    if (check(TokenType::Newline)) advance();
    
    return Ok(AstPtr(new FlagNode(loc, std::move(name))));
}

Result<AstPtr> Parser::parse_label() {
    SourceLocation loc = current().location;
    advance();  // consume '.'
    
    if (!check(TokenType::Identifier)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected label name after '.'");
    }
    std::string name = current().value;
    advance();
    
    if (check(TokenType::Newline)) advance();
    
    return Ok(AstPtr(new LabelNode(loc, std::move(name))));
}

Result<AstPtr> Parser::parse_check_command() {
    SourceLocation loc = current().location;
    
    auto cond_result = parse_expression();
    if (cond_result.is_err()) {
        return cond_result;
    }
    
    auto check_cmd = std::make_unique<CheckCommandNode>(loc, std::move(cond_result.unwrap()));
    
    if (check(TokenType::Newline)) advance();
    
    while (!at_end() && !check(TokenType::KwEndcheck)) {
        while (check(TokenType::Newline)) advance();
        if (at_end() || check(TokenType::KwEndcheck)) break;
        
        if (check(TokenType::KwSuccess)) {
            advance();
            if (check(TokenType::Newline)) advance();
            
            while (!at_end() && !check(TokenType::KwFail) && !check(TokenType::KwEndcheck)) {
                while (check(TokenType::Newline)) advance();
                if (at_end() || check(TokenType::KwFail) || check(TokenType::KwEndcheck)) break;
                
                auto stmt = parse_statement();
                if (stmt.is_err()) return stmt;
                check_cmd->add_success(std::move(stmt.unwrap()));
            }
        } else if (check(TokenType::KwFail)) {
            advance();
            if (check(TokenType::Newline)) advance();
            
            while (!at_end() && !check(TokenType::KwSuccess) && !check(TokenType::KwEndcheck)) {
                while (check(TokenType::Newline)) advance();
                if (at_end() || check(TokenType::KwSuccess) || check(TokenType::KwEndcheck)) break;
                
                auto stmt = parse_statement();
                if (stmt.is_err()) return stmt;
                check_cmd->add_failure(std::move(stmt.unwrap()));
            }
        } else {
            auto stmt = parse_statement();
            if (stmt.is_err()) return stmt;
            check_cmd->add_success(std::move(stmt.unwrap()));
        }
    }
    
    if (!match(TokenType::KwEndcheck)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected 'endcheck' to close '@check' block");
    }
    
    if (check(TokenType::Newline)) advance();
    
    return Ok(AstPtr(check_cmd.release()));
}

Result<AstPtr> Parser::parse_wait_command() {
    SourceLocation loc = current().location;
    
    double seconds = 0.0;
    if (check(TokenType::NumberLiteral)) {
        seconds = std::stod(current().value);
        advance();
        if (check(TokenType::Identifier)) {
            std::string unit = current().value;
            advance();
            if (unit == "ms") {
                seconds /= 1000.0;
            }
        }
    } else if (check(TokenType::Text) || check(TokenType::Identifier)) {
        std::string val = current().value;
        advance();
        if (val.size() > 0) {
            size_t pos = 0;
            while (pos < val.size() && !std::isdigit(static_cast<unsigned char>(val[pos]))) pos++;
            if (pos < val.size()) {
                seconds = std::stod(val.substr(pos));
            }
        }
    }
    
    if (check(TokenType::Newline)) advance();
    
    return Ok(AstPtr(new WaitNode(loc, seconds)));
}

Result<AstPtr> Parser::parse_ui_command() {
    SourceLocation loc = current().location;
    
    if (!check(TokenType::Identifier)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected 'show', 'hide', or 'track' after '@ui'");
    }
    
    std::string action = current().value;
    advance();
    
    if (action == "track") {
        return parse_ui_track();
    }
    
    if (action != "show" && action != "hide") {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected 'show', 'hide', or 'track' after '@ui'");
    }
    
    if (!check(TokenType::Identifier)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected UI component name after '@ui " + action + "'");
    }
    std::string target = current().value;
    advance();
    
    if (check(TokenType::Newline)) advance();
    
    auto ui_action = (action == "show") ? UiCommandNode::Action::Show : UiCommandNode::Action::Hide;
    return Ok(AstPtr(new UiCommandNode(loc, ui_action, std::move(target))));
}

Result<AstPtr> Parser::parse_ui_track() {
    SourceLocation loc = current().location;
    
    if (!check(TokenType::Identifier)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected UI track name after '@ui track'");
    }
    std::string name = current().value;
    advance();
    
    auto ui_track = std::make_unique<UiTrackNode>(loc, std::move(name));
    
    if (check(TokenType::Newline)) advance();
    
    while (!at_end() && check(TokenType::Identifier)) {
        std::string key = current().value;
        advance();
        
        if (!match(TokenType::Colon)) {
            return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected ':' after property key");
        }
        
        std::string value = parse_property_value();
        ui_track->add_property(std::move(key), std::move(value));
        
        if (check(TokenType::Newline)) advance();
    }
    
    if (!match_end_directive()) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected '@end' to close '@ui track' block");
    }
    
    if (check(TokenType::Newline)) advance();
    
    return Ok(AstPtr(ui_track.release()));
}

Result<AstPtr> Parser::parse_theme_def() {
    SourceLocation loc = current().location;
    
    if (!check(TokenType::Identifier)) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected theme name after '@theme'");
    }
    std::string name = current().value;
    advance();
    
    auto theme = std::make_unique<ThemeDefNode>(loc, std::move(name));
    
    if (check(TokenType::Newline)) advance();
    
    while (!at_end() && check(TokenType::Identifier)) {
        std::string key = current().value;
        advance();
        
        if (!match(TokenType::Colon)) {
            return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected ':' after property key");
        }
        
        std::string value = parse_property_value();
        theme->add_property(std::move(key), std::move(value));
        
        if (check(TokenType::Newline)) advance();
    }
    
    if (!match_end_directive()) {
        return make_error<AstPtr>(ErrorKind::ExpectedToken, "expected '@end' to close '@theme' block");
    }
    
    if (check(TokenType::Newline)) advance();
    
    return Ok(AstPtr(theme.release()));
}

Result<AstPtr> Parser::parse_front_matter() {
    SourceLocation loc = current().location;
    advance();  // consume first '-'
    advance();  // consume second '-'
    advance();  // consume third '-'
    
    if (check(TokenType::Newline)) advance();
    
    auto front_matter = std::make_unique<FrontMatterNode>(loc);
    
    while (!at_end()) {
        while (check(TokenType::Newline)) advance();
        if (at_end()) break;
        
        bool is_end_marker = check(TokenType::Minus) && m_pos + 2 < m_tokens.size() &&
            m_tokens[m_pos].type == TokenType::Minus &&
            m_tokens[m_pos + 1].type == TokenType::Minus &&
            m_tokens[m_pos + 2].type == TokenType::Minus;
        
        if (is_end_marker) {
            advance();
            advance();
            advance();
            if (check(TokenType::Newline)) advance();
            break;
        }
        
        if (check(TokenType::Identifier) || check(TokenType::Text)) {
            std::string key = current().value;
            advance();
            
            if (match(TokenType::Colon)) {
                std::string value = parse_property_value();
                front_matter->add_property(std::move(key), std::move(value));
            }
        } else {
            break;
        }
    }
    
    return Ok(AstPtr(front_matter.release()));
}

std::string Parser::parse_text_content() {
    std::string result;
    while (check(TokenType::Text) || check(TokenType::Identifier) || check(TokenType::StringLiteral)) {
        result += current().value;
        advance();
    }
    return result;
}

std::unique_ptr<InterpolatedTextNode> Parser::parse_interpolated_text(const std::string& text) {
    auto result = std::make_unique<InterpolatedTextNode>(SourceLocation::unknown());
    
    size_t pos = 0;
    std::string plain;
    
    while (pos < text.size()) {
        if (pos + 1 < text.size() && text[pos] == '{' && text[pos + 1] == '{') {
            if (!plain.empty()) {
                result->add_plain_text(std::move(plain));
                plain.clear();
            }
            
            pos += 2;
            size_t end = text.find("}}", pos);
            if (end != std::string::npos) {
                std::string var_name = text.substr(pos, end - pos);
                result->add_interpolation(var_name);
                pos = end + 2;
            } else {
                plain += "{{";
            }
        }
        else if (text[pos] == '{') {
            size_t colon_pos = text.find(':', pos);
            if (colon_pos != std::string::npos && colon_pos > pos + 1) {
                if (!plain.empty()) {
                    result->add_plain_text(std::move(plain));
                    plain.clear();
                }
                
                std::string style = text.substr(pos + 1, colon_pos - pos - 1);
                size_t end = text.find('}', colon_pos);
                if (end != std::string::npos) {
                    std::string content = text.substr(colon_pos + 1, end - colon_pos - 1);
                    result->add_inline_style(std::move(style), std::move(content));
                    pos = end + 1;
                } else {
                    plain += text[pos];
                    pos++;
                }
            } else {
                plain += text[pos];
                pos++;
            }
        }
        else {
            plain += text[pos];
            pos++;
        }
    }
    
    if (!plain.empty()) {
        result->add_plain_text(std::move(plain));
    }
    
    return result;
}

} // namespace nova
