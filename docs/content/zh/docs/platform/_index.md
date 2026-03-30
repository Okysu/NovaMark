---
title: "为平台、渲染与发行扩展方"
weight: 2
bookCollapseSection: true
---

# 为平台、渲染与发行扩展方

这一部分面向：

- 游戏工作室
- 发行平台接入方
- Web / Native 渲染器开发者
- 想自己消费 NovaMark 运行时状态的人

这里重点不是"怎么写剧情"，而是：

- NovaMark 的 VM 如何推进
- 运行时状态长什么样
- 宿主和脚本如何分工
- 为什么引擎只产出离散状态
- Web 模板和 C API 应该怎么理解

如果你要把 NovaMark 接入自己的产品，这一部分会比语法细节更重要。

---

## 平台接入指南

### Web 平台

| 文档 | 说明 |
|------|------|
| [WASM API]({{< ref "wasm-api" >}}) | WebAssembly 接口规范，在浏览器中运行 NovaMark VM |
| [Web 渲染模板]({{< ref "web-template" >}}) | 开箱即用的 Web 渲染器模板（Chat/VN 模式） |

### Native 平台

| 文档 | 说明 |
|------|------|
| [运行时与宿主]({{< ref "runtime-and-host" >}}) | 理解 VM 与宿主的职责边界、状态同步机制 |
| [Native 接入]({{< ref "native" >}}) | iOS / Android / Desktop 接入指南 |
| [C API]({{< ref "../api/c-api" >}}) | 原生 C 接口，用于集成到桌面/移动应用 |

### 参考手册

| 文档 | 说明 |
|------|------|
| [速查参考](../reference/) | 语法 / 状态 / API 快速定位 |
| [运行时状态](../api/runtime-state/) | `NovaState` JSON 结构与字段说明 |
