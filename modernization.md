# NLPC vs Modern LPC - Feature Comparison

## Overview

This document compares NLPC (NetCI LPC) with modern LPC implementations (LDMud 3.6+ and FluffOS 3.0+), identifying missing features and opportunities for enhancement. NLPC represents an early 1990s snapshot of LPC evolution, while modern LPC has continued to develop with significant new features.

## Executive Summary

**NLPC Status**: Early LPC implementation (~1995 era) with basic object-oriented features
**Modern LPC**: Advanced language with arrays, mappings, closures, structs, and sophisticated type system

**Key Missing Features**:
1. Dynamic arrays and array operations
2. Mapping (associative array) data type
3. Closures (lambda functions/function pointers)
4. Structs (user-defined data structures)
5. Advanced type system (mixed, void, type checking)
6. Automatic virtual filesystem management
7. Modern string handling (indexing, slicing)
8. Call-by-reference parameters
9. Preprocessor directives (#define, #ifdef, etc.)
10. Object inheritance (vs attach() system)

## Data Types Comparison

### Current NLPC Types

```c
int count;           // Integer only
string name;         // String only
object player;       // Object reference only
```

**Limitations**:
- No arrays
- No mappings
- No mixed type
- No void type
- No float/double
- No structs


### Modern LPC Types

```c
// Basic types
int count;
float damage;
string name;
object player;
void function();     // No return value
mixed value;         // Any type

// Complex types
int *array;          // Dynamic array
mapping map;         // Associative array (hash map)
struct point {       // User-defined structure
    int x;
    int y;
};
closure func;        // Function pointer/lambda

// Type modifiers
static int private_var;
private int hidden_var;
protected int inherited_var;
public int visible_var;
nosave int transient_var;
```

**Advantages**:
- Type safety with mixed
- Complex data structures
- Function pointers
- Access control modifiers
- Transient data (nosave)

## Feature Comparison Matrix

| Feature | NLPC | LDMud/FluffOS | Impact |
|---------|------|---------------|--------|
| **Data Structures** |
| Arrays | ❌ No | ✅ Yes | HIGH - Major limitation |
| Mappings | ❌ No (tables only) | ✅ Yes | HIGH - Workaround exists |
| Structs | ❌ No | ✅ Yes | MEDIUM - Can use objects |
| Mixed type | ❌ No | ✅ Yes | MEDIUM - Type flexibility |
| **Functions** |
| Closures | ❌ No | ✅ Yes | HIGH - No callbacks |
| Varargs | ❌ No | ✅ Yes | LOW - Fixed params work |
| Default params | ❌ No | ✅ Yes | LOW - Nice to have |
| Call by reference | ❌ No | ✅ Yes | MEDIUM - Must return values |
| **Object System** |
| Inheritance | ⚠️ attach() only | ✅ Full inheritance | MEDIUM - Workaround exists |
| Multiple inheritance | ⚠️ Multiple attach() | ✅ Yes | LOW - Attach works |
| Virtual functions | ❌ No | ✅ Yes | LOW - Can override |
| Abstract classes | ❌ No | ✅ Yes | LOW - Not critical |
| **String Handling** |
| String indexing | ❌ No | ✅ `str[0]` | MEDIUM - Have leftstr() |
| String slicing | ❌ No | ✅ `str[1..5]` | MEDIUM - Have midstr() |
| String mutation | ❌ No | ✅ `str[0] = 'X'` | LOW - Strings immutable |
| Regex | ❌ No | ✅ Yes | MEDIUM - Pattern matching |
| **Filesystem** |
| Auto-managed | ❌ Manual hide/unhide | ✅ Automatic | HIGH - Major pain point |
| File watching | ❌ No | ✅ Yes | LOW - Not critical |
| Include paths | ❌ Limited | ✅ Full support | MEDIUM - Affects organization |
| **Preprocessor** |
| #define | ⚠️ Limited | ✅ Full | MEDIUM - Workaround exists |
| #ifdef | ❌ No | ✅ Yes | LOW - Not critical |
| #include | ⚠️ Basic | ✅ Full | MEDIUM - Affects code reuse |
| Macros | ❌ No | ✅ Yes | LOW - Can use functions |
| **Advanced Features** |
| Coroutines | ❌ No | ✅ Yes (FluffOS) | LOW - Can use alarms |
| Async/await | ❌ No | ✅ Yes (FluffOS) | LOW - Can use callbacks |
| JSON support | ❌ No | ✅ Yes | MEDIUM - Manual parsing |
| SQLite | ❌ No | ✅ Yes (FluffOS) | LOW - Have tables |
| **Error Handling** |
| try/catch | ❌ No | ✅ Yes | MEDIUM - No exception handling |
| Error objects | ❌ No | ✅ Yes | LOW - Can check returns |
| **Security** |
| Valid_read | ❌ No | ✅ Yes | MEDIUM - Manual checks |
| Valid_write | ❌ No | ✅ Yes | MEDIUM - Manual checks |
| Privilege system | ✅ priv() | ✅ Similar | EQUAL |



## Missing Feature Deep Dive

### 1. Dynamic Arrays (HIGH PRIORITY)

**What's Missing**:
```c
// Modern LPC
int *numbers = ({ 1, 2, 3, 4, 5 });
string *names = ({ "Alice", "Bob", "Charlie" });
object *players = ({ player1, player2, player3 });

numbers[0] = 10;              // Index access
numbers += ({ 6, 7 });        // Append
int size = sizeof(numbers);   // Get size
numbers = numbers[1..3];      // Slice

// Array operations
numbers = sort_array(numbers, "compare_func");
names = map(names, "upcase");
int sum = reduce(numbers, "add");
string *filtered = filter(names, "starts_with_a");
```

**Current NLPC Workaround**:
```c
// Must use string concatenation and parsing
string numbers = "1,2,3,4,5";
string names = "Alice,Bob,Charlie";

// Manual parsing required
pos = instr(numbers, 1, ",");
first = leftstr(numbers, pos-1);  // Get first element
rest = rightstr(numbers, strlen(numbers)-pos);  // Get rest
```

**Impact**:
- **Code complexity**: 10x more code for array operations
- **Performance**: String parsing is slow
- **Maintainability**: Error-prone manual parsing
- **Expressiveness**: Can't easily represent lists

**Implementation Requirements**:
1. Add array type to variable system
2. Implement array literal syntax `({ ... })`
3. Add array indexing `arr[i]`
4. Add array slicing `arr[start..end]`
5. Implement array built-ins: `sizeof()`, `sort_array()`, `map()`, `filter()`, `reduce()`
6. Add array concatenation operator `+=`



### 2. Mappings (HIGH PRIORITY)

**What's Missing**:
```c
// Modern LPC
mapping inventory = ([]);
mapping stats = ([ "strength": 10, "dexterity": 15, "intelligence": 12 ]);

// Access
int str = stats["strength"];
stats["wisdom"] = 14;

// Operations
string *keys = keys(stats);
int *values = values(stats);
int size = sizeof(stats);
if (member(stats, "strength")) { ... }

// Iteration
foreach(string key, int value in stats) {
    write(key+": "+value+"\n");
}

// Nested structures
mapping player_data = ([
    "stats": ([ "str": 10, "dex": 15 ]),
    "inventory": ([ "sword": 1, "potion": 3 ]),
    "flags": ([ "builder": 1, "priv": 0 ])
]);
```

**Current NLPC Workaround**:
```c
// Use table system (global hash map)
table_set("player_15_strength", "10");
table_set("player_15_dexterity", "15");

// Access
str = atoi(table_get("player_15_strength"));

// No iteration support
// No nested structures
// Global namespace pollution
```

**Impact**:
- **Namespace pollution**: All tables are global
- **No nesting**: Can't have complex data structures
- **Type loss**: Everything stored as strings
- **No iteration**: Can't loop through keys/values
- **Memory**: Tables persist in database

**Implementation Requirements**:
1. Add mapping type to variable system
2. Implement mapping literal syntax `([ key: value ])`
3. Add mapping indexing `map[key]`
4. Implement mapping built-ins: `keys()`, `values()`, `sizeof()`, `member()`
5. Add `foreach` loop for iteration
6. Support nested mappings



### 3. Automatic Virtual Filesystem (HIGH PRIORITY)

**What's Missing**:
```c
// Modern LPC - Automatic
// Just create the file and it exists
write_file("/home/wizard/myfile.c", "code here\n");
cat("/home/wizard/myfile.c");  // Works immediately

// Compile automatically finds files
compile_object("/home/wizard/myobject");  // Just works
```

**Current NLPC Requirement**:
```c
// Must manually unhide files before use
unhide("/home/wizard/myfile.c", owner, 3);  // Make visible with rw permissions
fwrite("/home/wizard/myfile.c", "code here\n");
cat("/home/wizard/myfile.c");  // Now works

// Must unhide before compiling
unhide("/home/wizard/myobject.c", owner, 1);
compile_object("/home/wizard/myobject");
```

**Impact**:
- **User friction**: Extra step for every file operation
- **Error-prone**: Forgetting to unhide causes silent failures
- **Package complexity**: Package files must list every file
- **Discoverability**: Hidden files are invisible to ls
- **Permissions confusion**: File permissions separate from hide/unhide

**Current System**:
```c
// File states in NLPC
1. Hidden + doesn't exist on disk = Invisible, can't create
2. Hidden + exists on disk = Invisible, can't access
3. Unhidden + doesn't exist = Visible, can create
4. Unhidden + exists = Visible, can access

// Permissions are separate
unhide(path, owner, flags);  // flags: 0=none, 1=r, 2=w, 3=rw, 4-7=directory variants
```

**Proposed Modern System**:
```c
// Automatic visibility
1. File exists on disk = Automatically visible
2. File created = Automatically visible
3. Permissions control access (like Unix)

// No unhide() needed
fwrite("/home/wizard/test.c", "code");  // Creates and makes visible
cat("/home/wizard/test.c");             // Just works
```

**Implementation Requirements**:
1. Remove hide/unhide requirement
2. Auto-register files when created
3. Auto-discover files on disk
4. Unify permissions with visibility
5. Update package system to not require file lists
6. Add file watching for external changes

**Migration Path**:
- Keep `hide()`/`unhide()` for backward compatibility
- Make them no-ops or just set permissions
- Auto-unhide on first access
- Deprecate in documentation



### 4. Closures / Function Pointers (HIGH PRIORITY)

**What's Missing**:
```c
// Modern LPC - Closures (lambda functions)
closure add_func = (: $1 + $2 :);
int result = funcall(add_func, 5, 3);  // Returns 8

// Function pointers
closure sort_func = (: $1 > $2 :);
int *sorted = sort_array(numbers, sort_func);

// Inline closures for callbacks
call_out((: write("Delayed message\n") :), 5);

// Closures with context
int multiplier = 10;
closure multiply = (: $1 * multiplier :);
result = funcall(multiply, 5);  // Returns 50

// Map/filter with closures
string *names = ({ "alice", "bob", "charlie" });
string *upper = map(names, (: upcase($1) :));
int *evens = filter(({ 1, 2, 3, 4, 5 }), (: $1 % 2 == 0 :));

// Partial application
closure add_five = (: $1 + 5 :);
int *incremented = map(numbers, add_five);
```

**Current NLPC Workaround**:
```c
// Must use string function names and call_other
call_other(this_object(), "my_callback", arg1, arg2);

// For delayed execution
alarm(5, "delayed_function");

// No inline functions - must define separately
int delayed_function() {
    write("Delayed message\n");
}

// For sorting - must pass function name as string
sort_array(numbers, "compare_function");

int compare_function(int a, int b) {
    return a > b;
}

// No closures means no context capture
// Must use global variables or object properties
```

**Impact**:
- **No inline logic**: Can't define behavior at call site
- **No context capture**: Can't access local variables in callbacks
- **Verbose**: Must define named functions for simple operations
- **Less functional**: Can't do map/filter/reduce elegantly
- **Callback complexity**: Passing context requires workarounds

**Implementation Requirements**:
1. Add closure type to variable system
2. Implement closure literal syntax `(: expression :)`
3. Add `funcall()` built-in to execute closures
4. Support closure parameters `$1`, `$2`, etc.
5. Implement context capture for local variables
6. Add closure support to `call_out()`, `alarm()`
7. Update array functions to accept closures
8. Add partial application support



### 5. Structs (MEDIUM PRIORITY)

**What's Missing**:
```c
// Modern LPC - Struct definitions
struct point {
    int x;
    int y;
};

struct player_stats {
    int strength;
    int dexterity;
    int intelligence;
    int wisdom;
    int constitution;
    int charisma;
};

struct inventory_item {
    string name;
    int quantity;
    int weight;
    string description;
};

// Usage
struct point pos = (<point> x: 10, y: 20);
pos.x = 15;
int x_coord = pos.x;

struct player_stats stats = (<player_stats>
    strength: 10,
    dexterity: 15,
    intelligence: 12,
    wisdom: 14,
    constitution: 13,
    charisma: 8
);

// Arrays of structs
struct inventory_item *inventory = ({
    (<inventory_item> name: "sword", quantity: 1, weight: 5, description: "A sharp blade"),
    (<inventory_item> name: "potion", quantity: 3, weight: 1, description: "Healing potion")
});

// Nested structs
struct character {
    string name;
    struct player_stats stats;
    struct point position;
};
```

**Current NLPC Workaround**:
```c
// Option 1: Use separate variables
int player_strength;
int player_dexterity;
int player_intelligence;
// ... becomes unwieldy

// Option 2: Use objects as data containers
object stats = new("/obj/player_stats");
stats->strength = 10;
stats->dexterity = 15;

// Option 3: Use table system
table_set("player_15_strength", "10");
table_set("player_15_dexterity", "15");

// Option 4: Use string encoding
string stats = "10,15,12,14,13,8";  // Must parse manually
```

**Impact**:
- **No type safety**: Can't enforce structure
- **Verbose**: Must use objects or multiple variables
- **Performance**: Objects have overhead vs lightweight structs
- **Clarity**: Intent not clear from code
- **Memory**: Objects persist, structs are lightweight

**Implementation Requirements**:
1. Add struct keyword and definition syntax
2. Implement struct type in variable system
3. Add struct literal syntax `(<struct_name> field: value)`
4. Implement member access operator `.`
5. Support struct assignment and copying
6. Allow structs in arrays and mappings
7. Add `sizeof()` support for structs
8. Implement struct comparison operators



### 6. Advanced Type System (MEDIUM PRIORITY)

**What's Missing**:
```c
// Modern LPC - Mixed type
mixed value;
value = 10;           // Can hold int
value = "hello";      // Can hold string
value = ({ 1, 2 });   // Can hold array
value = ([ "a": 1 ]); // Can hold mapping

// Type checking
if (intp(value)) { ... }
if (stringp(value)) { ... }
if (arrayp(value)) { ... }
if (mappingp(value)) { ... }
if (objectp(value)) { ... }

// Void type for functions with no return
void setup() {
    // No return statement needed
}

// Type modifiers
static int private_var;      // File-local
private int hidden_var;      // Object-private
protected int inherited_var; // Accessible to children
public int visible_var;      // Accessible to all
nosave int transient_var;    // Not saved to disk

// Type casting
mixed m = "123";
int i = (int)m;  // Cast to int

// Function prototypes with types
int calculate(int a, int b);
string format_name(string first, string last);
mixed get_property(string key);
```

**Current NLPC Types**:
```c
// Only basic types
int count;
string name;
object player;

// No mixed type - must know type at compile time
// No void type - functions must return something
// No type modifiers - all variables are public
// No nosave - all variables persist
// No type checking functions
// No type casting
```

**Impact**:
- **Inflexibility**: Can't write generic functions
- **No polymorphism**: Can't handle multiple types
- **Verbosity**: Must write separate functions for each type
- **No transient data**: Everything persists to disk
- **No access control**: All variables are public
- **Type safety**: No runtime type checking

**Implementation Requirements**:
1. Add `mixed` type to variable system
2. Implement `void` return type
3. Add type checking functions: `intp()`, `stringp()`, `objectp()`, etc.
4. Implement type modifiers: `static`, `private`, `protected`, `public`, `nosave`
5. Add type casting syntax `(type)value`
6. Update compiler to handle mixed types
7. Add runtime type checking
8. Implement nosave variable handling



### 7. Modern String Handling (MEDIUM PRIORITY)

**What's Missing**:
```c
// Modern LPC - String indexing
string name = "Alice";
string first = name[0];        // "A"
string last = name[4];         // "e"
string middle = name[2];       // "i"

// String slicing
string substr = name[1..3];    // "lic"
string prefix = name[0..1];    // "Al"
string suffix = name[3..];     // "ce"
string all = name[..];         // "Alice"

// String mutation (in some implementations)
name[0] = 'B';                 // "Blice"

// Negative indexing
string last_char = name[-1];   // "e"
string last_two = name[-2..];  // "ce"

// String ranges
string reversed = name[<1..0]; // "ecilA"

// Regular expressions
if (regexp(name, "^A.*e$")) { ... }
string *matches = regexp_match(text, "([0-9]+)");
string replaced = regreplace(text, "[0-9]+", "X", 1);

// Advanced string functions
string *words = explode(sentence, " ");
string joined = implode(words, "-");
string trimmed = trim(text);
string padded = sprintf("%-20s", name);
```

**Current NLPC String Functions**:
```c
// Basic string functions only
string name = "Alice";

// Must use functions for everything
string first = leftstr(name, 1);           // Get first char
string last = rightstr(name, 1);           // Get last char
string middle = midstr(name, 3, 1);        // Get middle char

// Substring extraction
string substr = midstr(name, 2, 3);        // Get "lic"

// No string mutation - strings are immutable
// No negative indexing
// No slicing syntax
// No regex support

// Available functions
int len = strlen(name);
int pos = instr(name, 1, "i");            // Find position
string upper = upcase(name);
string lower = downcase(name);
string formatted = sprintf("%s is %d", name, age);
```

**Impact**:
- **Verbosity**: Must use function calls instead of operators
- **Readability**: `name[0]` clearer than `leftstr(name, 1)`
- **No regex**: Pattern matching requires manual parsing
- **Limited**: Can't easily extract substrings
- **Performance**: Function calls have overhead

**Implementation Requirements**:
1. Add string indexing operator `str[index]`
2. Implement string slicing `str[start..end]`
3. Add negative indexing support
4. Implement regex functions: `regexp()`, `regexp_match()`, `regreplace()`
5. Add string mutation (optional)
6. Implement additional string functions: `trim()`, `explode()`, `implode()`
7. Update compiler to handle string operators



### 8. Call-by-Reference Parameters (MEDIUM PRIORITY)

**What's Missing**:
```c
// Modern LPC - Reference parameters
void swap(int &a, int &b) {
    int temp = a;
    a = b;
    b = temp;
}

int x = 5, y = 10;
swap(&x, &y);  // x is now 10, y is now 5

// Multiple return values via references
int parse_coords(string input, int &x, int &y) {
    if (sscanf(input, "%d,%d", &x, &y) != 2) {
        return 0;  // Parse failed
    }
    return 1;  // Parse succeeded
}

int x, y;
if (parse_coords("10,20", &x, &y)) {
    // x = 10, y = 20
}

// Modifying array elements
void increment_all(int *&arr) {
    for (int i = 0; i < sizeof(arr); i++) {
        arr[i]++;
    }
}

int *numbers = ({ 1, 2, 3 });
increment_all(&numbers);  // numbers is now ({ 2, 3, 4 })
```

**Current NLPC Workaround**:
```c
// Must return values
int *swap_result(int a, int b) {
    return ({ b, a });  // Would need arrays first!
}

// Or use string encoding
string swap_result(int a, int b) {
    return sprintf("%d,%d", b, a);
}

// Parse and extract
string result = swap_result(5, 10);
int x = atoi(leftstr(result, instr(result, 1, ",")-1));
int y = atoi(rightstr(result, strlen(result)-instr(result, 1, ",")));

// For multiple returns, must use object properties
object result = new("/obj/result");
parse_coords("10,20", result);
int x = result->x;
int y = result->y;

// Or use global variables (bad practice)
int global_x, global_y;
parse_coords("10,20");  // Sets globals
```

**Impact**:
- **Awkward**: Must return multiple values via arrays/strings/objects
- **Performance**: Creating objects for return values is slow
- **Clarity**: Intent not clear from function signature
- **Limitations**: Can't modify caller's variables directly

**Implementation Requirements**:
1. Add reference parameter syntax `type &param`
2. Implement reference passing in function calls `func(&var)`
3. Update compiler to track reference parameters
4. Implement reference semantics in interpreter
5. Add reference support for all types
6. Update function call mechanism



### 9. Preprocessor Directives (MEDIUM PRIORITY)

**What's Missing**:
```c
// Modern LPC - Full preprocessor
#define MAX_PLAYERS 100
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define DEBUG

#ifdef DEBUG
    #define LOG(msg) write("DEBUG: " + msg + "\n")
#else
    #define LOG(msg)  // No-op in production
#endif

#ifndef COMBAT_H
#define COMBAT_H

// Header content here

#endif  // COMBAT_H

// Conditional compilation
#ifdef ENABLE_FEATURE_X
    void feature_x() {
        // Feature X code
    }
#endif

// Include guards
#pragma save_binary
#pragma strict_types
#pragma combine_strings

// Multi-line macros
#define VALIDATE_PLAYER(p) \
    if (!objectp(p) || !interactive(p)) { \
        return 0; \
    }

// Stringification
#define STR(x) #x
#define XSTR(x) STR(x)
string version = XSTR(VERSION);  // "1.2.3"
```

**Current NLPC Preprocessor**:
```c
// Very limited preprocessor
#include <sys.h>
#include "myheader.h"

// No #define support
// No #ifdef/#ifndef
// No macros
// No conditional compilation
// No pragmas

// Must use constants instead
int MAX_PLAYERS = 100;

// Must use functions instead of macros
int min(int a, int b) {
    return (a < b) ? a : b;
}

// No include guards - must be careful
// No conditional compilation - must use runtime checks
```

**Impact**:
- **No constants**: Can't define compile-time constants
- **No conditional compilation**: Can't enable/disable features
- **No include guards**: Risk of multiple inclusion
- **Verbosity**: Must use functions instead of simple macros
- **Performance**: Function calls vs inline macros

**Implementation Requirements**:
1. Implement `#define` for constants
2. Add `#define` for macros with parameters
3. Implement `#ifdef`, `#ifndef`, `#else`, `#endif`
4. Add `#undef` to undefine macros
5. Implement `#pragma` directives
6. Add stringification operator `#`
7. Add token pasting operator `##`
8. Update preprocessor to handle multi-line macros
9. Implement include guards



### 10. Object Inheritance (MEDIUM PRIORITY)

**What's Missing**:
```c
// Modern LPC - True inheritance
// base_living.c
int hp;
int max_hp;

void set_hp(int amount) {
    hp = amount;
}

int get_hp() {
    return hp;
}

void take_damage(int amount) {
    hp -= amount;
    if (hp < 0) hp = 0;
}

// player.c
inherit "/obj/base_living";

int level;
int experience;

void create() {
    ::create();  // Call parent create
    set_hp(100);
    level = 1;
}

void gain_experience(int amount) {
    experience += amount;
    if (experience >= level * 1000) {
        level_up();
    }
}

void level_up() {
    level++;
    max_hp += 10;
    set_hp(max_hp);  // Inherited function
}

// Multiple inheritance
inherit "/obj/base_living";
inherit "/obj/base_container";
inherit "/obj/base_describable";

// Virtual functions
void take_damage(int amount) {
    ::take_damage(amount);  // Call parent version
    write("Ouch! You took " + amount + " damage!\n");
}
```

**Current NLPC Attach System**:
```c
// player.c
attach("/obj/share/living");
attach("/obj/share/common");

int level;
int experience;

// Can call attached functions
void create() {
    set_hp(100);  // From living.c
    level = 1;
}

// But no true inheritance
// - Can't override attached functions
// - Can't call parent version with ::
// - No inheritance chain
// - Attached code runs in attached object's context

// Workarounds
void my_take_damage(int amount) {
    take_damage(amount);  // Call attached version
    write("Ouch!\n");     // Add custom behavior
}
```

**Impact**:
- **No override**: Can't customize inherited behavior
- **No super calls**: Can't call parent version
- **Context confusion**: Attached code runs in wrong context
- **Limited reuse**: Can't build inheritance hierarchies
- **Workarounds needed**: Must wrap functions to customize

**Differences**:
| Feature | NLPC attach() | Modern inherit |
|---------|---------------|----------------|
| Code reuse | ✅ Yes | ✅ Yes |
| Multiple "inheritance" | ✅ Yes | ✅ Yes |
| Override functions | ❌ No | ✅ Yes |
| Call parent version | ❌ No | ✅ Yes (::) |
| Inheritance chain | ❌ No | ✅ Yes |
| Virtual functions | ❌ No | ✅ Yes |
| Context | ⚠️ Attached object | ✅ This object |

**Implementation Requirements**:
1. Add `inherit` keyword
2. Implement inheritance chain tracking
3. Add `::` operator for parent calls
4. Implement virtual function dispatch
5. Support multiple inheritance
6. Handle function override resolution
7. Update object creation to initialize inheritance
8. Maintain backward compatibility with `attach()`



## Additional Missing Features

### Object System Features

#### 11. Multiple Inheritance (LOW PRIORITY)

**What's Missing**:
```c
// Modern LPC - Multiple inheritance
inherit "/obj/base_living";
inherit "/obj/base_container";
inherit "/obj/base_describable";

// All inherited functions available
void create() {
    ::create();  // Calls all parent create() functions
    set_hp(100);           // From base_living
    set_capacity(50);      // From base_container
    set_description("A player");  // From base_describable
}

// Conflict resolution
void reset() {
    base_living::reset();      // Call specific parent
    base_container::reset();   // Call another parent
}
```

**Current NLPC**:
```c
// Multiple attach works similarly
attach("/obj/share/living");
attach("/obj/share/container");
attach("/obj/share/describable");

// Can call all attached functions
void create() {
    set_hp(100);
    set_capacity(50);
    set_description("A player");
}

// But no conflict resolution mechanism
// Last attached wins if same function name
```

**Impact**:
- **LOW**: NLPC's attach() system already supports multiple "inheritance"
- **Conflict resolution**: No way to specify which parent's function to call
- **Workaround exists**: attach() provides similar functionality

**Implementation Requirements**:
1. Already supported via attach()
2. Could add `::parent_name::function()` syntax for conflict resolution
3. Low priority since attach() works well



#### 12. Virtual Functions (LOW PRIORITY)

**What's Missing**:
```c
// Modern LPC - Virtual function override
// base_living.c
void take_damage(int amount) {
    hp -= amount;
    if (hp < 0) hp = 0;
}

// player.c
inherit "/obj/base_living";

// Override parent function
void take_damage(int amount) {
    ::take_damage(amount);  // Call parent version first
    write("Ouch! You took " + amount + " damage!\n");
    if (hp <= 0) {
        write("You died!\n");
        die();
    }
}
```

**Current NLPC**:
```c
// Can't override attached functions
attach("/obj/share/living");

// Must create wrapper with different name
void my_take_damage(int amount) {
    take_damage(amount);  // Call attached version
    write("Ouch!\n");
    if (hp <= 0) {
        die();
    }
}

// Or reimplement entirely
void player_take_damage(int amount) {
    hp -= amount;
    if (hp < 0) hp = 0;
    write("Ouch!\n");
    if (hp <= 0) die();
}
```

**Impact**:
- **LOW**: Can work around with wrapper functions
- **Less elegant**: Must use different function names
- **Code duplication**: May need to reimplement parent logic

**Implementation Requirements**:
1. Implement function override mechanism
2. Add `::` operator for parent calls
3. Update function dispatch to check inheritance chain
4. Low priority - workarounds exist



#### 13. Abstract Classes (LOW PRIORITY)

**What's Missing**:
```c
// Modern LPC - Abstract classes
// base_weapon.c (abstract)
abstract void attack(object target);  // Must be implemented by children
abstract int get_damage();

void wield(object player) {
    // Common implementation
    player->set_wielded(this_object());
}

// sword.c
inherit "/obj/base_weapon";

void attack(object target) {
    int damage = get_damage();
    target->take_damage(damage);
}

int get_damage() {
    return 10 + random(5);
}
```

**Current NLPC**:
```c
// No abstract keyword
// Must document which functions should be implemented
// base_weapon.c
void attack(object target) {
    // Placeholder - override in child
    write("ERROR: attack() not implemented\n");
}

int get_damage() {
    // Placeholder - override in child
    return 0;
}

void wield(object player) {
    player->set_wielded(this_object());
}
```

**Impact**:
- **LOW**: Not critical for functionality
- **No compile-time checking**: Can't enforce implementation
- **Documentation**: Must rely on comments

**Implementation Requirements**:
1. Add `abstract` keyword
2. Compiler checks for abstract function implementation
3. Prevent instantiation of abstract classes
4. Low priority - not commonly needed



### Filesystem Features

#### 14. File Watching (LOW PRIORITY)

**What's Missing**:
```c
// Modern LPC - Automatic file watching
// Files automatically reload when changed on disk
// No manual intervention needed

// Driver detects changes and recompiles
// Objects automatically updated
```

**Current NLPC**:
```c
// No automatic file watching
// Must manually recompile after changes
compile_object("/obj/myobject");

// Or use update command
update("/obj/myobject");

// No notification of external changes
```

**Impact**:
- **LOW**: Manual recompilation works fine
- **Development convenience**: Would be nice for development
- **Not critical**: Production code doesn't change often

**Implementation Requirements**:
1. Add file system monitoring (inotify/kqueue)
2. Detect file changes
3. Trigger automatic recompilation
4. Low priority - manual update works



#### 15. Include Paths (MEDIUM PRIORITY)

**What's Missing**:
```c
// Modern LPC - Configurable include paths
// Driver config:
// include_dirs: /include, /sys, /lib/include

// In code:
#include <combat.h>      // Searches include_dirs
#include "local.h"       // Searches current directory

// Can organize headers better
#include <sys/types.h>
#include <lib/string_utils.h>
```

**Current NLPC**:
```c
// Limited include support
#include <sys.h>         // Must be in specific location
#include "myheader.h"    // Relative to current file

// No configurable search paths
// Must use full paths or relative paths
#include "/obj/share/common.h"
```

**Impact**:
- **MEDIUM**: Affects code organization
- **Workaround**: Use full paths
- **Maintenance**: Harder to reorganize code

**Implementation Requirements**:
1. Add include path configuration
2. Update preprocessor to search paths
3. Support angle bracket vs quote includes
4. Medium priority - affects organization



### Advanced Features

#### 16. JSON Support (MEDIUM PRIORITY)

**What's Missing**:
```c
// Modern LPC - Built-in JSON
mapping data = ([ "name": "Alice", "level": 5, "items": ({ "sword", "potion" }) ]);

// Serialize to JSON
string json = json_encode(data);
// {"name":"Alice","level":5,"items":["sword","potion"]}

// Parse JSON
mapping parsed = json_decode(json);
string name = parsed["name"];  // "Alice"

// Pretty print
string pretty = json_encode(data, 1);
```

**Current NLPC**:
```c
// Must manually build JSON strings
string json = "{";
json += "\"name\":\"" + name + "\",";
json += "\"level\":" + level + ",";
json += "\"items\":[\"sword\",\"potion\"]";
json += "}";

// Must manually parse JSON
// Very error-prone and tedious
int pos = instr(json, 1, "\"name\":");
// ... complex parsing logic
```

**Impact**:
- **MEDIUM**: JSON is common for APIs and data exchange
- **Workaround**: Manual string building/parsing
- **Error-prone**: Easy to create invalid JSON

**Implementation Requirements**:
1. Add `json_encode()` built-in
2. Add `json_decode()` built-in
3. Support mappings, arrays, strings, ints
4. Handle nested structures
5. Medium priority - useful for modern integrations



#### 17. SQLite Support (LOW PRIORITY - FluffOS only)

**What's Missing**:
```c
// FluffOS - SQLite support
int db = sqlite_open("/data/players.db");

sqlite_exec(db, "CREATE TABLE players (name TEXT, level INT)");
sqlite_exec(db, "INSERT INTO players VALUES ('Alice', 5)");

mixed *results = sqlite_query(db, "SELECT * FROM players WHERE level > 3");
foreach(mixed *row in results) {
    write(row[0] + " is level " + row[1] + "\n");
}

sqlite_close(db);
```

**Current NLPC**:
```c
// Use table system instead
table_set("player_alice_level", "5");
table_set("player_bob_level", "3");

// No SQL queries
// Must iterate manually
// No relational features
```

**Impact**:
- **LOW**: Table system works for most needs
- **No SQL**: Can't do complex queries
- **Workaround exists**: Tables provide key-value storage

**Implementation Requirements**:
1. Add SQLite library integration
2. Implement sqlite_* built-ins
3. Handle database connections
4. Low priority - tables work well



### Error Handling Features

#### 18. Error Objects (LOW PRIORITY)

**What's Missing**:
```c
// Modern LPC - Error objects
try {
    object item = load_object("/obj/items/sword");
    item->set_owner(player);
} catch(error err) {
    write("Error: " + err->message + "\n");
    write("File: " + err->file + "\n");
    write("Line: " + err->line + "\n");
    write("Trace: " + err->trace + "\n");
}

// Error object provides context
class error {
    string message;
    string file;
    int line;
    string trace;
}
```

**Current NLPC**:
```c
// Only string error messages
// No structured error information
object item = load_object("/obj/items/sword");
if (!item) {
    write("Error loading item\n");
    // No details about what went wrong
    return;
}

// Must manually track context
```

**Impact**:
- **LOW**: String errors work for most cases
- **Less information**: Can't get stack traces easily
- **Debugging**: Harder to debug complex errors

**Implementation Requirements**:
1. Create error object type
2. Capture stack traces
3. Store file/line information
4. Low priority - string errors work



### Security Features

#### 19. Valid_read / Valid_write (MEDIUM PRIORITY)

**What's Missing**:
```c
// Modern LPC - Security callbacks
// master.c
int valid_read(string file, object reader, string func) {
    // Check if reader can read file
    if (is_admin(reader)) return 1;
    if (file[0..9] == "/home/" + reader->query_name()) return 1;
    return 0;
}

int valid_write(string file, object writer, string func) {
    // Check if writer can write file
    if (is_admin(writer)) return 1;
    if (file[0..9] == "/home/" + writer->query_name()) return 1;
    return 0;
}

// Automatically called by driver
// In code:
string data = read_file("/home/alice/data.txt");  // Checks valid_read
write_file("/home/alice/data.txt", "new data");   // Checks valid_write
```

**Current NLPC**:
```c
// Must manually check permissions
string data = fread("/home/alice/data.txt");
// No automatic security checks

// Must implement own permission system
int can_read(string file, object reader) {
    if (priv(reader, "admin")) return 1;
    // ... manual checks
    return 0;
}

// Must call manually before every operation
if (!can_read(file, this_player())) {
    write("Permission denied\n");
    return;
}
string data = fread(file);
```

**Impact**:
- **MEDIUM**: Security is important
- **Manual checks**: Easy to forget
- **Inconsistent**: Different code may check differently

**Implementation Requirements**:
1. Add valid_read/valid_write callbacks to master
2. Call automatically on file operations
3. Integrate with permission system
4. Medium priority - security matters



### 20. Varargs and Default Parameters (LOW PRIORITY)

**What's Missing**:
```c
// Modern LPC - Varargs
void log(string format, mixed *args...) {
    string msg = sprintf(format, args...);
    write_file("/log/system.log", msg + "\n");
}

log("Player %s logged in", name);
log("Player %s at %d,%d", name, x, y);
log("Error: %s (code %d)", error, code);

// Default parameters
void teleport(object player, int x, int y, int z = 0) {
    // z defaults to 0 if not provided
}

teleport(player, 10, 20);      // z = 0
teleport(player, 10, 20, 5);   // z = 5
```

**Current NLPC**:
```c
// Must define multiple versions
void log1(string msg) {
    write_file("/log/system.log", msg + "\n");
}

void log2(string format, string arg1) {
    string msg = sprintf(format, arg1);
    write_file("/log/system.log", msg + "\n");
}

void log3(string format, string arg1, string arg2) {
    string msg = sprintf(format, arg1, arg2);
    write_file("/log/system.log", msg + "\n");
}

// Or use special values for optional params
void teleport(object player, int x, int y, int z) {
    if (z == -999) z = 0;  // Magic number for "not provided"
}
```

**Impact**:
- **Code duplication**: Must create multiple function versions
- **Magic numbers**: Using sentinel values is error-prone
- **Maintenance**: Adding parameters requires new function versions
- **API clarity**: Not obvious which parameters are optional

**Implementation Requirements**:
1. Add varargs syntax `type *args...` to function parameters
2. Implement default parameter syntax `type param = value`
3. Update compiler to handle variable argument counts
4. Implement argument unpacking for varargs
5. Add runtime support for default values
6. Update function call mechanism to handle optional parameters



### 21. Exception Handling (LOW PRIORITY)

**What's Missing**:
```c
// Modern LPC - try/catch
try {
    object item = load_object("/obj/items/sword");
    item->set_owner(player);
} catch(string error) {
    write("Error loading item: " + error + "\n");
    log_error(error);
}

// Finally block
try {
    file = fopen("/data/players.dat", "r");
    data = fread(file);
} catch(string error) {
    write("Error reading file: " + error + "\n");
} finally {
    if (file) fclose(file);
}

// Throwing exceptions
void validate_input(string input) {
    if (!input || input == "") {
        throw("Input cannot be empty");
    }
}
```

**Current NLPC**:
```c
// Must check return values
object item = load_object("/obj/items/sword");
if (!item) {
    write("Error loading item\n");
    return;
}

// No automatic cleanup
// Must manually check every operation
// No exception propagation
```

**Impact**:
- **Error handling verbosity**: Must check every operation manually
- **No automatic cleanup**: Resource leaks possible
- **Error propagation**: Must manually pass errors up call stack
- **Code clarity**: Error handling mixed with business logic

**Implementation Requirements**:
1. Add `try`, `catch`, `finally`, `throw` keywords
2. Implement exception object type
3. Add stack unwinding mechanism
4. Implement exception propagation
5. Add finally block cleanup guarantees
6. Update runtime to handle exceptions
7. Capture stack traces for debugging



### 22. Advanced Loop Constructs (LOW PRIORITY)

**What's Missing**:
```c
// Modern LPC - foreach
foreach(string name in names) {
    write(name + "\n");
}

foreach(string key, int value in mapping) {
    write(key + ": " + value + "\n");
}

// For loop with declaration
for(int i = 0; i < 10; i++) {
    write(i + "\n");
}

// Break and continue
while(1) {
    if (condition) break;
    if (other) continue;
}
```

**Current NLPC**:
```c
// Only while loops
int i = 0;
while(i < 10) {
    write(i + "\n");
    i++;
}

// No foreach - must iterate manually
// No for loops
// No break/continue
```

**Impact**:
- **Iteration verbosity**: Manual index management required
- **No early exit**: Can't break out of loops easily
- **No skip**: Can't skip iterations without complex logic
- **Less readable**: While loops less clear than for/foreach

**Implementation Requirements**:
1. Add `foreach` keyword and syntax
2. Implement `for` loop with initialization/condition/increment
3. Add `break` keyword to exit loops
4. Add `continue` keyword to skip iterations
5. Update compiler to handle new loop constructs
6. Implement loop control flow in interpreter
7. Support foreach with arrays and mappings



### 23. Coroutines and Async (LOW PRIORITY - FluffOS only)

**What's Missing**:
```c
// FluffOS - Coroutines
async void fetch_data() {
    string data = await http_get("https://api.example.com/data");
    process_data(data);
}

// Parallel execution
async void load_all() {
    await parallel({
        load_players(),
        load_items(),
        load_rooms()
    });
}
```

**Current NLPC**:
```c
// Must use callbacks and alarms
void fetch_data() {
    http_get("https://api.example.com/data", "on_data_received");
}

void on_data_received(string data) {
    process_data(data);
}
```

**Impact**:
- **Callback complexity**: Nested callbacks create "callback hell"
- **Code flow**: Async code split across multiple functions
- **Error handling**: Harder to handle errors in async operations
- **Readability**: Linear async code easier to understand
- **FluffOS-specific**: Not available in LDMud

**Implementation Requirements**:
1. Add `async` and `await` keywords
2. Implement coroutine state machine
3. Add promise/future mechanism
4. Implement async function transformation
5. Add parallel execution support
6. Update runtime for cooperative multitasking
7. Handle async error propagation
8. Very complex - FluffOS-specific feature



## Priority Recommendations

### Immediate High-Value Additions (Phase 1)
1. **Dynamic Arrays** - Fundamental data structure, enables many other features
2. **Mappings** - Critical for complex data, better than table workaround
3. **Automatic Virtual Filesystem** - Major quality-of-life improvement

### Important Enhancements (Phase 2)
4. **Closures** - Enables functional programming, callbacks, better APIs
5. **Advanced Type System** - Mixed type, void, type checking
6. **Modern String Handling** - Indexing, slicing, regex

### Nice-to-Have Features (Phase 3)
7. **Structs** - Lightweight data structures
8. **Call-by-Reference** - Better function APIs
9. **Preprocessor** - Macros, conditional compilation
10. **True Inheritance** - Better than attach(), but attach() works

### Low Priority (Phase 4)
11. Multiple inheritance (already works via attach)
12. Virtual functions (workarounds exist)
13. Abstract classes (not critical)
14. File watching (manual update works)
15. Include paths (can use full paths)
16. JSON support (manual parsing works)
17. SQLite (tables work well)
18. Error objects (string errors sufficient)
19. Valid_read/valid_write (manual checks work)
20. Varargs and default parameters
21. Exception handling
22. Advanced loop constructs
23. Coroutines (FluffOS-specific)



## Backward Compatibility Strategy

### Keep Working
- `attach()` system - Keep alongside `inherit`
- `table_*()` functions - Keep alongside mappings
- String functions (`leftstr`, `midstr`, `rightstr`) - Keep alongside indexing
- `hide()`/`unhide()` - Make no-ops or permission-only

### Deprecate Gracefully
- Document old approaches as "legacy"
- Provide migration guides
- Keep old functions working
- Add warnings in documentation

### New Features
- Add new keywords: `inherit`, `mixed`, `void`, `closure`, `struct`
- Add new operators: `[]`, `..`, `&`, `::`
- Add new built-ins: `sizeof()`, `keys()`, `values()`, `funcall()`
- Extend existing built-ins to work with new types



## Implementation Complexity Estimates

| Feature | Complexity | Effort | Dependencies |
|---------|-----------|--------|--------------|
| Arrays | High | 4-6 weeks | Type system, memory management |
| Mappings | High | 4-6 weeks | Type system, hash tables |
| Virtual FS | Medium | 2-3 weeks | File system, permissions |
| Closures | Very High | 6-8 weeks | Type system, context capture |
| Structs | Medium | 3-4 weeks | Type system, memory |
| Type System | High | 4-5 weeks | Compiler, runtime |
| String Ops | Low | 1-2 weeks | Parser, operators |
| References | Medium | 2-3 weeks | Compiler, interpreter |
| Preprocessor | Medium | 3-4 weeks | Lexer, parser |
| Inheritance | High | 4-6 weeks | Object system, dispatch |



## Testing Strategy

### Unit Tests
- Test each new feature in isolation
- Test edge cases and error conditions
- Test interaction with existing features

### Integration Tests
- Test new features together
- Test backward compatibility
- Test migration paths

### Performance Tests
- Benchmark new features vs workarounds
- Test memory usage
- Test execution speed

### Compatibility Tests
- Test existing code still works
- Test deprecated features
- Test migration guides

