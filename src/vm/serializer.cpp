#include "nova/vm/serializer.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cstring>

namespace nova {

using json = nlohmann::json;

namespace {

constexpr char SAVE_MAGIC[] = "NVMSAVE";
constexpr uint32_t SAVE_VERSION = 6;

template<typename T>
void write_pod(std::vector<uint8_t>& out, const T& value) {
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
    out.insert(out.end(), bytes, bytes + sizeof(T));
}

template<typename T>
bool read_pod(const std::vector<uint8_t>& data, size_t& offset, T& value) {
    if (offset + sizeof(T) > data.size()) {
        return false;
    }
    std::memcpy(&value, data.data() + offset, sizeof(T));
    offset += sizeof(T);
    return true;
}

void write_string(std::vector<uint8_t>& out, const std::string& value) {
    uint32_t size = static_cast<uint32_t>(value.size());
    write_pod(out, size);
    out.insert(out.end(), value.begin(), value.end());
}

bool read_string(const std::vector<uint8_t>& data, size_t& offset, std::string& value) {
    uint32_t size = 0;
    if (!read_pod(data, offset, size)) {
        return false;
    }
    if (offset + size > data.size()) {
        return false;
    }
    value.assign(reinterpret_cast<const char*>(data.data() + offset), size);
    offset += size;
    return true;
}

void write_optional_string(std::vector<uint8_t>& out, const std::optional<std::string>& value) {
    write_pod(out, value.has_value());
    if (value) {
        write_string(out, *value);
    }
}

bool read_optional_string(const std::vector<uint8_t>& data, size_t& offset, std::optional<std::string>& value) {
    bool hasValue = false;
    if (!read_pod(data, offset, hasValue)) {
        return false;
    }
    value.reset();
    if (!hasValue) {
        return true;
    }
    std::string parsed;
    if (!read_string(data, offset, parsed)) {
        return false;
    }
    value = std::move(parsed);
    return true;
}

template<typename T>
void write_optional_pod(std::vector<uint8_t>& out, const std::optional<T>& value) {
    write_pod(out, value.has_value());
    if (value) {
        write_pod(out, *value);
    }
}

template<typename T>
bool read_optional_pod(const std::vector<uint8_t>& data, size_t& offset, std::optional<T>& value) {
    bool hasValue = false;
    if (!read_pod(data, offset, hasValue)) {
        return false;
    }
    value.reset();
    if (!hasValue) {
        return true;
    }
    T parsed{};
    if (!read_pod(data, offset, parsed)) {
        return false;
    }
    value = parsed;
    return true;
}

template<typename K, typename V>
void write_map_string_key(std::vector<uint8_t>& out, const std::unordered_map<K, V>& map) {
    uint32_t count = static_cast<uint32_t>(map.size());
    write_pod(out, count);
    for (const auto& [key, value] : map) {
        write_string(out, key);
        write_pod(out, value);
    }
}

void write_string_map(std::vector<uint8_t>& out, const std::unordered_map<std::string, std::string>& map) {
    uint32_t count = static_cast<uint32_t>(map.size());
    write_pod(out, count);
    for (const auto& [key, value] : map) {
        write_string(out, key);
        write_string(out, value);
    }
}

template<typename V>
bool read_map_string_key(const std::vector<uint8_t>& data, size_t& offset, std::unordered_map<std::string, V>& map) {
    uint32_t count = 0;
    if (!read_pod(data, offset, count)) {
        return false;
    }
    map.clear();
    for (uint32_t i = 0; i < count; ++i) {
        std::string key;
        V value{};
        if (!read_string(data, offset, key) || !read_pod(data, offset, value)) {
            return false;
        }
        map.emplace(std::move(key), value);
    }
    return true;
}

bool read_string_map(const std::vector<uint8_t>& data, size_t& offset, std::unordered_map<std::string, std::string>& map) {
    uint32_t count = 0;
    if (!read_pod(data, offset, count)) {
        return false;
    }
    map.clear();
    for (uint32_t i = 0; i < count; ++i) {
        std::string key;
        std::string value;
        if (!read_string(data, offset, key) || !read_string(data, offset, value)) {
            return false;
        }
        map.emplace(std::move(key), std::move(value));
    }
    return true;
}

void write_string_set(std::vector<uint8_t>& out, const std::unordered_set<std::string>& set) {
    uint32_t count = static_cast<uint32_t>(set.size());
    write_pod(out, count);
    for (const auto& value : set) {
        write_string(out, value);
    }
}

bool read_string_set(const std::vector<uint8_t>& data, size_t& offset, std::unordered_set<std::string>& set) {
    uint32_t count = 0;
    if (!read_pod(data, offset, count)) {
        return false;
    }
    set.clear();
    for (uint32_t i = 0; i < count; ++i) {
        std::string value;
        if (!read_string(data, offset, value)) {
            return false;
        }
        set.insert(std::move(value));
    }
    return true;
}

void write_text_segments(std::vector<uint8_t>& out, const std::vector<TextSegment>& segments) {
    uint32_t count = static_cast<uint32_t>(segments.size());
    write_pod(out, count);
    for (const auto& segment : segments) {
        write_string(out, segment.text);
        write_string(out, segment.style);
    }
}

bool read_text_segments(const std::vector<uint8_t>& data, size_t& offset, std::vector<TextSegment>& segments) {
    uint32_t count = 0;
    if (!read_pod(data, offset, count)) {
        return false;
    }

    segments.clear();
    segments.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        TextSegment segment;
        if (!read_string(data, offset, segment.text) || !read_string(data, offset, segment.style)) {
            return false;
        }
        segments.push_back(std::move(segment));
    }

