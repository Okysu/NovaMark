---
title: "NovaMark"
weight: 1
---

# NovaMark

NovaMark 是一套为文字游戏/视觉小说设计的标记语言及其运行时引擎。

## 特性

- **简洁的语法** - 类似 Markdown 的自然语法，易于学习和使用
- **中文优先** - 完整支持中文标识符和内容
- **跨平台** - C++ 核心引擎，支持多平台部署
- **多端渲染** - 哑渲染器架构，同一游戏可运行在不同平台
- **完整的游戏逻辑** - 变量、物品、条件分支、骰子检定

## 快速开始

```bash
# 安装
git clone https://github.com/Okysu/NovaMark.git
cd novamark
cmake -B build -S .
cmake --build build

# 创建新项目
./build/src/cli/nova-cli init my-game

# 构建
./build/src/cli/nova-cli build my-game

# 运行
./build/src/cli/nova-cli run my-game/game.nvmp
```

## 文档目录

- [安装指南](/docs/getting-started/installation/) - 如何安装和配置 NovaMark
- [快速入门](/docs/getting-started/quickstart/) - 5 分钟创建你的第一个游戏
- [语法参考](/docs/syntax/) - 完整的语法规范
- [C API](/docs/api/c-api/) - C 语言接口文档

## 示例

```nvm
#scene_intro "序章"

@bg forest.png
@bgm theme.mp3

> 月光透过树梢洒落...

林晓: 这里是...哪里？

? 你要做什么？
- [探索周围] -> .explore
- [原地等待] -> .wait

.explore
林晓: 前方似乎有光亮。
-> scene_tower
```

## 许可证

MIT License
