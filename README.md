# NovaMark

<div align="center">

**为互动小说与视觉小说设计的标记语言及运行时引擎**

[English](./README_EN.md) | 简体中文

</div>

---

## 概述

NovaMark 是一套专为文字游戏、互动小说和视觉小说设计的标记语言及其跨平台运行时引擎。

**核心设计理念**：
- 🎯 **简洁语法** - 像 Markdown 一样易读易写
- ⚡ **高性能核心** - C++17 实现，轻量级虚拟机
- 🌐 **跨平台渲染** - 核心 VM 与渲染器分离，支持多端
- 📦 **单文件分发** - 剧本与资源打包为单一 `.nvmp` 文件

## 特性

### 语言特性
- ✅ 角色定义与对话
- ✅ 场景与标签跳转
- ✅ 分支选择系统
- ✅ 变量与表达式计算
- ✅ 骰子表达式（如 `2d6+3`）
- ✅ 条件分支（`@if`/`@else`）
- ✅ 背景图、立绘、BGM、SFX 指令
- ✅ 物品系统（`@give`/`@take`）
- ✅ 多结局支持
- ✅ 子程序调用（`@call`/`@return`）
- ✅ 存档点与存档系统

### 运行时特性
- ✅ Text Mode - 命令行文本渲染（调试用）
- 🚧 Web Renderer - 浏览器 GUI 渲染器（开发中）
  - Chat Mode - 聊天界面风格
  - VN Mode - 视觉小说风格

## 快速开始

### 语法示例

```novamark
---
title: 我的游戏
version: 1.0
---

@char 小明
  color: #4A90D9
  sprite_default: xiaoming_normal.png
@end

@var affection = 0

#scene_start "序章"

@bg room.png
@bgm peaceful.mp3

> 这是一个普通的早晨...

小明: 早安！今天天气真好。

? 你要怎么回应？
- [回以微笑] -> .smile
- [保持沉默] -> .silent

.smile
小明: 看来你心情不错呢。
@give affection 10
-> scene_next

.silent
小明: ...怎么了？
-> scene_next
```

### 构建项目

**前置要求**：
- CMake 3.16+
- C++17 编译器
- vcpkg 包管理器

```bash
# 1. 配置项目
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg]/scripts/buildsystems/vcpkg.cmake

# 2. 构建
cmake --build build --config Release

# 3. 运行测试
ctest --test-dir build --output-on-failure
```

### CLI 使用

```bash
# 构建游戏包
./build/src/cli/nova-cli build scripts/ -o game.nvmp

# 运行游戏（Text Mode）
./build/src/cli/nova-cli run game.nvmp

# 检查语法
./build/src/cli/nova-cli check scripts/
```

**运行时快捷键**（Text Mode）：
- `Enter` - 推进对话
- `1-9` - 选择选项
- `S` - 保存游戏
- `L` - 加载游戏
- `Q` - 退出

## 项目结构

```
NovaMark/
├── src/
│   ├── lexer/         # 词法分析器
│   ├── parser/        # 语法分析器
│   ├── ast/           # 抽象语法树
│   ├── semantic/      # 语义分析
│   ├── vm/            # 虚拟机
│   ├── packer/        # 打包工具
│   ├── renderer/      # 渲染器接口
│   └── cli/           # 命令行工具
├── include/nova/      # 公共头文件
├── tests/             # 测试套件
├── docs/              # 设计文档
├── examples/          # 示例剧本
└── template/          # 渲染器模板
    └── web/           # Web 渲染器模板（开发中）
```

## 架构设计

NovaMark 采用 **"核心 VM + 哑渲染器"** 架构：

```
┌─────────────────────────────────────────────────────────┐
│                    .nvmp 游戏包                          │
│  ┌─────────────┐  ┌──────────┐  ┌──────────────────┐   │
│  │ AST 字节码   │  │ 索引表    │  │ 资源数据          │   │
│  │             │  │          │  │ (图片/音频/字体)   │   │
│  └─────────────┘  └──────────┘  └──────────────────┘   │
└─────────────────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│                    NovaMark VM                          │
│  ┌──────────────────────────────────────────────────┐  │
│  │ 执行 AST → 更新 NovaState → 等待输入 → 继续执行    │  │
│  └──────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
                         │
        ┌────────────────┼────────────────┐
        ▼                ▼                ▼
┌─────────────┐  ┌─────────────┐  ┌─────────────┐
│ Text Mode   │  │ Web Chat    │  │ Web VN      │
│ (CLI 调试)   │  │ (聊天界面)   │  │ (视觉小说)   │
└─────────────┘  └─────────────┘  └─────────────┘
```

**核心原则**：
- VM 不关心如何渲染，只维护 `NovaState`
- 渲染器是"哑"的，只负责呈现状态
- 支持多种渲染器实现（CLI、Web、原生 GUI 等）

## 技术栈

| 组件 | 技术选型 |
|------|----------|
| 语言标准 | C++17 |
| 构建系统 | CMake 3.16+ |
| 包管理 | vcpkg |
| 测试框架 | GoogleTest |
| JSON 处理 | nlohmann-json |
| WASM 支持 | Emscripten（可选）|

## 依赖原则

**最小化依赖** - 仅引入必要的库：

| 依赖 | 用途 | 是否打包 |
|------|------|----------|
| nlohmann-json | JSON 序列化 | ✅ Header-only |
| GoogleTest | 单元测试 | ❌ 仅开发时 |

**禁止引入**：
- Boost（过于庞大）
- LLVM（除非需要编译器级复杂度）
- Qt（使用标准库替代）

## 文档

- [NovaMark 语法规范](./docs/NovaMark%20语法规范.md)
- [引擎架构与渲染指南](./docs/NovaMark%20引擎架构与渲染指南.md)

## 路线图

### v0.2 - Web 渲染器
- [ ] WASM 编译支持
- [ ] Chat Mode（聊天界面）
- [ ] VN Mode（视觉小说界面）
- [ ] 手动选择游戏包与存档
- [ ] WebAudio 音频支持

### v0.3 - 增强功能
- [ ] 存档缩略图
- [ ] 历史记录回看
- [ ] 自动/快进模式
- [ ] 设置面板

### v1.0 - 生产就绪
- [ ] 完整文档
- [ ] 性能优化
- [ ] 可视化编辑器（可选）

## 贡献

欢迎贡献代码、报告问题或提出建议！

## 许可证

MIT License

---

<div align="center">

**用 NovaMark 讲述你的故事** ✨

</div>
