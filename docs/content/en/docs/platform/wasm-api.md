---
title: "WASM API Reference"
weight: 3
---

# WASM API Reference

This page is for platform integrators, explaining the usage and calling flow of the **Web/WASM binding layer**.

Note: The WASM API is essentially the Web export version of the C API. The host still needs to follow the **"discrete state machine + host-driven rendering"** pattern.

---

## 1. Relationship with C API

The interfaces provided by the WASM layer are essentially exported versions of the C API:

- Semantically consistent (load package, advance, choose, read state, save)
- Calling environment changes from C/C++ to JS (Emscripten Module)
- Memory management is handled by the host side (`_malloc/_free` and `_nova_wasm_free`)

If you need to understand host responsibility boundaries, please read first:

- [Runtime and Host](./runtime-and-host/)
- [C API Reference](../api/c-api/)

---

## 2. Basic Calling Flow (Sequence)

Here is the typical calling sequence for Web/WASM:

1. Initialize runtime (`_nova_wasm_init`)
2. Load `.nvmp` game package (`_nova_wasm_load_package`)
3. Advance to the first state point (`_nova_wasm_advance`)
4. Read state and render UI
5. User interaction -> `advance()` or `choose()`
6. Repeat steps 3-5 until ending

This is completely consistent with the C API's `nova_advance / nova_choose` pattern.

---

## 3. Initialization and Loading

Core functions used in the template (from `template/web/nova_renderer.js`):

| Function | WASM Export Function | Description |
|----------|---------------------|-------------|
| Initialize | `_nova_wasm_init()` | Initialize VM runtime |
| Load package | `_nova_wasm_load_package(ptr, size)` | Load `.nvmp` from memory |
| Error message | `_nova_wasm_get_last_error()` | Get last error string (optional) |

> Note: `load_package` returning non-zero indicates failure. The template reads `get_last_error` for error details.

---

## 4. Advancement and Choice

| Function | WASM Export Function | Description |
|----------|---------------------|-------------|
| Advance | `_nova_wasm_advance()` | Advance to next "blocking point" |
| Choose | `_nova_wasm_choose(index)` | Select specified choice (index) |
| Status code | `_nova_wasm_get_status()` | 0=running, 1=waiting for input, 2=ended |

**Note**: The WASM version of `choose` uses **choice index**, not a choiceId string.

---

## 5. Runtime State and Text Configuration

### Runtime State (JSON)

The template uses the following interfaces to export complete state:

| Function | WASM Export Function | Description |
|----------|---------------------|-------------|
| Export state | `_nova_wasm_export_runtime_state_json(sizePtr)` | Returns JSON string pointer |
| Free memory | `_nova_wasm_free(ptr)` | Free WASM-allocated strings |

For the returned JSON structure, see:

- [Runtime State](../api/runtime-state/)

### Text Configuration (textConfig)

The template reads the following configuration fields:

| Field | WASM Export Function |
|-------|---------------------|
| Default font | `_nova_wasm_get_default_font()` |
| Font size | `_nova_wasm_get_default_font_size()` |
| Text speed | `_nova_wasm_get_default_text_speed()` |
| Background path | `_nova_wasm_get_base_bg_path()` |
| Sprite path | `_nova_wasm_get_base_sprite_path()` |
| Audio path | `_nova_wasm_get_base_audio_path()` |

---

## 6. Asset Reading

The template reads asset bytes from the game package:

| Function | WASM Export Function | Description |
|----------|---------------------|-------------|
| Get asset size | `_nova_wasm_get_asset_size(namePtr)` | Returns byte length |
| Read asset bytes | `_nova_wasm_get_asset_bytes(namePtr, bufferPtr, size)` | Write to specified buffer |

The host is responsible for:

- Constructing correct asset names (refer to `base_*_path`)
- Converting bytes to Blob / Image / Audio

---

## 7. Saves (WASM)

The template wraps these export functions:

| Function | WASM Export Function | Description |
|----------|---------------------|-------------|
| Export binary save | `_nova_wasm_export_save_binary(sizePtr)` | Returns byte pointer |
| Export JSON save | `_nova_wasm_export_save_json(sizePtr)` | Returns JSON string |
| Import binary save | `_nova_wasm_import_save_binary(ptr, size)` | Returns 0 on success |
| Import JSON save | `_nova_wasm_import_save_json(ptr, len)` | Returns 0 on success |

Binary is for formal saves, JSON is suitable for debugging and development tools.

---

## 8. Memory Management Conventions

Memory allocated by the WASM layer needs to be actively freed by the host:

- Host-side allocation: Use `_malloc/_free`
- WASM internal allocation: Use `_nova_wasm_free`

Typical flow (example/pseudocode):

```javascript
// Example/pseudocode
const sizePtr = runtime._malloc(4);
const jsonPtr = runtime._nova_wasm_export_runtime_state_json(sizePtr);
const size = runtime.getValue(sizePtr, 'i32');
runtime._free(sizePtr);

const jsonText = runtime.UTF8ToString(jsonPtr, size);
runtime._nova_wasm_free(jsonPtr);
```

---

## 9. Error Handling

Common error handling approaches:

- `load_package` returns non-zero -> Read `_nova_wasm_get_last_error()`
- `export_runtime_state_json` returns null pointer -> Treat as no state/abnormal state
- `import_save_*` returns non-zero -> Treat as import failure

Host-side recommendations:

- Log error codes and error strings
- Fall back UI on load failure

---

## 10. Minimal Example (Example/Pseudocode)

```javascript
// Example/pseudocode: Initialize + Load + Advance + Render
const runtime = await createNovaRuntime();
runtime._nova_wasm_init();

// Load nvmp
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

// Advance to first state point
runtime._nova_wasm_advance();

// Read state and render
const stateJsonPtr = runtime._nova_wasm_export_runtime_state_json(runtime._malloc(4));
// ...parse and render UI
```

---

## 11. Further Reading

- [Runtime and Host](./runtime-and-host/)
- [Web Rendering Template](./web-template/)
- [Runtime State](../api/runtime-state/)
- [C API Reference](../api/c-api/)
