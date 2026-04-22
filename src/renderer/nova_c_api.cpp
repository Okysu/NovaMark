#include "nova/renderer/nova_c_api.h"
#include "nova/ast/ast_snapshot.h"
#include "nova/packer/packer.h"
#include "nova/vm/vm.h"
#include "nova/vm/serializer.h"
#include "nova/packer/nvmp_writer.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <memory>

struct NovaVM {
    nova::NovaVM vm;
    std::string lastError;
    std::string textConfigFont;
    NovaStateCallback stateCallback = nullptr;
    void* callbackUserData = nullptr;
    std::vector<NovaSprite> spriteBuffer;
    std::vector<std::string> spriteFieldCache;
    std::vector<NovaSfx> sfxBuffer;
    std::vector<NovaChoice> choiceBuffer;
    std::vector<std::string> variableNameCache;
    std::vector<std::string> themeNameCache;
    std::unordered_map<std::string, std::vector<std::string>> themePropertyKeyCache;
};

namespace {

char* copy_string(const std::string& value) {
    char* buffer = static_cast<char*>(std::malloc(value.size() + 1));
    if (!buffer) {
        return nullptr;
    }

    std::memcpy(buffer, value.c_str(), value.size() + 1);
    return buffer;
}

}

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

void nova_advance(NovaVM* vm) {
    vm->vm.advance();
    
    if (vm->stateCallback) {
        NovaState state = nova_get_state(vm);
        vm->stateCallback(state, vm->callbackUserData);
    }
}

