# Arrays: Working with Lists

## What's an Array?

Imagine you're organizing a party and need to keep track of who's coming. You could write down names on separate pieces of paper, but wouldn't it be easier to have one list? That's exactly what an array does—it's a numbered list that holds multiple values in order.

Think of an array like:
- **A playlist**: Song #1, Song #2, Song #3...
- **A queue at the bank**: Person #1, Person #2, Person #3...
- **Your inventory**: Item #1 (sword), Item #2 (shield), Item #3 (potion)...

The best part? Arrays in NLPC grow and shrink automatically. Add more items? No problem. Remove some? Easy!

## Creating Your First Array

Arrays use curly braces with parentheses: `({ })`

```c
// An empty array (like a blank list)
array my_list = ({ });

// A list of player names
array players = ({ "Alice", "Bob", "Charlie" });

// A list of numbers
array scores = ({ 100, 85, 92 });

// You can even mix different types!
array mixed = ({ "sword", 50, "A sharp blade" });
```

## Important: Always Initialize!

Here's a common mistake new builders make:

```c
array my_list;  // Oops! This is 0, not an empty array
my_list[0] = "item";  // ERROR! Can't use it yet

// Do this instead:
array my_list = ({ });  // Now it's ready to use
my_list[0] = "item";  // Works perfectly!
```

**Remember:** An array variable starts as `0` until you give it a value. Always set it to `({ })` before using it!

## Accessing Items in Your Array

Arrays are numbered starting from **0** (not 1!). Think of it like floors in a building where the ground floor is 0.

```c
array colors = ({ "red", "green", "blue" });

// Get items by their position (index)
string first = colors[0];   // "red" (position 0)
string second = colors[1];  // "green" (position 1) 
string third = colors[2];   // "blue" (position 2)
```

**Why start at 0?** It's a programming tradition. Just remember: first item = position 0!

### Changing Items

You can update any item in your array:

```c
array items = ({ "sword", "shield", "potion" });
items[1] = "helmet";  // Replace "shield" with "helmet"
// Now: ({ "sword", "helmet", "potion" })
```

### How Many Items?

Use `sizeof()` to count items:

```c
array players = ({ "Alice", "Bob", "Charlie" });
int count = sizeof(players);  // count is 3

this_player().listen("There are " + itoa(count) + " players.\n");
// Prints: "There are 3 players."
```

## Adding and Removing Items

### Adding Items (Concatenation)

Use the `+` operator to combine arrays:

```c
array weapons = ({ "sword", "bow" });
array armor = ({ "helmet", "shield" });
array all_items = weapons + armor;
// Result: ({ "sword", "bow", "helmet", "shield" })
```

Want to add just one item? Put it in `({ })`:

```c
array items = ({ "sword", "shield" });
items = items + ({ "potion" });  // Add a potion
// Result: ({ "sword", "shield", "potion" })
```

Or use the shorthand `+=`:

```c
array items = ({ "sword", "shield" });
items += ({ "potion" });  // Same thing, shorter!
```

### Removing Items (Subtraction)

Use the `-` operator to remove items:

```c
array items = ({ "sword", "shield", "potion", "rope" });
array to_remove = ({ "shield", "rope" });
items = items - to_remove;
// Result: ({ "sword", "potion" })
```

**Important:** This removes ALL matching items:

```c
array numbers = ({ 1, 2, 1, 3, 1 });
numbers = numbers - ({ 1 });
// Result: ({ 2, 3 }) - all the 1's are gone!
```

## Common Array Tasks

### Looping Through an Array

Want to do something with each item? Use a loop:

```c
void show_inventory(array items) {
  int i;
  
  this_player().listen("Your inventory:\n");
  
  for (i = 0; i < sizeof(items); i++) {
    this_player().listen("  " + itoa(i+1) + ". " + items[i] + "\n");
  }
}

// Usage:
array my_stuff = ({ "sword", "shield", "potion" });
show_inventory(my_stuff);

// Output:
// Your inventory:
//   1. sword
//   2. shield
//   3. potion
```

### Checking if an Item Exists

