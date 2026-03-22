---
title: "Web Rendering Template"
weight: 2
---

# Web Rendering Template

`template/web` is the official Web renderer reference implementation provided by NovaMark. It demonstrates how the host correctly consumes runtime state, handles user input, and manages saves. It is the best example for platform integrators to understand the "core VM + dumb renderer" architecture.

---

## Template Purpose

The Web template is not "the only correct frontend", but:

- A **minimal but complete** integration demo
- A **ready-to-run** example
- **Reference code** showing host responsibility boundaries

You can use it to quickly validate game packages, or as a starting point for custom development.

---

## Directory Structure

```
template/web/
├── CMakeLists.txt          # WASM build configuration
├── index.html              # Entry page
├── style.css               # Stylesheet
├── app.js                  # Host logic (event binding, state rendering)
├── nova_renderer.js        # Renderer wrapper (VM state queries, asset loading)
└── src/
    └── nova_wasm_main.cpp  # WASM entry, exports C API functions
```

### Core File Responsibilities

| File | Responsibility |
|------|----------------|
| `nova_renderer.js` | Wraps WASM bindings, provides state query APIs (`getBackground()`, `getDialogueText()`, etc.) |
| `app.js` | Host logic: handles user interaction, calls `advance()`/`choose()`, updates DOM |
| `style.css` | Visual styles, supports VN mode and Chat mode layouts |
| `index.html` | Page skeleton, includes mode switch, menu, dialogue box, and other DOM structures |

---

## Local Execution

### Prerequisites

1. **Emscripten SDK** - Configured and activated
2. **vcpkg** - Installed and toolchain configured
3. **CMake 3.16+**

### Build Steps

```bash
# 1. Configure project (using Emscripten toolchain)
cmake -B build-wasm -S . \
  -DCMAKE_TOOLCHAIN_FILE=$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
  -DCMAKE_BUILD_TYPE=Release

# 2. Build WASM runtime
cmake --build build-wasm --target nova-wasm-runtime

# 3. Enter output directory
cd build-wasm/template/web

# 4. Start local server (requires HTTP server, cannot open HTML directly)
python -m http.server 8080
```

### Access

Open `http://localhost:8080` in your browser and select a `.nvmp` game package to run.

---

## Minimal Integration Steps

If you want to integrate NovaMark in other Web projects, you only need three files:

### 1. Include WASM Runtime

```html
<script src="nova.js"></script>        <!-- Emscripten-generated runtime -->
<script src="nova_renderer.js"></script>  <!-- State query wrapper -->
```

### 2. Initialize Renderer

```javascript
const renderer = new NovaRenderer();

// Load game package
async function loadGame(nvmpUrl) {
  const response = await fetch(nvmpUrl);
  const nvmpData = await response.arrayBuffer();
  await renderer.init(nvmpData);
  
  // Advance to first state point
  renderer.advance();
  renderFrame();
}
```

### 3. Render State

```javascript
function renderFrame() {
  // Read state
  const bg = renderer.getBackground();
  const dialogue = renderer.hasDialogue() 
    ? { speaker: renderer.getDialogueSpeaker(), text: renderer.getDialogueText() }
    : null;
  const choices = renderer.hasChoices() 
    ? Array.from({ length: renderer.getChoiceCount() }, (_, i) => renderer.getChoiceText(i))
    : [];
  
  // Render to your UI
  updateBackground(bg);
  updateDialogue(dialogue);
  updateChoices(choices);
}
```

---

## State-Driven Rendering

NovaMark uses a "discrete state machine" model: the VM stops at each state point requiring host handling and waits for a host response.

### State Query API

State query methods provided by `NovaRenderer`:

| Method | Return Value | Description |
|--------|--------------|-------------|
| `getStatus()` | `0`=running, `1`=waiting for input, `2`=ended | Current VM status |
| `isEnded()` | boolean | Whether game has ended |
| `getBackground()` | string | Background image asset name |
| `hasDialogue()` | boolean | Whether dialogue is pending display |
| `getDialogueSpeaker()` | string | Speaker name |
| `getDialogueText()` | string | Dialogue text |
| `getDialogueColor()` | string | Speaker color (hexadecimal) |
| `hasChoices()` | boolean | Whether choices are pending selection |
| `getChoiceCount()` | number | Number of choices |
| `getChoiceText(index)` | string | Text of specified choice |
| `isChoiceDisabled(index)` | boolean | Whether choice is disabled |
| `getSpriteCount()` | number | Current sprite count |
| `getSpriteUrl(index)` | string | Sprite asset name |
| `getSpriteX/Y(index)` | number | Sprite position (percentage) |
| `getSpriteOpacity(index)` | number | Sprite opacity (0-1) |
| `getBgm()` | string | BGM asset name |
| `getBgmVolume()` | number | BGM volume (0-1) |
| `getBgmLoop()` | boolean | Whether BGM loops |

