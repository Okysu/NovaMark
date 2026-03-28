---
title: "Runtime, Host, and Platform Integration"
weight: 1
---

# Runtime, Host, and Platform Integration

This page is not for "how to write stories", but for:

- Game studio technical leads
- Web / Native renderer developers
- Teams integrating NovaMark into their own products

If you care about:

- How the VM actually advances
- Why UI should not be controlled by scripts
- How runtime state is consumed by the frontend
- What scenarios the Web template and C API are suitable for

Then this page is your main entry point.

---

## 1. Understanding the NovaMark Runtime Model

NovaMark is not a continuous-time animation engine. Instead, it is:

> **A discrete state machine + host-controlled rendering and advancement**

This statement is critical.

It means:

- The engine does not continuously play a "timeline" on its own
- The engine does not control the HUD itself
- The engine does not decide when to show save UI
- The engine does not decide how long transition animations last

What the engine does:

1. Advance to the next discrete state according to the script
2. Produce a snapshot of the current state to display
3. Wait for the host to decide what to do next

---

## 2. What is "Discrete State"

NovaMark has only two core host actions:

- `advance()`
- `choose(choiceId)`

### `advance()`

Means:

> Continue advancing until the next state point that requires host handling.

For example:

- A new dialogue appears
- A choice appears
- An ending appears

### `choose(choiceId)`

Means:

> The player has selected an option, continue the story with it.

The host does not need to know how to "consume dialogue" internally, nor understand engine internal pause details.

This is the principle of NovaMark's external API:

> **The host expresses intent, the engine handles state advancement.**

---

## 3. How Host and Script Divide Responsibilities

This is one of NovaMark's most important design boundaries.

### Script is Responsible for

- Story content
- Scenes and jumps
- Conditional branches
- State changes
- Resource references
- Check and choice logic

### Host is Responsible for

- UI layout
- Dialogue box styling
- HUD
- Animations
- Typewriter effect
- Save timing
- File management
- Platform adaptation

This means the same content can be consumed by different platforms:

- Web template can render as a chat interface
- Native can render as a visual novel interface
- CLI can render as text mode

The script does not need to know these differences.

---

## 4. What is Runtime State

NovaMark's runtime state is the data the host actually consumes.

It typically includes:

- Current scene/label
- Variables
- Inventory
- Character definitions
- Item definitions
- Dialogue
- Choices
- Background
- BGM
- Sprites
- Text configuration

After receiving this state, the host can decide:

- How to display
- Where to display
- What animations to use

So from a platform integration perspective, NovaMark is more like:

> **A narrative state provider**

Not a "UI framework".

---

## 5. Why `transition / position / opacity / loop / volume` are Just Rendering Hints

You will see these fields in scripts:

- `transition`
- `position`
- `opacity`
- `loop`
- `volume`

These fields are preserved to express creator intent, but they do not mean the engine has a complete timeline system internally.

### Correct Understanding

They are:

- Rendering hints
- Consumable state fields

Not:

- Automatically executed continuous animations by the engine
- Internal media timeline controllers in the engine

This is exactly what platform integrators should note:

> These fields should be interpreted and rendered by your host platform.

---

## 6. What is the Web Template's Role

The Web template NovaMark currently provides is not "the only correct frontend", but:

- A reference implementation
- A runnable example
- A template showing how the host should consume runtime state

You can understand it as:

> **A minimal but complete integration demo**

It is suitable for:

- Quick testing
- Comparing against runtime state structure
- Understanding `advance()` / `choose()` host responsibilities

For detailed Web template documentation, see [Web Template Documentation](./web-template/).

But if you are a studio or platform, you can use it as a reference and then connect your own:

- Native UI
- Commercial distribution frontend

---

## 7. What is the C API's Role

If you don't want to bind internal classes directly at the C++ layer, but want a more stable host interface layer, then the C API is the better entry point.

It is suitable for:

- C/C++ integration
- Cross-language bindings
- WASM export
- NAPI / JNI / FFI wrappers

Currently you can view it as:

> **The standard host interface for the platform integration layer**

Its role is not to expose all internal implementation, but to let the host reliably:

- Create and destroy VM
- Load packages
- Advance story
- Select choices
- Read state
- Save and restore snapshots

For detailed C API documentation, see [C API](../api/c-api/). For Web platform bindings, see [WASM API](./wasm-api/).

---

## 8. How to Understand Saves in the Platform Integration Layer

NovaMark has split saves into two layers:

### Formal File Saves

- Binary format
- For players and production environments

### JSON Snapshots

- Debug format
- Test format
- Web/WASM development tool format

So for platform integrators, the recommended understanding is:

- Formal game saves -> Binary
- Debug and development -> JSON

---

## 9. Recommended Integration Order for Studios / Publishers

### Step 1: First Connect Package Loading and Runtime Advancement

First ensure:

- `load package`
- `advance()`
- `choose(choiceId)`

This chain works.

### Step 2: Connect Runtime State

First connect these states to your frontend:

- dialogue
- choices
- bg
- sprites
- variables / inventory

### Step 3: Then Connect Rendering Hint Fields

Such as:

- `transition`
- `position`
- `opacity`
- `loop`
- `volume`

### Step 4: Finally Connect Saves

Recommend using JSON debug snapshots first to get it working, then connect formal binary saves.

---

## 10. What to Read Next

If you are a platform integrator, continue with:

- [C API Reference](../api/c-api/) - Detailed host interface documentation
- [WASM API](./wasm-api/) - Web platform bindings
- [Web Template](./web-template/) - Complete frontend example
- [Native Integration](./native/) - Native platform guides
- Runtime state documentation
- Official sample projects

If you are a creator, return to:

- [For Creators](../guide/)
