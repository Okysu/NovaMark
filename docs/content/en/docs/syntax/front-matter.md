---
title: "Front Matter Metadata"
weight: 2
---

# Front Matter Metadata

Front Matter is a YAML-formatted metadata block at the beginning of a file, used to define basic game information.

## Syntax

```nvm
---
key: value
key2: "string value"
---

# Game content starts here...
```

Starts and ends with `---`, with YAML key-value pairs in between.

## Supported Fields

### Basic Information Fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `title` | String | No | Game title |
| `name` | String | No | Game name (used for package filename) |
| `author` | String | No | Author name |
| `version` | String | No | Version number, e.g., `1.0.0` |
| `description` | String | No | Game description |

### Resource Path Fields

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `base_bg_path` | String | `assets/bg/` | Background image base path |
| `base_sprite_path` | String | `assets/sprites/` | Sprite image base path |
| `base_audio_path` | String | `assets/audio/` | Audio file base path |

### Runtime Configuration Fields

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `entry_scene` | String | First scene | Entry scene ID |
| `default_font` | String | System default | Default font |
| `default_font_size` | Number | 24 | Default font size |
| `default_text_speed` | Number | 50 | Default text speed (chars/sec) |

## Complete Example

```nvm
---
title: Mist Forest
name: mist_forest
author: NovaMark Team
version: 1.0.0
description: An adventure story about being lost and finding your way
entry_scene: scene_intro
base_bg_path: assets/backgrounds/
base_sprite_path: assets/characters/
base_audio_path: assets/sounds/
default_font: SourceHanSansCN-Regular.ttf
default_font_size: 28
default_text_speed: 60
---

#scene_intro "Prologue"

@bg forest_mist.png
> The morning mist shrouds the forest...
```

## Single File vs Project Configuration

### Single File Mode

When building a single `.nvm` file with `nova-cli build game.nvm`:

- Front Matter metadata is read
- If no Front Matter, the filename is used as the game name

### Project Mode

When building a project with `project.yaml`:

- `project.yaml` configuration takes priority over Front Matter
- Front Matter resource path settings are preserved
