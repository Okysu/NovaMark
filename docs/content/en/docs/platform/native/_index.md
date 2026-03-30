---
title: "Native Integration"
weight: 1
bookCollapseSection: true
---

# Native Integration

This section covers how to integrate the NovaMark runtime into native platforms.

## Scope

- **iOS** - iPhone / iPad applications
- **Android** - Android applications
- **Desktop** - Windows / macOS / Linux desktop applications

## Integration Approaches

NovaMark supports two main native integration approaches:

### 1. WebView / WASM Approach

Compile NovaMark to WebAssembly and run it in the native application's WebView.

**Advantages**:
- Reuse Web renderer code
- Hot-update friendly
- High cross-platform consistency

**Disadvantages**:
- Additional performance overhead
- Native capability calls require bridging

### 2. Native Rendering Approach

Use the C API to directly integrate NovaMark VM and implement native UI rendering yourself.

**Advantages**:
- Best performance
- Direct access to native capabilities
- Fully controllable rendering

**Disadvantages**:
- Need to implement rendering logic yourself
- Multiple platforms require multiple UI codebases

## Subpages

Choose the corresponding integration guide based on your target platform:

- [iOS Integration Guide]({{< ref "ios" >}}) - Integrate NovaMark on iPhone / iPad
- [Android Integration Guide]({{< ref "android" >}}) - Integrate NovaMark in Android applications
- [Desktop Integration Guide]({{< ref "desktop" >}}) - Integrate NovaMark in desktop applications

## Core Concepts

Regardless of which integration approach you choose, you need to understand these core concepts:

- **NovaState** - Runtime state, the only data structure consumed by the renderer
- **C API** - Low-level interface for cross-language calls
- **Event Loop** - Interaction pattern between VM execution and renderer updates

For details, see the [Runtime and Host](../runtime-and-host/) documentation.
