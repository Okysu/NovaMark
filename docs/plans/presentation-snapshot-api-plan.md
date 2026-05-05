# 统一 Presentation Snapshot API 实施计划

> 状态: 规划稿 · 只读分析，未进入实施

---

## 1. 现状分析

### 1.1 现有 API 概览

当前 NovaMark 向渲染层暴露状态有三种渠道：

| 渠道 | 函数 / 入口 | 覆盖范围 | 备注 |
|------|------------|---------|------|
| **JSON 导出** | `nova_export_runtime_state_json()` | status, currentScene/Label, textConfig, variables, inventory, definitions, dialogue, choice | **缺少 bg/bgm/sprites/sfx/ending 等展示层字段** |
| **WASM 粒度 getter** | `nova_get_bgm()`, `nova_get_sprite_x()`, `nova_get_dialogue_segment_text()` 等 ~40 个函数 | 覆盖所有字段（包括粒度片段） | 调用昂贵，单帧需数十次 FFI 边界穿越 |
| **C API 结构体** | `nova_get_state()` → `NovaState` 结构体 | bg, bgm, sprites, sfx, dialogue, choices | C 结构体，WASM 侧无直接映射 |

### 1.2 关键缺口

`nova_export_runtime_state_json()` 的当前输出**缺少以下展示层字段**：

```
缺失字段                现有粒度 Getter              来源结构
──────────────────────  ─────────────────────────  ─────────────────
bg                       nova_get_bgm()              NovaState::bg
bgTransition             nova_get_bg_transition()    NovaState::bgTransition
bgm                      nova_get_bgm()              NovaState::bgm
bgmVolume                nova_get_bgm_volume()       NovaState::bgmVolume
bgmLoop                  nova_get_bgm_loop()         NovaState::bgmLoop
sfx[]                    nova_get_sfx_count/id/...   NovaState::sfx[]
sprites[]                nova_get_sprite_x/y/...     NovaState::sprites[]
endingId                 nova_get_ending_id()        NovaState::ending
endingTitle              nova_get_ending_title()     NovaState::endingTitle
runtimeStateVersion      nova_get_runtime_state_...  NovaVM::m_runtimeStateVersion
runtimeStateChangeFlags  nova_consume_runtime_...    NovaVM::m_runtimeStateChangeFlags
```

---

## 2. 设计决策：扩展 vs 新建

### 结论：**扩展 `nova_export_runtime_state_json`**

不在 `nova_wasm_api.h` 中新增独立函数。理由：

| 维度 | 扩展现有 | 新建独立函数 |
|------|---------|-------------|
| **向后兼容** | ✅ 新增字段，旧客户端忽略即可 | ✅ 旧函数不变 |
| **接入摩擦** | ✅ JS `getRuntimeState()` 自动获得新字段 | ❌ 需要 JS 层新增方法调用 |
| **心智模型** | ✅ "一个调用拿全量状态" | ❌ "该用哪个函数？" |
| **API 膨胀** | ✅ 零增长 | ❌ +1 导出符号 |
| **版本管理** | ✅ 单版本号跟踪所有变更 | ❌ 两个函数的版本号可能不同步 |
| **渲染器工作量** | ✅ web/native/cli 各渲染器只需改解析侧 | ❌ 各渲染器需新增调用入口 |

### 不拆分的理由

有人认为"运行时状态"和"展示快照"应该分开。但在 NovaMark 架构中，渲染器就是哑的——它不需要区分"内部状态"和"展示状态"，**它只需要知道"现在画什么"**。合并到一个 JSON 中让渲染器一次解析、全量使用，更符合"哑渲染器"的架构原则。

---

## 3. 实施步骤

### Step 1: 扩展 `nova_export_runtime_state_json` 输出（核心修改）

**文件**: `src/renderer/wasm/nova_wasm_exports.cpp`  
**函数**: `nova_export_runtime_state_json()` (行 526-659)

在 `json["choice"]` 块之后、`json.dump()` 之前，插入以下新字段：

```cpp
// ===== 新增：展示层字段（v2 扩展）=====

// BGM 状态
if (state.bgm.has_value()) {
    json["bgm"] = *state.bgm;
}
json["bgmVolume"] = state.bgmVolume;
json["bgmLoop"] = state.bgmLoop;

// BG 状态
if (state.bg.has_value()) {
    json["bg"] = *state.bg;
}
if (state.bgTransition.has_value()) {
    json["bgTransition"] = *state.bgTransition;
}

// 精灵数组
json["sprites"] = nlohmann::json::array();
for (const auto& sprite : state.sprites) {
    nlohmann::json s;
    s["id"] = sprite.id;
    if (sprite.url) s["url"] = *sprite.url;
    if (sprite.x) s["x"] = *sprite.x;
    if (sprite.y) s["y"] = *sprite.y;
    if (sprite.position) s["position"] = *sprite.position;
    if (sprite.opacity) s["opacity"] = *sprite.opacity;
    if (sprite.zIndex) s["zIndex"] = *sprite.zIndex;
    json["sprites"].push_back(std::move(s));
}

// 音效数组
json["sfx"] = nlohmann::json::array();
for (const auto& sfx : state.sfx) {
    json["sfx"].push_back({
        {"id", sfx.id},
        {"path", sfx.path},
        {"volume", sfx.volume},
        {"loop", sfx.loop}
    });
}

// 结局状态
if (state.ending.has_value()) {
    json["endingId"] = *state.ending;
}
if (state.endingTitle.has_value()) {
    json["endingTitle"] = *state.endingTitle;
}

// 运行时状态版本和变更标记（用于渲染器增量更新优化）
json["runtimeStateVersion"] = nova_vm->runtimeStateVersion();
json["runtimeStateChangeFlags"] = nova_vm->consumeRuntimeStateChangeFlags();
```

