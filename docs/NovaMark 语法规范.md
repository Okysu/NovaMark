# NovaMark 语法指南

NovaMark 是一门为**文字游戏 / 视觉小说 / 互动叙事**设计的脚本语言。

它的目标不是做一个“什么都能写”的通用编程语言，而是让创作者用尽量自然的方式表达：

- 谁在说话
- 现在显示什么画面
- 玩家可以做什么选择
- 变量、背包、结局如何变化
- 客户端应该拿到哪些运行时状态

这份文档采用**教程化**结构来讲解 NovaMark：

1. 先理解一个脚本由什么组成
2. 再学习最常用的语法
3. 最后再进入条件、选择、随机和资源控制

> 如果你是第一次接触 NovaMark，建议按顺序阅读。

---

## 1. 一个 NovaMark 文件长什么样

一个 `.nvm` 文件通常由几部分组成：

1. **元信息块**：描述游戏标题、资源路径、默认字体等
2. **定义块**：集中定义角色和物品
3. **场景块**：真正的剧情内容

一个最小示例：

```nvm
---
title: 失落的灯塔
default_font: SourceHanSansCN-Regular.ttf
---

@char 林晓
  color: #E8A0BF
  sprite_default: linxiao_default.png
@end

@item key
  name: "旧钥匙"
  description: "生锈但还能用。"
  icon: key.png
@end

@var hp = 100

#scene_intro "序章"

@bg room.png
林晓: 我这是在哪里？
> 你从陌生的房间里醒来。
```

你可以把它理解成：

- **上面**定义世界和数据
- **下面**编写剧情和交互

---

## 2. 元信息块（Front Matter）

元信息块写在文件最顶部，用 `---` 包起来。

它适合放：

- 标题
- 作者
- 版本
- 资源基础路径
- 默认字体与文字速度

```nvm
---
title: 迷雾森林
author: NovaMark Team
version: 1.0
base_bg_path: assets/bg/
base_sprite_path: assets/sprites/
base_audio_path: assets/audio/
base_icon_path: assets/icons/
default_font: SourceHanSansCN-Regular.ttf
default_font_size: 28
default_text_speed: 60
---
```

### 你应该如何理解它

它不是剧情本身，而是**整个脚本的默认配置**。

例如：

- 你写 `@bg forest.png` 时，客户端可以结合 `base_bg_path` 去找到真实资源
- 你没有手动写字体时，客户端可以使用 `default_font`

---

## 3. 定义角色与物品

NovaMark 推荐把角色和物品放在前面统一定义。

这样做的好处是：

- 脚本主体更干净
- GUI 能拿到角色和物品的元数据
- Web / Native / CLI 渲染逻辑更统一

### 3.1 角色定义 `@char`

```nvm
@char 林晓
  color: #E8A0BF
  description: 年轻的冒险者
  sprite_default: linxiao_default.png
  sprite_happy: linxiao_happy.png
@end
```

### 角色定义目前最重要的字段

| 字段 | 作用 |
|---|---|
| `color` | 对话时角色名颜色 |
| `description` | 角色说明 |
| `sprite_default` | 默认立绘 |
| `sprite_*` | 情绪立绘，例如 `sprite_happy` |

### 情绪立绘规则

NovaMark 现在正式支持这套约定：

- `林晓:` → 使用 `sprite_default`
- `林晓[happy]:` → 使用 `sprite_happy`

这套规则的好处是：

- 语法直观
- 客户端不用猜测角色情绪贴图如何命名

---

### 3.2 物品定义 `@item`

```nvm
@item healing_potion
  name: "治疗药水"
  description: "恢复生命值。"
  icon: healing_potion.png
@end
```

### 物品定义目前推荐使用的字段

| 字段 | 作用 |
|---|---|
| `name` | 物品显示名 |
| `description` | 物品描述 |
| `icon` | GUI 图标 |

这些定义会被导出到运行时状态里，方便 GUI 渲染背包、道具栏和提示信息。

---

## 4. 场景是什么

场景是剧情组织的基本单位。

```nvm
#scene_intro "序章"
```

你可以把一个场景理解为：

- 一个章节
- 一个房间
- 一个剧情片段
- 一段连续的事件流

通常一个脚本里会有多个场景：