    return true;
}

json serialize_text_segments(const std::vector<TextSegment>& segments) {
    json result = json::array();
    for (const auto& segment : segments) {
        result.push_back({
            {"text", segment.text},
            {"style", segment.style}
        });
    }
    return result;
}

void deserialize_text_segments(const json& value, std::vector<TextSegment>& segments) {
    segments.clear();
    if (!value.is_array()) {
        return;
    }

    for (const auto& item : value) {
        TextSegment segment;
        segment.text = item.value("text", "");
        segment.style = item.value("style", "");
        segments.push_back(std::move(segment));
    }
}

void write_call_stack(std::vector<uint8_t>& out, const std::vector<std::pair<std::string, size_t>>& stack) {
    uint32_t count = static_cast<uint32_t>(stack.size());
    write_pod(out, count);
    for (const auto& [scene, index] : stack) {
        write_string(out, scene);
        uint64_t idx = static_cast<uint64_t>(index);
        write_pod(out, idx);
    }
}

bool read_call_stack(const std::vector<uint8_t>& data, size_t& offset, std::vector<std::pair<std::string, size_t>>& stack) {
    uint32_t count = 0;
    if (!read_pod(data, offset, count)) {
        return false;
    }
    stack.clear();
    for (uint32_t i = 0; i < count; ++i) {
        std::string scene;
        uint64_t idx = 0;
        if (!read_string(data, offset, scene) || !read_pod(data, offset, idx)) {
            return false;
        }
        stack.emplace_back(std::move(scene), static_cast<size_t>(idx));
    }
    return true;
}

}

