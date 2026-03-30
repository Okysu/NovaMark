---
title: "NovaMark 文档"
type: docs
bookCollapseSection: false
---

# NovaMark 文档

欢迎来到 NovaMark 文档。

NovaMark 是一套为**文字游戏、视觉小说、互动叙事**设计的脚本语言与运行时系统。

如果你以前没有写过脚本语言，也不用担心。新的文档结构会优先回答这些问题：

- NovaMark 到底是什么？
- 我应该先学什么？
- 场景、对话、选择、变量分别是什么意思？
- 我写的内容和客户端 UI 的关系是什么？

## 选择你的入口

### 1. 为创作者

如果你想写剧情、写角色、写选择、写检定，请从这里开始。

你会看到：

- 如何写第一段故事
- 如何组织场景
- 如何理解状态、变量、物品
- 如何写选择、分支和检定

推荐入口：

- [什么是 NovaMark]({{< ref "guide/what-is-novamark" >}})
- [第一段故事]({{< ref "guide/your-first-story" >}})
- [状态、变量与物品]({{< ref "guide/state-and-variables" >}})
- [选择、分支与检定]({{< ref "guide/choices-and-checks" >}})

### 2. 为平台、渲染与发行扩展方

如果你是：

- 游戏工作室
- 引擎接入方
- 平台扩展开发者
- 想自己做 Native / Web 渲染器的人

那你更关心的是：

- VM 如何推进
- 运行时状态如何导出
- 宿主与脚本怎么分工
- C API / WASM / 模板仓库该怎么用

这个入口会重点解释这些内容。

### 3. Reference

Reference 面向已经知道自己要查什么的用户。

你可以在这里查：

- 语法参考
- API 参考
- 运行时状态字段
- 项目配置

### 4. Glossary

Glossary 用来解释术语，例如：

- 场景
- 标签
- 检定
- 快照
- 宿主
- 渲染提示
