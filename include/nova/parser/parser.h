#pragma once

#include "nova/lexer/lexer.h"
#include "nova/ast/ast_node.h"
#include "nova/core/result.h"
#include <memory>

namespace nova {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    
    Result<AstPtr> parse();
    
private:
    std::vector<Token> m_tokens;
    size_t m_pos = 0;
    
    const Token& current() const;
    const Token& peek() const;
    bool at_end() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match_end_directive();  // 匹配 @end 指令
    Result<Token> expect(TokenType type, const std::string& message);
    
    Result<AstPtr> parse_statement();
    Result<AstPtr> parse_dialogue();
    Result<AstPtr> parse_narrator();
    Result<AstPtr> parse_scene_def();
    Result<AstPtr> parse_choice();
    Result<AstPtr> parse_choice_option();
    Result<AstPtr> parse_if();
    Result<AstPtr> parse_char_def();
    Result<AstPtr> parse_item_def();
    Result<AstPtr> parse_directive();
    Result<AstPtr> parse_var_def();
    Result<AstPtr> parse_jump();
    Result<AstPtr> parse_bg_command();
    Result<AstPtr> parse_sprite_command();
    Result<AstPtr> parse_bgm_command();
    Result<AstPtr> parse_sfx_command();
    Result<AstPtr> parse_set_command();
    Result<AstPtr> parse_give_command();
    Result<AstPtr> parse_take_command();
    Result<AstPtr> parse_save_command();
    Result<AstPtr> parse_call_command();
    Result<AstPtr> parse_return_command();
    Result<AstPtr> parse_ending_command();
    Result<AstPtr> parse_flag_command();
    Result<AstPtr> parse_label();
    Result<AstPtr> parse_check_command();
    Result<AstPtr> parse_wait_command();
    Result<AstPtr> parse_theme_def();
    Result<AstPtr> parse_front_matter();
    Result<AstPtr> parse_property_block();
    std::string parse_property_value();
    std::string parse_text_content();
    std::unique_ptr<InterpolatedTextNode> parse_interpolated_text(const std::string& text);
    
    Result<AstPtr> parse_expression();
    Result<AstPtr> parse_comparison();
    Result<AstPtr> parse_term();
    Result<AstPtr> parse_factor();
    Result<AstPtr> parse_unary();
    Result<AstPtr> parse_primary();
    
    template<typename T>
    Result<T> make_error(ErrorKind kind, std::string message) {
        return Err<T>(kind, std::move(message), current().location);
    }
};

} // namespace nova
