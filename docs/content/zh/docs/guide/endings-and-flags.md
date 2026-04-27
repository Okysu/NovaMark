---
title: "结局与标记"
weight: 8
---

# 结局与标记

写多结局故事时，最重要的两个问题是：

> 玩家触发了哪个结局？
> 玩家在旅途中做出过哪些关键选择？

NovaMark 用 `@ending` 和 `@flag` 来回答这两个问题。

---

## 1. `@flag`：记录关键选择

`@flag` 用来标记"这件事发生过"。

它的语义很简单：设一个开关，之后可以检查它是否被开启过。

### 基本用法

```nvm
@flag saved_village
@flag met_spirit
@flag chose_dark_path
```

### 什么时候该用 `@flag`

适合记录那些：

- 不需要具体数值，只需要"是/否"的事情
- 跨场景的重要决策
- 影响后续剧情走向的关键事件

比如：

```nvm
// 玩家救了村民
> 你帮助村民击退了强盗。
@flag saved_village

// 之后在其他场景可以检查
if has_flag("saved_village")
  村民长: 恩人，你终于回来了！
else
  村民长: 我不认识你。
endif
```

### 检查标记

使用 `has_flag()` 函数：

```nvm
if has_flag("met_spirit")
  精灵: 我们又见面了。
else
  > 森林深处传来奇异的光芒...
endif
```

也可以用在条件选项里：

```nvm
? 你要怎么处理这个暗门？
- [离开] -> .leave
- [深入探索] -> .secret_room if has_flag("found_map")
```

---

## 2. `@ending`：触发结局

`@ending` 用来标记故事到达了某个结局。

它的语义是：

> 故事到此结束，这是一个正式的终点。

### 基本用法

```nvm
@ending good_ending
@ending bad_ending
@ending true_ending
```

### 结局触发示例

```nvm
#scene_final "命运的抉择"

? 站在命运的十字路口，你选择：
- [拯救世界] -> .save_world
- [追求力量] -> .seek_power

.save_world
> 你牺牲自己，拯救了所有人。
@flag heroic_sacrifice
@ending hero_ending

.seek_power
> 你获得了无尽的力量，但失去了所有朋友。
@ending dark_ending
```

### 结局会被记录

触发过的结局会被保存，可以在以下场景使用：

- 周目回顾，展示已解锁的结局
- 新周目根据历史解锁特殊内容
- 成就系统

用 `has_ending()` 检查：

```nvm
if has_ending("true_ending")
  神秘声音: 你已经见过真相了...
  -> .secret_epilogue
endif
```

---

## 3. `@flag` 与 `@ending` 的区别

很多人会问：它们不都是"标记"吗？

是的，但语义不同：

| 指令 | 语义 | 用途 |
|------|------|------|
| `@flag` | 这件事发生过 | 记录中间过程 |
| `@ending` | 故事到此结束 | 标记正式终点 |

简单来说：

- `@flag` 是旅途中的路标
- `@ending` 是旅途的终点

---

## 3.5 只继承结局与标记，不恢复现场

在多周目或跨设备同步场景里，你有时并不想“读档回到旧现场”，而只是想继承：

- 已经达成过哪些结局
- 已经触发过哪些关键标记

NovaMark 现在支持这种模式：

- 导入完整存档
- 只提取 `triggeredEndings` 和 `flags`
- 不恢复变量、背包、当前场景、对话、BGM、立绘等运行时状态

这意味着你可以安全地做：

- 新周目继承历史结局
- 已解锁内容继承
- 图鉴/成就/真结局条件同步

而不会把玩家强行传送回旧存档的剧情位置。

---

## 4. 多结局设计模式

### 模式一：分支式

玩家在关键节点做出选择，直接进入不同结局。

```nvm
#scene_climax "最终抉择"

? 面对被封印的古神，你决定：
- [重新封印] -> .seal
- [释放古神] -> .release
- [与它对话] -> .talk if has_flag("learned_truth")

.seal
> 你将古神永远封印，世界恢复了和平。
@ending peace_ending

.release
> 古神苏醒，世界陷入混沌。
@ending chaos_ending

.talk
> 古神: 你是第一个试图理解我的人...
> 古神的力量融入你的灵魂，你成为了新的守护者。
@ending guardian_ending
```

### 模式二：累积式

玩家的多个选择累积影响最终结果。

```nvm
// 在游戏开始定义一个变量
@var karma = 0

// 玩家的每个选择都会影响 karma
// 场景 A
? 路边的乞丐向你乞讨：
- [给钱] -> .give
- [无视] -> .ignore

.give
@set karma = karma + 10
-> next_scene

.ignore
@set karma = karma - 5
-> next_scene

// 场景 B
? 有人被欺负：
- [出手相助] -> .help
- [袖手旁观] -> .watch

.help
@flag helped_stranger
@set karma = karma + 15
-> next_scene

.watch
@set karma = karma - 10
-> next_scene

// 最终根据累积值判定结局
#scene_ending "终章"

if karma >= 50
  > 你的善行感动了世界。
  @ending light_ending
else if karma >= 20
  > 你活得平静而知足。
  @ending neutral_ending
else
  > 你走在孤独的黑暗中。
  @ending dark_ending
endif
```

