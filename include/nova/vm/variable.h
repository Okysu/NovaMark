#pragma once

#include <string>
#include <unordered_map>
#include <variant>
#include <optional>

namespace nova {

/// @brief 变量值类型
using VarValue = std::variant<std::monostate, double, std::string, bool>;

/// @brief 变量管理器
class VariableManager {
public:
    /// @brief 设置变量
    void set(const std::string& name, VarValue value);
    
    /// @brief 获取变量
    std::optional<VarValue> get(const std::string& name) const;
    
    /// @brief 检查变量是否存在
    bool exists(const std::string& name) const;
    
    /// @brief 删除变量
    void remove(const std::string& name);
    
    /// @brief 清空所有变量
    void clear();
    
    /// @brief 获取所有变量
    const std::unordered_map<std::string, VarValue>& all() const;
    
    /// @brief 数值运算
    void add(const std::string& name, double value);
    void subtract(const std::string& name, double value);
    void multiply(const std::string& name, double value);
    void divide(const std::string& name, double value);
    
    /// @brief 获取数值（如果不是数值返回 0）
    double asNumber(const std::string& name) const;
    
    /// @brief 获取字符串
    std::string asString(const std::string& name) const;
    
    /// @brief 获取布尔值
    bool asBool(const std::string& name) const;
    
    /// @brief 获取所有数值变量
    std::unordered_map<std::string, double> getAllNumbers() const;
    
    /// @brief 获取所有字符串变量
    std::unordered_map<std::string, std::string> getAllStrings() const;
    
    /// @brief 获取所有布尔变量
    std::unordered_map<std::string, bool> getAllBools() const;

private:
    std::unordered_map<std::string, VarValue> m_variables;
};

} // namespace nova