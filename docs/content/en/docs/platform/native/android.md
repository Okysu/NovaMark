---
title: "Android Integration Guide"
weight: 2
---

# Android Integration Guide

This document is for developers who need to integrate the NovaMark runtime into Android applications.

## Prerequisite Understanding

Before starting, make sure you have read [Runtime, Host, and Platform Integration](../runtime-and-host/) and understand these core concepts:

- **Discrete state machine**: NovaMark VM does not advance time on its own, but waits for host calls
- **Host responsibilities**: Android side handles UI rendering, animations, save management, platform adaptation
- **State-driven**: Renderer only consumes runtime state, does not participate in story logic

---

## Architecture Options

NovaMark supports two main integration approaches on Android:

### Option 1: WebView + WASM

Compile NovaMark to WebAssembly module and run it in Android WebView.

**Suitable Scenarios**:
- Have existing Web renderer to reuse
- Team familiar with Web frontend tech stack
- Need hot-update capability

**Integration Points**:
- Use Emscripten to compile NovaMark to `.wasm` file
- Package Web renderer template (HTML/CSS/JS) into Android resources
- WebView accesses native capabilities through JavaScript bridge (saves, file access, etc.)

**Notes**:
- Game package (`.nvmp`) needs to be passed to WASM from native layer through bridge
- Save operations need to bridge to Android native file system
- Audio playback recommends using native MediaPlayer instead of WebAudio

### Option 2: Native Rendering

Call NovaMark C API through JNI and implement Android native UI yourself.

**Suitable Scenarios**:
- Pursuing optimal performance
- Need deep integration with Android system capabilities
- Want fully controllable rendering

**Integration Points**:
- Use NDK to compile NovaMark as shared library (`.so`)
- Wrap C API through JNI for Kotlin/Java calls
- Implement UI rendering layer yourself (can use Jetpack Compose or traditional View system)

**Notes**:
- Need to understand `NovaState` structure and map to UI components
- Need to handle animations and transition effects yourself
- Need to manage threading model (VM on background thread, UI on main thread)

### Selection Recommendation

| Dimension | WebView + WASM | Native Rendering |
|-----------|----------------|------------------|
| Development cost | Low (reuse Web template) | High (need to build rendering layer) |
| Runtime performance | Medium | Optimal |
| Hot update | Supported | Not supported |
| Native capability integration | Requires bridging | Direct access |
| UI consistency | Consistent with Web | Fully customizable |

---

## Integration Steps

The following uses native rendering approach as an example to explain the basic integration flow.

### Step 1: Compile NovaMark Shared Library

1. Configure NDK environment
2. Cross-compile NovaMark to `libnova.so` using CMake
3. Configure NDK build parameters in `build.gradle`

### Step 2: Wrap JNI Interface

Wrap C API as callable Kotlin/Java interfaces. Core operations include:

**Lifecycle Management**:
- Create VM instance
- Load game package (`.nvmp` file)
- Destroy VM instance

**Story Advancement**:
- `advance()` - Advance to next state point
- `choose(choiceId)` - Select choice

**State Reading**:
- Get `NovaState` structure
- Parse dialogue, choices, background, sprites, etc.

### Step 3: Implement UI Rendering Layer

Design Android UI based on `NovaState` structure:

**Dialogue Display**:
- Read `dialogue.speaker` to show character name
- Read `dialogue.text` to show dialogue content
- Implement typewriter effect (controlled by host, not engine behavior)

**Choice Rendering**:
- Read `choices` array
- Dynamically generate choice buttons
- Call `choose()` after tap

**Background and Sprites**:
- Read `bg` field to get background image path
- Read `sprites` array to get sprite information
- Decide transition animation implementation yourself

### Step 4: Implement Event Loop

Establish interaction pattern between VM execution and UI updates:

```
User action -> Call advance()/choose() -> VM updates state -> Read NovaState -> Refresh UI -> Wait for next user action
```

---

## State-Driven Rendering

NovaMark uses a state-driven model. Android host needs to understand how to consume runtime state.

### NovaState Structure Overview

Runtime state contains these main fields:

