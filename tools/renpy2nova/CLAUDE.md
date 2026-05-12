[根目录](../../CLAUDE.md) > [tools](..) > **renpy2nova**

# renpy2nova -- Ren'Py 到 NovaMark 转换工具

## 模块职责

将 Ren'Py 脚本（`.rpy`）转换为 NovaMark 脚本（`.nvm`）。支持单文件转换和项目目录批量转换，提供结构化的转换诊断报告。

## 入口与启动

- **可执行文件：** `renpy2nova`（`tools/renpy2nova/src/main.cpp`）
- **入口函数：** `nova::renpy2nova::run_application(argc, argv, out, err)`

## 对外接口

### 命令行模式

| 模式 | 说明 |
|------|------|
| `SingleFile` | 单文件转换：`renpy2nova <input.rpy> -o <output.nvm> [-r report.json]` |
| `Project` | 项目目录转换：`renpy2nova --project <project_dir> --output <output_dir>` |

### 转换管线

```
Ren'Py 源码 -> RenpyLexer (行级 Token) -> RenpyAnalyzer (结构化 AST)
    -> NovamarkEmitter (NovaMark 源码生成) -> ConversionReport (诊断报告)
```

### 核心类

| 类 | 职责 |
|----|------|
| `RenpyLexer` | Ren'Py 源码 -> 行级 `RenpyToken` 流 |
| `RenpyAnalyzer` | Token 流 -> `RenpyProject` 结构化 AST |
| `Converter` | 单文件转换门面 |
| `ProjectConverter` | 项目目录批量转换 |
| `NovamarkEmitter` | `RenpyProject` -> NovaMark 源码文本 |
| `ConversionReport` | 转换诊断收集与序列化 |
| `ResourceManifest` | 项目级资源线索提取 |

### 支持的 Ren'Py 构造

`RenpyTokenType` 支持识别：`Label`, `Say`, `Narrator`, `Menu`, `MenuChoice`, `Jump`, `Call`, `Return`, `If/Elif/Else`, `Scene`, `Show`, `Hide`, `PlayMusic`, `PlaySound`, `DefineCharacter`, `DefaultStatement`, `DollarStatement`, `PythonBlock` 等。

### 降级跟踪

转换条目按支持程度分级：`FullySupported`, `PartiallySupported`, `Unsupported`, `ManualFixRequired`。

## 关键依赖与配置

- **上游依赖：** `nova-core`
- **CMake 目标：** `renpy2nova-lib`（静态库）、`renpy2nova`（可执行文件）

## 测试与质量

- 测试文件：`tools/renpy2nova/tests/renpy2nova_test.cpp`
- 测试模式：Fixture (`.rpy` 输入) + Golden (`.nvm` 期望输出) 对比
- Fixture 覆盖：简单对话、菜单分支、高级语法、不支持构造降级等场景

## 相关文件清单

| 文件 | 说明 |
|------|------|
| `include/renpy2nova/app/application.h` | CLI 入口与参数解析 |
| `include/renpy2nova/lexer/renpy_lexer.h` | Ren'Py 词法分析器 |
| `include/renpy2nova/analyzer/renpy_analyzer.h` | Ren'Py 结构分析器 |
| `include/renpy2nova/converter/converter.h` | 单文件转换门面 |
| `include/renpy2nova/converter/project_converter.h` | 项目批量转换 |
| `include/renpy2nova/emitter/novamark_emitter.h` | NovaMark 代码生成 |
| `include/renpy2nova/report/conversion_report.h` | 转换诊断与结果类型 |
| `include/renpy2nova/resource/resource_manifest.h` | 资源线索提取 |
| `src/main.cpp` | 入口函数 |
| `src/app/application.cpp` | CLI 应用实现 |
| `src/lexer/renpy_lexer.cpp` | Ren'Py 词法分析实现 |
| `src/analyzer/renpy_analyzer.cpp` | 结构分析实现 |
| `src/converter/converter.cpp` | 转换器实现 |
| `src/converter/project_converter.cpp` | 项目转换实现 |
| `src/emitter/novamark_emitter.cpp` | 代码生成实现 |
| `src/report/conversion_report.cpp` | 诊断报告实现 |
| `src/resource/resource_manifest.cpp` | 资源清单实现 |
| `tests/renpy2nova_test.cpp` | 单元测试 |
| `tests/fixtures/*.rpy` | 测试输入 |
| `tests/golden/*.nvm` | 期望输出 |
| `CMakeLists.txt` | CMake 构建配置 |

## 变更记录 (Changelog)

| 时间 | 操作 | 说明 |
|------|------|------|
| 2026-05-11 08:41:29 | 初始化 | 由 init-architect 自动生成 |
