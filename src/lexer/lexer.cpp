#include "nova/lexer/lexer.h"
#include <cctype>

namespace nova {

bool Lexer::is_first_non_whitespace_on_line() const {
    if (m_pos == 0) {
        return true;
    }

    size_t i = m_pos;
    while (i > 0) {
        --i;
        char c = m_source[i];
        if (c == '\n') {
            return true;
        }
        if (c != ' ' && c != '\t' && c != '\r') {
            return false;
        }
    }

    return true;
}

/// @brief 检查字节是否为 UTF-8 多字节序列的首字节
static bool is_utf8_start(unsigned char c) {
    return (c & 0xC0) != 0x80 && c >= 0x80;
}

/// @brief 检查字节是否为 UTF-8 continuation byte
static bool is_utf8_continuation(unsigned char c) {
    return (c & 0xC0) == 0x80;
}

/// @brief 检查字节是否可以作为标识符首字符
static bool is_ident_start_char(unsigned char c) {
    return std::isalpha(c) || c == '_' || is_utf8_start(c);
}

/// @brief 检查字节是否可以作为标识符字符
static bool is_ident_char(unsigned char c) {
    return std::isalnum(c) || c == '_' || is_utf8_start(c) || is_utf8_continuation(c);
}

Lexer::Lexer(std::string source, std::string filename)
    : m_source(std::move(source))
    , m_filename(std::move(filename))
{
}

Result<std::vector<Token>> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (true) {
        auto result = next_token();
        if (result.is_err()) {
            return Result<std::vector<Token>>(result.error());
        }
        Token token = result.unwrap();
        tokens.push_back(token);
        if (token.is(TokenType::Eof)) {
            break;
        }
    }
    
    return Ok(std::move(tokens));
}

Result<Token> Lexer::next_token() {
    if (m_scan_text_next) {
        m_scan_text_next = false;
        SourceLocation loc = here();
        std::string value;
        skip_whitespace();
        while (!at_end() && current() != '\n') {
            value += advance();
        }
        while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r')) {
            value.pop_back();
        }
        return Ok(Token{TokenType::Text, value, loc});
    }
    
    skip_whitespace();
    
    if (at_end()) {
        return Ok(Token{TokenType::Eof, "", here()});
    }
    
    char c = current();
    
    if (c == '\n') {
        advance();
        m_line_is_directive = false;
        return Ok(Token{TokenType::Newline, "\n", here()});
    }

    if (m_column == 1) {
        m_line_is_directive = false;
    }
    
    if (c == '/' && peek() == '/') {
        skip_comment();
        return next_token();
    }
    
    if (c == '"' || c == '\'') {
        return scan_string();
    }
    
    if (c == '>') {
        SourceLocation loc = here();
        bool was_first_non_ws = is_first_non_whitespace_on_line();
        advance();
        if (current() == '=') {
            advance();
            return Ok(Token{TokenType::GreaterEqual, ">=", loc});
        }
        if (!was_first_non_ws) {
            return Ok(Token{TokenType::Greater, ">", loc});
        }
        if (at_end() || current() == '\n') {
            return Ok(Token{TokenType::Greater, ">", loc});
        }
        m_scan_text_next = true;
        return Ok(Token{TokenType::Greater, ">", loc});
    }
    
    if (c == '?') {
        SourceLocation loc = here();
        advance();
        if (!at_end() && current() != '\n') {
            m_scan_text_next = true;
        }
        return Ok(Token{TokenType::Question, "?", loc});
    }
    
    if (std::isdigit(static_cast<unsigned char>(c))) {
        return scan_number();
    }
    
    if (is_ident_start_char(static_cast<unsigned char>(c))) {
        return scan_identifier();
    }
    
    return scan_operator();
}

bool Lexer::at_end() const {
    return m_pos >= m_source.size();
}

char Lexer::current() const {
    return at_end() ? '\0' : m_source[m_pos];
}

char Lexer::peek() const {
    return (m_pos + 1 >= m_source.size()) ? '\0' : m_source[m_pos + 1];
}

char Lexer::advance() {
    char c = current();
    ++m_pos;
    if (c == '\n') {
        ++m_line;
        m_column = 1;
    } else {
        ++m_column;
    }
    return c;
}

bool Lexer::match(char expected) {
    if (at_end() || current() != expected) {
        return false;
    }
    advance();
    return true;
}

SourceLocation Lexer::here() const {
    return {m_filename, m_line, m_column};
}

void Lexer::skip_whitespace() {
    while (!at_end() && current() != '\n' && std::isspace(static_cast<unsigned char>(current()))) {
        advance();
    }
}

void Lexer::skip_comment() {
    while (!at_end() && current() != '\n') {
        advance();
    }
}

