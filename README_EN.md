# NovaMark

<div align="center">

**A markup language and runtime engine for interactive fiction and visual novels**

English | [简体中文](./README.md)

</div>

---

## Overview

NovaMark is a markup language and cross-platform runtime engine designed for text-based games, interactive fiction, and visual novels.

**Core Design Principles**:
- 🎯 **Clean Syntax** - As readable and writable as Markdown
- ⚡ **High-Performance Core** - C++17 implementation with lightweight VM
- 🌐 **Cross-Platform Rendering** - Decoupled VM and renderers for multi-platform support
- 📦 **Single-File Distribution** - Scripts and assets packaged into one `.nvmp` file

## Features

### Language Features
- ✅ Character definitions and dialogues
- ✅ Scenes and label jumps
- ✅ Branching choice system
- ✅ Variables and expression evaluation
- ✅ Dice expressions (e.g., `2d6+3`)
- ✅ Conditional branching (`@if`/`@else`)
- ✅ Background, sprite, BGM, and SFX commands
- ✅ Item system (`@give`/`@take`)
- ✅ Multiple endings support
- ✅ Subroutine calls (`@call`/`@return`)
- ✅ Save points and save system

### Runtime Features
- ✅ Text Mode - Command-line text renderer (for debugging)
- 🚧 Web Renderer - Browser GUI renderer (in development)
  - Chat Mode - Chat interface style
  - VN Mode - Visual novel style

## Quick Start

### Syntax Example

```novamark
---
title: My Game
version: 1.0
---

@char Alice
  color: #4A90D9
  sprite_default: alice_normal.png
@end

@var affection = 0

#scene_start "Prologue"

@bg room.png
@bgm peaceful.mp3

> It was an ordinary morning...

Alice: Good morning! The weather is nice today.

? How do you respond?
- [Smile back] -> .smile
- [Stay silent] -> .silent

.smile
Alice: Looks like you're in a good mood!
@give affection 10
-> scene_next

.silent
Alice: ...Is something wrong?
-> scene_next
```

### Build

**Prerequisites**:
- CMake 3.16+
- C++17 compiler
- vcpkg package manager

```bash
# 1. Configure
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg]/scripts/buildsystems/vcpkg.cmake

# 2. Build
cmake --build build --config Release

# 3. Run tests
ctest --test-dir build --output-on-failure
```

### CLI Usage

```bash
# Build game package
./build/src/cli/nova-cli build scripts/ -o game.nvmp

# Run game (Text Mode)
./build/src/cli/nova-cli run game.nvmp

# Check syntax
./build/src/cli/nova-cli check scripts/
```

**Runtime Hotkeys** (Text Mode):
- `Enter` - Advance dialogue
- `1-9` - Select choice
- `S` - Save game
- `L` - Load game
- `Q` - Quit

## Project Structure

```
NovaMark/
├── src/
│   ├── lexer/         # Lexer
│   ├── parser/        # Parser
│   ├── ast/           # Abstract Syntax Tree
│   ├── semantic/      # Semantic analyzer
│   ├── vm/            # Virtual machine
│   ├── packer/        # Packaging tool
│   ├── renderer/      # Renderer interface
│   └── cli/           # Command-line tool
├── include/nova/      # Public headers
├── tests/             # Test suite
├── docs/              # Design documents
├── examples/          # Example scripts
└── template/          # Renderer templates
    └── web/           # Web renderer template (in development)
```

## Architecture

NovaMark uses a **"Core VM + Dumb Renderer"** architecture:

```
┌─────────────────────────────────────────────────────────┐
│                    .nvmp Game Package                    │
│  ┌─────────────┐  ┌──────────┐  ┌──────────────────┐   │
│  │ AST Bytecode│  │  Index   │  │   Asset Data     │   │
│  │             │  │  Table   │  │ (images/audio/   │   │
│  │             │  │          │  │  fonts)          │   │
│  └─────────────┘  └──────────┘  └──────────────────┘   │
└─────────────────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│                    NovaMark VM                          │
│  ┌──────────────────────────────────────────────────┐  │
│  │ Execute AST → Update NovaState → Wait Input →    │  │
│  │ Continue Execution                               │  │
│  └──────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
                         │
        ┌────────────────┼────────────────┐
        ▼                ▼                ▼
┌─────────────┐  ┌─────────────┐  ┌─────────────┐
│ Text Mode   │  │ Web Chat    │  │ Web VN      │
│ (CLI Debug) │  │ (Chat UI)   │  │ (Visual Nv) │
└─────────────┘  └─────────────┘  └─────────────┘
```

**Core Principles**:
- VM doesn't care about rendering, only maintains `NovaState`
- Renderers are "dumb" - they only present the state
- Multiple renderer implementations supported (CLI, Web, Native GUI, etc.)

## Tech Stack

| Component | Technology |
|-----------|------------|
| Language Standard | C++17 |
| Build System | CMake 3.16+ |
| Package Manager | vcpkg |
| Test Framework | GoogleTest |
| JSON Handling | nlohmann-json |
| WASM Support | Emscripten (optional) |

## Dependency Policy

**Minimal Dependencies** - Only essential libraries:

| Dependency | Purpose | Packaged? |
|------------|---------|-----------|
| nlohmann-json | JSON serialization | ✅ Header-only |
| GoogleTest | Unit testing | ❌ Dev only |

**Prohibited**:
- Boost (too bloated)
- LLVM (unless compiler-level complexity needed)
- Qt (use standard library alternatives)

## Documentation

- [NovaMark Syntax Specification](./docs/NovaMark%20语法规范.md) (Chinese)
- [Engine Architecture & Rendering Guide](./docs/NovaMark%20引擎架构与渲染指南.md) (Chinese)

## Roadmap

### v0.2 - Web Renderer
- [ ] WASM compilation support
- [ ] Chat Mode (chat interface)
- [ ] VN Mode (visual novel interface)
- [ ] Manual game package and save selection
- [ ] WebAudio support

### v0.3 - Enhanced Features
- [ ] Save thumbnails
- [ ] History log
- [ ] Auto/Skip modes
- [ ] Settings panel

### v1.0 - Production Ready
- [ ] Complete documentation
- [ ] Performance optimization
- [ ] Visual editor (optional)

## Contributing

Contributions, bug reports, and suggestions are welcome!

## License

MIT License

---

<div align="center">

**Tell your story with NovaMark** ✨

</div>
