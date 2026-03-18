---
title: "Quick Start"
weight: 2
---

# Quick Start

This guide will help you create your first NovaMark game in 5 minutes.

## Create Project

```bash
./build/src/cli/nova-cli init my-first-game
```

This creates the following directory structure:

```
my-first-game/
├── project.yaml      # Project configuration
├── scripts/
│   └── main.nvm      # Main script
├── assets/
│   ├── bg/           # Background images
│   ├── sprites/      # Character sprites
│   └── audio/        # Audio files
└── README.md
```

## Write Your First Scene

Edit `scripts/main.nvm`:

```nvm
---
title: My First Game
author: Your Name
---

#scene_start "Start"

@bg room.png

> This is an ordinary room.

林晓: Hello, world!

? What do you do?
- [Look around] -> .look_around
- [Leave room] -> .leave

.look_around
> There is a table and a chair in the room.
-> scene_start

.leave
> You walk out of the room.
@ending the_end
```

## Build Game

```bash
./build/src/cli/nova-cli build my-first-game
```

This generates `my-first-game/game.nvmp` file.

## Run Game

```bash
./build/src/cli/nova-cli run my-first-game/game.nvmp
```

## Next Steps

- [Syntax Reference](../../syntax/) - Learn the complete syntax
- [Complete Examples](https://github.com/Okysu/NovaMark/tree/main/examples) - View more examples
