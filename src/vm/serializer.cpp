#include "nova/vm/serializer.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace nova {

using json = nlohmann::json;

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
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    file << serializeSave(save);
    return true;
}

bool GameStateSerializer::loadFromFile(const std::string& path, SaveData& save) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    
    return deserializeSave(content, save);
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