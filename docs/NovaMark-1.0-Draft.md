# NovaMark v1.0 生产就绪功能草案

> **文档状态**：Draft · **目标版本**：v1.0.0 · **当前基线**：v0.2-patch2 (RC)
> **最后更新**：2026-05-04

---

## 1. 版本战略

### 1.1 发布节奏

| 阶段 | 版本号 | 内容 | 状态 |
|------|--------|------|------|
| Alpha | v0.1 | 语言核心 + 基础运行时 | 已完成 |
| 迭代 | v0.2-patch2 | 块级 choice、SFX、存档增强、平台模板 | 已完成（当前基线） |
| Release Candidate | **v1.0.0-rc.1** | 当前基线稳定化、状态契约修订、文档冻结 | 🎯 下一步 |
| Production | **v1.0.0** | 注册重载系统实现、性能基准、完整 API 覆盖 | 待定 |

**关键决策**：v1.0.0-rc.1 从当前 v0.2-patch2 基线发布，不含注册重载实现。注册重载系统是 1.0 正式版的实现目标，但接口规范在 rc 阶段冻结定义。

### 1.2 向后兼容承诺

从 v1.0.0-rc.1 起，NovaMark 做出以下兼容承诺：

1. **语法层面**：现有所有脚本语法（参见 §2）在 1.x 生命周期内不会移除或改变语义
2. **NVMP 格式**：已有 v1/v2 字节码包通过版本识别持续可加载
3. **C API**：已暴露的 C 函数签名不会 breaking change
4. **NovaState 契约**：已定义的字段不会删除，新增字段通过版本号区分（参见 §4.3）

---

## 2. 语法规范冻结

以下语法在 v1.0 中正式冻结，后续版本仅允许新增，不允许语义变更或移除。

### 2.1 文件结构

| 语法 | 说明 |
|------|------|
| `---` Front Matter | 项目元数据 |
| `#scene_xxx "标题"` | 场景声明 |
| `.label` | 标签定义 |

### 2.2 文本与对话

| 语法 | 说明 |
|------|------|
| `> 文本` | 旁白 |
| `角色: 文本` | 对话 |
| `角色[emotion]: 文本` | 情绪对话 |
| `{{expr}}` | 文本插值 |
| `{style:text}` | 内联样式 |

### 2.3 定义块

| 指令 | 说明 |
|------|------|
| `@var` | 变量定义 |
| `@char` | 角色定义 |
| `@item` | 物品定义 |

### 2.4 流程控制

| 语法 | 说明 |
|------|------|
| `-> scene` / `-> .label` | 跳转 |
| `@call` / `@return` | 子程序 |
| `@if` / `@else` / `@endif` | 条件分支 |
| `@check` / `@success` / `@fail` / `@endcheck` | 检定 |

### 2.5 状态变化

| 指令 | 说明 |
|------|------|
| `@set` | 变量赋值（支持表达式） |
| `@give` | 给予物品（支持表达式） |
| `@take` | 移除物品（支持表达式） |
| `@flag` | 设置标志 |
| `@ending` | 触发结局 |

### 2.6 媒体引用

| 指令 | 说明 |
|------|------|
| `@bg` | 背景图 |
| `@sprite` | 立绘 |
| `@bgm` | 背景音乐 |
| `@sfx` | 音效 |

### 2.7 选择

| 语法 | 说明 |
|------|------|
| `? 问题` | 选择块 |
| `- [选项] -> 目标` | 选项 |
| `- [选项] -> 目标 if 条件` | 条件选项 |
| `- [选项]` 后跟缩进块 | 块级选项（v0.2 新增，白名单：`@set`/`@flag`/`@give`/`@take`，末尾必须 `-> target`） |

### 2.8 内置函数

| 函数 | 说明 |
|------|------|
| `has_item(...)` | 物品持有检查 |
| `item_count(...)` | 物品数量查询 |
| `has_flag(...)` | 标志检查 |
| `has_ending(...)` | 结局检查 |
| `roll(...)` | 骰子表达式 |
| `random(min, max)` | 随机整数 |
| `chance(probability)` | 概率判定 |

---

## 3. 运行时契约修订

### 3.1 NovaState v2 契约

v1.0 对 NovaState 进行修订，新增以下标准字段：

```typescript
interface NovaState {
  // === 原有字段（不变） ===
  bg: string | null;
  bgm: string | null;
  sfx: Array<{ id: string; path: string; loop: boolean }>;
  sprites: Array<{ id: string; url: string; x: number; y: number; opacity: number; zIndex: number }>;
  dialogue: { isShow: boolean; name: string; text: string; color: string } | null;
  choices: Array<{ id: string; text: string; disabled: boolean }>;

  // === v1.0 新增字段 ===
  ending: {
    title: string | null;       // 结局标题
    reached: boolean;           // 是否已到达结局
  } | null;
  flags: string[];              // 已触发的标志列表
}
```

**兼容策略**：
- `ending` 和 `flags` 为新增字段，旧渲染器忽略即可
- 序列化快照中增加 `stateVersion: 2` 标识
- VM 在导出状态时始终填充新字段（`flags` 为空数组、`ending` 为 null 时仍序列化）

