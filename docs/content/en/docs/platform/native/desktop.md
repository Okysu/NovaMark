---
title: "Desktop Integration Guide"
weight: 3
---

# Desktop Integration Guide

This document is for Windows, macOS, and Linux desktop application developers, introducing how to integrate the NovaMark runtime into desktop applications.

---

## 1. Architecture Options

Desktop has two mainstream integration approaches. Choose based on your project needs:

### Option A: Embedded WebView + WASM

Compile NovaMark to WebAssembly and run it in an embedded WebView in the desktop application.

**Suitable Scenarios**:

- Have existing Web frontend team
- Need to quickly validate prototype
- Reuse Web template rendering logic
- Need hot-update capability

**Implementation Points**:

- Use platform-native WebView controls
- NovaMark WASM module communicates with host through JS bridge
- Host provides file system, saves, and other native capability bridging

**Notes**:

- WebView rendering performance slightly lower than native
- Need to handle cross-platform WebView differences
- Native capability calls require additional bridging layer

### Option B: Native Rendering

Directly integrate NovaMark VM using the C API and implement rendering at the native layer.

**Suitable Scenarios**:

- Pursuing best performance
- Need deep customization of rendering pipeline
- Need direct access to native capabilities
- Already have mature native UI framework

**Implementation Points**:

- Load `.nvmp` package and control VM through C API
- Host polls or listens for NovaState changes
- Implement all UI rendering logic yourself

**Notes**:

- Need to implement text rendering, image display, audio playback yourself
- Multiple platforms may require multiple rendering codebases
- Longer development cycle

---

## 2. Integration Steps

### Step 1: Prepare Runtime

Regardless of which approach you choose, you need:

1. **Get NovaMark Runtime**
   - WebView approach: Compile WASM module
   - Native approach: Compile C API static library / dynamic library

2. **Prepare Test Package**
   - Use `nova-cli build` to build a simple `.nvmp` test package
   - Ensure package can load normally

### Step 2: Implement Basic Control Flow

Implement these core functions:

**Load Package**

```
Load .nvmp file into VM
Initialize runtime state
```

**Advance Story**

Call `advance()` to let VM advance to the next state point requiring host handling.

**Handle Choice**

Call `choose(choiceId)` to tell VM which option the player selected.

**Validation Criteria**:

- Can load package and display initial dialogue
- Can advance dialogue until choice appears
- Can select option and continue story

### Step 3: Connect Runtime State

Connect NovaState's key fields to your rendering layer:

| State Field | Usage |
|-------------|-------|
| `dialogue` | Current dialogue text, speaking character |
| `choices` | Available choices list |
| `bg` | Background image resource reference |
| `sprites` | Sprite resource references |
| `bgm` / `sfx` | Audio resource references |
| `variables` | Variable values (for UI display) |
| `inventory` | Item inventory state |

**State Reading Principles**:

- Host only reads state, does not directly modify
- All state changes are driven by VM
- Host decides how to render based on state

### Step 4: Implement Rendering Hint Handling

NovaMark scripts may contain rendering hint fields:

| Field | Meaning | Host Handling Suggestion |
|-------|---------|--------------------------|
| `transition` | Transition effect type | Parse and implement transition animation |
| `position` | Element position | Map to layout coordinates |
| `opacity` | Opacity | Control element fade in/out |
| `loop` | Whether to loop | Control audio/animation looping |
| `volume` | Volume | Control audio playback volume |

**Important**: These fields are **rendering hints**, not automatically executed behaviors by the engine. The host is responsible for interpreting and implementing them.

### Step 5: Implement Save Functionality

NovaMark provides two save formats:

**Formal Save (Binary)**

- For production environment
- Small size, fast loading
- Operate through C API save interfaces

**Debug Snapshot (JSON)**

- For development and debugging
- Good readability
- Convenient for troubleshooting state issues

**Recommendation**:

- Use JSON snapshots to verify save logic during development
- Switch to binary format in production

---

## 3. State-Driven Rendering

