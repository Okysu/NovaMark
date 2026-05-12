[根目录](../../CLAUDE.md) > [template](..) > **web**

# Web Template -- WebAssembly 渲染器模板

## 模块职责

提供基于 Emscripten 的 WebAssembly 运行时构建配置，以及参考 Web 渲染器模板（Chat Mode 和 VN Mode），作为 Web 部署的起点。

## 入口与启动

- **WASM 入口：** `template/web/src/nova_wasm_main.cpp`
- **构建条件：** 需要 Emscripten 工具链 + `ENABLE_WASM=ON`
- **CMake 目标：** `nova-wasm-runtime`（输出 `nova.js` + `nova.wasm`）

## 对外接口

### WASM 导出函数

通过 Emscripten `EXPORTED_FUNCTIONS` 暴露的接口，完整列表参见 `nova_wasm_api.h`。核心函数包括：

- `nova_wasm_init` -- 初始化
- `nova_wasm_load_package` -- 加载 .nvmp
- `nova_wasm_advance` / `nova_wasm_choose` -- 步进/选择
- `nova_wasm_get_status` -- 运行状态查询
- `nova_wasm_get_dialogue_*` -- 对话数据访问
- `nova_wasm_get_choice_*` -- 选项数据访问
- `nova_wasm_get_sprite_*` -- 精灵数据访问
- `nova_wasm_get_asset_size/bytes` -- 资源访问
- `nova_wasm_export/import_save_*` -- 存档导入导出

### Web 渲染器文件

| 文件 | 说明 |
|------|------|
| `index.html` | HTML 页面模板 |
| `app.js` | 应用入口，WASM 初始化与主循环 |
| `nova_renderer.js` | NovaMark 渲染器 JS 绑定 |
| `style.css` | 样式表 |

## 关键依赖与配置

- **Emscripten 链接选项：**
  - `ALLOW_MEMORY_GROWTH=1`, `INITIAL_MEMORY=16MB`, `STACK_SIZE=1MB`
  - `MODULARIZE=1`, `EXPORT_NAME=createNovaRuntime`, `ENVIRONMENT=web`
  - `FORCE_FILESYSTEM=1`（支持虚拟文件系统）
  - 保留 RTTI 和异常支持
- **编译优化：** Release `-O2`，Debug `-g4` + `ASSERTIONS=2`
- **链接库：** `nova-vm`, `nova-packer`, `nova-renderer`

## 深度实现细节

### `nova_renderer.js` -- NovaRenderer 类

封装 WASM 运行时，提供完整的游戏状态查询和资源管理接口。

**核心方法分组：**

| 分组 | 方法 | 说明 |
|------|------|------|
| 生命周期 | `init(nvmpData)` | 加载 .nvmp 包到 WASM 内存 |
| 步进控制 | `advance()`, `selectChoice(index)` | VM 步进与分支选择 |
| 状态查询 | `getStatus()`, `isEnded()`, `hasDialogue()`, `hasChoices()` | 运行状态 |
| 文本配置 | `getTextConfig()` | 返回 font/size/speed/basePath 配置 |
| 对话 | `getDialogueSpeaker/Text/Segments/Emotion/Color()` | 对话数据 |
| 选项 | `getChoiceCount/Text/Segments/Id/Target/Question()` | 选项数据 |
| 精灵 | `getSpriteCount/Url/X/Y/Position/Opacity/ZIndex()` | 精灵数据 |
| 音频 | `getBgm/Volume/Loop()`, `getSfxCount/Id/Path/Volume/Loop()` | BGM/SFX |
| 主题 | `getThemeCount/Name/PropertyCount/Key/Value()` | 主题样式 |
| 存档 | `exportSaveBinary/Json()`, `importSaveBinary/Json()` | 存档序列化 |
| Playthrough | `importPlaythroughBinary/Json()` | 通关记录导入 |
| 资源 | `getAssetBytes(name, category)`, `getImageUrl(assetName, category)` | 资源加载（带缓存） |
| 音频播放 | `playBgm(assetName, loop, volume)`, `stopBgm()`, `playSfx(...)` | Web Audio API 播放 |

**资源解析策略：** `resolveAssetName()` 根据 category (`bg`/`sprite`/`audio`) 和文件扩展名自动添加路径前缀（从 `textConfig.baseXxxPath` 读取）。

### `app.js` -- 应用主循环

**双模式渲染：** VN Mode（视觉小说）和 Chat Mode（聊天），通过 `setMode()` 切换。

**关键组件：**

| 组件 | 说明 |
|------|------|
| `TypewriterEffect` | 打字机效果，支持逐字显示和立即补全（`textSpeed` 毫秒/字符，0 = 瞬间） |
| `renderVNMode()` | VN 模式渲染：背景图、精灵（含 `is-speaking`/`is-dimmed`/`is-entering` 状态）、对话打字机、选项列表 |
| `renderChatMode()` | Chat 模式渲染：消息气泡累积、打字机效果、选项列表 |
| `renderStatusBar()` | 状态栏：显示 variables（numbers/strings/bools）+ inventory items |
| `window.novaDebug` | 调试 API：`status()`, `snapshot()`, `choices()`, `advance()`, `select(i)`, `choose(i)` |

**内联样式系统：** 支持带 `style` 的文本段（segments），通过 `data-nova-style` 属性和 `.nova-inline-style--{name}` 类名映射到 CSS。

**精灵车道系统：** X ≤ 25 → `left`，X ≥ 75 → `right`，其余 → `center`；说话角色精灵获得 `is-forward` 类名。

## 相关文件清单

| 文件 | 说明 |
|------|------|
| `src/nova_wasm_main.cpp` | WASM 入口（调用 `nova_wasm_exports.cpp`） |
| `index.html` | HTML 页面模板 |
| `app.js` | 应用入口 JS（双模式渲染 + 打字机 + 调试 API） |
| `nova_renderer.js` | NovaRenderer JS 类（WASM 绑定 + 资源/音频管理） |
| `style.css` | 样式表 |
| `CMakeLists.txt` | Emscripten 构建配置 |

## 变更记录 (Changelog)

| 时间 | 操作 | 说明 |
|------|------|------|
| 2026-05-11 08:41:29 | 初始化 | 由 init-architect 自动生成 |
| 2026-05-11 08:49:00 | 补扫 | 深度读取 app.js 和 nova_renderer.js，补充实现细节 |