```nvm
#scene_intro "序章"
#scene_forest "迷雾森林"
#scene_tower "星光之塔"
```

---

## 5. 两种最常见的文本：旁白和对话

NovaMark 最核心的内容，其实就两种：

- **旁白**
- **角色对话**

### 5.1 旁白

```nvm
> 月光透过树梢洒落，森林安静得出奇。
```

旁白用于：

- 描述环境
- 描述动作
- 描述气氛
- 表达系统文字

### 5.2 角色对话

```nvm
林晓: 这里是哪里？
林晓[happy]: 太好了，我们找到出口了！
```

对话用于：

- 角色发言
- 情绪变化
- 驱动立绘切换

---

## 6. 跳转、标签、子场景调用

剧情脚本一定会遇到“接下来去哪”。

NovaMark 提供三种常见方式：

### 6.1 跳到另一个场景

```nvm
-> scene_forest
```

### 6.2 跳到当前场景内的标签

```nvm
.look_around
> 你环顾四周。

-> .look_around
```

### 6.3 调用并返回

```nvm
@call shop_scene
@return
```

这更适合：

- 商店
- 战斗子流程
- 可复用剧情片段

---

## 7. 变量、背包与状态变化

NovaMark 里最常见的运行时状态包括：

- 数值变量（如 HP、金币）
- 字符串变量
- 布尔变量
- 背包物品

### 7.1 定义变量

```nvm
@var hp = 100
@var gold = 50
@var player_name = "林晓"
@var met_spirit = false
```

### 7.2 修改变量

```nvm
@set hp = hp - 10
@set gold = gold + 20
```

### 7.3 给予和移除物品

```nvm
@give healing_potion 1
@take gold_coin 50
```

### 你应该如何理解这些状态

NovaMark 负责维护这些**游戏数据**，但不直接控制 HUD/UI。

也就是说：

- 引擎输出变量、背包、对话状态
- 客户端自己决定怎么显示

这也是当前 NovaMark 的重要设计原则：

> **脚本负责内容与状态，客户端负责界面。**

---

## 8. 条件判断：if / else

最直接的分支方式是 `if`。

```nvm
if hp > 0
  > 你还活着。
else
  > 你失去了意识。
endif
```

它适合：

- 简单条件判断
- 局部叙事变化
- 写起来像普通流程控制

### 条件里可以用什么

- 比较运算：`>`, `<`, `>=`, `<=`, `==`, `!=`
- 逻辑运算：`and`, `or`, `not`
- 内置函数：`has_item()`, `has_flag()` 等

例如：

```nvm
if item_count(gold_coin) >= 50 and courage > 10
  > 你有足够的金币，也有足够的勇气。
endif
```

---

## 9. 选择分支

互动叙事最关键的功能之一，就是给玩家选项。

```nvm
? 你要怎么做？
- [检查周围] -> .look_around
- [原地休息] -> .rest_here
```

### 带条件的选项

```nvm
? 塔门紧闭，你要：
- [尝试推门] -> .try_door
- [使用魔法石] -> .open_with_stone if has_item("magic_stone")
```

如果条件不满足，选项会被置为 disabled。

---

## 10. `@check`：更像“检定块”而不是普通 if

当你想表达：

- 检定成功做一套事
- 检定失败做另一套事

推荐使用 `@check`。

```nvm
@check roll("1d20") + courage >= 15
@success
  > 你成功通过了检定！
  @set courage = courage + 5
@fail
  > 检定失败。
  @set hp = hp - 10
@endcheck
```

### 为什么有 `@check`

因为很多互动叙事 / 跑团式系统里，“成功 / 失败” 是非常自然的一对概念。

相比普通 `if`：

- `if` 更通用
- `@check` 更强调“检定感”

### 当前正式语法

```nvm
@check 条件表达式
@success
  ...
@fail
  ...
@endcheck
```

> 旧的裸关键字 `success / fail / endcheck` 已经删除，不再支持。

---

## 11. 随机、骰子与常用函数

NovaMark 现在提供了一组适合叙事游戏的内置函数。

### 11.1 骰子

```nvm
@var damage = roll("2d6") + 3
```

支持：

- `roll("2d6")`
- `roll("1d20")`
- `roll("2d6+3")`
- `roll("1d8-1")`

### 11.2 随机数

