# NLPC vs Modern LPC - Feature Comparison

## Overview

This document compares NLPC (NetCI LPC) with modern LPC implementations (LDMud 3.6+ and FluffOS 3.0+), identifying missing features and opportunities for enhancement. NLPC represents an early 1990s snapshot of LPC evolution, while modern LPC has continued to develop with significant new features.

## Executive Summary

**NetCI 2.0 NLPC Status**: Modern LPC implementation with arrays, mappings, inheritance, and comprehensive built-ins
**Original NLPC (1995)**: Early LPC implementation with basic object-oriented features via attach()
**Target (LDMud/FluffOS)**: Advanced language with closures, structs, and sophisticated syntactic sugar

### ✅ NetCI 2.0 NLPC Has Implemented (vs Original):
1. ✅ **Dynamic arrays** - Full heap-allocated arrays with `({ })` literals
2. ✅ **Mappings** - Full hash table implementation with `([ ])` literals
3. ✅ **True inheritance** - `inherit` keyword with `::parent()` calls
4. ✅ **Array operations** - explode, implode, sizeof, sort_array, etc.
5. ✅ **Mapping operations** - keys, values, member, map_delete
6. ✅ **Type introspection** - typeof() with T_ARRAY, T_MAPPING, etc.
7. ✅ **Preprocessor** - #define macros with parameters, #include
8. ✅ **Modern loops** - for, do-while (in addition to while)
9. ✅ **LPC-standard file ops** - get_dir, read_file, write_file, etc.
10. ✅ **Virtual functions** - Function override with parent access
11. ✅ **String-based function pointers** - call_other, alarm, sort_array use "func_name"
12. ✅ **Automatic file discovery** - Files auto-registered via discover_file() and make_entry()
13. ✅ **Implicit varargs/default params** - Can call with fewer args, missing = INTEGER 0

