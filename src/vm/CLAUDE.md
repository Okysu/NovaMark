[根目录](../../CLAUDE.md) > [src](..) > **vm**

# VM -- 虚拟机

## 模块职责

NovaMark 引擎的核心运行时。加载 AST 程序并执行，维护离散状态机，在阻塞点（对话/选择/结局/场景切换）暂停，向渲染器推送 `NovaState` 快照。

## 入口与启动

- **主类：** `nova::NovaVM`（`include/nova/vm/v.h`）
- **典型流程：**
  1. `load(program)` -- 加载 AST
  2. `setEntryPoint(sceneName)` -- 设置入口场景
  3. 循环：`state()` 获取快照 -> 渲染器绘制 -> `advance()` 步进 / `choose(id)` 选择

## 对外接口

### 核心生命周期

| 方法 | 说明 |
|------|------|
| `load(program)` | 加载 AST 程序，构建场景索引 |
| `advance()` | 推进到下一个离散阻塞点 |
| `choose(choiceId)` | 选择分支选项 |
| `state()` | 获取当前渲染状态快照 (`NovaState`) |
| `reset()` | 重置 VM |

### 场景控制

| 方法 | 说明 |
|------|------|
| `setEntryPoint(sceneName)` | 设置入口场景 |
| `jumpToScene(sceneName)` | 跳转到指定场景 |
| `jumpToLabel(labelName)` | 跳转到标签 |
| `callScene(sceneName)` | 调用子场景（保存返回点） |
| `returnFromCall()` | 返回调用点 |

### 状态管理

| 方法 | 说明 |
|------|------|
| `captureState()` | 捕获完整游戏状态 (`GameState`) |
| `loadSave(save)` | 从存档恢复 |
| `loadPlaythroughOnly(state)` | 仅恢复多周目继承数据（结局+标志） |
| `variables()` | 变量管理器引用 |
| `inventory()` | 背包管理器引用 |
| `playthrough()` | 周目状态引用 |
| `saves()` | 存档管理器引用 |

### NovaState 结构

```
NovaState {
    VMStatus status;          // Running / WaitingChoice / Ended
    TextConfigState textConfig;
    string currentScene, currentLabel;
    optional<string> bg, bgTransition, bgm;
    vector<SfxState> sfx;
    vector<SpriteState> sprites;
    optional<DialogueState> dialogue;
    optional<ChoiceState> choice;
    optional<string> ending, endingTitle;
}
```

### GameState 结构（可序列化快照）

包含 NovaState 所有字段 + 变量 + 背包 + 结局 + 标志 + 调用栈，用于存档/读档。

## 子组件

| 组件 | 文件 | 职责 |
|------|------|------|
| `VariableManager` | `variable.h/.cpp` | 动态类型变量（double/string/bool）管理 |
| `Inventory` | `inventory.h/.cpp` | 物品背包管理 |
| `PlaythroughState` | `save_data.h/.cpp` | 多周目状态（结局、标志） |
| `SaveManager` | `save_data.h/.cpp` | 存档 CRUD |
| `GameStateSerializer` | `serializer.h/.cpp` | GameState JSON/二进制序列化 |

## 关键依赖与配置

- **上游依赖：** `nova-ast`（AST 节点）、`nova-core`（工具类型）
- **下游消费者：** `nova-renderer`（通过状态接口）、`nova-cli`（run 命令）
- **CMake 目标：** `nova-vm`（静态库）

## 测试与质量

- 测试文件：`tests/vm_test.cpp`
- 覆盖：场景跳转、变量运算、物品系统、条件分支、骰子表达式、存档/读档

## 相关文件清单

| 文件 | 说明 |
|------|------|
| `include/nova/vm/vm.h` | NovaVM 类声明 |
| `include/nova/vm/state.h` | NovaState 及相关状态结构 |
| `include/nova/vm/game_state.h` | GameState 快照结构 |
| `include/nova/vm/variable.h` | VariableManager 变量管理 |
| `include/nova/vm/inventory.h` | Inventory 背包管理 |
| `include/nova/vm/save_data.h` | PlaythroughState / SaveManager |
| `include/nova/vm/serializer.h` | GameStateSerializer 序列化 |
| `src/vm/vm.cpp` | NovaVM 实现 |
| `src/vm/variable.cpp` | 变量管理实现 |
| `src/vm/inventory.cpp` | 背包管理实现 |
| `src/vm/save_data.cpp` | 存档管理实现 |
| `src/vm/serializer.cpp` | 序列化实现 |
| `src/vm/CMakeLists.txt` | CMake 构建配置 |

## 变更记录 (Changelog)

| 时间 | 操作 | 说明 |
|------|------|------|
| 2026-05-11 08:41:29 | 初始化 | 由 init-architect 自动生成 |
