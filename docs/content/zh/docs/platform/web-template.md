---
title: "Web 渲染模板"
weight: 2
---

# Web 渲染模板

`template/web` 是 NovaMark 官方提供的 Web 渲染器参考实现。它展示了宿主如何正确消费运行时状态、处理用户输入、管理存档，是平台接入方理解"核心 VM + 哑渲染器"架构的最佳示例。

---

## 模板定位

Web 模板不是"唯一正确的前端"，而是：

- 一套**最小但完整**的接入示范
- 一套**开箱即试**的运行示例
- 一份展示宿主职责边界的**参考代码**

你可以基于它快速验证游戏包，也可以将其作为定制开发的起点。

---

## 目录结构

```
template/web/
├── CMakeLists.txt          # WASM 构建配置
├── index.html              # 入口页面
├── style.css               # 样式表
├── app.js                  # 宿主逻辑（事件绑定、状态渲染）
├── nova_renderer.js        # 渲染器封装（VM 状态查询、资源加载）
└── src/
    └── nova_wasm_main.cpp  # WASM 入口，导出 C API 函数
```

### 核心文件职责

| 文件 | 职责 |
|------|------|
| `nova_renderer.js` | 封装 WASM 绑定，提供状态查询 API（`getBackground()`、`getDialogueText()` 等） |
| `app.js` | 宿主逻辑：处理用户交互、调用 `advance()`/`choose()`、更新 DOM |
| `style.css` | 视觉样式，支持 VN 模式与 Chat 模式两套布局 |
| `index.html` | 页面骨架，包含模式切换、菜单、对话框等 DOM 结构 |

---

## 本地运行

### 前置条件

1. **Emscripten SDK** - 已配置并激活
2. **vcpkg** - 已安装并配置工具链
3. **CMake 3.16+**

### 构建步骤

```bash
# 1. 配置项目（使用 Emscripten 工具链）
cmake -B build-wasm -S . \
  -DCMAKE_TOOLCHAIN_FILE=$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
  -DCMAKE_BUILD_TYPE=Release

# 2. 构建 WASM 运行时
cmake --build build-wasm --target nova-wasm-runtime

# 3. 进入输出目录
cd build-wasm/template/web

# 4. 启动本地服务器（需要 HTTP 服务器，不能直接打开 HTML）
python -m http.server 8080
```

### 访问

浏览器打开 `http://localhost:8080`，选择 `.nvmp` 游戏包即可运行。

---

## 最小接入步骤

如果你想在其他 Web 项目中集成 NovaMark，只需三个文件：

### 1. 引入 WASM 运行时

```html
<script src="nova.js"></script>        <!-- Emscripten 生成的运行时 -->
<script src="nova_renderer.js"></script>  <!-- 状态查询封装 -->
```

### 2. 初始化渲染器

```javascript
const renderer = new NovaRenderer();

// 加载游戏包
async function loadGame(nvmpUrl) {
  const response = await fetch(nvmpUrl);
  const nvmpData = await response.arrayBuffer();
  await renderer.init(nvmpData);
  
  // 推进到第一个状态点
  renderer.advance();
  renderFrame();
}
```

### 3. 渲染状态

```javascript
function renderFrame() {
  // 读取状态
  const bg = renderer.getBackground();
  const dialogue = renderer.hasDialogue() 
    ? { speaker: renderer.getDialogueSpeaker(), text: renderer.getDialogueText() }
    : null;
  const choices = renderer.hasChoices() 
    ? Array.from({ length: renderer.getChoiceCount() }, (_, i) => renderer.getChoiceText(i))
    : [];
  
  // 渲染到你的 UI
  updateBackground(bg);
  updateDialogue(dialogue);
  updateChoices(choices);
}
```

---

## 状态驱动渲染

NovaMark 采用"离散状态机"模型：VM 每次执行到需要宿主处理的状态点时停止，等待宿主响应。

### 状态查询 API

`NovaRenderer` 提供的状态查询方法：

| 方法 | 返回值 | 说明 |
|------|--------|------|
| `getStatus()` | `0`=运行中, `1`=等待输入, `2`=已结束 | VM 当前状态 |
| `isEnded()` | boolean | 游戏是否已结束 |
| `getBackground()` | string | 背景图资源名 |
| `hasDialogue()` | boolean | 是否有对话待显示 |
| `getDialogueSpeaker()` | string | 说话人名称 |
| `getDialogueText()` | string | 对话文本 |
| `getDialogueColor()` | string | 说话人颜色（十六进制） |
| `hasChoices()` | boolean | 是否有选项待选择 |
| `getChoiceCount()` | number | 选项数量 |
| `getChoiceText(index)` | string | 指定选项的文本 |
| `isChoiceDisabled(index)` | boolean | 选项是否被禁用 |
| `getSpriteCount()` | number | 当前立绘数量 |
| `getSpriteUrl(index)` | string | 立绘资源名 |
| `getSpriteX/Y(index)` | number | 立绘位置（百分比） |
| `getSpriteOpacity(index)` | number | 立绘透明度（0-1） |
| `getBgm()` | string | BGM 资源名 |
| `getBgmVolume()` | number | BGM 音量（0-1） |
| `getBgmLoop()` | boolean | BGM 是否循环 |

