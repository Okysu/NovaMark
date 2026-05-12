[根目录](../../CLAUDE.md) > [src](..) > **renderer**

# Renderer -- 渲染器接口

## 模块职责

定义渲染器抽象接口 `IRenderer`，提供 C API (`nova_c_api.h`) 和 WebAssembly API (`nova_wasm_api.h`)，将 VM 状态快照桥接到外部渲染实现。

## 入口与启动

- **抽象接口：** `nova::IRenderer`（`include/nova/renderer/renderer.h`）
- **空实现：** `nova::NullRenderer`（全空方法，用于无渲染场景）
- **C API：** `include/nova/renderer/nova_c_api.h` -- 标准 C FFI
- **WASM API：** `include/nova/renderer/nova_wasm_api.h` -- Emscripten 导出

## 对外接口

### IRenderer 接口

| 方法 | 说明 |
|------|------|
| `render(state)` | 渲染完整状态 |
| `showBackground(path)` / `hideBackground()` | 背景图 |
| `showSprite(sprite)` / `hideSprite(id)` / `clearSprites()` | 精灵立绘 |
| `playBgm(path, loop, volume)` / `stopBgm()` | 背景音乐 |
| `playSfx(sfx)` / `stopSfx(id)` / `stopAllSfx()` | 音效 |
| `showDialogue(dialogue)` / `hideDialogue()` | 对话框 |
| `showChoice(choice)` / `hideChoice()` | 选项面板 |
| `setOnClick(cb)` / `setOnChoice(cb)` | 用户交互回调 |

### C API 核心函数

| 函数 | 说明 |
|------|------|
| `nova_create()` / `nova_destroy()` | VM 实例管理 |
| `nova_load_package(vm, path)` / `nova_load_from_buffer(vm, data, size)` | 加载 .nvmp |
| `nova_advance(vm)` | 步进 |
| `nova_choose(vm, choiceId)` | 选择分支 |
| `nova_get_state(vm)` | 获取完整状态 |
| `nova_save_snapshot_file()` / `nova_load_snapshot_file()` | 存档/读档 |
| `nova_export_ast_snapshot_*()` | AST 快照导出 |
| `nova_get_variable_*()` / `nova_get_inventory_count()` / `nova_has_item()` | 运行时查询 |
| `nova_get_theme_*()` | 主题查询 |

### WASM API 额外函数

| 函数类别 | 说明 |
|----------|------|
| 资源访问 | `nova_get_asset_size/bytes`, `nova_has_asset` |
| 存档导入导出 | `nova_export/import_save_json/binary`, `nova_import_playthrough_*` |
| 扩展状态 | `nova_get_current_scene/label`, `nova_get_status`, `nova_is_ended` |
| 对话片段 | `nova_get_dialogue_segment_count/text/style` |
| 选项片段 | `nova_get_choice_segment_count/text/style` |
| 精灵详情 | `nova_get_sprite_position/x/y/opacity/z_index` |
| 音效 | `nova_get_sfx_count/id/path/volume/loop` |
| 主题 | `nova_get_theme_count/name/property_*` |

## 关键依赖与配置

- **上游依赖：** `nova-vm`（状态结构）、`nova-packer`（.nvmp 加载）
- **CMake 目标：** `nova-renderer`（静态库）
- **编译定义：** `NOVA_EXPORTS`（导出符号）

## 测试与质量

- 测试文件：`tests/c_api_test.cpp`
- 覆盖：C API 的加载、步进、选择、存档等核心流程

## 相关文件清单

| 文件 | 说明 |
|------|------|
| `include/nova/renderer/renderer.h` | IRenderer 接口与 NullRenderer |
| `include/nova/renderer/nova_c_api.h` | C API 声明 |
| `include/nova/renderer/nova_wasm_api.h` | WASM API 声明 |
| `src/renderer/nova_c_api.cpp` | C API 实现 |
| `src/renderer/wasm/nova_wasm_exports.cpp` | WASM API 实现 |
| `src/renderer/CMakeLists.txt` | CMake 构建配置 |

## 变更记录 (Changelog)

| 时间 | 操作 | 说明 |
|------|------|------|
| 2026-05-11 08:41:29 | 初始化 | 由 init-architect 自动生成 |
