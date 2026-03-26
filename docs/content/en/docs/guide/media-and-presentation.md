---
title: "Media and Presentation"
weight: 6
---

# Media and Presentation

By this chapter, you can already tell an interactive story with text.

But if you want players to "see" and "hear" your world, you need to understand media commands.

NovaMark's media commands are very simple:

- `@bg` - Background
- `@sprite` - Sprites / Character images
- `@bgm` - Background music
- `@sfx` - Sound effects

---

## First, Understand: Resource Paths

Before writing media commands, you need to know where resources are placed.

NovaMark doesn't enforce a specific directory structure, but it's recommended to configure base paths in the metadata block:

```nvm
---
title: Misty Forest
base_bg_path: assets/bg/
base_sprite_path: assets/sprites/
base_audio_path: assets/audio/
---
```

The benefit of doing this:

- You write `@bg forest.png`, and the engine knows to look in `assets/bg/forest.png`
- When deploying to different platforms, you only need to change the base paths, not the script

If you don't configure base paths, resource paths are treated as relative paths.

---

## Background Images: `@bg`

Backgrounds are the most basic visual element.

### Basic Usage

```nvm
@bg forest_night.png
```

This tells the engine: change the background to `forest_night.png`.

### Adding Transition Effects

```nvm
@bg forest_night.png transition:fade
```

Transition effects are implemented by the client. Common ones include:

- `fade` - Fade in/out
- `dissolve` - Dissolve
- `slide` - Slide

### Why Use Transitions

Switching backgrounds directly can feel abrupt. Adding transitions makes scene changes smoother.

---

## Sprites: `@sprite`

Sprites are character images displayed on top of the background.

### Showing a Sprite

```nvm
@sprite Lin Xiao show url:linxiao_default.png position:left
```

This means:

- Character name is `Lin Xiao`
- Show the sprite
- Image is `linxiao_default.png`
- Position is on the left

### Hiding a Sprite

```nvm
@sprite Lin Xiao hide
```

### Explicit `show` Semantics

- `@sprite Lin Xiao show url:linxiao_default.png position:left`: show or update the sprite with the provided properties
- `@sprite Lin Xiao show position:left`: if `@char Lin Xiao` defines `sprite_default`, NovaMark uses it automatically
- `@sprite Lin Xiao hide`: remove that character from the current sprite state

So in common cases, you do not need to repeat the image path every time. If you only want the character to enter with their default portrait, `show` is enough.

### Specifying Exact Position

You can also use coordinates:

```nvm
@sprite Lin Xiao show url:linxiao_happy.png x:70 y:100
```

NovaMark preserves the raw `x` and `y` values in runtime state and does not derive coordinates inside the VM.
How those values are interpreted is up to the host platform.

### Emotion Sprite Shortcut

If you set emotion sprites in your character definition, you can use them directly in dialogue:

```nvm
@char Lin Xiao
  sprite_default: linxiao_default.png
  sprite_happy: linxiao_happy.png
@end

#scene_test "Test"

Lin Xiao[happy]: The weather is great today!
```

`Lin Xiao[happy]` will automatically use the image defined in `sprite_happy`.

If you omit the emotion, for example:

```nvm
Lin Xiao: I'm here.
```

NovaMark falls back to `sprite_default`.

### Do sprites stay on screen forever?

Current NovaMark rules are:

- Within the same scene, sprites remain until you explicitly write `@sprite CharacterName hide`
- When switching to a new scene, the current scene's sprites are cleared automatically

This matches the common VN mental model more closely: **scene changes clear the stage automatically, while within a scene you control portraits as needed**.

### What does sparse sprite state mean?

NovaMark only exports sprite fields that were explicitly set by the author.

For example:

```nvm
@sprite Lin Xiao show position:left opacity:0.8
```

The host receives:

- `id`
- `url`
- `position`
- `opacity`

But NovaMark does not inject extra defaults such as `x=0`, `y=0`, or `zIndex=0`.

Recommended host fallback order:

