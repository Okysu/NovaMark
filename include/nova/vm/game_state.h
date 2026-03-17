#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <chrono>

namespace nova {

/// @brief 游戏状态快照（可序列化）
struct GameState {
    std::string currentScene;
    size_t statementIndex = 0;
    
    std::vector<std::pair<std::string, size_t>> callStack;
    
    std::unordered_map<std::string, double> numberVariables;
    std::unordered_map<std::string, std::string> stringVariables;
    std::unordered_map<std::string, bool> boolVariables;
    
    std::unordered_map<std::string, int> inventory;
    
    std::unordered_set<std::string> triggeredEndings;
    std::unordered_set<std::string> flags;
    
    void clear() {
        currentScene.clear();
        statementIndex = 0;
        callStack.clear();
        numberVariables.clear();
        stringVariables.clear();
        boolVariables.clear();
        inventory.clear();
        triggeredEndings.clear();
        flags.clear();
    }
};

/// @brief 存档数据
struct SaveData {
    std::string saveId;
    std::string label;
    std::chrono::system_clock::time_point timestamp;
    std::string screenshot;
    GameState state;
};

} // namespace nova
