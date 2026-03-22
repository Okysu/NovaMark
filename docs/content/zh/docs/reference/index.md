---
title: "语法/命令速查表"
weight: 1
---

# 语法/命令速查表

一页总览 NovaMark 核心语法。详细说明见 [命令参考](../syntax/commands/)。

---

## 文件结构

```nvm
---
title: 游戏标题
version: 1.0
---

@var 变量名 = 初始值

#scene_start "场景标题"
```

---

## 对话

```nvm
角色名: 对话内容
> 旁白文本
```

---

## 变量

```nvm
@var hp = 100              // 定义
@var name = "林晓"
@var alive = true

@set hp = 80               // 修改
@set gold = gold + 10      // 运算
```

**支持的运算**: `+`, `-`, `*`, `/`, `%`

---

## 物品

```nvm
// 定义
@item healing_potion
  name: 治疗药水
  description: 恢复 50 HP
  icon: potion.png
@end

// 操作
@give healing_potion 2     // 获得
@take healing_potion 1     // 失去
```

---

## 选择

```nvm
? 提示文本
- [选项1] -> 目标
- [选项2] -> .label

.label
> 选项2的内容
```

---

## 检定

```nvm
@check roll("2d6") >= 8
@success
  > 成功！
@fail
  > 失败...
@endcheck

@check has_item("key")
@check hp >= 50
@check has_flag("met_spirit")
```

**内置函数**:

| 函数 | 用途 |
|------|------|
| `roll("XdY")` | 骰子检定 |
| `has_item("id")` | 检查物品 |
| `item_count("id")` | 物品数量 |
| `has_flag("id")` | 检查标记 |
| `has_ending("id")` | 检查结局 |

---

## 场景

```nvm
#scene_forest "森林"       // 定义

-> scene_forest            // 跳转
-> .label                  // 跳转标签

.label                     // 标签定义
> 标签内容

@call shop_scene           // 调用子场景
@return                    // 从子场景返回
```

---

## 媒体

```nvm
@bg image.png              // 背景
@bg image.png transition:fade

@sprite 角色 show url:img.png position:left
@sprite 角色 hide

@bgm music.mp3             // BGM
@bgm stop

@sfx sound.mp3             // 音效
```

---

## 角色定义

```nvm
@char 小明
  color: #4A90D9
  description: 主角
  sprite_default: xiaoming.png
  sprite_happy: xiaoming_happy.png
@end
```

---

## 结局与标记

```nvm
@flag met_spirit           // 设置标记（跨周目）
@ending good_ending        // 触发结局
```

---

## 条件分支

```nvm
@if hp <= 0
  > 你倒下了...
  -> game_over
@else
  > 你还活着。
@endif
```

---

## 运行时状态

渲染器读取的 JSON 结构（详见 [运行时状态](../api/runtime-state/)）：

```json
{
  "status": 0,
  "currentScene": "scene_start",
  "variables": {
    "numbers": { "hp": 100 },
    "strings": { "playerName": "林晓" },
    "bools": { "metSpirit": true }
  },
  "inventory": { "healing_potion": 2 },
  "inventoryItems": [...],
  "characterDefinitions": {...}
}
```

---

## 更多参考

- [完整命令参考](../syntax/commands/)
- [变量系统详解](../syntax/variables/)
- [场景与标签](../syntax/scenes/)
- [选择与分支](../syntax/branches/)
- [项目配置](../syntax/project-config/)
