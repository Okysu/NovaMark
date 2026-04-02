---
title: "What is NovaMark"
weight: 1
---

# What is NovaMark

NovaMark is a scripting language and runtime for **text adventures, visual novels, and interactive narrative games**.

At a high level, it helps creators describe:

- who is speaking
- what the player can choose
- how state changes
- what runtime data the host UI should receive

NovaMark is designed around a simple split:

- the script describes content and state
- the host application controls UI, timing, animation, and save UX

---

## What problem does it solve?

If you write story logic in general-purpose code, the usual problems are:

- dialogue and program logic get mixed together
- writers cannot easily read the story structure
- a single story is hard to reuse across Web, Native, and CLI

NovaMark separates these responsibilities:

### The script owns

- content
- branches
- state changes
- asset references

### The host owns

- UI layout
- animation
- typewriter effects
- save file management
- buttons and HUD

That means the same story can:

- run as a Web chat UI
- become a Native visual novel UI
- run in CLI text mode

without rewriting the script itself.

---

## The 4 core concepts

If this is your first time, focus on these four ideas.

### 1. Scene

A scene is the basic unit of story structure. Think of it as:

- a chapter
- a room
- a story fragment

### 2. Dialogue and narration

Most of what you write is either:

- narration
- character dialogue

### 3. Choices and branches

Interactive narrative relies on:

- what the player can choose
- where those choices lead

### 4. State

State includes:

- variables
- items
- flags
- endings

State decides how the script continues.

---

## What NovaMark is NOT

To avoid confusion, NovaMark **does not** try to be:

- a continuous timeline animation engine
- a UI framework
- a HUD layout system
- an automatic save policy system

NovaMark outputs **discrete state**, not a continuous time flow.

The host application decides:

- when to advance
- how to display the current state
- how to play animations and transitions

---

## Suggested reading order

If this is your first time with NovaMark, follow this path:

1. [Your First Story]({{< ref "your-first-story" >}})
2. [State, Variables, and Items]({{< ref "state-and-variables" >}})
3. [Choices, Branches, and Checks]({{< ref "choices-and-checks" >}})
4. Then use [Syntax Reference]({{< ref "/docs/syntax" >}}) or [Quick Reference]({{< ref "/docs/reference" >}}) for details

This way you'll build intuition first, then learn the specifics.

---

## What's Next

Now that you understand the basic concepts of NovaMark, it's time to get your hands dirty:

Next page:

- [Your First Story]({{< ref "your-first-story" >}})