```c
int has_item(array items, string item) {
  int i;
  
  for (i = 0; i < sizeof(items); i++) {
    if (items[i] == item) {
      return 1;  // Found it!
    }
  }
  return 0;  // Not found
}

// Usage:
array inventory = ({ "sword", "shield", "potion" });

if (has_item(inventory, "potion")) {
  this_player().listen("You have a potion!\n");
}
```

### Building an Array Gradually

```c
array build_player_list() {
  array players;
  int i;
  
  players = ({ });  // Start empty
  
  // Add players one by one
  players += ({ "Alice" });
  players += ({ "Bob" });
  players += ({ "Charlie" });
  
  return players;  // ({ "Alice", "Bob", "Charlie" })
}
```

## Real-World Examples

### Example: A Simple Quest System

```c
// Track which quests a player has completed
array completed_quests;

create() {
  completed_quests = ({ });
}

void complete_quest(string quest_name) {
  // Add to completed list
  completed_quests += ({ quest_name });
  this_player().listen("Quest completed: " + quest_name + "!\n");
}

int has_completed(string quest_name) {
  int i;
  
  for (i = 0; i < sizeof(completed_quests); i++) {
    if (completed_quests[i] == quest_name) {
      return 1;
    }
  }
  return 0;
}

void show_quests() {
  int i;
  
  if (sizeof(completed_quests) == 0) {
    this_player().listen("You haven't completed any quests yet.\n");
    return;
  }
  
  this_player().listen("Completed quests:\n");
  for (i = 0; i < sizeof(completed_quests); i++) {
    this_player().listen("  - " + completed_quests[i] + "\n");
  }
}
```

### Example: Managing a Party

```c
array party_members;

create() {
  party_members = ({ });
}

void join_party(string player_name) {
  party_members += ({ player_name });
  this_player().listen(player_name + " joined the party!\n");
}

void leave_party(string player_name) {
  party_members -= ({ player_name });
  this_player().listen(player_name + " left the party.\n");
}

void show_party() {
  int i;
  
  this_player().listen("Party members (" + itoa(sizeof(party_members)) + "):\n");
  for (i = 0; i < sizeof(party_members); i++) {
    this_player().listen("  " + itoa(i+1) + ". " + party_members[i] + "\n");
  }
}
```

## Things to Remember

### Arrays Start at 0
The first item is always at position 0, not 1:
```c
array items = ({ "first", "second", "third" });
items[0]  // "first"
items[1]  // "second" 
items[2]  // "third"
```

### Always Initialize
Don't forget to set your array to `({ })` before using it:
```c
array my_list;  // This is 0, not an array!
my_list = ({ });  // NOW it's an array
```

### Use sizeof() for Loops
Always use `sizeof()` to avoid going past the end:
```c
for (i = 0; i < sizeof(my_array); i++) {
  // Safe! Won't go past the end
}
```

### Shared Arrays
When you assign an array to another variable, they share the same data:
```c
array a = ({ 1, 2, 3 });
array b = a;  // b and a point to the SAME array

b[0] = 99;  // Changes both a and b!
// a is now ({ 99, 2, 3 })
// b is now ({ 99, 2, 3 })
```

To make a real copy:
```c
array a = ({ 1, 2, 3 });
array b = ({ }) + a;  // b is a separate copy

b[0] = 99;  // Only changes b
// a is still ({ 1, 2, 3 })
// b is ({ 99, 2, 3 })
```

## Quick Reference

| What You Want | How To Do It | Example |
|---------------|--------------|---------|  
| Make an empty array | `({ })` | `array list = ({ });` |
| Make an array with items | `({ item1, item2 })` | `array names = ({ "Alice", "Bob" });` |
| Get an item | `array[position]` | `name = names[0];` |
| Change an item | `array[position] = value` | `names[0] = "Charlie";` |
| Count items | `sizeof(array)` | `count = sizeof(names);` |
| Add items | `array += ({ item })` | `names += ({ "Dave" });` |
| Remove items | `array -= ({ item })` | `names -= ({ "Bob" });` |
| Combine arrays | `array1 + array2` | `all = names + scores;` |

---

**Next:** Learn about [Mappings](nlpc-mappings-guide.md) for when you need to look things up by name instead of number!