```nvm
@var reward = random(1, 100)
```

表示在区间内生成一个整数。

### 11.3 概率判定

```nvm
@var crit = chance(0.25)
```

表示有 25% 概率返回 true。

### 11.4 状态查询函数

```nvm
has_item("magic_stone")
item_count("gold_coin")
has_flag("met_spirit")
has_ending("good_ending")
```

这些函数主要用于：

- 条件判断
- 选择解锁
- 分支控制

---

## 12. 媒体控制

NovaMark 提供基础媒体命令，但不会替你决定最终 UI 布局。

### 12.1 背景

```nvm
@bg forest_night.png transition:fade
```

### 12.2 立绘 / 精灵

```nvm
@sprite 林晓 show url:linxiao.png position:left
@sprite 林晓 show url:linxiao.png x:70% y:100px
@sprite 林晓 hide
```

### 关于 position / x / y

- `position` 推荐写 `left` / `right`
- `x` / `y` 可以写字符串样式值
- 最终怎么解释，由不同平台自己决定

这和 NovaMark 当前整体设计一致：

> 引擎输出状态，客户端负责呈现。

### 12.3 背景音乐与音效

```nvm
@bgm ambient_forest.mp3 loop:true volume:0.3
@sfx unlock.mp3
```

---

## 13. 主题和文本样式

如果你需要定义一组统一样式，可以使用 `@theme`。

```nvm
@theme dark_forest
  dialog_bg: #1a1a2e
  text_color: #ffffff
@end
```

同时，普通文本中也支持一些内联样式：

```nvm
林晓: {shake:不要过来！}
> 当前生命：{{hp}}
```

---

## 14. 什么属于脚本，什么属于客户端

这是 NovaMark 很重要的一条边界：

## 脚本负责

- 剧情内容
- 分支逻辑
- 状态变化
- 角色/物品定义
- 媒体资源引用

## 客户端负责

- HUD
- 背包界面
- 按钮样式
- 动画表现
- 页面布局
- 打字机效果细节

这意味着你可以把同一个 `.nvmp`：

- 在 Web 里渲染成聊天界面
- 在 Native 里渲染成视觉小说界面
- 在 CLI 里渲染成纯文本体验

而脚本本身不需要重写。

---

## 15. 运行时状态与存档的关系

NovaMark 当前支持：

- 运行时状态导出
- 快照保存与恢复
- 文件存档 API

但脚本语言层**不再提供 `@save` 语法**。

也就是说：

- 存档是宿主 / 客户端能力
- 不是脚本语法能力

这样职责更清楚：

- 脚本负责剧情和状态
- 客户端负责何时保存、保存到哪里、如何读档

---

## 16. 一份更完整的示例

```nvm
---
title: 迷雾森林
default_font: SourceHanSansCN-Regular.ttf
---

@char 林晓
  color: #E8A0BF
  sprite_default: linxiao_default.png
  sprite_happy: linxiao_happy.png
@end

@item magic_stone
  name: "魔法石"
  description: "会发光的神秘宝石。"
  icon: magic_stone.png
@end

@var hp = 100
@var courage = 10

#scene_intro "序章"

@bg forest_night.png transition:fade
@bgm ambient_forest.mp3 loop:true volume:0.3

林晓: 这里是哪里？
> 你从迷雾森林中醒来。

? 你要怎么做？
- [检查周围] -> .look_around
- [大声呼救] -> .call_help

.look_around
@check roll("2d6") >= 8
@success
  > 你在草丛里发现了一颗发光的石头。
  @give magic_stone 1
@fail
  > 你什么也没有找到。
@endcheck

-> .continue

.call_help
林晓: 有人吗？！
@set courage = courage - 2
-> .continue

.continue
if has_item("magic_stone")
  林晓[happy]: 也许这能帮上忙。
endif
```

---

## 17. 你接下来应该看什么

如果你已经读到这里，建议继续看：

- 运行时状态与快照 API
- 命令参考
- 分支与变量说明
- 示例脚本

如果你是第一次使用 NovaMark，最好的方式不是先记住全部语法，而是：

1. 先写一个只有 1 个场景的脚本
2. 加 1 个变量
3. 加 1 个选择
4. 再加 `@check`
5. 最后再接 GUI

这样会比死记语法更快掌握这门语言。
