---
title: "Endings and Flags"
weight: 8
---

# Endings and Flags

When writing multi-ending stories, two key questions arise:

> Which ending did the player trigger?
> What key choices did the player make during their journey?

NovaMark uses `@ending` and `@flag` to answer these questions.

---

## 1. `@flag`: Recording Key Choices

`@flag` is used to mark "this thing happened."

Its semantics are simple: set a switch that can later be checked to see if it was turned on.

### Basic Usage

```nvm
@flag saved_village
@flag met_spirit
@flag chose_dark_path
```

### When to Use `@flag`

Good for recording things that:

- Don't need specific values, just "yes/no"
- Are cross-scene important decisions
- Key events that affect future story direction

For example:

```nvm
// Player saved the villagers
> You helped the villagers fight off the bandits.
@flag saved_village

// Later, in another scene, you can check
if has_flag("saved_village")
  Village Elder: Our savior, you've finally returned!
else
  Village Elder: I don't know you.
endif
```

### Checking Flags

Use the `has_flag()` function:

```nvm
if has_flag("met_spirit")
  Spirit: We meet again.
else
  > A strange light emanates from deep within the forest...
endif
```

Can also be used in conditional choices:

```nvm
? How do you handle this hidden door?
- [Leave] -> .leave
- [Explore deeper] -> .secret_room if has_flag("found_map")
```

---

## 2. `@ending`: Triggering Endings

`@ending` marks that the story has reached a specific ending.

Its semantics are:

> The story ends here, this is an official conclusion.

### Basic Usage

```nvm
@ending good_ending
@ending bad_ending
@ending true_ending
```

### Ending Trigger Example

```nvm
#scene_final "Fate's Choice"

? Standing at the crossroads of destiny, you choose:
- [Save the world] -> .save_world
- [Seek power] -> .seek_power

.save_world
> You sacrifice yourself to save everyone.
@flag heroic_sacrifice
@ending hero_ending

.seek_power
> You gain endless power, but lose all your friends.
@ending dark_ending
```

### Endings Are Recorded

Triggered endings are saved and can be used for:

- Playthrough reviews, showing unlocked endings
- New playthroughs unlocking special content based on history
- Achievement systems

Check with `has_ending()`:

```nvm
if has_ending("true_ending")
  Mysterious Voice: You have already seen the truth...
  -> .secret_epilogue
endif
```

---

## 3. Difference Between `@flag` and `@ending`

Many ask: aren't they both "markers"?

Yes, but with different semantics:

| Directive | Meaning | Purpose |
|-----------|---------|---------|
| `@flag` | This thing happened | Record intermediate events |
| `@ending` | The story ends here | Mark official conclusions |

Simply put:

- `@flag` is a milestone on the journey
- `@ending` is the destination

---

## 4. Multi-Ending Design Patterns

### Pattern 1: Branching

Players make choices at key nodes, directly entering different endings.

```nvm
#scene_climax "Final Choice"

? Facing the sealed ancient god, you decide to:
- [Reseal it] -> .seal
- [Release it] -> .release
- [Talk to it] -> .talk if has_flag("learned_truth")

.seal
> You seal the ancient god forever, and peace returns to the world.
@ending peace_ending

.release
> The ancient god awakens, and the world falls into chaos.
@ending chaos_ending

.talk
> Ancient God: You are the first to try to understand me...
> The god's power merges with your soul, and you become the new guardian.
@ending guardian_ending
```

### Pattern 2: Cumulative

Player's multiple choices accumulate to affect the final result.

```nvm
// Define a variable at game start
@var karma = 0

// Each choice affects karma
// Scene A
? A beggar by the road asks you for help:
- [Give money] -> .give
- [Ignore] -> .ignore

.give
@set karma = karma + 10
-> next_scene

.ignore
@set karma = karma - 5
-> next_scene

// Scene B
? Someone is being bullied:
- [Help them] -> .help
- [Watch] -> .watch

.help
@flag helped_stranger
@set karma = karma + 15
-> next_scene

.watch
@set karma = karma - 10
-> next_scene

// Final ending based on accumulated value
#scene_ending "Epilogue"

if karma >= 50
  > Your good deeds have moved the world.
  @ending light_ending
else if karma >= 20
  > You live a peaceful and contented life.
  @ending neutral_ending
else
  > You walk alone in darkness.
  @ending dark_ending
endif
```

