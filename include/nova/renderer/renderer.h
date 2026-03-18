#pragma once

#include "nova/vm/state.h"
#include <functional>
#include <memory>

namespace nova {

class IRenderer {
public:
    virtual ~IRenderer() = default;
    
    virtual void render(const NovaState& state) = 0;
    
    virtual void clear() = 0;
    
    virtual void showBackground(const std::string& path) = 0;
    virtual void hideBackground() = 0;
    
    virtual void showSprite(const SpriteState& sprite) = 0;
    virtual void hideSprite(const std::string& id) = 0;
    virtual void clearSprites() = 0;
    
    virtual void playBgm(const std::string& path, bool loop = true, double volume = 1.0) = 0;
    virtual void stopBgm() = 0;
    
    virtual void playSfx(const SfxState& sfx) = 0;
    virtual void stopSfx(const std::string& id) = 0;
    virtual void stopAllSfx() = 0;
    
    virtual void showDialogue(const DialogueState& dialogue) = 0;
    virtual void hideDialogue() = 0;
    
    virtual void showChoice(const ChoiceState& choice) = 0;
    virtual void hideChoice() = 0;
    
    virtual void showHud(const HudState& hud) = 0;
    virtual void hideHud(const std::string& id) = 0;
    
    using ClickCallback = std::function<void(double x, double y)>;
    using ChoiceCallback = std::function<void(const std::string& choiceId)>;
    
    virtual void setOnClick(ClickCallback callback) = 0;
    virtual void setOnChoice(ChoiceCallback callback) = 0;
};

class NullRenderer : public IRenderer {
public:
    void render(const NovaState&) override {}
    void clear() override {}
    void showBackground(const std::string&) override {}
    void hideBackground() override {}
    void showSprite(const SpriteState&) override {}
    void hideSprite(const std::string&) override {}
    void clearSprites() override {}
    void playBgm(const std::string&, bool, double) override {}
    void stopBgm() override {}
    void playSfx(const SfxState&) override {}
    void stopSfx(const std::string&) override {}
    void stopAllSfx() override {}
    void showDialogue(const DialogueState&) override {}
    void hideDialogue() override {}
    void showChoice(const ChoiceState&) override {}
    void hideChoice() override {}
    void showHud(const HudState&) override {}
    void hideHud(const std::string&) override {}
    void setOnClick(ClickCallback) override {}
    void setOnChoice(ChoiceCallback) override {}
};

}
