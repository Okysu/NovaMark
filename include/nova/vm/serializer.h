#pragma once

#include "nova/vm/game_state.h"
#include "nova/vm/state.h"
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

    static std::vector<uint8_t> serializeSaveBinary(const SaveData& save);

    static bool deserializeSaveBinary(const std::vector<uint8_t>& data, SaveData& save);
    
    /// @brief 从 VM 组件构建游戏状态
    static GameState captureState(
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
    );
    
    /// @brief 将游戏状态应用到 VM 组件
    static void restoreState(
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
    );
};

} // namespace nova