std::string GameStateSerializer::serialize(const GameState& state) {
    json j;
    j["currentScene"] = state.currentScene;
    j["currentLabel"] = state.currentLabel;
    j["statementIndex"] = state.statementIndex;
    j["textConfig"] = {
        {"defaultFont", state.textConfig.defaultFont},
        {"defaultFontSize", state.textConfig.defaultFontSize},
        {"defaultTextSpeed", state.textConfig.defaultTextSpeed}
    };
    j["bg"] = state.bg;
    j["bgTransition"] = state.bgTransition;
    j["bgm"] = state.bgm;
    j["bgmVolume"] = state.bgmVolume;
    j["bgmLoop"] = state.bgmLoop;
    j["currentTheme"] = state.currentTheme;
    j["themeProperties"] = state.themeProperties;
    j["stateVersion"] = state.stateVersion;
    if (state.ending) {
        j["ending"] = {{"title", state.ending->title}, {"reached", state.ending->reached}};
    } else {
        j["ending"] = nullptr;
    }
    j["sprites"] = json::array();
    for (const auto& sp : state.sprites) {
        json sprite = {{"id", sp.id}};
        if (sp.url) sprite["url"] = *sp.url;
        if (sp.x) sprite["x"] = *sp.x;
        if (sp.y) sprite["y"] = *sp.y;
        if (sp.position) sprite["position"] = *sp.position;
        if (sp.opacity) sprite["opacity"] = *sp.opacity;
        if (sp.zIndex) sprite["zIndex"] = *sp.zIndex;
        j["sprites"].push_back(std::move(sprite));
    }
    if (state.dialogue) {
        j["dialogue"] = {
            {"isShow", state.dialogue->isShow},
            {"speaker", state.dialogue->speaker},
            {"text", state.dialogue->text},
            {"segments", serialize_text_segments(state.dialogue->segments)},
            {"emotion", state.dialogue->emotion},
            {"color", state.dialogue->color}
        };
    }
    if (state.choice) {
        j["choice"] = {
            {"isShow", state.choice->isShow},
            {"question", state.choice->question},
            {"questionSegments", serialize_text_segments(state.choice->questionSegments)},
            {"options", json::array()}
        };
        for (const auto& opt : state.choice->options) {
            j["choice"]["options"].push_back({
                {"id", opt.id},
                {"text", opt.text},
                {"segments", serialize_text_segments(opt.segments)},
                {"target", opt.target},
                {"disabled", opt.disabled}
            });
        }
    }
    
    json callStackJson = json::array();
    for (const auto& [scene, idx] : state.callStack) {
        callStackJson.push_back({{"scene", scene}, {"index", idx}});
    }
    j["callStack"] = callStackJson;
    
    j["numberVariables"] = state.numberVariables;
    j["stringVariables"] = state.stringVariables;
    j["boolVariables"] = state.boolVariables;
    j["inventory"] = state.inventory;
    j["triggeredEndings"] = state.triggeredEndings;
    j["flags"] = state.flags;

    // extensions（stateVersion 3）—— 每个值为 JSON 字符串，需要解析后写入
    if (!state.extensions.empty()) {
        json extObj = json::object();
        for (const auto& [key, value] : state.extensions) {
            try {
                extObj[key] = json::parse(value);
            } catch (...) {
                extObj[key] = value;  // 解析失败则作为纯字符串
            }
        }
        j["extensions"] = std::move(extObj);
        if (state.stateVersion < 3) {
            j["stateVersion"] = 3;
        }
    }

    return j.dump(2);
}

