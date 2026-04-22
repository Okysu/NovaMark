#include "renpy2nova/emitter/novamark_emitter.h"

#include <cctype>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace nova::renpy2nova {
namespace {

constexpr size_t INDENT_WIDTH = 4;

bool parse_quoted_literal(const std::string& input, size_t start, std::string& text, size_t& end);
bool starts_with(const std::string& value, const std::string& prefix);
std::string trim(const std::string& value);
bool parse_audio_payload(const std::string& payload, std::string& path, std::string& extras);
std::optional<std::string> map_simple_transition_name(const std::string& value);

std::string indent(size_t depth) {
    return std::string(depth * INDENT_WIDTH, ' ');
}

std::string reserved_none_sentinel_literal() {
    return "\"" + std::string(RESERVED_NONE_SENTINEL) + "\"";
}

std::string escape_string(const std::string& text) {
    std::string escaped;
    escaped.reserve(text.size());

    for (char ch : text) {
        switch (ch) {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped.push_back(ch);
            break;
        }
    }

    return escaped;
}

std::string extract_character_display_name(const std::string& value) {
    const size_t open_paren = value.find('(');
    const size_t first_quote = value.find('"', open_paren == std::string::npos ? 0 : open_paren);
    if (first_quote == std::string::npos) {
        return {};
    }

    size_t end_quote = first_quote + 1;
    while (end_quote < value.size()) {
        if (value[end_quote] == '"' && value[end_quote - 1] != '\\') {
            return value.substr(first_quote + 1, end_quote - first_quote - 1);
        }
        ++end_quote;
    }

    return {};
}

std::optional<std::string> extract_call_argument(const std::string& value, const std::string& key) {
    const std::string pattern = key + "=";
    const size_t key_pos = value.find(pattern);
    if (key_pos == std::string::npos) {
        return std::nullopt;
    }

    size_t cursor = key_pos + pattern.size();
    while (cursor < value.size() && std::isspace(static_cast<unsigned char>(value[cursor])) != 0) {
        ++cursor;
    }

    std::string parsed;
    size_t end = 0;
    if (parse_quoted_literal(value, cursor, parsed, end)) {
        return parsed;
    }

    size_t finish = cursor;
    while (finish < value.size() && value[finish] != ',' && value[finish] != ')') {
        ++finish;
    }

    const std::string raw = trim(value.substr(cursor, finish - cursor));
    if (raw.empty()) {
        return std::nullopt;
    }

    return raw;
}

std::string normalize_literal_booleans_and_none_sentinel(const std::string& value) {
    std::string result;
    result.reserve(value.size());

    bool in_single = false;
    bool in_double = false;
    for (size_t index = 0; index < value.size();) {
        const char ch = value[index];
        if (ch == '\\' && index + 1 < value.size()) {
            result.push_back(ch);
            result.push_back(value[index + 1]);
            index += 2;
            continue;
        }

        if (!in_double && ch == '\'') {
            in_single = !in_single;
            result.push_back(ch);
            ++index;
            continue;
        }

        if (!in_single && ch == '"') {
            in_double = !in_double;
            result.push_back(ch);
            ++index;
            continue;
        }

        if (!in_single && !in_double && std::isalpha(static_cast<unsigned char>(ch)) != 0) {
            size_t end = index + 1;
            while (end < value.size() && (std::isalnum(static_cast<unsigned char>(value[end])) != 0 || value[end] == '_')) {
                ++end;
            }

            const std::string word = value.substr(index, end - index);
            if (word == "True") {
                result += "true";
            } else if (word == "False") {
                result += "false";
            } else if (word == "None") {
                result += reserved_none_sentinel_literal();
            } else {
                result += word;
            }
            index = end;
            continue;
        }

        result.push_back(ch);
        ++index;
    }

    return result;
}

struct CharacterMetadata {
    std::string display_name;
    std::vector<std::pair<std::string, std::string>> properties;
};

CharacterMetadata extract_character_metadata(const std::string& value) {
    CharacterMetadata metadata;
    metadata.display_name = extract_character_display_name(value);

    if (const auto color = extract_call_argument(value, "color"); color.has_value()) {
        const std::string normalized = trim(*color);
        if (!normalized.empty() && !starts_with(normalized, "#") && normalized.size() == 6) {
            bool valid_hex = true;
            for (char ch : normalized) {
                if (std::isxdigit(static_cast<unsigned char>(ch)) == 0) {
                    valid_hex = false;
                    break;
                }
            }
            if (valid_hex) {
                metadata.properties.emplace_back("color", "#" + normalized);
            }
        } else if (!normalized.empty() && normalized[0] == '#') {
            metadata.properties.emplace_back("color", normalized);
        }
    }

    if (const auto image = extract_call_argument(value, "image"); image.has_value()) {
        const std::string normalized = trim(*image);
        if (!normalized.empty() && normalized.find_first_of(" \t") == std::string::npos) {
            metadata.properties.emplace_back("sprite_default", normalized + ".png");
        }
    }

    return metadata;
}

std::string scene_anchor_name(const std::string& label_name) {
    return "scene_" + label_name;
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

bool starts_with(const std::string& value, const std::string& prefix) {
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
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

bool split_first_word(const std::string& value, std::string& head, std::string& tail) {
    const std::string trimmed = trim(value);
    if (trimmed.empty()) {
        head.clear();
        tail.clear();
        return false;
    }

    const size_t separator = trimmed.find_first_of(" \t");
    if (separator == std::string::npos) {
        head = trimmed;
        tail.clear();
        return true;
    }

    head = trimmed.substr(0, separator);
    tail = trim(trimmed.substr(separator + 1));
    return true;
}

std::vector<std::string> split_words(const std::string& value) {
    std::vector<std::string> words;
    const std::string trimmed = trim(value);
    if (trimmed.empty()) {
        return words;
    }

    size_t cursor = 0;
    while (cursor < trimmed.size()) {
        while (cursor < trimmed.size() && std::isspace(static_cast<unsigned char>(trimmed[cursor])) != 0) {
            ++cursor;
        }

        if (cursor >= trimmed.size()) {
            break;
        }

        size_t end = cursor;
        while (end < trimmed.size() && std::isspace(static_cast<unsigned char>(trimmed[end])) == 0) {
            ++end;
        }

        words.push_back(trimmed.substr(cursor, end - cursor));
        cursor = end;
    }

    return words;
}

std::string normalize_scene_target(const std::string& value) {
    const std::string trimmed = trim(value);
    if (starts_with(trimmed, "bg ")) {
        return trim(trimmed.substr(3));
    }
    return trimmed;
}

bool rewrite_simple_assignment(const std::string& expression, std::string& rewritten) {
    static const char* operators[] = {"+=", "-=", "*=", "/="};

    const std::string trimmed = trim(expression);
    for (const char* op : operators) {
        const std::string token(op);
        const size_t pos = trimmed.find(token);
        if (pos == std::string::npos) {
            continue;
        }

        const std::string left = trim(trimmed.substr(0, pos));
        const std::string right = trim(trimmed.substr(pos + token.size()));
        if (left.empty() || right.empty()) {
            return false;
        }

        rewritten = left + " = " + left + " " + std::string(1, token[0]) + " " + normalize_literal_booleans_and_none_sentinel(right);
        return true;
    }

    const size_t equals_pos = trimmed.find('=');
    if (equals_pos == std::string::npos) {
        return false;
    }

    if (equals_pos > 0) {
        const char prev = trimmed[equals_pos - 1];
        if (prev == '+' || prev == '-' || prev == '*' || prev == '/' || prev == '=' || prev == '!' || prev == '<' || prev == '>') {
            return false;
        }
    }

    if (equals_pos + 1 < trimmed.size() && trimmed[equals_pos + 1] == '=') {
        return false;
    }

    const std::string left = trim(trimmed.substr(0, equals_pos));
    const std::string right = trim(trimmed.substr(equals_pos + 1));
    if (left.empty() || right.empty()) {
        return false;
    }

    rewritten = left + " = " + normalize_literal_booleans_and_none_sentinel(right);
    return true;
}

std::string normalize_expression(const std::string& expression) {
    return normalize_literal_booleans_and_none_sentinel(trim(expression));
}

std::string normalize_menu_condition_suffix(const std::string& condition) {
    const std::string normalized = normalize_expression(condition);
    return normalized.empty() ? std::string{} : " if " + normalized;
}

struct ParsedShowCommand {
    std::string sprite_name;
    std::string expression;
    std::string position;
    std::string transition;
    std::string unsupported_transition;
};

std::string join_words(const std::vector<std::string>& words, size_t start_index) {
    std::ostringstream stream;
    for (size_t index = start_index; index < words.size(); ++index) {
        if (index > start_index) {
            stream << ' ';
        }
        stream << words[index];
    }
    return stream.str();
}

bool parse_show_command(const std::string& value, ParsedShowCommand& parsed) {
    parsed = ParsedShowCommand{};

    const std::vector<std::string> words = split_words(value);
    if (words.empty()) {
        return false;
    }

    parsed.sprite_name = words[0];
    if (parsed.sprite_name.empty()) {
        return false;
    }

    size_t index = 1;
    while (index < words.size()) {
        const std::string& word = words[index];
        if (word == "at") {
            if (index + 1 >= words.size()) {
                return false;
            }

            const std::string& position = words[index + 1];
            if (position == "left" || position == "right") {
                if (!parsed.position.empty()) {
                    return false;
                }
                parsed.position = position;
                index += 2;
                continue;
            }

            return false;
        }

        if (word == "with") {
            if (index + 1 >= words.size()) {
                return false;
            }

            const std::string transition_value = join_words(words, index + 1);
            if (const auto mapped = map_simple_transition_name(transition_value); mapped.has_value()) {
                parsed.transition = *mapped;
            } else {
                parsed.unsupported_transition = transition_value;
            }
            return true;
        }

        if (!parsed.expression.empty()) {
            return false;
        }

        if (word == "as" || word == "behind" || word == "zorder" || word == "onlayer"
            || word == "with" || word == "xalign" || word == "yalign" || word == "xpos"
            || word == "ypos" || word == "zoom" || word == "alpha") {
            return false;
        }

        parsed.expression = word;
        ++index;
    }

    return true;
}

std::string format_show_command(const ParsedShowCommand& parsed) {
    if (parsed.sprite_name.empty()) {
        return "@sprite show";
    }

    std::ostringstream line;
    line << "@sprite " << parsed.sprite_name << " show";
    if (!parsed.expression.empty()) {
        line << ' ' << parsed.expression;
    }
    if (!parsed.position.empty()) {
        line << " position:" << parsed.position;
    }
    return line.str();
}

struct ParsedSceneCommand {
    std::string target;
    std::string transition;
    std::string unsupported_transition;
};

ParsedSceneCommand parse_scene_command(const std::string& value) {
    ParsedSceneCommand parsed;

    const std::string trimmed = trim(value);
    const size_t with_pos = trimmed.rfind(" with ");
    if (with_pos == std::string::npos) {
        parsed.target = normalize_scene_target(trimmed);
        return parsed;
    }

    parsed.target = normalize_scene_target(trim(trimmed.substr(0, with_pos)));
    const std::string transition_value = trim(trimmed.substr(with_pos + 6));
    if (const auto mapped = map_simple_transition_name(transition_value); mapped.has_value()) {
        parsed.transition = *mapped;
    } else {
        parsed.unsupported_transition = transition_value;
    }

    return parsed;
}

std::string append_transition(const std::string& command, const std::string& transition) {
    if (transition.empty()) {
        return command;
    }
    return command + " transition:" + transition;
}

std::optional<std::string> map_simple_transition_name(const std::string& value) {
    const std::string trimmed = trim(value);
    if (trimmed == "dissolve" || trimmed == "fade") {
        return trimmed;
    }
    return std::nullopt;
}

struct ParsedAudioCommand {
    std::string path;
    bool stop = false;
    bool unsupported = false;
    std::vector<std::pair<std::string, std::string>> options;
};

bool is_simple_numeric_literal(const std::string& value) {
    const std::string trimmed = trim(value);
    if (trimmed.empty()) {
        return false;
    }

    size_t index = 0;
    bool saw_digit = false;
    bool saw_dot = false;
    if (trimmed[index] == '+' || trimmed[index] == '-') {
        ++index;
    }

    for (; index < trimmed.size(); ++index) {
        const char ch = trimmed[index];
        if (std::isdigit(static_cast<unsigned char>(ch)) != 0) {
            saw_digit = true;
            continue;
        }
        if (ch == '.' && !saw_dot) {
            saw_dot = true;
            continue;
        }
        return false;
    }

    return saw_digit;
}

bool parse_audio_command(const std::string& payload, ParsedAudioCommand& parsed) {
    parsed = ParsedAudioCommand{};

    std::string path;
    std::string extras;
    if (!parse_audio_payload(payload, path, extras)) {
        return false;
    }

    if (path == "stop") {
        parsed.stop = true;
        parsed.path = path;
        parsed.unsupported = !extras.empty();
        return true;
    }

    parsed.path = path;
    if (extras.empty()) {
        return true;
    }

    const std::vector<std::string> words = split_words(extras);
    for (size_t index = 0; index < words.size();) {
        const std::string& word = words[index];
        if (word == "loop") {
            parsed.options.emplace_back("loop", "true");
            ++index;
            continue;
        }

        if (word == "noloop") {
            parsed.options.emplace_back("loop", "false");
            ++index;
            continue;
        }

        if (word == "volume") {
            if (index + 1 >= words.size() || !is_simple_numeric_literal(words[index + 1])) {
                parsed.unsupported = true;
                return true;
            }
            parsed.options.emplace_back("volume", words[index + 1]);
            index += 2;
            continue;
        }

        parsed.unsupported = true;
        return true;
    }

    return true;
}

std::string format_audio_command(const ParsedAudioCommand& parsed, RenpyNodeKind kind) {
    std::ostringstream line;
    line << (kind == RenpyNodeKind::PlayMusic ? "@bgm " : "@sfx ") << parsed.path;
    for (const auto& option : parsed.options) {
        line << ' ' << option.first << ':' << option.second;
    }
    return line.str();
}

bool has_audio_option(const ParsedAudioCommand& parsed, const std::string& name) {
    for (const auto& option : parsed.options) {
        if (option.first == name) {
            return true;
        }
    }
    return false;
}

bool parse_audio_payload(const std::string& payload, std::string& path, std::string& extras) {
    const std::string trimmed = trim(payload);
    if (trimmed.empty()) {
        return false;
    }

    if (trimmed == "stop") {
        path = "stop";
        extras.clear();
        return true;
    }

    size_t end = 0;
    if (parse_quoted_literal(trimmed, 0, path, end)) {
        extras = trim(trimmed.substr(end));
        return true;
    }

    std::string head;
    if (!split_first_word(trimmed, head, extras)) {
        return false;
    }

    path = head;
    return true;
}

std::string unsupported_feature_tag(const RenpyNode& node) {
    switch (node.unsupported_kind) {
    case RenpyUnsupportedKind::Screen:
        return "screen";
    case RenpyUnsupportedKind::Transform:
        return "transform";
    case RenpyUnsupportedKind::With:
        return "with";
    case RenpyUnsupportedKind::Image:
        return "image";
    case RenpyUnsupportedKind::Audio:
        return "audio";
    case RenpyUnsupportedKind::Unknown:
    case RenpyUnsupportedKind::None:
        break;
    }

    return node.name.empty() ? "unsupported" : node.name;
}

struct UnsupportedInfo {
    ConversionEntryKind kind = ConversionEntryKind::Unsupported;
    ConversionSeverity severity = ConversionSeverity::Warning;
    std::string feature_tag;
    std::string reason;
    std::string action;
    std::string original_text;
};

struct MenuChoiceCallSafety {
    bool safe_terminal_call = false;
    size_t prelude_count = 0;
    size_t call_index = 0;
};

std::string menu_choice_original_text(const RenpyNode& node) {
    return "\"" + node.value + "\"" + (node.name.empty() ? std::string{} : " if " + node.name) + ":";
}

std::optional<MenuChoiceCallSafety> analyze_menu_choice_call_safety(const RenpyNode& node) {
    if (node.children.empty()) {
        return std::nullopt;
    }

    size_t call_count = 0;
    size_t call_index = 0;
    for (size_t index = 0; index < node.children.size(); ++index) {
        if (node.children[index].kind == RenpyNodeKind::Call) {
            ++call_count;
            call_index = index;
        }
    }

    if (call_count != 1) {
        return std::nullopt;
    }

    for (size_t index = 0; index < call_index; ++index) {
        if (node.children[index].kind != RenpyNodeKind::DollarStatement) {
            return std::nullopt;
        }
    }

    return MenuChoiceCallSafety{
        call_index + 1 == node.children.size(),
        call_index,
        call_index,
    };
}

UnsupportedInfo make_unsupported_info(const RenpyNode& node) {
    UnsupportedInfo info;
    info.feature_tag = unsupported_feature_tag(node);
    info.original_text = node.value.empty() ? node.name : node.value;

    switch (node.kind) {
    case RenpyNodeKind::MenuChoice:
        info.kind = ConversionEntryKind::ManualFixRequired;
        info.severity = ConversionSeverity::Warning;
        info.feature_tag = "menu_call_body";
        info.reason = "Menu choice body contains a call shape that cannot be converted safely to NovaMark block choices.";
        info.action = "Rewrite this choice body manually so control flow after the call is preserved explicitly, or simplify it to an allowed @set prelude plus terminal @call.";
        info.original_text = menu_choice_original_text(node);
        return info;
    case RenpyNodeKind::PythonBlock:
        info.kind = ConversionEntryKind::ManualFixRequired;
        info.severity = ConversionSeverity::Warning;
        info.feature_tag = "python";
        info.reason = "Python blocks cannot be mapped safely to NovaMark.";
        info.action = "Rewrite this block as explicit script commands or port the logic manually.";
        info.original_text = "python:";
        return info;
    case RenpyNodeKind::InitPythonBlock:
        info.kind = ConversionEntryKind::ManualFixRequired;
        info.severity = ConversionSeverity::Warning;
        info.feature_tag = "init_python";
        info.reason = "init python setup logic requires manual migration.";
        info.action = "Rewrite initialization side effects as startup configuration that NovaMark can express.";
        info.original_text = "init python:";
        return info;
    case RenpyNodeKind::With:
    case RenpyNodeKind::Unsupported:
        break;
    default:
        info.kind = ConversionEntryKind::PartiallySupported;
        info.severity = ConversionSeverity::Warning;
        info.reason = "This node was emitted with a conservative fallback.";
        info.action = "Review the generated output to confirm it matches the intended behavior.";
        return info;
    }

    switch (node.unsupported_kind) {
    case RenpyUnsupportedKind::Screen:
        info.reason = "screen language is not supported for automatic conversion in the current version.";
        info.action = "Rewrite it manually as scene, menu, or UI state script content.";
        info.original_text = node.name.empty() ? node.value : "screen " + node.value + ":";
        break;
    case RenpyUnsupportedKind::Transform:
        info.reason = "transform/ATL animation cannot be mapped automatically in v0.";
        info.action = "Add transition, animation, or state-change logic manually.";
        info.original_text = node.name.empty() ? node.value : "transform " + node.value + ":";
        break;
    case RenpyUnsupportedKind::With:
        info.kind = ConversionEntryKind::PartiallySupported;
        info.reason = "The with transition was recorded only as a comment; no executable transition command was generated.";
        info.action = "Add the corresponding NovaMark transition manually based on the intended effect.";
        info.original_text = "with " + (node.name.empty() ? node.value : node.name);
        break;
    case RenpyUnsupportedKind::Image:
        info.reason = "Image definitions must be migrated separately at the resource layer.";
        info.action = "Create the resource mapping manually and add the required character or sprite configuration.";
        info.original_text = node.name.empty() ? node.value : "image " + node.value;
        break;
    case RenpyUnsupportedKind::Audio:
        info.kind = ConversionEntryKind::PartiallySupported;
        info.reason = "This audio statement was only partially recognized, so no NovaMark audio command was generated safely.";
        info.action = "Add @bgm / @sfx manually, or confirm that the statement can be ignored.";
        info.original_text = node.name.empty() ? node.value : "play " + node.value;
        break;
    case RenpyUnsupportedKind::Unknown:
    case RenpyUnsupportedKind::None:
        info.reason = "This Ren'Py statement is not recognized yet and cannot be converted reliably.";
        info.action = "Inspect it manually and add the corresponding NovaMark logic.";
        if (info.original_text.empty()) {
            info.original_text = node.name;
        }
        break;
    }

    return info;
}

class EmitterContext {
public:
    void emit_project(const RenpyProject& project) {
        for (const auto& statement : project.statements) {
            if (statement.kind != RenpyNodeKind::CharacterDefinition) {
                continue;
            }

            const CharacterMetadata metadata = extract_character_metadata(statement.value);
            if (!metadata.display_name.empty()) {
                m_character_names[statement.name] = metadata.display_name;
            }
        }

        bool first = true;
        for (size_t index = 0; index < project.statements.size(); ++index) {
            if (!first) {
                m_stream << '\n';
            }

            std::string transition;
            if (try_get_attached_transition(project.statements, index, transition)) {
                emit_node(project.statements[index], 0, transition);
                add_simple_transition_report(project.statements[index + 1], transition);
                ++index;
            } else {
                emit_node(project.statements[index], 0);
            }

            first = false;
        }
    }

    ConversionResult<std::string> finish() {
        return ConversionResult<std::string>(m_stream.str(), std::move(m_report));
    }

private:
    void add_inline_entry(const ConversionEntry& entry) {
        m_report.add_entry(entry);
    }

    std::string resolve_character_name(const std::string& alias) const {
        const auto it = m_character_names.find(alias);
        if (it != m_character_names.end()) {
            return it->second;
        }
        return alias;
    }

    void emit_line(size_t depth, const std::string& text) {
        m_stream << indent(depth) << text << '\n';
    }

    void emit_todo(const RenpyNode& node, size_t depth) {
        const UnsupportedInfo info = make_unsupported_info(node);
        m_report.add_entry(ConversionEntry::single_line(
            info.kind,
            info.severity,
            info.feature_tag,
            info.reason,
            info.action,
            info.original_text,
            node.line));

        std::ostringstream todo;
        todo << "// TODO source: line " << node.line
             << "; reason: " << info.reason
             << "; action: " << info.action
             << "; original: " << info.original_text;
        emit_line(depth, todo.str());
    }

    void emit_menu_choice(const RenpyNode& node, size_t depth) {
        const std::string condition_suffix = normalize_menu_condition_suffix(node.name);
        if (node.children.size() == 1 && node.children[0].kind == RenpyNodeKind::Jump) {
            emit_line(depth, "- [" + escape_string(node.value) + "] -> " + scene_anchor_name(node.children[0].name) + condition_suffix);
            return;
        }

        if (const auto call_safety = analyze_menu_choice_call_safety(node); call_safety.has_value()) {
            if (!call_safety->safe_terminal_call) {
                emit_line(depth, "- [" + escape_string(node.value) + "]" + condition_suffix);
                emit_todo(node, depth + 1);
                return;
            }

            emit_line(depth, "- [" + escape_string(node.value) + "]" + condition_suffix);
            for (size_t index = 0; index < call_safety->prelude_count; ++index) {
                emit_node(node.children[index], depth + 1);
            }
            emit_node(node.children[call_safety->call_index], depth + 1);
            return;
        }

        emit_line(depth, "- [" + escape_string(node.value) + "]" + condition_suffix);
        if (node.children.empty()) {
            RenpyNode synthetic = node;
            synthetic.kind = RenpyNodeKind::Unsupported;
            synthetic.unsupported_kind = RenpyUnsupportedKind::Unknown;
            synthetic.name = "menu_choice";
            synthetic.value = menu_choice_original_text(node);
            emit_todo(synthetic, depth + 1);
            return;
        }

        emit_todo(node, depth + 1);
    }

    void emit_sequence(const std::vector<RenpyNode>& nodes, size_t depth) {
        for (size_t index = 0; index < nodes.size(); ++index) {
            std::string transition;
            if (try_get_attached_transition(nodes, index, transition)) {
                emit_node(nodes[index], depth, transition);
                add_simple_transition_report(nodes[index + 1], transition);
                ++index;
                continue;
            }

            emit_node(nodes[index], depth);
        }
    }

    void emit_children_with_todo_header(const RenpyNode& node, size_t depth) {
        emit_todo(node, depth);
        emit_sequence(node.children, depth + 1);
    }

    void emit_conditional(const RenpyNode& node, size_t depth) {
        emit_line(depth, "if " + normalize_expression(node.name));
        emit_sequence(node.children, depth + 1);

        if (!node.else_children.empty()) {
            emit_line(depth, "else");
            emit_sequence(node.else_children, depth + 1);
        }

        emit_line(depth, "endif");
    }

    bool can_attach_transition_to(const RenpyNode& node) const {
        if (node.kind == RenpyNodeKind::Scene) {
            return true;
        }

        if (node.kind == RenpyNodeKind::Show) {
            ParsedShowCommand parsed;
            return parse_show_command(node.name, parsed);
        }

        if (node.kind == RenpyNodeKind::Hide) {
            std::string sprite_name;
            std::string sprite_args;
            return split_first_word(node.name, sprite_name, sprite_args) && !sprite_name.empty();
        }

        return false;
    }

    bool try_get_attached_transition(const std::vector<RenpyNode>& nodes, size_t index, std::string& transition) const {
        transition.clear();
        if (index + 1 >= nodes.size() || !can_attach_transition_to(nodes[index])) {
            return false;
        }

        const RenpyNode& next = nodes[index + 1];
        if (next.kind != RenpyNodeKind::With || next.unsupported_kind != RenpyUnsupportedKind::With) {
            return false;
        }

        const auto mapped = map_simple_transition_name(next.value.empty() ? next.name : next.value);
        if (!mapped.has_value()) {
            return false;
        }

        transition = *mapped;
        return true;
    }

    void add_simple_transition_report(const RenpyNode& with_node, const std::string& transition) {
        add_inline_entry(ConversionEntry::single_line(
            ConversionEntryKind::PartiallySupported,
            ConversionSeverity::Info,
            "with",
            "The with transition was conservatively mapped to transition:" + transition + ".",
            "Confirm that attaching this transition to the previous media command still matches the original script behavior.",
            "with " + (with_node.name.empty() ? with_node.value : with_node.name),
            with_node.line));
    }

    void emit_node(const RenpyNode& node, size_t depth, const std::string& attached_transition = {}) {
        switch (node.kind) {
        case RenpyNodeKind::CharacterDefinition: {
            const CharacterMetadata metadata = extract_character_metadata(node.value);
            const std::string resolved_name = metadata.display_name.empty() ? node.name : metadata.display_name;
            emit_line(depth, "@char " + resolved_name);
            if (resolved_name.empty()) {
                RenpyNode synthetic = node;
                synthetic.kind = RenpyNodeKind::Unsupported;
                synthetic.unsupported_kind = RenpyUnsupportedKind::Unknown;
                synthetic.name = "character_definition";
                synthetic.value = node.value;
                emit_todo(synthetic, depth + 1);
            } else {
                for (const auto& property : metadata.properties) {
                    emit_line(depth + 1, property.first + ": " + property.second);
                }
            }
            emit_line(depth, "@end");
            break;
        }
        case RenpyNodeKind::DefaultStatement:
            emit_line(depth, "@var " + node.name + " = " + normalize_expression(node.value));
            break;
        case RenpyNodeKind::If:
            emit_conditional(node, depth);
            break;
        case RenpyNodeKind::Elif:
        case RenpyNodeKind::Else:
            emit_todo(node, depth);
            break;
        case RenpyNodeKind::Label:
            emit_line(depth, "#" + scene_anchor_name(node.name) + " \"" + escape_string(node.name) + "\"");
            emit_sequence(node.children, depth);
            break;
        case RenpyNodeKind::Jump:
            emit_line(depth, "-> " + scene_anchor_name(node.name));
            break;
        case RenpyNodeKind::Call:
            emit_line(depth, "@call " + scene_anchor_name(node.name));
            break;
        case RenpyNodeKind::Return:
            if (node.name.empty()) {
                emit_line(depth, "@return");
            } else {
                emit_line(depth, "@return");
                RenpyNode synthetic = node;
                synthetic.kind = RenpyNodeKind::Unsupported;
                synthetic.unsupported_kind = RenpyUnsupportedKind::Unknown;
                synthetic.name = "return_value";
                synthetic.value = "return " + node.name;
                emit_todo(synthetic, depth + 1);
            }
            break;
        case RenpyNodeKind::Say:
            emit_line(depth, resolve_character_name(node.name) + ": " + escape_string(node.value));
            break;
        case RenpyNodeKind::Narration:
            emit_line(depth, "> " + escape_string(node.value));
            break;
        case RenpyNodeKind::Menu:
            emit_line(depth, node.value.empty() ? "? Choose an option" : "? " + escape_string(node.value));
            emit_sequence(node.children, depth);
            break;
        case RenpyNodeKind::MenuChoice:
            emit_menu_choice(node, depth);
            break;
        case RenpyNodeKind::Scene: {
            const ParsedSceneCommand parsed = parse_scene_command(node.name);
            const std::string transition = parsed.transition.empty() ? attached_transition : parsed.transition;
            emit_line(depth, append_transition("@bg " + parsed.target, transition));
            if (!parsed.unsupported_transition.empty()) {
                RenpyNode synthetic = node;
                synthetic.kind = RenpyNodeKind::With;
                synthetic.unsupported_kind = RenpyUnsupportedKind::With;
                synthetic.name = parsed.unsupported_transition;
                synthetic.value.clear();
                emit_todo(synthetic, depth);
            }
            break;
        }
        case RenpyNodeKind::Show: {
            ParsedShowCommand parsed;
            if (parse_show_command(node.name, parsed)) {
                const std::string transition = parsed.transition.empty() ? attached_transition : parsed.transition;
                emit_line(depth, append_transition(format_show_command(parsed), transition));
                if (!parsed.unsupported_transition.empty()) {
                    RenpyNode synthetic = node;
                    synthetic.kind = RenpyNodeKind::With;
                    synthetic.unsupported_kind = RenpyUnsupportedKind::With;
                    synthetic.name = parsed.unsupported_transition;
                    synthetic.value.clear();
                    emit_todo(synthetic, depth);
                }
            } else {
                RenpyNode synthetic = node;
                synthetic.kind = RenpyNodeKind::Unsupported;
                synthetic.unsupported_kind = RenpyUnsupportedKind::Unknown;
                synthetic.name = "show";
                synthetic.value = "show " + node.name;
                emit_todo(synthetic, depth);
            }
            break;
        }
        case RenpyNodeKind::Hide: {
            std::string sprite_name;
            std::string sprite_args;
            if (split_first_word(node.name, sprite_name, sprite_args)) {
                emit_line(depth, append_transition("@sprite " + sprite_name + " hide" + (sprite_args.empty() ? std::string{} : " " + sprite_args), attached_transition));
            } else {
                emit_line(depth, "@sprite hide");
            }
            break;
        }
        case RenpyNodeKind::PlayMusic:
        case RenpyNodeKind::PlaySound: {
            ParsedAudioCommand parsed;
            if (parse_audio_command(node.name, parsed)
                && !parsed.unsupported
                && !(parsed.stop && node.kind == RenpyNodeKind::PlaySound)
                && (node.kind != RenpyNodeKind::PlaySound || !has_audio_option(parsed, "loop"))) {
                emit_line(depth, format_audio_command(parsed, node.kind));
            } else {
                RenpyNode synthetic = node;
                synthetic.kind = RenpyNodeKind::Unsupported;
                synthetic.unsupported_kind = RenpyUnsupportedKind::Audio;
                synthetic.value = std::string(node.kind == RenpyNodeKind::PlayMusic ? "music " : "sound ") + node.name;
                emit_todo(synthetic, depth);
            }
            break;
        }
        case RenpyNodeKind::DollarStatement: {
            std::string rewritten;
            emit_line(depth, "@set " + (rewrite_simple_assignment(node.name, rewritten) ? rewritten : node.name));
            break;
        }
        case RenpyNodeKind::PythonBlock:
        case RenpyNodeKind::InitPythonBlock:
            emit_children_with_todo_header(node, depth);
            break;
        case RenpyNodeKind::With:
            emit_todo(node, depth);
            break;
        case RenpyNodeKind::Unsupported:
            emit_children_with_todo_header(node, depth);
            break;
        }
    }

    std::ostringstream m_stream;
    std::unordered_map<std::string, std::string> m_character_names;
    ConversionReport m_report;
};

} // namespace

ConversionResult<std::string> NovamarkEmitter::emit(const RenpyProject& project) const {
    EmitterContext context;
    context.emit_project(project);
    return context.finish();
}

} // namespace nova::renpy2nova
