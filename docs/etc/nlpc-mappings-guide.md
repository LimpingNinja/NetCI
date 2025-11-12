# Mappings: Looking Things Up by Name

## What's a Mapping?

Imagine you're running a shop. You have items with prices:
- Sword costs 50 gold
- Shield costs 30 gold  
- Potion costs 10 gold

With an array, you'd need TWO lists—one for item names and one for prices. Then you'd have to search through the names to find the matching price. What a pain!

**Mappings solve this problem.** A mapping lets you look things up directly by name (or number, or even an object). It's like a phonebook: you look up a name and immediately get the phone number.

Think of a mapping like:
- **A dictionary**: Look up a word, get its definition
- **A phonebook**: Look up a name, get a phone number
- **A price list**: Look up an item, get its price
- **Player stats**: Look up "strength", get the value

## Creating Your First Mapping

Mappings use `([` and `])` with colons between keys and values:

```c
// An empty mapping (like a blank phonebook)
mapping my_map = ([ ]);

// A price list
mapping prices = ([ 
  "sword": 50, 
  "shield": 30, 
  "potion": 10 
]);

// Player stats
mapping stats = ([ 
  "strength": 15, 
  "dexterity": 12, 
  "wisdom": 10 
]);
```

## Important: Always Initialize!

Just like arrays, mappings start as `0` until you give them a value:

```c
mapping my_map;  // Oops! This is 0, not an empty mapping
my_map["key"] = "value";  // ERROR! Can't use it yet

// Do this instead:
mapping my_map = ([ ]);  // Now it's ready to use
my_map["key"] = "value";  // Works perfectly!
```

## Looking Things Up

The magic of mappings is how easy they are to use:

```c
mapping prices = ([ "sword": 50, "shield": 30, "potion": 10 ]);

// Look up a price
int sword_price = prices["sword"];  // 50
int potion_price = prices["potion"];  // 10

// Show the player
this_player().listen("A sword costs " + itoa(sword_price) + " gold.\n");
```

### What if the Key Doesn't Exist?

If you look up something that's not there, you get `0`:

```c
mapping prices = ([ "sword": 50, "shield": 30 ]);

int price = prices["dragon"];  // Returns 0 (doesn't exist)
```

**Tip:** Use the `member()` efun to check if a key exists:

```c
if (member(prices, "sword")) {
  this_player().listen("We have swords in stock!\n");
}
```

## Adding and Changing Values

Just assign to a key—if it doesn't exist, it's created automatically:

```c
mapping inventory = ([ ]);

// Add new items
inventory["sword"] = 5;    // Now have 5 swords
inventory["shield"] = 3;   // Now have 3 shields

// Update existing items
inventory["sword"] = 10;   // Now have 10 swords

// Add more
inventory["sword"] += 2;   // Now have 12 swords
```

This is called **auto-vivification**—keys spring into existence when you use them!

## Removing Keys

Use `map_delete()` to remove a key:

```c
mapping inventory = ([ "sword": 5, "shield": 3, "potion": 10 ]);

map_delete(inventory, "shield");  // Remove shields

// inventory is now ([ "sword": 5, "potion": 10 ])
```

## How Many Keys?

Use `sizeof()` to count keys:

```c
mapping stats = ([ "strength": 15, "dexterity": 12, "wisdom": 10 ]);
int count = sizeof(stats);  // count is 3

this_player().listen("You have " + itoa(count) + " stats.\n");
```

## Merging Mappings

Want to combine two mappings? Use the `+` operator:

```c
mapping defaults = ([ "timeout": 30, "retries": 3, "debug": 0 ]);
mapping user_settings = ([ "timeout": 60, "debug": 1 ]);

mapping final = defaults + user_settings;
// Result: ([ "timeout": 60, "retries": 3, "debug": 1 ])
```

### The Merge Rule: Right Wins!

When you merge mappings, **the right-hand mapping wins** if there are duplicate keys:

```c
mapping m1 = ([ "a": 1, "b": 2, "c": 3 ]);
mapping m2 = ([ "b": 99, "c": 88, "d": 4 ]);

mapping m3 = m1 + m2;
// Result: ([ "a": 1, "b": 99, "c": 88, "d": 4 ])
//                    ^^^^ m2 wins!  ^^^^ m2 wins!
```

