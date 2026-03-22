---
title: "Glossary"
weight: 3
bookCollapseSection: true
---

# Glossary

Glossary explains recurring NovaMark terms in plain language.

---

## VM (Virtual Machine)

The NovaMark VM is the core runtime that executes scripts, advances the story, and updates state.

It does not draw UI directly, and it does not decide animations, layouts, HUD behavior, or save UI. It only advances the story to the next discrete state point.

Related docs:

- [Runtime, Host, and Platform Integration](../platform/runtime-and-host/)
- [API Reference](../api/)

## Runtime State (NovaState)

Runtime state is the snapshot of data that the host actually consumes. It usually includes the current scene, variables, inventory, character definitions, dialogue, choices, background, BGM, sprites, and text configuration.

You can think of it as:

> what the engine tells the renderer to show at a given moment.

Related docs:

- [Runtime State](../api/runtime-state/)
- [Runtime, Host, and Platform Integration](../platform/runtime-and-host/)

## Host

The host is the outer application that embeds the NovaMark VM. It can be a CLI app, a Web frontend, a desktop client, a mobile app, or any program that can call the runtime interfaces.

The host is responsible for:

- receiving player input
- calling `advance()` / `choose()`
- reading runtime state
- deciding UI, animation, audio, and save behavior

## Renderer

The renderer is the layer that turns runtime state into an actual interface. NovaMark renderers are intentionally “dumb”: they do not own story logic, they only present state.

That means the same `.nvmp` package can be used by multiple renderers, such as:

- Text Mode
- Web Chat / Web VN
- Native GUI

## `.nvmp` Package

`.nvmp` is NovaMark's single-file distribution format. It packages scripts, compiled content structures, and asset data into one distributable file.

That means:

- you do not need to manage a separate asset tree for distribution
- integrators only need to load one package file
- Web / Native / CLI can consume the same game package

## Scene

A scene is the main structural unit of a story. You can think of it as a chapter fragment or a narrative node.

It is usually defined with `#scene_xxx "Title"`.

## Label

A label is a local jump point inside a scene, usually written as `.label_name`.

It is commonly used to:

- jump to a position inside the current scene
- split a long scene into smaller fragments
- organize branch-heavy content

## Dialogue

Dialogue is text spoken by a character, usually written as:

```nvm
CharacterName: Dialogue text
```

From the host's perspective, dialogue usually appears in the current state's dialogue field, then the renderer decides how to present it.

## Narration

Narration is descriptive text without a speaker name, usually written as:

```nvm
> Moonlight falls through the trees.
```

It is commonly used for environmental description, action description, and system prompts.

## Choice

A choice is an interaction node where the player decides what happens next. It usually consists of a prompt line and several options.

```nvm
? What do you do?
- [Explore the area] -> .explore
- [Wait here] -> .wait
```

## Check

A check is an explicit success / failure evaluation, usually written with `@check`, `@success`, `@fail`, and `@endcheck`.

It is well suited for:

- dice expressions
- stat comparisons
- resource conditions

## Variable

A variable is an internal script state value, usually defined with `@var` and modified with `@set`.

It is suitable for storing:

- numbers
- strings
- booleans
- internal story switches

## Item / Inventory

An item is a player-facing state entry that is often displayed in UI. Common commands include `@item`, `@give`, and `@take`.

It can represent traditional items, but also currency, HP, sanity, or any other visible resource.

## Flag

A flag records important story milestones or long-lived state. It is usually set with `@flag` and checked with `has_flag()`.

It is useful for storing:

- whether the player met a character
- whether a hidden route is unlocked
- whether a key event has happened

## Ending

An ending represents an official story completion state. It is usually triggered with `@ending` and checked with `has_ending()`.

Unlike an ordinary branch, an ending is usually something the platform records as an unlocked result for review, achievements, or multi-run progression.

## Snapshot

A snapshot is a saved result of runtime state at a given moment. NovaMark currently distinguishes between:

- formal binary save files
- JSON snapshots for debugging and developer tools

## Rendering Hint

Rendering hints are presentation suggestions written in scripts, such as:

- `transition`
- `position`
- `opacity`
- `loop`
- `volume`

These fields express creator intent, but they do not mean the engine contains a full timeline system. How they are actually played, animated, or faded depends on the host and renderer.

## `advance()` / `choose()`

These are the two most important host actions:

- `advance()` moves the story forward until the next host-visible state point
- `choose()` tells the engine which option the player selected and then continues the story

If you are building a platform integration, understanding these two actions matters more than memorizing every piece of syntax.
