[根目录](../../CLAUDE.md) > [src](..) > **cli**

# CLI -- 命令行工具

## 模块职责

提供 `nova-cli` 命令行工具，集成编译、静态检查和 Text Mode 调试运行三大功能。

## 入口与启动

- **可执行文件：** `nova-cli`（`src/cli/main.cpp`）
- **子命令：** `init`, `build`, `run`, `check`, `help`

## 对外接口

### 命令行接口

```bash
# 初始化项目
nova-cli init <project_name>

# 语法检查与静态分析
nova-cli check <source_path>

# 编译为 .nvmp
nova-cli build <source_path> -o <output_path>

# Text Mode 调试运行
nova-cli run <package_path>
```

### 核心函数

| 函数 | 说明 |
|------|------|
| `parse_args(argc, argv)` | 解析命令行参数 -> `CliConfig` |
| `do_init(config)` | 初始化项目 |
| `do_build(config)` | 编译打包 |
| `do_run(config)` | Text Mode 调试运行 |
| `do_check(config)` | 静态分析检查 |
| `load_project_metadata(dir)` | 从项目目录加载 `project.yaml` 元数据 |
| `extract_front_matter(file)` | 从脚本 Front Matter 提取元数据 |

### CliConfig 结构

```cpp
struct CliConfig {
    Command command;     // Init/Build/Run/Check/Help
    string source_path;
    string output_path;
    string project_name;
    bool verbose;
    bool show_version;
};
```

## 关键依赖与配置

- **链接库：** `nova-core`, `nova-lexer`, `nova-parser`, `nova-ast`, `nova-semantic`, `nova-packer`, `nova-vm`
- **CMake 目标：** `nova-cli`（可执行文件）

## Text Mode 调试热键

| 按键 | 功能 |
|------|------|
| `Enter` | 步进 |
| `1`-`9` | 选择分支选项 |
| `S` | 创建快照（存档） |
| `L` | 恢复快照（读档） |
| `Q` | 退出 |

## 深度实现细节

### 编译流程（`do_build`）

1. **单文件模式**：检测 `.nvm` 扩展名 → 提取 Front Matter 元数据 → `Packer` 打包为 `.nvmp`
2. **项目模式**：读取 `project.yaml` → 扫描 `scripts/` 目录 → `Packer` 打包（含 assets）

### 运行时流程（`do_run`）

1. 加载 `.nvmp` → `NvmpReader` 读取字节码和资源 → `AstDeserializer` 反序列化为 AST
2. `NovaVM` 加载 AST → 进入 Text Mode REPL 循环
3. 每次步进：输出状态变更（BG/BGM/Sprite/Dialogue/Choice/Ending）→ 等待用户输入
4. 结局触发时自动保存 Playthrough 数据（endings + flags）到 `.novamark/playthrough.json`

### 检查流程（`do_check`）

- **项目模式**：合并所有 `.nvm` 文件为单一 AST → `SemanticAnalyzer` 全局分析
- **单文件模式**：逐文件 Lex → Parse → Semantic Analyze
- 报告错误数和通过数

### 辅助工具

| 函数 | 说明 |
|------|------|
| `natural_compare(a, b)` | 自然排序比较（数字部分按数值比较），用于 `.nvm` 文件排序 |
| `collect_nvm_files_sorted(path, files)` | 收集目录下所有 `.nvm` 文件并自然排序 |
| `save_game(vm, sceneName)` | 保存到 `.novamark/saves/` |
| `load_game(vm, path, ...)` | 从文件恢复状态 |
| `setup_console()` | Windows 下设置 UTF-8 控制台编码 |

## 相关文件清单

| 文件 | 说明 |
|------|------|
| `include/nova/cli/cli.h` | CLI 函数声明 |
| `src/cli/cli.cpp` | CLI 实现（~820 行） |
| `src/cli/main.cpp` | 入口函数（参数解析 + 命令分发） |
| `src/cli/CMakeLists.txt` | CMake 构建配置 |

## 变更记录 (Changelog)

| 时间 | 操作 | 说明 |
|------|------|------|
| 2026-05-11 08:41:29 | 初始化 | 由 init-architect 自动生成 |
| 2026-05-11 08:49:00 | 补扫 | 深度读取 cli.cpp，补充编译/运行/检查流程细节 |
