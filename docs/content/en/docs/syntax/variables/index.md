---
title: "Variables and Items"
weight: 3
---

# Variables and Items

## Variable Definition

```nvm
@var hp = 100
@var gold = 50
@var name = "林晓"
@var is_alive = true
```

## Variable Modification

```nvm
@set hp = 80
@set gold = gold + 10
@set hp = hp - 20
```

## Item Definition

```nvm
@item healing_potion
  name: Healing Potion
  description: Restores 50 HP
  rarity: common
@end
```

## Item Operations

### Give Item

```nvm
@give healing_potion 1
@give gold 100
```

### Remove Item

```nvm
@take healing_potion 1
```

## Check Items

Use in conditions:

```nvm
@check has_item("healing_potion")
success
  > You used a healing potion.
  @take healing_potion 1
  @set hp = hp + 50
fail
  > You don't have any potions.
endcheck
```

### Check Item Count

```nvm
@check item_count("gold") >= 100
success
  > You have enough gold.
fail
  > Not enough gold.
endcheck
```

## Built-in Functions

| Function | Description |
|----------|-------------|
| `has_item("item_id")` | Check if player has item |
| `item_count("item_id")` | Get item count |
| `has_ending("ending_id")` | Check if ending was triggered |
| `has_flag("flag_id")` | Check if flag was set |
| `roll("2d6+3")` | Dice check |
