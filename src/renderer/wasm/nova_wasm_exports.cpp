#include "nova/renderer/nova_wasm_api.h"
#include "nova/vm/vm.h"
#include "nova/vm/state.h"
#include "nova/vm/serializer.h"
#include "nova/vm/game_state.h"
#include "nova/vm/save_data.h"
#include "nova/packer/nvmp_writer.h"
#include <nlohmann/json.hpp>
#include <cstring>
#include <cstdlib>
#include <memory>

namespace {

nova::NvmpReader* g_current_reader = nullptr;

nova::NovaVM* cast_vm(void* vm) {
    return static_cast<nova::NovaVM*>(vm);
}

} // namespace

extern "C" {

void nova_wasm_set_reader(nova::NvmpReader* reader);

// ========== 资源访问 API ==========

size_t nova_get_asset_size(void*, const char* name) {
    if (!g_current_reader || !name) return 0;
    
    std::vector<uint8_t> data;
    return g_current_reader->getAsset(name, data) ? data.size() : 0;
}

int nova_get_asset_bytes(void*, const char* name, 
                          unsigned char* buffer, size_t bufferSize) {
    if (!g_current_reader || !name || !buffer) return -1;
    
    std::vector<uint8_t> data;
    if (!g_current_reader->getAsset(name, data)) return -1;
    
    size_t copySize = std::min(bufferSize, data.size());
    std::memcpy(buffer, data.data(), copySize);
    return static_cast<int>(copySize);
}

int nova_has_asset(void*, const char* name) {
    if (!g_current_reader || !name) return 0;
    
    std::vector<uint8_t> data;
    return g_current_reader->getAsset(name, data) ? 1 : 0;
}

// ========== 存档导入导出 API ==========

const char* nova_export_save_json(void* vm, size_t* outSize) {
    auto* nova_vm = cast_vm(vm);
    if (!nova_vm || !outSize) {
        if (outSize) *outSize = 0;
        return nullptr;
    }
    
    nova::GameState state = nova_vm->captureState();
    
    nova::SaveData save;
    save.saveId = "wasm-export";
    save.label = "Web Save";
    save.timestamp = std::chrono::system_clock::now();
    save.state = state;
    
    std::string json = nova::GameStateSerializer::serializeSave(save);
    
    char* result = static_cast<char*>(std::malloc(json.size() + 1));
    if (!result) {
        *outSize = 0;
        return nullptr;
    }
    
    std::memcpy(result, json.c_str(), json.size() + 1);
    *outSize = json.size();
    return result;
}

void* nova_export_save_binary(void* vm, size_t* outSize) {
    auto* nova_vm = cast_vm(vm);
    if (!nova_vm || !outSize) {
        if (outSize) *outSize = 0;
        return nullptr;
    }

    nova::GameState state = nova_vm->captureState();

    nova::SaveData save;
    save.saveId = "wasm-export";
    save.label = "Web Save";
    save.timestamp = std::chrono::system_clock::now();
    save.state = state;

    auto bytes = nova::GameStateSerializer::serializeSaveBinary(save);
    void* result = std::malloc(bytes.size());
    if (!result) {
        *outSize = 0;
        return nullptr;
    }

    std::memcpy(result, bytes.data(), bytes.size());
    *outSize = bytes.size();
    return result;
}

int nova_import_save_json(void* vm, const char* json, size_t size) {
    auto* nova_vm = cast_vm(vm);
    if (!nova_vm || !json || size == 0) return -1;
    
    std::string jsonStr(json, size);
    nova::SaveData save;
    
    if (!nova::GameStateSerializer::deserializeSave(jsonStr, save)) return -1;
    
    return nova_vm->loadSave(save.state) ? 0 : -1;
}

int nova_import_save_binary(void* vm, const unsigned char* data, size_t size) {
    auto* nova_vm = cast_vm(vm);
    if (!nova_vm || !data || size == 0) return -1;

    nova::SaveData save;
    std::vector<uint8_t> bytes(data, data + size);
    if (!nova::GameStateSerializer::deserializeSaveBinary(bytes, save)) return -1;

    return nova_vm->loadSave(save.state) ? 0 : -1;
}

// ========== 内存管理 API ==========

void nova_wasm_free(void* ptr) {
    if (ptr) std::free(ptr);
}

// ========== 扩展状态 API ==========

const char* nova_get_current_scene(void* vm) {
    auto* nova_vm = cast_vm(vm);
    return nova_vm ? nova_vm->state().currentScene.c_str() : "";
}

const char* nova_get_current_label(void* vm) {
    auto* nova_vm = cast_vm(vm);
    return nova_vm ? nova_vm->state().currentLabel.c_str() : "";
}

int nova_get_status(void* vm) {
    auto* nova_vm = cast_vm(vm);
    if (!nova_vm) return 3;
    
    switch (nova_vm->state().status) {
        case nova::VMStatus::Running:       return 0;
        case nova::VMStatus::WaitingChoice: return 1;
        case nova::VMStatus::Ended:         return 2;
    }
    return 2;
}

unsigned long long nova_get_runtime_state_version(void* vm) {
    auto* nova_vm = cast_vm(vm);
    return nova_vm ? static_cast<unsigned long long>(nova_vm->runtimeStateVersion()) : 0ULL;
}

int nova_consume_runtime_state_change_flags(void* vm) {
    auto* nova_vm = cast_vm(vm);
    return nova_vm ? nova_vm->consumeRuntimeStateChangeFlags() : 0;
}

int nova_is_ended(void* vm) {
    auto* nova_vm = cast_vm(vm);
    return (!nova_vm || nova_vm->state().status == nova::VMStatus::Ended) ? 1 : 0;
}

const char* nova_get_ending_id(void* vm) {
    auto* nova_vm = cast_vm(vm);
    if (!nova_vm || !nova_vm->state().ending.has_value()) return nullptr;
    return nova_vm->state().ending->c_str();
}

const char* nova_get_default_font(void* vm) {
    auto* nova_vm = cast_vm(vm);
    if (!nova_vm) return "sans-serif";
    return nova_vm->state().textConfig.defaultFont.c_str();
}

int nova_get_default_font_size(void* vm) {
    auto* nova_vm = cast_vm(vm);
    if (!nova_vm) return 24;
    return nova_vm->state().textConfig.defaultFontSize;
}

int nova_get_default_text_speed(void* vm) {
    auto* nova_vm = cast_vm(vm);
    if (!nova_vm) return 50;
    return nova_vm->state().textConfig.defaultTextSpeed;
}

const char* nova_get_base_bg_path(void*) {
    static std::string value;
    value = g_current_reader ? g_current_reader->readMetadata().base_bg_path : "";
    return value.c_str();
}

const char* nova_get_base_sprite_path(void*) {
    static std::string value;
    value = g_current_reader ? g_current_reader->readMetadata().base_sprite_path : "";
    return value.c_str();
}

const char* nova_get_base_audio_path(void*) {
    static std::string value;
    value = g_current_reader ? g_current_reader->readMetadata().base_audio_path : "";
    return value.c_str();
}

const char* nova_export_runtime_state_json(void* vm, size_t* outSize) {
    auto* nova_vm = cast_vm(vm);
    if (!outSize) {
        return nullptr;
    }

    *outSize = 0;
    if (!nova_vm) {
        return nullptr;
    }

    nlohmann::json json;

    const auto& state = nova_vm->state();
    json["status"] = static_cast<int>(state.status);
    json["currentScene"] = state.currentScene;
    json["currentLabel"] = state.currentLabel;
    json["textConfig"] = {
        {"defaultFont", state.textConfig.defaultFont},
        {"defaultFontSize", state.textConfig.defaultFontSize},
        {"defaultTextSpeed", state.textConfig.defaultTextSpeed}
    };

    json["variables"] = {
        {"numbers", nova_vm->variables().getAllNumbers()},
        {"strings", nova_vm->variables().getAllStrings()},
        {"bools", nova_vm->variables().getAllBools()}
    };

    json["inventory"] = nova_vm->inventory().getAllItems();

    json["itemDefinitions"] = nlohmann::json::object();
    for (const auto& [id, item] : nova_vm->itemDefinitions()) {
        json["itemDefinitions"][id] = {
            {"id", item.id},
            {"name", item.name},
            {"description", item.description},
            {"icon", item.icon},
            {"defaultValue", item.defaultValue}
        };
    }

    json["characterDefinitions"] = nlohmann::json::object();
    for (const auto& [id, ch] : nova_vm->characterDefinitions()) {
        json["characterDefinitions"][id] = {
            {"id", ch.id},
            {"color", ch.color},
            {"description", ch.description},
            {"sprites", ch.sprites}
        };
    }

    json["inventoryItems"] = nlohmann::json::array();
    for (const auto& [id, count] : nova_vm->inventory().getAllItems()) {
        auto it = nova_vm->itemDefinitions().find(id);
        if (it != nova_vm->itemDefinitions().end()) {
            json["inventoryItems"].push_back({
                {"id", id},
                {"name", it->second.name},
                {"description", it->second.description},
                {"icon", it->second.icon},
                {"defaultValue", it->second.defaultValue},
                {"count", count}
            });
        } else {
            json["inventoryItems"].push_back({
                {"id", id},
                {"name", id},
                {"description", ""},
                {"icon", ""},
                {"defaultValue", ""},
                {"count", count}
            });
        }
    }

    std::string text = json.dump();
    char* result = static_cast<char*>(std::malloc(text.size() + 1));
    if (!result) {
        return nullptr;
    }

    std::memcpy(result, text.c_str(), text.size() + 1);
    *outSize = text.size();
    return result;
}

void nova_wasm_set_reader(nova::NvmpReader* reader) {
    g_current_reader = reader;
}

} // extern "C"
