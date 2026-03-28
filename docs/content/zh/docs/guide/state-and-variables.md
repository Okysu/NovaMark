---
title: "状态、变量与物品"
weight: 3
---

# 状态、变量与物品

写互动剧情时，最重要的一件事是：

> 你的故事必须“记住”已经发生过什么。

比如：

- 玩家是否拿到了钥匙
- 玩家还有多少金钱
- 玩家现在生命值是多少
- 是否见过某个角色

这些东西统称为：**状态**。

---

## 两类常见状态

NovaMark 现在建议你把状态大致分成两类：

### 1. `@var` —— 内部逻辑变量

适合：

- 流程控制
- 内部标记
- 不一定直接展示给玩家的值

例如：

```nvm
@var met_spirit = false
@var chapter_index = 1
@var courage = 10
```

### 2. `@item` —— 面向玩家可见的状态项 / 物品项

适合：

- 传统道具
- 钱币
- 可公开属性
- GUI 想直接展示的状态

例如：

```nvm
@item money
  name: "金钱"
  description: "玩家当前可使用的货币。"
  default_value: 100
@end
```

或者：

```nvm
@item hp
  name: "生命值"
  description: "角色当前生命值。"
  default_value: 100
@end
```

---

## 为什么要分成 `@var` 和 `@item`

因为不是所有状态都适合直接暴露给玩家。

### 更适合 `@var` 的例子

```nvm
@var met_spirit = false
@var hidden_route_open = false
```

这些值更像：

- 剧情内部开关
- 系统记忆
- 分支条件

### 更适合 `@item` 的例子

```nvm
@item money
  name: "金钱"
  default_value: 20
@end

@item sanity
  name: "理智值"
  default_value: 100
@end
```

这些值更像：

- 玩家看得见的资源
- HUD 想展示的条目
- GUI 想拿去做状态面板的数据

---

## 如何定义变量

```nvm
@var hp = 100
@var gold = 50
@var player_name = "沈砚"
@var met_spirit = false
```

这表示：

- `hp` 是数字
- `player_name` 是字符串
- `met_spirit` 是布尔值

---

## 如何修改变量

```nvm
@set hp = hp - 10
@set gold = gold + 20
@set met_spirit = true
```

`@set` 支持表达式，所以你不需要总是先创建临时变量。

---

## 如何给予和移除物品/状态项

```nvm
@give money 10
@take money 2
```

现在 `@give` 和 `@take` 也支持表达式：

```nvm
@take money 1 + 1
@give money random(1, 10)
@take money count + 1
```

这意味着创作者可以更自然地写：

- 固定数值
- 变量
- 函数结果
- 运算表达式

而不必总是绕一层临时变量。

---

## 如何检查状态

NovaMark 里最常用的状态函数包括：

```nvm
has_item("magic_stone")
item_count("money")
has_flag("met_spirit")
has_ending("true_end")
```

这些通常会出现在：

- `if`
- `@check`
- 条件选项

例如：

```nvm
if item_count("money") >= 50
  > 你有足够的钱。
endif
```

---

## 运行时状态为什么对客户端很重要

NovaMark 的设计里，脚本不直接管理 HUD。

所以真正推荐的做法是：

- 脚本维护状态
- 客户端读取状态并决定怎么显示

例如，客户端可以根据运行时状态：

- 把 `money` 显示在状态栏
- 把 `hp` 显示成血条
- 把 `sanity` 显示成数值或图标

这也是为什么 `@item` 现在适合承担“玩家可见状态项”的原因。

---

## 下一步该学什么

现在你已经知道怎么存储和修改状态了。

下一步最自然的问题是：

- 怎么根据状态进入不同剧情？
- 怎么写一次明确的成功/失败检定？

下一页建议看：

- [选择、分支与检定]({{< ref "choices-and-checks" >}})
