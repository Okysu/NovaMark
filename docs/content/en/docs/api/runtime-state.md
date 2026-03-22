---
title: "Runtime State & Snapshot API"
weight: 2
---

# Runtime State & Snapshot API

NovaMark exports runtime state to clients through a unified interface. Renderers, GUIs, and debugging tools should read variables, inventory, text configuration, and definition metadata from this interface rather than relying on inline UI description syntax.

NovaMark no longer directly controls HUD or other client interface elements through scripts. Developers should decide UI layout and refresh timing based on runtime data such as variables, inventory, and dialogue.

## Runtime State JSON

Current Web/WASM export interface:

```c
const char* nova_export_runtime_state_json(void* vm, size_t* outSize);
```

Example return structure:

```json
{
  "status": 0,
  "currentScene": "scene_start",
  "currentLabel": "",
  "textConfig": {
    "defaultFont": "SourceHanSansCN-Regular.ttf",
    "defaultFontSize": 28,
    "defaultTextSpeed": 60
  },
  "variables": {
    "numbers": { "hp": 100, "gold": 20 },
    "strings": { "playerName": "Alice" },
    "bools": { "metSpirit": true }
  },
  "inventory": {
    "healing_potion": 2
  },
  "itemDefinitions": {
    "healing_potion": {
      "id": "healing_potion",
      "name": "Healing Potion",
      "description": "Restore HP"
    }
  },
  "characterDefinitions": {
    "ForestSpirit": {
      "id": "ForestSpirit",
      "color": "#90EE90",
      "description": "Guardian of the forest"
    }
  },
  "inventoryItems": [
    {
      "id": "healing_potion",
      "name": "Healing Potion",
      "description": "Restore HP",
      "count": 2
    }
  ]
}
```

## Field Reference

### textConfig

| Field | Type | Description |
|-------|------|-------------|
| `defaultFont` | string | Default font file |
| `defaultFontSize` | number | Default font size in pixels |
| `defaultTextSpeed` | number | Default typewriter speed (chars/sec) |

### variables

| Field | Type | Description |
|-------|------|-------------|
| `numbers` | object | Numeric variables (key → value) |
| `strings` | object | String variables (key → value) |
| `bools` | object | Boolean variables (key → value) |

### inventory / inventoryItems

| Field | Type | Description |
|-------|------|-------------|
| `inventory` | object | Raw item ID → count mapping |
| `inventoryItems` | array | GUI-friendly item array with name, description, count |

### itemDefinitions / characterDefinitions

Static definition metadata for GUI lookup:

- Item display names and descriptions
- Character colors and descriptions

## Renderer Responsibilities

NovaMark only outputs state; it doesn't decide specific UI layout for Web, Native, or CLI. Clients should:

1. Read runtime state
2. Decide how to display variables, inventory, and other UI elements
3. Render dialogue using `dialogue.color` or character definition colors
4. Control typewriter speed based on `textConfig.defaultTextSpeed`

## Web Debugging

In Web templates, use directly:

```js
novaDebug.runtimeState()
novaDebug.snapshot().runtimeState
```

For inspecting current variables, inventory, and definition metadata.

## Save Format

NovaMark recommends this responsibility split:

- **Official save files**: Binary format, for production use
- **JSON snapshots**: For debugging, testing, Web/WASM toolchains

JSON remains an important development format but is no longer recommended as the official player save format.
