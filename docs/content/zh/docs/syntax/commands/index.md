---
title: "命令参考"
weight: 4
---

# 命令参考

本文档列出所有 NovaMark 支持的命令。

## 注释

NovaMark 支持 `//` 单行注释：

```nvm
// 这是整行注释
林晓: 你好！ // 这是行尾注释
```

注释不会被执行，仅用于代码说明。

---

## 定义命令

### @char - 角色定义

定义游戏中的角色。

```nvm
@char 角色名
  color: #RRGGBB
  description: 角色描述
  sprite_default: 默认立绘.png
  sprite_happy: 开心立绘.png
@end
```

**属性**:
| 属性 | 类型 | 说明 |
|------|------|------|
| `color` | 颜色值 | 对话框中角色名的颜色 |
| `description` | 字符串 | 角色描述 |
| `sprite_default` | 文件名 | 默认立绘文件 |
| `sprite_*` | 文件名 | 情绪贴图约定，如 `sprite_happy` |

---

### @item - 物品定义

定义游戏中的物品。

```nvm
@item 物品ID
  name: 显示名称
  description: 物品描述
  icon: 图标文件
@end
```

**属性**:
| 属性 | 类型 | 说明 |
|------|------|------|
| `name` | 字符串 | 物品显示名称 |
| `description` | 字符串 | 物品描述 |
| `icon` | 文件名 | 物品图标 |

---

### @var - 变量定义

定义游戏变量。

```nvm
@var 变量名 = 初始值
```

**支持的类型**:
- 数字: `@var hp = 100`
- 字符串: `@var name = "林晓"`
- 布尔值: `@var is_alive = true`

---

### @theme - 主题定义

定义 UI 主题样式。

```nvm
@theme 主题名
  dialog_bg: #000000
  text_color: #FFFFFF
@end
```

---

## 场景控制

### #scene - 场景定义

定义一个场景。

```nvm
#scene_id "场景标题"
```

**示例**:
```nvm
#scene_forest "幽暗的森林"
#scene_tower "星光之塔"
```

---

### -> - 跳转

跳转到场景或标签。

```nvm
-> scene_name      // 跳转到场景
-> .label_name     // 跳转到当前场景内的标签
```

---

### @call - 调用场景

调用场景，执行完毕后返回。

```nvm
@call shop_scene
```

被调用的场景中使用 `@return` 返回。

---

### @return - 返回

从 `@call` 调用返回。

```nvm
@return
```

---

### .label - 标签定义

定义场景内的标签。

```nvm
.look_around
> 你环顾四周。
```

标签名以 `.` 开头。

---

## 显示命令

### @bg - 背景图片

显示或切换背景图片。

```nvm
@bg image.png
@bg image.png transition:fade
```

**参数**:
| 参数 | 类型 | 说明 |
|------|------|------|
| `transition` | 字符串 | 过渡效果 (fade/slide/none) |

---

### @sprite - 精灵/立绘

显示或切换角色立绘。

```nvm
@sprite 角色名 show url:立绘.png position:left
@sprite 角色名 show url:立绘.png x:70% y:100px
@sprite 角色名 hide
```

**参数**:
| 参数 | 类型 | 说明 |
|------|------|------|
| `show` | - | 显示精灵 |
| `hide` | - | 隐藏精灵 |
| `url` | 文件名 | 精灵图片 |
| `position` | 字符串 | 推荐 `left` / `right`，原样透传 |
| `x` | 字符串/数字 | 平台自行解释 |
| `y` | 字符串/数字 | 平台自行解释 |
| `opacity` | 数字 | 透明度 (0-1) |

---

### @bgm - 背景音乐

播放或停止背景音乐。

```nvm
@bgm music.mp3
@bgm music.mp3 loop:true volume:0.8
@bgm stop
```

**参数**:
| 参数 | 类型 | 说明 |
|------|------|------|
| `loop` | 布尔 | 是否循环播放 |
| `volume` | 数字 | 音量 (0-1) |

---

### @sfx - 音效

播放音效。

```nvm
@sfx click.mp3
@sfx ambient.mp3 loop:true volume:0.5
```

**参数**:
| 参数 | 类型 | 说明 |
|------|------|------|
| `loop` | 布尔 | 是否循环播放 |
| `volume` | 数字 | 音量 (0-1) |

---

## 游戏逻辑

### @set - 修改变量

修改变量值。

```nvm
@set hp = 80
@set gold = gold + 10
@set courage = courage - 5
```

支持运算: `+`, `-`, `*`, `/`, `%`

---

### @give - 给予物品

给玩家添加物品。

```nvm
@give healing_potion 1
@give gold 100
```

---

### @take - 移除物品

从玩家背包移除物品。

```nvm
@take healing_potion 1
```

---

### @check - 条件检定

基于条件执行不同分支。

```nvm
@check 条件表达式
@success
  // 成功时执行
@fail
  // 失败时执行
@endcheck
```

**示例**:
```nvm
@check roll("2d6") >= 8
@success
  林晓: 成功了！
@fail
  林晓: 失败了...
@endcheck

@check has_item("key")
@success
  > 你打开了宝箱。
@fail
  > 没有钥匙。
@endcheck

@check hp >= 50
@success
  林晓: 我还能坚持！
@fail
  林晓: 我需要休息...
  @set hp = 50
@endcheck
```

---

### @flag - 设置标记

设置游戏标记（用于多周目）。

```nvm
@flag met_spirit
```

---

### @ending - 触发结局

触发游戏结局。

```nvm
@ending good_ending
@ending bad_ending
```

---

## 表达式与函数

### 算术运算

```nvm
@set total = a + b
@set diff = a - b
@set product = a * b
@set quotient = a / b
```

### 比较运算

```nvm
@check hp > 0
@check gold >= 100
@check hp == 100
@check hp != 0
```

### 逻辑运算

```nvm
@check has_item("key") and hp > 0
@check not has_flag("boss_defeated")
```

### 内置函数

| 函数 | 说明 | 示例 |
|------|------|------|
| `has_item("id")` | 检查是否有物品 | `has_item("key")` |
| `item_count("id")` | 获取物品数量 | `item_count("gold")` |
| `has_ending("id")` | 检查是否触发过结局 | `has_ending("true_end")` |
| `has_flag("id")` | 检查是否设置过标记 | `has_flag("met_spirit")` |
| `roll("XdY+Z")` | 骰子检定 | `roll("2d6+3")` |

### 骰子表达式

```nvm
roll("2d6")      // 2个6面骰
roll("1d20")     // 1个20面骰
roll("2d6+3")    // 2个6面骰+3修正值
roll("1d8-1")    // 1个8面骰-1修正值
```
