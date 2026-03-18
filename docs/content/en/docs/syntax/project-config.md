---
title: "project.yaml Configuration"
weight: 3
---

# project.yaml Configuration

`project.yaml` is the NovaMark project configuration file that defines project structure, build options, and runtime settings.

## File Location

Place it in the project root:

```
my-game/
├── project.yaml        # Project configuration
├── scripts/            # Scripts directory
│   └── *.nvm
└── assets/             # Assets directory
    ├── bg/
    ├── sprites/
    └── audio/
```

## Complete Configuration Example

```yaml
name: mist_forest
title: Mist Forest
version: 1.0.0
author: NovaMark Team

entry_scene: scene_intro

scripts_path: scripts
assets_path: assets

default_font: fonts/SourceHanSansCN-Regular.ttf
default_font_size: 28
default_text_speed: 60
```

## Field Descriptions

### Basic Information

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | String | Yes | Project name (internal identifier, used for .nvmp filename) |
| `title` | String | No | Game display title (defaults to name) |
| `version` | String | No | Version number |
| `author` | String | No | Author/team name |

### Project Structure

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `scripts_path` | String | `scripts` | Script files directory |
| `assets_path` | String | `assets` | Asset files directory |

### Runtime Configuration

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `entry_scene` | String | First scene | Game entry scene ID |
| `default_font` | String | System font | Default font file path |
| `default_font_size` | Number | 24 | Default font size |
| `default_text_speed` | Number | 50 | Text display speed (chars/sec) |

## Project Directory Structure

Standard structure created by `nova-cli init my-game`:

```
my-game/
├── project.yaml            # Project configuration
├── scripts/
│   ├── 00_init.nvm         # Initialization scripts (character, item definitions)
│   └── 01_main.nvm         # Main script
├── assets/
│   ├── bg/                 # Background images (.png, .jpg)
│   ├── sprites/            # Character sprites (.png)
│   └── audio/              # Audio files (.mp3, .ogg)
└── README.md
```

## Multiple Script Files

Project mode supports multiple `.nvm` files, merged in filename order:

```yaml
scripts_path: scripts
```

```
scripts/
├── 00_characters.nvm    # Character definitions
├── 01_items.nvm         # Item definitions
├── 02_intro.nvm         # Prologue
├── 03_chapter1.nvm      # Chapter 1
└── 04_chapter2.nvm      # Chapter 2
```

**Merge Rules**:
- Sorted by filename dictionary order
- All scripts merged into one AST
- Scene definitions can reference across files

## Build Commands

```bash
# Build project (auto-finds project.yaml)
nova-cli build

# Build specific project directory
nova-cli build ./my-game

# Specify output file
nova-cli build ./my-game -o release/game.nvmp

# Show verbose output
nova-cli build ./my-game -v
```

## Configuration Validation

Use the `check` command to validate configuration:

```bash
nova-cli check ./my-game
```

Checks:
- `project.yaml` format correctness
- `scripts_path` directory exists
- All `.nvm` files have correct syntax
- Scene jump references exist
