#pragma once

#include <stddef.h>

#ifdef _WIN32
    #ifdef NOVA_EXPORTS
        #define NOVA_API __declspec(dllexport)
    #else
        #define NOVA_API __declspec(dllimport)
    #endif
#else
    #define NOVA_API __attribute__((visibility("default")))
#endif

extern "C" {

typedef struct NovaVM NovaVM;

typedef struct {
    const char* path;
    const char* content;
} NovaMemoryScript;

typedef struct {
    const char* id;
    const char* url;
    const char* x;
    const char* y;
    const char* position;
    const char* opacity;
    const char* zIndex;
} NovaSprite;

typedef struct {
    const char* text;
    const char* style;
} NovaTextSegment;

typedef struct {
    int isShow;
    const char* name;
    const char* text;
    const NovaTextSegment* segments;
    size_t segmentCount;
    const char* emotion;
    const char* color;
} NovaDialogue;

typedef struct {
    const char* defaultFont;
    int defaultFontSize;
    int defaultTextSpeed;
} NovaTextConfig;

typedef struct {
    const char* id;
    const char* text;
    const NovaTextSegment* segments;
    size_t segmentCount;
    const char* target;
    int disabled;
} NovaChoice;

typedef struct {
    const char* id;
    const char* path;
    int loop;
    double volume;
} NovaSfx;

typedef struct {
    const char* title;
    int reached;
} NovaEnding;

typedef struct {
    const char* bg;
    const char* bgTransition;
    const char* bgm;
    double bgmVolume;
    int bgmLoop;

    const char* currentTheme;
    
    const NovaSprite* sprites;
    size_t spriteCount;

    const NovaSfx* sfx;
    size_t sfxCount;
    
    NovaDialogue dialogue;
    int hasDialogue;
    
    const NovaChoice* choices;
    size_t choiceCount;

    const char* choiceQuestion;
    const NovaTextSegment* choiceQuestionSegments;
    size_t choiceQuestionSegmentCount;
    int hasChoices;
    int choiceIsShow;

    NovaEnding ending;
    int hasEnding;

    const char* const* flags;
    size_t flagCount;
} NovaState;

NOVA_API size_t nova_get_theme_count(NovaVM* vm);
NOVA_API const char* nova_get_theme_name(NovaVM* vm, size_t index);
NOVA_API size_t nova_get_theme_property_count(NovaVM* vm, const char* themeId);
NOVA_API const char* nova_get_theme_property_key(NovaVM* vm, const char* themeId, size_t index);
NOVA_API const char* nova_get_theme_property_value(NovaVM* vm, const char* themeId, const char* key);
NOVA_API const char* nova_get_current_theme(NovaVM* vm);
NOVA_API size_t nova_get_current_theme_property_count(NovaVM* vm);
NOVA_API const char* nova_get_current_theme_property_key(NovaVM* vm, size_t index);
NOVA_API const char* nova_get_current_theme_property_value(NovaVM* vm, const char* key);

NOVA_API NovaVM* nova_create();
NOVA_API void nova_destroy(NovaVM* vm);

NOVA_API int nova_load_package(NovaVM* vm, const char* path);
NOVA_API int nova_load_from_buffer(NovaVM* vm, const unsigned char* data, size_t size);

NOVA_API void nova_advance(NovaVM* vm);
NOVA_API void nova_choose(NovaVM* vm, const char* choiceId);

NOVA_API NovaState nova_get_state(NovaVM* vm);
NOVA_API NovaTextConfig nova_get_text_config(NovaVM* vm);

NOVA_API const char* nova_get_error(NovaVM* vm);

typedef void (*NovaStateCallback)(NovaState state, void* userData);
NOVA_API void nova_set_state_callback(NovaVM* vm, NovaStateCallback callback, void* userData);

NOVA_API int nova_save_snapshot_file(NovaVM* vm, const char* path);
NOVA_API int nova_load_snapshot_file(NovaVM* vm, const char* path);

NOVA_API size_t nova_get_variable_count(NovaVM* vm);
NOVA_API const char* nova_get_variable_name(NovaVM* vm, size_t index);
NOVA_API double nova_get_variable_number(NovaVM* vm, const char* name);
NOVA_API const char* nova_get_variable_string(NovaVM* vm, const char* name);

NOVA_API size_t nova_get_inventory_count(NovaVM* vm, const char* itemId);
NOVA_API int nova_has_item(NovaVM* vm, const char* itemId);

NOVA_API size_t nova_get_flags_count(NovaVM* vm);
NOVA_API const char* nova_get_flag(NovaVM* vm, size_t index);

NOVA_API char* nova_export_ast_snapshot_from_path(const char* path);
NOVA_API char* nova_export_ast_snapshot_from_scripts(const NovaMemoryScript* scripts, size_t count);
NOVA_API char* nova_ast_snapshot_to_source_files(const char* snapshotJson);
NOVA_API void nova_string_free(char* str);

// ===== 注册重载系统 C API =====

/// @brief 自定义指令回调
/// @param name 指令名（不含@前缀）
/// @param argCount 参数数量
/// @param argKeys 参数 key 数组
/// @param argValues 参数 value 数组
/// @param userData 用户数据
/// @return 0=未处理, 1=已处理, 2=已处理且需再次步进
typedef int (*NovaDirectiveCallback)(
    const char* name,
    size_t argCount,
    const char* const* argKeys,
    const char* const* argValues,
    void* userData);

/// @brief 自定义函数回调
/// @param name 函数名
/// @param argCount 参数数量
/// @param argJsons 参数 JSON 数组（每个参数为 JSON 字符串）
/// @param resultJson 输出参数：函数返回值的 JSON 字符串（调用方需 nova_string_free）
/// @param userData 用户数据
/// @return 0=失败, 1=成功
typedef int (*NovaFunctionCallback)(
    const char* name,
    size_t argCount,
    const char* const* argJsons,
    char** resultJson,
    void* userData);

/// @brief 状态字段序列化回调
typedef char* (*NovaStateFieldSerializeCb)(void* userData);

/// @brief 状态字段反序列化回调
typedef void (*NovaStateFieldDeserializeCb)(const char* json, void* userData);

NOVA_API int nova_register_directive(NovaVM* vm, const char* name,
    NovaDirectiveCallback callback, void* userData, int override_);

NOVA_API int nova_register_function(NovaVM* vm, const char* name,
    NovaFunctionCallback callback, void* userData, int override_);

NOVA_API int nova_register_state_field(NovaVM* vm, const char* key,
    NovaStateFieldSerializeCb serialize, NovaStateFieldDeserializeCb deserialize,
    const char* defaultValueJson, void* userData);

} // extern "C"
