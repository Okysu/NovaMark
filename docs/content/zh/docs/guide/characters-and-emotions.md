---
title: "角色与情绪"
weight: 5
---

# 角色与情绪

写互动故事时，最重要的问题之一是：

> 谁在说话？他们现在是什么情绪？

NovaMark 用 `@char` 来定义角色，用 `角色[情绪]:` 来表达带情绪的对话。

---

## 为什么需要先定义角色

你可能已经写过这样的对话：

```nvm
沈砚: 今天天气真好。
```

这确实能跑。但如果你的故事里有 5 个角色、每人有 3 种情绪状态，事情就会变得混乱：

- 谁是谁？
- 角色名应该用什么颜色显示？
- 不同情绪对应哪张立绘？

`@char` 就是为了解决这些问题。

---

## 最简单的角色定义

```nvm
@char 林晓
  color: #E8A0BF
@end
```

这表示：

- 角色名是 `林晓`
- 对话时，她的名字会用粉色 `#E8A0BF` 显示

---

## 给角色加上立绘

如果你的游戏是视觉小说风格，可能需要给角色配上立绘。

```nvm
@char 林晓
  color: #E8A0BF
  sprite_default: linxiao_normal.png
@end
```

这表示：

- `sprite_default` 是默认立绘
- 当你写 `林晓: 台词` 时，客户端知道该用哪张图

---

## 情绪与立绘切换

角色不会永远只有一种表情。

NovaMark 用一种直观的方式处理这件事：

### 定义多种情绪立绘

```nvm
@char 林晓
  color: #E8A0BF
  sprite_default: linxiao_normal.png
  sprite_happy: linxiao_happy.png
  sprite_sad: linxiao_sad.png
  sprite_angry: linxiao_angry.png
@end
```

### 在对话中使用情绪

```nvm
林晓: 你好。                    # 使用 sprite_default
林晓[happy]: 真的太好了！        # 使用 sprite_happy
林晓[sad]: 我不知道该怎么办...   # 使用 sprite_sad
林晓[angry]: 你怎么能这样！      # 使用 sprite_angry
```

你不需要手动写"切换立绘"的命令。只要写 `角色[情绪]:`，客户端就知道该用哪张图。

---

## 情绪命名规则

NovaMark 不会限制你用哪些情绪名。你可以根据自己游戏的需求来命名。

### 常见例子

```nvm
@char 沈砚
  color: #4A90D9
  sprite_default: shenyan_normal.png
  sprite_happy: shenyan_happy.png
  sprite_worried: shenyan_worried.png
  sprite_surprised: shenyan_surprised.png
@end
```

### 你也可以用中文情绪名

```nvm
@char 林晓
  color: #E8A0BF
  sprite_default: linxiao_normal.png
  sprite_开心: linxiao_happy.png
  sprite_难过: linxiao_sad.png
@end
```

然后这样用：

```nvm
林晓[开心]: 太棒了！
林晓[难过]: 为什么会这样...
```

唯一的要求是：

- 定义时用 `sprite_情绪名`
- 使用时用 `角色[情绪名]`

---

## 一个完整示例

```nvm
---
title: 角色示例
---

@char 林晓
  color: #E8A0BF
  description: 来自远方的旅人
  sprite_default: linxiao_normal.png
  sprite_happy: linxiao_happy.png
  sprite_sad: linxiao_sad.png
@end

@char 沈砚
  color: #4A90D9
  description: 沉默的守塔人
  sprite_default: shenyan_normal.png
  sprite_worried: shenyan_worried.png
@end

#scene_meeting "相遇"

@bg forest_clearing.png

> 森林深处，你们第一次相遇。

林晓: 你好，请问这里是哪里？

沈砚: ...这里是星光之塔的周围。

林晓[happy]: 星光之塔！我终于找到了！

沈砚[worried]: 你不该来这里的。

? 林晓似乎察觉到了什么不对劲。
- [追问] -> .ask_more
- [沉默] -> .stay_silent

.ask_more
林晓: 发生了什么事？

.stay_silent
> 你决定先观察情况。
```

---

## 角色定义的所有字段

目前 `@char` 支持以下字段：

| 字段 | 作用 | 是否必需 |
|---|---|---|
| `color` | 角色名显示颜色 | 推荐 |
| `description` | 角色简介 | 可选 |
| `sprite_default` | 默认立绘 | 推荐 |
| `sprite_*` | 情绪立绘 | 按需添加 |

---

## 写对话时的小技巧

### 不带情绪的对话

```nvm
林晓: 今天天气不错。
```

这会使用 `sprite_default`。

### 带情绪的对话

```nvm
林晓[happy]: 今天天气不错！
```

这会使用 `sprite_happy`（如果你定义了的话）。

### 情绪连续出现

```nvm
林晓[happy]: 太好了！
林晓: 我们快走吧。
```

第二句没有写情绪，所以会切回 `sprite_default`。

---

## 客户端会怎么处理这些信息

NovaMark 的设计原则是：

> 脚本提供数据和状态，客户端决定怎么呈现。

所以当你写：

```nvm
@char 林晓
  color: #E8A0BF
  sprite_happy: linxiao_happy.png
@end

林晓[happy]: 你好！
```

客户端会收到类似这样的状态：

- 当前说话者：林晓
- 当前情绪：happy
- 角色颜色：#E8A0BF
- 应该加载的立绘：linxiao_happy.png

然后客户端可以：

- 在视觉小说界面里切换立绘
- 在聊天界面里改变名字颜色
- 在纯文本模式里忽略这些信息

脚本不需要关心最终怎么显示。

---

## 下一步该学什么

现在你已经知道怎么定义角色和情绪了。

下一页，我们会讲：

- 怎么控制背景图片
- 怎么显示立绘
- 怎么播放音乐和音效

下一页建议看：

- [媒体与呈现](./media-and-presentation/)
