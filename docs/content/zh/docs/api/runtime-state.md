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
