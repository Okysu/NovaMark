---
title: "Command Reference"
weight: 4
---

# Command Reference

This document lists all commands supported by NovaMark.

## Comments

NovaMark supports `//` single-line comments:

```nvm
// This is a full-line comment
林晓: Hello! // This is an end-of-line comment
```

Comments are not executed and are for code documentation only.

---

## Definition Commands

### @char - Character Definition

Define a character in the game.

```nvm
@char CharacterName
  color: #RRGGBB
  description: Character description
  sprite_default: default_sprite.png
  sprite_happy: happy_sprite.png
@end
```

**Properties**:
| Property | Type | Description |
|----------|------|-------------|
| `color` | Color | Character name color in dialogue box |
| `description` | String | Character description |
| `sprite_default` | Filename | Default sprite file |
| `sprite_*` | Filename | Emotion sprite convention such as `sprite_happy` |

---

### @item - Item Definition

Define an item in the game.

```nvm
@item item_id
  name: Display Name
  description: Item description
  icon: icon_file.png
@end
```

**Properties**:
| Property | Type | Description |
|----------|------|-------------|
| `name` | String | Item display name |
| `description` | String | Item description |
| `icon` | Filename | Item icon file |

---

### @var - Variable Definition

Define a game variable.

```nvm
@var variable_name = initial_value
```

**Supported Types**:
- Number: `@var hp = 100`
- String: `@var name = "Alice"`
- Boolean: `@var is_alive = true`

---

### @theme - Theme Definition

Define UI theme styles.

```nvm
@var theme_name
  dialog_bg: #000000
  text_color: #FFFFFF
@end
```

---

## Scene Control

### #scene - Scene Definition

Define a scene.

```nvm
#scene_id "Scene Title"
```

**Example**:
```nvm
#scene_forest "Dark Forest"
#scene_tower "Tower of Stars"
```

---

### -> - Jump

Jump to a scene or label.

```nvm
-> scene_name      // Jump to scene
-> .label_name     // Jump to label within current scene
```

---

### @call - Call Scene

Call a scene and return after completion.

```nvm
@call shop_scene
```

Use `@return` in the called scene to return.

---

### @return - Return

Return from a `@call` invocation.

```nvm
@return
```

---

### .label - Label Definition

Define a label within a scene.

```nvm
.look_around
> You look around.
```

Label names start with `.`.

---

## Display Commands

### @bg - Background Image

Display or switch background image.

```nvm
@bg image.png
@bg image.png transition:fade
```

**Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `transition` | String | Transition effect (fade/slide/none) |

---

### @sprite - Sprite/Portrait

Display or switch character sprites.

```nvm
@sprite CharacterName show url:sprite.png position:left
@sprite CharacterName show url:sprite.png x:70% y:100px
@sprite CharacterName hide
```

**Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `show` | - | Show sprite |
| `hide` | - | Hide sprite |
| `url` | Filename | Sprite image |
| `position` | String | Recommended `left` / `right`, passed through as-is |
| `x` | String/Number | Interpreted by each platform |
| `y` | String/Number | Interpreted by each platform |
| `opacity` | Number | Opacity (0-1) |

---

### @bgm - Background Music

Play or stop background music.

```nvm
@bgm music.mp3
@bgm music.mp3 loop:true volume:0.8
@bgm stop
```

**Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `loop` | Boolean | Loop playback |
| `volume` | Number | Volume (0-1) |

---

### @sfx - Sound Effect

Play a sound effect.

```nvm
@sfx click.mp3
@sfx ambient.mp3 loop:true volume:0.5
```

**Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `loop` | Boolean | Loop playback |
| `volume` | Number | Volume (0-1) |

---

## Game Logic

### @set - Modify Variable

Modify variable value.

```nvm
@set hp = 80
@set gold = gold + 10
@set courage = courage - 5
```

Supports: `+`, `-`, `*`, `/`, `%`

---

### @give - Give Item

Add item to player inventory.

```nvm
@give healing_potion 1
@give gold 100
```

---

### @take - Remove Item

Remove item from player inventory.

```nvm
@take healing_potion 1
```

---

### @check - Condition Check

Execute different branches based on condition.

```nvm
@check condition_expression
@success
  // Execute on success
@fail
  // Execute on failure
@endcheck
```

**Examples**:
```nvm
@check roll("2d6") >= 8
@success
  林晓: Success!
@fail
  林晓: Failed...
@endcheck

@check has_item("key")
@success
  > You opened the chest.
@fail
  > No key.
@endcheck

@check hp >= 50
@success
  林晓: I can keep going!
@fail
  林晓: I need to rest...
  @set hp = 50
@endcheck
```

---

### @flag - Set Flag

Set a game flag (for multi-playthrough).

```nvm
@flag met_spirit
```

---

### @ending - Trigger Ending

Trigger a game ending.

```nvm
@ending good_ending
@ending bad_ending
```

---

### @wait - Wait

Pause execution for a duration.

```nvm
@wait 1.5
```

Parameter is wait time in seconds.

---

## Expressions and Functions

### Arithmetic Operations

```nvm
@set total = a + b
@set diff = a - b
@set product = a * b
@set quotient = a / b
```

### Comparison Operations

```nvm
@check hp > 0
@check gold >= 100
@check hp == 100
@check hp != 0
```

### Logical Operations

```nvm
@check has_item("key") and hp > 0
@check not has_flag("boss_defeated")
```

### Built-in Functions

| Function | Description | Example |
|----------|-------------|---------|
| `has_item("id")` | Check if player has item | `has_item("key")` |
| `item_count("id")` | Get item count | `item_count("gold")` |
| `has_ending("id")` | Check if ending was triggered | `has_ending("true_end")` |
| `has_flag("id")` | Check if flag was set | `has_flag("met_spirit")` |
| `roll("XdY+Z")` | Dice check | `roll("2d6+3")` |

### Dice Expressions

```nvm
roll("2d6")      // 2 six-sided dice
roll("1d20")     // 1 twenty-sided die
roll("2d6+3")    // 2d6 + 3 modifier
roll("1d8-1")    // 1d8 - 1 modifier
```
