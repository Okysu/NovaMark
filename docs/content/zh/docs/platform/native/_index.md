---
title: "Native 接入"
weight: 1
bookCollapseSection: true
---

# Native 接入

本专题介绍如何将 NovaMark 运行时接入原生平台。

## 适用范围

- **iOS** - iPhone / iPad 应用
- **Android** - Android 应用
- **Desktop** - Windows / macOS / Linux 桌面应用

## 接入方式

NovaMark 支持两种主要的原生接入方式：

### 1. WebView / WASM 方案

将 NovaMark 编译为 WebAssembly，在原生应用的 WebView 中运行。

**优点**：
- 复用 Web 渲染器代码
- 热更新友好
- 跨平台一致性高

**缺点**：
- 性能有额外开销
- 原生能力调用需要桥接

### 2. 原生渲染方案

使用 C API 直接集成 NovaMark VM，自行实现原生 UI 渲染。

**优点**：
- 性能最优
- 可直接访问原生能力
- 渲染完全可控

**缺点**：
- 需要自行实现渲染逻辑
- 多平台需要多套 UI 代码

## 子页面

根据你的目标平台，选择对应的接入指南：

- [iOS 接入指南](./ios/) - 在 iPhone / iPad 上集成 NovaMark
- [Android 接入指南](./android/) - 在 Android 应用中集成 NovaMark
- [Desktop 接入指南](./desktop/) - 在桌面应用中集成 NovaMark

## 核心概念

无论选择哪种接入方式，都需要理解以下核心概念：

- **NovaState** - 运行时状态，渲染器消费的唯一数据结构
- **C API** - 跨语言调用的底层接口
- **事件循环** - VM 执行与渲染器更新的交互模式

详细信息请参考 [运行时与宿主](../runtime-and-host/) 文档。
