---
title: "State, Variables, and Items"
weight: 3
---

# State, Variables, and Items

When writing interactive narratives, the most important thing is:

> Your story must "remember" what has already happened.

For example:

- Whether the player has obtained a key
- How much money the player has
- What the player's current health is
- Whether the player has met a certain character

These are collectively called: **state**.

---

## Two Types of Common State

NovaMark currently recommends dividing state into two categories:

### 1. `@var` — Internal Logic Variables

Best for:

- Flow control
- Internal flags
- Values not necessarily shown directly to players

For example:

```nvm
@var met_spirit = false
@var chapter_index = 1
@var courage = 10
```

### 2. `@item` — Player-Facing State Items

Best for:

- Traditional inventory items
- Currency
- Public attributes
- State data that GUI wants to display directly

For example:

```nvm
@item money
  name: "Money"
  description: "Currency available to the player."
  default_value: 100
@end
```

Or:

```nvm
@item hp
  name: "Health"
  description: "The character's current health."
  default_value: 100
@end
```

---

## Why Split Into `@var` and `@item`

Because not all state should be exposed directly to players.

### Examples Better Suited for `@var`

```nvm
@var met_spirit = false
@var hidden_route_open = false
```

These values are more like:

- Internal story switches
- System memories
- Branch conditions

### Examples Better Suited for `@item`

```nvm
@item money
  name: "Money"
  default_value: 20
@end

@item sanity
  name: "Sanity"
  default_value: 100
@end
```

These values are more like:

- Resources visible to players
- Entries the HUD wants to display
- Data the GUI can use for status panels

---

## How to Define Variables

```nvm
@var hp = 100
@var gold = 50
@var player_name = "沈砚"
@var met_spirit = false
```

This means:

- `hp` is a number
- `player_name` is a string
- `met_spirit` is a boolean

---

## How to Modify Variables

```nvm
@set hp = hp - 10
@set gold = gold + 20
@set met_spirit = true
```

`@set` supports expressions, so you don't always need to create temporary variables.

---

## How to Give and Take Items/State Items

```nvm
@give money 10
@take money 2
```

Now `@give` and `@take` also support expressions:

```nvm
@take money 1 + 1
@give money random(1, 10)
@take money count + 1
```

This means creators can write more naturally:

- Fixed values
- Variables
- Function results
- Arithmetic expressions

Without always needing a temporary variable.

---

## How to Check State

NovaMark's most commonly used state functions include:

```nvm
has_item("magic_stone")
item_count("money")
has_flag("met_spirit")
has_ending("true_end")
```

These typically appear in:

- `if` statements
- `@check` blocks
- Conditional choices

For example:

```nvm
if item_count("money") >= 50
  > You have enough money.
endif
```

---

## Why Runtime State Matters for Clients

In NovaMark's design, scripts don't directly manage HUDs.

So the recommended approach is:

- Scripts maintain state
- Clients read state and decide how to display it

For example, a client can use runtime state to:

- Display `money` in a status bar
- Show `hp` as a health bar
- Render `sanity` as a number or icon

This is why `@item` is now suitable for "player-facing state items."

---

## What's Next

Now you know how to store and modify state.

The natural next questions are:

- How do you branch into different storylines based on state?
- How do you write a clear success/failure check?

Next page:

- [Choices, Branches, and Checks](../choices-and-checks/)