bool GameStateSerializer::deserialize(const std::string& jsonStr, GameState& state) {
    try {
        json j = json::parse(jsonStr);
        
        state.currentScene = j.value("currentScene", "");
        state.currentLabel = j.value("currentLabel", "");
        state.statementIndex = j.value("statementIndex", 0);
        if (j.contains("textConfig")) {
            auto cfg = j["textConfig"];
            state.textConfig.defaultFont = cfg.value("defaultFont", "sans-serif");
            state.textConfig.defaultFontSize = cfg.value("defaultFontSize", 24);
            state.textConfig.defaultTextSpeed = cfg.value("defaultTextSpeed", 50);
        }
        if (j.contains("bg") && !j["bg"].is_null()) state.bg = j["bg"].get<std::string>();
        if (j.contains("bgTransition") && !j["bgTransition"].is_null()) state.bgTransition = j["bgTransition"].get<std::string>();
        if (j.contains("bgm") && !j["bgm"].is_null()) state.bgm = j["bgm"].get<std::string>();
        state.bgmVolume = j.value("bgmVolume", 1.0);
        state.bgmLoop = j.value("bgmLoop", true);
        state.currentTheme.reset();
        state.themeProperties.clear();
        if (j.contains("currentTheme") && !j["currentTheme"].is_null()) {
            state.currentTheme = j["currentTheme"].get<std::string>();
        }
        state.themeProperties = j.value("themeProperties", std::unordered_map<std::string, std::string>{});

        // ending 反序列化：兼容 v1（string 格式）和 v2（EndingState 对象格式）
        state.stateVersion = j.value("stateVersion", 1);
        if (j.contains("ending") && !j["ending"].is_null()) {
            if (j["ending"].is_string()) {
                // v1 格式：ending 为字符串（结局 ID），转成 EndingState
                std::string endingId = j["ending"].get<std::string>();
                std::string endingTitle;
                if (j.contains("endingTitle") && !j["endingTitle"].is_null()) {
                    endingTitle = j["endingTitle"].get<std::string>();
                }
                state.ending = EndingState{endingTitle, true};
            } else if (j["ending"].is_object()) {
                // v2 格式：ending 为 EndingState 对象
                EndingState es;
                es.title = j["ending"].value("title", "");
                es.reached = j["ending"].value("reached", true);
                state.ending = es;
            }
        }

        // stateVersion 自动升级到当前版本
        if (state.stateVersion < 2) {
            state.stateVersion = 2;
        }

        state.sprites.clear();
        if (j.contains("sprites")) {
            for (const auto& item : j["sprites"]) {
                SpriteState sp;
                sp.id = item.value("id", "");
                if (item.contains("url") && !item["url"].is_null()) sp.url = item["url"].get<std::string>();
                if (item.contains("x") && !item["x"].is_null()) sp.x = item["x"].get<std::string>();
                if (item.contains("y") && !item["y"].is_null()) sp.y = item["y"].get<std::string>();
                if (item.contains("position") && !item["position"].is_null()) sp.position = item["position"].get<std::string>();
                if (item.contains("opacity") && !item["opacity"].is_null()) sp.opacity = item["opacity"].get<double>();
                if (item.contains("zIndex") && !item["zIndex"].is_null()) sp.zIndex = item["zIndex"].get<int>();
                state.sprites.push_back(std::move(sp));
            }
        }

        if (j.contains("dialogue") && !j["dialogue"].is_null()) {
            DialogueState dialog;
            const auto& d = j["dialogue"];
            dialog.isShow = d.value("isShow", false);
            dialog.speaker = d.value("speaker", "");
            dialog.text = d.value("text", "");
            if (d.contains("segments")) {
                deserialize_text_segments(d["segments"], dialog.segments);
            } else if (!dialog.text.empty()) {
                dialog.segments.push_back(TextSegment{dialog.text, ""});
            }
            dialog.emotion = d.value("emotion", "");
            dialog.color = d.value("color", "");
            state.dialogue = std::move(dialog);
        }

        if (j.contains("choice") && !j["choice"].is_null()) {
            ChoiceState choice;
            const auto& c = j["choice"];
            choice.isShow = c.value("isShow", false);
            choice.question = c.value("question", "");
            if (c.contains("questionSegments")) {
                deserialize_text_segments(c["questionSegments"], choice.questionSegments);
            } else if (!choice.question.empty()) {
                choice.questionSegments.push_back(TextSegment{choice.question, ""});
            }
            if (c.contains("options")) {
                for (const auto& item : c["options"]) {
                    ChoiceOption opt;
                    opt.id = item.value("id", "");
                    opt.text = item.value("text", "");
                    if (item.contains("segments")) {
                        deserialize_text_segments(item["segments"], opt.segments);
                    } else if (!opt.text.empty()) {
                        opt.segments.push_back(TextSegment{opt.text, ""});
                    }
                    opt.target = item.value("target", "");
                    opt.disabled = item.value("disabled", false);
                    choice.options.push_back(std::move(opt));
                }
            }
            state.choice = std::move(choice);
        }
        
        state.callStack.clear();
        if (j.contains("callStack")) {
            for (const auto& item : j["callStack"]) {
                state.callStack.push_back({
                    item.value("scene", ""),
                    item.value("index", 0)
                });
            }
        }
        
        state.numberVariables = j.value("numberVariables", std::unordered_map<std::string, double>{});
        state.stringVariables = j.value("stringVariables", std::unordered_map<std::string, std::string>{});
        state.boolVariables = j.value("boolVariables", std::unordered_map<std::string, bool>{});
        state.inventory = j.value("inventory", std::unordered_map<std::string, int>{});
        state.triggeredEndings = j.value("triggeredEndings", std::unordered_set<std::string>{});
        state.flags = j.value("flags", std::unordered_set<std::string>{});

        // extensions（stateVersion 3 兼容）—— 每个值序列化为 JSON 字符串
        if (j.contains("extensions") && j["extensions"].is_object()) {
            for (auto& [key, val] : j["extensions"].items()) {
                state.extensions[key] = val.dump();
            }
        }

        return true;
    } catch (const json::exception&) {
        return false;
    }
}

std::string GameStateSerializer::serializeSave(const SaveData& save) {
    json j;
    j["saveId"] = save.saveId;
    j["label"] = save.label;
    
    auto timestamp = std::chrono::system_clock::to_time_t(save.timestamp);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&timestamp), "%Y-%m-%d %H:%M:%S");
    j["timestamp"] = oss.str();
    
    j["screenshot"] = save.screenshot;
    j["state"] = json::parse(serialize(save.state));
    
    return j.dump(2);
}

