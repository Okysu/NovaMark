#include "nova/ast/ast_source_emitter.h"

#include <nlohmann/json.hpp>

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace nova {

namespace {

using json = nlohmann::json;

struct FileGroup {
    std::string file;
    std::vector<const json*> nodes;
};

std::string get_string(const json& node, const char* key, const std::string& fallback = {}) {
    if (!node.contains(key) || node.at(key).is_null()) return fallback;
    if (node.at(key).is_string()) return node.at(key).get<std::string>();
    if (node.at(key).is_boolean()) return node.at(key).get<bool>() ? "true" : "false";
    if (node.at(key).is_number_integer()) return std::to_string(node.at(key).get<long long>());
    if (node.at(key).is_number_float()) {
        std::ostringstream oss;
        oss << node.at(key).get<double>();
        return oss.str();
    }
    return fallback;
}

std::string location_file(const json& node) {
    if (node.contains("location") && node.at("location").is_object()) {
        auto file = get_string(node.at("location"), "file");
        if (!file.empty() && file != "<packed>" && file != "<unknown>") {
            return file;
        }
    }
    return "__unnamed__.nvm";
}

std::string indent(int level) {
    return std::string(static_cast<size_t>(level) * 2, ' ');
}

std::string quote_string(const std::string& value) {
    std::ostringstream oss;
    oss << '"';
    for (char ch : value) {
        switch (ch) {
            case '\\': oss << "\\\\"; break;
            case '"': oss << "\\\""; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default: oss << ch; break;
        }
    }
    oss << '"';
    return oss.str();
}

std::string format_number(double value) {
    std::ostringstream oss;
    oss << std::setprecision(15) << value;
    auto s = oss.str();
    if (s.find('.') != std::string::npos) {
        while (!s.empty() && s.back() == '0') s.pop_back();
        if (!s.empty() && s.back() == '.') s.pop_back();
    }
    return s.empty() ? "0" : s;
}

int precedence(const std::string& op) {
    if (op == "or") return 1;
    if (op == "and") return 2;
    if (op == "==" || op == "!=" || op == ">" || op == ">=" || op == "<" || op == "<=") return 3;
    if (op == "+" || op == "-") return 4;
    if (op == "*" || op == "/" || op == "%") return 5;
    return 0;
}

std::string emit_expr(const json& node, int parent_prec = 0);
std::string emit_node(const json& node, int level = 0);

std::string emit_expr_child(const json& parent, const char* key, int parent_prec = 0) {
    if (!parent.contains(key) || parent.at(key).is_null()) return "";
    return emit_expr(parent.at(key), parent_prec);
}

std::string emit_expr(const json& node, int parent_prec) {
    if (node.is_null()) return "";
    auto type = get_string(node, "type");

    if (type == "Literal") {
        const auto& value = node.at("value");
        if (value.is_null()) return "null";
        if (value.is_string()) return quote_string(value.get<std::string>());
        if (value.is_boolean()) return value.get<bool>() ? "true" : "false";
        if (value.is_number_integer()) return std::to_string(value.get<long long>());
        if (value.is_number_float()) return format_number(value.get<double>());
        return "null";
    }

    if (type == "Identifier") {
        return get_string(node, "name");
    }

    if (type == "BinaryExpr") {
        auto op = get_string(node, "op");
        int prec = precedence(op);
        auto left = emit_expr_child(node, "left", prec);
        auto right = emit_expr_child(node, "right", prec + 1);
        auto expr = left + " " + op + " " + right;
        return prec < parent_prec ? "(" + expr + ")" : expr;
    }

    if (type == "UnaryExpr") {
        auto op = get_string(node, "op");
        auto operand = emit_expr_child(node, "operand", 6);
        auto expr = op == "not" ? "not " + operand : op + operand;
        return 6 < parent_prec ? "(" + expr + ")" : expr;
    }

    if (type == "CallExpr") {
        std::vector<std::string> args;
        if (node.contains("arguments") && node.at("arguments").is_array()) {
            for (const auto& arg : node.at("arguments")) {
                args.push_back(emit_expr(arg));
            }
        }
        std::string joined;
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) joined += ", ";
            joined += args[i];
        }
        return get_string(node, "name") + "(" + joined + ")";
    }

    if (type == "DiceExpr") {
        int count = node.value("count", 1);
        int sides = node.value("sides", 6);
        int modifier = node.value("modifier", 0);
        std::string result = std::to_string(count) + "d" + std::to_string(sides);
        if (modifier > 0) result += "+" + std::to_string(modifier);
        if (modifier < 0) result += std::to_string(modifier);
        return result;
    }

    if (type == "Interpolation") {
        return "{{" + get_string(node, "varName") + "}}";
    }

    if (type == "InlineStyle") {
        return "{" + get_string(node, "style") + ":" + get_string(node, "text") + "}";
    }

    if (type == "InterpolatedText") {
        if (node.contains("segments") && node.at("segments").is_array()) {
            std::string result;
            for (const auto& segment : node.at("segments")) {
                auto segment_type = get_string(segment, "type");
                if (segment_type == "Interpolation") result += "{{" + get_string(segment, "content") + "}}";
                else if (segment_type == "InlineStyle") result += "{" + get_string(segment, "style") + ":" + get_string(segment, "content") + "}";
                else result += get_string(segment, "content");
            }
            return result;
        }
        return get_string(node, "plainText");
    }

    return "";
}

