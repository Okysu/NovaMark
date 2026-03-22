<div align="center">

<h1>NovaMark</h1>

<p><strong>A domain-specific scripting language and runtime engine for text games, interactive fiction, and visual novels.</strong></p>

<p>
  <a href="#architecture">Architecture</a> &middot;
  <a href="#syntax">Syntax</a> &middot;
  <a href="#quickstart">Quick Start</a> &middot;
  <a href="#platforms">Platforms</a> &middot;
  <a href="#roadmap">Roadmap</a>
</p>

<p>
  <img src="https://img.shields.io/badge/version-v0.1_alpha-blue" alt="Version">
  <img src="https://img.shields.io/badge/license-MIT-green" alt="License">
  <img src="https://img.shields.io/badge/C%2B%2B-17-orange" alt="C++17">
</p>

</div>

---

NovaMark enforces a strict boundary between narrative logic and the rendering layer, allowing a single compiled game package to run unmodified across Web, desktop, and mobile platforms.

> **Documentation:** Full syntax specification, architecture reference, and API docs are available at the [NovaMark Official Documentation](https://novamark.example.com) site *(source in `docs/`, built with Hugo)*.

---

## Architecture & Design Principles <a id="architecture"></a>

Most visual novel engines tightly couple game logic to a specific renderer. Ren'Py scripts depend on the Ren'Py runtime; Ink requires substantial integration work for anything beyond text. NovaMark takes a different position: **the engine core is a pure state machine with zero rendering dependencies**, and renderers are deliberately "dumb" — they receive state snapshots and draw them.

This separation produces concrete engineering benefits:

<table>
<thead>
<tr><th>Benefit</th><th>Description</th></tr>
</thead>
<tbody>
<tr>
  <td><strong>Portable game packages</strong></td>
  <td>Compiled <code>.nvmp</code> files bundle all scripts, assets, and metadata into a single binary archive. No extra toolchain or asset pipeline is needed at distribution time.</td>
</tr>
<tr>
  <td><strong>Swappable renderers</strong></td>
  <td>The same game package runs in a terminal renderer for debugging, in a browser via WebAssembly, or in a native app via C FFI. Adding a new target platform requires no changes to the game or engine core.</td>
</tr>
<tr>
  <td><strong>Serializable state</strong></td>
  <td>All game state flows through a single <code>NovaState</code> struct. Save/load, replay, and rollback are implementable entirely at the renderer layer — the engine core is untouched.</td>
</tr>
</tbody>
</table>

### Data Flow

```
.nvmp Game Package
  ├── AST Bytecode
  ├── Asset Index (byte offsets)
  └── Binary Asset Data (images, audio, fonts)
          |
          v
    NovaMark VM
  (Execute AST → Maintain discrete state machine → Listen for external input)
          |
    ┌─────┴──────┬──────────────┐
    v            v              v
Text Mode    Web Chat Mode   Web VN Mode
(CLI debug)  (WASM renderer) (WASM renderer)
```

### State Contract

The renderer obtains the current snapshot through `vm.state()` after each VM state deduction, then calls `vm.advance()` to step forward, or calls `vm.choose(id)` to select a branch, driving the main loop.

```typescript
interface NovaState {
  bg: string | null;
  bgm: string | null;
  sfx: Array<{ id: string; path: string; loop: boolean }>;
  sprites: Array<{ id: string; url: string; x: number; y: number; opacity: number; zIndex: number }>;
  dialogue: { isShow: boolean; name: string; text: string; color: string } | null;
  choices: Array<{ id: string; text: string; disabled: boolean }>;
}
```

---

## Syntax Overview <a id="syntax"></a>

NovaMark uses an indentation- and directive-based minimal syntax with support for variable arithmetic and branching control flow.

```novamark
---
title: Example Scene
version: 1.0
---

@char Xiaoming
  color: #4A90D9
  sprite_default: xiaoming_normal.png
@end

@var affection = 0

#scene_start "Prologue"

@bg room.png
@bgm peaceful.mp3

> It is an ordinary morning.

Xiaoming: Good morning! What a lovely day.

? How will you respond?
- [Smile back]    -> .smile
- [Stay silent]   -> .silent

.smile
Xiaoming: You seem to be in a good mood.
@set affection = affection + 10
-> scene_next

.silent
Xiaoming: ...Something wrong?
-> scene_next
```

---

## Quick Start <a id="quickstart"></a>

### Prerequisites

- CMake 3.16 or higher
- C++17-compatible compiler (GCC, Clang, or MSVC)
- vcpkg package manager

### Build from Source

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg_path]/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

