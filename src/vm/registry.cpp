#include "nova/vm/registry.h"
#include <iostream>
#include <algorithm>

namespace nova {

// 内置指令名列表
static const std::vector<std::string> BUILTIN_DIRECTIVES = {
    "bg", "sprite", "bgm", "sfx", "set", "give", "take",
    "flag", "ending", "call", "return", "check", "if",
    "var", "char", "item"
};

// 内置函数名列表
static const std::vector<std::string> BUILTIN_FUNCTIONS = {
    "has_item", "item_count", "has_flag", "has_ending",
    "roll", "random", "chance", "var"
};

bool Registry::registerDirective(const std::string& name,
                                  DirectiveHandler handler,
                                  bool override_) {
    if (name.empty() || !handler) return false;

    if (!override_) {
        // 不允许覆写时：如果已注册（内置或自定义），返回失败
        if (m_directives.count(name) > 0) return false;
    }

    if (override_) {
        if (isBuiltinDirective(name)) {
            std::cerr << "[NovaMark Registry] WARNING: Overriding built-in directive @" << name << std::endl;
        } else if (m_directives.count(name) > 0) {
            std::cerr << "[NovaMark Registry] WARNING: Overriding previously registered directive @" << name << std::endl;
        }
    }

    m_directives[name] = std::move(handler);
    return true;
}

bool Registry::registerFunction(const std::string& name,
                                 FunctionHandler handler,
                                 bool override_) {
    if (name.empty() || !handler) return false;

    if (!override_) {
        if (m_functions.count(name) > 0) return false;
    }

    if (override_) {
        if (isBuiltinFunction(name)) {
            std::cerr << "[NovaMark Registry] WARNING: Overriding built-in function " << name << "()" << std::endl;
        } else if (m_functions.count(name) > 0) {
            std::cerr << "[NovaMark Registry] WARNING: Overriding previously registered function " << name << "()" << std::endl;
        }
    }

    m_functions[name] = std::move(handler);
    return true;
}

bool Registry::registerStateField(const std::string& key,
                                   StateFieldSerializer serializer,
                                   StateFieldDeserializer deserializer,
                                   const std::string& defaultValue) {
    if (key.empty() || !serializer || !deserializer) return false;

    m_stateFields[key] = StateFieldEntry{
        key,
        std::move(serializer),
        std::move(deserializer),
        defaultValue
    };
    return true;
}

const Registry::DirectiveHandler* Registry::findDirective(const std::string& name) const {
    auto it = m_directives.find(name);
    return it != m_directives.end() ? &(it->second) : nullptr;
}

const Registry::FunctionHandler* Registry::findFunction(const std::string& name) const {
    auto it = m_functions.find(name);
    return it != m_functions.end() ? &(it->second) : nullptr;
}

const Registry::StateFieldEntry* Registry::findStateField(const std::string& key) const {
    auto it = m_stateFields.find(key);
    return it != m_stateFields.end() ? &(it->second) : nullptr;
}

bool Registry::isBuiltinDirective(const std::string& name) const {
    return std::find(BUILTIN_DIRECTIVES.begin(), BUILTIN_DIRECTIVES.end(), name) != BUILTIN_DIRECTIVES.end();
}

bool Registry::isBuiltinFunction(const std::string& name) const {
    return std::find(BUILTIN_FUNCTIONS.begin(), BUILTIN_FUNCTIONS.end(), name) != BUILTIN_FUNCTIONS.end();
}

std::unordered_map<std::string, std::string> Registry::serializeExtensions() const {
    std::unordered_map<std::string, std::string> extensions;
    for (const auto& [key, entry] : m_stateFields) {
        try {
            extensions[key] = entry.serializer();
        } catch (const std::exception& e) {
            std::cerr << "[NovaMark Registry] Error serializing extension '" << key << "': " << e.what() << std::endl;
            extensions[key] = entry.defaultValue;
        }
    }
    return extensions;
}

void Registry::deserializeExtensions(const std::unordered_map<std::string, std::string>& extensions) const {
    for (const auto& [key, entry] : m_stateFields) {
        auto it = extensions.find(key);
        if (it != extensions.end() && !it->second.empty()) {
            try {
                entry.deserializer(it->second);
            } catch (const std::exception& e) {
                std::cerr << "[NovaMark Registry] Error deserializing extension '" << key << "': " << e.what() << std::endl;
            }
        }
    }
}

std::unordered_map<std::string, std::string> Registry::getDefaultExtensions() const {
    std::unordered_map<std::string, std::string> extensions;
    for (const auto& [key, entry] : m_stateFields) {
        extensions[key] = entry.defaultValue;
    }
    return extensions;
}

std::vector<std::string> Registry::getRegisteredDirectiveNames() const {
    std::vector<std::string> names;
    names.reserve(m_directives.size());
    for (const auto& [name, _] : m_directives) {
        names.push_back(name);
    }
    return names;
}

std::vector<std::string> Registry::getRegisteredFunctionNames() const {
    std::vector<std::string> names;
    names.reserve(m_functions.size());
    for (const auto& [name, _] : m_functions) {
        names.push_back(name);
    }
    return names;
}

} // namespace nova
