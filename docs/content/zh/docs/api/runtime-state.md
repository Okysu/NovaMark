---
title: "运行时状态与快照 API"
weight: 2
---

# 运行时状态与快照 API

NovaMark 通过统一运行时状态接口向客户端导出游戏执行过程中的动态数据。渲染器、GUI 和调试工具应从该接口读取变量、背包、文本配置与定义元数据，而不是依赖脚本内联的界面描述语法。

NovaMark 不再通过脚本直接控制 HUD 或其他客户端界面元素。开发者应基于变量、背包和对话等运行时数据，自行决定界面布局与刷新时机。

## 运行时状态 JSON

当前 Web/WASM 层导出接口：

```c
const char* nova_export_runtime_state_json(void* vm, size_t* outSize);
```

这个接口现在已经是 **统一的 presentation snapshot（展示态快照）**：

- 保留旧版已有字段
- 新增对渲染层直接有用的展示字段
- 不要求宿主再通过大量单独 getter 拼装完整界面状态

也就是说，宿主现在推荐优先消费这一份 JSON 快照；旧的粒度 getter 仍然保留，用于兼容或低层优化场景。

返回结构示例：

```json
{
  "status": 0,
  "runtimeStateVersion": 12,
  "runtimeStateChangeFlags": 0,
  "currentScene": "scene_start",
  "currentLabel": "",
  "textConfig": {
    "defaultFont": "SourceHanSansCN-Regular.ttf",
    "defaultFontSize": 28,
    "defaultTextSpeed": 60,
    "baseBgPath": "bg/",
    "baseSpritePath": "sprites/",
    "baseAudioPath": "audio/"
  },
  "bg": "room.png",
  "bgTransition": "fade",
  "bgm": "theme.mp3",
  "bgmVolume": 1.0,
  "bgmLoop": true,
  "sprites": [
    {
      "id": "林晓",
      "url": "xiaoxiao_happy.png",
      "position": "left",
      "opacity": 1.0,
      "zIndex": 1
    }
  ],
  "sfx": [
    {
      "id": "click",
      "path": "click.wav",
      "loop": false,
      "volume": 0.5
    }
  ],
  "dialogue": {
    "isShow": true,
    "speaker": "林晓",
    "text": "你好 世界",
    "segments": [
      { "text": "你好 ", "style": "" },
      { "text": "世界", "style": "accent" }
    ],
    "emotion": "happy",
    "color": "#90EE90"
  },
  "choice": {
    "isShow": true,
    "question": "你的名字是 林晓 吗？",
    "questionSegments": [
      { "text": "你的", "style": "" },
      { "text": "名字", "style": "accent" },
      { "text": "是 ", "style": "" },
      { "text": "林晓", "style": "" },
      { "text": " 吗？", "style": "" }
    ],
    "options": [
      {
        "id": "0",
        "text": "继续",
        "target": ".next",
        "disabled": false,
        "segments": [
          { "text": "继续", "style": "" }
        ]
      }
    ]
  },
  "endingId": "good_end",
  "endingTitle": "好结局",
  "variables": {
    "numbers": { "hp": 100, "gold": 20 },
    "strings": { "playerName": "林晓" },
    "bools": { "metSpirit": true }
  },
  "inventory": {
    "healing_potion": 2
  },
  "itemDefinitions": {
    "healing_potion": {
      "id": "healing_potion",
      "name": "治疗药水",
      "description": "恢复生命值"
    }
  },
  "characterDefinitions": {
    "神秘精灵": {
      "id": "神秘精灵",
      "color": "#90EE90",
      "description": "森林的守护者",
      "sprites": {
        "happy": "spirit_happy.png"
      }
    }
  },
  "inventoryItems": [
    {
      "id": "healing_potion",
      "name": "治疗药水",
      "description": "恢复生命值",
      "count": 2
    }
  ]
}
```

## 字段说明

### status / runtimeStateVersion / runtimeStateChangeFlags

- `status`：运行状态，0=运行中，1=等待选择，2=已结束
- `runtimeStateVersion`：运行时状态版本号，可用于宿主缓存或增量刷新判断
- `runtimeStateChangeFlags`：状态变化标记位，供宿主做轻量更新优化

### textConfig

- `defaultFont`：默认字体
- `defaultFontSize`：默认字号
- `defaultTextSpeed`：默认打字速度
- `baseBgPath`：背景资源基路径
- `baseSpritePath`：立绘资源基路径
- `baseAudioPath`：音频资源基路径

### 媒体与展示字段

- `bg`：当前背景图
- `bgTransition`：背景切换提示
- `bgm`：当前背景音乐
- `bgmVolume`：背景音乐音量
- `bgmLoop`：背景音乐是否循环
- `sprites`：当前立绘状态数组
- `sfx`：当前待消费/待表现的音效数组
- `endingId` / `endingTitle`：当前结局信息（如果已进入结局）

