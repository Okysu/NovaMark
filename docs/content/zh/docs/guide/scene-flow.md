---
title: "场景组织与调用"
weight: 7
---

# 场景组织与调用

当你的故事越来越长，很快就会遇到一个问题：

> 怎么组织场景，才能不写重复代码？

NovaMark 提供两种关键机制帮你解决这个问题：

1. **场景内标签** —— 把场景切成更小的段落
2. **@call / @return** —— 像函数一样调用一个场景，执行完再回来

---

## 1. 先回顾：场景和标签

你已经知道场景用 `#scene_xxx` 来定义：

```nvm
#scene_forest "迷雾森林"
```

场景内部可以用标签（以 `.` 开头）来分段：

```nvm
#scene_forest "迷雾森林"

> 你走进了一片浓雾。

.look_around
> 你环顾四周，什么也看不清。

.call_help
> 你大喊了一声，只有回声回应你。
```

标签的作用是：

- 让选择可以跳到场景内的特定位置
- 把一长段剧情切成更易管理的片段

---

## 2. 普通跳转：`->`

用 `->` 可以跳到另一个位置：

```nvm
-> scene_tower
```

这表示"直接跳到 `scene_tower` 场景，不再回来"。

这种跳转适合：

- 章节切换
- 线性剧情推进
- 进入一个新区域后不再回头

---

## 3. 调用并返回：`@call` / `@return`

但有时候你希望的是：

> 执行一段剧情，然后回到原来的地方继续。

这就是 `@call` 和 `@return` 的用途。

### 基本用法

```nvm
#scene_main "主流程"

> 你走进了一间小屋。

? 你要做什么？
- [和店主交谈] -> .talk
- [打开商店] -> .shop
- [离开] -> scene_village

.talk
店主: 有什么可以帮你的？
-> .back

.shop
@call shop_scene
.back
> 你离开了商店。

#scene_village "村庄"
> 你回到了村庄广场。
```

### 发生了什么

当玩家选择"打开商店"时：

1. 执行到 `@call shop_scene`
2. 跳转到 `shop_scene`，开始执行那里的内容
3. 当 `shop_scene` 执行到 `@return` 时
4. 回到 `@call` 的下一行，也就是 `.back` 标签处

---

## 4. 被调用场景怎么写

一个可以被 `@call` 调用的场景，通常长这样：

```nvm
#shop_scene "商店"

> 店主微笑着向你展示商品。

? 你要买什么？
- [治疗药水 - 20 金币] -> .buy_potion if item_count("gold") >= 20
- [离开] -> .leave

.buy_potion
@take gold 20
@give healing_potion 1
店主: 好的，这是你的药水。
-> .continue

.leave
店主: 欢迎下次光临。
-> .continue

.continue
@return
```

### 关键点

1. 场景最后用 `@return` 结束
2. 所有分支最终都会流向 `@return`
3. 这样调用方就能正确回到原来的位置

---

## 5. @call 和普通跳转的区别

| 方式 | 行为 | 适用场景 |
|------|------|----------|
| `-> scene_xxx` | 跳过去，不回来 | 章节切换、进入新区域 |
| `@call scene_xxx` | 跳过去，执行完回来 | 商店、战斗、可复用片段 |

你可以把它理解成：

- `->` 是"去那边，待在那边"
- `@call` 是"去那边办完事，再回来"

---

## 6. 什么时候该用 @call

### 适合用 @call 的场景

**商店 / 交易系统**

```nvm
@call shop_scene
```

无论在哪个城镇、哪个章节，商店逻辑都是一样的。

**战斗子流程**

```nvm
@call battle_wolves
```

战斗结束后回到原来的剧情继续。

**通用事件**

```nvm
@call random_encounter
```

随机遭遇事件，处理完回到主线。

**休息 / 恢复点**

```nvm
@call inn_rest
```

在旅馆休息，恢复生命值，然后继续冒险。

### 不适合用 @call 的场景

**主线章节切换**

```nvm
-> scene_chapter_2
```

这是永久性的场景切换，不需要回来。

**进入新地图**

```nvm
-> scene_dungeon
```

玩家会在新地图探索很久，不是"办完事回来"。

---

## 7. 一个完整示例

```nvm
---
title: 示例游戏
---

@var gold = 100

@item healing_potion
  name: "治疗药水"
  default_value: 0
@end

#scene_start "开始"

> 你站在村庄广场上。

? 你要去哪里？
- [去商店] -> .go_shop
- [去旅馆休息] -> .go_inn
- [离开村庄] -> scene_forest

.go_shop
@call shop_scene
-> .back

.go_inn
@call inn_scene
-> .back

.back
> 你回到了广场。
-> scene_start

#shop_scene "商店"

店主: 欢迎光临！

? 你要买什么？
- [治疗药水 - 30 金币] -> .buy if item_count("gold") >= 30
- [离开] -> .leave

.buy
@take gold 30
@give healing_potion 1
店主: 这是你的药水。
-> .done

.leave
店主: 欢迎下次光临。

.done
@return

#inn_scene "旅馆"

> 你走进了一家温馨的旅馆。

旅馆老板: 一晚 10 金币，要住吗？

? 
- [住宿] -> .stay if item_count("gold") >= 10
- [离开] -> .leave

.stay
@take gold 10
> 你美美地睡了一觉，精神焕发。
-> .done

.leave
> 你决定不住宿。

.done
@return

#scene_forest "森林"

> 你离开了村庄，走向未知的森林。
```

这个示例展示了：

- 主场景用 `@call` 调用商店和旅馆
- 商店和旅馆场景用 `@return` 返回
- 所有分支都最终到达 `@return`

---

## 8. 嵌套调用

`@call` 可以嵌套，也就是说：

```nvm
#scene_a
@call scene_b

#scene_b
@call scene_c

#scene_c
@return  -- 回到 scene_b
@return  -- 回到 scene_a（需要 scene_b 里也有 @return）
```

不过建议不要嵌套太深，保持代码可读性。

---

## 9. 小结

记住这几条就够了：

1. `#scene_xxx` 定义场景
2. `.label` 定义场景内标签
3. `-> target` 永久跳转，不回来
4. `@call scene` 调用场景，执行完回来
5. `@return` 从被调用场景返回

用好这些，你的剧本就能：

- 避免重复代码
- 保持结构清晰
- 方便维护和扩展

---

## 下一步该学什么

现在你已经知道怎么组织场景和复用剧情片段了。

下一步最自然的问题是：

- 怎么标记玩家的关键选择？
- 怎么触发不同的结局？

下一页建议看：

- [结局与标记](../guide/endings-and-flags/)
