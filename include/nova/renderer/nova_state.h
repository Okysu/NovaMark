#pragma once

#include <string>
#include <vector>
#include <optional>

namespace nova {

struct SpriteState {
    std::string id;
    std::string url;
    double x = 0;
    double y = 0;
    double opacity = 1.0;
    int zIndex = 0;
};

struct HudState {
    std::string id;
    bool show = false;
    std::string content;
    std::string icon;
    double x = 0;
    double y = 0;
};

struct DialogueState {
    bool isShow = false;
    std::string name;
    std::string text;
    std::string color = "#FFFFFF";
};

struct ChoiceState {
    std::string id;
    std::string text;
    bool disabled = false;
};

struct SoundEffect {
    std::string id;
    std::string path;
    bool loop = false;
};

struct NovaState {
    std::string bg;
    std::string bgm;
    std::vector<SoundEffect> sfx;
    
    std::vector<SpriteState> sprites;
    std::vector<HudState> huds;
    
    std::optional<DialogueState> dialogue;
    std::vector<ChoiceState> choices;
    
    void clear() {
        bg.clear();
        bgm.clear();
        sfx.clear();
        sprites.clear();
        huds.clear();
        dialogue.reset();
        choices.clear();
    }
};

} // namespace nova