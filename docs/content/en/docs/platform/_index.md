---
title: "Platform, Rendering, and Distribution"
weight: 2
bookCollapseSection: true
---

# Platform, Rendering, and Distribution

This section is for:

- Game studios
- Distribution platform integrators
- Web / Native renderer developers
- Anyone who wants to consume NovaMark runtime state directly

The focus here is not "how to write stories", but:

- How the NovaMark VM advances
- What runtime state looks like
- How host and script responsibilities are divided
- Why the engine only produces discrete states
- How to understand the Web template and C API

If you are integrating NovaMark into your own products, this section is more important than syntax details.

---

## Platform Integration Guide

### Web Platform

| Document | Description |
|----------|-------------|
| [WASM API]({{< ref "wasm-api" >}}) | WebAssembly interface specification for running NovaMark VM in the browser |
| [Web Rendering Template]({{< ref "web-template" >}}) | Ready-to-use Web renderer template (Chat/VN modes) |

### Native Platform

| Document | Description |
|----------|-------------|
| [Runtime and Host]({{< ref "runtime-and-host" >}}) | Understand the VM and host responsibility boundary, state synchronization mechanism |
| [Native Integration]({{< ref "native" >}}) | iOS / Android / Desktop integration guides |
| [C API]({{< ref "../api/c-api" >}}) | Native C interface for integrating into desktop/mobile applications |

### Reference Manual

| Document | Description |
|----------|-------------|
| [Quick Reference](../reference/) | Quick lookup for syntax / state / API |
| [Runtime State](../api/runtime-state/) | `NovaState` JSON structure and field descriptions |
