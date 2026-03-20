#pragma once

#include "nova/lexer/token.h"
#include "nova/core/result.h"
#include <string>
#include <vector>

namespace nova {

class Lexer {
public:
    explicit Lexer(std::string source, std::string filename = "<stdin>");
    
    Result<std::vector<Token>> tokenize();
    Result<Token> next_token();
    bool at_end() const;

private:
    std::string m_source;
    std::string m_filename;
    size_t m_pos = 0;
    int m_line = 1;
    int m_column = 1;
    bool m_scan_text_next = false;
    bool m_line_is_directive = false;

    bool is_first_non_whitespace_on_line() const;
    
    char current() const;
    char peek() const;
    char advance();
    bool match(char expected);
    
    SourceLocation here() const;
    
    void skip_whitespace();
    void skip_comment();
    
    Result<Token> scan_identifier();
    Result<Token> scan_string();
    Result<Token> scan_number();
    Result<Token> scan_operator();
    Result<Token> scan_text_line(SourceLocation loc);
    
    template<typename T>
    Result<T> make_error(ErrorKind kind, std::string message) {
        return Err<T>(kind, std::move(message), here());
    }
};

} // namespace nova
