---
title: "Reference"
weight: 2
bookCollapseSection: true
---

# Reference

Reference is for users who already know what they want to look up.

This is a manual, not a tutorial.

## Quick Reference Entries

- [Syntax/Command Cheat Sheet]({{< ref "cheatsheet" >}}) — One-page syntax overview
- [Syntax Reference]({{< ref "../syntax" >}}) — Complete syntax manual
- [Runtime State]({{< ref "../api/runtime-state" >}}) — NovaState JSON structure
- [C API]({{< ref "../api/c-api" >}}) — Native integration interface
- [Platform Integration]({{< ref "../platform" >}}) — Host and renderer integration

## Find by Scenario

### I want to write...

| Need | Reference |
|------|-----------|
| Character dialogue | [Cheat Sheet → Dialogue]({{< ref "cheatsheet" >}}#dialogue) |
| Player choices | [Cheat Sheet → Choices]({{< ref "cheatsheet" >}}#choices) |
| Condition checks | [Cheat Sheet → Checks]({{< ref "cheatsheet" >}}#checks) |
| Variable operations | [Cheat Sheet → Variables]({{< ref "cheatsheet" >}}#variables) |
| Item system | [Cheat Sheet → Items]({{< ref "cheatsheet" >}}#items) |
| Scene transitions | [Cheat Sheet → Scenes]({{< ref "cheatsheet" >}}#scenes) |
| Media resources | [Cheat Sheet → Media]({{< ref "cheatsheet" >}}#media) |

### I want to integrate...

| Need | Reference |
|------|-----------|
| Read game state | [Runtime State]({{< ref "../api/runtime-state" >}}) |
| C/C++ embedding | [C API]({{< ref "../api/c-api" >}}) |
| Implement a renderer | [Platform Integration]({{< ref "../platform" >}}) |
| Save/Load | [Runtime State → Save Format]({{< ref "../api/runtime-state" >}}#save-format) |

### Got an error...

| Error Type | Reference |
|------------|-----------|
| Syntax error | [Command Reference]({{< ref "../syntax/commands" >}}) |
| Undefined variable | [Variables]({{< ref "../syntax/variables" >}}) |
| Scene jump failed | [Scenes & Labels]({{< ref "../syntax/scenes" >}}) |
| Runtime crash | [C API Error Handling]({{< ref "../api/c-api" >}}) |
