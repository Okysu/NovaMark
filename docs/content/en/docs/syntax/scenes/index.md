---
title: "Scenes and Labels"
weight: 2
---

# Scenes and Labels

## Scene Definition

Use `#SceneName` to define a scene:

```nvm
#scene_forest "Dark Forest"

> You arrive at a forest...

#scene_tower "Tower of Stars"

> A tall tower looms before you...
```

Scene names can use Chinese, with the title in quotes.

## Labels

Labels are used for jumps within a scene, starting with `.`:

```nvm
#scene_start "Start"

林晓: What should I do?

.look_around
> You look around.
-> scene_start

.wait
> You decide to wait.
-> scene_start
```

## Jumps

### Scene Jump

```nvm
-> scene_forest
```

### Label Jump

```nvm
-> .look_around
```

## Call and Return

`@call` is used to call a scene and return:

```nvm
@call shop_scene
林晓: Done shopping, back to adventure.
```

Use `@return` to return in `shop_scene`:

```nvm
#shop_scene "Shop"

林晓: Shopkeeper, I want to buy something!
@return
```