bool GameStateSerializer::deserializeSave(const std::string& jsonStr, SaveData& save) {
    try {
        json j = json::parse(jsonStr);
        
        save.saveId = j.value("saveId", "");
        save.label = j.value("label", "");
        save.screenshot = j.value("screenshot", "");
        
        std::string timestampStr = j.value("timestamp", "");
        if (!timestampStr.empty()) {
            std::tm tm = {};
            std::istringstream iss(timestampStr);
            iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
            if (!iss.fail()) {
                save.timestamp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
            }
        }
        
        if (j.contains("state")) {
            return deserialize(j["state"].dump(), save.state);
        }
        
        return true;
    } catch (const json::exception&) {
        return false;
    }
}

bool GameStateSerializer::saveToFile(const std::string& path, const SaveData& save) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    auto data = GameStateSerializer::serializeSaveBinary(save);
    file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return file.good();
}

bool GameStateSerializer::loadFromFile(const std::string& path, SaveData& save) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    std::vector<uint8_t> content((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());

    return GameStateSerializer::deserializeSaveBinary(content, save);
}

std::vector<uint8_t> GameStateSerializer::serializeSaveBinary(const SaveData& save) {
    std::vector<uint8_t> out;
    out.insert(out.end(), SAVE_MAGIC, SAVE_MAGIC + sizeof(SAVE_MAGIC) - 1);
    write_pod(out, SAVE_VERSION);

    write_string(out, save.saveId);
    write_string(out, save.label);
    write_string(out, save.screenshot);

    int64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(save.timestamp.time_since_epoch()).count();
    write_pod(out, timestamp);

    write_string(out, save.state.currentScene);
    write_string(out, save.state.currentLabel);
    uint64_t statementIndex = static_cast<uint64_t>(save.state.statementIndex);
    write_pod(out, statementIndex);
    write_string(out, save.state.textConfig.defaultFont);
    write_pod(out, save.state.textConfig.defaultFontSize);
    write_pod(out, save.state.textConfig.defaultTextSpeed);
    write_pod(out, save.state.bg.has_value()); if (save.state.bg) write_string(out, *save.state.bg);
    write_pod(out, save.state.bgTransition.has_value()); if (save.state.bgTransition) write_string(out, *save.state.bgTransition);
    write_pod(out, save.state.bgm.has_value()); if (save.state.bgm) write_string(out, *save.state.bgm);
    write_pod(out, save.state.bgmVolume);
    write_pod(out, save.state.bgmLoop);
    write_pod(out, save.state.currentTheme.has_value());
    if (save.state.currentTheme) write_string(out, *save.state.currentTheme);
    write_string_map(out, save.state.themeProperties);
    uint32_t spriteCount = static_cast<uint32_t>(save.state.sprites.size());
    write_pod(out, spriteCount);
    for (const auto& sp : save.state.sprites) {
        write_string(out, sp.id);
        write_optional_string(out, sp.url);
        write_optional_string(out, sp.x);
        write_optional_string(out, sp.y);
        write_optional_string(out, sp.position);
        write_optional_pod(out, sp.opacity);
        write_optional_pod(out, sp.zIndex);
    }
    write_pod(out, save.state.dialogue.has_value());
    if (save.state.dialogue) {
        write_pod(out, save.state.dialogue->isShow);
        write_string(out, save.state.dialogue->speaker);
        write_string(out, save.state.dialogue->text);
        write_text_segments(out, save.state.dialogue->segments);
        write_string(out, save.state.dialogue->emotion);
        write_string(out, save.state.dialogue->color);
    }
    write_pod(out, save.state.choice.has_value());
    if (save.state.choice) {
        write_pod(out, save.state.choice->isShow);
        write_string(out, save.state.choice->question);
        write_text_segments(out, save.state.choice->questionSegments);
        uint32_t optionCount = static_cast<uint32_t>(save.state.choice->options.size());
        write_pod(out, optionCount);
        for (const auto& opt : save.state.choice->options) {
            write_string(out, opt.id);
            write_string(out, opt.text);
            write_text_segments(out, opt.segments);
            write_string(out, opt.target);
            write_pod(out, opt.disabled);
        }
    }
    write_pod(out, save.state.ending.has_value());
    if (save.state.ending) {
        write_string(out, save.state.ending->title);
        write_pod(out, save.state.ending->reached);
    }
    write_call_stack(out, save.state.callStack);
    write_map_string_key(out, save.state.numberVariables);
    write_string_map(out, save.state.stringVariables);
    write_map_string_key(out, save.state.boolVariables);
    write_map_string_key(out, save.state.inventory);
    write_string_set(out, save.state.triggeredEndings);
    write_string_set(out, save.state.flags);
    // extensions 以 JSON 对象字符串形式追加
    nlohmann::json extJson = nlohmann::json::object();
    for (const auto& [key, val] : save.state.extensions) {
        try {
            extJson[key] = nlohmann::json::parse(val);
        } catch (...) {
            extJson[key] = val;
        }
    }
    write_string(out, extJson.dump());
    return out;
}