1. Use `x/y` if present
2. Otherwise use `position`
3. Otherwise use the host's default dialogue portrait layout

---

## Background Music: `@bgm`

BGM sets the atmosphere.

### Basic Usage

```nvm
@bgm ambient_forest.mp3
```

### Loop Playback

```nvm
@bgm ambient_forest.mp3 loop:true
```

In most cases, BGM should loop.

### Adjusting Volume

```nvm
@bgm ambient_forest.mp3 loop:true volume:0.3
```

Volume ranges from `0.0` to `1.0`. `0.3` means 30% volume.

### Stopping Music

```nvm
@bgm stop
```

---

## Sound Effects: `@sfx`

Sound effects are for short sounds, like door opening or footsteps.

### Basic Usage

```nvm
@sfx door_open.mp3
```

Sound effects don't loop by default, they play once and stop.

### Adjusting Volume

```nvm
@sfx door_open.mp3 volume:0.5
```

---

## A Complete Scene Example

Combining everything above:

```nvm
---
title: Star Tower
base_bg_path: assets/bg/
base_sprite_path: assets/sprites/
base_audio_path: assets/audio/
---

@char Lin Xiao
  color: #E8A0BF
  sprite_default: linxiao_default.png
  sprite_happy: linxiao_happy.png
@end

#scene_tower "Star Tower"

@bg tower_entrance.png transition:fade
@bgm mystery.mp3 loop:true volume:0.4

> An ancient tower stands under the moonlight.

@sprite Lin Xiao show url:linxiao_default.png position:left

Lin Xiao: Is this the legendary Star Tower?

? What do you want to do first?
- [Push the door open] -> .enter
- [Walk around the tower] -> .walk_around

.enter
@sfx door_creak.mp3 volume:0.6
@bg tower_interior.png transition:fade
> The heavy wooden door slowly opens.
-> .continue

.walk_around
Lin Xiao[happy]: Look, there are strange runes on the wall.
-> .continue

.continue
> You feel a mysterious power.
```

This script includes:

- Background switching with transitions
- BGM loop playback
- Sprite display
- Sound effect triggering
- Emotion sprites

---

## Common Questions

### 1. What resource formats are supported?

This depends on your client implementation. Common support:

| Type | Common Formats |
|------|----------------|
| Backgrounds | PNG, JPG |
| Sprites | PNG (with transparency) |
| BGM | MP3, OGG |
| Sound Effects | MP3, WAV |

### 2. What happens if the resource path is wrong?

The engine outputs an error message but won't crash. Players might see/hear a "missing resource" indicator.

### 3. Can multiple sound effects play at the same time?

NovaMark doesn't restrict this, but the actual behavior depends on the client. Most clients support layered sound effects.

### 4. Does BGM switch automatically?

No. You need to explicitly write `@bgm` in your script to switch or stop music.

---

## Media Command Quick Reference

| Command | Purpose | Example |
|---------|---------|---------|
| `@bg` | Switch background | `@bg forest.png transition:fade` |
| `@sprite show` | Show sprite | `@sprite Lin Xiao show url:x.png position:left` |
| `@sprite hide` | Hide sprite | `@sprite Lin Xiao hide` |
| `@bgm` | Play music | `@bgm bgm.mp3 loop:true volume:0.5` |
| `@bgm stop` | Stop music | `@bgm stop` |
| `@sfx` | Play sound effect | `@sfx click.mp3` |

---

## Media Commands and Client Relationship

Remember NovaMark's design principle:

> The script is responsible for "when to play what", the client is responsible for "how to play".

This means:

- You write `@bg forest.png transition:fade` in your script
- The client decides how many seconds fade takes and what curve to use
- Different platforms can have different presentation effects

The benefit: the same script can be used on Web, desktop, and mobile without writing different versions for each platform.

---

## What's Next

Now you know how to make your story "visible and audible".

The natural next questions are:

- How do you jump between multiple scenes?
- How do you organize a complete story structure?

Next page:

- [Scene Flow](./scene-flow/)
