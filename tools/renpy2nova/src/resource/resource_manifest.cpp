#include "renpy2nova/resource/resource_manifest.h"

#include <cctype>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace nova::renpy2nova {
namespace {

std::string to_generic_string(const filesystem_compat::path& path) {
    return path.generic_string();
}

void append_json_escaped(std::ostringstream& out, const std::string& value) {
    for (unsigned char ch : value) {
        switch (ch) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (ch < 0x20U) {
                    static const char* HEX = "0123456789ABCDEF";
                    out << "\\u00"
                        << HEX[(ch >> 4U) & 0x0FU]
                        << HEX[ch & 0x0FU];
                } else {
                    out << static_cast<char>(ch);
                }
                break;
        }
    }
}

void append_json_string_field(std::ostringstream& out,
                              const char* key,
                              const std::string& value,
                              bool trailing_comma,
                              int indent) {
    out << std::string(static_cast<size_t>(indent), ' ') << '"' << key << "\": \"";
    append_json_escaped(out, value);
    out << '"';
    if (trailing_comma) {
        out << ',';
    }
    out << '\n';
}

void append_json_numeric_field(std::ostringstream& out,
                               const char* key,
                               size_t value,
                               bool trailing_comma,
                               int indent) {
    out << std::string(static_cast<size_t>(indent), ' ') << '"' << key << "\": " << value;
    if (trailing_comma) {
        out << ',';
    }
    out << '\n';
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

bool starts_with(const std::string& value, const std::string& prefix) {
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
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
    const size_t with_pos = trimmed.rfind(" with ");
    const std::string without_transition = with_pos == std::string::npos
        ? trimmed
        : trim(trimmed.substr(0, with_pos));
    if (starts_with(without_transition, "bg ")) {
        return trim(without_transition.substr(3));
    }
    return without_transition;
}

struct CharacterMetadata {
    std::string display_name;
    std::string color;
    std::string path_hint;
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
                metadata.color = "#" + normalized;
            }
        } else if (!normalized.empty() && normalized[0] == '#') {
            metadata.color = normalized;
        }
    }

    if (const auto image = extract_call_argument(value, "image"); image.has_value()) {
        const std::string normalized = trim(*image);
        if (!normalized.empty() && normalized.find_first_of(" \t") == std::string::npos) {
            metadata.path_hint = normalized + ".png";
        }
    }

    return metadata;
}

struct ParsedShowReference {
    std::string sprite_name;
    std::string expression;
};

bool parse_show_reference(const std::string& value, ParsedShowReference& parsed) {
    parsed = ParsedShowReference{};
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
            if (position != "left" && position != "right") {
                return false;
            }
            index += 2;
            continue;
        }

        if (word == "with") {
            return index + 1 < words.size();
        }

        if (!parsed.expression.empty()) {
            return false;
        }

        if (word == "as" || word == "behind" || word == "zorder" || word == "onlayer"
            || word == "xalign" || word == "yalign" || word == "xpos"
            || word == "ypos" || word == "zoom" || word == "alpha") {
            return false;
        }

        parsed.expression = word;
        ++index;
    }

    return true;
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

bool parse_audio_reference(const std::string& payload, std::string& path_hint) {
    std::string extras;
    if (!parse_audio_payload(payload, path_hint, extras)) {
        return false;
    }

    if (path_hint == "stop") {
        return true;
    }

    const std::vector<std::string> words = split_words(extras);
    for (size_t index = 0; index < words.size();) {
        const std::string& word = words[index];
        if (word == "loop" || word == "noloop") {
            ++index;
            continue;
        }

        if (word == "volume") {
            if (index + 1 >= words.size() || !is_simple_numeric_literal(words[index + 1])) {
                return false;
            }
            index += 2;
            continue;
        }

        return false;
    }

    return true;
}

bool parse_image_definition(const std::string& value,
                            std::string& image_name,
                            std::string& expression,
                            std::string& path_hint) {
    const size_t equals_pos = value.find('=');
    if (equals_pos == std::string::npos) {
        return false;
    }

    const std::string lhs = trim(value.substr(0, equals_pos));
    const std::string rhs = trim(value.substr(equals_pos + 1));
    if (lhs.empty() || rhs.empty()) {
        return false;
    }

    size_t end = 0;
    if (!parse_quoted_literal(rhs, 0, path_hint, end) || !trim(rhs.substr(end)).empty()) {
        return false;
    }

    std::string head;
    std::string tail;
    if (!split_first_word(lhs, head, tail)) {
        return false;
    }

    image_name = head;
    expression = tail;
    return !image_name.empty();
}

