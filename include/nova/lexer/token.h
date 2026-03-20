#pragma once

#include "nova/core/source_location.h"
#include <string>
#include <unordered_map>

namespace nova {

enum class TokenType {
    Identifier,
    StringLiteral,
    NumberLiteral,
    Text,
    
    KwIf,
    KwEndif,
    KwElse,
    KwEnd,
    KwTrue,
    KwFalse,
    KwAnd,
    KwOr,
    KwNot,
    
    AtSign,
    Hash,
    Colon,
    Arrow,
    Equals,
    Comma,
    Dot,
    Greater,
    Question,
    
    LeftParen,
    RightParen,
    LeftBracket,
    RightBracket,
    LeftBrace,
    RightBrace,
    
    Plus,
    Minus,
    Asterisk,
    Slash,
    Percent,
    
    EqualEqual,
    BangEqual,
    Less,
    LessEqual,
    GreaterEqual,

    Newline,
    Eof,
};

inline const char* token_type_str(TokenType type) {
    switch (type) {
        case TokenType::Identifier:     return "Identifier";
        case TokenType::StringLiteral:  return "StringLiteral";
        case TokenType::NumberLiteral:  return "NumberLiteral";
        case TokenType::Text:           return "Text";
        case TokenType::KwIf:           return "if";
        case TokenType::KwEndif:        return "endif";
        case TokenType::KwElse:         return "else";
        case TokenType::KwEnd:          return "end";
        case TokenType::KwTrue:         return "true";
        case TokenType::KwFalse:        return "false";
        case TokenType::KwAnd:          return "and";
        case TokenType::KwOr:           return "or";
        case TokenType::KwNot:          return "not";
        case TokenType::AtSign:         return "@";
        case TokenType::Hash:           return "#";
        case TokenType::Colon:          return ":";
        case TokenType::Arrow:          return "->";
        case TokenType::Equals:         return "=";
        case TokenType::Comma:          return ",";
        case TokenType::Dot:            return ".";
        case TokenType::Greater:        return ">";
        case TokenType::Question:       return "?";
        case TokenType::LeftParen:      return "(";
        case TokenType::RightParen:     return ")";
        case TokenType::LeftBracket:    return "[";
        case TokenType::RightBracket:   return "]";
        case TokenType::LeftBrace:      return "{";
        case TokenType::RightBrace:     return "}";
        case TokenType::Plus:           return "+";
        case TokenType::Minus:          return "-";
        case TokenType::Asterisk:       return "*";
        case TokenType::Slash:          return "/";
        case TokenType::Percent:        return "%";
        case TokenType::EqualEqual:     return "==";
        case TokenType::BangEqual:      return "!=";
        case TokenType::Less:           return "<";
        case TokenType::LessEqual:      return "<=";
        case TokenType::GreaterEqual:   return ">=";
        case TokenType::Newline:        return "Newline";
        case TokenType::Eof:            return "Eof";
    }
    return "Unknown";
}

struct Token {
    TokenType type = TokenType::Eof;
    std::string value;
    SourceLocation location;
    
    bool is(TokenType t) const { return type == t; }
    std::string to_string() const;
};

inline const std::unordered_map<std::string, TokenType>& get_keywords() {
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"if", TokenType::KwIf},
        {"endif", TokenType::KwEndif},
        {"else", TokenType::KwElse},
        {"end", TokenType::KwEnd},
        {"true", TokenType::KwTrue},
        {"false", TokenType::KwFalse},
        {"and", TokenType::KwAnd},
        {"or", TokenType::KwOr},
        {"not", TokenType::KwNot},
    };
    return keywords;
}

} // namespace nova