### 3.2 存档格式版本兼容

| 版本标识 | 字节码版本 | 说明 |
|----------|-----------|------|
| NVMP v1 | 1 | 初始版本 |
| NVMP v2 | 2 | 块级 choice、ending title |
| NVMP v3 | 3 | 注册重载扩展点（1.0 正式版） |

**加载策略**：VM 通过文件头版本标识自动选择对应的字节码解析路径。v1/v2/v3 包在同一 VM 实例中均可正常加载。

### 3.3 Playthrough-Only 导入

v1.0 正式支持仅导入通关记录（结局+标志），不恢复变量和背包状态：

| API | 说明 |
|-----|------|
| C++ | `vm.loadPlaythroughOnly(json/binary)` |
| C API | `nova_load_playthrough_json/binary()` |
| WASM | `nova_wasm_import_playthrough_json/binary()` |
| ArkTS | `NovaRuntime.importPlaythroughJson/Binary()` |
| JS | `importPlaythroughJson/Binary()` |

**语义**：替换式导入——将存档中的结局和标志覆盖到当前 VM 状态，不改变变量/背包/场景位置。

---

## 4. 注册重载系统（1.0 正式版核心特性）

### 4.1 设计目标

注册重载系统允许宿主在运行时扩展和覆写 NovaMark 的指令与函数，而无需等待引擎核心更新。

**核心场景**：

1. **扩展自定义指令**：宿主定义 `@custom_effect` 指令，在 VM 遇到时回调宿主代码
2. **覆写内置指令**：宿主覆写 `@bg` 的参数解析，添加自定义过渡参数
3. **扩展内置函数**：宿主注册 `custom_random()` 供脚本调用
4. **状态扩展**：宿主在 NovaState 中注册自定义字段

### 4.2 指令注册 API

```cpp
/// @brief 注册自定义指令处理器
/// @param name 指令名（不含@前缀），如 "custom_effect"
/// @param handler 指令执行回调
/// @param override 若为 true，将覆写同名内置指令
/// @return 注册是否成功
bool registerDirective(
    const std::string& name,
    DirectiveHandler handler,
    bool override = false
);

/// @brief 指令处理器签名
/// @param args 指令参数（经词法分析后的 token 列表）
/// @param state 当前 VM 状态（只读引用）
/// @return 指令执行结果（状态变更描述）
using DirectiveHandler = std::function<DirectiveResult(const std::vector<Token>& args, const NovaState& state)>;
```

**约束**：
- 自定义指令在词法分析阶段被识别为 `TokenType::Identifier` + `@` 前缀
- 语义分析阶段通过注册表判断指令合法性
- 未注册且非内置的 `@xxx` 产生编译期 warning（非 error），保持宽松扩展

### 4.3 函数注册 API

```cpp
/// @brief 注册自定义脚本函数
/// @param name 函数名
/// @param handler 函数执行回调
/// @param override 若为 true，将覆写同名内置函数
bool registerFunction(
    const std::string& name,
    FunctionHandler handler,
    bool override = false
);

/// @brief 函数处理器签名
/// @param args 函数参数列表
/// @return 函数返回值
using FunctionHandler = std::function<Value(const std::vector<Value>& args)>;
```

### 4.4 状态快照扩展

#### 4.4.1 字段扩展

宿主可通过注册机制向 NovaState 注入自定义字段：

```cpp
/// @brief 注册自定义状态字段
/// @param key 字段名（建议宿主使用命名空间前缀，如 "com.example.myfield"）
/// @param serializer 序列化/反序列化回调
/// @param defaultValue 默认值（用于快照恢复时字段缺失的场景）
void registerStateField(
    const std::string& key,
    StateFieldSerializer serializer,
    const nlohmann::json& defaultValue
);
```

**序列化契约**：
- 自定义字段在 JSON 快照中出现在 `extensions` 顶层键下
- 二进制快照中自定义字段追加在标准字段之后，以 TLV 格式编码
- 版本标识：`stateVersion: 3`（区分有无 extensions）

**JSON 示例**：
```json
{
  "stateVersion": 3,
  "bg": "room.png",
  "ending": null,
  "flags": [],
  "extensions": {
    "com.example.quest_log": { "activeQuests": ["main_01"] },
    "com.example.reputation": { "faction_a": 50 }
  }
}
```

#### 4.4.2 格式版本兼容

| stateVersion | 包含内容 | 兼容策略 |
|-------------|---------|---------|
| 1 | 原始 NovaState 字段 | 最小集，所有版本必须支持 |
| 2 | + `ending`、`flags` | 忽略新字段的旧渲染器不受影响 |
| 3 | + `extensions` | 未注册对应字段的宿主忽略 extensions |

**规则**：
- VM 导出的快照总是使用当前最高 stateVersion
- 导入时，VM 忽略无法识别的 extensions 字段（向前兼容）
- 缺失的 extensions 字段使用注册时的 defaultValue 填充（向后兼容）

### 4.5 覆写安全约束

