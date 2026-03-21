#pragma once

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

namespace nova {

struct TextConfigState {
    std::string defaultFont = "sans-serif";
    int defaultFontSize = 24;
    int defaultTextSpeed = 50;
};

/// @brief 精灵状态
struct SpriteState {
    std::string id;
    std::string url;
    double x = 0.0;
    double y = 0.0;
    std::string position;
    double opacity = 1.0;
    int zIndex = 0;

    bool operator==(const SpriteState& other) const {
        return id == other.id && url == other.url && x == other.x && y == other.y &&
               position == other.position &&
               opacity == other.opacity && zIndex == other.zIndex;
    }
};

/// @brief 音效状态
struct SfxState {
    std::string id;
    std::string path;
    bool loop = false;
    double volume = 1.0;
};

/// @brief 对话状态
struct DialogueState {
    bool isShow = false;
    std::string speaker;
    std::string text;
    std::string emotion;
    std::string color;

    bool operator==(const DialogueState& other) const {
        return isShow == other.isShow && speaker == other.speaker && text == other.text &&
               emotion == other.emotion && color == other.color;
    }
};

/// @brief 选择选项
struct ChoiceOption {
    std::string id;
    std::string text;
    std::string target;
    bool disabled = false;

    bool operator==(const ChoiceOption& other) const {
        return id == other.id && text == other.text && target == other.target && disabled == other.disabled;
    }
};

/// @brief 选择状态
struct ChoiceState {
    bool isShow = false;
    std::string question;
    std::vector<ChoiceOption> options;

    bool operator==(const ChoiceState& other) const {
        return isShow == other.isShow && question == other.question && options == other.options;
    }

    bool operator!=(const ChoiceState& other) const {
        return !(*this == other);
    }
};

/// @brief VM 运行状态
enum class VMStatus {
    Running,
    WaitingChoice,
    Ended
};

/// @brief NovaMark 渲染状态（VM 输出给渲染器）
struct NovaState {
    VMStatus status = VMStatus::Running;

    TextConfigState textConfig;
    
    std::string currentScene;
    std::string currentLabel;
    
    std::optional<std::string> bg;
    std::optional<std::string> bgTransition;
    
    std::optional<std::string> bgm;
    double bgmVolume = 1.0;
    bool bgmLoop = true;
    
    std::vector<SfxState> sfx;
    
    std::vector<SpriteState> sprites;
    
    std::optional<DialogueState> dialogue;
    
    std::optional<ChoiceState> choice;
    
    std::optional<std::string> ending;
    
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
