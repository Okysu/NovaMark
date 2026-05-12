---
title: "Registry Override System"
weight: 3
---

# Registry Override System (v1.0)

The Registry Override System allows hosts to **extend and override** NovaMark's built-in functions and state fields at runtime, without modifying the engine core.

## Overview

| Capability | Status | Description |
|------|------|------|
| Custom function registration | ✅ Available | Scripts call `custom_func(args)`, VM dispatches via Registry |
| Built-in function override | ✅ Available | e.g. override `random()` to return a fixed value for testing |
| State field extensions | ✅ Available | Host injects custom fields into `NovaState.extensions`, auto-serialized with save data |
| Custom directive registration | ✅ Available | Scripts use `@custom_directive key:value`, VM dispatches via Registry |

## Usage Scenarios

### Scenario 1: Custom Functions

Call host-provided functionality from scripts:

```novamark
@var skill = custom_skill_level("stealth", 3)
@if skill >= 5
    > Your stealth skill is high enough to pass silently.
@endif
```

Host side (C++):

```cpp
vm.registry().registerFunction("custom_skill_level",
    [](const std::vector<VarValue>& args) -> VarValue {
        std::string name = std::get<std::string>(args[0]);
        double level = std::get<double>(args[1]);
        return getSkillFromDatabase(name) * level;
    }
);
```

### Scenario 2: Override Built-in Functions (Testing/Debugging)

```cpp
// Override random() to return a fixed value for deterministic testing
vm.registry().registerFunction("random",
    [](const std::vector<VarValue>&) -> VarValue { return 42.0; },
    true  // override = true
);
```

### Scenario 3: State Field Extensions

Hosts can inject renderer-specific state into `NovaState.extensions`, automatically saved/restored with save data:

```cpp
struct GameHud {
    double questProgress = 0.0;
    int notifications = 0;
};
GameHud hud;

vm.registry().registerStateField(
    "com.example.hud",
    [&]() -> nlohmann::json {
        return {{"questProgress", hud.questProgress},
                {"notifications", hud.notifications}};
    },
    [&](const nlohmann::json& j) {
        hud.questProgress = j.value("questProgress", 0.0);
        hud.notifications = j.value("notifications", 0);
    },
    nlohmann::json::object()
);
```

Resulting save JSON:

```json
{
  "stateVersion": 3,
  "bg": "forest.png",
  "flags": ["met_king"],
  "extensions": {
    "com.example.hud": {
      "questProgress": 75.5,
      "notifications": 3
    }
  }
}
```

### Scenario 4: Custom Directives

Use custom `@` directives in scripts, with `key:value` pair arguments:

```novamark
#scene_battle "Battle"
@custom_skill_check difficulty:hard target:boss
> You unleashed a special skill!
```

Host side (C++):

```cpp
vm.registry().registerDirective("custom_skill_check",
    [&](const std::string& name,
        const std::vector<std::pair<std::string, std::string>>& args,
        NovaState& state) -> DirectiveResult {
        for (const auto& [key, value] : args) {
            if (key == "difficulty") { /* set difficulty */ }
            else if (key == "target") { /* set target */ }
        }
        state.bg = "battle_effect.png";
        return {true, false};
    }
);
```

**Argument format**: Supports `key:value` pairs and positional tokens:

```novamark
@custom_cmd positional_arg key:value "quoted value" 42
```

## C API

### nova_register_function

```c
int nova_register_function(NovaVM* vm, const char* name,
    NovaFunctionCallback callback, void* userData, int override_);
```

Register a custom function.

**Parameters**:
- `name` - Function name (invoked as `name(args)` in scripts)
- `callback` - Function callback (arguments passed via JSON string array)
- `userData` - User data pointer
- `override_` - Non-zero allows overriding a built-in function of the same name

**Returns**: 1 on success, 0 on failure.

**Callback signature**:

```c
typedef int (*NovaFunctionCallback)(
    const char* name,
    size_t argCount,
    const char* const* argJsons,   // Each argument as a JSON string
    char** resultJson,              // Output: return value as JSON string (caller must free via nova_string_free)
    void* userData
);
```

### nova_register_state_field

```c
int nova_register_state_field(NovaVM* vm, const char* key,
    NovaStateFieldSerializeCb serialize,
    NovaStateFieldDeserializeCb deserialize,
    const char* defaultValueJson,
    void* userData);
```

Register a custom state field.

**Parameters**:
- `key` - Field name (recommended: `"com.example.field"` format)
- `serialize` - Serialization callback, returns JSON string
- `deserialize` - Deserialization callback, receives JSON string
- `defaultValueJson` - Default value as JSON string

## Web / JS API

```javascript
// Register a custom function
renderer.registerFunction("custom_skill_check", (args) => {
    const [skillName, level] = args;
    return db.getSkill(skillName) * level;
}, false);

// Look up a registered function
const fn = renderer.findCustomFunction("custom_skill_check");
```

## Override Safety Constraints

1. **Explicit override required**: `override = true` must be explicitly passed
2. **Override is irreversible**: Once a built-in function is overridden, the original behavior cannot be restored within the same VM lifecycle
3. **Override logging**: Override operations log a Warning to `stderr`
4. **Semantic compatibility**: When overriding built-in functions, maintain superset compatibility of parameter formats

## Semantic Analysis Behavior

The Registry Override System follows a **loose extension** policy:

- Function calls not in the built-in function list produce a **Warning** instead of an Error
- This allows hosts to register functions at runtime without blocking compilation
- If a function is also not registered at runtime, the VM returns `0.0` (no crash)

## Built-in Function List

The following function names are recognized as built-in; override with `override = true`:

| Function | Parameters |
|------|------|
| `has_ending(id)` | 1 string |
| `has_flag(id)` | 1 string |
| `has_item(id)` | 1 string |
| `item_count(id)` | 1 string |
| `var(name)` | 1 string |
| `roll(expr)` | 1 dice expression |
| `random(min, max)` | 2 numbers |
| `chance(probability)` | 1 number |