NovaMark's core design is **state-driven**. Understanding this is critical for desktop integration.

### What is State-Driven

```
┌─────────────┐     State Change     ┌─────────────┐
│  NovaMark   │ ───────────────────▶ │    Host     │
│     VM      │                      │  Renderer   │
└─────────────┘                      └─────────────┘
                                           │
                                           ▼
                                      Update UI Display
```

The engine maintains `NovaState`. The host is responsible for "observing" this state and rendering.

### Event Loop Pattern

Typical desktop event loop:

```
1. User input (click/keypress)
       │
       ▼
2. Host calls advance() or choose()
       │
       ▼
3. VM updates NovaState
       │
       ▼
4. Host reads new state
       │
       ▼
5. Host updates rendering
       │
       ▼
6. Wait for next user input
```

### Host Rendering Responsibilities

Based on NovaState, the host needs to decide:

- **How to display dialogue**: Text box style, typewriter effect, character name position
- **How to present choices**: Button style, layout, animation
- **How to switch background/sprites**: Transition effects, animation duration
- **How to play audio**: BGM switching, sound effect triggering
- **When UI appears**: Menu, save interface, settings panel

All of these are controlled by the host. Scripts do not interfere.

---

## 4. Input Handling

### Core Input Actions

Desktop needs to handle these inputs:

| Input Type | Corresponding VM Operation | Description |
|------------|---------------------------|-------------|
| Click/Enter | `advance()` | Advance to next state point |
| Select choice | `choose(choiceId)` | Player makes a choice |

### Extended Inputs (Optional)

Besides core inputs, the host can decide whether to support:

- **Hotkeys**: Save/Load/Skip/Auto-play
- **History review**: Display previous dialogues
- **Menu buttons**: Open settings/save interface

These features are implemented by the host. The VM is unaware of them.

### Input Timing

Only allow input at specific states:

- When dialogue exists: Allow `advance()`
- When choices exist: Block `advance()`, only allow `choose()`
- Idle state: Allow opening menu

The host needs to judge currently allowed input types based on NovaState.

---

## 5. Save Management

### Save Data Content

NovaMark saves contain:

- Current execution position (scene/label)
- All variable values
- Item inventory state
- Triggered endings/events markers

### Save Operation Flow

**Save**:

```
1. Call VM save interface to get snapshot data
2. Write data to local file
3. Optional: Generate save metadata (timestamp, screenshot, etc.)
```

**Load**:

```
1. Read snapshot data from file
2. Call VM load interface to restore state
3. Update rendering based on restored NovaState
```

### Multiple Save Slots

If multiple save slots are needed:

- Host manages slot list and metadata
- Each slot corresponds to an independent save file
- VM only handles generating/consuming single save data

---

## 6. Debugging Suggestions

### Use JSON Snapshots

Recommend using JSON snapshot format during development:

- Can directly view state content
- Convenient for locating rendering issues
- Easy for automated testing

### State Change Logging

Recommend logging in development builds:

- Every `advance()` / `choose()` call
- NovaState key field changes
- Rendering hint field values

### Compare with Web Template

If rendering results don't match expectations:

1. Run the same package in Web template
2. Compare against Web template's state display
3. Find differences to locate issues

### Common Issue Troubleshooting

| Issue | Troubleshooting Direction |
|-------|---------------------------|
| Dialogue not displaying | Check if dialogue field is correctly read |
| Choices not selectable | Check choices list and choiceId mapping |
| Background not switching | Check bg field and resource loading |
| Save not restoring | Check snapshot data integrity |

---

## 7. Summary

Key points for desktop integration of NovaMark:

1. **Understand state-driven model**: VM manages state, host manages rendering
2. **Choose right architecture**: WebView for quick validation, native rendering for best performance
3. **Follow steps**: Get flow working first, then refine details
4. **Mind save separation**: JSON for development, binary for production
5. **Use debugging tools**: Snapshot logging and Web template comparison

After integration is complete, your desktop application can run NovaMark game content.
