[根目录](../../CLAUDE.md) > [src](..) > **lexer**

# Lexer -- 词法分析器

## 模块职责

将 NovaMark 源码字符串切分为 Token 流，供 Parser 消费。负责识别标识符、字面量、运算符、关键字、文本行和空白缩进。

## 入口与启动

- **主类：** `nova::Lexer`（`include/nova/lexer/lexer.h`）
- **入口函数：** `Lexer::tokenize()` -- 返回 `Result<std::vector<Token>>`
- **单 Token 扫描：** `Lexer::next_token()` -- 返回 `Result<Token>`

## 对外接口

### Token 类型 (`TokenType`)

支持以下 Token 类别：

| 类别 | Token |
|------|-------|
| 字面量 | `Identifier`, `StringLiteral`, `NumberLiteral`, `Text` |
| 关键字 | `KwIf`, `KwEndif`, `KwElse`, `KwEnd`, `KwTrue`, `KwFalse`, `KwAnd`, `KwOr`, `KwNot` |
| 指令符号 | `AtSign`(@), `Hash`(#), `Colon`(:), `Arrow`(->), `Equals`(=), `Comma`, `Dot`, `Greater`(>), `Question`(?) |
| 括号 | `LeftParen`, `RightParen`, `LeftBracket`, `RightBracket`, `LeftBrace`, `RightBrace` |
| 算术运算 | `Plus`, `Minus`, `Asterisk`, `Slash`, `Percent` |
| 比较运算 | `EqualEqual`, `BangEqual`, `Less`, `LessEqual`, `GreaterEqual` |
| 控制符 | `Newline`, `Eof` |

### 关键方法

- `Lexer(source, filename)` -- 构造，接收源码字符串
- `tokenize()` -- 完整词法分析
- `next_token()` -- 逐个扫描
- `at_end()` -- 是否到达末尾

## 关键依赖与配置

- **上游依赖：** `nova-core`（`Result<T>`, `SourceLocation`）
- **下游消费者：** `nova-parser`
- **CMake 目标：** `nova-lexer`（静态库）

## 数据模型

- **Token 结构：** `{ type: TokenType, value: string, location: SourceLocation }`
- **SourceLocation：** 记录文件名、行号、列号

## 测试与质量

- 测试文件：`tests/lexer_test.cpp`
- 测试套件：`LexerTest`
- 覆盖要求：每个 Token 类型独立测试、边界条件（空输入/超长输入/非法字符）、错误路径（未闭合字符串）

## 相关文件清单

| 文件 | 说明 |
|------|------|
| `include/nova/lexer/lexer.h` | Lexer 类声明 |
| `include/nova/lexer/token.h` | Token 结构与 TokenType 枚举 |
| `src/lexer/lexer.cpp` | Lexer 实现 |
| `src/lexer/token.cpp` | Token 工具方法实现 |
| `src/lexer/CMakeLists.txt` | CMake 构建配置 |

## 变更记录 (Changelog)

| 时间 | 操作 | 说明 |
|------|------|------|
| 2026-05-11 08:41:29 | 初始化 | 由 init-architect 自动生成 |
