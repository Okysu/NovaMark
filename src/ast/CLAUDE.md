[根目录](../../CLAUDE.md) > [src](..) > **ast**

# AST -- 抽象语法树

## 模块职责

定义 NovaMark 的 AST 节点类型体系，并提供 AST 快照导出（JSON 格式）和源码回导功能。

## 入口与启动

- **根节点：** `nova::ProgramNode`
- **节点基类：** `nova::AstNode`（抽象，使用 `AstPtr = std::unique_ptr<AstNode>` 管理生命周期）
- **快照导出：** `export_ast_snapshot_string()` / `export_ast_snapshot_string_from_scripts()` / `export_ast_snapshot_string_from_path()`
- **源码回导：** `ast_snapshot_to_source_files_json()` -- 将 AST 快照 JSON 转换回 NovaMark 源码

## 对外接口

### 节点类型枚举 (`NodeType`)

| 类别 | 节点类型 |
|------|----------|
| 文件级 | `Program`, `FrontMatter` |
| 定义 | `CharDef`, `ItemDef`, `VarDef`, `SceneDef`, `ThemeDef` |
| 语句 | `Dialogue`, `Narrator`, `InterpolatedText`, `Choice`, `ChoiceOption`, `Branch`, `Jump`, `Call`, `Return`, `Label` |
| 指令 | `BgCommand`, `SpriteCommand`, `BgmCommand`, `SfxCommand`, `GiveCommand`, `TakeCommand`, `SetCommand`, `CheckCommand`, `Ending`, `Flag` |
| 表达式 | `BinaryExpr`, `UnaryExpr`, `Literal`, `Identifier`, `CallExpr`, `DiceExpr`, `Interpolation`, `InlineStyle` |

### 关键节点结构

- **LiteralNode：** 值类型为 `variant<monostate, string, double, bool>`
- **DialogueNode：** 包含 speaker/emotion/text + 可选 InterpolatedTextNode
- **ChoiceNode：** 包含 question + options 列表
- **BranchNode：** condition + then_branch + else_branch
- **CheckCommandNode：** condition + success_branch + failure_branch（TRPG 检定）
- **DiceExprNode：** count/sides/modifier（骰子表达式如 `2d6+3`）
- **InterpolatedTextNode：** 由 PlainText/Interpolation/InlineStyle Segment 序列组成

## 关键依赖与配置

- **上游依赖：** `nova-core`（SourceLocation）
- **下游消费者：** `nova-parser`（构建）、`nova-semantic`（分析）、`nova-vm`（执行）、`nova-packer`（序列化）
- **CMake 目标：** `nova-ast`（静态库）

## 数据模型

AST 采用树形结构，`ProgramNode` 为根节点，包含 `vector<AstPtr>` 语句列表。所有节点继承自 `AstNode`，使用 `unique_ptr` 管理所有权。

## 测试与质量

- 测试文件：`tests/ast_source_emitter_test.cpp`
- 覆盖：AST 快照导出和源码回导的完整性

## 相关文件清单

| 文件 | 说明 |
|------|------|
| `include/nova/ast/ast_node.h` | 全部 AST 节点定义（~790 行，核心头文件） |
| `include/nova/ast/ast_snapshot.h` | AST 快照导出函数声明 |
| `include/nova/ast/ast_source_emitter.h` | AST 快照转源码函数声明 |
| `src/ast/ast_node.cpp` | AST 节点辅助方法实现 |
| `src/ast/ast_snapshot.cpp` | AST 快照导出实现 |
| `src/ast/ast_source_emitter.cpp` | 源码回导实现 |
| `src/ast/CMakeLists.txt` | CMake 构建配置 |

## 变更记录 (Changelog)

| 时间 | 操作 | 说明 |
|------|------|------|
| 2026-05-11 08:41:29 | 初始化 | 由 init-architect 自动生成 |
