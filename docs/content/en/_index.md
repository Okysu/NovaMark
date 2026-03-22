---
title: "NovaMark"
weight: 1
---

# NovaMark

NovaMark is a markup language and runtime engine designed for text games and visual novels.

## Features

- **Simple Syntax** - Markdown-like natural syntax, easy to learn and use
- **Chinese First** - Full support for Chinese identifiers and content
- **Cross-platform** - C++ core engine, supports multiple platforms
- **Multi-platform Rendering** - Dumb renderer architecture, same game runs on different platforms
- **Complete Game Logic** - Variables, items, conditional branches, dice checks

## Quick Start

```bash
# Install
git clone https://github.com/Okysu/NovaMark.git
cd NovaMark
cmake -B build -S .
cmake --build build

# Create new project
./build/src/cli/nova-cli init my-game

# Build
./build/src/cli/nova-cli build my-game

# Run
./build/src/cli/nova-cli run my-game/game.nvmp
```

## Documentation

- [Installation Guide](/docs/getting-started/installation/) - How to install and configure NovaMark
- [Quick Start](/docs/getting-started/quickstart/) - Create your first game in 5 minutes
- [For Creators](/docs/guide/) - Learn NovaMark through the creator-first guide
- [Quick Reference](/docs/reference/) - Look up syntax, state, API, and configuration
- [Platform Integration](/docs/platform/) - C API, WASM, Native, and runtime model

## Example

```nvm
#scene_intro "Prologue"

@bg forest.png
@bgm theme.mp3

> Moonlight filters through the trees...

林晓: Where... am I?

? What do you do?
- [Explore] -> .explore
- [Wait] -> .wait

.explore
林晓: There seems to be light ahead.
-> scene_tower
```

## License

MIT License
