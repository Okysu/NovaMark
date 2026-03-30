---
title: "速查参考"
weight: 2
bookCollapseSection: true
---

# 速查参考

Reference 适合已经知道自己要查什么的用户。

这里更像手册，而不是教程。

## 速查入口

- [语法/命令速查表]({{< ref "cheatsheet" >}}) — 一页总览语法结构
- [语法详细参考]({{< ref "../syntax" >}}) — 完整语法手册
- [运行时状态字段]({{< ref "../api/runtime-state" >}}) — NovaState JSON 结构
- [C API]({{< ref "../api/c-api" >}}) — 原生接口
- [平台集成]({{< ref "../platform" >}}) — 宿主与渲染器对接

## 按场景查找

### 我要写...

| 需求 | 参考 |
|------|------|
| 角色对话 | [语法速查 → 对话]({{< ref "cheatsheet" >}}#对话) |
| 玩家选择 | [语法速查 → 选择]({{< ref "cheatsheet" >}}#选择) |
| 条件检定 | [语法速查 → 检定]({{< ref "cheatsheet" >}}#检定) |
| 变量操作 | [语法速查 → 变量]({{< ref "cheatsheet" >}}#变量) |
| 物品系统 | [语法速查 → 物品]({{< ref "cheatsheet" >}}#物品) |
| 场景跳转 | [语法速查 → 场景]({{< ref "cheatsheet" >}}#场景) |
| 媒体资源 | [语法速查 → 媒体]({{< ref "cheatsheet" >}}#媒体) |

### 我要对接...

| 需求 | 参考 |
|------|------|
| 读取游戏状态 | [运行时状态]({{< ref "../api/runtime-state" >}}) |
| C/C++ 嵌入 | [C API]({{< ref "../api/c-api" >}}) |
| 实现渲染器 | [平台集成]({{< ref "../platform" >}}) |
| 存档/读档 | [运行时状态 → 存档格式]({{< ref "../api/runtime-state" >}}#存档格式说明) |

### 报错了...

| 错误类型 | 参考 |
|----------|------|
| 语法错误 | [命令参考]({{< ref "../syntax/commands" >}}) |
| 变量未定义 | [变量系统]({{< ref "../syntax/variables" >}}) |
| 场景跳转失败 | [场景与标签]({{< ref "../syntax/scenes" >}}) |
| 运行时崩溃 | [C API 错误处理]({{< ref "../api/c-api" >}}) |