bool GameStateSerializer::deserializeSaveBinary(const std::vector<uint8_t>& data, SaveData& save) {
    if (data.size() < sizeof(SAVE_MAGIC) - 1 + sizeof(uint32_t)) {
        return false;
    }

    size_t offset = 0;
    if (std::memcmp(data.data(), SAVE_MAGIC, sizeof(SAVE_MAGIC) - 1) != 0) {
        return false;
    }
    offset += sizeof(SAVE_MAGIC) - 1;

    uint32_t version = 0;
    if (!read_pod(data, offset, version) || (version != 1 && version != 2 && version != 3 && version != 4 && version != 5 && version != SAVE_VERSION)) {
        return false;
    }

    int64_t timestamp = 0;
    uint64_t statementIndex = 0;
    uint32_t spriteCount = 0;

    save = SaveData{};
    bool hasBg = false, hasBgTransition = false, hasBgm = false, hasDialogue = false, hasChoice = false, hasEnding = false;
    if (!read_string(data, offset, save.saveId) ||
        !read_string(data, offset, save.label) ||
        !read_string(data, offset, save.screenshot) ||
        !read_pod(data, offset, timestamp) ||
        !read_string(data, offset, save.state.currentScene) ||
        !read_string(data, offset, save.state.currentLabel) ||
        !read_pod(data, offset, statementIndex) ||
        !read_string(data, offset, save.state.textConfig.defaultFont) ||
        !read_pod(data, offset, save.state.textConfig.defaultFontSize) ||
        !read_pod(data, offset, save.state.textConfig.defaultTextSpeed) ||
        !read_pod(data, offset, hasBg)) {
        return false;
    }
    if (hasBg) {
        std::string bg;
        if (!read_string(data, offset, bg)) return false;
        save.state.bg = std::move(bg);
    }
    if (!read_pod(data, offset, hasBgTransition)) {
        return false;
    }
    if (hasBgTransition) {
        std::string bgTransition;
        if (!read_string(data, offset, bgTransition)) return false;
        save.state.bgTransition = std::move(bgTransition);
    }
    if (!read_pod(data, offset, hasBgm)) {
        return false;
    }
    if (hasBgm) {
        std::string bgm;
        if (!read_string(data, offset, bgm)) return false;
        save.state.bgm = std::move(bgm);
    }
    if (!read_pod(data, offset, save.state.bgmVolume) ||
        !read_pod(data, offset, save.state.bgmLoop)) {
        return false;
    }
    if (version >= 2) {
        bool hasCurrentTheme = false;
        if (!read_pod(data, offset, hasCurrentTheme)) {
            return false;
        }
        if (hasCurrentTheme) {
            std::string currentTheme;
            if (!read_string(data, offset, currentTheme)) {
                return false;
            }
            save.state.currentTheme = std::move(currentTheme);
        }
        if (!read_string_map(data, offset, save.state.themeProperties)) {
            return false;
        }
    }
    if (!read_pod(data, offset, spriteCount)) {
        return false;
    }

    save.state.sprites.clear();
    save.state.sprites.reserve(spriteCount);
    for (uint32_t i = 0; i < spriteCount; ++i) {
        SpriteState sp;
        if (!read_string(data, offset, sp.id)) {
            return false;
        }
        if (version >= 3) {
            if (!read_optional_string(data, offset, sp.url) ||
                !read_optional_string(data, offset, sp.x) ||
                !read_optional_string(data, offset, sp.y) ||
                !read_optional_string(data, offset, sp.position) ||
                !read_optional_pod(data, offset, sp.opacity) ||
                !read_optional_pod(data, offset, sp.zIndex)) {
                return false;
            }
        } else {
            std::string legacyUrl;
            double legacyX = 0.0;
            double legacyY = 0.0;
            std::string legacyPosition;
            double legacyOpacity = 1.0;
            int legacyZIndex = 0;
            if (!read_string(data, offset, legacyUrl) ||
                !read_pod(data, offset, legacyX) ||
                !read_pod(data, offset, legacyY) ||
                !read_string(data, offset, legacyPosition) ||
                !read_pod(data, offset, legacyOpacity) ||
                !read_pod(data, offset, legacyZIndex)) {
                return false;
            }
            sp.url = std::move(legacyUrl);
            sp.x = std::to_string(legacyX);
            sp.y = std::to_string(legacyY);
            if (!legacyPosition.empty()) sp.position = std::move(legacyPosition);
            sp.opacity = legacyOpacity;
            sp.zIndex = legacyZIndex;
        }
        save.state.sprites.push_back(std::move(sp));
    }

    if (!read_pod(data, offset, hasDialogue)) {
        return false;
    }
    if (hasDialogue) {
        DialogueState dialog;
        if (!read_pod(data, offset, dialog.isShow) ||
            !read_string(data, offset, dialog.speaker) ||
            !read_string(data, offset, dialog.text) ||
            (version >= 5 && !read_text_segments(data, offset, dialog.segments)) ||
            !read_string(data, offset, dialog.emotion) ||
            !read_string(data, offset, dialog.color)) {
            return false;
        }
        if (version < 5 && !dialog.text.empty()) {
            dialog.segments.push_back(TextSegment{dialog.text, ""});
        }
        save.state.dialogue = std::move(dialog);
    }

    if (!read_pod(data, offset, hasChoice)) {
        return false;
    }
    if (hasChoice) {
        ChoiceState choice;
        uint32_t optionCount = 0;
        if (!read_pod(data, offset, choice.isShow) ||
            !read_string(data, offset, choice.question) ||
            (version >= 5 && !read_text_segments(data, offset, choice.questionSegments)) ||
            !read_pod(data, offset, optionCount)) {
            return false;
        }
        if (version < 5 && !choice.question.empty()) {
            choice.questionSegments.push_back(TextSegment{choice.question, ""});
        }
        choice.options.reserve(optionCount);
        for (uint32_t i = 0; i < optionCount; ++i) {
            ChoiceOption opt;
            if (!read_string(data, offset, opt.id) ||
                !read_string(data, offset, opt.text) ||
                (version >= 5 && !read_text_segments(data, offset, opt.segments)) ||
                !read_string(data, offset, opt.target) ||
                !read_pod(data, offset, opt.disabled)) {
                return false;
            }
            if (version < 5 && !opt.text.empty()) {
                opt.segments.push_back(TextSegment{opt.text, ""});
            }
            choice.options.push_back(std::move(opt));
        }
        save.state.choice = std::move(choice);
    }

    if (!read_pod(data, offset, hasEnding)) {
        return false;
    }
    if (hasEnding) {
        if (version >= 6) {
            // v6+: EndingState 格式 (title + reached)
            EndingState es;
            if (!read_string(data, offset, es.title)) {
                return false;
            }
            if (!read_pod(data, offset, es.reached)) {
                return false;
            }
            save.state.ending = std::move(es);
        } else {
            // v1-v5: string 格式 (ending ID)
            std::string endingStr;
            if (!read_string(data, offset, endingStr)) {
                return false;
            }
            // v4+ 还有 endingTitle 字段
            std::optional<std::string> endingTitle;
            if (version >= 4) {
                if (!read_optional_string(data, offset, endingTitle)) {
                    return false;
                }
            }
            // 迁移到 EndingState：优先使用 endingTitle，否则使用 ending ID
            std::string title = (endingTitle.has_value() && !endingTitle->empty())
                                ? *endingTitle : endingStr;
            save.state.ending = EndingState{title, true};
            save.state.stateVersion = 2;
        }
    } else {
        if (version >= 4 && version < 6) {
            // 没有 ending 但旧版本可能有 endingTitle 需要跳过
            std::optional<std::string> dummy;
            if (!read_optional_string(data, offset, dummy)) {
                return false;
            }
        }
    }

    if (!read_call_stack(data, offset, save.state.callStack) ||
        !read_map_string_key(data, offset, save.state.numberVariables) ||
        !read_string_map(data, offset, save.state.stringVariables) ||
        !read_map_string_key(data, offset, save.state.boolVariables) ||
        !read_map_string_key(data, offset, save.state.inventory) ||
        !read_string_set(data, offset, save.state.triggeredEndings) ||
        !read_string_set(data, offset, save.state.flags)) {
        return false;
    }

    // extensions（v6+ 二进制格式）—— 每个值 dump 为 JSON 字符串
    if (version >= 6) {
        std::string extJson;
        if (!read_string(data, offset, extJson)) {
            return false;
        }
        try {
            auto j = nlohmann::json::parse(extJson);
            if (j.is_object()) {
                for (auto& [key, val] : j.items()) {
                    save.state.extensions[key] = val.dump();
                }
            }
        } catch (...) {
            // 忽略解析失败，extensions 为空
        }
    }

    save.state.statementIndex = static_cast<size_t>(statementIndex);
    save.timestamp = std::chrono::system_clock::time_point(std::chrono::seconds(timestamp));
    return true;
}

