#include "nova/vm/variable.h"
#include <stdexcept>

namespace nova {

void VariableManager::set(const std::string& name, VarValue value) {
    m_variables[name] = std::move(value);
}

std::optional<VarValue> VariableManager::get(const std::string& name) const {
    auto it = m_variables.find(name);
    if (it != m_variables.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool VariableManager::exists(const std::string& name) const {
    return m_variables.find(name) != m_variables.end();
}

void VariableManager::remove(const std::string& name) {
    m_variables.erase(name);
}

void VariableManager::clear() {
    m_variables.clear();
}

const std::unordered_map<std::string, VarValue>& VariableManager::all() const {
    return m_variables;
}

void VariableManager::add(const std::string& name, double value) {
    double current = asNumber(name);
    m_variables[name] = current + value;
}

void VariableManager::subtract(const std::string& name, double value) {
    double current = asNumber(name);
    m_variables[name] = current - value;
}

void VariableManager::multiply(const std::string& name, double value) {
    double current = asNumber(name);
    m_variables[name] = current * value;
}

void VariableManager::divide(const std::string& name, double value) {
    double current = asNumber(name);
    if (value != 0.0) {
        m_variables[name] = current / value;
    }
}

double VariableManager::asNumber(const std::string& name) const {
    auto it = m_variables.find(name);
    if (it != m_variables.end()) {
        if (std::holds_alternative<double>(it->second)) {
            return std::get<double>(it->second);
        }
    }
    return 0.0;
}

std::string VariableManager::asString(const std::string& name) const {
    auto it = m_variables.find(name);
    if (it != m_variables.end()) {
        if (std::holds_alternative<std::string>(it->second)) {
            return std::get<std::string>(it->second);
        } else if (std::holds_alternative<double>(it->second)) {
            return std::to_string(std::get<double>(it->second));
        } else if (std::holds_alternative<bool>(it->second)) {
            return std::get<bool>(it->second) ? "true" : "false";
        }
    }
    return "";
}

bool VariableManager::asBool(const std::string& name) const {
    auto it = m_variables.find(name);
    if (it != m_variables.end()) {
        if (std::holds_alternative<bool>(it->second)) {
            return std::get<bool>(it->second);
        } else if (std::holds_alternative<double>(it->second)) {
            return std::get<double>(it->second) != 0.0;
        } else if (std::holds_alternative<std::string>(it->second)) {
            return !std::get<std::string>(it->second).empty();
        }
    }
    return false;
}

std::unordered_map<std::string, double> VariableManager::getAllNumbers() const {
    std::unordered_map<std::string, double> result;
    for (const auto& [name, value] : m_variables) {
        if (std::holds_alternative<double>(value)) {
            result[name] = std::get<double>(value);
        }
    }
    return result;
}

std::unordered_map<std::string, std::string> VariableManager::getAllStrings() const {
    std::unordered_map<std::string, std::string> result;
    for (const auto& [name, value] : m_variables) {
        if (std::holds_alternative<std::string>(value)) {
            result[name] = std::get<std::string>(value);
        }
    }
    return result;
}

std::unordered_map<std::string, bool> VariableManager::getAllBools() const {
    std::unordered_map<std::string, bool> result;
    for (const auto& [name, value] : m_variables) {
        if (std::holds_alternative<bool>(value)) {
            result[name] = std::get<bool>(value);
        }
    }
    return result;
}

} // namespace nova