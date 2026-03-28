---
title: "媒体与表现"
weight: 6
---

# 媒体与表现

到这一章，你已经能用文字讲一个互动故事了。

但如果想让玩家"看到"和"听到"你的世界，就需要了解媒体命令。

NovaMark 的媒体命令非常简单：

- `@bg` - 背景
- `@sprite` - 立绘 / 精灵
- `@bgm` - 背景音乐
- `@sfx` - 音效

---

## 先理解一个概念：资源路径

在写媒体命令之前，你需要知道资源放在哪里。

NovaMark 不会硬性规定资源必须放在哪个目录，但推荐在元信息块里配置基础路径：

```nvm
---
title: 迷雾森林
base_bg_path: assets/bg/
base_sprite_path: assets/sprites/
base_audio_path: assets/audio/
---
```

这样做的好处是：

- 你写 `@bg forest.png`，引擎知道去 `assets/bg/forest.png` 找
- 换个平台时，只需要改基础路径，不用改脚本

如果你没有配置基础路径，资源路径就按相对路径处理。

---

## 背景图：`@bg`

背景是最基础的视觉元素。

### 基本用法

```nvm
@bg forest_night.png
```

这会告诉引擎：把背景换成 `forest_night.png`。

### 加转场效果

```nvm
@bg forest_night.png transition:fade
```

转场效果由客户端决定如何实现，常见的有：

- `fade` - 淡入淡出
- `dissolve` - 溶解
- `slide` - 滑动

### 为什么要用转场

直接换背景可能显得生硬。加上转场后，画面切换会更自然。

---

## 立绘：`@sprite`

立绘是显示在背景上的角色图像。

### 显示立绘

```nvm
@sprite 林晓 show url:linxiao_default.png position:left
```

这表示：

- 角色名是 `林晓`
- 显示立绘
- 图片是 `linxiao_default.png`
- 位置在左边

### 隐藏立绘

```nvm
@sprite 林晓 hide
```

### `show` 的明确语义

- `@sprite 林晓 show url:linxiao_default.png position:left`：显示/更新该立绘，并应用你写出的参数
- `@sprite 林晓 show position:left`：如果 `@char 林晓` 里定义了 `sprite_default`，会直接使用默认立绘
- `@sprite 林晓 hide`：把该角色从当前立绘状态中移除

这意味着常见情况下，你不一定每次都要手写图片路径；如果只是让角色按默认立绘登场，可以直接写 `show`。

### 指定精确位置

你也可以直接写坐标值：

```nvm
@sprite 林晓 show url:linxiao_happy.png x:70 y:100
```

NovaMark 会把 `x` 和 `y` 原样保留到运行时状态里，不在 VM 内做数值推导。
是否把它解释成百分比、像素或别的坐标体系，由宿主自己决定。

### 情绪立绘的快捷写法

如果你在角色定义里设置了情绪立绘，对话时可以直接用：

```nvm
@char 林晓
  sprite_default: linxiao_default.png
  sprite_happy: linxiao_happy.png
@end

#scene_test "测试"

林晓[happy]: 今天天气真好！
```

`林晓[happy]` 会自动使用 `sprite_happy` 定义的图片。

如果没有写情绪，例如：

```nvm
林晓: 我到了。
```

那么会自动回退到 `sprite_default`。

### 立绘会一直留在屏幕上吗？

NovaMark 当前的规则是：

- 同一场景内，立绘会持续保留，直到你显式写 `@sprite 角色名 hide`
- 切换到新的场景时，当前场景的立绘会自动清空

这更接近传统 VN 的常见心智模型：**场景切换时自动清场，场景内按需要显式调度**。

### 稀疏立绘状态是什么意思？

NovaMark 的运行时只会输出你显式设置过的立绘字段。

例如：

```nvm
@sprite 林晓 show position:left opacity:0.8
```

那么宿主拿到的就是：

- `id`
- `url`
- `position`
- `opacity`

而不会再额外补出默认的 `x=0`、`y=0`、`zIndex=0` 之类字段。

因此推荐宿主按这个顺序处理：

1. 先看 `x/y`
2. 再看 `position`
3. 都没有时使用宿主自己的默认对白布局

