---
title: "快速入门"
weight: 2
---

# 快速入门

本指南将帮助你在 5 分钟内创建第一个 NovaMark 游戏。

## 创建项目

```bash
./build/src/cli/nova-cli init my-first-game
```

这会创建以下目录结构:

```
my-first-game/
├── project.yaml      # 项目配置
├── scripts/
│   └── main.nvm      # 主脚本
├── assets/
│   ├── bg/           # 背景图片
│   ├── sprites/      # 角色立绘
│   └── audio/        # 音频文件
└── README.md
```

## 编写你的第一个场景

编辑 `scripts/main.nvm`:

```nvm
---
title: 我的第一个游戏
author: 你的名字
---

#scene_start "开始"

@bg room.png

> 这是一个普通的房间。

林晓: 你好，世界！

? 你要做什么？
- [环顾四周] -> .look_around
- [离开房间] -> .leave

.look_around
> 房间里有一张桌子和一把椅子。
-> scene_start

.leave
> 你走出了房间。
@ending the_end
```

## 构建游戏

```bash
./build/src/cli/nova-cli build my-first-game
```

这会生成 `my-first-game/game.nvmp` 文件。

## 运行游戏

```bash
./build/src/cli/nova-cli run my-first-game/game.nvmp
```

## 下一步

- [语法参考](../../syntax/) - 学习完整的语法
- [完整示例](https://github.com/Okysu/NovaMark/tree/main/examples) - 查看更多示例