GameState GameStateSerializer::captureState(
    const std::string& currentScene,
    const std::string& currentLabel,
    size_t statementIndex,
    const TextConfigState& textConfig,
    const std::optional<std::string>& bg,
    const std::optional<std::string>& bgTransition,
    const std::optional<std::string>& bgm,
    double bgmVolume,
    bool bgmLoop,
    const std::optional<std::string>& currentTheme,
    const std::unordered_map<std::string, std::string>& themeProperties,
    const std::vector<SpriteState>& sprites,
    const std::optional<DialogueState>& dialogue,
    const std::optional<ChoiceState>& choice,
    const std::optional<EndingState>& ending,
    const std::vector<std::pair<std::string, size_t>>& callStack,
    const VariableManager& variables,
    const Inventory& inventory,
    const std::unordered_set<std::string>& endings,
    const std::unordered_set<std::string>& flags
) {
    GameState state;
    state.stateVersion = 2;
    state.currentScene = currentScene;
    state.currentLabel = currentLabel;
    state.statementIndex = statementIndex;
    state.textConfig = textConfig;
    state.bg = bg;
    state.bgTransition = bgTransition;
    state.bgm = bgm;
    state.bgmVolume = bgmVolume;
    state.bgmLoop = bgmLoop;
    state.currentTheme = currentTheme;
    state.themeProperties = themeProperties;
    state.sprites = sprites;
    state.dialogue = dialogue;
    state.choice = choice;
    state.ending = ending;
    state.callStack = callStack;
    state.numberVariables = variables.getAllNumbers();
    state.stringVariables = variables.getAllStrings();
    state.boolVariables = variables.getAllBools();
    state.inventory = inventory.getAllItems();
    state.triggeredEndings = endings;
    state.flags = flags;
    return state;
}

