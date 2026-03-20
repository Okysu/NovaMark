#include "nova/renderer/nova_wasm_api.h"
#include "nova/vm/vm.h"
#include "nova/vm/state.h"
#include "nova/vm/serializer.h"
#include "nova/vm/game_state.h"
#include "nova/vm/save_data.h"
#include "nova/packer/nvmp_writer.h"
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

size_t nova_get_asset_size(void* vm, const char* name) {
    if (!g_current_reader || !name) return 0;
    
    std::vector<uint8_t> data;
    return g_current_reader->getAsset(name, data) ? data.size() : 0;
}

int nova_get_asset_bytes(void* vm, const char* name, 
                          unsigned char* buffer, size_t bufferSize) {
    if (!g_current_reader || !name || !buffer) return -1;
    
    std::vector<uint8_t> data;
    if (!g_current_reader->getAsset(name, data)) return -1;
    
    size_t copySize = std::min(bufferSize, data.size());
    std::memcpy(buffer, data.data(), copySize);
    return static_cast<int>(copySize);
}

int nova_has_asset(void* vm, const char* name) {
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

int nova_import_save_json(void* vm, const char* json, size_t size) {
    auto* nova_vm = cast_vm(vm);
    if (!nova_vm || !json || size == 0) return -1;
    
    std::string jsonStr(json, size);
    nova::SaveData save;
    
    if (!nova::GameStateSerializer::deserializeSave(jsonStr, save)) return -1;
    
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
        case nova::VMStatus::WaitingInput:  return 2;
        case nova::VMStatus::WaitingDelay:  return 2;
        case nova::VMStatus::Ended:         return 3;
    }
    return 3;
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

void nova_wasm_set_reader(nova::NvmpReader* reader) {
    g_current_reader = reader;
}

} // extern "C"
