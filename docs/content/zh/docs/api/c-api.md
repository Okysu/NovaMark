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

### nova_advance

```c
void nova_advance(NovaVM* vm);
```

推进到下一个离散阻塞点（如对话、选择或结局）。

### nova_choose

```c
void nova_choose(NovaVM* vm, const char* choiceId);
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

### 运行时状态快照

当前 Web/WASM 扩展接口提供统一运行时状态导出：

```c
const char* nova_export_runtime_state_json(void* vm, size_t* outSize);
```

该接口返回 JSON 字符串，包含：

- `textConfig`
- `variables`
- `inventory`
- `itemDefinitions`
- `characterDefinitions`
- `inventoryItems`

适合 GUI 与调试工具一次性读取完整运行时状态。

## 回调

### nova_set_state_callback

```c
void nova_set_state_callback(NovaVM* vm, NovaStateCallback callback, void* userData);

typedef void (*NovaStateCallback)(NovaState state, void* userData);
```

设置状态变化时的回调函数。

## 存档

NovaMark 当前将存档分为两层：

- **正式文件存档**：使用二进制格式，供玩家和产品环境使用
- **JSON 导入导出**：保留给 Web/WASM 调试、测试和开发工具使用

### nova_save_snapshot_file

```c
int nova_save_snapshot_file(NovaVM* vm, const char* path);
```

将当前运行时快照以**二进制格式**保存到文件。

### nova_load_snapshot_file

```c
int nova_load_snapshot_file(NovaVM* vm, const char* path);
```

从**二进制快照文件**恢复运行时状态。

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