std::string emit_args(const json& node) {
    if (!node.contains("args") || !node.at("args").is_array()) return "";
    std::string result;
    for (const auto& arg : node.at("args")) {
        auto key = get_string(arg, "key");
        auto value = get_string(arg, "value");
        if (key.empty()) continue;
        result += " " + key;
        if (!value.empty() && value != "true") result += ":" + value;
    }
    return result;
}

std::string emit_properties(const json& node, int level) {
    std::string result;
    if (node.contains("properties") && node.at("properties").is_array()) {
        for (const auto& prop : node.at("properties")) {
            result += indent(level) + get_string(prop, "key") + ": " + get_string(prop, "value") + "\n";
        }
    }
    return result;
}

std::string emit_nodes(const json& nodes, int level) {
    std::string result;
    if (!nodes.is_array()) return result;
    for (const auto& child : nodes) {
        result += emit_node(child, level);
    }
    return result;
}

std::string emit_choice_option(const json& node, int level) {
    auto text = get_string(node, "text");
    auto condition = node.contains("condition") && !node.at("condition").is_null()
        ? emit_expr(node.at("condition"))
        : std::string{};
    bool has_body = node.contains("body") && node.at("body").is_array() && !node.at("body").empty();

    if (!has_body) {
        std::string line = indent(level) + "- [" + text + "] -> " + get_string(node, "target");
        if (!condition.empty()) line += " if " + condition;
        return line + "\n";
    }

    std::string result = indent(level) + "- [" + text + "]";
    if (!condition.empty()) result += " if " + condition;
    result += "\n";
    result += emit_nodes(node.at("body"), level + 1);
    return result;
}

