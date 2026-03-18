---
title: "project.yaml 配置"
weight: 3
---

# project.yaml 配置

`project.yaml` 是 NovaMark 项目的配置文件，定义项目结构、构建选项和运行时设置。

## 文件位置

放在项目根目录下：

```
my-game/
├── project.yaml        # 项目配置
├── scripts/            # 脚本目录
│   └── *.nvm
└── assets/             # 资源目录
    ├── bg/
    ├── sprites/
    └── audio/
```

## 完整配置示例

```yaml
name: mist_forest
title: 迷雾森林
version: 1.0.0
author: NovaMark Team

entry_scene: scene_intro

scripts_path: scripts
assets_path: assets

default_font: fonts/SourceHanSansCN-Regular.ttf
default_font_size: 28
default_text_speed: 60
```

## 字段说明

### 基本信息

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `name` | 字符串 | 是 | 项目名称（内部标识，用于生成 .nvmp 文件名） |
| `title` | 字符串 | 否 | 游戏显示标题（默认使用 name） |
| `version` | 字符串 | 否 | 版本号 |
| `author` | 字符串 | 否 | 作者/团队名称 |

### 项目结构

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `scripts_path` | 字符串 | `scripts` | 脚本文件目录 |
| `assets_path` | 字符串 | `assets` | 资源文件目录 |

### 运行时配置

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `entry_scene` | 字符串 | 第一个场景 | 游戏入口场景ID |
| `default_font` | 字符串 | 系统字体 | 默认字体文件路径 |
| `default_font_size` | 数字 | 24 | 默认字号 |
| `default_text_speed` | 数字 | 50 | 文字显示速度（字符/秒） |

## 项目目录结构

`nova-cli init my-game` 创建的标准结构：

```
my-game/
├── project.yaml            # 项目配置
├── scripts/
│   ├── 00_init.nvm         # 初始化脚本（角色、物品定义）
│   └── 01_main.nvm         # 主脚本
├── assets/
│   ├── bg/                 # 背景图片 (.png, .jpg)
│   ├── sprites/            # 角色立绘 (.png)
│   └── audio/              # 音频文件 (.mp3, .ogg)
└── README.md
```

## 多脚本文件

项目模式支持多个 `.nvm` 文件，按文件名排序合并：

```yaml
scripts_path: scripts
```

```
scripts/
├── 00_characters.nvm    # 角色定义
├── 01_items.nvm         # 物品定义
├── 02_intro.nvm         # 序章
├── 03_chapter1.nvm      # 第一章
└── 04_chapter2.nvm      # 第二章
```

**合并规则**：
- 按文件名字典序排序
- 所有脚本合并为一个 AST
- 场景定义可以跨文件引用

## 构建命令

```bash
# 构建项目（自动查找 project.yaml）
nova-cli build

# 构建指定目录的项目
nova-cli build ./my-game

# 指定输出文件
nova-cli build ./my-game -o release/game.nvmp

# 显示详细信息
nova-cli build ./my-game -v
```

## 配置验证

使用 `check` 命令验证配置：

```bash
nova-cli check ./my-game
```

会检查：
- `project.yaml` 格式是否正确
- `scripts_path` 目录是否存在
- 所有 `.nvm` 文件语法是否正确
- 场景跳转引用是否存在
