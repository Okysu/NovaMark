[根目录](../../CLAUDE.md) > [src](..) > **semantic**

# Semantic -- 语义分析器

## 模块职责

对 Parser 产出的 AST 执行四遍语义分析，检查定义完整性、引用有效性、控制流结构和未使用符号。收集符号表并生成诊断报告。

## 入口与启动

- **主类：** `nova::SemanticAnalyzer`（`include/nova/semantic/semantic_analyzer.h`）
- **入口函数：** `SemanticAnalyzer::analyze(const ProgramNode*)` -- 返回 `SemanticAnalysisResult`

## 对外接口

### 分析结果

```cpp
struct SemanticAnalysisResult {
    bool success;
    DiagnosticCollector diagnostics;
    SymbolTable symbol_table;
};
```

### 四遍分析流程

| 遍次 | 方法 | 职责 |
|------|------|------|
| 1 | `collect_definitions()` | 收集场景、变量、角色、物品、标签、主题定义 |
| 2 | `check_references()` | 检查跳转目标、变量引用、角色引用、物品引用 |
| 3 | `check_structures()` | 控制流和结构检查（分支完整性、选项目标等） |
| 4 | `check_unused_symbols()` | 报告未使用的定义 |

### 符号表 (`SymbolTable`)

支持以下符号类型（`SymbolKind`）：`Scene`, `Variable`, `Character`, `Item`, `Label`, `Theme`

- 场景级作用域（`enter_scene` / `leave_scene`）
- 块级作用域（`enter_block` / `leave_block`，用于 if/check 分支）
- 符号使用追踪（`mark_used`）和未使用符号查询（`get_unused_symbols`）

### 诊断系统 (`DiagnosticCollector`)

- **级别：** `Error`, `Warning`, `Note`
- **错误类型：** `UndefinedScene`, `UndefinedVariable`, `UndefinedCharacter`, `UndefinedItem`, `UndefinedLabel`, `DuplicateScene`, `DuplicateVariable` 等 17 种
- **输出：** 每条诊断包含级别、错误类型、消息、源码位置

## 关键依赖与配置

- **上游依赖：** `nova-ast`（AST 节点）、`nova-core`（Result、SourceLocation）
- **下游消费者：** `nova-cli`（check 命令）
- **CMake 目标：** `nova-semantic`（静态库）

## 测试与质量

- 测试文件：`tests/semantic_test.cpp`
- 覆盖：未定义引用、重复定义、控制流错误、未使用符号等

## 相关文件清单

| 文件 | 说明 |
|------|------|
| `include/nova/semantic/semantic_analyzer.h` | SemanticAnalyzer 类声明 |
| `include/nova/semantic/diagnostic.h` | 诊断类型与收集器 |
| `include/nova/semantic/symbol_table.h` | 符号表管理器 |
| `src/semantic/semantic_analyzer.cpp` | 语义分析实现 |
| `src/semantic/diagnostic.cpp` | 诊断实现 |
| `src/semantic/symbol_table.cpp` | 符号表实现 |
| `src/semantic/CMakeLists.txt` | CMake 构建配置 |

## 变更记录 (Changelog)

| 时间 | 操作 | 说明 |
|------|------|------|
| 2026-05-11 08:41:29 | 初始化 | 由 init-architect 自动生成 |
