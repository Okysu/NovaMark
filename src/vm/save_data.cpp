#include "nova/vm/save_data.h"
#include <algorithm>

namespace nova {

void PlaythroughState::triggerEnding(const std::string& endingId) {
    m_endings.insert(endingId);
}

bool PlaythroughState::hasEnding(const std::string& endingId) const {
    return m_endings.find(endingId) != m_endings.end();
}

void PlaythroughState::setFlag(const std::string& flagId) {
    m_flags.insert(flagId);
}

bool PlaythroughState::hasFlag(const std::string& flagId) const {
    return m_flags.find(flagId) != m_flags.end();
}

void PlaythroughState::clearFlag(const std::string& flagId) {
    m_flags.erase(flagId);
}

const std::unordered_set<std::string>& PlaythroughState::endings() const {
    return m_endings;
}

const std::unordered_set<std::string>& PlaythroughState::flags() const {
    return m_flags;
}

void PlaythroughState::clear() {
    m_endings.clear();
    m_flags.clear();
}

void PlaythroughState::resetForNewGame() {
}

SaveData SaveManager::createSave(const std::string& label, const GameState& state) {
    SaveData save;
    save.saveId = "save_" + std::to_string(m_nextSaveId++);
    save.label = label;
    save.timestamp = std::chrono::system_clock::now();
    save.state = state;
    m_saves.push_back(save);
    return save;
}

const std::vector<SaveData>& SaveManager::saves() const {
    return m_saves;
}

bool SaveManager::deleteSave(const std::string& saveId) {
    auto it = std::find_if(m_saves.begin(), m_saves.end(),
        [&saveId](const SaveData& save) { return save.saveId == saveId; });
    if (it != m_saves.end()) {
        m_saves.erase(it);
        return true;
    }
    return false;
}

void SaveManager::clear() {
    m_saves.clear();
}

} // namespace nova