void GameStateSerializer::restoreState(
    const GameState& state,
    std::string& currentScene,
    std::string& currentLabel,
    size_t& statementIndex,
    TextConfigState& textConfig,
    std::optional<std::string>& bg,
    std::optional<std::string>& bgTransition,
    std::optional<std::string>& bgm,
    double& bgmVolume,
    bool& bgmLoop,
    std::optional<std::string>& currentTheme,
    std::unordered_map<std::string, std::string>& themeProperties,
    std::vector<SpriteState>& sprites,
    std::optional<DialogueState>& dialogue,
    std::optional<ChoiceState>& choice,
    std::optional<EndingState>& ending,
    std::vector<std::pair<std::string, size_t>>& callStack,
    VariableManager& variables,
    Inventory& inventory,
    std::unordered_set<std::string>& endings,
    std::unordered_set<std::string>& flags
) {
    currentScene = state.currentScene;
    currentLabel = state.currentLabel;
    statementIndex = state.statementIndex;
    textConfig = state.textConfig;
    bg = state.bg;
    bgTransition = state.bgTransition;
    bgm = state.bgm;
    bgmVolume = state.bgmVolume;
    bgmLoop = state.bgmLoop;
    currentTheme = state.currentTheme;
    themeProperties = state.themeProperties;
    sprites = state.sprites;
    dialogue = state.dialogue;
    choice = state.choice;
    ending = state.ending;
    callStack = state.callStack;
    
    variables.clear();
    for (const auto& [name, value] : state.numberVariables) {
        variables.set(name, value);
    }
    for (const auto& [name, value] : state.stringVariables) {
        variables.set(name, value);
    }
    for (const auto& [name, value] : state.boolVariables) {
        variables.set(name, value);
    }
    
    inventory.clear();
    for (const auto& [itemId, count] : state.inventory) {
        inventory.add(itemId, count);
    }
    
    endings = state.triggeredEndings;
    flags = state.flags;
}

} // namespace nova
