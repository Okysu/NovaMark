---
title: "变量和物品"
weight: 3
---

# 变量和物品

## 变量定义

```nvm
@var hp = 100
@var gold = 50
@var name = "林晓"
@var is_alive = true
```

## 变量修改

```nvm
@set hp = 80
@set gold = gold + 10
@set hp = hp - 20
```

## 物品定义

```nvm
@item healing_potion
  name: 治疗药水
  description: 恢复 50 点生命值
  rarity: common
@end
```

## 物品操作

### 给予物品

```nvm
@give healing_potion 1
@give gold 100
```

### 移除物品

```nvm
@take healing_potion 1
```

## 检查物品

在条件中使用：

```nvm
@check has_item("healing_potion")
@success
  > 你使用了一瓶治疗药水。
  @take healing_potion 1
  @set hp = hp + 50
@fail
  > 你没有任何药水。
@endcheck
```

### 检查物品数量

```nvm
@check item_count("gold") >= 100
@success
  > 你有足够的金币。
@fail
  > 金币不足。
@endcheck
```

## 内置函数

| 函数 | 说明 |
|------|------|
| `has_item("item_id")` | 检查是否有物品 |
| `item_count("item_id")` | 获取物品数量 |
| `has_ending("ending_id")` | 检查是否触发过结局 |
| `has_flag("flag_id")` | 检查是否设置过标记 |
| `roll("2d6+3")` | 骰子检定 |
| `random(1, hp)` | 生成区间内随机整数 |
| `chance(0.25)` | 按概率返回布尔结果 |
