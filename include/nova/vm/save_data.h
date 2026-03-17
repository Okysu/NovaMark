#pragma once

#include "nova/vm/game_state.h"
#include <string>
#include <unordered_set>
#include <vector>

namespace nova {

// SaveData 定义已移至 game_state.h，现在包含完整的 GameState

/// @brief 多周目状态管理
class PlaythroughState {
public:
    /// @brief 触发结局
    void triggerEnding(const std::string& endingId);
    
    /// @brief 检查是否已触发结局
    bool hasEnding(const std::string& endingId) const;
    
    /// @brief 设置标记
    void setFlag(const std::string& flagId);
    
    /// @brief 检查标记是否存在
    bool hasFlag(const std::string& flagId) const;
    
    /// @brief 清除标记
    void clearFlag(const std::string& flagId);
    
    /// @brief 获取所有结局
    const std::unordered_set<std::string>& endings() const;
    
    /// @brief 获取所有标记
    const std::unordered_set<std::string>& flags() const;
    
    /// @brief 清空所有状态
    void clear();
    
    /// @brief 重置周目（保留结局和标记）
    void resetForNewGame();

private:
    std::unordered_set<std::string> m_endings;
    std::unordered_set<std::string> m_flags;
};

/// @brief 存档管理器
class SaveManager {
public:
    /// @brief 创建存档
    SaveData createSave(const std::string& label, const GameState& state);
    
    /// @brief 获取所有存档
    const std::vector<SaveData>& saves() const;
    
    /// @brief 删除存档
    bool deleteSave(const std::string& saveId);
    
    /// @brief 清空所有存档
    void clear();

private:
    std::vector<SaveData> m_saves;
    int m_nextSaveId = 1;
};

} // namespace nova