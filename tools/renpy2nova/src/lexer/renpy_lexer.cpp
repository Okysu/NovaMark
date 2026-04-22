#include "renpy2nova/lexer/renpy_lexer.h"

#include <cctype>
#include <string>
#include <utility>
#include <vector>

namespace nova::renpy2nova {
namespace {

bool starts_with(const std::string& value, const std::string& prefix) {
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

bool ends_with(const std::string& value, const std::string& suffix) {
    return value.size() >= suffix.size()
        && value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string trim(const std::string& value) {
    size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0) {
        ++begin;
    }

    size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }

    return value.substr(begin, end - begin);
}

size_t count_indent(const std::string& line) {
    size_t indent = 0;
    for (char ch : line) {
        if (ch == ' ') {
            ++indent;
        } else if (ch == '\t') {
            indent += 4;
        } else {
            break;
        }
    }
    return indent;
}

std::vector<std::string> split_lines(const std::string& source) {
    std::vector<std::string> lines;
    std::string current;

    for (char ch : source) {
        if (ch == '\r') {
            continue;
        }

        if (ch == '\n') {
            lines.push_back(current);
            current.clear();
            continue;
        }

        current.push_back(ch);
    }

    if (!current.empty()) {
        lines.push_back(current);
    }

    return lines;
}

bool parse_quoted_literal(const std::string& input, size_t start, std::string& text, size_t& end) {
    if (start >= input.size()) {
        return false;
    }

    const char quote = input[start];
    if (quote != '\'' && quote != '"') {
        return false;
    }

    for (size_t index = start + 1; index < input.size(); ++index) {
        if (input[index] == quote && input[index - 1] != '\\') {
            text = input.substr(start + 1, index - start - 1);
            end = index + 1;
            return true;
        }
    }

    return false;
}

bool parse_menu_choice(const std::string& trimmed, std::string& text, std::string& condition) {
    size_t quoted_end = 0;
    if (!parse_quoted_literal(trimmed, 0, text, quoted_end)) {
        return false;
    }

    const std::string remainder = trim(trimmed.substr(quoted_end));
    if (remainder == ":") {
        condition.clear();
        return true;
    }

    if (starts_with(remainder, "if ") && ends_with(remainder, ":")) {
        condition = trim(remainder.substr(3, remainder.size() - 4));
        return !condition.empty();
    }

    return false;
}

RenpyToken make_basic_token(
    RenpyTokenType type,
    const std::string& line,
    size_t line_number,
    size_t column,
    size_t indent,
    std::string primary = {},
    std::string secondary = {},
    RenpyUnsupportedKind unsupported_kind = RenpyUnsupportedKind::None) {
    RenpyToken token;
    token.type = type;
    token.lexeme = line;
    token.primary = std::move(primary);
    token.secondary = std::move(secondary);
    token.unsupported_kind = unsupported_kind;
    token.line = line_number;
    token.column = column;
    token.indent = indent;
    return token;
}

RenpyToken classify_line(const std::string& line, size_t line_number) {
    const size_t indent = count_indent(line);
    const size_t column = indent + 1;
    const std::string trimmed = trim(line);

    if (trimmed.empty()) {
        return make_basic_token(RenpyTokenType::Blank, line, line_number, column, indent);
    }

    if (starts_with(trimmed, "#")) {
        return make_basic_token(RenpyTokenType::Comment, line, line_number, column, indent, trimmed.substr(1));
    }

    if (starts_with(trimmed, "define ")) {
        const std::string definition = trim(trimmed.substr(7));
        const size_t equals_pos = definition.find('=');
        if (equals_pos != std::string::npos) {
            const std::string name = trim(definition.substr(0, equals_pos));
            const std::string value = trim(definition.substr(equals_pos + 1));
            if (!name.empty() && starts_with(value, "Character(")) {
                return make_basic_token(
                    RenpyTokenType::DefineCharacter,
                    line,
                    line_number,
                    column,
                    indent,
                    name,
                    value);
            }
        }
    }

    if (starts_with(trimmed, "default ")) {
        const std::string definition = trim(trimmed.substr(8));
        const size_t equals_pos = definition.find('=');
        if (equals_pos != std::string::npos) {
            const std::string name = trim(definition.substr(0, equals_pos));
            const std::string value = trim(definition.substr(equals_pos + 1));
            return make_basic_token(RenpyTokenType::DefaultStatement, line, line_number, column, indent, name, value);
        }
    }

    if (starts_with(trimmed, "if ") && ends_with(trimmed, ":")) {
        return make_basic_token(
            RenpyTokenType::IfStatement,
            line,
            line_number,
            column,
            indent,
            trim(trimmed.substr(3, trimmed.size() - 4)));
    }

    if (starts_with(trimmed, "elif ") && ends_with(trimmed, ":")) {
        return make_basic_token(
            RenpyTokenType::ElifStatement,
            line,
            line_number,
            column,
            indent,
            trim(trimmed.substr(5, trimmed.size() - 6)));
    }

    if (trimmed == "else:") {
        return make_basic_token(RenpyTokenType::ElseStatement, line, line_number, column, indent, "else");
    }

    if (starts_with(trimmed, "label ") && ends_with(trimmed, ":")) {
        return make_basic_token(
            RenpyTokenType::Label,
            line,
            line_number,
            column,
            indent,
            trim(trimmed.substr(6, trimmed.size() - 7)));
    }

    if (trimmed == "menu:") {
        return make_basic_token(RenpyTokenType::Menu, line, line_number, column, indent);
    }

    if (starts_with(trimmed, "menu ") && ends_with(trimmed, ":")) {
        const std::string payload = trim(trimmed.substr(4, trimmed.size() - 5));
        std::string prompt;
        size_t prompt_end = 0;
        if (parse_quoted_literal(payload, 0, prompt, prompt_end) && trim(payload.substr(prompt_end)).empty()) {
            return make_basic_token(RenpyTokenType::Menu, line, line_number, column, indent, prompt);
        }

        return make_basic_token(RenpyTokenType::Menu, line, line_number, column, indent, payload);
    }

    if (trimmed == "python:") {
        return make_basic_token(RenpyTokenType::PythonBlockStart, line, line_number, column, indent, "python");
    }

    if (trimmed == "init python:") {
        return make_basic_token(RenpyTokenType::InitPythonBlockStart, line, line_number, column, indent, "init python");
    }

    if (starts_with(trimmed, "screen ") && ends_with(trimmed, ":")) {
        return make_basic_token(
            RenpyTokenType::UnsupportedConstruct,
            line,
            line_number,
            column,
            indent,
            "screen",
            trim(trimmed.substr(7, trimmed.size() - 8)),
            RenpyUnsupportedKind::Screen);
    }

    if (starts_with(trimmed, "transform ") && ends_with(trimmed, ":")) {
        return make_basic_token(
            RenpyTokenType::UnsupportedConstruct,
            line,
            line_number,
            column,
            indent,
            "transform",
            trim(trimmed.substr(10, trimmed.size() - 11)),
            RenpyUnsupportedKind::Transform);
    }

    if (starts_with(trimmed, "with ")) {
        return make_basic_token(
            RenpyTokenType::WithStatement,
            line,
            line_number,
            column,
            indent,
            trim(trimmed.substr(5)),
            {},
            RenpyUnsupportedKind::With);
    }

    if (starts_with(trimmed, "image ")) {
        return make_basic_token(
            RenpyTokenType::UnsupportedConstruct,
            line,
            line_number,
            column,
            indent,
            "image",
            trim(trimmed.substr(6)),
            RenpyUnsupportedKind::Image);
    }

    if (starts_with(trimmed, "jump ")) {
        return make_basic_token(RenpyTokenType::Jump, line, line_number, column, indent, trim(trimmed.substr(5)));
    }

    if (starts_with(trimmed, "call ")) {
        return make_basic_token(RenpyTokenType::Call, line, line_number, column, indent, trim(trimmed.substr(5)));
    }

    if (trimmed == "return") {
        return make_basic_token(RenpyTokenType::Return, line, line_number, column, indent);
    }

    if (starts_with(trimmed, "return ")) {
        return make_basic_token(RenpyTokenType::Return, line, line_number, column, indent, trim(trimmed.substr(7)));
    }

    if (starts_with(trimmed, "scene ")) {
        return make_basic_token(RenpyTokenType::Scene, line, line_number, column, indent, trim(trimmed.substr(6)));
    }

    if (starts_with(trimmed, "show ")) {
        return make_basic_token(RenpyTokenType::Show, line, line_number, column, indent, trim(trimmed.substr(5)));
    }

    if (starts_with(trimmed, "hide ")) {
        return make_basic_token(RenpyTokenType::Hide, line, line_number, column, indent, trim(trimmed.substr(5)));
    }

    if (starts_with(trimmed, "play ")) {
        const std::string payload = trim(trimmed.substr(5));
        if (starts_with(payload, "music ")) {
            return make_basic_token(RenpyTokenType::PlayMusic, line, line_number, column, indent, trim(payload.substr(6)));
        }

        if (starts_with(payload, "sound ")) {
            return make_basic_token(RenpyTokenType::PlaySound, line, line_number, column, indent, trim(payload.substr(6)));
        }

        return make_basic_token(
            RenpyTokenType::UnsupportedConstruct,
            line,
            line_number,
            column,
            indent,
            "audio",
            payload,
            RenpyUnsupportedKind::Audio);
    }

    if (!trimmed.empty() && trimmed[0] == '$') {
        return make_basic_token(RenpyTokenType::DollarStatement, line, line_number, column, indent, trim(trimmed.substr(1)));
    }

    std::string quoted_text;
    std::string choice_condition;
    if (parse_menu_choice(trimmed, quoted_text, choice_condition)) {
        return make_basic_token(RenpyTokenType::MenuChoice, line, line_number, column, indent, quoted_text, choice_condition);
    }

    size_t quoted_end = 0;
    if (parse_quoted_literal(trimmed, 0, quoted_text, quoted_end)) {
        if (trim(trimmed.substr(quoted_end)).empty()) {
            return make_basic_token(RenpyTokenType::Narrator, line, line_number, column, indent, quoted_text);
        }
    }

    const size_t first_quote_pos = trimmed.find('"');
    if (first_quote_pos != std::string::npos && first_quote_pos > 0) {
        const std::string speaker = trim(trimmed.substr(0, first_quote_pos));
        std::string text;
        size_t end = 0;
        if (!speaker.empty() && parse_quoted_literal(trimmed, first_quote_pos, text, end) && trim(trimmed.substr(end)).empty()) {
            return make_basic_token(RenpyTokenType::Say, line, line_number, column, indent, speaker, text);
        }
    }

    const size_t first_single_quote_pos = trimmed.find('\'');
    if (first_single_quote_pos != std::string::npos && first_single_quote_pos > 0) {
        const std::string speaker = trim(trimmed.substr(0, first_single_quote_pos));
        std::string text;
        size_t end = 0;
        if (!speaker.empty() && parse_quoted_literal(trimmed, first_single_quote_pos, text, end) && trim(trimmed.substr(end)).empty()) {
            return make_basic_token(RenpyTokenType::Say, line, line_number, column, indent, speaker, text);
        }
    }

    return make_basic_token(
        RenpyTokenType::Unknown,
        line,
        line_number,
        column,
        indent,
        trimmed,
        {},
        RenpyUnsupportedKind::Unknown);
}

} // namespace

ConversionResult<std::vector<RenpyToken>> RenpyLexer::tokenize(const std::string& source) const {
    std::vector<RenpyToken> tokens;
    const std::vector<std::string> lines = split_lines(source);
    tokens.reserve(lines.size() + 1);

    size_t line_number = 1;
    for (const std::string& line : lines) {
        tokens.push_back(classify_line(line, line_number));
        ++line_number;
    }

    RenpyToken eof_token;
    eof_token.type = RenpyTokenType::Eof;
    eof_token.line = line_number;
    eof_token.column = 1;
    tokens.push_back(std::move(eof_token));

    return ConversionResult<std::vector<RenpyToken>>(std::move(tokens));
}

} // namespace nova::renpy2nova