---

## 背景音乐：`@bgm`

BGM 用来营造氛围。

### 基本用法

```nvm
@bgm ambient_forest.mp3
```

### 循环播放

```nvm
@bgm ambient_forest.mp3 loop:true
```

大多数情况下，BGM 都应该循环播放。

### 调整音量

```nvm
@bgm ambient_forest.mp3 loop:true volume:0.3
```

音量范围是 `0.0` 到 `1.0`，`0.3` 表示 30% 音量。

### 停止音乐

```nvm
@bgm stop
```

---

## 音效：`@sfx`

音效用于短促的声音，比如开门声、脚步声。

### 基本用法

```nvm
@sfx door_open.mp3
```

音效默认不循环，播放一次就结束。

### 调整音量

```nvm
@sfx door_open.mp3 volume:0.5
```

---

## 一个完整的场景示例

把上面所有内容组合起来：

```nvm
---
title: 星光之塔
base_bg_path: assets/bg/
base_sprite_path: assets/sprites/
base_audio_path: assets/audio/
---

@char 林晓
  color: #E8A0BF
  sprite_default: linxiao_default.png
  sprite_happy: linxiao_happy.png
@end

#scene_tower "星光之塔"

@bg tower_entrance.png transition:fade
@bgm mystery.mp3 loop:true volume:0.4

> 古老的塔楼矗立在月光下。

@sprite 林晓 show url:linxiao_default.png position:left

林晓: 这里就是传说中的星光之塔吗？

? 你要先做什么？
- [推门进去] -> .enter
- [绕着塔走一圈] -> .walk_around

.enter
@sfx door_creak.mp3 volume:0.6
@bg tower_interior.png transition:fade
> 厚重的木门缓缓打开。
-> .continue

.walk_around
林晓[happy]: 看，墙上有奇怪的符文。
-> .continue

.continue
> 你感受到了一股神秘的力量。
```

这段脚本包含了：

- 背景切换 + 转场
- BGM 循环播放
- 立绘显示
- 音效触发
- 情绪立绘

---

## 几个常见问题

### 1. 资源格式有什么要求？

这取决于你的客户端实现。常见支持：

| 类型 | 常见格式 |
|------|----------|
| 背景图 | PNG, JPG |
| 立绘 | PNG（带透明通道） |
| BGM | MP3, OGG |
| 音效 | MP3, WAV |

### 2. 资源路径写错了会怎样？

引擎会输出错误信息，但不会崩溃。玩家可能会看到/听到"资源缺失"的提示。

### 3. 可以同时播放多个音效吗？

NovaMark 不限制，但具体表现由客户端决定。大部分客户端支持多个音效叠加。

### 4. BGM 会自动切换吗？

不会。你需要在脚本里明确写 `@bgm` 来切换或停止音乐。

---

## 媒体命令速查表

| 命令 | 用途 | 示例 |
|------|------|------|
| `@bg` | 切换背景 | `@bg forest.png transition:fade` |
| `@sprite show` | 显示立绘 | `@sprite 林晓 show url:x.png position:left` |
| `@sprite hide` | 隐藏立绘 | `@sprite 林晓 hide` |
| `@bgm` | 播放音乐 | `@bgm bgm.mp3 loop:true volume:0.5` |
| `@bgm stop` | 停止音乐 | `@bgm stop` |
| `@sfx` | 播放音效 | `@sfx click.mp3` |

---

## 媒体命令和客户端的关系

记住 NovaMark 的设计原则：

> 脚本负责"什么时候播放什么"，客户端负责"怎么播放"。

这意味着：

- 你在脚本里写 `@bg forest.png transition:fade`
- 客户端决定 fade 具体用多少秒、用什么曲线
- 不同平台可以有不同的表现效果

好处是：同一份脚本，可以在 Web、桌面、移动端使用，而不用为每个平台写不同版本。

---

## 下一步该学什么

现在你已经知道如何让你的故事"有画面、有声音"了。

下一步最自然的问题是：

- 多个场景之间怎么跳转？
- 怎么组织一个完整的故事结构？

下一页建议看：

- [场景与流程控制]({{< ref "scene-flow" >}})
