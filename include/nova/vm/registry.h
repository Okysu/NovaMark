#pragma once

#include "nova/vm/state.h"
#include "nova/vm/variable.h"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

namespace nova {

/// @brief 指令执行结果（注册重载系统）
struct DirectiveResult {
    bool handled = false;       ///< 是否已处理该指令
    bool advanceAgain = false;  ///< 处理后是否需要再次步进
};

/// @brief 注册重载系统
///
/// 允许宿主在运行时扩展和覆写 NovaMark 的指令与函数。
/// 核心场景：扩展自定义指令、覆写内置指令、扩展内置函数、状态扩展。
class Registry {
public:
    /// @brief 指令处理器签名
    /// @param name 指令名（不含@前缀）
    /// @param args 指令参数（key-value 对）
    /// @param state 当前 VM 状态（可修改）
    /// @return 指令执行结果
    using DirectiveHandler = std::function<DirectiveResult(
        const std::string& name,
        const std::vector<std::pair<std::string, std::string>>& args,
        NovaState& state)>;

    /// @brief 函数处理器签名
    /// @param args 函数参数列表
    /// @return 函数返回值
    using FunctionHandler = std::function<VarValue(const std::vector<VarValue>& args)>;

    /// @brief 状态字段序列化回调
    using StateFieldSerializer = std::function<nlohmann::json()>;

    /// @brief 状态字段反序列化回调
    using StateFieldDeserializer = std::function<void(const nlohmann::json&)>;

    /// @brief 状态字段注册条目
    struct StateFieldEntry {
        std::string key;
        StateFieldSerializer serializer;
        StateFieldDeserializer deserializer;
        nlohmann::json defaultValue;
    };

public:
    /// @brief 注册自定义指令处理器
    /// @param name 指令名（不含@前缀），如 "custom_effect"
    /// @param handler 指令执行回调
    /// @param override 若为 true，将覆写同名内置指令
    /// @return 注册是否成功
    bool registerDirective(const std::string& name,
                          DirectiveHandler handler,
                          bool override = false);

    /// @brief 注册自定义脚本函数
    /// @param name 函数名
    /// @param handler 函数执行回调
    /// @param override 若为 true，将覆写同名内置函数
    /// @return 注册是否成功
    bool registerFunction(const std::string& name,
                         FunctionHandler handler,
                         bool override = false);

    /// @brief 注册自定义状态字段
    /// @param key 字段名（建议使用命名空间前缀，如 "com.example.myfield"）
    /// @param serializer 序列化回调
    /// @param deserializer 反序列化回调
    /// @param defaultValue 默认值（用于快照恢复时字段缺失的场景）
    /// @return 注册是否成功
    bool registerStateField(const std::string& key,
                           StateFieldSerializer serializer,
                           StateFieldDeserializer deserializer,
                           const nlohmann::json& defaultValue);

    /// @brief 查找已注册的指令处理器
    /// @param name 指令名
    /// @return 处理器指针，未找到返回 nullptr
    const DirectiveHandler* findDirective(const std::string& name) const;

    /// @brief 查找已注册的函数处理器
    /// @param name 函数名
    /// @return 处理器指针，未找到返回 nullptr
    const FunctionHandler* findFunction(const std::string& name) const;

    /// @brief 查找已注册的状态字段
    /// @param key 字段名
    /// @return 字段条目指针，未找到返回 nullptr
    const StateFieldEntry* findStateField(const std::string& key) const;

    /// @brief 检查是否为内置指令名
    bool isBuiltinDirective(const std::string& name) const;

    /// @brief 检查是否为内置函数名
    bool isBuiltinFunction(const std::string& name) const;

    /// @brief 序列化所有注册的状态字段为 extensions map
    std::unordered_map<std::string, nlohmann::json> serializeExtensions() const;

    /// @brief 反序列化 extensions map 到注册的状态字段
    void deserializeExtensions(const std::unordered_map<std::string, nlohmann::json>& extensions) const;

    /// @brief 获取所有注册字段的默认值作为 extensions map
    std::unordered_map<std::string, nlohmann::json> getDefaultExtensions() const;

    /// @brief 获取所有已注册的指令名列表
    std::vector<std::string> getRegisteredDirectiveNames() const;

    /// @brief 获取所有已注册的函数名列表
    std::vector<std::string> getRegisteredFunctionNames() const;

private:
    std::unordered_map<std::string, DirectiveHandler> m_directives;
    std::unordered_map<std::string, FunctionHandler> m_functions;
    std::unordered_map<std::string, StateFieldEntry> m_stateFields;
};

} // namespace nova
