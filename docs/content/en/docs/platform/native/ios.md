---
title: "iOS Integration Guide"
weight: 1
---

# iOS Integration Guide

This guide is for developers who need to integrate the NovaMark runtime on the iOS platform.

## Architecture Options

iOS has two mainstream integration approaches, each with pros and cons:

### WebView + WASM Approach

Compile the NovaMark core to WebAssembly and run it in `WKWebView`.

**Advantages**:
- Reuse existing Web renderer
- Hot-update friendly, no need to resubmit to App Store
- High cross-platform rendering consistency
- Fast development iteration

**Disadvantages**:
- WASM runtime has additional overhead
- Native capability calls require bridging
- First load requires downloading WASM module

**Suitable Scenarios**:
- Have existing Web renderer to reuse
- Need to launch quickly
- Not highly demanding on native performance

### Native Rendering Approach

Directly integrate NovaMark VM through the C API and implement native UI rendering using Swift or Objective-C.

**Advantages**:
- Best performance, no intermediate layer overhead
- Direct access to iOS native capabilities
- Fully controllable rendering effects
- Can deeply customize animations and interactions

**Disadvantages**:
- Need to implement complete rendering logic yourself
- Dialogue, choices, sprites, backgrounds all need independent development
- Higher maintenance cost

**Suitable Scenarios**:
- Pursuing ultimate performance
- Need deep native customization
- Already have mature iOS UI framework

---

## Integration Steps

### Step 1: Prepare NovaMark Runtime

Based on your chosen approach:

**WebView + WASM Approach**:
1. Use Emscripten to compile NovaMark to WASM module
2. Package WASM file with Web renderer template
3. Put resource files in App Bundle or load from server

**Native Rendering Approach**:
1. Compile NovaMark to static or dynamic library supported by iOS
2. Link C API header files in Xcode project
3. Call through FFI or Swift/C bridging layer

### Step 2: Load Game Package

NovaMark games are distributed as single `.nvmp` files.

**Core Operations**:
- Read game package from App Bundle
- Or download from server and cache locally
- Call load interface to initialize VM

**Notes**:
- Large game packages are recommended to be loaded in chunks or preloaded in background
- Resource files (images, audio) can be managed separately and loaded on demand

### Step 3: Implement Event Loop

NovaMark uses a **discrete state machine + host-controlled advancement** model.

The engine does not auto-play, but waits for host commands:

- `advance()` - Advance to next blocking point
- `choose(choiceId)` - Continue after player selects an option

The host needs to:
1. Listen for user input (taps, swipes, key presses, etc.)
2. Decide whether to call `advance()` or `choose()` based on current state
3. Update UI to present new state

---

## State-Driven Rendering

NovaMark's core design principle:

> **Engine produces state, host handles rendering**

### NovaState Structure

Runtime state is the only data source for rendering, containing:

| Field | Description |
|-------|-------------|
| `dialogue` | Current dialogue content, speaking character, color |
| `choices` | List of available choices |
| `bg` | Background image reference |
| `sprites` | Sprite list and positions |
| `bgm` | Background music reference |
| `sfx` | Sound effect reference |
| `variables` | Game variables |
| `inventory` | Player inventory |

### Renderer Responsibilities

After receiving state, the host decides:
- Dialogue box style and position
- Sprite animations and transitions
- Background transition effects
- Choice UI layout

Fields like `transition`, `position`, `opacity` in scripts are **rendering hints**, not automatically executed animations by the engine. The host can:
- Follow these hints
- Or ignore them and use own rendering logic

### Text Configuration

Runtime state includes `textConfig`:
- `defaultFont` - Default font
- `defaultFontSize` - Default font size
- `defaultTextSpeed` - Typewriter speed

The host should read these configurations and implement typewriter effects.

---

## Input Handling

### advance() Trigger Timing

`advance()` means "continue advancing until the next state point requiring host handling".

Common trigger methods:
- Tap dialogue box or blank screen area
- Press specific key
- Voice command (if supported)

**Notes**:
- If choices currently exist, `advance()` calls should be blocked
- Should judge based on state whether dialogue is pending consumption

### choose(choiceId) Trigger Timing

`choose(choiceId)` means "player has selected an option".

**Implementation Points**:
- Read `choices` list to render choice UI
- After player selects, pass corresponding `choiceId`
- Choice UI should clear or hide after call

### Event Loop Example

```
User tap -> Check current state
         -> Has choices? Call choose(choiceId)
         -> No choices? Call advance()
         -> Wait for state update
         -> Re-render UI
```

---

## Save Management

NovaMark provides two save formats:

### Binary Save (Formal)

Used for player formal saves, compact format, fast loading.

**Host Responsibilities**:
- Manage save slots
- Save thumbnails (optional)
- Save timestamps
- Cloud sync (optional)

### JSON Snapshot (Debug)

Used for development debugging, test verification.

**Uses**:
- Print state in Xcode console
- Export snapshots for desktop debugging
- Unit test verification

**Not recommended** as formal save format.

### Save Timing

NovaMark does not automatically decide save timing. The host needs to:
- Prompt for save at key points (chapter end, after important choices)
- Provide auto-save slot
- Implement quick save / quick load features

---

## Debugging Suggestions

### Use JSON Snapshots

During development, export runtime snapshots in JSON format:
- Check variable values in Xcode console
- Verify inventory items are correct
- Confirm character definitions are loaded

### Runtime State Inspection

Real-time inspection through runtime state interface:
- Current scene and label
- Variable value changes
- Item counts

### Phased Verification

Recommended verification order:

1. **Package loading** - Confirm `.nvmp` file loads correctly
2. **State advancement** - `advance()` can normally advance dialogue
3. **Choice selection** - `choose()` can correctly handle branches
4. **State reading** - Can get correct runtime state
5. **Save restore** - Can correctly restore after saving

### Common Issue Troubleshooting

| Issue | Check Points |
|-------|--------------|
| Package load failure | File path, format version, resource integrity |
| Dialogue not advancing | Whether `advance()` is called correctly |
| Choice not responding | Whether passed `choiceId` is correct |
| Save restore abnormal | Snapshot format, version compatibility |

---

## Host and Script Responsibility Boundary

Correctly understanding responsibility division is key to smooth integration:

### Script is Responsible for

- Story content and branch logic
- Character definitions and attributes
- Item definitions and effects
- Scene jump relationships
- Conditional judgments

### Host is Responsible for

- UI layout and styling
- Animations and transition effects
- Typewriter effect
- Save UI and management
- File access and caching
- Audio playback control
- Platform adaptation (notch, safe areas, etc.)

The same NovaMark script can be rendered by:
- Chat interface style
- Visual novel style
- Pure text terminal

The script doesn't need to know these differences.

---

## Further Reference

- [Runtime, Host, and Platform Integration](../runtime-and-host/) - Core concepts explained
- [C API Reference](../../../api/c-api/) - Low-level interface documentation
- [Runtime State API](../../../api/runtime-state/) - State structure description