void collect_entries(const std::vector<RenpyNode>& nodes,
                     const filesystem_compat::path& source_relative_path,
                     std::vector<ResourceManifestEntry>& entries) {
    for (const auto& node : nodes) {
        switch (node.kind) {
            case RenpyNodeKind::CharacterDefinition: {
                const CharacterMetadata metadata = extract_character_metadata(node.value);
                if (!metadata.path_hint.empty()) {
                    ResourceManifestEntry entry;
                    entry.kind = ResourceKind::CharacterDefinition;
                    entry.source_relative_path = source_relative_path;
                    entry.line = node.line;
                    entry.name = node.name;
                    entry.path_hint = metadata.path_hint;
                    entry.display_name = metadata.display_name;
                    entry.color = metadata.color;
                    entry.original_text = node.value;
                    entries.push_back(std::move(entry));
                }
                break;
            }
            case RenpyNodeKind::Scene: {
                const std::string target = normalize_scene_target(node.name);
                if (!target.empty()) {
                    ResourceManifestEntry entry;
                    entry.kind = ResourceKind::SceneReference;
                    entry.source_relative_path = source_relative_path;
                    entry.line = node.line;
                    entry.name = target;
                    entry.path_hint = target;
                    entry.original_text = node.name;
                    entries.push_back(std::move(entry));
                }
                break;
            }
            case RenpyNodeKind::Show: {
                ParsedShowReference parsed;
                if (parse_show_reference(node.name, parsed)) {
                    ResourceManifestEntry entry;
                    entry.kind = ResourceKind::ShowReference;
                    entry.source_relative_path = source_relative_path;
                    entry.line = node.line;
                    entry.name = parsed.sprite_name;
                    entry.expression = parsed.expression;
                    entry.original_text = node.name;
                    entries.push_back(std::move(entry));
                }
                break;
            }
            case RenpyNodeKind::PlayMusic:
            case RenpyNodeKind::PlaySound: {
                std::string path_hint;
                if (parse_audio_reference(node.name, path_hint)) {
                    ResourceManifestEntry entry;
                    entry.kind = node.kind == RenpyNodeKind::PlayMusic ? ResourceKind::PlayMusic : ResourceKind::PlaySound;
                    entry.source_relative_path = source_relative_path;
                    entry.line = node.line;
                    entry.name = path_hint;
                    entry.path_hint = path_hint;
                    entry.original_text = node.name;
                    entries.push_back(std::move(entry));
                }
                break;
            }
            case RenpyNodeKind::Unsupported: {
                if (node.unsupported_kind == RenpyUnsupportedKind::Image) {
                    std::string image_name;
                    std::string expression;
                    std::string path_hint;
                    if (parse_image_definition(node.value, image_name, expression, path_hint)) {
                        ResourceManifestEntry entry;
                        entry.kind = ResourceKind::ImageDefinition;
                        entry.source_relative_path = source_relative_path;
                        entry.line = node.line;
                        entry.name = image_name;
                        entry.expression = expression;
                        entry.path_hint = path_hint;
                        entry.original_text = node.value;
                        entries.push_back(std::move(entry));
                    }
                }
                break;
            }
            default:
                break;
        }

        if (!node.children.empty()) {
            collect_entries(node.children, source_relative_path, entries);
        }
        if (!node.else_children.empty()) {
            collect_entries(node.else_children, source_relative_path, entries);
        }
    }
}

void append_optional_string_field(std::ostringstream& out,
                                  const char* key,
                                  const std::string& value,
                                  bool trailing_comma,
                                  int indent) {
    if (value.empty()) {
        return;
    }
    append_json_string_field(out, key, value, trailing_comma, indent);
}

} // namespace

std::vector<ResourceManifestEntry> extract_resource_manifest_entries(
    const RenpyProject& project,
    const filesystem_compat::path& source_relative_path) {
    std::vector<ResourceManifestEntry> entries;
    collect_entries(project.statements, source_relative_path, entries);
    return entries;
}

const char* resource_kind_to_string(ResourceKind kind) {
    switch (kind) {
        case ResourceKind::CharacterDefinition: return "character_definition";
        case ResourceKind::SceneReference: return "scene_reference";
        case ResourceKind::ShowReference: return "show_reference";
        case ResourceKind::PlayMusic: return "play_music";
        case ResourceKind::PlaySound: return "play_sound";
        case ResourceKind::ImageDefinition: return "image_definition";
    }

    return "unknown";
}

std::string serialize_resource_manifest_json(const ResourceManifest& manifest) {
    std::ostringstream out;
    out << "{\n";
    append_json_numeric_field(out, "version", 1, true, 2);
    append_json_string_field(out, "project_root", to_generic_string(manifest.project_root), true, 2);
    append_json_string_field(out, "output_dir", to_generic_string(manifest.output_dir), true, 2);
    out << "  \"summary\": {\n";
    append_json_numeric_field(out, "resource_count", manifest.entries.size(), false, 4);
    out << "  },\n";
    out << "  \"resources\": [\n";
    for (size_t index = 0; index < manifest.entries.size(); ++index) {
        const auto& entry = manifest.entries[index];
        out << "    {\n";
        append_json_string_field(out, "kind", resource_kind_to_string(entry.kind), true, 6);
        append_json_string_field(out, "source_path", to_generic_string(entry.source_relative_path), true, 6);
        append_json_numeric_field(out, "line", entry.line, true, 6);
        append_json_string_field(out, "name", entry.name, true, 6);
        append_optional_string_field(out, "path_hint", entry.path_hint, true, 6);
        append_optional_string_field(out, "display_name", entry.display_name, true, 6);
        append_optional_string_field(out, "color", entry.color, true, 6);
        append_optional_string_field(out, "expression", entry.expression, true, 6);
        append_json_string_field(out, "original_text", entry.original_text, false, 6);
        out << "    }";
        if (index + 1 < manifest.entries.size()) {
            out << ',';
        }
        out << '\n';
    }
    out << "  ]\n";
    out << "}\n";
    return out.str();
}

} // namespace nova::renpy2nova
