---
title: "正式支持清单"
weight: 3
---

# 正式支持清单

本文档列出 **当前 NovaMark 正式支持** 的语法、函数、运行时能力与宿主职责边界。

这份清单的目标不是展示所有历史设计，而是明确：

- 现在什么是正式支持的
- 什么是推荐用法
- 什么能力属于宿主，而不是脚本引擎

---

## 1. 正式支持的脚本语法

### 1.1 文件结构

- Front Matter (`---`)
- 场景定义 `#scene_xxx "标题"`
- 标签 `.label`

### 1.2 文本与对话

- 旁白：`> 文本`
- 对话：`角色: 文本`
- 情绪对话：`角色[emotion]: 文本`

### 1.3 定义块

- `@var`
- `@char`
- `@item`

`@item` 当前适合定义**面向玩家可见、可展示的状态项或道具项**。

例如：

- 传统道具：钥匙、药水、金币
- 可公开属性：hp、money、sanity

支持字段包括：

- `name`
- `description`
- `icon`
- `default_value`

### 1.4 流程控制

- `-> scene`
- `-> .label`
- `@call`
- `@return`
- `if / else / endif`
- `@check / @success / @fail / @endcheck`

### 1.5 状态变化

- `@set`
- `@give`
- `@take`
- `@flag`
- `@ending`

其中 `@set / @give / @take` 当前都支持表达式参数，而不仅仅是字面量。

例如：

```nvm
@take money 1 + 1
@give money random(1, 10)
@set hp = hp - item_count("wound")
```

### 1.6 媒体引用

- `@bg`
- `@sprite`
- `@bgm`
- `@sfx`

### 1.7 选择

- `? 问题`
- `- [选项] -> 目标`
- `- [选项] -> 目标 if 条件`

---

## 2. 正式支持的内置函数

- `has_item(...)`
- `item_count(...)`
- `has_flag(...)`
- `has_ending(...)`
- `roll(...)`
- `random(min, max)`
- `chance(probability)`

### 参数规则

#### 状态查询类

以下函数支持：

- 字符串字面量
- 标识符

```nvm
has_item("magic_stone")
has_flag("met_spirit")
item_count(gold_coin)
```

#### 骰子

`roll()` 需要字符串骰子表达式：

```nvm
roll("2d6")
roll("1d20+3")
```

#### 随机数

`random()` 支持数值表达式：

```nvm
random(1, hp)
```

#### 概率判定

`chance()` 支持 0~1 的数值表达式：

```nvm
chance(0.25)
```

---

## 3. 正式支持的运行时能力

### 3.1 离散推进模型

NovaMark 当前正式采用：

- `advance()`
- `choose(choiceId)`

也就是说，宿主不需要理解内部“消费对话”的机制，只需要表达：

- 继续推进
- 选择某个选项

### 3.2 运行时状态导出

当前正式支持导出：

- 当前场景/标签
- 变量
- 背包
- itemDefinitions
- characterDefinitions
- inventoryItems
- dialogue
- choices
- bg / bgm / sprites
- textConfig

### 3.3 快照与存档

当前正式支持：

- 运行时快照捕获
- 快照恢复
- 文件存档/读档

### 存档格式规则

- **正式文件存档**：二进制
- **JSON**：调试 / 测试 / Web/WASM 工具格式

---

## 4. 保留但应理解为“渲染提示”的字段

这些字段保留，但它们不是引擎内建时间轴或动画系统：

- `transition`
- `position`
- `opacity`
- `loop`
- `volume`

它们表达的是：

- 脚本希望传达的渲染意图
- 宿主可以消费的状态提示

而不是：

- 引擎自动执行连续动画
- 引擎自动管理媒体时间流

---

## 5. 宿主职责边界

NovaMark 当前遵循以下原则：

1. 引擎只产出离散状态，不直接拥有连续时间
2. 宿主控制推进、保存、时间、动画
3. 语法不要暗示比实现更多的能力
4. 对外 API 只表达宿主意图，不暴露内部消费机制
5. 同一件事只保留一套主要语义，不要多套近义接口并存

### 脚本负责

- 剧情内容
- 分支逻辑
- 状态变化
- 媒体引用
- 角色/物品定义

### 宿主负责

- UI 布局
- HUD
- 打字机实现细节
- 动画/过渡时间
- 存档时机
- 文件管理

---

## 6. 推荐使用方式

### 对脚本作者

- 用 `if` 表达一般条件分支
- 用 `@check` 表达明确的成功/失败检定
- 用 `@char` / `@item` 做统一定义
- 把 UI 当成宿主逻辑，不要在脚本里期待 HUD 控制语义

### 对宿主开发者

- 用 `advance()` 推进剧情
- 用 `choose(choiceId)` 选择选项
- 用 runtime state 决定界面渲染
- 用二进制快照做正式存档
- 把 JSON 接口只当作调试工具
