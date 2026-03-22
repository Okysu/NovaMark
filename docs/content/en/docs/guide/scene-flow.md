---
title: "Scene Flow"
weight: 7
---

# Scene Flow

As your story grows longer, you'll quickly face a problem:

> How do you organize scenes without writing repetitive code?

NovaMark provides two key mechanisms to solve this:

1. **Scene Labels** — Break scenes into smaller segments
2. **@call / @return** — Call a scene like a function, then return after it finishes

---

## 1. Review: Scenes and Labels

You already know scenes are defined with `#scene_xxx`:

```nvm
#scene_forest "Misty Forest"
```

Inside a scene, you can use labels (starting with `.`) to create sections:

```nvm
#scene_forest "Misty Forest"

> You walk into thick fog.

.look_around
> You look around, but can't see anything clearly.

.call_help
> You shout, but only your echo responds.
```

Labels serve two purposes:

- Let choices jump to specific positions within a scene
- Break long narratives into manageable segments

---

## 2. Simple Jumps: `->`

Use `->` to jump to another location:

```nvm
-> scene_tower
```

This means "jump directly to `scene_tower` and don't come back."

This type of jump is suitable for:

- Chapter transitions
- Linear story progression
- Entering a new area with no return

---

## 3. Call and Return: `@call` / `@return`

But sometimes what you want is:

> Execute a segment of story, then return to where you were.

This is what `@call` and `@return` are for.

### Basic Usage

```nvm
#scene_main "Main Flow"

> You enter a small hut.

? What do you want to do?
- [Talk to the shopkeeper] -> .talk
- [Open the shop] -> .shop
- [Leave] -> scene_village

.talk
Shopkeeper: How can I help you?
-> .back

.shop
@call shop_scene
.back
> You leave the shop.

#scene_village "Village"
> You return to the village square.
```

### What Happens

When the player chooses "Open the shop":

1. Execution reaches `@call shop_scene`
2. Jumps to `shop_scene`, starts executing that content
3. When `shop_scene` reaches `@return`
4. Returns to the line after `@call`, which is the `.back` label

---

## 4. How to Write a Callable Scene

A scene that can be called with `@call` typically looks like this:

```nvm
#shop_scene "Shop"

> The shopkeeper smiles and shows you the goods.

? What do you want to buy?
- [Healing Potion - 20 gold] -> .buy_potion if item_count("gold") >= 20
- [Leave] -> .leave

.buy_potion
@take gold 20
@give healing_potion 1
Shopkeeper: Here's your potion.
-> .continue

.leave
Shopkeeper: Come again soon.
-> .continue

.continue
@return
```

### Key Points

1. The scene ends with `@return`
2. All branches eventually flow to `@return`
3. This way the caller can correctly return to the original position

---

## 5. Difference Between @call and Simple Jump

| Method | Behavior | Use Case |
|--------|----------|----------|
| `-> scene_xxx` | Jump there, don't come back | Chapter transitions, entering new areas |
| `@call scene_xxx` | Jump there, return when done | Shops, battles, reusable segments |

You can think of it as:

- `->` is "go there and stay there"
- `@call` is "go there to handle something, then come back"

---

## 6. When to Use @call

### Good Use Cases for @call

**Shop / Trading System**

```nvm
@call shop_scene
```

The shop logic is the same in every town and chapter.

**Battle Subroutines**

```nvm
@call battle_wolves
```

After the battle ends, return to the main story.

**Common Events**

```nvm
@call random_encounter
```

Random encounter events, return to main story after handling.

**Rest / Recovery Points**

```nvm
@call inn_rest
```

Rest at an inn, recover health, then continue adventuring.

### Not Suitable for @call

**Main Chapter Transitions**

```nvm
-> scene_chapter_2
```

This is a permanent scene change, no need to return.

**Entering a New Map**

```nvm
-> scene_dungeon
```

The player will explore the new map for a while, it's not "finish something and return."

---

## 7. A Complete Example

```nvm
---
title: Example Game
---

@var gold = 100

@item healing_potion
  name: "Healing Potion"
  default_value: 0
@end

#scene_start "Beginning"

> You stand in the village square.

? Where do you want to go?
- [Go to the shop] -> .go_shop
- [Rest at the inn] -> .go_inn
- [Leave the village] -> scene_forest

.go_shop
@call shop_scene
-> .back

.go_inn
@call inn_scene
-> .back

.back
> You return to the square.
-> scene_start

#shop_scene "Shop"

Shopkeeper: Welcome!

? What would you like to buy?
- [Healing Potion - 30 gold] -> .buy if item_count("gold") >= 30
- [Leave] -> .leave

.buy
@take gold 30
@give healing_potion 1
Shopkeeper: Here's your potion.
-> .done

.leave
Shopkeeper: Come again soon.

.done
@return

#inn_scene "Inn"

> You enter a cozy inn.

Innkeeper: 10 gold for a night. Would you like to stay?

? 
- [Stay] -> .stay if item_count("gold") >= 10
- [Leave] -> .leave

.stay
@take gold 10
> You have a good night's sleep and wake up refreshed.
-> .done

.leave
> You decide not to stay.

.done
@return

#scene_forest "Forest"

> You leave the village and head into the unknown forest.
```

This example demonstrates:

- Main scene calls shop and inn with `@call`
- Shop and inn scenes return with `@return`
- All branches eventually reach `@return`

---

## 8. Nested Calls

`@call` can be nested, meaning:

```nvm
#scene_a
@call scene_b

#scene_b
@call scene_c

#scene_c
@return  -- returns to scene_b
@return  -- returns to scene_a (requires @return in scene_b too)
```

However, it's recommended not to nest too deeply to maintain code readability.

---

## 9. Summary

Just remember these points:

1. `#scene_xxx` defines a scene
2. `.label` defines a label within a scene
3. `-> target` permanent jump, no return
4. `@call scene` call a scene, return when done
5. `@return` return from a called scene

Using these well, your script can:

- Avoid repetitive code
- Keep structure clear
- Be easy to maintain and extend

---

## What's Next

Now you know how to organize scenes and reuse story segments.

The natural next questions are:

- How do you mark key player choices?
- How do you trigger different endings?

Next page:

- [Endings and Flags](./endings-and-flags/)
