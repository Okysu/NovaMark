---
title: "WASM API 参考"
weight: 3
---

# WASM API 参考

本页面向平台接入方，说明 **Web/WASM 绑定层** 的使用方式与调用流程。

注意：WASM API 实际上是 C API 的 Web 导出版本，宿主仍需遵循 **“离散状态机 + 宿主驱动渲染”** 的模式。

---

## 1. 与 C API 的关系

WASM 层提供的接口，本质上是 C API 的导出版本：

- 语义一致（加载包、推进、选择、读状态、存档）
- 调用环境从 C/C++ 变为 JS（Emscripten Module）
- 内存管理由宿主侧负责（`_malloc/_free` 与 `_nova_wasm_free`）

如果你需要了解宿主职责边界，请先阅读：

- [运行时与宿主](./runtime-and-host/)
- [C API 参考](../api/c-api/)

---

## 2. 基本调用流程（顺序）

下面是 Web/WASM 典型的调用顺序：

1. 初始化运行时（`_nova_wasm_init`）
2. 加载 `.nvmp` 游戏包（`_nova_wasm_load_package`）
3. 推进到第一个状态点（`_nova_wasm_advance`）
4. 读取状态并渲染 UI
5. 用户交互 → `advance()` 或 `choose()`
6. 重复 3-5 直到结局

这与 C API 的 `nova_advance / nova_choose` 模式完全一致。

---

## 3. 初始化与加载

模板中使用的核心函数（来自 `template/web/nova_renderer.js`）：

| 功能 | WASM 导出函数 | 说明 |
|------|--------------|------|
| 初始化 | `_nova_wasm_init()` | 初始化 VM 运行时 |
| 加载包 | `_nova_wasm_load_package(ptr, size)` | 从内存加载 `.nvmp` |
| 错误信息 | `_nova_wasm_get_last_error()` | 获取最近一次错误字符串（可选） |

> ⚠️ `load_package` 返回非 0 表示失败。模板会读取 `get_last_error` 作为错误详情。

---

## 4. 推进与选择

| 功能 | WASM 导出函数 | 说明 |
|------|--------------|------|
| 推进 | `_nova_wasm_advance()` | 推进到下一个“阻塞点” |
| 选择 | `_nova_wasm_choose(index)` | 选择指定选项（索引） |
| 状态码 | `_nova_wasm_get_status()` | 0=运行中，1=等待输入，2=已结束 |

**注意**：WASM 版 `choose` 使用的是 **选项索引**，而不是 choiceId 字符串。

---

## 5. 运行时状态与文本配置

### 运行时状态（JSON）

模板使用以下接口导出完整状态：

| 功能 | WASM 导出函数 | 说明 |
|------|--------------|------|
| 导出状态 | `_nova_wasm_export_runtime_state_json(sizePtr)` | 返回 JSON 字符串指针 |
| 释放内存 | `_nova_wasm_free(ptr)` | 释放 WASM 内部分配的字符串 |

返回的 JSON 结构详见：

- [运行时状态](../api/runtime-state/)

### 文本配置（textConfig）

模板会读取以下配置字段：

| 字段 | WASM 导出函数 |
|------|--------------|
| 默认字体 | `_nova_wasm_get_default_font()` |
| 字号 | `_nova_wasm_get_default_font_size()` |
| 文字速度 | `_nova_wasm_get_default_text_speed()` |
| 背景路径 | `_nova_wasm_get_base_bg_path()` |
| 立绘路径 | `_nova_wasm_get_base_sprite_path()` |
| 音频路径 | `_nova_wasm_get_base_audio_path()` |

---

## 6. 资源读取

模板会从游戏包中读取资源字节：

| 功能 | WASM 导出函数 | 说明 |
|------|--------------|------|
| 获取资源大小 | `_nova_wasm_get_asset_size(namePtr)` | 返回字节长度 |
| 读取资源字节 | `_nova_wasm_get_asset_bytes(namePtr, bufferPtr, size)` | 写入指定缓冲区 |

宿主负责：

- 拼出正确的资源名（可参考 `base_*_path`）
- 将字节转成 Blob / Image / Audio

---

## 7. 存档（WASM）

模板封装了以下导出函数：

| 功能 | WASM 导出函数 | 说明 |
|------|--------------|------|
| 导出二进制存档 | `_nova_wasm_export_save_binary(sizePtr)` | 返回字节指针 |
| 导出 JSON 存档 | `_nova_wasm_export_save_json(sizePtr)` | 返回 JSON 字符串 |
| 导入二进制存档 | `_nova_wasm_import_save_binary(ptr, size)` | 返回 0 表示成功 |
| 导入 JSON 存档 | `_nova_wasm_import_save_json(ptr, len)` | 返回 0 表示成功 |

二进制用于正式存档，JSON 适合调试与开发工具。

---

## 8. 内存管理约定

WASM 层分配的内存需要宿主主动释放：

- 宿主侧分配：使用 `_malloc/_free`
- WASM 内部分配：使用 `_nova_wasm_free`

典型流程（示例/伪代码）：

```javascript
// 示例/伪代码
const sizePtr = runtime._malloc(4);
const jsonPtr = runtime._nova_wasm_export_runtime_state_json(sizePtr);
const size = runtime.getValue(sizePtr, 'i32');
runtime._free(sizePtr);

const jsonText = runtime.UTF8ToString(jsonPtr, size);
runtime._nova_wasm_free(jsonPtr);
```

---

## 9. 错误处理

常见错误处理方式：

- `load_package` 返回非 0 → 读取 `_nova_wasm_get_last_error()`
- `export_runtime_state_json` 返回空指针 → 视为无状态/异常状态
- `import_save_*` 返回非 0 → 视为导入失败

宿主侧建议：

- 记录错误码与错误字符串
- 遇到加载失败时回退 UI

---

## 10. 最小示例（示例/伪代码）

```javascript
// 示例/伪代码：初始化 + 加载 + 推进 + 渲染
const runtime = await createNovaRuntime();
runtime._nova_wasm_init();

// 加载 nvmp
const data = await fetch('game.nvmp').then(r => r.arrayBuffer());
const ptr = runtime._malloc(data.byteLength);
runtime.writeArrayToMemory(new Uint8Array(data), ptr);
const result = runtime._nova_wasm_load_package(ptr, data.byteLength);
runtime._free(ptr);

if (result !== 0) {
  const err = runtime._nova_wasm_get_last_error
    ? runtime.UTF8ToString(runtime._nova_wasm_get_last_error())
    : '';
  throw new Error('load failed: ' + err);
}

// 推进到第一个状态点
runtime._nova_wasm_advance();

// 读取状态并渲染
const stateJsonPtr = runtime._nova_wasm_export_runtime_state_json(runtime._malloc(4));
// ...解析并渲染 UI
```

---

## 11. 延伸阅读

- [运行时与宿主](./runtime-and-host/)
- [Web 渲染模板](./web-template/)
- [运行时状态](../api/runtime-state/)
- [C API 参考](../api/c-api/)