1. **覆写需显式声明**：`override = true` 必须显式传递，默认不覆写
2. **覆写不可逆**：一旦覆写内置指令/函数，在同 VM 生命周期内无法恢复原始行为
3. **覆写日志**：所有覆写操作通过 VM 日志系统记录，便于调试
4. **语义兼容建议**：覆写内置指令时，建议保持参数格式的超集兼容（新参数可选，旧参数语义不变）

### 4.6 C API 暴露

```c
// 指令注册
bool nova_register_directive(
    void* vm,
    const char* name,
    NovaDirectiveCallback callback,
    void* user_data,
    bool override
);

// 函数注册
bool nova_register_function(
    void* vm,
    const char* name,
    NovaFunctionCallback callback,
    void* user_data,
    bool override
);

// 状态字段注册
bool nova_register_state_field(
    void* vm,
    const char* key,
    NovaStateFieldSerialize serialize,
    NovaStateFieldDeserialize deserialize,
    const char* default_value_json,
    void* user_data
);
```

### 4.7 WASM / JS API 暴露

```javascript
// 注册自定义指令
novaRegisterDirective("custom_effect", (args, state) => {
  return { type: "custom", action: "effect", params: args };
}, false);

// 覆写内置指令
novaRegisterDirective("bg", (args, state) => {
  // 自定义 @bg 处理，添加过渡参数
  return { type: "bg", path: args[0], transition: "fade", duration: 500 };
}, true);

// 注册自定义函数
novaRegisterFunction("custom_random", (args) => {
  return Math.floor(Math.random() * args[0]) + 1;
}, false);
```

---

## 5. 平台支持现状

### 5.1 渲染器模板

| 平台 | 仓库/目录 | 状态 |
|------|----------|------|
| Web (WASM) | `template/web/` | ✅ 稳定（Chat Mode + VN Mode） |
| HarmonyOS (ArkTS) | NovaFlow 仓库 | ✅ 稳定 |
| iOS | — | 📋 规划中 |
| Android | — | 📋 规划中 |
| Desktop (Terminal) | `nova-cli run` | ✅ 稳定（调试用途） |

### 5.2 工具链

| 工具 | 状态 |
|------|------|
| `nova-cli` (build/run/check) | ✅ 稳定 |
| AST → DAG 转换 | ✅ 已实现 |
| Creator 可视化编辑器 | 🔧 AST2DAG 已完成，双向编辑制作中 |
| Creator 打包 GUI | 🔧 鸿蒙端已有，其他平台制作中 |
| AST 快照导出 | ✅ 已实现 |
| AST → Source 回导 | ✅ 已实现 |

### 5.3 宿主职责边界（重申）

以下能力**明确属于宿主**，不属于 NovaMark 引擎：

| 能力 | 原因 |
|------|------|
| 自动步进 / 倍速 / 跳过 | `advance()` + `choose()` 已足够，宿主可轻松实现定时器 |
| 历史文本回溯 | 宿主缓存状态序列即可实现 |
| 状态快照缩略图 | 宿主基于当前状态自行渲染 |
| 打字机效果 | 由 `textConfig.defaultTextSpeed` 指导，宿主实现动画 |
| UI 布局 / HUD | 引擎只产出数据，不控制渲染 |

---

## 6. 1.0 正式版发布检查清单

### v1.0.0-rc.1 发布前

- [x] 语法全部稳定，323 个测试通过
- [x] NVMP v1/v2 兼容加载
- [x] WASM + C API + ArkTS 全链路可用
- [x] 块级 choice 语法
- [x] SFX 音效全链路
- [x] Playthrough-only 导入
- [x] AST 快照 + Source 回导
- [x] Ending title 全链路传递
- [ ] README 路线图更新
- [ ] 文档站内容与代码同步
- [ ] 中英文文档一致性检查

### v1.0.0 正式版发布前

- [ ] 注册重载系统实现（§4）
- [ ] NovaState v2 契约实现（§3.1）
- [ ] 状态快照 extensions 字段支持（§4.4）
- [ ] stateVersion 兼容机制实现（§4.4.2）
- [ ] 注册重载 C API / WASM / JS API 暴露（§4.6, §4.7）
- [ ] 性能基准测试建立
- [ ] 完整 API Reference 覆盖
- [ ] 注册重载功能测试覆盖

---

## 7. 术语表

| 术语 | 定义 |
|------|------|
| **宿主（Host）** | 嵌入 NovaMark VM 的上层应用程序，负责渲染和用户交互 |
| **渲染器（Renderer）** | 宿主中负责将 NovaState 绘制为可视界面的组件 |
| **状态契约（State Contract）** | VM 与宿主之间关于 NovaState 结构的协议 |
| **注册重载（Registry Override）** | 宿主在运行时扩展或覆写 VM 内置指令/函数的机制 |
| **Playthrough-Only** | 仅包含结局和标志的存档导入模式，不恢复完整游戏状态 |
| **NVMP** | NovaMark Package，编译后的二进制游戏包格式 |
| **stateVersion** | 状态快照的版本标识，用于序列化格式的向前/向后兼容 |