### ❌ Still Missing (vs Modern LPC):
1. ❌ Closures/lambdas (have string-based function pointers, not closures)
2. ❌ Structs (must use objects or mappings)
3. ❌ String/array indexing syntax (`str[0]`, `arr[1..5]`)
4. ❌ Exception handling (try/catch/throw)
5. ❌ foreach keyword (docs show pattern, not implemented)
6. ❌ break/continue (must use flags)
7. ❌ Regex support
8. ❌ Conditional compilation (#ifdef/#ifndef)
9. ❌ Explicit varargs syntax (have implicit via optional params)

## Data Types Comparison

### NetCI 2.0 NLPC Types

```c
// Basic types
int count;                          // Integer
string name;                        // String
object player;                      // Object reference

// Complex types (NetCI 2.0)
int *numbers = ({ 1, 2, 3 });      // Dynamic heap array
string *names = ({ "A", "B" });    // Array of strings
object *items = ({ sword, shield }); // Array of objects
mapping stats = ([ "str": 10, "dex": 15 ]); // Hash table

// Type-flexible (no mixed keyword, but typeof() works)
value = 42;                         // Can hold any type
value = "string";                   // Type changes dynamically
value = ({ 1, 2, 3 });             // Can be array
value = ([ "key": "val" ]);        // Can be mapping
if (typeof(value) == T_ARRAY) { }  // Runtime type check
```

**What's Available**:
- ✅ Integers
- ✅ Strings (immutable)
- ✅ Objects
- ✅ **Arrays** (heap-allocated, dynamic)
- ✅ **Mappings** (hash tables)
- ⚠️ Flexible typing (no `mixed` keyword, but typeof() works)
- ❌ No void type
- ❌ No float/double
- ❌ No structs


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

| Feature | Original NLPC (1995) | NetCI 2.0 NLPC | LDMud/FluffOS | Notes |
|---------|------|------|---------------|--------|
| **Data Structures** |
| Arrays | ❌ No | ✅ **FULL** | ✅ Yes | Heap-allocated with `({ })` literals, full ops |
| Mappings | ❌ No | ✅ **FULL** | ✅ Yes | Heap-allocated with `([ ])` literals, hash tables |
| Structs | ❌ No | ❌ No | ✅ Yes | Can use objects or mappings |
| Mixed type | ❌ No | ⚠️ Partial | ✅ Yes | No keyword, but flexible typing via typeof() |
| **Functions** |
| Closures/Lambdas | ❌ No | ❌ No | ✅ Yes | No closures, but string-based function pointers work |
| Function Pointers | ❌ No | ✅ **String-based** | ✅ Yes | call_other(), alarm(), sort_array() use "func_name" |
| Varargs | ❌ No | ⚠️ **Implicit** | ✅ Yes | Can call with fewer args, missing params = INTEGER 0 |
| Default params | ❌ No | ⚠️ **Implicit** | ✅ Yes | Missing args default to INTEGER 0 (undocumented) |
| Call by reference | ❌ No | ⚠️ Partial | ✅ Yes | fread() uses &pos pattern, not general |
| **Object System** |
| Inheritance | ❌ attach() | ✅ **FULL** | ✅ Yes | `inherit` keyword + `::parent()` calls |
| Multiple inheritance | ⚠️ attach() | ✅ **FULL** | ✅ Yes | Multiple `inherit` statements supported |
| Virtual functions | ❌ No | ✅ **FULL** | ✅ Yes | Function override with :: parent access |
| Abstract classes | ❌ No | ❌ No | ✅ Yes | No enforcement mechanism |
| **String Handling** |
| String indexing | ❌ No | ❌ No | ✅ `str[0]` | Have leftstr/rightstr/midstr functions |
| String slicing | ❌ No | ❌ No | ✅ `str[1..5]` | Have midstr(str, pos, len) |
| String mutation | ❌ No | ❌ No | ✅ `str[0]='X'` | Strings are immutable |
| Regex | ❌ No | ❌ No | ✅ Yes | Manual parsing required |
| **Filesystem** |
| Auto-managed | ❌ hide/unhide | ✅ **Auto-register** | ✅ Automatic | Files auto-added via discover_file() + make_entry() |
| File watching | ❌ No | ❌ No | ✅ Yes | Manual recompile needed |
| Include paths | ❌ Limited | ✅ **FULL** | ✅ Yes | INCLUDE_PATH configured |
| **Preprocessor** |
| #define | ❌ No | ✅ **FULL** | ✅ Yes | Constants and macros with parameters |
| #ifdef | ❌ No | ❌ No | ✅ Yes | No conditional compilation |
| #include | ⚠️ Basic | ✅ **FULL** | ✅ Yes | Supports <> and "" includes |
| Macros | ❌ No | ✅ **FULL** | ✅ Yes | #define with parameters |
| **Loop Constructs** |
| while | ✅ Yes | ✅ **FULL** | ✅ Yes | Standard while loops |
| for | ❌ No | ✅ **FULL** | ✅ Yes | for(init; cond; inc) syntax |
| do-while | ❌ No | ✅ **FULL** | ✅ Yes | do { } while(cond) syntax |
| foreach | ❌ No | ⚠️ Docs only | ✅ Yes | No keyword, used in examples/docs |
| break/continue | ❌ No | ❌ No | ✅ Yes | Must use flags or returns |
| **Array Operations** |
| Array literals | ❌ No | ✅ **FULL** | ✅ Yes | `({ })` syntax implemented |
| sizeof() | ❌ No | ✅ **FULL** | ✅ Yes | Works on arrays and mappings |
| explode() | ❌ No | ✅ **FULL** | ✅ Yes | String to array splitting |
| implode() | ❌ No | ✅ **FULL** | ✅ Yes | Array to string joining |
| member_array() | ❌ No | ✅ **FULL** | ✅ Yes | Find element in array |
| sort_array() | ❌ No | ✅ **FULL** | ✅ Yes | Sort with function name |
| reverse() | ❌ No | ✅ **FULL** | ✅ Yes | Reverse array order |
| unique_array() | ❌ No | ✅ **FULL** | ✅ Yes | Remove duplicates |
| Array slicing | ❌ No | ❌ No | ✅ `arr[1..5]` | RANGE_TOK defined but not implemented |
| **Mapping Operations** |
| Mapping literals | ❌ No | ✅ **FULL** | ✅ Yes | `([ ])` syntax implemented |
| keys() | ❌ No | ✅ **FULL** | ✅ Yes | Get array of keys |
| values() | ❌ No | ✅ **FULL** | ✅ Yes | Get array of values |
| member() | ❌ No | ✅ **FULL** | ✅ Yes | Check key existence |
| map_delete() | ❌ No | ✅ **FULL** | ✅ Yes | Remove key from mapping |
| **Type Introspection** |
| typeof() | ❌ No | ✅ **FULL** | ✅ Yes | Returns T_INT, T_STRING, T_ARRAY, T_MAPPING, T_OBJECT |
| intp() | ❌ No | ⚠️ Via typeof | ✅ Yes | Use typeof(x) == T_INT |
| stringp() | ❌ No | ⚠️ Via typeof | ✅ Yes | Use typeof(x) == T_STRING |
| objectp() | ❌ No | ⚠️ Via typeof | ✅ Yes | Use typeof(x) == T_OBJECT |
| arrayp() | ❌ No | ⚠️ Via typeof | ✅ Yes | Use typeof(x) == T_ARRAY |
| mappingp() | ❌ No | ⚠️ Via typeof | ✅ Yes | Use typeof(x) == T_MAPPING |
| **File Operations** |
| get_dir() | ❌ No | ✅ **FULL** | ✅ Yes | Returns string array of filenames |
| read_file() | ❌ fread | ✅ **FULL** | ✅ Yes | LPC-standard name (fread alias) |
| write_file() | ❌ fwrite | ✅ **FULL** | ✅ Yes | LPC-standard name (fwrite alias) |
| remove() | ❌ rm | ✅ **FULL** | ✅ Yes | LPC-standard name (rm alias) |
| rename() | ❌ mv | ✅ **FULL** | ✅ Yes | LPC-standard name (mv alias) |
| file_size() | ❌ No | ✅ **FULL** | ✅ Yes | Get byte count |
| **Advanced Features** |
| Coroutines | ❌ No | ❌ No | ✅ Yes (FluffOS) | Use alarm()/call_out() |
| Async/await | ❌ No | ❌ No | ✅ Yes (FluffOS) | Use callbacks |
| JSON support | ❌ No | ❌ No | ✅ Yes | Manual string building |
| SQLite | ❌ No | ❌ No | ✅ Yes (FluffOS) | Have table_*() system |
| **Error Handling** |
| try/catch | ❌ No | ❌ No | ✅ Yes | Manual error checking |
| Error objects | ❌ No | ❌ No | ✅ Yes | String error messages |
| **Security** |
| Valid_read | ❌ No | ⚠️ Partial | ✅ Yes | Manual checks in sefuns |
| Valid_write | ❌ No | ⚠️ Partial | ✅ Yes | Manual checks in sefuns |
| Privilege system | ✅ priv() | ✅ **priv()** | ✅ Similar | Set/query privilege levels |



## Missing Feature Deep Dive

> **Note**: This section was written for Original NLPC (1995). Many features have been implemented in NetCI 2.0. See the Feature Comparison Matrix above for current status.

### 1. Dynamic Arrays ✅ **IMPLEMENTED IN NETCI 2.0**

**Original Problem (Now Solved)**:
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



### 2. Mappings ✅ **IMPLEMENTED IN NETCI 2.0**

**Original Problem (Now Solved)**:
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



### 10. Object Inheritance ✅ **IMPLEMENTED IN NETCI 2.0**

**Original Problem (Now Solved)**:
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



## Priority Recommendations (Updated for NetCI 2.0)

### ✅ Already Implemented (Celebrate!)
1. ✅ **Dynamic Arrays** - COMPLETE with literals and full operations
2. ✅ **Mappings** - COMPLETE with hash tables and full operations  
3. ✅ **True Inheritance** - COMPLETE with inherit and :: syntax
4. ✅ **Preprocessor** - COMPLETE with #define macros
5. ✅ **Type Introspection** - COMPLETE with typeof()
6. ✅ **Modern File Operations** - COMPLETE with get_dir, read_file, etc.
7. ✅ **Advanced Loop Constructs** - COMPLETE with for and do-while
8. ✅ **Virtual Functions** - COMPLETE with function override

### High Priority (Phase 1 - Syntax Sugar)
1. **Array/String Slicing Syntax** - `arr[1..5]`, `str[0..3]` (RANGE_TOK defined, needs implementation)
2. **String Indexing** - `str[0]`, `str[i]` (sugar for leftstr/midstr)
3. **foreach Keyword** - Formalize the documentation pattern
4. **break/continue** - Early loop exit

### Medium Priority (Phase 2 - Language Features)
5. **True Closures/Lambdas** - (have string-based pointers, need closures with context capture)
6. **Structs** - Lightweight data structures
7. **Regex Support** - Pattern matching
8. **Conditional Compilation** - #ifdef/#ifndef
9. **Exception Handling** - try/catch/throw

### Low Priority (Phase 3 - Nice to Have)
11. **Varargs** - Variable argument functions
12. **Default Parameters** - Optional function parameters
13. **General Call-by-Reference** - Beyond &pos pattern
14. **void Return Type** - Explicit no-return functions
15. **mixed Keyword** - Explicit any-type declaration
16. **Type Checking Functions** - intp(), stringp(), etc. (vs typeof())
17. **Static/Private/Protected** - Access modifiers (flags exist but not exposed)
18. **Abstract Classes** - Compile-time enforcement
19. **JSON Support** - Built-in encode/decode
20. **File Watching** - Auto-recompile

### Not Planned (Out of Scope)
21. **SQLite** - table_*() system sufficient
22. **Coroutines/Async** - FluffOS-specific, alarm() works
23. **Float/Double** - Integer math sufficient for MUDs
24. **Error Objects** - String errors work fine



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


## NetCI 2.0 NLPC Achievement Summary

### Major Accomplishments ✅

NetCI 2.0 NLPC has successfully implemented **most of the critical features** that differentiate modern LPC from early 1990s implementations:

**Core Language Features:**
- ✅ Full dynamic arrays with literals `({ })`
- ✅ Full hash table mappings with literals `([ ])`
- ✅ True object inheritance with `inherit` keyword
- ✅ Virtual function override with `::parent()` calls
- ✅ Multiple inheritance support
- ✅ Type introspection via `typeof()`
- ✅ Full preprocessor with `#define` macros

**Standard Library:**
- ✅ Complete array operations (explode, implode, sizeof, sort_array, reverse, unique_array, member_array)
- ✅ Complete mapping operations (keys, values, member, map_delete)
- ✅ LPC-standard file functions (get_dir, read_file, write_file, remove, rename, file_size)
- ✅ Advanced loop constructs (for, do-while, while)

**Development Quality:**
- ✅ Zero compiler warnings (recently cleaned up)
- ✅ Comprehensive documentation (netci-functions.md)
- ✅ Working examples in Melville mudlib
- ✅ Reference-counted memory management
- ✅ Hash-based mapping implementation

### Comparison to Modern LPC

| Category | NetCI 2.0 Completion | Notes |
|----------|---------------------|--------|
| **Data Structures** | 85% | Arrays ✅, Mappings ✅, Structs ❌ |
| **Object System** | 95% | Inheritance ✅, Virtual ✅, Abstract ❌ |
| **Type System** | 70% | typeof() ✅, mixed keyword ❌, void ❌ |
| **Loops** | 80% | for/while/do ✅, foreach ❌, break/continue ❌ |
| **String Ops** | 60% | Functions ✅, Indexing syntax ❌, Regex ❌ |
| **Array Ops** | 90% | All functions ✅, Slicing syntax ❌ |
| **Preprocessor** | 70% | #define ✅, #include ✅, #ifdef ❌ |
| **File System** | 95% | Modern functions ✅, Auto-discover ✅ |
| **Function Pointers** | 80% | String-based ✅, Closures/lambdas ❌ |
| **Advanced** | 25% | Exceptions ❌, Varargs ❌, Structs ❌ |

**Overall: NetCI 2.0 NLPC is ~78% feature-complete compared to modern LPC**

### What This Means

**For Developers:**
- NetCI 2.0 is a **fully modern LPC** for building MUDs
- Arrays and mappings work exactly like LDMud/FluffOS
- Inheritance system is complete and matches modern LPC
- Missing features are mostly syntactic sugar, not capabilities
- You can build complex systems using available features

**For Migration:**
- Code using arrays/mappings is directly portable
- `inherit` works the same way as modern LPC
- `attach()` system remains for backward compatibility
- Most LPC patterns can be implemented

**What's Actually Missing:**
- **Closures/lambdas** - Have string-based function pointers (call_other, alarm, sort_array), but not true closures
- **foreach** - Must use for/while loops with explicit indexing
- **Slicing syntax** - Must use midstr() functions (RANGE_TOK exists but not implemented)
- **Structs** - Must use mappings or objects
- **Exceptions** - Must check return values manually

### Development Priorities

**Quick Wins (Syntax Sugar):**
1. Implement RANGE_TOK for slicing (`arr[1..5]`)
2. Add string indexing sugar (`str[0]`)
3. Implement foreach keyword
4. Add break/continue

**Long-term (Language Features):**
1. Closures/function pointers
2. Structs
3. Exception handling
4. Regex support

**Quality of Life:**
1. Conditional compilation (#ifdef)
2. Varargs support
3. Better error messages

### Conclusion

NetCI 2.0 NLPC has evolved **far beyond** the original 1995 implementation. The modernization document's "Key Missing Features" list is **largely obsolete** - NetCI 2.0 has successfully implemented arrays, mappings, inheritance, and comprehensive built-ins that make it a competitive modern LPC implementation.

The remaining gaps are primarily:
- Advanced language features (true closures/lambdas, structs, exceptions)
- Syntactic conveniences (slicing syntax, foreach, break/continue)
- Quality-of-life improvements (#ifdef, better error messages)

None of these prevent building production-quality MUDs. NetCI 2.0 is **ready for serious development**.

