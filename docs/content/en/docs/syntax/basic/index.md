---
title: "Basic Syntax"
weight: 1
---

# Basic Syntax

## Comments

Use `//` for single-line comments:

```nvm
// This is a comment, will not be executed
林晓: Hello! // End of line comment
```

## Narrative Text

Text starting with `>` is narrator narration:

```nvm
> Moonlight filters through the trees, all is silent.
> You don't remember how you got here.
```

## Dialogue

Format: `CharacterName: Dialogue content`

```nvm
林晓: Where... am I?
神秘精灵: Welcome to the magic forest.
```

Dialogue with emotion:

```nvm
林晓 (surprised): How is this possible!
```

## Text Interpolation `{{ }}`

NovaMark supports embedding expression results inside **player-facing text**. The currently supported locations are:

- Narration
- Dialogue
- Choice option text

Basic usage:

```nvm
@var hp = 100
@var mp = 35

> Current HP: {{hp}}
林晓: I still have {{mp}} mana left.
? What will you do?
- [Cast a spell for {{mp}} mana] -> .cast
```

### Supported expressions

`{{ }}` is not limited to variable names. It can contain any valid expression, for example:

```nvm
@var hp = 100
@var cost = 20

> HP after damage: {{hp - 10}}
> Has the key: {{has_flag("got_key")}}
- [Pay {{cost}} gold] -> .pay
```

Common cases:

- `{{hp}}`: variable lookup
- `{{hp - 10}}`: arithmetic expression
- `{{has_flag("got_key")}}`: function call

### Scope and limitations

`{{ }}` currently works only in **display text**. It is not used for:

- Command arguments themselves
- Conditions in `@check` / `if`
- Structural syntax such as jump targets, character names, or asset paths

In other words, its job is to display runtime values inside text, not to replace the rest of the scripting syntax.

### Failure behavior

If `{{ }}` is not properly closed, or the expression inside cannot be parsed, NovaMark keeps the original text instead of aborting the whole text line.

## Front Matter

YAML format metadata can be used at the beginning of the file:

```nvm
---
title: Game Title
author: Author Name
version: 1.0
description: Game Description
---
```

## Identifiers

NovaMark fully supports Chinese identifiers:

```nvm
@var health = 100
@var gold = 50

林晓: I have {{gold}} gold coins.
```
