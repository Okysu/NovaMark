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
constexpr uint32_t SAVE_VERSION = 1;

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
    j["ending"] = state.ending;
    j["sprites"] = json::array();
    for (const auto& sp : state.sprites) {
        j["sprites"].push_back({
            {"id", sp.id}, {"url", sp.url}, {"x", sp.x}, {"y", sp.y},
            {"position", sp.position}, {"opacity", sp.opacity}, {"zIndex", sp.zIndex}
        });
    }
    if (state.dialogue) {
        j["dialogue"] = {
            {"isShow", state.dialogue->isShow},
            {"speaker", state.dialogue->speaker},
            {"text", state.dialogue->text},
            {"emotion", state.dialogue->emotion},
            {"color", state.dialogue->color}
        };
    }
    if (state.choice) {
        j["choice"] = {
            {"isShow", state.choice->isShow},
            {"question", state.choice->question},
            {"options", json::array()}
        };
        for (const auto& opt : state.choice->options) {
            j["choice"]["options"].push_back({
                {"id", opt.id}, {"text", opt.text}, {"target", opt.target}, {"disabled", opt.disabled}
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
        if (j.contains("ending") && !j["ending"].is_null()) state.ending = j["ending"].get<std::string>();

        state.sprites.clear();
        if (j.contains("sprites")) {
            for (const auto& item : j["sprites"]) {
                SpriteState sp;
                sp.id = item.value("id", "");
                sp.url = item.value("url", "");
                sp.x = item.value("x", 0.0);
                sp.y = item.value("y", 0.0);
                sp.position = item.value("position", "");
                sp.opacity = item.value("opacity", 1.0);
                sp.zIndex = item.value("zIndex", 0);
                state.sprites.push_back(std::move(sp));
            }
        }

        if (j.contains("dialogue") && !j["dialogue"].is_null()) {
            DialogueState dialog;
            const auto& d = j["dialogue"];
            dialog.isShow = d.value("isShow", false);
            dialog.speaker = d.value("speaker", "");
            dialog.text = d.value("text", "");
            dialog.emotion = d.value("emotion", "");
            dialog.color = d.value("color", "");
            state.dialogue = std::move(dialog);
        }

        if (j.contains("choice") && !j["choice"].is_null()) {
            ChoiceState choice;
            const auto& c = j["choice"];
            choice.isShow = c.value("isShow", false);
            choice.question = c.value("question", "");
            if (c.contains("options")) {
                for (const auto& item : c["options"]) {
                    ChoiceOption opt;
                    opt.id = item.value("id", "");
                    opt.text = item.value("text", "");
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
    uint32_t spriteCount = static_cast<uint32_t>(save.state.sprites.size());
    write_pod(out, spriteCount);
    for (const auto& sp : save.state.sprites) {
        write_string(out, sp.id); write_string(out, sp.url); write_pod(out, sp.x); write_pod(out, sp.y);
        write_string(out, sp.position); write_pod(out, sp.opacity); write_pod(out, sp.zIndex);
    }
    write_pod(out, save.state.dialogue.has_value());
    if (save.state.dialogue) {
        write_pod(out, save.state.dialogue->isShow);
        write_string(out, save.state.dialogue->speaker);
        write_string(out, save.state.dialogue->text);
        write_string(out, save.state.dialogue->emotion);
        write_string(out, save.state.dialogue->color);
    }
    write_pod(out, save.state.choice.has_value());
    if (save.state.choice) {
        write_pod(out, save.state.choice->isShow);
        write_string(out, save.state.choice->question);
        uint32_t optionCount = static_cast<uint32_t>(save.state.choice->options.size());
        write_pod(out, optionCount);
        for (const auto& opt : save.state.choice->options) {
            write_string(out, opt.id); write_string(out, opt.text); write_string(out, opt.target); write_pod(out, opt.disabled);
        }
    }
    write_pod(out, save.state.ending.has_value()); if (save.state.ending) write_string(out, *save.state.ending);
    write_call_stack(out, save.state.callStack);
    write_map_string_key(out, save.state.numberVariables);
    write_string_map(out, save.state.stringVariables);
    write_map_string_key(out, save.state.boolVariables);
    write_map_string_key(out, save.state.inventory);
    write_string_set(out, save.state.triggeredEndings);
    write_string_set(out, save.state.flags);
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
    if (!read_pod(data, offset, version) || version != SAVE_VERSION) {
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
        !read_pod(data, offset, save.state.bgmLoop) ||
        !read_pod(data, offset, spriteCount)) {
        return false;
    }

    save.state.sprites.clear();
    save.state.sprites.reserve(spriteCount);
    for (uint32_t i = 0; i < spriteCount; ++i) {
        SpriteState sp;
        if (!read_string(data, offset, sp.id) ||
            !read_string(data, offset, sp.url) ||
            !read_pod(data, offset, sp.x) ||
            !read_pod(data, offset, sp.y) ||
            !read_string(data, offset, sp.position) ||
            !read_pod(data, offset, sp.opacity) ||
            !read_pod(data, offset, sp.zIndex)) {
            return false;
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
            !read_string(data, offset, dialog.emotion) ||
            !read_string(data, offset, dialog.color)) {
            return false;
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
            !read_pod(data, offset, optionCount)) {
            return false;
        }
        choice.options.reserve(optionCount);
        for (uint32_t i = 0; i < optionCount; ++i) {
            ChoiceOption opt;
            if (!read_string(data, offset, opt.id) ||
                !read_string(data, offset, opt.text) ||
                !read_string(data, offset, opt.target) ||
                !read_pod(data, offset, opt.disabled)) {
                return false;
            }
            choice.options.push_back(std::move(opt));
        }
        save.state.choice = std::move(choice);
    }

    if (!read_pod(data, offset, hasEnding)) {
        return false;
    }
    if (hasEnding) {
        std::string ending;
        if (!read_string(data, offset, ending)) {
            return false;
        }
        save.state.ending = std::move(ending);
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
    const std::vector<SpriteState>& sprites,
    const std::optional<DialogueState>& dialogue,
    const std::optional<ChoiceState>& choice,
    const std::optional<std::string>& ending,
    const std::vector<std::pair<std::string, size_t>>& callStack,
    const VariableManager& variables,
    const Inventory& inventory,
    const std::unordered_set<std::string>& endings,
    const std::unordered_set<std::string>& flags
) {
    GameState state;
    state.currentScene = currentScene;
    state.currentLabel = currentLabel;
    state.statementIndex = statementIndex;
    state.textConfig = textConfig;
    state.bg = bg;
    state.bgTransition = bgTransition;
    state.bgm = bgm;
    state.bgmVolume = bgmVolume;
    state.bgmLoop = bgmLoop;
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
    std::vector<SpriteState>& sprites,
    std::optional<DialogueState>& dialogue,
    std::optional<ChoiceState>& choice,
    std::optional<std::string>& ending,
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
