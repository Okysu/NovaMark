#pragma once

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

namespace nova {

/// @brief 精灵状态
struct SpriteState {
    std::string id;
    std::string url;
    double x = 0.0;
    double y = 0.0;
    double opacity = 1.0;
    int zIndex = 0;
    std::optional<std::string> animation;
};

/// @brief 音效状态
struct SfxState {
    std::string id;
    std::string path;
    bool loop = false;
    double volume = 1.0;
};

/// @brief HUD (常驻UI) 状态
struct HudState {
    std::string id;
    bool show = true;
    std::string content;
    std::string icon;
    std::string position;
    std::string color;
};

/// @brief 对话状态
struct DialogueState {
    bool isShow = false;
    std::string speaker;
    std::string text;
    std::string emotion;
    std::string color;
};

/// @brief 选择选项
struct ChoiceOption {
    std::string id;
    std::string text;
    std::string target;
    bool disabled = false;
};

/// @brief 选择状态
struct ChoiceState {
    bool isShow = false;
    std::string question;
    std::vector<ChoiceOption> options;
};

/// @brief VM 运行状态
enum class VMStatus {
    Running,
    WaitingChoice,
    WaitingInput,
    WaitingDelay,
    Ended
};

/// @brief NovaMark 渲染状态（VM 输出给渲染器）
struct NovaState {
    VMStatus status = VMStatus::Running;
    
    std::string currentScene;
    std::string currentLabel;
    
    std::optional<std::string> bg;
    std::optional<std::string> bgTransition;
    
    std::optional<std::string> bgm;
    double bgmVolume = 1.0;
    bool bgmLoop = true;
    
    std::vector<SfxState> sfx;
    
    std::vector<SpriteState> sprites;
    
    std::vector<HudState> huds;
    
    std::optional<DialogueState> dialogue;
    
    std::optional<ChoiceState> choice;
    
    std::optional<std::string> ending;
    
    std::string saveLabel;
    
    void clear() {
        status = VMStatus::Running;
        bg.reset();
        bgTransition.reset();
        bgm.reset();
        sfx.clear();
        sprites.clear();
        dialogue.reset();
        choice.reset();
        ending.reset();
    }
    
    void clearDialogue() {
        dialogue.reset();
    }
    
    void clearChoice() {
        choice.reset();
    }
};

} // namespace nova