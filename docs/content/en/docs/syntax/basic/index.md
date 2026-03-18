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
