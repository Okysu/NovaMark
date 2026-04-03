#include "nova/semantic/symbol_table.h"

namespace nova {

bool SymbolTable::define(const std::string& name, SymbolKind kind,
                         SourceLocation loc, const std::string& scope) {
    std::string key = make_key(name, scope);
    
    if (scope.empty()) {
        if (m_global_symbols.find(key) != m_global_symbols.end()) {
            return false;
        }
        m_global_symbols[key] = Symbol{name, kind, std::move(loc), scope, false};
    } else {
        if (m_local_symbols.find(key) != m_local_symbols.end()) {
            return false;
        }
        m_local_symbols[key] = Symbol{name, kind, std::move(loc), scope, false};
    }
    return true;
}

std::optional<Symbol> SymbolTable::lookup(const std::string& name,
                                          const std::string& scope) const {
    if (!scope.empty()) {
        std::string search_scope = scope;
        while (!search_scope.empty()) {
            std::string key = make_key(name, search_scope);
            auto it = m_local_symbols.find(key);
            if (it != m_local_symbols.end()) {
                return it->second;
            }

            auto separator = search_scope.rfind("::");
            if (separator == std::string::npos) {
                break;
            }
            search_scope = search_scope.substr(0, separator);
        }
    }
    
    auto it = m_global_symbols.find(name);
    if (it != m_global_symbols.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

bool SymbolTable::exists(const std::string& name, const std::string& scope) const {
    return lookup(name, scope).has_value();
}

void SymbolTable::mark_used(const std::string& name, const std::string& scope) {
    if (!scope.empty()) {
        std::string key = make_key(name, scope);
        auto it = m_local_symbols.find(key);
        if (it != m_local_symbols.end()) {
            it->second.is_used = true;
            return;
        }
    }
    
    auto it = m_global_symbols.find(name);
    if (it != m_global_symbols.end()) {
        it->second.is_used = true;
    }
}

std::vector<Symbol> SymbolTable::get_unused_symbols() const {
    std::vector<Symbol> result;
    
    for (const auto& [key, sym] : m_global_symbols) {
        if (!sym.is_used) {
            result.push_back(sym);
        }
    }
    
    for (const auto& [key, sym] : m_local_symbols) {
        if (!sym.is_used) {
            result.push_back(sym);
        }
    }
    
    return result;
}

std::vector<Symbol> SymbolTable::get_symbols_by_kind(SymbolKind kind) const {
    std::vector<Symbol> result;
    
    for (const auto& [key, sym] : m_global_symbols) {
        if (sym.kind == kind) {
            result.push_back(sym);
        }
    }
    
    for (const auto& [key, sym] : m_local_symbols) {
        if (sym.kind == kind) {
            result.push_back(sym);
        }
    }
    
    return result;
}

void SymbolTable::enter_scene(const std::string& scene_name) {
    m_current_scene = scene_name;
    m_block_scopes.clear();
    m_next_block_id = 0;
}

void SymbolTable::leave_scene() {
    m_current_scene.clear();
    m_block_scopes.clear();
    m_next_block_id = 0;
}

void SymbolTable::enter_block() {
    std::string scope = current_variable_scope();
    scope += "::block" + std::to_string(m_next_block_id++);
    m_block_scopes.push_back(std::move(scope));
}

void SymbolTable::leave_block() {
    if (!m_block_scopes.empty()) {
        m_block_scopes.pop_back();
    }
}

std::string SymbolTable::current_variable_scope() const {
    if (!m_block_scopes.empty()) {
        return m_block_scopes.back();
    }
    return m_current_scene;
}

bool SymbolTable::exists_in_current_scope(const std::string& name) const {
    const std::string scope = current_variable_scope();
    if (scope.empty()) {
        return m_global_symbols.find(name) != m_global_symbols.end();
    }
    return m_local_symbols.find(make_key(name, scope)) != m_local_symbols.end();
}

void SymbolTable::clear() {
    m_global_symbols.clear();
    m_local_symbols.clear();
    m_current_scene.clear();
    m_block_scopes.clear();
    m_next_block_id = 0;
}

} // namespace nova
