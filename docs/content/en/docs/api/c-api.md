---
title: "C API Reference"
weight: 1
---

# C API Reference

NovaMark provides a C API for integration with game engines.

## Header File

```c
#include "nova/renderer/nova_c_api.h"
```

## Lifecycle Management

### nova_create

```c
NovaVM* nova_create();
```

Create a new VM instance.

**Returns**: Pointer to VM instance, or `NULL` on failure.

### nova_destroy

```c
void nova_destroy(NovaVM* vm);
```

Destroy a VM instance and free all resources.

## Game Loading

### nova_load_package

```c
int nova_load_package(NovaVM* vm, const char* path);
```

Load a game package from file.

**Parameters**:
- `vm` - VM instance
- `path` - Path to .nvmp file

**Returns**: 1 on success, 0 on failure. Use `nova_get_error()` for error details.

### nova_load_from_buffer

```c
int nova_load_from_buffer(NovaVM* vm, const unsigned char* data, size_t size);
```

Load a game package from memory buffer.

## Game Control

### nova_start

```c
void nova_start(NovaVM* vm);
```

Start the game from the first scene.

### nova_next

```c
void nova_next(NovaVM* vm);
```

Advance to the next statement.

### nova_make_choice

```c
void nova_make_choice(NovaVM* vm, const char* choiceId);
```

Select a choice option.

**Parameters**:
- `vm` - VM instance
- `choiceId` - ID of the choice to select

## State Retrieval

### nova_get_state

```c
NovaState nova_get_state(NovaVM* vm);
```

Get the current rendering state.

**Returns**: `NovaState` structure containing all rendering information.

### nova_get_error

```c
const char* nova_get_error(NovaVM* vm);
```

Get the last error message.

## Callbacks

### nova_set_state_callback

```c
void nova_set_state_callback(NovaVM* vm, NovaStateCallback callback, void* userData);

typedef void (*NovaStateCallback)(NovaState state, void* userData);
```

Set a callback to be notified when game state changes.

## Save/Load

### nova_save_game

```c
int nova_save_game(NovaVM* vm, const char* path);
```

Save game state to file.

### nova_load_game

```c
int nova_load_game(NovaVM* vm, const char* path);
```

Load game state from file.

## Variables and Inventory

### nova_get_variable_number

```c
double nova_get_variable_number(NovaVM* vm, const char* name);
```

Get a numeric variable value.

### nova_get_variable_string

```c
const char* nova_get_variable_string(NovaVM* vm, const char* name);
```

Get a string variable value.

### nova_get_inventory_count

```c
size_t nova_get_inventory_count(NovaVM* vm, const char* itemId);
```

Get the count of an item in inventory.

### nova_has_item

```c
int nova_has_item(NovaVM* vm, const char* itemId);
```

Check if player has an item.

**Returns**: 1 if player has the item, 0 otherwise.