> ⚠️ **注意**: 上面的 `consumeRuntimeStateChangeFlags()` 有副作用（消费标志位）。
> `nova_export_runtime_state_json` 目前被文档标记为只读快照，引入副作用需要文档说明。
> 替代方案：读取但不消费，让调用者单独调用 `nova_consume_runtime_state_change_flags`。
> **推荐方案**：改为读取 `m_runtimeStateChangeFlags` 但不消费（如果 VM 不提供非消费读取，则添加到 VM 接口）。

### 新 JSON 输出示例

```json
{
  "status": 1,
  "currentScene": "scene_forest",
  "currentLabel": "meeting",
  "textConfig": { "...": "..." },
  "variables": { "...": "..." },
  "inventory": { "...": "..." },
  "inventoryItems": [ "..." ],
  "itemDefinitions": { "...": "..." },
  "characterDefinitions": { "...": "..." },

  "dialogue": {
    "isShow": true,
    "speaker": "林晓",
    "text": "你好，{em:世界}！",
    "segments": [
      { "text": "你好，", "style": "" },
      { "text": "世界", "style": "em" },
      { "text": "！", "style": "" }
    ],
    "emotion": "happy",
    "color": "#4A90D9"
  },

  "choice": { "..." : "..." },

  "bg": "forest_day.png",
  "bgTransition": "fade",
  "bgm": "forest_ambient.mp3",
  "bgmVolume": 0.7,
  "bgmLoop": true,

  "sprites": [
    { "id": "xiaoming", "url": "xiaoming_happy.png", "x": "100", "y": "200", "opacity": 1.0, "zIndex": 10 }
  ],

  "sfx": [
    { "id": "footstep", "path": "step_grass.mp3", "volume": 0.5, "loop": false }
  ],

  "endingId": null,
  "endingTitle": null,

  "runtimeStateVersion": 42,
  "runtimeStateChangeFlags": 3
}
```

### Step 2: VM 接口补充（如果需要）

**文件**: `include/nova/vm/vm.h`

如果决定让 `runtimeStateChangeFlags` 可被读取而不消费，添加：

```cpp
/// @brief 读取运行时状态变更标志（不消费）
int runtimeStateChangeFlags() const { return m_runtimeStateChangeFlags; }
```

### Step 3: WASM API 头文件声明（无变化）

**文件**: `include/nova/renderer/nova_wasm_api.h`

不需要任何声明变更。`nova_export_runtime_state_json` 签名不变，只是输出内容更多。所有现有 WASM getter 声明保持不变。

### Step 4: WASM 桥接层确认（无变化）

**文件**: `template/web/src/nova_wasm_main.cpp`

`nova_wasm_export_runtime_state_json()` 已委托给 `nova_export_runtime_state_json()`，不需要修改。行 189-191 无需改动。

### Step 5: JS 渲染器层 — 可选优化

**文件**: `template/web/nova_renderer.js`

**现有 `getRuntimeState()`** 自动获得新字段（因为它解析 JSON）。**不需要任何修改**。

可选的便利方法（非必需，但建议添加）：

```javascript
/// @brief 获取统一展示快照（同 getRuntimeState，语义更清晰）
getPresentationSnapshot() {
    return this.getRuntimeState();
}
```

此别名让新用户更容易发现统一的快照 API，同时保持 `getRuntimeState()` 向后兼容。

### Step 6: 文档更新

**文件**:
- `docs/content/en/docs/api/runtime-state.md`
- `docs/content/zh/docs/api/runtime-state.md`

在 JSON 示例中添加展示层字段段，并更新字段说明表：

```
### 展示层字段 (v2 扩展)

| 字段 | 类型 | 说明 |
|------|------|------|
| `bg` | string? | 当前背景图路径 |
| `bgTransition` | string? | 背景过渡效果 |
| `bgm` | string? | 当前背景音乐路径 |
| `bgmVolume` | number | 背景音乐音量 (0.0–1.0) |
| `bgmLoop` | boolean | 背景音乐是否循环 |
| `sprites` | array | 精灵列表 (含 id, url, x, y, position, opacity, zIndex) |
| `sfx` | array | 音效列表 (含 id, path, volume, loop) |
| `endingId` | string? | 结局 ID (仅游戏结束后) |
| `endingTitle` | string? | 结局标题 (仅游戏结束后) |
| `runtimeStateVersion` | number | 运行时状态版本号，单调递增 |
| `runtimeStateChangeFlags` | number | 位掩码，标记本轮哪些数据变更 (1=变量, 2=背包, 3=两者) |
```

