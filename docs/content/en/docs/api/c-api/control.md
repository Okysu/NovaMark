---
title: "Game Control"
weight: 2
---

# Game Control

## nova_load_package

Load a game package from a file.

```c
int nova_load_package(NovaVM* vm, const char* path);
```

**Parameters**:
- `vm` - VM instance
- `path` - Path to the .nvmp file

**Returns**: 1 on success, 0 on failure

**Example**:
```c
if (!nova_load_package(vm, "game.nvmp")) {
    printf("Error: %s\n", nova_get_error(vm));
    return 1;
}
```

## nova_load_from_buffer

Load a game package from memory.

```c
int nova_load_from_buffer(NovaVM* vm, const unsigned char* data, size_t size);
```

**Parameters**:
- `vm` - VM instance
- `data` - Pointer to the package data
- `size` - Size of the data in bytes

**Returns**: 1 on success, 0 on failure

## nova_start

Start the game from the beginning.

```c
void nova_start(NovaVM* vm);
```

## nova_next

Advance the game by one step.

```c
void nova_next(NovaVM* vm);
```

Call this when the player clicks to continue (after dialogue or narration).

## nova_make_choice

Make a choice selection.

```c
void nova_make_choice(NovaVM* vm, const char* choiceId);
```

**Parameters**:
- `vm` - VM instance
- `choiceId` - ID of the selected choice

**Example**:
```c
// Player selected choice 0
nova_make_choice(vm, "0");
```

## nova_get_error

Get the last error message.

```c
const char* nova_get_error(NovaVM* vm);
```

**Returns**: Error message string, or NULL if no error.
