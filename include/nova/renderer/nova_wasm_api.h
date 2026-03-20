#pragma once

/// @brief NovaMark WebAssembly API
/// 提供给 Web 渲染器使用的额外 API，用于资源读取和存档导入导出

#include <stddef.h>

#ifdef __EMSCRIPTEN__
    #define NOVA_WASM_API __attribute__((visibility("default")))
#else
    #define NOVA_WASM_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ========== 资源访问 API ==========

/// @brief 获取资源大小
/// @param vm VM 实例
/// @param name 资源名称（相对路径，如 "assets/bg/forest.png"）
/// @return 资源大小（字节），0 表示资源不存在
NOVA_WASM_API size_t nova_get_asset_size(void* vm, const char* name);

/// @brief 读取资源数据
/// @param vm VM 实例
/// @param name 资源名称
/// @param buffer 输出缓冲区
/// @param bufferSize 缓冲区大小
/// @return 实际读取的字节数，-1 表示失败
NOVA_WASM_API int nova_get_asset_bytes(void* vm, const char* name, 
                                        unsigned char* buffer, size_t bufferSize);

/// @brief 检查资源是否存在
/// @param vm VM 实例
/// @param name 资源名称
/// @return 1 表示存在，0 表示不存在
NOVA_WASM_API int nova_has_asset(void* vm, const char* name);

// ========== 存档导入导出 API ==========

/// @brief 导出存档为 JSON
/// @param vm VM 实例
/// @param outSize 输出 JSON 大小
/// @return JSON 字符串指针（需要调用 nova_wasm_free 释放）
NOVA_WASM_API const char* nova_export_save_json(void* vm, size_t* outSize);

/// @brief 从 JSON 导入存档
/// @param vm VM 实例
/// @param json JSON 字符串
/// @param size JSON 大小
/// @return 0 表示成功，-1 表示失败
NOVA_WASM_API int nova_import_save_json(void* vm, const char* json, size_t size);

// ========== 内存管理 API ==========

/// @brief 释放由 WASM API 分配的内存
/// @param ptr 要释放的指针
NOVA_WASM_API void nova_wasm_free(void* ptr);

// ========== 扩展状态 API ==========

/// @brief 获取当前场景名
/// @param vm VM 实例
/// @return 场景名（不需要释放，内部管理）
NOVA_WASM_API const char* nova_get_current_scene(void* vm);

/// @brief 获取当前标签名
/// @param vm VM 实例
/// @return 标签名（不需要释放，内部管理）
NOVA_WASM_API const char* nova_get_current_label(void* vm);

/// @brief 获取游戏状态（Running/WaitingChoice/Ended 等）
/// @param vm VM 实例
/// @return 状态码：0=Running, 1=WaitingChoice, 2=WaitingInput, 3=Ended
NOVA_WASM_API int nova_get_status(void* vm);

/// @brief 检查游戏是否结束
/// @param vm VM 实例
/// @return 1 表示已结束，0 表示未结束
NOVA_WASM_API int nova_is_ended(void* vm);

/// @brief 获取结局 ID（如果游戏已结束且有结局）
/// @param vm VM 实例
/// @return 结局 ID，NULL 表示无结局
NOVA_WASM_API const char* nova_get_ending_id(void* vm);

#ifdef __cplusplus
}
#endif
