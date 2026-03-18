#pragma once

#include <string>
#include <unordered_map>
#include <optional>

namespace nova {

/// @brief 物品槽位
struct ItemSlot {
    std::string itemId;
    int count;
};

/// @brief 背包管理器
class Inventory {
public:
    /// @brief 添加物品
    void add(const std::string& itemId, int count = 1);
    
    /// @brief 移除物品
    bool remove(const std::string& itemId, int count = 1);
    
    /// @brief 获取物品数量
    int count(const std::string& itemId) const;
    
    /// @brief 检查是否有物品
    bool has(const std::string& itemId) const;
    
    /// @brief 清空背包
    void clear();
    
    /// @brief 获取所有物品
    const std::unordered_map<std::string, int>& all() const;
    
    /// @brief 检查是否满足物品条件
    bool checkRequirement(const std::string& itemId, int requiredCount) const;
    
    /// @brief 获取所有物品（用于序列化）
    std::unordered_map<std::string, int> getAllItems() const { return m_items; }
    
    /// @brief 从存档数据加载背包
    void loadFrom(const std::unordered_map<std::string, int>& items) { m_items = items; }

 private:
    std::unordered_map<std::string, int> m_items;
};

} // namespace nova