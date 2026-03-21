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
    j["statementIndex"] = state.statementIndex;
    
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
        state.statementIndex = j.value("statementIndex", 0);
        
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
    uint64_t statementIndex = static_cast<uint64_t>(save.state.statementIndex);
    write_pod(out, statementIndex);
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

    save = SaveData{};
    if (!read_string(data, offset, save.saveId) ||
        !read_string(data, offset, save.label) ||
        !read_string(data, offset, save.screenshot) ||
        !read_pod(data, offset, timestamp) ||
        !read_string(data, offset, save.state.currentScene) ||
        !read_pod(data, offset, statementIndex) ||
        !read_call_stack(data, offset, save.state.callStack) ||
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
    size_t statementIndex,
    const std::vector<std::pair<std::string, size_t>>& callStack,
    const VariableManager& variables,
    const Inventory& inventory,
    const std::unordered_set<std::string>& endings,
    const std::unordered_set<std::string>& flags
) {
    GameState state;
    state.currentScene = currentScene;
    state.statementIndex = statementIndex;
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
    size_t& statementIndex,
    std::vector<std::pair<std::string, size_t>>& callStack,
    VariableManager& variables,
    Inventory& inventory,
    std::unordered_set<std::string>& endings,
    std::unordered_set<std::string>& flags
) {
    currentScene = state.currentScene;
    statementIndex = state.statementIndex;
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