### dialogue

- `text`：向后兼容的平铺纯文本
- `segments`：结构化文本片段，`style == ""` 表示普通文本
- `emotion`：当前情绪标签
- `color`：当前对话颜色

### choice

- `question`：向后兼容的平铺纯文本问题
- `questionSegments`：选择题问题的结构化文本片段
- `options[].text`：向后兼容的平铺纯文本选项
- `options[].segments`：每个选项的结构化文本片段

### variables

- `numbers`：数值变量
- `strings`：字符串变量
- `bools`：布尔变量

### inventory / inventoryItems

- `inventory`：底层物品 ID 到数量的映射
- `inventoryItems`：GUI 友好的物品数组，包含显示名、描述和数量

### itemDefinitions / characterDefinitions

用于 GUI 查询静态定义元数据：

- 物品显示名与描述
- 角色颜色、描述与立绘映射

## 推荐使用方式

### 优先使用统一快照

对大多数宿主来说，推荐直接消费 `nova_export_runtime_state_json` 导出的统一展示态快照，而不是用大量粒度 getter 手动拼装：

1. 一次读取完整 presentation snapshot
2. 直接渲染背景、BGM、立绘、对话、选项与状态栏
3. 需要结构化文本时消费 `dialogue.segments` / `choice.questionSegments` / `choice.options[].segments`

### 何时继续使用粒度 getter

以下场景仍然适合保留旧 getter：

- 你只想读某一个字段
- 你想避免解析 JSON
- 你在做低层桥接或增量优化

旧 getter 目前全部保留，作为兼容层和低层接口继续有效。

## 渲染器职责

NovaMark 只负责输出状态，不负责决定 Web、Native 或 CLI 的具体界面布局。客户端应：

1. 读取运行时状态快照
2. 决定变量、背包与其他界面元素的展示方式
3. 根据 `dialogue.color` 或角色定义颜色渲染对话
4. 根据 `dialogue.segments` / `choice.*Segments` 决定局部样式渲染
5. 根据 `textConfig.defaultTextSpeed` 决定打字机速度

## Web 调试

Web 模板中可直接使用：

```js
novaDebug.runtimeState()
novaDebug.snapshot().runtimeState
```

模板侧的 `NovaRenderer` 也提供：

```js
renderer.getRuntimeState()
renderer.getPresentationState()
```

其中：

- `getRuntimeState()`：旧名称，保持兼容
- `getPresentationState()`：新名称，强调其“统一展示态快照”的语义

两者当前返回相同的数据结构。

## AST 快照导出

除了运行时状态快照，NovaMark 还提供 **AST 快照导出**，用于调试解析结果、构建 Creator 工具链、或验证脚本最终被编译成了什么结构。

它和运行时状态快照的区别是：

- **运行时状态快照**：描述“游戏当前运行到了哪里、变量是什么、界面该显示什么”
- **AST 快照**：描述“脚本被解析后形成了什么语法树”

### C++ 导出接口

```cpp
std::string export_ast_snapshot_string(const ProgramNode* program);
std::string export_ast_snapshot_string_from_scripts(const std::vector<MemoryScript>& scripts);
std::string export_ast_snapshot_string_from_path(const std::string& path);
```

### C API 导出接口

```c
char* nova_export_ast_snapshot_from_path(const char* path);
char* nova_export_ast_snapshot_from_scripts(const NovaMemoryScript* scripts, size_t count);
```

### 适用场景

- 调试 Parser 输出
- 校验多脚本项目最终合并后的 Program 结构
- 为 Creator / 编辑器提供 AST 浏览能力
- 为测试或 CI 导出稳定的 JSON 结构进行比对

### 输出格式

AST 快照当前以 **JSON 字符串** 形式导出。顶层结构示意：

```json
{
  "version": 1,
  "root": {
    "type": "Program",
    "children": []
  }
}
```

对于包含文本插值的节点，快照中会额外出现 `InterpolatedText` 结构，并记录分段信息，例如：

- `PlainText`：普通文本片段
- `Interpolation`：`{{expr}}` 插值片段
- `InlineStyle`：`{style:text}` 内联样式片段

这让上层工具不仅能看到原始文本，还能精确知道文本内部哪些部分是表达式、哪些部分是样式。当前这套结构也已经用于运行时的 `dialogue.segments`、`choice.questionSegments` 与 `choice.options[].segments`。

## 存档格式说明

NovaMark 当前推荐的职责划分是：

- **正式存档文件**：二进制格式，供产品环境使用
- **JSON 快照**：供调试、测试、Web/WASM 工具链使用

也就是说，JSON 仍然是重要的开发格式，但不再建议作为玩家正式存档格式。