Think of it like updating settings—the new values override the old ones.

### Shorthand: `+=`

```c
mapping config = ([ "timeout": 30, "retries": 3 ]);

// Update multiple settings at once
config += ([ "timeout": 60, "debug": 1 ]);
// config is now ([ "timeout": 60, "retries": 3, "debug": 1 ])
```

## Removing Keys in Bulk

Use the `-` operator to remove multiple keys:

```c
mapping data = ([ "a": 1, "b": 2, "c": 3, "d": 4 ]);
mapping to_remove = ([ "b": 0, "d": 0 ]);

mapping result = data - to_remove;
// Result: ([ "a": 1, "c": 3 ])
```

**Note:** The values in the second mapping don't matter—only the keys are checked!

```c
// These all do the same thing:
data - ([ "b": 0, "d": 0 ])
data - ([ "b": 999, "d": "anything" ])
data - ([ "b": "ignored", "d": "also ignored" ])
```

### Shorthand: `-=`

```c
mapping flags = ([ "temp": 1, "active": 1, "debug": 1 ]);

// Remove temporary flags
flags -= ([ "temp": 0, "debug": 0 ]);
// flags is now ([ "active": 1 ])
```

## Real-World Examples

### Example: A Shop System

```c
mapping prices;
mapping stock;

create() {
  prices = ([ 
    "sword": 50, 
    "shield": 30, 
    "potion": 10 
  ]);
  
  stock = ([ 
    "sword": 5, 
    "shield": 3, 
    "potion": 20 
  ]);
}

void buy_item(string item) {
  // Check if we sell it
  if (!member(prices, item)) {
    this_player().listen("We don't sell that.\n");
    return;
  }
  
  // Check if in stock
  if (stock[item] < 1) {
    this_player().listen("Sorry, we're out of stock!\n");
    return;
  }
  
  // Make the sale
  int price = prices[item];
  this_player().listen("That'll be " + itoa(price) + " gold.\n");
  stock[item] -= 1;
}

void show_shop() {
  array items;
  int i;
  
  items = keys(prices);  // Get all item names
  this_player().listen("=== Shop Inventory ===\n");
  
  for (i = 0; i < sizeof(items); i++) {
    string item = items[i];
    this_player().listen(
      item + ": " + itoa(prices[item]) + " gold " +
      "(" + itoa(stock[item]) + " in stock)\n"
    );
  }
}
```

### Example: Player Statistics

```c
mapping stats;

create() {
  stats = ([ 
    "strength": 10,
    "dexterity": 10,
    "constitution": 10,
    "intelligence": 10,
    "wisdom": 10,
    "charisma": 10
  ]);
}

void modify_stat(string stat_name, int amount) {
  if (!member(stats, stat_name)) {
    this_player().listen("Unknown stat: " + stat_name + "\n");
    return;
  }
  
  stats[stat_name] += amount;
  
  // Keep within bounds
  if (stats[stat_name] < 1) stats[stat_name] = 1;
  if (stats[stat_name] > 20) stats[stat_name] = 20;
  
  this_player().listen(stat_name + " is now " + itoa(stats[stat_name]) + "\n");
}

int query_stat(string stat_name) {
  return stats[stat_name];  // Returns 0 if doesn't exist
}

void show_stats() {
  array stat_names;
  int i;
  
  stat_names = keys(stats);
  this_player().listen("=== Your Stats ===\n");
  
  for (i = 0; i < sizeof(stat_names); i++) {
    string stat = stat_names[i];
    this_player().listen(
      stat + ": " + itoa(stats[stat]) + "\n"
    );
  }
}
```

### Example: Configuration with Defaults

```c
mapping get_config(mapping user_config) {
  mapping defaults;
  
  // Set up sensible defaults
  defaults = ([
    "timeout": 30,
    "max_retries": 3,
    "debug_mode": 0,
    "auto_save": 1,
    "sound_enabled": 1
  ]);
  
  // User settings override defaults
  return defaults + user_config;
}

// Usage:
mapping my_config = get_config(([ "timeout": 60, "debug_mode": 1 ]));
// Result: ([ "timeout": 60, "max_retries": 3, "debug_mode": 1, 
//            "auto_save": 1, "sound_enabled": 1 ])
```

