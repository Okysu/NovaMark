---
title: "选择和分支"
weight: 4
---

# 选择和分支

## 玩家选择 (?)

让玩家做出选择，影响故事走向。

### 基本语法

```nvm
? 问题描述
- [选项文本] -> 目标场景
- [选项文本] -> .标签
```

### 示例

```nvm
? 你要做什么？
- [检查周围] -> .look_around
- [大声呼救] -> .call_for_help
- [原地等待] -> .wait_here

.look_around
> 你环顾四周...
-> next_scene

.call_for_help
> 没有人回应...
-> next_scene

.wait_here
> 你决定等待...
-> next_scene
```

### 带条件的选择

某些选项可以根据条件显示或禁用：

```nvm
? 你要如何行动？
- [继续前进] -> .continue
- [使用钥匙] -> .use_key
- [使用魔法石] -> .use_stone if has_item("magic_stone")
```

**条件语法**：`-> 目标 if 条件表达式`

### 选择选项详解

| 部分 | 说明 |
|------|------|
| `-` | 选项开始标记 |
| `[文本]` | 显示给玩家的选项文本 |
| `-> 目标` | 选择后跳转的目标 |
| `if 条件` | 可选的条件判断，写在目标之后 |

## 条件检定 (@check)

基于条件执行不同分支。

### 基本语法

```nvm
@check 条件表达式
success
  // 条件为真时执行
fail
  // 条件为假时执行
endcheck
```

### 示例

```nvm
@check hp >= 50
success
  林晓: 我还能坚持！
fail
  林晓: 我需要休息...
  @set hp = 50
endcheck
```

### 骰子检定

```nvm
@check roll("2d6") >= 8
success
  > 你成功通过了检定！
  @set courage = courage + 5
fail
  > 检定失败...
  @set hp = hp - 10
endcheck
```

### 物品检查

```nvm
@check has_item("key")
success
  > 你用钥匙打开了门。
  @take key 1
  -> .inside
fail
  > 门锁着，你没有钥匙。
endcheck
```

### 标记检查

```nvm
@check has_flag("met_spirit")
success
  神秘精灵: 我们又见面了，年轻的旅人。
fail
  > 这片森林似乎隐藏着什么秘密...
endcheck
```

## 分支语句 (if/endif)

基于条件执行代码块。

### 基本语法

```nvm
if 条件表达式
  // 条件为真时执行
endif

if 条件表达式
  // 条件为真时执行
else
  // 条件为假时执行
endif
```

### 示例

```nvm
if has_item("treasure_map")
  > 你拿出地图查看位置。
else
  > 你没有地图，只能凭感觉走。
endif

if gold >= 100
  @set gold = gold - 100
  @give legendary_sword 1
  > 你买下了传奇之剑！
endif
```

## 条件表达式

### 比较运算符

| 运算符 | 说明 | 示例 |
|--------|------|------|
| `==` | 等于 | `hp == 100` |
| `!=` | 不等于 | `hp != 0` |
| `>` | 大于 | `gold > 50` |
| `>=` | 大于等于 | `hp >= 50` |
| `<` | 小于 | `hp < 20` |
| `<=` | 小于等于 | `gold <= 10` |

### 逻辑运算符

| 运算符 | 说明 | 示例 |
|--------|------|------|
| `and` | 与 | `has_item("key") and hp > 0` |
| `or` | 或 | `has_flag("saved") or gold >= 100` |
| `not` | 非 | `not has_flag("boss_defeated")` |

### 算术运算

可以在条件中使用算术表达式：

```nvm
@check str + roll("1d20") >= 15
success
  > 你成功说服了守卫！
fail
  > 守卫不为所动...
endcheck
```

支持的运算：`+`, `-`, `*`, `/`, `%`

## 完整示例

```nvm
#scene_combat "战斗"

@check hp > 0
success
  ? 敌人逼近，你要：
  - [攻击] -> .attack
  - [防御] -> .defend
  - [使用药水] if has_item("healing_potion") -> .use_potion
  - [逃跑] -> .flee
  
  .attack
  @check roll("1d20") >= 10
  success
    > 你的攻击命中了！
    @set enemy_hp = enemy_hp - 15
  fail
    > 你miss了！
    @set hp = hp - 10
  endcheck
  -> .check_result
  
  .defend
  > 你举起盾牌防御。
  @set defense_bonus = 5
  -> .enemy_turn
  
  .use_potion
  @take healing_potion 1
  @set hp = hp + 50
  > 你恢复了 50 点生命值！
  -> .enemy_turn
  
  .flee
  @check roll("1d20") >= 12
  success
    > 你成功逃跑了！
    -> scene_escape
  fail
    > 逃跑失败！
    @set hp = hp - 20
  endcheck
  -> .check_result
  
  .check_result
  @check enemy_hp <= 0
  success
    @flag defeated_enemy
    > 你击败了敌人！
    -> scene_victory
  fail
    @check hp > 0
    success
      -> .enemy_turn
    fail
      > 你被打败了...
      @ending game_over
    endcheck
  endcheck
  
fail
  > 你已经倒下了...
  @ending game_over
endcheck
```
