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
    const char* id;
    const char* url;
    double x;
    double y;
    double opacity;
    int zIndex;
} NovaSprite;

typedef struct {
    const char* id;
    int show;
    const char* content;
    const char* icon;
    double x;
    double y;
} NovaHud;

typedef struct {
    int isShow;
    const char* name;
    const char* text;
    const char* color;
} NovaDialogue;

typedef struct {
    const char* id;
    const char* text;
    int disabled;
} NovaChoice;

typedef struct {
    const char* bg;
    const char* bgm;
    
    const NovaSprite* sprites;
    size_t spriteCount;
    
    const NovaHud* huds;
    size_t hudCount;
    
    NovaDialogue dialogue;
    int hasDialogue;
    
    const NovaChoice* choices;
    size_t choiceCount;
} NovaState;

NOVA_API NovaVM* nova_create();
NOVA_API void nova_destroy(NovaVM* vm);

NOVA_API int nova_load_package(NovaVM* vm, const char* path);
NOVA_API int nova_load_from_buffer(NovaVM* vm, const unsigned char* data, size_t size);

NOVA_API void nova_start(NovaVM* vm);
NOVA_API void nova_next(NovaVM* vm);
NOVA_API void nova_make_choice(NovaVM* vm, const char* choiceId);

NOVA_API NovaState nova_get_state(NovaVM* vm);

NOVA_API const char* nova_get_error(NovaVM* vm);

typedef void (*NovaStateCallback)(NovaState state, void* userData);
NOVA_API void nova_set_state_callback(NovaVM* vm, NovaStateCallback callback, void* userData);

NOVA_API int nova_save_game(NovaVM* vm, const char* path);
NOVA_API int nova_load_game(NovaVM* vm, const char* path);

NOVA_API size_t nova_get_variable_count(NovaVM* vm);
NOVA_API const char* nova_get_variable_name(NovaVM* vm, size_t index);
NOVA_API double nova_get_variable_number(NovaVM* vm, const char* name);
NOVA_API const char* nova_get_variable_string(NovaVM* vm, const char* name);

NOVA_API size_t nova_get_inventory_count(NovaVM* vm, const char* itemId);
NOVA_API int nova_has_item(NovaVM* vm, const char* itemId);

} // extern "C"