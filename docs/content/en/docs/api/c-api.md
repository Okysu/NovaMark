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

### nova_advance

```c
void nova_advance(NovaVM* vm);
```

Advance to the next discrete blocking point (dialogue, choice, ending, etc.).

### nova_choose

```c
void nova_choose(NovaVM* vm, const char* choiceId);
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

### Runtime State Snapshot

The Web/WASM extension interface provides unified runtime state export:

```c
const char* nova_export_runtime_state_json(void* vm, size_t* outSize);
```

Returns a JSON string containing:

- `textConfig`
- `variables`
- `inventory`
- `itemDefinitions`
- `characterDefinitions`
- `inventoryItems`

Suitable for GUI and debugging tools to read complete runtime state in one call.

### AST Snapshot Export

The C API also provides AST snapshot export:

```c
char* nova_export_ast_snapshot_from_path(const char* path);
char* nova_export_ast_snapshot_from_scripts(const NovaMemoryScript* scripts, size_t count);
```

These functions return **AST JSON strings**, suitable for:

- Visualizing syntax trees in editor / Creator tools
- Debugging script parse results
- Inspecting merged output from multi-file projects

Specifically:

- `nova_export_ast_snapshot_from_path`: export from a project path
- `nova_export_ast_snapshot_from_scripts`: export from in-memory script arrays

If the text contains `{{}}` interpolation or `{style:text}` inline styling, the exported AST snapshot preserves the corresponding text segment metadata.

## Callbacks

### nova_set_state_callback

```c
void nova_set_state_callback(NovaVM* vm, NovaStateCallback callback, void* userData);

typedef void (*NovaStateCallback)(NovaState state, void* userData);
```

Set a callback to be notified when game state changes.

## Save/Load

NovaMark now separates save functionality into two layers:

- **official save files**: binary format for players and shipped products
- **JSON import/export**: retained for Web/WASM debugging, tests, and developer tools

The runtime also supports a **playthrough-only import** mode:

- restore previously triggered endings only
- restore persistent flags only
- **do not** restore variables, inventory, current scene, dialogue, BGM, or other live runtime state

This is useful for:

- carrying unlocked ending history into a new run
- syncing meta progression across devices
- restoring achievement / glossary progress without overwriting the player's current session

### nova_save_snapshot_file

```c
int nova_save_snapshot_file(NovaVM* vm, const char* path);
```

Save the current runtime snapshot to a **binary file**.

### nova_load_snapshot_file

```c
int nova_load_snapshot_file(NovaVM* vm, const char* path);
```

Restore runtime state from a **binary snapshot file**.

### Playthrough-only import (host / bridge layer)

At the NAPI / host bridge layer, two playthrough-only import APIs are now available:

```text
runtimeImportPlaythroughJson(vmHandle, saveJson)
runtimeImportPlaythroughBinary(vmHandle, saveBuffer)
```

Semantics:

- the input is still a full save payload (JSON or binary)
- the runtime only extracts:
  - `triggeredEndings`
  - `flags`
- the current VM keeps its variables, scene position, inventory, and rendering state unchanged

These APIs do not replace `runtimeImportSaveJson` / `runtimeImportSaveBinary`; they provide a safer option for syncing only meta-playthrough progress.

## Variables and Inventory

### nova_get_variable_count

```c
size_t nova_get_variable_count(NovaVM* vm);
```

Get the total number of variables.

### nova_get_variable_name

```c
const char* nova_get_variable_name(NovaVM* vm, size_t index);
```

Get a variable name by index.

**Ordering**: names are returned in lexicographic (ascending) order, index starts at 0.

**Notes**:

- The internal cache is rebuilt on each call
- Returned pointers may become invalid after the next call
- Returns `NULL` if `index` is out of range

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