Result<Token> Lexer::scan_identifier() {
    SourceLocation loc = here();
    std::string value;
    
    while (!at_end() && is_ident_char(static_cast<unsigned char>(current()))) {
        value += advance();
    }
    
    auto& keywords = get_keywords();
    auto it = keywords.find(value);
    if (it != keywords.end()) {
        return Ok(Token{it->second, value, loc});
    }
    
    return Ok(Token{TokenType::Identifier, value, loc});
}

Result<Token> Lexer::scan_string() {
    SourceLocation loc = here();
    char quote = advance();
    std::string value;
    
    while (!at_end() && current() != quote) {
        if (current() == '\n') {
            return make_error<Token>(ErrorKind::UnterminatedString, "unterminated string");
        }
        if (current() == '\\') {
            advance();
            if (at_end()) {
                return make_error<Token>(ErrorKind::UnterminatedString, "unterminated string");
            }
            char escaped = advance();
            switch (escaped) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                case '\\': value += '\\'; break;
                case '"': value += '"'; break;
                case '\'': value += '\''; break;
                default:
                    return make_error<Token>(ErrorKind::InvalidEscape, "invalid escape sequence");
            }
        } else {
            value += advance();
        }
    }
    
    if (at_end()) {
        return make_error<Token>(ErrorKind::UnterminatedString, "unterminated string");
    }
    
    advance();
    return Ok(Token{TokenType::StringLiteral, value, loc});
}

Result<Token> Lexer::scan_number() {
    SourceLocation loc = here();
    std::string value;
    
    while (!at_end() && std::isdigit(static_cast<unsigned char>(current()))) {
        value += advance();
    }
    
    if (current() == '.' && std::isdigit(static_cast<unsigned char>(peek()))) {
        value += advance();
        while (!at_end() && std::isdigit(static_cast<unsigned char>(current()))) {
            value += advance();
        }
    }
    
    return Ok(Token{TokenType::NumberLiteral, value, loc});
}

Result<Token> Lexer::scan_text_line(SourceLocation loc) {
    std::string value;
    skip_whitespace();
    while (!at_end() && current() != '\n') {
        value += advance();
    }
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r')) {
        value.pop_back();
    }
    return Ok(Token{TokenType::Text, value, loc});
}

Result<Token> Lexer::scan_operator() {
    SourceLocation loc = here();
    char c = advance();
    
    switch (c) {
        case '@':
            m_line_is_directive = true;
            return Ok(Token{TokenType::AtSign, "@", loc});
        case '#':
            return Ok(Token{TokenType::Hash, "#", loc});
        case ':':
            if (!m_line_is_directive && !at_end() && current() != '\n') {
                m_scan_text_next = true;
            }
            return Ok(Token{TokenType::Colon, ":", loc});
        case '?': return Ok(Token{TokenType::Question, "?", loc});
        case '(': return Ok(Token{TokenType::LeftParen, "(", loc});
        case ')': return Ok(Token{TokenType::RightParen, ")", loc});
        case '[': return Ok(Token{TokenType::LeftBracket, "[", loc});
        case ']': return Ok(Token{TokenType::RightBracket, "]", loc});
        case '{': return Ok(Token{TokenType::LeftBrace, "{", loc});
        case '}': return Ok(Token{TokenType::RightBrace, "}", loc});
        case ',': return Ok(Token{TokenType::Comma, ",", loc});
        case '.': return Ok(Token{TokenType::Dot, ".", loc});
        case '+': return Ok(Token{TokenType::Plus, "+", loc});
        case '*': return Ok(Token{TokenType::Asterisk, "*", loc});
        case '%': return Ok(Token{TokenType::Percent, "%", loc});
        
        case '-':
            if (match('>')) return Ok(Token{TokenType::Arrow, "->", loc});
            return Ok(Token{TokenType::Minus, "-", loc});
        
        case '=':
            if (match('=')) return Ok(Token{TokenType::EqualEqual, "==", loc});
            return Ok(Token{TokenType::Equals, "=", loc});
        
        case '!':
            if (match('=')) return Ok(Token{TokenType::BangEqual, "!=", loc});
            return Ok(Token{TokenType::Text, "!", loc});
        
        case '<':
            if (match('=')) return Ok(Token{TokenType::LessEqual, "<=", loc});
            return Ok(Token{TokenType::Less, "<", loc});
        
        case '>':
            if (match('=')) return Ok(Token{TokenType::GreaterEqual, ">=", loc});
            return Ok(Token{TokenType::Greater, ">", loc});
        
        case '/':
            return Ok(Token{TokenType::Slash, "/", loc});
        
        default:
            return make_error<Token>(ErrorKind::UnexpectedChar, 
                std::string("unexpected character: '") + c + "'");
    }
}

} // namespace nova
