#include "nova/renderer/nova_c_api.h"
#include "nova/vm/vm.h"
#include "nova/vm/serializer.h"
#include "nova/packer/nvmp_writer.h"
#include <cstring>
#include <string>
#include <vector>
#include <memory>

struct NovaVM {
    nova::NovaVM vm;
    std::string lastError;
    NovaStateCallback stateCallback = nullptr;
    void* callbackUserData = nullptr;
    std::vector<NovaSprite> spriteBuffer;
    std::vector<NovaHud> hudBuffer;
    std::vector<NovaChoice> choiceBuffer;
};

extern "C" {

NovaVM* nova_create() {
    return new NovaVM();
}

void nova_destroy(NovaVM* vm) {
    delete vm;
}

int nova_load_package(NovaVM* vm, const char* path) {
    nova::NvmpReader reader;
    if (!reader.loadFromFile(path)) {
        vm->lastError = reader.error();
        return 0;
    }
    
    const auto& bytecode = reader.bytecode();
    return nova_load_from_buffer(vm, bytecode.data(), bytecode.size());
}

int nova_load_from_buffer(NovaVM* vm, const unsigned char* data, size_t size) {
    if (!data || size == 0) {
        vm->lastError = "Invalid buffer";
        return 0;
    }
    
    std::vector<uint8_t> bytecode(data, data + size);
    
    nova::AstDeserializer deserializer;
    auto program = deserializer.deserialize(bytecode);
    
    if (!program) {
        vm->lastError = deserializer.errorMessage();
        return 0;
    }
    
    vm->vm.load(program.get());
    return 1;
}

void nova_start(NovaVM* vm) {
    vm->vm.run();
}

void nova_next(NovaVM* vm) {
    vm->vm.advance();
    
    if (vm->stateCallback) {
        NovaState state = nova_get_state(vm);
        vm->stateCallback(state, vm->callbackUserData);
    }
}

void nova_make_choice(NovaVM* vm, const char* choiceId) {
    if (choiceId) {
        vm->vm.selectChoiceById(std::string(choiceId));
        
        if (vm->stateCallback) {
            NovaState state = nova_get_state(vm);
            vm->stateCallback(state, vm->callbackUserData);
        }
    }
}

NovaState nova_get_state(NovaVM* vm) {
    NovaState state = {};
    
    const auto& novaState = vm->vm.state();
    
    if (novaState.bg) {
        vm->lastError = *novaState.bg;
        state.bg = vm->lastError.c_str();
    }
    
    if (novaState.bgm) {
        vm->lastError = *novaState.bgm;
        state.bgm = vm->lastError.c_str();
    }
    
    vm->spriteBuffer.clear();
    vm->spriteBuffer.reserve(novaState.sprites.size());
    
    for (const auto& sp : novaState.sprites) {
        NovaSprite sprite = {};
        sprite.id = sp.id.c_str();
        sprite.url = sp.url.c_str();
        sprite.x = sp.x;
        sprite.y = sp.y;
        sprite.opacity = sp.opacity;
        sprite.zIndex = sp.zIndex;
        vm->spriteBuffer.push_back(sprite);
    }
    
    state.sprites = vm->spriteBuffer.data();
    state.spriteCount = vm->spriteBuffer.size();
    
    vm->hudBuffer.clear();
    vm->hudBuffer.reserve(novaState.huds.size());
    
    for (const auto& hud : novaState.huds) {
        NovaHud h = {};
        h.id = hud.id.c_str();
        h.show = hud.show ? 1 : 0;
        h.content = hud.content.c_str();
        h.icon = hud.icon.c_str();
        vm->hudBuffer.push_back(h);
    }
    
    state.huds = vm->hudBuffer.data();
    state.hudCount = vm->hudBuffer.size();
    
    state.hasDialogue = novaState.dialogue.has_value() ? 1 : 0;
    if (novaState.dialogue) {
        state.dialogue.isShow = novaState.dialogue->isShow ? 1 : 0;
        state.dialogue.name = novaState.dialogue->speaker.c_str();
        state.dialogue.text = novaState.dialogue->text.c_str();
        state.dialogue.color = novaState.dialogue->color.c_str();
    }
    
    vm->choiceBuffer.clear();
    
    if (novaState.choice && novaState.choice->isShow) {
        vm->choiceBuffer.reserve(novaState.choice->options.size());
        
        for (const auto& opt : novaState.choice->options) {
            NovaChoice choice = {};
            choice.id = opt.id.c_str();
            choice.text = opt.text.c_str();
            choice.disabled = opt.disabled ? 1 : 0;
            vm->choiceBuffer.push_back(choice);
        }
    }
    
    state.choices = vm->choiceBuffer.data();
    state.choiceCount = vm->choiceBuffer.size();
    
    return state;
}

const char* nova_get_error(NovaVM* vm) {
    return vm->lastError.c_str();
}

void nova_set_state_callback(NovaVM* vm, NovaStateCallback callback, void* userData) {
    vm->stateCallback = callback;
    vm->callbackUserData = userData;
}

int nova_save_game(NovaVM* vm, const char* path) {
    if (!path) return 0;
    
    nova::GameState state;
    state.currentScene = vm->vm.currentScene();
    state.statementIndex = vm->vm.statementIndex();
    state.numberVariables = vm->vm.variables().getAllNumbers();
    state.stringVariables = vm->vm.variables().getAllStrings();
    state.boolVariables = vm->vm.variables().getAllBools();
    state.inventory = vm->vm.inventory().getAllItems();
    
    nova::SaveData save;
    save.saveId = "save_0";
    save.state = state;
    save.timestamp = std::chrono::system_clock::now();
    
    return nova::GameStateSerializer::saveToFile(std::string(path), save) ? 1 : 0;
}

int nova_load_game(NovaVM* vm, const char* path) {
    if (!path) return 0;
    
    nova::SaveData save;
    if (!nova::GameStateSerializer::loadFromFile(std::string(path), save)) {
        return 0;
    }
    
    std::string scene = save.state.currentScene;
    size_t index = save.state.statementIndex;
    
    for (const auto& [name, value] : save.state.numberVariables) {
        vm->vm.variables().set(name, value);
    }
    for (const auto& [name, value] : save.state.stringVariables) {
        vm->vm.variables().set(name, value);
    }
    for (const auto& [name, value] : save.state.boolVariables) {
        vm->vm.variables().set(name, value);
    }
    
    for (const auto& [itemId, count] : save.state.inventory) {
        vm->vm.inventory().add(itemId, count);
    }
    
    vm->vm.jumpToScene(scene);
    
    return 1;
}

size_t nova_get_variable_count(NovaVM* vm) {
    return vm->vm.variables().all().size();
}

const char* nova_get_variable_name(NovaVM* vm, size_t index) {
    return nullptr;
}

double nova_get_variable_number(NovaVM* vm, const char* name) {
    if (!name) return 0.0;
    auto val = vm->vm.variables().get(std::string(name));
    if (val && std::holds_alternative<double>(*val)) {
        return std::get<double>(*val);
    }
    return 0.0;
}

const char* nova_get_variable_string(NovaVM* vm, const char* name) {
    if (!name) return nullptr;
    auto val = vm->vm.variables().get(std::string(name));
    if (val && std::holds_alternative<std::string>(*val)) {
        vm->lastError = std::get<std::string>(*val);
        return vm->lastError.c_str();
    }
    return nullptr;
}

size_t nova_get_inventory_count(NovaVM* vm, const char* itemId) {
    if (!itemId) return 0;
    return vm->vm.inventory().count(std::string(itemId));
}

int nova_has_item(NovaVM* vm, const char* itemId) {
    if (!itemId) return 0;
    return vm->vm.inventory().has(std::string(itemId)) ? 1 : 0;
}

} // extern "C"
