---
title: "场景和标签"
weight: 2
---

# 场景和标签

## 场景定义

使用 `#场景名` 定义场景：

```nvm
#scene_forest "幽暗的森林"

> 你来到了一片森林...

#scene_tower "星光之塔"

> 高塔耸立在眼前...
```

场景名可以使用中文，引号内为场景标题。

## 标签

标签用于场景内的跳转，以 `.` 开头：

```nvm
#scene_start "开始"

林晓: 我该怎么做？

.look_around
> 你环顾四周。
-> scene_start

.wait
> 你决定等待。
-> scene_start
```

## 跳转

### 场景跳转

```nvm
-> scene_forest
```

### 标签跳转

```nvm
-> .look_around
```

## 调用和返回

`@call` 用于调用场景并返回：

```nvm
@call shop_scene
林晓: 买完了，继续冒险。
```

在 `shop_scene` 中使用 `@return` 返回：

```nvm
#shop_scene "商店"

林晓: 老板，买东西！
@return
```
