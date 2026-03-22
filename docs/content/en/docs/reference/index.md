---
title: "Syntax/Command Cheat Sheet"
weight: 1
---

# Syntax/Command Cheat Sheet

A one-page overview of NovaMark core syntax. For detailed explanations, see [Command Reference](../syntax/commands/).

---

## File Structure

```nvm
---
title: Game Title
version: 1.0
---

@var variable_name = initial_value

#scene_start "Scene Title"
```

---

## Dialogue

```nvm
CharacterName: Dialogue text
> Narrator text
```

---

## Variables

```nvm
@var hp = 100              // define
@var name = "Alice"
@var alive = true

@set hp = 80               // modify
@set gold = gold + 10      // arithmetic
```

**Supported operators**: `+`, `-`, `*`, `/`, `%`

---

## Items

```nvm
// define
@item healing_potion
  name: Healing Potion
  description: Restore 50 HP
  icon: potion.png
@end

// operations
@give healing_potion 2     // acquire
@take healing_potion 1     // remove
```

---

## Choices

```nvm
? Prompt text
- [Option1] -> target
- [Option2] -> .label

.label
> Content for option 2
```

---

## Checks

```nvm
@check roll("2d6") >= 8
@success
  > Success!
@fail
  > Failure...
@endcheck

@check has_item("key")
@check hp >= 50
@check has_flag("met_spirit")
```

**Built-in functions**:

| Function | Purpose |
|----------|---------|
| `roll("XdY")` | Dice roll check |
| `has_item("id")` | Check item possession |
| `item_count("id")` | Get item count |
| `has_flag("id")` | Check flag status |
| `has_ending("id")` | Check ending unlocked |

---

## Scenes

```nvm
#scene_forest "Forest"      // define

-> scene_forest             // jump
-> .label                   // jump to label

.label                      // label definition
> Label content

@call shop_scene            // call subroutine
@return                     // return from subroutine
```

---

## Media

```nvm
@bg image.png               // background
@bg image.png transition:fade

@sprite char_id show url:img.png position:left
@sprite char_id hide

@bgm music.mp3              // BGM
@bgm stop

@sfx sound.mp3              // sound effect
```

---

## Character Definition

```nvm
@char Alice
  color: #4A90D9
  description: The protagonist
  sprite_default: alice.png
  sprite_happy: alice_happy.png
@end
```

---

## Endings & Flags

```nvm
@flag met_spirit            // set flag (persists across playthroughs)
@ending good_ending         // trigger ending
```

---

## Conditionals

```nvm
@if hp <= 0
  > You collapsed...
  -> game_over
@else
  > You're still alive.
@endif
```

---

## Runtime State

JSON structure read by renderers (see [Runtime State](../api/runtime-state/) for details):

```json
{
  "status": 0,
  "currentScene": "scene_start",
  "variables": {
    "numbers": { "hp": 100 },
    "strings": { "playerName": "Alice" },
    "bools": { "metSpirit": true }
  },
  "inventory": { "healing_potion": 2 },
  "inventoryItems": [...],
  "characterDefinitions": {...}
}
```

---

## More Reference

- [Complete Command Reference](../syntax/commands/)
- [Variables](../syntax/variables/)
- [Scenes & Labels](../syntax/scenes/)
- [Choices & Branches](../syntax/branches/)
- [Project Configuration](../syntax/project-config/)
