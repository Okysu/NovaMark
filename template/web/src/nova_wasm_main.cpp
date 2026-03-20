#include "nova/renderer/nova_c_api.h"
#include "nova/renderer/nova_wasm_api.h"
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

namespace {

std::unique_ptr<nova::NovaVM> g_vm;
std::unique_ptr<nova::NvmpReader> g_reader;

} // namespace

extern "C" {

void nova_wasm_set_reader(nova::NvmpReader* reader);

EMSCRIPTEN_KEEPALIVE
int nova_wasm_init() {
    g_vm = std::make_unique<nova::NovaVM>();
    g_reader = std::make_unique<nova::NvmpReader>();
    nova_wasm_set_reader(g_reader.get());
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int nova_wasm_load_package(const unsigned char* data, size_t size) {
    if (!g_reader || !g_vm) {
        return -1;
    }
    
    if (!g_reader->loadFromBuffer(std::vector<uint8_t>(data, data + size))) {
        return -1;
    }
    
    const auto& bytecode = g_reader->bytecode();
    nova::AstDeserializer deserializer;
    auto program = deserializer.deserialize(bytecode);
    
    if (!program) {
        return -1;
    }
    
    g_vm->load(program.get());
    return 0;
}

EMSCRIPTEN_KEEPALIVE
void nova_wasm_start() {
    if (g_vm) {
        g_vm->run();
    }
}

EMSCRIPTEN_KEEPALIVE
void nova_wasm_next() {
    if (g_vm) {
        g_vm->step();
    }
}

EMSCRIPTEN_KEEPALIVE
void nova_wasm_select_choice(int index) {
    if (g_vm && g_vm->state().choice.has_value()) {
        const auto& options = g_vm->state().choice->options;
        if (index >= 0 && static_cast<size_t>(index) < options.size()) {
            g_vm->selectChoice(index);
        }
    }
}

EMSCRIPTEN_KEEPALIVE
int nova_wasm_get_status() {
    return nova_get_status(g_vm.get());
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
int nova_wasm_import_save_json(const char* json, size_t size) {
    return ::nova_import_save_json(g_vm.get(), json, size);
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
        return g_vm->state().sprites[index].url.c_str();
    }
    return "";
}

EMSCRIPTEN_KEEPALIVE
double nova_wasm_get_sprite_x(int index) {
    if (g_vm && index >= 0 && static_cast<size_t>(index) < g_vm->state().sprites.size()) {
        return g_vm->state().sprites[index].x;
    }
    return 0.0;
}

EMSCRIPTEN_KEEPALIVE
double nova_wasm_get_sprite_y(int index) {
    if (g_vm && index >= 0 && static_cast<size_t>(index) < g_vm->state().sprites.size()) {
        return g_vm->state().sprites[index].y;
    }
    return 0.0;
}

EMSCRIPTEN_KEEPALIVE
double nova_wasm_get_sprite_opacity(int index) {
    if (g_vm && index >= 0 && static_cast<size_t>(index) < g_vm->state().sprites.size()) {
        return g_vm->state().sprites[index].opacity;
    }
    return 1.0;
}

EMSCRIPTEN_KEEPALIVE
int nova_wasm_get_sprite_z_index(int index) {
    if (g_vm && index >= 0 && static_cast<size_t>(index) < g_vm->state().sprites.size()) {
        return g_vm->state().sprites[index].zIndex;
    }
    return 0;
}

} // extern "C"
