#pragma once

#include "nova/core/source_location.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <optional>

namespace nova {

/// @brief 符号类型
enum class SymbolKind {
    Scene,
    Variable,
    Character,
    Item,
    Label,
    Theme
};

/// @brief 符号信息
struct Symbol {
    std::string name;
    SymbolKind kind;
    SourceLocation definition_location;
    std::string scope;  // 所属场景（标签和局部变量使用）
    bool is_used = false;
};

/// @brief 符号表管理器
class SymbolTable {
public:
    /// @brief 定义符号
    bool define(const std::string& name, SymbolKind kind, 
                SourceLocation loc, const std::string& scope = "");
    
    /// @brief 查找符号
    std::optional<Symbol> lookup(const std::string& name, 
                                  const std::string& scope = "") const;
    
    /// @brief 检查符号是否存在
    bool exists(const std::string& name, const std::string& scope = "") const;
    
    /// @brief 标记符号为已使用
    void mark_used(const std::string& name, const std::string& scope = "");
    
    /// @brief 获取所有未使用的符号
    std::vector<Symbol> get_unused_symbols() const;
    
    /// @brief 获取指定类型的所有符号
    std::vector<Symbol> get_symbols_by_kind(SymbolKind kind) const;
    
    /// @brief 进入新场景作用域
    void enter_scene(const std::string& scene_name);
    
    /// @brief 离开当前场景作用域
    void leave_scene();
    
    /// @brief 获取当前场景名
    const std::string& current_scene() const { return m_current_scene; }
    
    /// @brief 清空符号表
    void clear();
    
private:
    std::unordered_map<std::string, Symbol> m_global_symbols;
    std::unordered_map<std::string, Symbol> m_local_symbols;  // 带作用域的符号
    std::string m_current_scene;
    
    std::string make_key(const std::string& name, const std::string& scope) const {
        return scope.empty() ? name : scope + "::" + name;
    }
};

} // namespace nova