### CLI Usage

The compiler and debugger are integrated into the `nova-cli` tool:

```bash
# Syntax check and static analysis
./build/src/cli/nova-cli check scripts/

# Compile scripts and assets into a binary package
./build/src/cli/nova-cli build scripts/ -o game.nvmp

# Launch a debug run in Text Mode
./build/src/cli/nova-cli run game.nvmp
```

**Text Mode hotkeys:** `Enter` — step forward &middot; `1`–`9` — select choice &middot; `S` — create snapshot (save) &middot; `L` — restore snapshot (load) &middot; `Q` — quit

---

## Platform Integration <a id="platforms"></a>

NovaMark provides two standardized integration paths for host applications:

### WebAssembly

The VM is compiled via Emscripten and exposes a JavaScript API. Two reference renderer layouts (chat mode and visual novel mode) are provided under `template/web/` and are recommended as the starting point for any Web deployment.

### C API / Native FFI

For native targets, the VM exposes a standard C API callable from any language with C FFI support (Rust, C#, Python, etc.). All reference renderer implementations are kept under 200 lines to facilitate direct forking and adaptation.

---

## Project Status & Roadmap <a id="roadmap"></a>

**Current version: v0.1 (Alpha)**

### Feature Matrix

<table>
<thead>
<tr><th>Module</th><th>Feature</th><th>Status</th></tr>
</thead>
<tbody>
<tr>
  <td rowspan="3"><strong>Language Parser</strong></td>
  <td>Character definitions, scene declarations, label jumps</td>
  <td>Stable</td>
</tr>
<tr>
  <td>Variable arithmetic, dice expressions (<code>2d6+3</code>), conditionals (<code>@if</code> / <code>@else</code>)</td>
  <td>Stable</td>
</tr>
<tr>
  <td>Item system (<code>@give</code> / <code>@take</code>), multiple endings, subroutines (<code>@call</code>)</td>
  <td>Stable</td>
</tr>
<tr>
  <td rowspan="3"><strong>Runtime</strong></td>
  <td>Text Mode terminal renderer (development &amp; debugging)</td>
  <td>Stable</td>
</tr>
<tr>
  <td>WASM compilation support and Web renderer templates</td>
  <td>Stable</td>
</tr>
<tr>
  <td>Cross-language C API exposure</td>
  <td>Stable</td>
</tr>
</tbody>
</table>

### Iteration Plan

**v0.2 — Renderer Enhancements**
- [ ] State snapshot thumbnail generation
- [ ] History text rollback system
- [ ] Auto-advance and skip logic

**v0.3 — Toolchain**
- [ ] NovaMark Creator (visual node editor and packaging GUI)
- [ ] Native platform renderer references (iOS / Android / HarmonyOS)

**v1.0 — Production Ready**
- [ ] Syntax specification freeze and backward-compatibility guarantee
- [ ] Performance profiling benchmarks and memory allocation optimization
- [ ] Complete API Reference coverage

---

## Dependencies & Repository Structure

To maintain an extremely low overhead footprint for mobile and embedded targets, NovaMark strictly limits third-party dependencies and avoids heavy frameworks such as Boost or Qt.

<table>
<thead>
<tr><th>Library</th><th>Purpose</th><th>Distribution</th></tr>
</thead>
<tbody>
<tr>
  <td>nlohmann/json</td>
  <td>State machine snapshots and configuration parsing</td>
  <td>Header-only, bundled with source</td>
</tr>
<tr>
  <td>GoogleTest</td>
  <td>Core logic unit testing</td>
  <td>Development dependency only</td>
</tr>
</tbody>
</table>

### Directory Layout

```
NovaMark/
├── src/           # Engine core
│   ├── lexer/     # Lexer
│   ├── parser/    # Parser
│   ├── ast/       # AST definitions
│   ├── semantic/  # Semantic analysis
│   ├── vm/        # State machine and runtime
│   ├── packer/    # Asset packing module
│   └── cli/       # CLI entry point
├── include/nova/  # Public C++ headers and C API
├── tests/         # Unit and integration tests
├── docs/          # Documentation site source
├── examples/      # Example projects covering various syntax features
└── template/      # Reference renderer implementations per platform
```

---

## Contributing

Bug reports and feature proposals are welcome via Issues. Pull requests that involve architectural changes or syntax modifications must open an Issue for technical review first. All submissions must pass the full CMake CTest suite.

## License

This project is open source under the [MIT License](./LICENSE).