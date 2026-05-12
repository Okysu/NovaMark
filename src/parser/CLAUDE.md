[根目录](../../CLAUDE.md) > [src](..) > **parser**

# Parser -- 语法分析器

## 模块职责

将 Lexer 产出的 Token 流解析为抽象语法树（AST）。采用递归下降解析策略，支持 NovaMark 全部语法结构。

## 入口与启动

- **主类：** `nova::Parser`（`include/nova/parser/parser.h`）
- **入口函数：** `Parser::parse()` -- 返回 `Result<AstPtr>`
- **单表达式解析：** `Parser::parse_single_expression()`

## 对外接口

### 顶层解析

- `parse()` -- 解析完整程序，返回 `ProgramNode`
- `parse_single_expression()` -- 解析单个表达式

### 语句解析方法（内部，私有）

| 方法 | 语法结构 |
|------|----------|
| `parse_dialogue()` | 角色对话 `角色名: 文本` |
| `parse_narrator()` | 旁白 `> 文本` |
| `parse_scene_def()` | 场景定义 `#scene_name "标题"` |
| `parse_choice()` | 选择分支 `? 问题` |
| `parse_if()` | 条件分支 `@if/@else/@endif` |
| `parse_char_def()` | 角色定义 `@char` |
| `parse_item_def()` | 物品定义 `@item` |
| `parse_directive()` | 通用指令分发 |
| `parse_var_def()` | 变量定义 `@var` |
| `parse_jump()` | 跳转 `-> target` |
| `parse_call_command()` / `parse_return_command()` | 子程序调用/返回 |
| `parse_bg_command()` / `parse_sprite_command()` | 背景/立绘指令 |
| `parse_bgm_command()` / `parse_sfx_command()` | 音乐/音效指令 |
| `parse_set_command()` | 变量赋值 `@set` |
| `parse_give_command()` / `parse_take_command()` | 物品给予/取走 |
| `parse_ending_command()` | 结局触发 `@ending` |
| `parse_flag_command()` | 标记设置 `@flag` |
| `parse_label()` | 子场景标签 `.label` |
| `parse_check_command()` | 检定 `@check` |
| `parse_theme_def()` | 主题定义 `@theme` |
| `parse_front_matter()` | YAML 元信息块 |

### 表达式解析（优先级递归下降）

`parse_expression()` -> `parse_or()` -> `parse_and()` -> `parse_comparison()` -> `parse_term()` -> `parse_factor()` -> `parse_unary()` -> `parse_primary()`

### 插值文本解析

- `parse_interpolated_text()` -- 解析 `{{expr}}` 变量插值和 `{style:text}` 内联样式

## 关键依赖与配置

- **上游依赖：** `nova-lexer`（Token 流）、`nova-ast`（AST 节点）、`nova-core`（Result、SourceLocation）
- **下游消费者：** `nova-semantic`、`nova-packer`、`nova-vm`
- **CMake 目标：** `nova-parser`（静态库）

## 数据模型

产出 `ProgramNode`（AST 根节点），包含全局定义与场景语句。详见 [AST 模块文档](../ast/CLAUDE.md)。

## 测试与质量

- 测试文件：`tests/parser_test.cpp`
- 覆盖要求：每个语法结构必须有解析测试

## 相关文件清单

| 文件 | 说明 |
|------|------|
| `include/nova/parser/parser.h` | Parser 类声明 |
| `src/parser/parser.cpp` | Parser 实现 |
| `src/parser/CMakeLists.txt` | CMake 构建配置 |

## 变更记录 (Changelog)

| 时间 | 操作 | 说明 |
|------|------|------|
| 2026-05-11 08:41:29 | 初始化 | 由 init-architect 自动生成 |