### Step 7: 测试

**文件**: `tests/c_api_test.cpp`（扩展）或新建 `tests/presentation_snapshot_test.cpp`

测试要点：

```cpp
// 1. 验证新字段出现在 JSON 输出中
TEST(PresentationSnapshotTest, IncludesAllPresentationFields) {
    // 构造含 bg, bgm, sprites, sfx 的场景
    // 调用 nova_export_runtime_state_json
    // 解析 JSON
    // 断言 .contains("bg"), .contains("bgm"), .contains("sprites"), .contains("sfx")
    // 断言 .contains("runtimeStateVersion"), .contains("runtimeStateChangeFlags")
}

// 2. 验证旧字段名称未变更
TEST(PresentationSnapshotTest, PreservesExistingFieldNames) {
    // 断言 .contains("status"), .contains("currentScene"), ...
    // 断言 .contains("variables"), .contains("inventory"), ...
    // 断言 .contains("dialogue"), .contains("choice")
}

// 3. 验证 sprites 数组字段完整性
TEST(PresentationSnapshotTest, SpriteFields) {
    // 场景包含精灵
    // 断言 sprites[0].contains("id"), .contains("url")
    // 断言 sprites[0].contains("x"), .contains("y")
    // 断言 sprites[0].contains("opacity"), .contains("zIndex")
}

// 4. 验证无对话/选择时，dialogue 和 choice 字段不出现 null
TEST(PresentationSnapshotTest, OptionalFieldsOmittedWhenAbsent) {
    // 场景无对话选择
    // 断言 bg 可能为空时不序列化或为 null
}
```

---

## 4. 兼容性保证（显式声明不变的部分）

以下 API 和行为的 **零变更**：

| 类别 | 具体内容 | 保证 |
|------|---------|------|
| ✅ WASM 粒度 getter | 所有 ~40 个 `nova_get_*` 函数签名和行为不变 | 完全不变 |
| ✅ WASM API 头文件 | `nova_wasm_api.h` 导出符号不变 | 不新增也不删除 |
| ✅ C API | `nova_c_api.h` 中 `nova_get_state()` 结构体和所有函数不变 | 完全不变 |
| ✅ JSON 字段名 | 所有已有字段 (`status`, `currentScene`, `variables`, `dialogue` 等) 名称和类型不变 | 完全不变 |
| ✅ 存档格式 | `nova_export_save_json/binary`, `nova_import_save_*` 行为不变 | 完全不变 |
| ✅ JS 渲染器方法 | `getRuntimeState()`, `advance()`, `selectChoice()` 等签名不变 | 完全不变 |
| ✅ 测试 | 所有现有测试继续通过 | 零回归 |

**唯一变化**: `nova_export_runtime_state_json()` 输出的 JSON 字符串新增展示层字段。

---

## 5. 迁移说明

- **无迁移要求**：纯新增字段，旧消费者自动忽略未识别字段
- **渲染器收益**：Web 渲染器可删除分散的 `getBackground()`, `getBgm()`, `getSpriteX()` 等调用，改为一次 `getRuntimeState()` 拿到全量数据。但保留旧 getter 供需要用粒度控制的渲染器使用
- **推荐做法**：新渲染器优先使用 `getRuntimeState()`/`getPresentationSnapshot()`，旧渲染器逐步迁移

---

## 6. 可选优化（后续迭代）

1. **按需快照模式**：若后续发现 JSON 全量快照性能瓶颈，可考虑添加轻量增量模式（仅返回变更部分），但 v1 不做
2. **二进制快照**：对高频渲染循环，后续可添加 Protocol Buffers / FlatBuffers 格式快照，但 v1 不做
3. **导出函数 `nova_export_presentation_snapshot_json`**：如果未来需要语义严格区分"VM 内状态"和"渲染层展示"，可以添加此别名函数（内部调用相同的扩展逻辑），但 v1 阶段以扩展现有函数为单一推荐入口

---

## 7. 文件变更清单总结

| 文件 | 操作 | 范围 |
|------|------|------|
| `src/renderer/wasm/nova_wasm_exports.cpp` | **修改** | 在 `nova_export_runtime_state_json` 中新增 ~50 行 JSON 构建代码 |
| `include/nova/vm/vm.h` | 可选修改 | 如果添加非消费 flags 读取，新增 1 行内联函数 |
| `template/web/nova_renderer.js` | 可选新增 | 添加 `getPresentationSnapshot()` 别名（1 行） |
| `docs/content/*/docs/api/runtime-state.md` | **修改** | 更新 JSON 示例和字段表（中英文各一份） |
| `tests/c_api_test.cpp` 或 `tests/presentation_snapshot_test.cpp` | **新增/扩展** | 4 个测试用例覆盖所有新字段 |
