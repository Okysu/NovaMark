---
title: "Your First Story"
weight: 2
---

# Your First Story

The goal of this page isn't to teach you every bit of syntax. It's to help you write **your first runnable story**.

If this is your first time with NovaMark, just focus on understanding:

- How to write a scene
- How to write narration
- How to write dialogue
- How to write choices

---

## A Minimal Example

```nvm
#scene_intro "Beginning"

> The rain has just stopped. The streets still carry a damp scent.

沈砚: Something different might happen today.

? What do you notice first?
- [The old bookstore by the street] -> .bookstore
- [The flickering lamp at the alley entrance] -> .lamp

.bookstore
> A faint warm glow seeps through the glass door.

.lamp
> The light flickers, as if trying to tell you something.
```

This script already contains the 4 most important elements in NovaMark:

1. A scene
2. A narration line
3. A dialogue line
4. A choice

---

## Understanding It Line by Line

### `#scene_intro "Beginning"`

This means:

- Create a new scene
- The scene ID is `scene_intro`
- The title is `Beginning`

You can think of a scene as "a container for a segment of your story."

### `> The rain has just stopped. The streets still carry a damp scent.`

This is narration.

Narration is typically used for:

- Environment descriptions
- Atmosphere
- Action descriptions
- System messages

### `沈砚: Something different might happen today.`

This is character dialogue.

The format is simple:

```nvm
CharacterName: Content
```

### `? What do you notice first?`

This is a choice question.

It's usually followed by multiple options.

### `- [The old bookstore by the street] -> .bookstore`

This means:

- The option text is "The old bookstore by the street"
- When the player selects it, jump to `.bookstore`

---

## Why Write Labels as `.bookstore`

`.bookstore` is a label.

Labels serve two purposes:

- Divide a scene into smaller fragments
- Let choices jump to specific positions within the scene

You can think of it as:

> "A small node within the current scene"

---

## What to Remember For Now

Just remember these 4 rules:

1. `#scene_xxx` starts a scene
2. `>` writes narration
3. `CharacterName:` writes dialogue
4. `?` and `- [option] -> target` write interactive choices

With just these, you can already write a basic interactive fiction prototype.

---

## What's Next

Now that you have your first story, the natural next questions are:

- How do you remember what the player has obtained?
- How do you make certain choices appear only when conditions are met?

Next page:

- [State, Variables, and Items](./state-and-variables/)
