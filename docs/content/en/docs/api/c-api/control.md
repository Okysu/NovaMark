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

## nova_advance

Advance the VM to the next discrete blocking point.

```c
void nova_advance(NovaVM* vm);
```

Call this when the host wants the story to continue.

## nova_choose

Make a choice selection.

```c
void nova_choose(NovaVM* vm, const char* choiceId);
```

**Parameters**:
- `vm` - VM instance
- `choiceId` - ID of the selected choice

**Example**:
```c
// Player selected choice 0
nova_choose(vm, "0");
```

## nova_get_error

Get the last error message.

```c
const char* nova_get_error(NovaVM* vm);
```

**Returns**: Error message string, or NULL if no error.