## Looping Through Mappings

Use `keys()` to get all the keys, then loop through them:

```c
void show_inventory(mapping inventory) {
  array item_names;
  int i;
  
  item_names = keys(inventory);
  
  if (sizeof(item_names) == 0) {
    this_player().listen("Your inventory is empty.\n");
    return;
  }
  
  this_player().listen("=== Inventory ===\n");
  for (i = 0; i < sizeof(item_names); i++) {
    string item = item_names[i];
    int quantity = inventory[item];
    this_player().listen("  " + item + " x" + itoa(quantity) + "\n");
  }
}
```

You can also use `values()` to get all the values:

```c
mapping scores = ([ "Alice": 100, "Bob": 85, "Charlie": 92 ]);

array names = keys(scores);      // ({ "Alice", "Bob", "Charlie" })
array points = values(scores);   // ({ 100, 85, 92 })
```

## When to Use Mappings vs Arrays

### Use a Mapping When:
- You need to **look things up by name** (or other non-numeric key)
- You have **paired data** (name → value, item → price, etc.)
- You need **fast lookups** (mappings are super fast!)
- Keys might be **sparse** (not every number from 0-100 is used)

**Examples:**
- Player inventory (item name → quantity)
- Configuration settings (option name → value)
- Statistics (stat name → value)
- Prices (item name → cost)

### Use an Array When:
- You need things **in order** (first, second, third...)
- You're working with **lists** (players in a room, quest steps, etc.)
- Position/index matters (leaderboard rankings)

**Examples:**
- List of players in a room
- Quest objectives in order
- Command history
- Sorted high scores

### Can't Decide? Here's a Simple Test:

**Ask yourself:** "Do I look things up by name/key, or by position/number?"
- **By name** → Use a mapping
- **By position** → Use an array

## Things to Remember

### Always Initialize
```c
mapping m;  // This is 0, not a mapping!
m = ([ ]);  // NOW it's a mapping
```

### Right-Hand Wins on Merge
```c
m1 = ([ "a": 1, "b": 2 ]);
m2 = ([ "b": 99 ]);
m3 = m1 + m2;  // "b" becomes 99 (m2 wins!)
```

### Use member() to Check Existence
```c
if (member(my_map, "key")) {
  // Key exists
}
```

### Subtraction Only Cares About Keys
```c
m1 - ([ "key": 0 ])  // Removes "key"
m1 - ([ "key": 999 ])  // Also removes "key" (value doesn't matter)
```

### Single Key? Use Subscript!
```c
// Good - direct and fast
m["key"] = "value";

// Bad - slow and wasteful
m += ([ "key": "value" ]);
```

## Quick Reference

| What You Want | How To Do It | Example |
|---------------|--------------|---------|
| Make an empty mapping | `([ ])` | `mapping m = ([ ]);` |
| Make a mapping with data | `([ key: value ])` | `mapping prices = ([ "sword": 50 ]);` |
| Look up a value | `mapping[key]` | `price = prices["sword"];` |
| Add/change a value | `mapping[key] = value` | `prices["sword"] = 60;` |
| Check if key exists | `member(mapping, key)` | `if (member(prices, "sword"))` |
| Remove a key | `map_delete(mapping, key)` | `map_delete(prices, "sword");` |
| Count keys | `sizeof(mapping)` | `count = sizeof(prices);` |
| Get all keys | `keys(mapping)` | `items = keys(prices);` |
| Get all values | `values(mapping)` | `costs = values(prices);` |
| Merge mappings | `m1 + m2` | `all = defaults + user_config;` |
| Remove keys | `m1 - m2` | `filtered = data - to_remove;` |
| Merge in place | `m1 += m2` | `config += new_settings;` |
| Remove in place | `m1 -= m2` | `flags -= temp_flags;` |

---

**Previous:** [Arrays Guide](nlpc-arrays-guide.md) | **Next:** Continue building your MUD!