std::string emit_node(const json& node, int level) {
    auto type = get_string(node, "type");
    auto pad = indent(level);

    if (type == "Program") return emit_nodes(node.value("children", json::array()), level);
    if (type == "FrontMatter") return "---\n" + emit_properties(node, 0) + "---\n";
    if (type == "SceneDef") {
        auto line = pad + "#" + get_string(node, "name");
        auto title = get_string(node, "title");
        if (!title.empty()) line += " " + quote_string(title);
        return line + "\n";
    }
    if (type == "Label") return pad + "." + get_string(node, "name") + "\n";
    if (type == "Narrator") return pad + "> " + get_string(node, "text") + "\n";
    if (type == "Dialogue") {
        auto emotion = get_string(node, "emotion");
        return pad + get_string(node, "speaker") + (emotion.empty() ? "" : "[" + emotion + "]") + ": " + get_string(node, "text") + "\n";
    }
    if (type == "Jump") return pad + "-> " + get_string(node, "target") + "\n";
    if (type == "Call") return pad + "@call " + get_string(node, "target") + "\n";
    if (type == "Return") return pad + "@return\n";
    if (type == "VarDef") {
        auto line = pad + "@var " + get_string(node, "name");
        if (node.contains("init") && !node.at("init").is_null()) line += " = " + emit_expr(node.at("init"));
        return line + "\n";
    }
    if (type == "SetCommand") return pad + "@set " + get_string(node, "name") + " = " + emit_expr_child(node, "value") + "\n";
    if (type == "GiveCommand") return pad + "@give " + get_string(node, "item") + " " + emit_expr_child(node, "count") + "\n";
    if (type == "TakeCommand") return pad + "@take " + get_string(node, "item") + " " + emit_expr_child(node, "count") + "\n";
    if (type == "Flag") return pad + "@flag " + get_string(node, "name") + "\n";
    if (type == "Ending") {
        auto line = pad + "@ending " + get_string(node, "name");
        auto title = get_string(node, "title");
        if (!title.empty()) line += " " + quote_string(title);
        return line + "\n";
    }
    if (type == "BgCommand") return pad + "@bg " + get_string(node, "image") + emit_args(node) + "\n";
    if (type == "SpriteCommand") return pad + "@sprite " + get_string(node, "name") + emit_args(node) + "\n";
    if (type == "BgmCommand") return pad + "@bgm " + get_string(node, "file") + emit_args(node) + "\n";
    if (type == "SfxCommand") return pad + "@sfx " + get_string(node, "file") + emit_args(node) + "\n";
    if (type == "CharDef") return pad + "@char " + get_string(node, "name") + "\n" + emit_properties(node, level + 1) + pad + "@end\n";
    if (type == "ItemDef") return pad + "@item " + get_string(node, "name") + "\n" + emit_properties(node, level + 1) + pad + "@end\n";
    if (type == "ThemeDef") return pad + "@theme " + get_string(node, "name") + "\n" + emit_properties(node, level + 1) + pad + "@end\n";
    if (type == "Choice") {
        std::string result = pad + "? " + get_string(node, "question") + "\n";
        if (node.contains("options")) result += emit_nodes(node.at("options"), level);
        return result;
    }
    if (type == "ChoiceOption") return emit_choice_option(node, level);
    if (type == "Branch") {
        std::string result = pad + "if " + emit_expr_child(node, "condition") + "\n";
        result += emit_nodes(node.value("then", json::array()), level + 1);
        if (node.contains("else") && node.at("else").is_array() && !node.at("else").empty()) {
            result += pad + "else\n";
            result += emit_nodes(node.at("else"), level + 1);
        }
        result += pad + "endif\n";
        return result;
    }
    if (type == "CheckCommand") {
        std::string result = pad + "@check " + emit_expr_child(node, "condition") + "\n";
        result += pad + "@success\n";
        result += emit_nodes(node.value("success", json::array()), level + 1);
        if (node.contains("failure") && node.at("failure").is_array() && !node.at("failure").empty()) {
            result += pad + "@fail\n";
            result += emit_nodes(node.at("failure"), level + 1);
        }
        result += pad + "@endcheck\n";
        return result;
    }

    return "";
}

} // namespace

std::string ast_snapshot_to_source_files_json(const std::string& snapshot_json) {
    auto snapshot = json::parse(snapshot_json);
    json output = {{"version", 1}, {"files", json::array()}};

    if (!snapshot.contains("root") || !snapshot.at("root").contains("children") || !snapshot.at("root").at("children").is_array()) {
        return output.dump();
    }

    std::vector<FileGroup> groups;
    std::unordered_map<std::string, size_t> index_by_file;

    for (const auto& child : snapshot.at("root").at("children")) {
        auto file = location_file(child);
        auto it = index_by_file.find(file);
        if (it == index_by_file.end()) {
            index_by_file[file] = groups.size();
            groups.push_back({file, {}});
            it = index_by_file.find(file);
        }
        groups[it->second].nodes.push_back(&child);
    }

    for (const auto& group : groups) {
        std::string content;
        for (const auto* node : group.nodes) {
            content += emit_node(*node, 0);
        }
        output["files"].push_back({{"file", group.file}, {"content", content}});
    }

    return output.dump();
}

} // namespace nova