### 渲染循环

模板采用"事件驱动"模式，而非持续轮询：

```
用户点击 → advance()/choose() → VM 推进 → 读取新状态 → 更新 UI
```

核心代码示例（`app.js`）：

```javascript
function handleAdvance() {
  if (renderer.isEnded()) return;
  if (renderer.hasChoices()) return;  // 有选项时不响应点击推进
  
  renderer.advance();
  renderGame();
}

function handleChoice(index) {
  renderer.selectChoice(index);
  renderer.advance();
  renderGame();
}
```

### 打字机效果

模板内置打字机效果，基于 `textConfig.defaultTextSpeed` 控制：

```javascript
// textSpeed = 0 表示瞬间显示，否则为毫秒/字符
typewriter.start(element, text, textSpeed, onComplete);
```

---

## 输入与存档

### 用户输入

宿主负责将用户交互转换为 VM 操作：

| 用户操作 | VM 操作 |
|----------|---------|
| 点击对话框/背景 | `advance()` |
| 点击选项按钮 | `choose(index)` |

### 存档机制

NovaMark 提供两种存档格式：

| 格式 | 用途 | API |
|------|------|-----|
| 二进制（`.nvs`） | 正式存档，体积小 | `exportSaveBinary()` / `importSaveBinary(bytes)` |
| JSON | 调试、开发工具 | `exportSaveJson()` / `importSaveJson(json)` |

示例：

```javascript
// 保存
function handleSave() {
  const bytes = renderer.exportSaveBinary();
  const blob = new Blob([bytes], { type: 'application/octet-stream' });
  const url = URL.createObjectURL(blob);
  // 触发下载...
}

// 加载
async function handleLoad(file) {
  const bytes = await file.arrayBuffer();
  renderer.importSaveBinary(new Uint8Array(bytes));
  renderGame();
}
```

### 运行时状态快照

调试时可获取完整运行时状态：

```javascript
const state = renderer.getRuntimeState();
// state 包含：variables, inventory, characterDefinitions, currentScene 等
```

---

## 调试建议

### 浏览器控制台 API

模板暴露了 `window.novaDebug` 对象，可在控制台直接操作：

```javascript
// 查看当前状态
novaDebug.status()

// 获取完整快照
novaDebug.snapshot()

// 手动推进
novaDebug.advance()

// 选择选项
novaDebug.select(0)

// 查看存档数据
novaDebug.saveJson()
```

### 常见问题排查

| 问题 | 排查方向 |
|------|----------|
| 游戏包加载失败 | 检查 `.nvmp` 文件完整性，查看控制台错误信息 |
| 资源无法加载 | 确认资源已正确打包到 `.nvmp` 中 |
| 音频无法播放 | 浏览器可能阻止自动播放，需要用户交互后才能播放 |
| 存档恢复失败 | 检查存档版本是否与游戏版本匹配 |

---

## 定制与扩展

### 替换 UI 样式

`style.css` 是纯 CSS，可以直接修改或替换。主要样式类：

- `.vn-dialogue` - VN 模式对话框
- `.vn-choice` - VN 模式选项按钮
- `.vn-sprite` - 立绘
- `.chat-message` - Chat 模式消息气泡
- `.chat-choice` - Chat 模式选项按钮

### 替换渲染逻辑

`app.js` 中的 `renderVNMode()` 和 `renderChatMode()` 是渲染入口。你可以：

1. **完全重写** - 删除现有渲染函数，按你的设计实现
2. **增量修改** - 在现有基础上添加动画、特效
3. **切换框架** - 将原生 DOM 操作替换为 React/Vue/Svelte

### 添加新功能

模板未实现但可以扩展的功能：

- **历史记录回看** - 缓存过去的对话，添加回看 UI
- **自动播放** - 定时调用 `advance()`
- **快进模式** - 跳过打字机效果，快速推进
- **设置面板** - 调整文字速度、音量等
- **存档缩略图** - 在存档时截取屏幕画面

### 资源加载优化

模板已内置资源缓存（`assetCache`、`imageCache`）。如果需要进一步优化：

- 使用 IndexedDB 缓存大型资源
- 实现资源预加载
- 使用 Web Workers 处理解压

---

## 下一步

- [运行时与宿主](./runtime-and-host/) - 理解 VM 与宿主的职责边界
- [运行时状态](../api/runtime-state/) - 完整的状态字段定义
- [C API](../api/c-api/) - 原生平台接入参考
