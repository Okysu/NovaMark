---
title: "注册重载系统"
weight: 3
---

# 注册重载系统（v1.0）

注册重载系统允许宿主在运行时**扩展和覆写** NovaMark 的内置函数与状态字段，无需修改引擎核心。

## 概述

| 能力 | 状态 | 说明 |
|------|------|------|
| 自定义函数注册 | ✅ 可用 | 脚本调用 `custom_func(args)`，VM 通过 Registry 查找执行 |
| 覆写内置函数 | ✅ 可用 | 如覆写 `random()` 返回固定值用于测试 |
| 状态字段扩展 | ✅ 可用 | 宿主注入自定义字段到 `NovaState.extensions`，随存档序列化 |
| 自定义指令注册 | ✅ 可用 | 脚本中 `@custom_directive key:value`，VM 查 Registry 执行 |

## 使用场景

### 场景 1: 自定义函数

在脚本中调用宿主提供的功能：

```novamark
@var skill = custom_skill_level("stealth", 3)
@if skill >= 5
    > 你的潜行技能已足够无声地通过。
@endif
```

宿主侧（C++）：

```cpp
vm.registry().registerFunction("custom_skill_level",
    [](const std::vector<VarValue>& args) -> VarValue {
        // args[0] = 技能名, args[1] = 等级
        std::string name = std::get<std::string>(args[0]);
        double level = std::get<double>(args[1]);
        // 从宿主数据库查询技能等级
        return getSkillFromDatabase(name) * level;
    }
);
```

### 场景 2: 覆写内置函数（测试/调试用）

```cpp
// 覆写 random() 为固定值，方便编写确定性测试
vm.registry().registerFunction("random",
    [](const std::vector<VarValue>&) -> VarValue { return 42.0; },
    true  // override = true
);
```

### 场景 3: 状态字段扩展

宿主可将渲染器特有的状态注入 `NovaState.extensions`，随存档自动保存/恢复：

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

存档 JSON 中的输出：

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

### 场景 4: 自定义指令

在脚本中使用自定义 `@` 指令，参数以 `key:value` 对传递：

```novamark
#scene_battle "战斗"
@custom_skill_check difficulty:hard target:boss
> 你发动了特殊技能！
```

宿主侧（C++）：

```cpp
vm.registry().registerDirective("custom_skill_check",
    [&](const std::string& name,
        const std::vector<std::pair<std::string, std::string>>& args,
        NovaState& state) -> DirectiveResult {
        for (const auto& [key, value] : args) {
            if (key == "difficulty") {
                // 设置难度
            } else if (key == "target") {
                // 设置目标
            }
        }
        // 可以修改 NovaState 来影响渲染
        state.bg = "battle_effect.png";
        return {true, false};  // handled, no re-advance needed
    }
);
```

**参数格式**：支持 `key:value` 对有值参数和独立的标识符参数：

```novamark
@custom_cmd positional_arg key:value "quoted value" 42
```

## C API

### nova_register_function

```c
int nova_register_function(NovaVM* vm, const char* name,
    NovaFunctionCallback callback, void* userData, int override_);
```

注册自定义函数。

**参数**:
- `name` - 函数名（脚本中以 `name(args)` 调用）
- `callback` - 函数回调（参数通过 JSON 字符串数组传递）
- `userData` - 用户数据指针
- `override_` - 非 0 表示可覆写同名内置函数

**返回值**: 成功返回 1，失败返回 0。

**回调签名**:

```c
typedef int (*NovaFunctionCallback)(
    const char* name,
    size_t argCount,
    const char* const* argJsons,   // 每个参数为 JSON 字符串
    char** resultJson,              // 输出：返回值 JSON 字符串（调用方需 nova_string_free）
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

注册自定义状态字段。

**参数**:
- `key` - 字段名（推荐 `"com.example.field"` 格式）
- `serialize` - 序列化回调，返回 JSON 字符串
- `deserialize` - 反序列化回调，接收 JSON 字符串
- `defaultValueJson` - 默认值 JSON 字符串

## Web / JS API

```javascript
// 注册自定义函数
renderer.registerFunction("custom_skill_check", (args) => {
    const [skillName, level] = args;
    return db.getSkill(skillName) * level;
}, false);

// 查找已注册的函数
const fn = renderer.findCustomFunction("custom_skill_check");
```

## 覆写安全约束

1. **覆写需显式声明**：`override = true` 必须显式传递
2. **覆写不可逆**：一旦覆写内置函数，同 VM 生命周期内无法恢复
3. **覆写日志**：覆写操作通过 `stderr` 输出 Warning 日志
4. **语义兼容建议**：覆写内置函数时，建议保持参数格式的超集兼容

## 语义分析行为

注册重载系统采用**宽松扩展**策略：

- 对内置函数列表中不存在的函数调用，语义分析器产生 **Warning** 而非 Error
- 这允许宿主在运行时注册函数，编译期不阻塞
- 如果函数在运行时也未注册，VM 返回 `0.0`（不会崩溃）

## 内置函数列表

以下函数名被识别为内置函数，使用 `override = true` 可覆写：

| 函数 | 参数 |
|------|------|
| `has_ending(id)` | 1 个字符串 |
| `has_flag(id)` | 1 个字符串 |
| `has_item(id)` | 1 个字符串 |
| `item_count(id)` | 1 个字符串 |
| `var(name)` | 1 个字符串 |
| `roll(expr)` | 1 个骰子表达式 |
| `random(min, max)` | 2 个数值 |
| `chance(probability)` | 1 个数值 |
