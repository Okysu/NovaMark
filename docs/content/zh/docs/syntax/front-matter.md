---
title: "Front Matter 元数据"
weight: 2
---

# Front Matter 元数据

Front Matter 是文件开头的 YAML 格式元数据块，用于定义游戏的基本信息。

## 语法

```nvm
---
key: value
key2: "string value"
---

# 游戏内容从这里开始...
```

以 `---` 开始和结束，中间是 YAML 格式的键值对。

## 支持的字段

### 基本信息字段

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `title` | 字符串 | 否 | 游戏标题 |
| `name` | 字符串 | 否 | 游戏名称（用于打包文件名） |
| `author` | 字符串 | 否 | 作者名称 |
| `version` | 字符串 | 否 | 版本号，如 `1.0.0` |
| `description` | 字符串 | 否 | 游戏描述 |

### 资源路径字段

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `base_bg_path` | 字符串 | `assets/bg/` | 背景图片基础路径 |
| `base_sprite_path` | 字符串 | `assets/sprites/` | 精灵图片基础路径 |
| `base_audio_path` | 字符串 | `assets/audio/` | 音频文件基础路径 |

### 运行时配置字段

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `entry_scene` | 字符串 | 第一个场景 | 入口场景ID |
| `default_font` | 字符串 | 系统默认 | 默认字体 |
| `default_font_size` | 数字 | 24 | 默认字号 |
| `default_text_speed` | 数字 | 50 | 默认文字速度（字符/秒） |

## 完整示例

```nvm
---
title: 迷雾森林
name: mist_forest
author: NovaMark Team
version: 1.0.0
description: 一个关于迷失与寻找的冒险故事
entry_scene: scene_intro
base_bg_path: assets/backgrounds/
base_sprite_path: assets/characters/
base_audio_path: assets/sounds/
default_font: SourceHanSansCN-Regular.ttf
default_font_size: 28
default_text_speed: 60
---

#scene_intro "序章"

@bg forest_mist.png
> 清晨的薄雾笼罩着森林...
```

## 单文件 vs 项目配置

### 单文件模式

当使用 `nova-cli build game.nvm` 构建单个 `.nvm` 文件时：

- Front Matter 中的元数据会被读取
- 如果没有 Front Matter，默认使用文件名作为游戏名

### 项目模式

当使用 `nova-cli build` 构建包含 `project.yaml` 的项目时：

- `project.yaml` 的配置优先级高于 Front Matter
- Front Matter 中的资源路径设置会被保留

## 读取顺序

NovaMark 按以下顺序确定游戏元数据：

1. `project.yaml`（项目模式）
2. 第一个 `.nvm` 文件的 Front Matter
3. 默认值