void nova_choose(NovaVM* vm, const char* choiceId) {
    if (choiceId) {
        vm->vm.choose(std::string(choiceId));
        
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

    if (novaState.bgTransition) {
        vm->lastError = *novaState.bgTransition;
        state.bgTransition = vm->lastError.c_str();
    }
     
    if (novaState.bgm) {
        vm->lastError = *novaState.bgm;
        state.bgm = vm->lastError.c_str();
    }

    state.bgmVolume = novaState.bgmVolume;
    state.bgmLoop = novaState.bgmLoop ? 1 : 0;
    
    vm->spriteBuffer.clear();
    vm->spriteBuffer.reserve(novaState.sprites.size());
    vm->spriteFieldCache.clear();
    vm->spriteFieldCache.reserve(novaState.sprites.size() * 5);
    
    for (const auto& sp : novaState.sprites) {
        NovaSprite sprite = {};
        sprite.id = sp.id.c_str();
        sprite.url = sp.url ? sp.url->c_str() : nullptr;
        sprite.x = sp.x ? sp.x->c_str() : nullptr;
        sprite.y = sp.y ? sp.y->c_str() : nullptr;
        sprite.position = sp.position ? sp.position->c_str() : nullptr;
        if (sp.opacity) {
            vm->spriteFieldCache.push_back(std::to_string(*sp.opacity));
            sprite.opacity = vm->spriteFieldCache.back().c_str();
        }
        if (sp.zIndex) {
            vm->spriteFieldCache.push_back(std::to_string(*sp.zIndex));
            sprite.zIndex = vm->spriteFieldCache.back().c_str();
        }
        vm->spriteBuffer.push_back(sprite);
    }
    
    state.sprites = vm->spriteBuffer.data();
    state.spriteCount = vm->spriteBuffer.size();

    vm->sfxBuffer.clear();
    vm->sfxBuffer.reserve(novaState.sfx.size());
    for (const auto& sfx : novaState.sfx) {
        NovaSfx sfxState = {};
        sfxState.id = sfx.id.c_str();
        sfxState.path = sfx.path.c_str();
        sfxState.loop = sfx.loop ? 1 : 0;
        sfxState.volume = sfx.volume;
        vm->sfxBuffer.push_back(sfxState);
    }

    state.sfx = vm->sfxBuffer.data();
    state.sfxCount = vm->sfxBuffer.size();
    
    state.hasDialogue = novaState.dialogue.has_value() ? 1 : 0;
    if (novaState.dialogue) {
        state.dialogue.isShow = novaState.dialogue->isShow ? 1 : 0;
        state.dialogue.name = novaState.dialogue->speaker.c_str();
        state.dialogue.text = novaState.dialogue->text.c_str();
        state.dialogue.emotion = novaState.dialogue->emotion.c_str();
        state.dialogue.color = novaState.dialogue->color.c_str();
    }
    
    vm->choiceBuffer.clear();

    state.hasChoices = (novaState.choice && novaState.choice->isShow) ? 1 : 0;
    state.choiceQuestion = nullptr;
    if (novaState.choice && novaState.choice->isShow) {
        vm->choiceBuffer.reserve(novaState.choice->options.size());
        state.choiceQuestion = novaState.choice->question.c_str();
         
        for (const auto& opt : novaState.choice->options) {
            NovaChoice choice = {};
            choice.id = opt.id.c_str();
            choice.text = opt.text.c_str();
            choice.target = opt.target.c_str();
            choice.disabled = opt.disabled ? 1 : 0;
            vm->choiceBuffer.push_back(choice);
        }
    }
    
    state.choices = vm->choiceBuffer.data();
    state.choiceCount = vm->choiceBuffer.size();
    
    return state;
}

size_t nova_get_theme_count(NovaVM* vm) {
    return vm ? vm->vm.themeDefinitions().size() : 0;
}

const char* nova_get_theme_name(NovaVM* vm, size_t index) {
    if (!vm) return nullptr;
    vm->themeNameCache.clear();
    vm->themeNameCache.reserve(vm->vm.themeDefinitions().size());
    for (const auto& entry : vm->vm.themeDefinitions()) {
        vm->themeNameCache.push_back(entry.first);
    }
    if (index >= vm->themeNameCache.size()) return nullptr;
    return vm->themeNameCache[index].c_str();
}

size_t nova_get_theme_property_count(NovaVM* vm, const char* themeId) {
    if (!vm || !themeId) return 0;
    auto it = vm->vm.themeDefinitions().find(themeId);
    if (it == vm->vm.themeDefinitions().end()) return 0;
    return it->second.properties.size();
}

const char* nova_get_theme_property_key(NovaVM* vm, const char* themeId, size_t index) {
    if (!vm || !themeId) return nullptr;
    auto it = vm->vm.themeDefinitions().find(themeId);
    if (it == vm->vm.themeDefinitions().end()) return nullptr;
    auto& cache = vm->themePropertyKeyCache[themeId];
    if (cache.empty()) {
        cache.reserve(it->second.properties.size());
        for (const auto& entry : it->second.properties) {
            cache.push_back(entry.first);
        }
    }
    if (index >= cache.size()) return nullptr;
    return cache[index].c_str();
}

const char* nova_get_theme_property_value(NovaVM* vm, const char* themeId, const char* key) {
    if (!vm || !themeId || !key) return nullptr;
    auto it = vm->vm.themeDefinitions().find(themeId);
    if (it == vm->vm.themeDefinitions().end()) return nullptr;
    auto propIt = it->second.properties.find(key);
    if (propIt == it->second.properties.end()) return nullptr;
    return propIt->second.c_str();
}

NovaTextConfig nova_get_text_config(NovaVM* vm) {
    NovaTextConfig config = {};

    const auto& textConfig = vm->vm.state().textConfig;
    vm->textConfigFont = textConfig.defaultFont;

    config.defaultFont = vm->textConfigFont.c_str();
    config.defaultFontSize = textConfig.defaultFontSize;
    config.defaultTextSpeed = textConfig.defaultTextSpeed;
    return config;
}

const char* nova_get_error(NovaVM* vm) {
    return vm->lastError.c_str();
}

void nova_set_state_callback(NovaVM* vm, NovaStateCallback callback, void* userData) {
    vm->stateCallback = callback;
    vm->callbackUserData = userData;
}

int nova_save_snapshot_file(NovaVM* vm, const char* path) {
    if (!path) return 0;

    nova::GameState state = vm->vm.captureState();
    
    nova::SaveData save;
    save.saveId = "save_0";
    save.state = state;
    save.timestamp = std::chrono::system_clock::now();
    
    return nova::GameStateSerializer::saveToFile(std::string(path), save) ? 1 : 0;
}

int nova_load_snapshot_file(NovaVM* vm, const char* path) {
    if (!path) return 0;
    
    nova::SaveData save;
    if (!nova::GameStateSerializer::loadFromFile(std::string(path), save)) {
        return 0;
    }
    
    return vm->vm.loadSave(save.state) ? 1 : 0;
}

size_t nova_get_variable_count(NovaVM* vm) {
    return vm->vm.variables().all().size();
}

const char* nova_get_variable_name(NovaVM* vm, size_t index) {
    if (!vm) return nullptr;

    vm->variableNameCache.clear();
    vm->variableNameCache.reserve(vm->vm.variables().all().size());
    for (const auto& entry : vm->vm.variables().all()) {
        vm->variableNameCache.push_back(entry.first);
    }

    if (index >= vm->variableNameCache.size()) {
        return nullptr;
    }

    std::sort(vm->variableNameCache.begin(), vm->variableNameCache.end());
    return vm->variableNameCache[index].c_str();
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

char* nova_export_ast_snapshot_from_path(const char* path) {
    if (!path) {
        return nullptr;
    }

    try {
        return copy_string(nova::export_ast_snapshot_string_from_path(path));
    } catch (...) {
        return nullptr;
    }
}

char* nova_export_ast_snapshot_from_scripts(const NovaMemoryScript* scripts, size_t count) {
    if (!scripts || count == 0) {
        return nullptr;
    }

    std::vector<nova::MemoryScript> memoryScripts;
    memoryScripts.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        memoryScripts.push_back({
            scripts[i].path ? scripts[i].path : "",
            scripts[i].content ? scripts[i].content : ""
        });
    }

    try {
        return copy_string(nova::export_ast_snapshot_string_from_scripts(memoryScripts));
    } catch (...) {
        return nullptr;
    }
}

char* nova_ast_snapshot_to_source_files(const char* snapshotJson) {
    if (!snapshotJson) {
        return nullptr;
    }

    try {
        return copy_string(nova::ast_snapshot_to_source_files_json(snapshotJson));
    } catch (...) {
        return nullptr;
    }
}

void nova_string_free(char* str) {
    if (str) {
        std::free(str);
    }
}

} // extern "C"
