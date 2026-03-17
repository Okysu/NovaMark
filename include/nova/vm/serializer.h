#pragma once

#include "nova/vm/game_state.h"
#include "nova/vm/variable.h"
#include "nova/vm/inventory.h"
#include <string>
#include <vector>

namespace nova {

/// @brief 游戏状态序列化器
class GameStateSerializer {
public:
    /// @brief 序列化游戏状态到 JSON 字符串
    static std::string serialize(const GameState& state);
    
    /// @brief 从 JSON 字符串反序列化游戏状态
    static bool deserialize(const std::string& json, GameState& state);
    
    /// @brief 序列化存档到 JSON 字符串
    static std::string serializeSave(const SaveData& save);
    
    /// @brief 从 JSON 字符串反序列化存档
    static bool deserializeSave(const std::string& json, SaveData& save);
    
    /// @brief 保存存档到文件
    static bool saveToFile(const std::string& path, const SaveData& save);
    
    /// @brief 从文件加载存档
    static bool loadFromFile(const std::string& path, SaveData& save);
    
    /// @brief 从 VM 组件构建游戏状态
    static GameState captureState(
        const std::string& currentScene,
        size_t statementIndex,
        const std::vector<std::pair<std::string, size_t>>& callStack,
        const VariableManager& variables,
        const Inventory& inventory,
        const std::unordered_set<std::string>& endings,
        const std::unordered_set<std::string>& flags
    );
    
    /// @brief 将游戏状态应用到 VM 组件
    static void restoreState(
        const GameState& state,
        std::string& currentScene,
        size_t& statementIndex,
        std::vector<std::pair<std::string, size_t>>& callStack,
        VariableManager& variables,
        Inventory& inventory,
        std::unordered_set<std::string>& endings,
        std::unordered_set<std::string>& flags
    );
};

} // namespace nova