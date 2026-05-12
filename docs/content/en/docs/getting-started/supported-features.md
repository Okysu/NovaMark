---
title: "Supported Features"
weight: 3
---

# Supported Features

This document lists what NovaMark **officially supports today**.

Its purpose is to make three things clear:

- what is officially supported right now
- what is the recommended way to use it
- what belongs to the host application instead of the script engine

---

## 1. Officially supported script syntax

### 1.1 File structure

- Front Matter (`---`)
- Scene definition `#scene_xxx "Title"`
- Label `.label`

### 1.2 Text and dialogue

- Narration: `> text`
- Dialogue: `Character: text`
- Emotion dialogue: `Character[emotion]: text`
- Text interpolation: `{{expr}}`
- Inline style: `{style:text}`

### 1.3 Definition blocks

- `@var`
- `@char`
- `@item`

`@item` is currently suitable for **player-visible, exposable state entries or item-like entries**.

Examples include:

- traditional items: keys, potions, coins
- exposed stats: hp, money, sanity

Supported fields include:

- `name`
- `description`
- `icon`
- `default_value`

### 1.4 Flow control

- `-> scene`
- `-> .label`
- `@call`
- `@return`
- `if / else / endif`
- `@check / @success / @fail / @endcheck`

### 1.5 Custom directives (v1.0 Registry Override System)

When the parser encounters an unknown `@xxx` directive, it produces a `CustomCommandNode`. The VM dispatches to a host-registered handler via the Registry.

```nvm
@custom_skill_check difficulty:hard target:boss
```

Arguments support `key:value` pairs and standalone identifiers/literals.

### 1.6 State changes

- `@set`
- `@give`
- `@take`
- `@flag`
- `@ending`（supports optional title `@ending id "Display Title"`）

`@set / @give / @take` all support expressions, not just literals.

For example:

```nvm
@take money 1 + 1
@give money random(1, 10)
@set hp = hp - item_count("wound")
```

### 1.7 Media references

- `@bg`
- `@sprite`
- `@bgm`
- `@sfx`

### 1.8 Choices

- `? question`
- `- [choice] -> target`
- `- [choice] -> target if condition`

---

## 2. Officially supported built-in functions

- `has_item(...)`
- `item_count(...)`
- `has_flag(...)`
- `has_ending(...)`
- `var(...)`
- `roll(...)`
- `random(min, max)`
- `chance(probability)`

### v1.0: Custom functions

Through the Registry Override System, hosts can register custom functions callable from scripts. See [Registry Override System]({{< ref "/docs/api/registry" >}}).

### Parameter rules

#### State query helpers

These functions accept:

- string literals
- identifiers

```nvm
has_item("magic_stone")
has_flag("met_spirit")
item_count(gold_coin)
```

#### Dice rolls

`roll()` expects a string dice expression:

```nvm
roll("2d6")
roll("1d20+3")
```

#### Random integer

`random()` accepts numeric expressions:

```nvm
random(1, hp)
```

#### Probability

`chance()` accepts a 0~1 numeric expression:

```nvm
chance(0.25)
```

---

## 3. Officially supported runtime capabilities

### 3.1 Discrete progression model

NovaMark officially uses:

- `advance()`
- `choose(choiceId)`

The host does not need to understand any internal “consume dialogue” mechanics. It only needs to express:

- continue
- choose an option

### 3.2 Runtime state export

Officially supported exported state includes:

- current scene / label
- variables
- inventory
- itemDefinitions
- characterDefinitions
- inventoryItems
- dialogue
- choices
- bg / bgm / sprites
- textConfig

### 3.3 Snapshots and save/load (v1.0 stateVersion compatibility)

NovaMark officially supports:

- Runtime snapshot capture (`captureState`)
- Snapshot restore (`loadSave`)
- File save/load (JSON + binary)
- Playthrough-only import (`loadPlaythroughOnly`)

### Save format rule

- **official save files**: binary (v6)
- **JSON**: debugging / tests / Web/WASM tooling
- **stateVersion**: v1/v2/v3 saves auto-migrate upward on load

### 3.4 Registry Override System (v1.0)

Hosts can extend and override NovaMark capabilities at runtime:

- **Custom functions**: `registerFunction()` — host functions callable from scripts
- **Custom directives**: `registerDirective()` — scripts use `@custom_directive` syntax
- **State field extensions**: `registerStateField()` — inject custom fields into `NovaState.extensions`
- **Built-in function override**: `registerFunction("random", handler, true)` — replace built-in with custom implementation

See [Registry Override System API]({{< ref "/docs/api/registry" >}}).

### 3.5 Ending titles and flag exposure (v1.0)

- `@ending id "Title"` — endings support optional display titles, exposed as `EndingState` in NovaState
- `NovaState.flags` — currently triggered flags list, queryable by renderers
- `NovaState.ending.title` / `NovaState.ending.reached` — full ending information

---

## 4. Fields kept as rendering hints

These fields are supported, but they are **not** an internal animation timeline system:

- `transition`
- `position`
- `opacity`
- `loop`
- `volume`

They represent:

- rendering intent from the script
- state hints the host can consume

They do **not** mean:

- the engine runs continuous animations automatically
- the engine owns media timing

---

## 5. Host responsibility boundary

NovaMark currently follows these principles:

1. The engine outputs discrete states and does not own continuous time
2. The host controls progression, saving, timing, and animation
3. Syntax should not imply capabilities the engine does not really have
4. Public APIs should express host intent, not internal consumption details
5. One concern should have one primary semantic path, not many near-duplicate APIs

### Script side

- story content
- branching logic
- state changes
- media references
- character/item definitions

### Host side

- UI layout
- HUD
- typewriter presentation details
- animation / transition timing
- save timing
- file management

---

## 6. Recommended usage

### For script authors

- use `if` for general conditional branches
- use `@check` for explicit success/fail checks
- use `@char` / `@item` for centralized definitions
- treat UI as host logic, not script logic

### For host developers

- use `advance()` to progress the story
- use `choose(choiceId)` to select options
- use runtime state to drive UI rendering
- use binary snapshots for official save files
- treat JSON interfaces as debugging tools only