| Field | Description | Usage |
|-------|-------------|-------|
| `dialogue` | Current dialogue | Display character name and text |
| `choices` | Available choices list | Render selection buttons |
| `bg` | Background image path | Load and display background |
| `sprites` | Sprite list | Display character sprites |
| `bgm` | Background music path | Play BGM |
| `sfx` | Sound effect path | Play sound effects |
| `variables` | Variable table | Display status, unlock conditions, etc. |
| `inventory` | Item inventory | Display inventory interface |
| `textConfig` | Text configuration | Dialogue box style, font, etc. |

### Rendering Hint Fields

The following fields are provided by scripts but **interpreted and executed by the host**:

- `transition` - Transition effect hint (fade, slide, etc.)
- `position` - Position hint (left, center, right, etc.)
- `opacity` - Opacity hint
- `loop` - Loop hint (for BGM, etc.)
- `volume` - Volume hint

These fields express creator intent. Android host can:
- Fully follow hints
- Partially adopt, partially use platform defaults
- Ignore certain hints (e.g., degraded rendering on low-end devices)

### State Update Timing

VM state only changes at these times:
- After calling `advance()`
- After calling `choose()`
- After loading a save

Host does not need to poll state, just read new state after calling VM methods.

---

## Input Handling

NovaMark has only two core inputs: `advance()` and `choose()`.

### advance Trigger Timing

Host should call `advance()` when user performs:
- Tap blank area of dialogue box
- Tap "Continue" button
- Press confirm key

### choose Trigger Timing

When `NovaState.choices` is not empty:
- Display choice list
- After user taps a choice, call `choose(choiceId)`
- `choiceId` is obtained from `choices` array

### Threading Model

Recommended threading approach:

```
Main thread (UI thread):
  - Render interface
  - Receive user input
  - Update UI components

Background thread:
  - Call VM methods (advance/choose)
  - Read NovaState
  - Parse resource paths
```

Use `LiveData`, `Flow`, or `Handler` to pass state updates between threads.

---

## Save Management

NovaMark divides saves into two layers. Android host needs to handle them separately.

### Binary Save (Production)

Use C API snapshot functionality:

- `nova_save_snapshot_file()` - Save binary snapshot
- `nova_load_snapshot_file()` - Load binary snapshot

**Recommended save file locations**:
- Internal storage: `saves/` directory under `getFilesDir()`
- Support multiple save slots

**Save timing**:
- User manual save
- Auto-save (on scene transitions)
- Before key choices

### JSON Snapshot (Debug and Test)

Use `nova_export_runtime_state_json()` to get JSON format state:

**Uses**:
- View complete state during development
- Debug tool display
- Automated test verification

**Not recommended** for formal saves because:
- Larger file size
- Slower parsing

### Save Interface Responsibilities

Save interface implementation is entirely host's responsibility:
- Save slot layout
- Save thumbnails (host takes and saves screenshots itself)
- Save timestamps
- Save metadata (e.g., current scene name)

NovaMark engine only provides save and restore functionality for state snapshots.

---

## Debugging Suggestions

### Logging and Error Handling

- Catch C API errors in JNI layer, output through `Log.e()`
- Use `nova_get_error()` to get detailed error information
- Distinguish VM errors (e.g., script logic) from host errors (e.g., resource load failure)

### Resource Debugging

If resource loading fails, check:
- Whether `.nvmp` package correctly packaged resources
- Whether resource paths are correctly passed to Android resource system
- Whether file permissions are correct

### State Verification

During development, you can:
- Export complete state using JSON snapshot
- Compare expected state with actual state
- Verify variables, inventory, scene are correct

### Performance Tuning

- Use appropriate sampling rate for image resources
- Use streaming playback for BGM instead of one-time loading
- Use sprite atlases to reduce draw calls
- Avoid repeatedly reading `NovaState` every frame, only read after state changes

---

## Integration Checklist

After completing integration, confirm these features work:

- [ ] Load `.nvmp` game package
- [ ] Display dialogue content (character name + text)
- [ ] Display choices and respond to selection
- [ ] Display background image
- [ ] Display sprites
- [ ] Play BGM and sound effects
- [ ] Save and load games
- [ ] Variable reading (for UI display)
- [ ] Inventory item display

---

## Related Documentation

- [Runtime, Host, and Platform Integration](../runtime-and-host/)
- [C API Reference](../../api/c-api/)
- [Runtime State Structure](../../api/runtime-state/)
