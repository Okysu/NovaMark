---
title: "Lifecycle Management"
weight: 1
---

# Lifecycle Management

## nova_create

Create a new VM instance.

```c
NovaVM* nova_create(void);
```

**Returns**: Pointer to a new `NovaVM` instance, or `NULL` on failure.

**Example**:
```c
NovaVM* vm = nova_create();
if (!vm) {
    fprintf(stderr, "Failed to create VM\n");
    return 1;
}
```

## nova_destroy

Destroy a VM instance and free all resources.

```c
void nova_destroy(NovaVM* vm);
```

**Parameters**:
- `vm` - Pointer to the VM instance to destroy

**Example**:
```c
nova_destroy(vm);
```

## Full Lifecycle Example

```c
#include "nova/renderer/nova_c_api.h"

int main() {
    // Create VM
    NovaVM* vm = nova_create();
    if (!vm) {
        return 1;
    }
    
    // ... use VM ...
    
    // Clean up
    nova_destroy(vm);
    return 0;
}
```
