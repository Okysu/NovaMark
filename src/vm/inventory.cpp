#include "nova/vm/inventory.h"

namespace nova {

void Inventory::add(const std::string& itemId, int count) {
    if (count <= 0) return;
    m_items[itemId] += count;
}

bool Inventory::remove(const std::string& itemId, int count) {
    auto it = m_items.find(itemId);
    if (it == m_items.end() || it->second < count) {
        return false;
    }
    it->second -= count;
    if (it->second <= 0) {
        m_items.erase(it);
    }
    return true;
}

int Inventory::count(const std::string& itemId) const {
    auto it = m_items.find(itemId);
    return it != m_items.end() ? it->second : 0;
}

bool Inventory::has(const std::string& itemId) const {
    return m_items.find(itemId) != m_items.end() && m_items.at(itemId) > 0;
}

void Inventory::clear() {
    m_items.clear();
}

const std::unordered_map<std::string, int>& Inventory::all() const {
    return m_items;
}

bool Inventory::checkRequirement(const std::string& itemId, int requiredCount) const {
    return count(itemId) >= requiredCount;
}

} // namespace nova