### Pattern 3: Hidden Condition

Certain endings require specific conditions to trigger.

```nvm
#scene_final "The Abyss"

if has_flag("saved_spirit") and has_flag("found_truth") and karma >= 30
  > The spirit appears before you and shows you the true path.
  @flag true_route_open
  -> .true_ending
else
  -> .normal_route
endif

.true_ending
> You finally understand the truth of the world.
@ending true_ending

.normal_route
> You complete your journey, but doubts remain in your heart.
@ending normal_ending
```

---

## 5. Complete Example: Three-Ending Story

```nvm
---
title: Tower of Fate
---

@var courage = 10
@var wisdom = 10

@char Traveler
  color: #87CEEB
@end

#scene_start "Tower Entrance"

Traveler: Legend says the top of this tower holds the truth of the world.

? How do you enter?
- [Main entrance] -> .main_entrance
- [Secret passage on the side] -> .secret_passage if has_item("old_map")

.main_entrance
@flag entered_frontally
> You push open the heavy door.
-> scene_tower

.secret_passage
@flag found_shortcut
Traveler: You actually know about this secret passage!
-> scene_tower_core

#scene_tower "Inside the Tower"

? You encounter an injured spirit. You:
- [Help it] -> .help_spirit
- [Ignore it] -> .ignore_spirit

.help_spirit
@flag saved_spirit
@set wisdom = wisdom + 10
Spirit: Thank you... I will remember this kindness.
-> scene_choice

.ignore_spirit
@set courage = courage + 5
> You continue forward.
-> scene_choice

#scene_choice "Critical Choice"

Traveler: The top is within reach, but I sense danger.

? The final fork in the road:
- [Go alone] -> .alone
- [Go together with Traveler] -> .together

.alone
@flag chose_solitude
-> scene_ending

.together
@flag chose_partnership
-> scene_ending

#scene_ending "Fate"

if has_flag("saved_spirit") and has_flag("chose_partnership")
  // True ending
  > The spirit's power and Traveler's trust reveal the truth at the tower's peak.
  Spirit: This is the origin of the world...
  Traveler: We witnessed it together.
  @ending true_ending
  
else if has_flag("chose_partnership")
  // Good ending
  > You and Traveler reach the top together.
  Traveler: We didn't see all the truth, but this journey itself is the answer.
  @ending good_ending
  
else if has_flag("saved_spirit")
  // Normal ending
  > The spirit appears at the last moment and saves your life.
  Spirit: Your kindness deserves a reward.
  @ending spirit_ending
  
else
  // Tragic ending
  > You stand alone at the top of the tower, surrounded by emptiness.
  > No truth, no answers, only endless loneliness.
  @ending lonely_ending
endif
```

---

## 6. Design Tips

### Number of Endings

- Short games: 2-3 endings
- Medium games: 3-5 endings
- Long games: 5-10 endings

Too many endings can exhaust players, too few reduces exploration motivation.

### Ending Names

Use meaningful IDs for easy management:

```nvm
@ending true_ending      // True ending
@ending good_ending      // Good ending
@ending normal_ending    // Normal ending
@ending bad_ending       // Bad ending
@ending secret_ending    // Hidden ending
```

### Make Flags Purposeful

Don't overuse `@flag`, only mark truly important nodes. 10-20 flags in a game is usually enough.

---

## Summary

- `@flag` records important events during the journey, check with `has_flag()`
- `@ending` marks official story conclusions, check with `has_ending()`
- Multi-ending stories can use branching, cumulative, or hidden condition patterns
- Reasonably control the number of endings, make each one meaningful

You've now mastered NovaMark's core features and can start creating your own interactive stories.

---

## What's Next

Congratulations! You've completed the core creator's guide for NovaMark!

If you want to dive deeper into more details, we recommend looking at:

- [Syntax Reference](../syntax/) - detailed syntax manual and command reference
- [Quick Reference](../reference/) - quick lookup for syntax, state, API & configuration
- [Installation Guide](../getting-started/installation/) - set up your development environment
- [Quickstart](../getting-started/quickstart/) - create a project from scratch

Now, start creating your interactive stories!