### 模式三：隐藏条件式

某些结局需要满足特定条件才能触发。

```nvm
#scene_final "深渊"

if has_flag("saved_spirit") and has_flag("found_truth") and karma >= 30
  > 精灵出现在你面前，为你指明了真正的道路。
  @flag true_route_open
  -> .true_ending
else
  -> .normal_route
endif

.true_ending
> 你终于明白了世界的真相。
@ending true_ending

.normal_route
> 你完成了旅程，但心中仍有疑惑。
@ending normal_ending
```

---

## 5. 完整示例：三结局故事

```nvm
---
title: 命运之塔
---

@var courage = 10
@var wisdom = 10

@char 旅人
  color: #87CEEB
@end

#scene_start "塔的入口"

旅人: 传说这座塔的顶端藏着世界的真理。

? 你要如何进入？
- [正门] -> .main_entrance
- [侧面的密道] -> .secret_passage if has_item("old_map")

.main_entrance
@flag entered_frontally
> 你推开沉重的大门。
-> scene_tower

.secret_passage
@flag found_shortcut
旅人: 你居然知道这条密道！
-> scene_tower_core

#scene_tower "塔中"

? 遇到一只受伤的精灵，你：
- [救助它] -> .help_spirit
- [无视它] -> .ignore_spirit

.help_spirit
@flag saved_spirit
@set wisdom = wisdom + 10
精灵: 感谢你...我会记住这份恩情。
-> scene_choice

.ignore_spirit
@set courage = courage + 5
> 你继续前进。
-> scene_choice

#scene_choice "关键抉择"

旅人: 塔顶就在眼前，但我感觉到了危险。

? 最后的岔路：
- [独自前进] -> .alone
- [与旅人同行] -> .together

.alone
@flag chose_solitude
-> scene_ending

.together
@flag chose_partnership
-> scene_ending

#scene_ending "命运"

if has_flag("saved_spirit") and has_flag("chose_partnership")
  // 真结局
  > 精灵的力量与旅人的信任，让你看到了塔顶的真相。
  精灵: 这就是世界的本源...
  旅人: 我们一起见证了它。
  @ending true_ending
  
else if has_flag("chose_partnership")
  // 好结局
  > 你和旅人一起到达了塔顶。
  旅人: 虽然没有看到全部的真相，但这段旅程本身就是答案。
  @ending good_ending
  
else if has_flag("saved_spirit")
  // 普通结局
  > 精灵在最后关头出现，救了你一命。
  精灵: 你的善良值得回报。
  @ending spirit_ending
  
else
  // 悲剧结局
  > 你独自站在塔顶，四周空无一人。
  > 没有真相，没有答案，只有无尽的孤独。
  @ending lonely_ending
endif
```

---

## 6. 设计建议

### 结局数量

- 短篇游戏：2-3 个结局
- 中篇游戏：3-5 个结局  
- 长篇游戏：5-10 个结局

太多结局会让玩家感到疲惫，太少又缺乏探索动力。

### 结局命名

使用有意义的 ID，方便后续管理：

```nvm
@ending true_ending      // 真结局
@ending good_ending      // 好结局
@ending normal_ending    // 普通结局
@ending bad_ending       // 坏结局
@ending secret_ending    // 隐藏结局
```

### 让标记有据可查

不要滥用 `@flag`，只标记真正重要的节点。一个游戏里有 10-20 个标记通常就足够了。

---

## 小结

- `@flag` 记录旅途中的重要事件，用 `has_flag()` 检查
- `@ending` 标记故事的正式终点，用 `has_ending()` 检查
- 多结局可以采用分支式、累积式、隐藏条件式等设计模式
- 合理控制结局数量，让每个结局都有意义

现在你已经掌握了 NovaMark 的核心功能，可以开始创作自己的互动故事了。

---

## 下一步该学什么

恭喜你已经完成了 NovaMark 创作者指南的核心内容！

如果你想深入了解更多细节，建议接下来看：

- [语法参考]({{< ref "/docs/syntax" >}}) - 详细的语法手册和命令参考
- [速查参考]({{< ref "/docs/reference" >}}) - 快速查找语法、状态、API 与配置
- [安装指南]({{< ref "/docs/getting-started/installation" >}}) - 搭建开发环境
- [快速入门]({{< ref "/docs/getting-started/quickstart" >}}) - 从零开始创建项目

现在，开始创作你的互动故事吧！
