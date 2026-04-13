#include "nova/renderer/nova_wasm_api.h"
#include "nova/core/game_metadata.h"
#include "nova/vm/vm.h"
#include "nova/packer/nvmp_writer.h"
#include "nova/packer/ast_serializer.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif
#include <memory>
#include <string>
#include <vector>
#include <cstdio>

namespace {

std::unique_ptr<nova::NovaVM> g_vm;
std::unique_ptr<nova::NvmpReader> g_reader;
nova::AstPtr g_program;
std::string g_last_error;

} // namespace

extern "C" {

void nova_wasm_set_reader(nova::NvmpReader* reader);

EMSCRIPTEN_KEEPALIVE
int nova_wasm_init() {
    printf("[WASM] nova_wasm_init() called\n");
    fflush(stdout);
    
    try {
        g_program.reset();
        g_last_error.clear();
        g_vm = std::make_unique<nova::NovaVM>();
        g_reader = std::make_unique<nova::NvmpReader>();
        nova_wasm_set_reader(g_reader.get());
        
        printf("[WASM] nova_wasm_init() success, VM status: %d\n", 
               static_cast<int>(g_vm->state().status));
        fflush(stdout);
        return 0;
    } catch (const std::exception& e) {
        printf("[WASM] nova_wasm_init() exception: %s\n", e.what());
        fflush(stdout);
        return -1;
    } catch (...) {
        printf("[WASM] nova_wasm_init() unknown exception\n");
        fflush(stdout);
        return -1;
    }
}

EMSCRIPTEN_KEEPALIVE
int nova_wasm_load_package(const unsigned char* data, size_t size) {
    printf("[WASM] nova_wasm_load_package() called, size=%zu\n", size);
    fflush(stdout);
    
    if (!g_reader || !g_vm) {
        g_last_error = "reader/vm not initialized";
        printf("[WASM] ERROR: g_reader or g_vm is null\n");
        fflush(stdout);
        return -1;
    }
    
    printf("[WASM] Loading from buffer...\n");
    fflush(stdout);
    
    if (!g_reader->loadFromBuffer(std::vector<uint8_t>(data, data + size))) {
        g_last_error = "NvmpReader.loadFromBuffer failed";
        printf("[WASM] ERROR: loadFromBuffer failed\n");
        fflush(stdout);
        return -1;
    }

    const nova::GameMetadata metadata = g_reader->readMetadata();
    if (metadata.valid) {
        nova::TextConfigState config;
        config.defaultFont = metadata.default_font;
        config.defaultFontSize = metadata.default_font_size;
        config.defaultTextSpeed = metadata.default_text_speed;
        g_vm->setTextConfig(config);
    }
    
    const auto& bytecode = g_reader->bytecode();
    printf("[WASM] Bytecode size: %zu bytes\n", bytecode.size());
    fflush(stdout);
    
    nova::AstDeserializer deserializer;
    g_program = deserializer.deserialize(bytecode);
    
    if (!g_program) {
        g_last_error = "AstDeserializer.deserialize returned null";
        printf("[WASM] ERROR: deserialize returned null\n");
        fflush(stdout);
        return -1;
    }
    
    printf("[WASM] Deserialization success, loading into VM...\n");
    fflush(stdout);
    
    g_vm->load(g_program);
    g_last_error.clear();
    
    printf("[WASM] VM loaded, status: %d\n", 
           static_cast<int>(g_vm->state().status));
    fflush(stdout);
    
    return 0;
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_last_error() {
    return g_last_error.c_str();
}

EMSCRIPTEN_KEEPALIVE
void nova_wasm_advance() {
    if (g_vm) {
        g_vm->advance();
    }
}

EMSCRIPTEN_KEEPALIVE
void nova_wasm_choose(int index) {
    if (g_vm && g_vm->state().choice.has_value()) {
        const auto& options = g_vm->state().choice->options;
        if (index >= 0 && static_cast<size_t>(index) < options.size()) {
            g_vm->choose(options[index].id);
        }
    }
}

EMSCRIPTEN_KEEPALIVE
int nova_wasm_get_status() {
    return nova_get_status(g_vm.get());
}

EMSCRIPTEN_KEEPALIVE
unsigned long long nova_wasm_get_runtime_state_version() {
    return nova_get_runtime_state_version(g_vm.get());
}

EMSCRIPTEN_KEEPALIVE
int nova_wasm_consume_runtime_state_change_flags() {
    return nova_consume_runtime_state_change_flags(g_vm.get());
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_default_font() {
    return nova_get_default_font(g_vm.get());
}

EMSCRIPTEN_KEEPALIVE
int nova_wasm_get_default_font_size() {
    return nova_get_default_font_size(g_vm.get());
}

EMSCRIPTEN_KEEPALIVE
int nova_wasm_get_default_text_speed() {
    return nova_get_default_text_speed(g_vm.get());
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_base_bg_path() {
    return nova_get_base_bg_path(g_vm.get());
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_bg_transition() {
    return nova_get_bg_transition(g_vm.get());
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_base_sprite_path() {
    return nova_get_base_sprite_path(g_vm.get());
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_base_audio_path() {
    return nova_get_base_audio_path(g_vm.get());
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_export_runtime_state_json(size_t* outSize) {
    return nova_export_runtime_state_json(g_vm.get(), outSize);
}

EMSCRIPTEN_KEEPALIVE
size_t nova_wasm_get_asset_size(const char* name) {
    return ::nova_get_asset_size(nullptr, name);
}

EMSCRIPTEN_KEEPALIVE
int nova_wasm_get_asset_bytes(const char* name, unsigned char* buffer, size_t bufferSize) {
    return ::nova_get_asset_bytes(nullptr, name, buffer, bufferSize);
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_export_save_json(size_t* outSize) {
    return ::nova_export_save_json(g_vm.get(), outSize);
}

EMSCRIPTEN_KEEPALIVE
void* nova_wasm_export_save_binary(size_t* outSize) {
    return ::nova_export_save_binary(g_vm.get(), outSize);
}

EMSCRIPTEN_KEEPALIVE
int nova_wasm_import_save_json(const char* json, size_t size) {
    return ::nova_import_save_json(g_vm.get(), json, size);
}

EMSCRIPTEN_KEEPALIVE
int nova_wasm_import_save_binary(const unsigned char* data, size_t size) {
    return ::nova_import_save_binary(g_vm.get(), data, size);
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_bg() {
    if (g_vm && g_vm->state().bg.has_value()) {
        return g_vm->state().bg->c_str();
    }
    return "";
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_bgm() {
    if (g_vm && g_vm->state().bgm.has_value()) {
        return g_vm->state().bgm->c_str();
    }
    return "";
}

EMSCRIPTEN_KEEPALIVE
double nova_wasm_get_bgm_volume() {
    if (g_vm) {
        return g_vm->state().bgmVolume;
    }
    return 1.0;
}

EMSCRIPTEN_KEEPALIVE
int nova_wasm_get_bgm_loop() {
    if (g_vm) {
        return g_vm->state().bgmLoop ? 1 : 0;
    }
    return 1;
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_dialogue_speaker() {
    if (g_vm && g_vm->state().dialogue.has_value()) {
        return g_vm->state().dialogue->speaker.c_str();
    }
    return "";
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_dialogue_text() {
    if (g_vm && g_vm->state().dialogue.has_value()) {
        return g_vm->state().dialogue->text.c_str();
    }
    return "";
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_dialogue_color() {
    if (g_vm && g_vm->state().dialogue.has_value()) {
        return g_vm->state().dialogue->color.c_str();
    }
    return "";
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_dialogue_emotion() {
    if (g_vm && g_vm->state().dialogue.has_value()) {
        return g_vm->state().dialogue->emotion.c_str();
    }
    return "";
}

EMSCRIPTEN_KEEPALIVE
int nova_wasm_has_dialogue() {
    return (g_vm && g_vm->state().dialogue.has_value() && 
            g_vm->state().dialogue->isShow) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
int nova_wasm_get_choice_count() {
    if (g_vm && g_vm->state().choice.has_value()) {
        return static_cast<int>(g_vm->state().choice->options.size());
    }
    return 0;
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_choice_text(int index) {
    if (g_vm && g_vm->state().choice.has_value()) {
        const auto& options = g_vm->state().choice->options;
        if (index >= 0 && static_cast<size_t>(index) < options.size()) {
            return options[index].text.c_str();
        }
    }
    return "";
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_choice_id(int index) {
    if (g_vm && g_vm->state().choice.has_value()) {
        const auto& options = g_vm->state().choice->options;
        if (index >= 0 && static_cast<size_t>(index) < options.size()) {
            return options[index].id.c_str();
        }
    }
    return "";
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_choice_target(int index) {
    if (g_vm && g_vm->state().choice.has_value()) {
        const auto& options = g_vm->state().choice->options;
        if (index >= 0 && static_cast<size_t>(index) < options.size()) {
            return options[index].target.c_str();
        }
    }
    return "";
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_choice_question() {
    if (g_vm && g_vm->state().choice.has_value()) {
        return g_vm->state().choice->question.c_str();
    }
    return "";
}

EMSCRIPTEN_KEEPALIVE
int nova_wasm_is_choice_disabled(int index) {
    if (g_vm && g_vm->state().choice.has_value()) {
        const auto& options = g_vm->state().choice->options;
        if (index >= 0 && static_cast<size_t>(index) < options.size()) {
            return options[index].disabled ? 1 : 0;
        }
    }
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int nova_wasm_has_choices() {
    return (g_vm && g_vm->state().choice.has_value() && 
            g_vm->state().choice->isShow) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
int nova_wasm_get_sprite_count() {
    if (g_vm) {
        return static_cast<int>(g_vm->state().sprites.size());
    }
    return 0;
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_sprite_url(int index) {
    if (g_vm && index >= 0 && static_cast<size_t>(index) < g_vm->state().sprites.size()) {
        const auto& sprite = g_vm->state().sprites[index];
        return sprite.url ? sprite.url->c_str() : "";
    }
    return "";
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_sprite_x(int index) {
    if (g_vm && index >= 0 && static_cast<size_t>(index) < g_vm->state().sprites.size()) {
        const auto& sprite = g_vm->state().sprites[index];
        return sprite.x ? sprite.x->c_str() : "";
    }
    return "";
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_sprite_y(int index) {
    if (g_vm && index >= 0 && static_cast<size_t>(index) < g_vm->state().sprites.size()) {
        const auto& sprite = g_vm->state().sprites[index];
        return sprite.y ? sprite.y->c_str() : "";
    }
    return "";
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_sprite_opacity(int index) {
    if (g_vm && index >= 0 && static_cast<size_t>(index) < g_vm->state().sprites.size()) {
        static std::string value;
        const auto& sprite = g_vm->state().sprites[index];
        if (!sprite.opacity) return "";
        value = std::to_string(*sprite.opacity);
        return value.c_str();
    }
    return "";
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_sprite_z_index(int index) {
    if (g_vm && index >= 0 && static_cast<size_t>(index) < g_vm->state().sprites.size()) {
        static std::string value;
        const auto& sprite = g_vm->state().sprites[index];
        if (!sprite.zIndex) return "";
        value = std::to_string(*sprite.zIndex);
        return value.c_str();
    }
    return "";
}

EMSCRIPTEN_KEEPALIVE
int nova_wasm_get_sfx_count() {
    return g_vm ? static_cast<int>(g_vm->state().sfx.size()) : 0;
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_sfx_id(int index) {
    if (g_vm && index >= 0 && static_cast<size_t>(index) < g_vm->state().sfx.size()) {
        return g_vm->state().sfx[index].id.c_str();
    }
    return "";
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_sfx_path(int index) {
    if (g_vm && index >= 0 && static_cast<size_t>(index) < g_vm->state().sfx.size()) {
        return g_vm->state().sfx[index].path.c_str();
    }
    return "";
}

EMSCRIPTEN_KEEPALIVE
double nova_wasm_get_sfx_volume(int index) {
    if (g_vm && index >= 0 && static_cast<size_t>(index) < g_vm->state().sfx.size()) {
        return g_vm->state().sfx[index].volume;
    }
    return 1.0;
}

EMSCRIPTEN_KEEPALIVE
int nova_wasm_get_sfx_loop(int index) {
    if (g_vm && index >= 0 && static_cast<size_t>(index) < g_vm->state().sfx.size()) {
        return g_vm->state().sfx[index].loop ? 1 : 0;
    }
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int nova_wasm_get_theme_count() {
    return g_vm ? static_cast<int>(g_vm->themeDefinitions().size()) : 0;
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_theme_name(int index) {
    if (!g_vm || index < 0) return "";
    static std::vector<std::string> names;
    names.clear();
    for (const auto& entry : g_vm->themeDefinitions()) {
        names.push_back(entry.first);
    }
    if (static_cast<size_t>(index) >= names.size()) return "";
    return names[static_cast<size_t>(index)].c_str();
}

EMSCRIPTEN_KEEPALIVE
int nova_wasm_get_theme_property_count(const char* themeId) {
    if (!g_vm || !themeId) return 0;
    auto it = g_vm->themeDefinitions().find(themeId);
    if (it == g_vm->themeDefinitions().end()) return 0;
    return static_cast<int>(it->second.properties.size());
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_theme_property_key(const char* themeId, int index) {
    if (!g_vm || !themeId || index < 0) return "";
    auto it = g_vm->themeDefinitions().find(themeId);
    if (it == g_vm->themeDefinitions().end()) return "";
    static std::vector<std::string> keys;
    keys.clear();
    for (const auto& entry : it->second.properties) {
        keys.push_back(entry.first);
    }
    if (static_cast<size_t>(index) >= keys.size()) return "";
    return keys[static_cast<size_t>(index)].c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_theme_property_value(const char* themeId, const char* key) {
    if (!g_vm || !themeId || !key) return "";
    auto it = g_vm->themeDefinitions().find(themeId);
    if (it == g_vm->themeDefinitions().end()) return "";
    auto propIt = it->second.properties.find(key);
    if (propIt == it->second.properties.end()) return "";
    return propIt->second.c_str();
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_sprite_position(int index) {
    if (g_vm && index >= 0 && static_cast<size_t>(index) < g_vm->state().sprites.size()) {
        const auto& sprite = g_vm->state().sprites[index];
        return sprite.position ? sprite.position->c_str() : "";
    }
    return "";
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_ending_id() {
    if (g_vm && g_vm->state().ending.has_value()) {
        return g_vm->state().ending->c_str();
    }
    return nullptr;
}

EMSCRIPTEN_KEEPALIVE
const char* nova_wasm_get_ending_title() {
    if (g_vm && g_vm->state().endingTitle.has_value()) {
        return g_vm->state().endingTitle->c_str();
    }
    return nullptr;
}

} // extern "C"
