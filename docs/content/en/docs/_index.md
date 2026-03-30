---
title: "NovaMark Docs"
type: docs
bookCollapseSection: false
---

# NovaMark Docs

Welcome to the NovaMark documentation.

NovaMark is a scripting language and runtime system for **text adventures, visual novels, and interactive narrative games**.

If you do not come from a programming background, the new documentation structure is designed to answer questions in this order:

- What is NovaMark?
- What should I learn first?
- What do scenes, dialogue, choices, and variables mean?
- What belongs to the script, and what belongs to the client UI?

## Choose your entry point

### 1. For Creators

If you want to write stories, characters, choices, and checks, start here.

You will learn:

- how to write your first story
- how to structure scenes
- how to understand state, variables, and items
- how to write choices, branches, and checks

Recommended entry points:

- [What is NovaMark]({{< ref "guide/what-is-novamark" >}})
- [Your First Story]({{< ref "guide/your-first-story" >}})
- [State, Variables, and Items]({{< ref "guide/state-and-variables" >}})
- [Choices, Branches, and Checks]({{< ref "guide/choices-and-checks" >}})

### 2. For Platform / Rendering / Distribution Integrators

If you are building a renderer, a runtime integration, or a game delivery pipeline, this is the better starting point.

You likely care about:

- how the VM advances
- how runtime state is exported
- how the host and script split responsibilities
- how to use C API / WASM / templates

This entry point focuses on those topics.

### 3. Reference

Use Reference when you already know what you want to look up.

You can find:

- syntax reference
- API reference
- runtime state fields
- project configuration

### 4. Glossary

Glossary explains recurring NovaMark terms.

For example:

- scene
- label
- check
- snapshot
- host
- rendering hint
