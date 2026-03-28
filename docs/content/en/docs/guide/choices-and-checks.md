---
title: "Choices, Branches, and Checks"
weight: 4
---

# Choices, Branches, and Checks

The biggest difference between interactive narratives and regular fiction is:

> Players don't just read, they make decisions.

The three most common ways to branch storylines in NovaMark are:

1. `if` — standard conditional branching
2. Choices — player-driven decisions
3. `@check` — explicit success/failure checks

---

## 1. Standard Conditional Branching: `if`

`if` is best for expressing:

- Whether a condition is met
- Whether a segment should appear
- Whether a result should take effect

```nvm
if item_count("money") >= 50
  > You have enough money.
else
  > You can't afford that right now.
endif
```

### How to Think About It

- Condition is true → execute this branch
- Condition is false → execute the other branch

It's like everyday logical judgment.

---

## 2. Player Choices

When you want "the player to decide what happens next," use choices.

```nvm
? What do you do?
- [Knock on the door] -> .knock
- [Push the door open] -> .push
```

### Structure Breakdown

#### `?`

Indicates this is a question.

#### `- [text] -> target`

Represents an option.

When the player selects it, the story jumps to the corresponding target.

---

## 3. Conditional Options

Sometimes not all options should be available at all times.

For example:

- Can't open a door without a key
- Can't purchase with insufficient money

You can add conditions to options:

```nvm
? The tower door is tightly closed. You:
- [Try to push it open] -> .push
- [Use the key] -> .unlock if has_item("key")
```

### What Happens

If the condition isn't met, this option won't be available.

The client can present it as:

- A disabled button
- A grayed-out option
- An unselectable hint

---

## 4. `@check`: Explicit Success/Failure Checks

When you want to express "this isn't just a condition, it's a check," use `@check`.

```nvm
@check roll("2d6") >= 8
@success
  > You passed the check.
@fail
  > The check failed.
@endcheck
```

### Why Not Just Use `if`

Because `@check` emphasizes a narrative semantic:

- This is a check
- It has clear success and failure branches

Compared to regular `if`, it's better suited for:

- Dice rolls
- Contests
- Probability events
- Condition challenges

---

## 5. Formal Syntax for `@check`

```nvm
@check condition_expression
@success
  ...
@fail
  ...
@endcheck
```

Note that the official syntax is:

> `@success / @fail / @endcheck`

Not the older bare keyword style.

---

## 6. What Conditions Can You Use in Checks

### 6.1 Numeric Comparison

```nvm
@check hp > 0
```

### 6.2 Item Conditions

```nvm
@check has_item("magic_stone")
```

### 6.3 Dice

```nvm
@check roll("1d20") + courage >= 15
```

### 6.4 Logical Combinations

```nvm
@check has_item("key") and item_count("money") >= 50
```

---

## 7. When to Use Which

This is the most important practical question.

### Use `if`

When you just want to decide "whether to execute some content" based on state.

### Use Choices

When you want the player to actively decide where to go or what to do next.

### Use `@check`

When you want to express a check with clear success/failure semantics.

---

## 8. A Combined Example

```nvm
? How do you handle this door?
- [Push it open directly] -> .push
- [Use the key] -> .unlock if has_item("key")

.push
@check roll("1d20") + courage >= 15
@success
  > You force the door open.
@fail
  > The door doesn't budge. Your shoulder aches.
  @set hp = hp - 5
@endcheck

.unlock
> You unlock the door with the key.
@take key 1
```

This example uses:

- Player choice
- Conditional option
- `@check`
- State changes

This is the most common interactive structure in NovaMark.

---

## What's Next

At this point, you've mastered NovaMark's basic syntax: scenes, dialogue, state, and branching.

Next, enter Phase 2 and learn how to make your story more expressive:

[Characters and Emotions →]({{< ref "characters-and-emotions" >}})
