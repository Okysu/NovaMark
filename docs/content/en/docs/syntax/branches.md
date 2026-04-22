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

In addition to single-line options, NovaMark now supports **block-style options** for flows such as “update state first, then jump”.

```nvm
? 1. Feeling uneasy, worried, or irritable
- [Never]
  @set score = score + 0
  -> .q2
- [Sometimes]
  @set score = score + 1
  @flag answered_q1
  -> .q2
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
- [Use magic stone] -> .use_stone if has_item("magic_stone")
```

**Condition syntax**: `if condition_expression`

Block-style options also support a condition in the option header:

```nvm
? How do you proceed?
- [Use key] if has_item("key")
  @flag used_key
  -> .open_door
- [Leave]
  -> .leave
```

### Block-Style Options

Use a block-style option when you want to execute a small whitelist of actions after selection, then jump.

```nvm
? Stress test
- [Never]
  @set score = score + 0
  -> .q2
- [Constantly]
  @set score = score + 3
  @flag high_stress
  -> .q2
```

The execution order is fixed:

1. The player selects the option
2. The actions in the block run in order
3. The final `-> target` runs

### Block-Style Option Restrictions

The first version of block-style options has these constraints:

- The last statement in the option block must be one of:
  - `-> target` (jump to a label or scene)
  - `@call scene` (invoke a sub-scene)
- Before the terminal statement, only the following prelude commands are allowed:
  - `@set`
  - `@flag`
  - `@give`
  - `@take`
- `@return` is not allowed inside a choice body
- Nothing may follow `@call` within the same choice block
- You cannot use `@bg`, `@check`, nested choices, or other statements inside the block
- Single-line options and block-style options cannot be mixed together

### Valid Examples

Ending with `-> target`:

```nvm
- [Use potion]
  @give potion 1
  -> .next
```

Ending with `@call scene`:

```nvm
- [Visit the shop]
  @set visited_shop = true
  @call shop_scene
```

### Invalid Examples

```nvm
- [Sometimes]
  @set score = score + 1
```

This is invalid because it is missing a terminal statement.

```nvm
- [Sometimes] -> .q2
  @set score = score + 1
```

This is also invalid because a single-line option cannot be followed by an indented block.

```nvm
- [Call and continue]
  @call shop_scene
  @set shop_visited = true
```

No statements are allowed after `@call` in the same choice body.

### Choice Option Details

| Part | Description |
|------|-------------|
| `-` | Option start marker |
| `[text]` | Option text shown to player |
| `-> target` | Target to jump to after selection |
| `if condition` | Optional condition check. In single-line syntax it comes after the target; in block-style syntax it appears in the option header |
| indented block | Optional block body that allows `@set/@flag/@give/@take` before a final jump |

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
@success
  > You successfully persuaded the guard!
@fail
  > The guard is unmoved...
@endcheck
```

Supported operations: `+`, `-`, `*`, `/`, `%`

## Complete Example

```nvm
#scene_combat "Combat"

@check hp > 0
@success
  ? The enemy approaches. You choose:
  - [Attack] -> .attack
  - [Defend] -> .defend
  - [Use potion] if has_item("healing_potion") -> .use_potion
  - [Flee] -> .flee
  
  .attack
  @check roll("1d20") >= 10
  @success
    > Your attack hits!
    @set enemy_hp = enemy_hp - 15
  @fail
    > You missed!
    @set hp = hp - 10
  @endcheck
  -> .check_result
  
  .defend
  > You raise your shield.
  @set defense_bonus = 5
  -> .enemy_turn
  
  .use_potion
  @take healing_potion 1
  @set hp = hp + 50
  > You recover 50 HP!
  -> .enemy_turn
  
  .flee
  @check roll("1d20") >= 12
  @success
    > You escaped!
    -> scene_escape
  @fail
    > Escape failed!
    @set hp = hp - 20
  @endcheck
  -> .check_result
  
  .check_result
  @check enemy_hp <= 0
  @success
    @flag defeated_enemy
    > You defeated the enemy!
    -> scene_victory
  @fail
    @check hp > 0
    @success
      -> .enemy_turn
    @fail
      > You were defeated...
      @ending game_over
    @endcheck
  @endcheck
  
@fail
  > You have already fallen...
  @ending game_over
@endcheck
```
