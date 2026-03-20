---
title: "Choices and Branches"
weight: 4
---

# Choices and Branches

## Player Choices (?)

Let players make choices that affect the story direction.

### Basic Syntax

```nvm
? Question text
- [Option text] -> target_scene
- [Option text] -> .label
```

### Example

```nvm
? What do you do?
- [Look around] -> .look_around
- [Call for help] -> .call_for_help
- [Wait here] -> .wait_here

.look_around
> You look around...
-> next_scene

.call_for_help
> No one responds...
-> next_scene

.wait_here
> You decide to wait...
-> next_scene
```

### Conditional Choices

Some options can be shown or disabled based on conditions:

```nvm
? How do you proceed?
- [Continue forward] -> .continue
- [Use key] -> .use_key
- [Use magic stone] if has_item("magic_stone") -> .use_stone
```

**Condition syntax**: `if condition_expression`

### Choice Option Details

| Part | Description |
|------|-------------|
| `-` | Option start marker |
| `[text]` | Option text shown to player |
| `if condition` | Optional condition check |
| `-> target` | Target to jump to after selection |

## Condition Check (@check)

Execute different branches based on conditions.

### Basic Syntax

```nvm
@check condition_expression
@success
  // Execute when condition is true
@fail
  // Execute when condition is false
@endcheck
```

### Example

```nvm
@check hp >= 50
@success
  林晓: I can keep going!
@fail
  林晓: I need to rest...
  @set hp = 50
@endcheck
```

### Dice Check

```nvm
@check roll("2d6") >= 8
@success
  > You passed the check!
  @set courage = courage + 5
@fail
  > Check failed...
  @set hp = hp - 10
@endcheck
```

### Item Check

```nvm
@check has_item("key")
@success
  > You unlocked the door with the key.
  @take key 1
  -> .inside
@fail
  > The door is locked, you don't have a key.
@endcheck
```

### Flag Check

```nvm
@check has_flag("met_spirit")
@success
  神秘精灵: We meet again, young traveler.
@fail
  > This forest seems to hide some secret...
@endcheck
```

## Branch Statements (if/endif)

Execute code blocks based on conditions.

### Basic Syntax

```nvm
if condition_expression
  // Execute when condition is true
endif

if condition_expression
  // Execute when condition is true
else
  // Execute when condition is false
endif
```

### Example

```nvm
if has_item("treasure_map")
  > You take out the map to check the location.
else
  > You don't have a map, you can only follow your instincts.
endif

if gold >= 100
  @set gold = gold - 100
  @give legendary_sword 1
  > You bought the legendary sword!
endif
```

## Condition Expressions

### Comparison Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `==` | Equal | `hp == 100` |
| `!=` | Not equal | `hp != 0` |
| `>` | Greater than | `gold > 50` |
| `>=` | Greater than or equal | `hp >= 50` |
| `<` | Less than | `hp < 20` |
| `<=` | Less than or equal | `gold <= 10` |

### Logical Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `and` | And | `has_item("key") and hp > 0` |
| `or` | Or | `has_flag("saved") or gold >= 100` |
| `not` | Not | `not has_flag("boss_defeated")` |

### Arithmetic Operations

You can use arithmetic expressions in conditions:

```nvm
@check str + roll("1d20") >= 15
success
  > You successfully persuaded the guard!
fail
  > The guard is unmoved...
endcheck
```

Supported operations: `+`, `-`, `*`, `/`, `%`
