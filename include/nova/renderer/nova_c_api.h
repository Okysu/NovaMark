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
    int isShow;
    const char* name;
    const char* text;
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
    int hasChoices;
    int choiceIsShow;
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

NOVA_API char* nova_export_ast_snapshot_from_path(const char* path);
NOVA_API char* nova_export_ast_snapshot_from_scripts(const NovaMemoryScript* scripts, size_t count);
NOVA_API char* nova_ast_snapshot_to_source_files(const char* snapshotJson);
NOVA_API void nova_string_free(char* str);

} // extern "C"
