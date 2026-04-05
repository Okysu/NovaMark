#pragma once

#include "nova/vm/state.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <chrono>

namespace nova {

/// @brief 游戏状态快照（可序列化）
struct GameState {
    std::string currentScene;
    std::string currentLabel;
    size_t statementIndex = 0;
    TextConfigState textConfig;

    std::optional<std::string> bg;
    std::optional<std::string> bgTransition;
    std::optional<std::string> bgm;
    double bgmVolume = 1.0;
    bool bgmLoop = true;

    std::optional<std::string> currentTheme;
    std::unordered_map<std::string, std::string> themeProperties;

    std::vector<SpriteState> sprites;
    std::optional<DialogueState> dialogue;
    std::optional<ChoiceState> choice;
    std::optional<std::string> ending;
    std::optional<std::string> endingTitle;
    
    std::vector<std::pair<std::string, size_t>> callStack;
    
    std::unordered_map<std::string, double> numberVariables;
    std::unordered_map<std::string, std::string> stringVariables;
    std::unordered_map<std::string, bool> boolVariables;
    
    std::unordered_map<std::string, int> inventory;
    
    std::unordered_set<std::string> triggeredEndings;
    std::unordered_set<std::string> flags;
    
    void clear() {
        currentScene.clear();
        currentLabel.clear();
        statementIndex = 0;
        textConfig = TextConfigState{};
        bg.reset();
        bgTransition.reset();
        bgm.reset();
        bgmVolume = 1.0;
        bgmLoop = true;
        currentTheme.reset();
        themeProperties.clear();
        sprites.clear();
        dialogue.reset();
        choice.reset();
        ending.reset();
        endingTitle.reset();
        callStack.clear();
        numberVariables.clear();
        stringVariables.clear();
        boolVariables.clear();
        inventory.clear();
        triggeredEndings.clear();
        flags.clear();
    }
};

/// @brief 存档数据
struct SaveData {
    std::string saveId;
    std::string label;
    std::chrono::system_clock::time_point timestamp;
    std::string screenshot;
    GameState state;
};

} // namespace nova
