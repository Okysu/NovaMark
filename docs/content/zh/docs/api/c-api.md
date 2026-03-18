---
title: "C API 参考"
weight: 1
---

# C API 参考

NovaMark 提供 C 语言接口，便于与游戏引擎集成。

## 头文件

```c
#include "nova/renderer/nova_c_api.h"
```

## 生命周期管理

### nova_create

```c
NovaVM* nova_create();
```

创建 VM 实例。

**返回值**: VM 实例指针，失败返回 `NULL`。

### nova_destroy

```c
void nova_destroy(NovaVM* vm);
```

销毁 VM 实例并释放所有资源。

## 游戏加载

### nova_load_package

```c
int nova_load_package(NovaVM* vm, const char* path);
```

从文件加载游戏包。

**参数**:
- `vm` - VM 实例
- `path` - .nvmp 文件路径

**返回值**: 成功返回 1，失败返回 0。使用 `nova_get_error()` 获取错误详情。

### nova_load_from_buffer

```c
int nova_load_from_buffer(NovaVM* vm, const unsigned char* data, size_t size);
```

从内存缓冲区加载游戏包。

## 游戏控制

### nova_start

```c
void nova_start(NovaVM* vm);
```

从第一个场景开始游戏。

### nova_next

```c
void nova_next(NovaVM* vm);
```

前进到下一条语句。

### nova_make_choice

```c
void nova_make_choice(NovaVM* vm, const char* choiceId);
```

选择选项。

**参数**:
- `vm` - VM 实例
- `choiceId` - 要选择的选项 ID

## 状态获取

### nova_get_state

```c
NovaState nova_get_state(NovaVM* vm);
```

获取当前渲染状态。

**返回值**: 包含所有渲染信息的 `NovaState` 结构体。

### nova_get_error

```c
const char* nova_get_error(NovaVM* vm);
```

获取最后的错误信息。

## 回调

### nova_set_state_callback

```c
void nova_set_state_callback(NovaVM* vm, NovaStateCallback callback, void* userData);

typedef void (*NovaStateCallback)(NovaState state, void* userData);
```

设置状态变化时的回调函数。

## 存档

### nova_save_game

```c
int nova_save_game(NovaVM* vm, const char* path);
```

保存游戏状态到文件。

### nova_load_game

```c
int nova_load_game(NovaVM* vm, const char* path);
```

从文件加载游戏状态。

## 变量和背包

### nova_get_variable_number

```c
double nova_get_variable_number(NovaVM* vm, const char* name);
```

获取数值变量的值。

### nova_get_variable_string

```c
const char* nova_get_variable_string(NovaVM* vm, const char* name);
```

获取字符串变量的值。

### nova_get_inventory_count

```c
size_t nova_get_inventory_count(NovaVM* vm, const char* itemId);
```

获取背包中物品的数量。

### nova_has_item

```c
int nova_has_item(NovaVM* vm, const char* itemId);
```

检查玩家是否拥有某物品。

**返回值**: 拥有返回 1，否则返回 0。
