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

This interface now acts as a **unified presentation snapshot**:

- it preserves all previously exposed fields
- it adds renderer-facing presentation fields
- hosts no longer need to reconstruct the full visual state from many individual getters in the common case

In other words, hosts are now encouraged to consume this JSON snapshot first, while the old fine-grained getters remain available for compatibility and low-level optimization.

Example return structure:

```json
{
  "status": 0,
  "runtimeStateVersion": 12,
  "runtimeStateChangeFlags": 0,
  "currentScene": "scene_start",
  "currentLabel": "",
  "textConfig": {
    "defaultFont": "SourceHanSansCN-Regular.ttf",
    "defaultFontSize": 28,
    "defaultTextSpeed": 60,
    "baseBgPath": "bg/",
    "baseSpritePath": "sprites/",
    "baseAudioPath": "audio/"
  },
  "bg": "room.png",
  "bgTransition": "fade",
  "bgm": "theme.mp3",
  "bgmVolume": 1.0,
  "bgmLoop": true,
  "sprites": [
    {
      "id": "Alice",
      "url": "alice_happy.png",
      "position": "left",
      "opacity": 1.0,
      "zIndex": 1
    }
  ],
  "sfx": [
    {
      "id": "click",
      "path": "click.wav",
      "loop": false,
      "volume": 0.5
    }
  ],
  "dialogue": {
    "isShow": true,
    "speaker": "Alice",
    "text": "Hello world",
    "segments": [
      { "text": "Hello ", "style": "" },
      { "text": "world", "style": "accent" }
    ],
    "emotion": "happy",
    "color": "#90EE90"
  },
  "choice": {
    "isShow": true,
    "question": "Is your name Alice?",
    "questionSegments": [
      { "text": "Is your ", "style": "" },
      { "text": "name", "style": "accent" },
      { "text": " Alice?", "style": "" }
    ],
    "options": [
      {
        "id": "0",
        "text": "Continue",
        "target": ".next",
        "disabled": false,
        "segments": [
          { "text": "Continue", "style": "" }
        ]
      }
    ]
  },
  "endingId": "good_end",
  "endingTitle": "Good Ending",
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
      "description": "Guardian of the forest",
      "sprites": {
        "happy": "spirit_happy.png"
      }
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

### status / runtimeStateVersion / runtimeStateChangeFlags

| Field | Type | Description |
|-------|------|-------------|
| `status` | number | Runtime status: 0=running, 1=waiting for choice, 2=ended |
| `runtimeStateVersion` | number | Runtime state version counter for cache/incremental refresh decisions |
| `runtimeStateChangeFlags` | number | Bit flags describing what changed since the previous snapshot |

### textConfig

| Field | Type | Description |
|-------|------|-------------|
| `defaultFont` | string | Default font file |
| `defaultFontSize` | number | Default font size in pixels |
| `defaultTextSpeed` | number | Default typewriter speed (chars/sec) |
| `baseBgPath` | string | Base path for background assets |
| `baseSpritePath` | string | Base path for sprite assets |
| `baseAudioPath` | string | Base path for audio assets |

### Presentation fields

| Field | Type | Description |
|-------|------|-------------|
| `bg` | string | Current background asset |
| `bgTransition` | string | Background transition hint |
| `bgm` | string | Current background music asset |
| `bgmVolume` | number | Current BGM volume |
| `bgmLoop` | boolean | Whether current BGM loops |
| `sprites` | array | Current sprite presentation state |
| `sfx` | array | Current sound effect presentation queue/state |
| `endingId` | string | Current ending id if the game has ended |
| `endingTitle` | string | Current ending title if available |

### dialogue

| Field | Type | Description |
|-------|------|-------------|
| `text` | string | Flattened plain text, kept for backward compatibility |
| `segments` | array | Structured text segments; `style == ""` means plain text |
| `emotion` | string | Current emotion tag |
| `color` | string | Current dialogue color |

### choice

| Field | Type | Description |
|-------|------|-------------|
| `question` | string | Flattened plain text choice question |
| `questionSegments` | array | Structured segments for the choice question |
| `options[].text` | string | Flattened plain text option label |
| `options[].segments` | array | Structured segments for each choice option |

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
- Character colors, descriptions, and sprite mappings

## Recommended Consumption Pattern

### Prefer the unified snapshot

For most hosts, the recommended path is to consume `nova_export_runtime_state_json` directly instead of rebuilding the full presentation state from many individual getters:

1. Read the full presentation snapshot once
2. Render background, BGM, sprites, dialogue, choices, and status UI from it
3. Use `dialogue.segments`, `choice.questionSegments`, and `choice.options[].segments` when structured text is needed

### When to keep using granular getters

Granular getters still make sense when:

- you only need one field
- you want to avoid JSON parsing
- you are implementing a very low-level bridge or incremental optimization path

All legacy getters remain available and unchanged.

## Renderer Responsibilities

NovaMark only outputs state; it doesn't decide specific UI layout for Web, Native, or CLI. Clients should:

1. Read runtime state snapshots
2. Decide how to display variables, inventory, and other UI elements
3. Render dialogue using `dialogue.color` or character definition colors
4. Render local styling from `dialogue.segments` / `choice.*Segments`
5. Control typewriter speed based on `textConfig.defaultTextSpeed`

## Web Debugging

In Web templates, use directly:

```js
novaDebug.runtimeState()
novaDebug.snapshot().runtimeState
```

The template-side `NovaRenderer` also provides:

```js
renderer.getRuntimeState()
renderer.getPresentationState()
```

Where:

- `getRuntimeState()` is the legacy name kept for compatibility
- `getPresentationState()` is the newer semantic alias emphasizing that this is a unified presentation snapshot

Both currently return the same structure.

## AST Snapshot Export

In addition to runtime state snapshots, NovaMark also provides **AST snapshot export** for debugging parse results, building Creator/editor tooling, and verifying the structure produced from scripts.

The difference is:

- **Runtime state snapshot**: describes where the game is now, what variables contain, and what the UI should display
- **AST snapshot**: describes the syntax tree produced after parsing the scripts

### C++ export APIs

```cpp
std::string export_ast_snapshot_string(const ProgramNode* program);
std::string export_ast_snapshot_string_from_scripts(const std::vector<MemoryScript>& scripts);
std::string export_ast_snapshot_string_from_path(const std::string& path);
```

### C API export APIs

```c
char* nova_export_ast_snapshot_from_path(const char* path);
char* nova_export_ast_snapshot_from_scripts(const NovaMemoryScript* scripts, size_t count);
```

### Common use cases

- Debugging parser output
- Verifying the final merged `Program` structure in multi-script projects
- Providing AST browsing in Creator / editor tools
- Exporting stable JSON for test or CI comparisons

### Output format

AST snapshots are currently exported as **JSON strings**. The top-level shape looks like this:

```json
{
  "version": 1,
  "root": {
    "type": "Program",
    "children": []
  }
}
```

For nodes containing interpolated text, the snapshot includes an `InterpolatedText` structure with segment metadata such as:

- `PlainText`: normal text segment
- `Interpolation`: a `{{expr}}` segment
- `InlineStyle`: an inline `{style:text}` segment

This lets tools understand not only the original text, but also which parts are expressions and which parts are styling markers. The same segment model is now also used at runtime through `dialogue.segments`, `choice.questionSegments`, and `choice.options[].segments`.

## Save Format

NovaMark recommends this responsibility split:

- **Official save files**: Binary format, for production use
- **JSON snapshots**: For debugging, testing, Web/WASM toolchains

JSON remains an important development format but is no longer recommended as the official player save format.
