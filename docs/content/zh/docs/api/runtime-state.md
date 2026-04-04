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

返回结构示例：

```json
{
  "status": 0,
  "currentScene": "scene_start",
  "currentLabel": "",
  "textConfig": {
    "defaultFont": "SourceHanSansCN-Regular.ttf",
    "defaultFontSize": 28,
    "defaultTextSpeed": 60
  },
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
      "description": "森林的守护者"
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

### textConfig

- `defaultFont`：默认字体
- `defaultFontSize`：默认字号
- `defaultTextSpeed`：默认打字速度

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
- 角色颜色与描述

## 渲染器职责

NovaMark 只负责输出状态，不负责决定 Web、Native 或 CLI 的具体界面布局。客户端应：

1. 读取运行时状态
2. 决定变量、背包与其他界面元素的展示方式
3. 根据 `dialogue.color` 或角色定义颜色渲染对话
4. 根据 `textConfig.defaultTextSpeed` 决定打字机速度

## Web 调试

Web 模板中可直接使用：

```js
novaDebug.runtimeState()
novaDebug.snapshot().runtimeState
```

用于检查当前变量、背包和定义元数据。

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

这让上层工具不仅能看到原始文本，还能精确知道文本内部哪些部分是表达式、哪些部分是样式。

## 存档格式说明

NovaMark 当前推荐的职责划分是：

- **正式存档文件**：二进制格式，供产品环境使用
- **JSON 快照**：供调试、测试、Web/WASM 工具链使用

也就是说，JSON 仍然是重要的开发格式，但不再建议作为玩家正式存档格式。