### Render Loop

The template uses an "event-driven" pattern, not continuous polling:

```
User click -> advance()/choose() -> VM advances -> Read new state -> Update UI
```

Core code example (`app.js`):

```javascript
function handleAdvance() {
  if (renderer.isEnded()) return;
  if (renderer.hasChoices()) return;  // Don't respond to advance clicks when choices exist
  
  renderer.advance();
  renderGame();
}

function handleChoice(index) {
  renderer.selectChoice(index);
  renderer.advance();
  renderGame();
}
```

### Typewriter Effect

The template has a built-in typewriter effect, controlled by `textConfig.defaultTextSpeed`:

```javascript
// textSpeed = 0 means instant display, otherwise milliseconds per character
typewriter.start(element, text, textSpeed, onComplete);
```

---

## Input and Saves

### User Input

The host is responsible for converting user interactions to VM operations:

| User Action | VM Operation |
|-------------|--------------|
| Click dialogue box/background | `advance()` |
| Click choice button | `choose(index)` |

### Save Mechanism

NovaMark provides two save formats:

| Format | Purpose | API |
|--------|---------|-----|
| Binary (`.nvs`) | Formal saves, smaller size | `exportSaveBinary()` / `importSaveBinary(bytes)` |
| JSON | Debug, development tools | `exportSaveJson()` / `importSaveJson(json)` |

Example:

```javascript
// Save
function handleSave() {
  const bytes = renderer.exportSaveBinary();
  const blob = new Blob([bytes], { type: 'application/octet-stream' });
  const url = URL.createObjectURL(blob);
  // Trigger download...
}

// Load
async function handleLoad(file) {
  const bytes = await file.arrayBuffer();
  renderer.importSaveBinary(new Uint8Array(bytes));
  renderGame();
}
```

### Runtime State Snapshot

During debugging, you can get complete runtime state:

```javascript
const state = renderer.getRuntimeState();
// state contains: variables, inventory, characterDefinitions, currentScene, etc.
```

---

## Debugging Suggestions

### Browser Console API

The template exposes a `window.novaDebug` object for direct console operations:

```javascript
// View current status
novaDebug.status()

// Get complete snapshot
novaDebug.snapshot()

// Manual advance
novaDebug.advance()

// Select choice
novaDebug.select(0)

// View save data
novaDebug.saveJson()
```

### Common Issue Troubleshooting

| Issue | Troubleshooting Direction |
|-------|---------------------------|
| Game package load failure | Check `.nvmp` file integrity, check console error messages |
| Assets not loading | Confirm assets are correctly packaged into `.nvmp` |
| Audio not playing | Browser may block autoplay, needs user interaction before playing |
| Save restore failure | Check if save version matches game version |

---

## Customization and Extension

### Replace UI Styles

`style.css` is pure CSS and can be directly modified or replaced. Main style classes:

- `.vn-dialogue` - VN mode dialogue box
- `.vn-choice` - VN mode choice buttons
- `.vn-sprite` - Sprites
- `.chat-message` - Chat mode message bubbles
- `.chat-choice` - Chat mode choice buttons

### Replace Rendering Logic

`renderVNMode()` and `renderChatMode()` in `app.js` are rendering entry points. You can:

1. **Complete rewrite** - Delete existing render functions, implement your own design
2. **Incremental modification** - Add animations and effects on existing basis
3. **Switch frameworks** - Replace native DOM operations with React/Vue/Svelte

### Add New Features

Features the template does not implement but can be extended:

- **History review** - Cache past dialogues, add review UI
- **Auto-play** - Call `advance()` on timer
- **Skip mode** - Skip typewriter effect, fast advance
- **Settings panel** - Adjust text speed, volume, etc.
- **Save thumbnails** - Capture screen when saving

### Asset Loading Optimization

The template has built-in asset caching (`assetCache`, `imageCache`). For further optimization:

- Use IndexedDB for large asset caching
- Implement asset preloading
- Use Web Workers for decompression

---

## Next Steps

- [Runtime and Host](./runtime-and-host/) - Understand VM and host responsibility boundaries
- [Runtime State](../api/runtime-state/) - Complete state field definitions
- [C API](../api/c-api/) - Native platform integration reference
