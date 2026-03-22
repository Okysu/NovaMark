---
title: "Characters and Emotions"
weight: 5
---

# Characters and Emotions

When writing interactive stories, one of the most important questions is:

> Who is speaking? What emotion are they feeling right now?

NovaMark uses `@char` to define characters and `Character[emotion]:` to express dialogue with emotions.

---

## Why Define Characters First?

You might have already written dialogue like this:

```nvm
Shen Yan: The weather is nice today.
```

This works. But if your story has 5 characters, each with 3 emotional states, things get messy:

- Who is who?
- What color should each character's name be displayed in?
- Which sprite corresponds to which emotion?

`@char` solves these problems.

---

## The Simplest Character Definition

```nvm
@char Lin Xiao
  color: #E8A0BF
@end
```

This means:

- The character's name is `Lin Xiao`
- When she speaks, her name will be displayed in pink `#E8A0BF`

---

## Adding Sprites to Characters

If your game uses a visual novel style, you might want to assign character sprites.

```nvm
@char Lin Xiao
  color: #E8A0BF
  sprite_default: linxiao_normal.png
@end
```

This means:

- `sprite_default` is the default sprite
- When you write `Lin Xiao: dialogue`, the client knows which image to use

---

## Emotions and Sprite Switching

Characters don't always have just one expression.

NovaMark handles this in an intuitive way:

### Defining Multiple Emotion Sprites

```nvm
@char Lin Xiao
  color: #E8A0BF
  sprite_default: linxiao_normal.png
  sprite_happy: linxiao_happy.png
  sprite_sad: linxiao_sad.png
  sprite_angry: linxiao_angry.png
@end
```

### Using Emotions in Dialogue

```nvm
Lin Xiao: Hello.                    # Uses sprite_default
Lin Xiao[happy]: This is wonderful!  # Uses sprite_happy
Lin Xiao[sad]: I don't know what to do...  # Uses sprite_sad
Lin Xiao[angry]: How could you!      # Uses sprite_angry
```

You don't need to write manual "switch sprite" commands. Just write `Character[emotion]:` and the client knows which image to use.

---

## Emotion Naming Rules

NovaMark doesn't restrict which emotion names you can use. Name them according to your game's needs.

### Common Examples

```nvm
@char Shen Yan
  color: #4A90D9
  sprite_default: shenyan_normal.png
  sprite_happy: shenyan_happy.png
  sprite_worried: shenyan_worried.png
  sprite_surprised: shenyan_surprised.png
@end
```

### You Can Also Use Non-English Emotion Names

```nvm
@char Lin Xiao
  color: #E8A0BF
  sprite_default: linxiao_normal.png
  sprite_kaixin: linxiao_happy.png
  sprite_nanguo: linxiao_sad.png
@end
```

Then use them like this:

```nvm
Lin Xiao[kaixin]: That's great!
Lin Xiao[nanguo]: Why did this happen...
```

The only requirement is:

- Define with `sprite_emotion_name`
- Use with `Character[emotion_name]`

---

## A Complete Example

```nvm
---
title: Character Example
---

@char Lin Xiao
  color: #E8A0BF
  description: A traveler from afar
  sprite_default: linxiao_normal.png
  sprite_happy: linxiao_happy.png
  sprite_sad: linxiao_sad.png
@end

@char Shen Yan
  color: #4A90D9
  description: The silent tower keeper
  sprite_default: shenyan_normal.png
  sprite_worried: shenyan_worried.png
@end

#scene_meeting "The Encounter"

@bg forest_clearing.png

> Deep in the forest, you meet for the first time.

Lin Xiao: Hello, where is this place?

Shen Yan: ...This is the area around the Star Tower.

Lin Xiao[happy]: The Star Tower! I finally found it!

Shen Yan[worried]: You shouldn't be here.

? Lin Xiao seems to sense something is wrong.
- [Ask more] -> .ask_more
- [Stay silent] -> .stay_silent

.ask_more
Lin Xiao: What happened?

.stay_silent
> You decide to observe the situation first.
```

---

## All Character Definition Fields

Currently `@char` supports these fields:

| Field | Purpose | Required |
|---|---|---|
| `color` | Character name display color | Recommended |
| `description` | Character description | Optional |
| `sprite_default` | Default sprite | Recommended |
| `sprite_*` | Emotion sprites | Add as needed |

---

## Tips for Writing Dialogue

### Dialogue Without Emotion

```nvm
Lin Xiao: The weather is nice today.
```

This uses `sprite_default`.

### Dialogue With Emotion

```nvm
Lin Xiao[happy]: The weather is nice today!
```

This uses `sprite_happy` (if you defined it).

### Consecutive Emotions

```nvm
Lin Xiao[happy]: That's great!
Lin Xiao: Let's go.
```

The second line doesn't specify an emotion, so it switches back to `sprite_default`.

---

## How Clients Handle This Information

NovaMark's design principle is:

> The script provides data and state, the client decides how to present it.

So when you write:

```nvm
@char Lin Xiao
  color: #E8A0BF
  sprite_happy: linxiao_happy.png
@end

Lin Xiao[happy]: Hello!
```

The client receives state like this:

- Current speaker: Lin Xiao
- Current emotion: happy
- Character color: #E8A0BF
- Sprite to load: linxiao_happy.png

Then the client can:

- Switch sprites in a visual novel interface
- Change name colors in a chat interface
- Ignore this information in plain text mode

The script doesn't need to care about how it's ultimately displayed.

---

## What's Next

Now you know how to define characters and emotions.

On the next page, we'll cover:

- How to control background images
- How to display sprites
- How to play music and sound effects

Next page:

- [Media and Presentation](./media-and-presentation/)
