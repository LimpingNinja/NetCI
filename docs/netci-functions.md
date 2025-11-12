# NetCI and NLPC Reference Guide

Welcome to the comprehensive reference for NetCI's programming environment! This guide covers everything you need to know about writing softcode for NetCI—from the fundamental data types of the NLPC language to the rich library of functions at your disposal.

Whether you're building your first room or architecting a complex game system, this document is your authoritative source. We've organized it to be both a learning resource and a quick reference, with practical examples throughout.

---

# Table of Contents

- [Introduction](#introduction)
- [NLPC Types](#nlpc-types)
- [Language Syntax Features](#language-syntax-features)
  - [inherit - Code Inheritance](#inherit---code-inheritance)
  - [Array Literals](#array-literals)
  - [Mapping Literals](#mapping-literals)
  - [Parent Function Calls](#parent-function-calls)
- [Efuns (External Functions)](#efuns)
  - **Command System**: [add_verb](#add_verb), [add_xverb](#add_xverb), [remove_verb](#remove_verb), [set_localverbs](#set_localverbs), [localverbs](#localverbs), [next_verb](#next_verb)
  - **Scheduling**: [alarm](#alarm), [remove_alarm](#remove_alarm)
  - **Security**: [set_priv](#set_priv), [priv](#priv), [crypt](#crypt)
  - **Type Conversion**: [itoa](#itoa), [atoi](#atoi), [chr](#chr), [asc](#asc), [otoa](#otoa), [atoo](#atoo), [otoi](#otoi), [itoo](#itoo)
  - **Serialization**: [save_value](#save_value), [restore_value](#restore_value)
  - **Network/DNS**: [get_hostname](#get_hostname), [get_address](#get_address)
  - **Device I/O**: [send_device](#send_device), [connect_device](#connect_device), [reconnect_device](#reconnect_device), [disconnect_device](#disconnect_device), [flush_device](#flush_device), [get_devconn](#get_devconn), [get_devport](#get_devport), [get_devnet](#get_devnet), [get_devidle](#get_devidle), [get_conntime](#get_conntime)
  - **File System**: [cat](#cat), [ls](#ls), [rm](#rm), [ferase](#ferase), [cp](#cp), [mv](#mv), [mkdir](#mkdir), [rmdir](#rmdir), [fread](#fread), [fwrite](#fwrite), [fstat](#fstat), [fowner](#fowner), [chmod](#chmod), [chown](#chown), [hide](#hide), [unhide](#unhide), [edit](#edit), [in_editor](#in_editor)
  - **String Functions**: [strlen](#strlen), [leftstr](#leftstr), [rightstr](#rightstr), [midstr](#midstr), [instr](#instr), [subst](#subst), [sprintf](#sprintf), [sscanf](#sscanf), [upcase](#upcase), [downcase](#downcase), [is_legal](#is_legal), [replace_string](#replace_string)
  - **Array Functions**: [explode](#explode), [implode](#implode), [member_array](#member_array), [sort_array](#sort_array), [reverse](#reverse), [unique_array](#unique_array), [sizeof](#sizeof)
  - **Mapping Functions**: [keys](#keys), [values](#values), [map_delete](#map_delete), [member](#member), [sizeof](#sizeof)
  - **Object Management**: [compile_object](#compile_object), [compile_string](#compile_string), [get_object](#get_object), [clone](#clone), [clone_object](#clone_object), [destruct](#destruct), [move_object](#move_object), [contents](#contents), [next_object](#next_object), [location](#location)
  - **Object Introspection**: [this_object](#this_object), [this_player](#this_player), [caller_object](#caller_object), [parent](#parent), [next_child](#next_child), [next_proto](#next_proto), [prototype](#prototype), [get_master](#get_master), [is_master](#is_master), [typeof](#typeof)
  - **Composition System**: [attach](#attach), [detach](#detach), [this_component](#this_component)
  - **Input/Output**: [redirect_input](#redirect_input), [input_to](#input_to), [get_input_func](#get_input_func), [write](#write), [syswrite](#syswrite)
  - **Player Management**: [next_who](#next_who), [set_interactive](#set_interactive), [interactive](#interactive), [connected](#connected)
  - **Miscellaneous**: [call_other](#call_other), [random](#random), [time](#time), [mktime](#mktime), [command](#command), [iterate](#iterate), [sysctl](#sysctl), [syslog](#syslog), [table_set](#table_set), [table_get](#table_get), [table_delete](#table_delete)
- [Simulated Efuns](#simulated-efuns)
  - **Object Lookup**: [find_object](#find_object), [present](#present)
  - **Object Creation**: [compile_object (sefun)](#compile_object-sefun), [get_object (sefun)](#get_object-sefun), [clone_object (sefun)](#clone_object-sefun)
  - **String Utilities**: [capitalize](#capitalize), [lowercase](#lowercase), [uppercase](#uppercase), [trim](#trim)
  - **Array Utilities**: [contains](#contains), [remove_element](#remove_element)
  - **Player Utilities**: [is_player](#is_player), [is_living](#is_living), [get_short](#get_short), [get_long](#get_long)
  - **Security**: [has_privilege](#has_privilege), [is_wizard](#is_wizard), [is_admin](#is_admin)
  - **Output**: [tell](#tell), [tell_room](#tell_room), [say](#say), [write (sefun)](#write-sefun)
  - **Heartbeat**: [set_heart_beat](#set_heart_beat), [do_heart_beat](#do_heart_beat)
- [Applies / Callbacks](#applies--callbacks)
  - **Master Object**: [connect](#connect), [disconnect](#disconnect), [valid_read](#valid_read), [valid_write](#valid_write)
  - **Object Lifecycle**: [init](#init), [allow_attach](#allow_attach)
  - **System Queries**: [listen](#listen)
- [Notes & Caveats](#notes--caveats)

---

# Introduction

NetCI is a MUD server built on a custom virtual machine that executes NLPC (NetCI LPC) code. If you're familiar with LPC from other MUD drivers like MudOS or DGD, you'll feel right at home—but NetCI has its own personality and quirks that make it unique.

## What You'll Find Here

This reference documents three main categories of callable functions:

**Efuns (External Functions)**: These are the core system calls implemented directly in the NetCI driver (written in C). They're fast, powerful, and form the foundation of everything you can do. Think of them as your direct line to the engine. You'll find them defined in `src/compile1.c` (the `scall_array`) and `src/protos.h`.

**Simulated Efuns (SEFUNs)**: These are convenience functions provided by `/sys/auto.c` that are automatically available to all objects. They're written in NLPC and build on top of efuns to provide higher-level functionality. They feel just like efuns when you call them, but they're implemented in softcode like your own functions.

**Applies and Callbacks**: These are functions that the driver or mudlib calls *on your objects* when certain events occur. For example, `init()` is called when your object is cloned, and `connect()` is called on the master object when a player connects. Understanding these is key to making your objects respond to the world around them.

**Privileged Wrappers**: Special functions in `/boot.c` that wrap sensitive operations like compilation and cloning with security checks. These ensure that only authorized code can perform potentially dangerous operations.

## How to Use This Guide

Each function entry follows a consistent format inspired by traditional Unix man pages:

- **NAME**: The function name and a brief description
- **SYNOPSIS**: The function signature with parameter types and return type
- **DESCRIPTION**: Detailed explanation of what the function does, including return values, side effects, and important behavioral notes
- **EXAMPLE**: Practical code showing typical usage
- **SEE ALSO**: Related functions you might want to explore

We've validated every detail against the actual engine source code, so you can trust that what you read here matches what the driver actually does.

---

# NLPC Types

NLPC is a dynamically-typed language with a small set of fundamental types. Understanding these types is essential because they determine how data flows through your programs and what operations are valid.

## Basic Types

### INTEGER
A signed long integer (typically 32 or 64 bits depending on your platform). This is your workhorse for numbers—counters, hit points, timestamps, you name it.

```c
int hp = 100;
int damage = 15;
hp -= damage;  // hp is now 85
```

**Special note**: Many string functions will treat the integer `0` as an empty string for convenience. This is a quirk inherited from C and can be handy, but watch out for it.

### STRING
A NUL-terminated character array, just like in C. Strings are immutable in NLPC—when you concatenate or modify them, you're creating new strings.

```c
string name = "Gandalf";
string title = "the Grey";
string full = name + " " + title;  // "Gandalf the Grey"
```

### OBJECT
A reference to a runtime object—either a prototype (the "program" loaded from a .c file) or a clone (an instance created from a prototype). Objects are the heart of your MUD: rooms, players, items, NPCs, everything.

```c
object room = atoo("/world/start/tavern");
object sword = clone_object("/items/weapons/longsword");
```

Use `otoa()` to convert an object to its file path, or `otoi()` to get its numeric ID.

### ARRAY
A dynamically-sized, heap-allocated array that can hold any mix of types. Arrays are reference-counted, so they're automatically freed when no longer needed.

Arrays are declared with `*` after the type name. If you want an array that can hold any type, just use `*` without a type prefix (though `int *`, `string *`, and `object *` are also valid for documentation purposes).

```c
// Array declarations - all valid
int *numbers = ({ 1, 2, 3 });
string *names = ({ "Alice", "Bob" });
object *items = ({ sword, shield });

// Untyped array (can hold anything)
*inventory = ({ sword, shield, potion });
int count = sizeof(inventory);  // 3
```

**Important Note**: NLPC does **not** have a `mixed` keyword. In documentation and examples, you may see `mixed *` used to indicate "an array of any type", but in actual NLPC code, you would just write `*` or use a specific type like `int *`, `string *`, or `object *`. The type system is flexible—you don't need to declare types for variables in many cases.

Create arrays with the literal syntax `({ ... })` or with functions like `explode()` and `allocate()`.

### MAPPING
A hash table that maps keys to values. Keys can be strings, integers, or objects. Mappings are perfect for storing properties, tracking relationships, or building lookup tables.

```c
mapping stats = ([ "str": 18, "dex": 14, "con": 16 ]);
int strength = stats["str"];  // 18
stats["wis"] = 12;            // add a new key
```

**Important**: The keys in a mapping are stored in a hash table with chaining, which means they have no guaranteed order when you iterate. However, `keys()` and `values()` will always return arrays in corresponding order—`keys(m)[i]` matches `values(m)[i]`.

## Type Introspection

The `typeof()` efun returns a numeric code indicating a value's type. This is useful when you need to handle different types differently:

```c
// Note: NLPC doesn't have "mixed" keyword - just don't specify a type
value = /* ... */;  // No type declaration needed
if (typeof(value) == T_ARRAY) {
    // handle array
} else if (typeof(value) == T_MAPPING) {
    // handle mapping
}
```

**Documentation Note**: Throughout this guide, you may see `mixed` used in function signatures and examples to indicate "any type". This is for documentation clarity only—NLPC does not have a `mixed` keyword. In actual code, you simply omit the type declaration or use a specific type like `int`, `string`, `object`, etc.

## Function Categories

Now that you understand the data types, let's talk about the different kinds of functions you'll encounter:

### EFUN / SFUN (System Functions)
These are the built-in functions implemented in the driver's C code. In the source, they're named with an `s_` prefix (like `s_sizeof`, `s_explode`), but you call them without the prefix. They're fast and form the primitive operations everything else builds on.

### SEFUN (Simulated External Functions)
These live in `/sys/auto.c` and are written in NLPC. They're automatically available to all objects, just like efuns. The distinction is mostly academic from a user perspective—they work the same way. Common examples include `find_object()`, `present()`, and convenience wrappers like `capitalize()`.

### Applies and Callbacks
These aren't functions you call—they're functions the driver calls *on your objects*. When you define `init()` in your object, the driver calls it at the right time. When a player types a command, the driver might call your `cmd_hook()`. Understanding the apply/callback contract is essential for making objects that interact properly with the game world.

---

# Language Syntax Features

These are language-level features, not efuns. They're part of the NLPC syntax itself and are essential to understand before diving into the function libraries.

## inherit - Code Inheritance

### SYNTAX
```c
inherit "path/to/file";
```

### DESCRIPTION
The `inherit` keyword allows you to inherit code from another object, similar to class inheritance in other languages. When you inherit a file, you get all its functions and global variables.

**Key points**:
- `inherit` statements must appear at the **top of the file**, before any variable declarations or functions
- You can inherit multiple files (multiple inheritance)
- Inherited functions can be overridden by defining them locally
- Use `::function()` to call the parent version of an overridden function
- Global variables from parents are accessible directly
- Variable name conflicts cause compilation errors

### EXAMPLES
```c
// Basic inheritance
inherit "/inherits/object";

// Multiple inheritance
inherit "/inherits/object";
inherit "/inherits/container";

// Override parent function
inherit "/inherits/living";

// Override take_damage but call parent version
void take_damage(int amount) {
    // Custom logic first
    if (has_shield()) {
        amount = amount / 2;
    }
    
    // Call parent implementation
    ::take_damage(amount);
}

// Access inherited variables
inherit "/inherits/object";

void show_description() {
    // 'properties' mapping is inherited from object.c
    write(properties["description"] + "\n");
}
```

### INHERITANCE RULES
1. **Order matters**: Inherits are processed top-to-bottom
2. **No late inherits**: Must be before variables and functions
3. **No cycles**: Can't create circular inheritance
4. **Variable conflicts**: Same-named variables in multiple parents cause errors
5. **Function override**: Local functions shadow inherited ones

## Array Literals

### SYNTAX
```c
({ element1, element2, element3 })
```

### DESCRIPTION
Create arrays inline using `({ })` syntax. Elements can be any type and can be expressions.

### EXAMPLES
```c
int *numbers = ({ 1, 2, 3, 4, 5 });
string *names = ({ "Alice", "Bob", "Charlie" });
object *items = ({ clone("/items/sword"), clone("/items/shield") });
int *computed = ({ x + y, x * 2, y / 2 });
```

## Mapping Literals

### SYNTAX
```c
([ key1: value1, key2: value2 ])
```

### DESCRIPTION
Create mappings (associative arrays) inline using `([ ])` syntax.

### EXAMPLES
```c
mapping stats = ([ "hp": 100, "mp": 50, "level": 5 ]);
mapping exits = ([ "north": "/world/forest", "south": "/world/town" ]);
```

## Parent Function Calls

### SYNTAX
```c
::function_name(args)
```

### DESCRIPTION
Call the parent version of an overridden function using `::` prefix. This walks up the inheritance chain to find the next implementation.

### EXAMPLES
```c
inherit "/inherits/living";

void init() {
    ::init();  // Call parent's init first
    // Then do our own initialization
    set_name("goblin");
}

void take_damage(int amount) {
    write("Ouch!\n");
    ::take_damage(amount);  // Call parent's damage handling
}
```

---

# Efuns

Efuns are the core system functions provided by the NetCI driver. They're implemented in C for performance and provide the primitive operations that everything else builds on.

## Detailed Efuns Reference

### add_verb

#### NAME
add_verb()  -  register a command verb on this object

#### SYNOPSIS
```c
int add_verb(string action, string func);
```

#### DESCRIPTION
Registers a verb (command word) that players can use to interact with this object. When a player types the verb, the driver calls the specified function on your object to handle it.

For example, if you're writing a sword object, you might add verbs like "wield", "swing", or "examine". When a player types "wield sword", your `do_wield()` function gets called.

The function must exist in your object and should return 1 if it successfully handles the command, or 0 if it doesn't apply (allowing other objects to try). If you add a verb to a prototype, all clones inherit it automatically.

**Returns**: 0 on success, 1 on error (function doesn't exist, verb name too long, etc.).

#### EXAMPLE
```c
// In a sword object
void init() {
    add_verb("wield", "do_wield");
    add_verb("swing", "do_swing");
}

int do_wield(string arg) {
    if (arg != "sword") return 0;
    write("You wield the mighty sword!\n");
    return 1;
}
```

#### SEE ALSO
add_xverb(3), remove_verb(3), next_verb(3), set_localverbs(3), command(3)

---

### add_xverb

#### NAME
add_xverb()  -  register an exclusive (high-priority) verb

#### SYNOPSIS
```c
int add_xverb(string action, string func);
```

#### DESCRIPTION
Like `add_verb()`, but marks the verb as "exclusive" (xverb), giving it higher priority in command resolution. When the driver is searching for a verb handler, xverbs are checked before regular verbs.

This is useful for objects that need to override standard commands. For example, a magic mirror might want its "look" command to take precedence over the room's "look" command, showing a special reflection instead of the normal room description.

Use this sparingly—too many xverbs can create confusing command conflicts.

**Returns**: 0 on success, 1 on error (function doesn't exist, verb name too long, etc.).

#### EXAMPLE
```c
// In a magic mirror object
void init() {
    add_xverb("look", "do_special_look");
}

int do_special_look(string arg) {
    if (arg == "mirror" || arg == "in mirror") {
        write("You see a distorted reflection of yourself...\n");
        return 1;
    }
    return 0;  // Let other objects handle it
}
```

#### SEE ALSO
add_verb(3), remove_verb(3), set_localverbs(3)

---

### alarm

#### NAME
alarm()  -  schedule a delayed function call

#### SYNOPSIS
```c
int alarm(int seconds, string funcname);
```

#### DESCRIPTION
Schedules a function to be called on this object after a specified delay. This is your timer mechanism—use it for delayed actions, periodic events, timed effects, or anything that needs to happen "later".

The function will be called with no arguments after the specified number of seconds. If you need to pass data to the delayed function, store it in object variables.

You can have multiple alarms pending on the same object, even for the same function. Each alarm is independent.

**Returns**: 0 on success, 1 on error (function doesn't exist, invalid seconds, etc.).

#### EXAMPLE
```c
// Poison effect that ticks every 5 seconds
void apply_poison() {
    write("You've been poisoned!\n");
    alarm(5, "poison_tick");
}

void poison_tick() {
    write("The poison burns through your veins!\n");
    this_player()->add_hp(-5);
    
    // Schedule another tick
    alarm(5, "poison_tick");
}

// One-time delayed message
void delayed_greeting() {
    alarm(3, "say_hello");
}

void say_hello() {
    write("Hello there! (after 3 seconds)\n");
}
```

#### SEE ALSO
remove_alarm(3), time(3)

---

### remove_alarm

#### NAME
remove_alarm()  -  cancel scheduled alarms

#### SYNOPSIS
```c
int remove_alarm([string funcname]);
```

#### DESCRIPTION
Cancels pending alarms on this object. If you provide a function name, only alarms for that specific function are cancelled. If you omit the argument, ALL alarms on this object are cancelled.

This is essential for cleaning up when effects end early. For example, if a poison effect is cured, you want to stop the poison_tick alarms. Or if an object is being destructed, you should cancel its alarms to avoid calling functions on a dead object.

**Returns**: The number of alarms that were cancelled (0 if none were pending).

#### EXAMPLE
```c
// Cancel a specific alarm
void cure_poison() {
    int cancelled = remove_alarm("poison_tick");
    if (cancelled > 0) {
        write("The poison has been cured!\n");
    }
}

// Cancel all alarms when cleaning up
void destruct_me() {
    remove_alarm();  // Cancel everything
    destruct(this_object());
}

// Check if any alarms were pending
void check_alarms() {
    int count = remove_alarm("some_function");
    write("Cancelled " + itoa(count) + " alarms\n");
}
```

#### SEE ALSO
alarm(3)

---

### in_editor

#### NAME
in_editor()  -  check if an object is using the line editor

#### SYNOPSIS
```c
int in_editor(object obj);
```

#### DESCRIPTION
Tests whether an object (typically a player) is currently in the line editor. The line editor is a built-in text editing interface that players can use to write mail, edit files, or compose multi-line text.

When a player is in the editor, their input goes to the editor instead of normal command processing. This function lets you check that state—useful for preventing certain actions while editing, or for displaying appropriate prompts.

**Returns**: 1 if the object is in the editor, 0 if not.

#### EXAMPLE
```c
// Don't allow combat while editing
void attack_player(object target) {
    if (in_editor(target)) {
        write(target->query_name() + " is busy editing and can't fight!\n");
        return;
    }
    // Proceed with combat
}

// Show different prompt based on editor state
string get_prompt() {
    if (in_editor(this_player())) {
        return "[Editor] > ";
    }
    return "> ";
}
```

#### SEE ALSO
edit(3), redirect_input(3)

---

### set_priv

#### NAME
set_priv()  -  set the privilege flag on an object

#### SYNOPSIS
```c
int set_priv(object obj, int enabled);
```

#### DESCRIPTION
Sets or clears the PRIV (privilege) flag on an object. Privileged objects have special permissions in the system—they can perform operations that normal objects can't, like destructing other objects, accessing protected files, or calling sensitive system functions.

Only already-privileged code can grant privilege to other objects. This is a security mechanism to prevent untrusted code from escalating its own permissions. Typically, only the master object and system daemons have this flag set.

Pass `1` to grant privilege, `0` to revoke it.

**Returns**: 0 on success, 1 if the calling object doesn't have permission to set privilege.

#### EXAMPLE
```c
// In the master object or another privileged context
object daemon = compile_object("/sys/daemons/security_d");
if (set_priv(daemon, 1) == 0) {
    write("Granted privilege to security daemon\n");
}

// Revoke privilege
set_priv(daemon, 0);
```

#### SEE ALSO
priv(3), is_master(3)

---

### priv

#### NAME
priv()  -  check if an object has the privilege flag set

#### SYNOPSIS
```c
int priv(object obj);
```

#### DESCRIPTION
Tests whether an object has the PRIV flag set. This is useful for implementing security checks—before allowing a sensitive operation, you can verify that the calling object is privileged.

Most game objects won't have this flag. It's reserved for system infrastructure: the master object, security daemons, and other trusted code that needs elevated permissions.

**Returns**: 1 if the object is privileged, 0 if not.

#### EXAMPLE
```c
// Security check before a sensitive operation
void dangerous_operation() {
    if (!priv(caller_object())) {
        write("Access denied: insufficient privileges\n");
        return;
    }
    // Proceed with the operation
    write("Executing privileged operation...\n");
}

// Check if the master object is privileged (it should be)
object master = atoo("/boot");
if (priv(master)) {
    write("Master object has privilege (as expected)\n");
}
```

#### SEE ALSO
set_priv(3), caller_object(3), get_master(3)

---

### crypt

#### NAME
crypt()  -  secure password hashing and verification

#### SYNOPSIS
```c
string crypt(string password);
int crypt(string password, string hash);
```

#### DESCRIPTION
Provides cryptographically secure password hashing using bcrypt (Linux/BSD) or SHA-256 with 1000 rounds (macOS). This is the proper way to store and verify passwords in production.

**With one argument**: Generates a secure hash of the password with a random salt. Each call produces a different hash even for the same password.

**With two arguments**: Verifies a password against a previously generated hash. Returns 1 if the password matches, 0 if it doesn't.

The hash format includes the salt and algorithm identifier, so you only need to store the hash string. Never store plaintext passwords.

Hashing is intentionally slow (100-200ms) to resist brute-force attacks. This is a security feature, not a bug.

**Returns**: 
- One argument: hash string (60-128 characters depending on platform)
- Two arguments: 1 if password matches hash, 0 if not

#### EXAMPLE
```c
// Creating a user account
void create_user(string name, string password) {
    string hash = crypt(password);
    // Store hash in player file
    player.set_password_hash(hash);
}

// Verifying login
int check_login(string name, string password) {
    string stored_hash = player.query_password_hash();
    if (crypt(password, stored_hash)) {
        write("Login successful\n");
        return 1;
    }
    write("Invalid password\n");
    return 0;
}

// Using auto.c wrappers (recommended)
string hash = hash_password("mypassword");
if (verify_password("mypassword", hash)) {
    write("Password verified\n");
}
```

#### SEE ALSO
hash_password (auto.c), verify_password (auto.c), set_priv(3)

---

### itoa

#### NAME
itoa()  -  convert an integer to its string representation

#### SYNOPSIS
```c
string itoa(int n);
```

#### DESCRIPTION
Converts an integer to its decimal string representation. This is your go-to function when you need to display numbers to players or concatenate them with other strings.

Negative numbers get a leading minus sign, and zero becomes "0". Simple and reliable.

**Returns**: A string containing the decimal representation of the integer.

#### EXAMPLE
```c
int score = 1337;
string msg = "Your score is " + itoa(score) + " points!";
// msg is "Your score is 1337 points!"

int damage = -15;
write("You take " + itoa(damage) + " damage.\n");
// Displays: "You take -15 damage."
```

#### SEE ALSO
atoi(3), sprintf(3)

---

### atoi

#### NAME
atoi()  -  convert a string to an integer

#### SYNOPSIS
```c
int atoi(string s);
```

#### DESCRIPTION
Parses a string and extracts an integer from it. This is essential for processing player input—when someone types "get 5 coins", you'll use atoi() to turn that "5" into a number you can work with.

The function skips leading whitespace, handles an optional sign (+ or -), and reads digits until it hits a non-digit character. If the string doesn't start with a valid number (after whitespace), you get 0.

**Returns**: The parsed integer, or 0 if the string doesn't contain a valid number.

#### EXAMPLE
```c
string input = "42";
int num = atoi(input);  // 42

string mixed = "123abc";
int val = atoi(mixed);  // 123 (stops at 'a')

string negative = "-99";
int n = atoi(negative);  // -99

string invalid = "hello";
int zero = atoi(invalid);  // 0
```

#### SEE ALSO
itoa(3), sscanf(3), sprintf(3)

---

### chr

#### NAME
chr()  -  convert an ASCII code to a single-character string

#### SYNOPSIS
```c
string chr(int code);
```

#### DESCRIPTION
Takes an ASCII character code (0-127, or extended 0-255) and returns a single-character string containing that character. This is perfect for generating special characters, control codes, or building strings character by character.

Want a newline? `chr(10)`. Need a tab? `chr(9)`. Building a custom protocol with special delimiters? chr() is your friend.

**Returns**: A single-character string containing the character corresponding to the ASCII code.

#### EXAMPLE
```c
string newline = chr(10);  // "\n"
string tab = chr(9);       // "\t"
string bell = chr(7);      // Bell/beep character

// Build a string character by character
string greeting = chr(72) + chr(105);  // "Hi"

// Generate a delimiter
string delim = chr(1);  // SOH (Start of Heading)
```

#### SEE ALSO
asc(3), sprintf(3)

---

### asc

#### NAME
asc()  -  get the ASCII code of the first character in a string

#### SYNOPSIS
```c
int asc(string s);
```

#### DESCRIPTION
Returns the ASCII code (0-255) of the first character in the string. If you're parsing binary data, implementing custom protocols, or just need to know what character you're dealing with, asc() gives you the numeric value.

If the string is empty, the behavior is undefined (typically returns 0, but don't rely on it—check your string length first).

**Returns**: The ASCII code of the first character as an integer.

#### EXAMPLE
```c
string letter = "A";
int code = asc(letter);  // 65

string word = "Hello";
int first = asc(word);   // 72 (the 'H')

// Check for special characters
string input = "\n";
if (asc(input) == 10) {
    write("Got a newline!\n");
}

// Case conversion check
if (asc("a") >= 97 && asc("a") <= 122) {
    write("Lowercase letter\n");
}
```

#### SEE ALSO
chr(3), upcase(3), downcase(3)

---

### save_value

#### NAME
save_value()  -  serialize a value to a string for storage

#### SYNOPSIS
```c
string save_value(mixed value);
```

#### DESCRIPTION
Converts any NLPC value (integers, strings, arrays, mappings, even nested structures) into a string representation that can be stored in a file or database. This is your persistence layer—use it to save player data, game state, or any other information that needs to survive server restarts.

The format is driver-specific but human-readable-ish. You can save complex nested structures: arrays of mappings, mappings of arrays, whatever you need. Objects can't be serialized directly (they're runtime references), but you can save their paths with `otoa()` and reconstruct them later.

**Returns**: A string containing the serialized representation of the value.

#### EXAMPLE
```c
// Save player stats
mapping stats = ([ "hp": 100, "mp": 50, "level": 5 ]);
string data = save_value(stats);
fwrite("/data/players/gandalf.o", data);

// Save a complex structure
mixed *inventory = ({ "sword", "shield", "potion" });
mapping player = ([
    "name": "Frodo",
    "stats": stats,
    "items": inventory
]);
string saved = save_value(player);
```

#### SEE ALSO
restore_value(3), fwrite(3), fread(3)

---

### restore_value

#### NAME
restore_value()  -  deserialize a string back into an NLPC value

#### SYNOPSIS
```c
mixed restore_value(string str);
```

#### DESCRIPTION
Takes a string created by `save_value()` and reconstructs the original NLPC value. This is how you load saved game data—read the file, pass it to restore_value(), and you get your arrays, mappings, and data structures back exactly as they were.

If the string is malformed or wasn't created by save_value(), you'll get 0 or unpredictable results. Always check your return value!

**Returns**: The deserialized value (could be any type), or 0 if the string is invalid.

#### EXAMPLE
```c
// Load player stats
string data = fread("/data/players/gandalf.o", 0);
mapping stats = restore_value(data);
if (stats) {
    int hp = stats["hp"];
    write("HP: " + itoa(hp) + "\n");
}

// Load and validate
string saved = fread("/data/world/state.o", 0);
mixed state = restore_value(saved);
if (typeof(state) == T_MAPPING) {
    // Process the restored mapping
} else {
    write("Error: corrupted save file!\n");
}
```

#### SEE ALSO
save_value(3), fread(3), typeof(3)

---

### get_hostname

#### NAME
get_hostname()  -  perform reverse DNS lookup (IP to hostname)

#### SYNOPSIS
```c
string get_hostname(string ipaddr);
```

#### DESCRIPTION
Performs a reverse DNS lookup to convert an IP address into a hostname. This is useful for logging, displaying where players are connecting from, or implementing IP-based access controls with friendly names.

The lookup may take time (it's a network operation), so don't call this in performance-critical code. The driver may cache results.

**Returns**: The hostname as a string, or 0 if the lookup fails or the IP has no reverse DNS entry.

#### EXAMPLE
```c
string ip = "8.8.8.8";
string host = get_hostname(ip);
if (host) {
    write("Resolved to: " + host + "\n");
    // Might print: "Resolved to: dns.google"
} else {
    write("No reverse DNS for " + ip + "\n");
}
```

#### SEE ALSO
get_address(3), connected(3)

---

### get_address

#### NAME
get_address()  -  perform forward DNS lookup (hostname to IP)

#### SYNOPSIS
```c
string get_address(string hostname);
```

#### DESCRIPTION
Performs a forward DNS lookup to convert a hostname into an IP address. Need to connect to a remote server? Want to resolve a player's hostname to check against an IP ban list? This is your function.

Like get_hostname(), this is a network operation that may block briefly. Results may be cached by the driver.

**Returns**: The IP address as a string, or 0 if the lookup fails or the hostname doesn't exist.

#### EXAMPLE
```c
string host = "google.com";
string ip = get_address(host);
if (ip) {
    write(host + " is at " + ip + "\n");
    // Might print: "google.com is at 142.250.80.46"
} else {
    write("Could not resolve " + host + "\n");
}

// Check if a hostname matches a banned IP
string player_host = "evil.hacker.com";
string resolved = get_address(player_host);
if (resolved == "1.2.3.4") {
    write("Banned IP detected!\n");
}
```

#### SEE ALSO
get_hostname(3), connect_device(3)

---

### get_devconn

#### NAME
get_devconn()  -  get low-level connection information

#### SYNOPSIS
```c
mixed get_devconn(object obj);
```

#### DESCRIPTION
Returns driver-specific information about an object's network connection. This is a low-level diagnostic function that gives you access to internal connection details—useful for debugging network issues, implementing connection monitoring, or building admin tools.

The exact format of the returned data is driver-defined and may include file descriptors, socket information, or other connection metadata. If the object isn't connected (no active network device), you get 0.

**Returns**: Driver-specific connection information (typically a mapping or integer), or 0 if not connected.

#### EXAMPLE
```c
// Check connection details for a player
object player = this_player();
mixed conn_info = get_devconn(player);
if (conn_info) {
    write("Connection info: " + save_value(conn_info) + "\n");
} else {
    write("Player not connected\n");
}
```

#### SEE ALSO
connected(3), interactive(3), get_devport(3)

---

### send_device

#### NAME
send_device()  -  send raw data to the current connection

#### SYNOPSIS
```c
void send_device(string msg);
```

#### DESCRIPTION
Sends a raw string directly to the network connection associated with the current object. This bypasses all normal output processing—no formatting, no wrapping, no filtering. The bytes you send are the bytes that go out.

This is useful for implementing custom protocols, sending binary data, or when you need precise control over what goes over the wire. For normal player messages, use `write()` instead.

If the current object doesn't have an active connection, this does nothing.

**Returns**: Nothing (void).

#### EXAMPLE
```c
// Send a telnet control sequence
void send_telnet_command() {
    send_device(chr(255) + chr(251) + chr(1));  // IAC WILL ECHO
}

// Send raw data without processing
void send_raw(string data) {
    send_device(data);
}
```

#### SEE ALSO
write(3), syswrite(3), flush_device(3)

---

### connect_device

#### NAME
connect_device()  -  initiate an outbound network connection

#### SYNOPSIS
```c
int connect_device(string address, int port);
```

#### DESCRIPTION
Opens a network connection to a remote host. This is how you implement intermud communication, connect to external services, or build network clients within your MUD.

The connection is asynchronous—this function returns immediately, and the actual connection happens in the background. You'll typically get a callback when the connection succeeds or fails.

**Returns**: 0 on success (connection initiated), non-zero on error (invalid address, port out of range, etc.).

#### EXAMPLE
```c
// Connect to a remote MUD
void connect_to_remote_mud() {
    if (connect_device("mud.example.com", 4000) == 0) {
        write("Connecting to remote MUD...\n");
    } else {
        write("Connection failed\n");
    }
}
```

#### SEE ALSO
disconnect_device(3), get_address(3), connected(3)

---

### reconnect_device

#### NAME
reconnect_device()  -  transfer a connection to another object

#### SYNOPSIS
```c
int reconnect_device(object obj);
```

#### DESCRIPTION
Transfers the current object's network connection to another object. This is useful for implementing login systems where you start with a connection on a login object, then transfer it to a player object after authentication.

After this call, `obj` becomes the interactive object with the connection, and the current object loses it.

**Returns**: 0 on success, non-zero on error (no connection to transfer, invalid target object, etc.).

#### EXAMPLE
```c
// In a login object, after successful authentication
void complete_login(object player_ob) {
    if (reconnect_device(player_ob) == 0) {
        write("Welcome to the game!\n");
        destruct(this_object());  // Clean up login object
    }
}
```

#### SEE ALSO
connect_device(3), set_interactive(3)

---

### disconnect_device

#### NAME
disconnect_device()  -  close the current network connection

#### SYNOPSIS
```c
void disconnect_device();
```

#### DESCRIPTION
Closes the network connection associated with the current object. The connection is terminated immediately, and any pending output is discarded (use `flush_device()` first if you want to ensure buffered data is sent).

This is typically called when a player quits, times out, or is forcibly disconnected by an admin.

**Returns**: Nothing (void).

#### EXAMPLE
```c
// Kick a player
void kick_player(string reason) {
    write("You have been disconnected: " + reason + "\n");
    flush_device();  // Make sure they see the message
    disconnect_device();
}

// Timeout handler
void idle_timeout() {
    write("Connection closed due to inactivity.\n");
    disconnect_device();
}
```

#### SEE ALSO
connect_device(3), flush_device(3), connected(3)

---

### flush_device

#### NAME
flush_device()  -  force pending output to be sent immediately

#### SYNOPSIS
```c
void flush_device();
```

#### DESCRIPTION
Forces any buffered output to be sent to the network connection immediately. Normally, output is buffered and sent in batches for efficiency. This function bypasses that buffering.

Use this when you need to ensure a message is delivered right away—for example, before disconnecting a player, or when sending time-critical information.

**Returns**: Nothing (void).

#### EXAMPLE
```c
// Ensure goodbye message is sent before disconnect
void quit_game() {
    write("Goodbye! Thanks for playing.\n");
    flush_device();  // Make sure they see it
    disconnect_device();
}

// Send urgent message
void send_urgent(string msg) {
    write(msg);
    flush_device();  // Don't wait for buffer to fill
}
```

#### SEE ALSO
send_device(3), disconnect_device(3)

---

### cat

#### NAME
cat()  -  display file contents to the player

#### SYNOPSIS
```c
int cat(string path);
```

#### DESCRIPTION
Reads a file and displays its contents to the current player (or current object if no player). Each line is sent through the `listen()` function, so it respects the normal message delivery system.

This is the MUD equivalent of the Unix `cat` command—perfect for implementing "help" commands, displaying message files, or letting wizards view source code.

**Returns**: 0 on success, 1 on error (file doesn't exist, permission denied, etc.).

#### EXAMPLE
```c
// Show a help file
int do_help(string topic) {
    string path = "/doc/help/" + topic;
    if (cat(path) != 0) {
        write("No help available for '" + topic + "'\n");
        return 0;
    }
    return 1;
}

// Display MOTD on login
void show_motd() {
    cat("/etc/motd");
}
```

#### SEE ALSO
fread(3), ls(3), edit(3)

---

### ls

#### NAME
ls()  -  list directory contents

#### SYNOPSIS
```c
int ls(string path);
```

#### DESCRIPTION
Lists the files and subdirectories in the specified path, displaying them to the current player. The output format is driver-specific but typically includes file names, sizes, and modification times.

This is essential for wizards browsing the file system, implementing file browsers, or just exploring what's available.

**Returns**: 0 on success, non-zero on error (directory doesn't exist, permission denied, etc.).

#### EXAMPLE
```c
// List current directory
void do_ls(string dir) {
    if (!dir || dir == "") {
        dir = ".";
    }
    if (ls(dir) != 0) {
        write("Cannot list directory: " + dir + "\n");
    }
}

// Browse help files
void browse_help() {
    write("Available help topics:\n");
    ls("/doc/help");
}
```

#### SEE ALSO
cat(3), fstat(3), mkdir(3)

---

### rm

#### NAME
rm()  -  remove a file

#### SYNOPSIS
```c
int rm(string path);
```

#### DESCRIPTION
Deletes a file from the filesystem. The file is permanently removed—there's no trash bin or undo. Use with caution!

This is typically restricted to privileged objects for security reasons. You don't want players deleting important game files.

**Returns**: 0 on success, 1 on error (file doesn't exist, permission denied, file is protected, etc.).

#### EXAMPLE
```c
// Clean up temporary file
void cleanup_temp() {
    if (rm("/tmp/session_data.tmp") == 0) {
        write("Temporary file removed\n");
    }
}

// Delete old player file
void delete_player(string name) {
    string path = "/data/players/" + name + ".o";
    if (rm(path) == 0) {
        write("Player " + name + " deleted\n");
    } else {
        write("Could not delete player file\n");
    }
}
```

#### SEE ALSO
ferase(3), fwrite(3), ls(3), remove(3)

---

### get_dir

#### NAME
get_dir()  -  get directory listing as array (LPC standard)

#### SYNOPSIS
```c
string *get_dir(string path);
```

#### DESCRIPTION
Returns an array of filenames in the specified directory. This is the standard LPC function for programmatic directory traversal, compatible with MudOS, DGD, and LDMud drivers.

Unlike `ls()` which uses callbacks for interactive display, `get_dir()` returns the filenames directly as an array, making it suitable for daemons, command discovery systems, file browsers, and any code that needs to process directory contents programmatically.

The function performs permission checks via the master object's `valid_read()` callback and respects the filesystem security model.

**Returns**: Array of filename strings, or 0 on error (directory doesn't exist, permission denied, not a directory, etc.).

**Alias**: `ls()` is deprecated and maps to `get_dir()` for compatibility.

#### EXAMPLE
```c
// List all command files
void discover_commands() {
    string *files = get_dir("/cmds/player");
    if (!files) {
        write("Cannot access command directory\n");
        return;
    }
    
    foreach (string file in files) {
        int len = strlen(file);
        if (len > 2 && leftstr(file, 1) == "_" && rightstr(file, 2) == ".c") {
            // Found a command file
            process_command(file);
        }
    }
}

// Build help index
void index_help_files() {
    string *topics = get_dir("/doc/help");
    mapping index = ([]);
    
    foreach (string topic in topics) {
        int len = strlen(topic);
        if (len > 4 && rightstr(topic, 4) == ".txt") {
            string name = leftstr(topic, len - 4);  // Strip .txt
            index[name] = "/doc/help/" + topic;
        }
    }
    return index;
}

// Recursive directory walker
void walk_directory(string path, function callback) {
    string *entries = get_dir(path);
    if (!entries) return;
    
    foreach (string entry in entries) {
        string full_path = path + "/" + entry;
        callback(full_path);
        
        // Recurse into subdirectories
        if (fstat(full_path) & DIRECTORY) {
            walk_directory(full_path, callback);
        }
    }
}
```

#### SEE ALSO
ls(3), fstat(3), file_size(3)

---

### read_file

#### NAME
read_file()  -  read file contents as string (LPC standard)

#### SYNOPSIS
```c
string read_file(string path);
string read_file(string path, int start_line);
string read_file(string path, int start_line, int num_lines);
```

#### DESCRIPTION
Reads the contents of a file and returns it as a string. This is the standard LPC function for file reading, compatible with MudOS, DGD, and LDMud drivers.

With one argument, reads the entire file. With two arguments, reads from `start_line` to end of file. With three arguments, reads `num_lines` starting from `start_line`. Line numbers are 1-indexed.

This function is simpler than `fread()` which requires position tracking through lvalue references. Use `read_file()` when you want to read file content directly without managing file positions.

**Returns**: String containing the file contents (with newlines preserved), or 0 on error (file doesn't exist, permission denied, etc.).

**Alias**: `fread()` is deprecated and maps to `read_file()` for compatibility.

#### EXAMPLE
```c
// Read entire file
void display_help(string topic) {
    string content = read_file("/doc/help/" + topic + ".txt");
    if (content) {
        write(content);
    } else {
        write("Help topic not found\n");
    }
}

// Read specific lines
void show_function(string file, int line, int count) {
    string code = read_file(file, line, count);
    if (code) {
        write("Lines " + line + "-" + (line + count - 1) + ":\n");
        write(code);
    }
}

// Load configuration
mapping load_config() {
    string data = read_file("/etc/config.txt");
    if (!data) return ([]);
    
    mapping config = ([]);
    string *lines = explode(data, "\n");
    foreach (string line in lines) {
        if (strlen(line) > 0 && leftstr(line, 1) == "#") continue;  // Skip comments
        string *parts = explode(line, "=");
        if (sizeof(parts) == 2) {
            config[parts[0]] = parts[1];
        }
    }
    return config;
}
```

#### SEE ALSO
fread(3), write_file(3), file_size(3)

---

### write_file

#### NAME
write_file()  -  append string to file (LPC standard)

#### SYNOPSIS
```c
int write_file(string path, string content);
```

#### DESCRIPTION
Appends the given content string to the specified file. If the file doesn't exist, it is created. This is the standard LPC function for file writing, compatible with MudOS, DGD, and LDMud drivers.

The content is appended to the end of the file, preserving existing contents. To overwrite a file, use `ferase()` first, or `remove()` to delete it completely.

This function is simpler than `fwrite()` as it handles file operations in a single call without requiring separate open/close operations.

**Returns**: 1 on success, 0 on error (permission denied, disk full, path invalid, etc.).

**Alias**: `fwrite()` is deprecated and maps to `write_file()` for compatibility.

#### EXAMPLE
```c
// Log a message
void log_event(string message) {
    string timestamp = ctime(time());
    string entry = timestamp + ": " + message + "\n";
    
    if (!write_file("/log/events.log", entry)) {
        write("Failed to write to log file\n");
    }
}

// Save player data
void save_player(string name, string data) {
    string path = "/data/players/" + name + ".o";
    
    // Clear old file first
    ferase(path);
    
    // Write new data
    if (write_file(path, data)) {
        write("Player data saved\n");
    } else {
        write("Error saving player data\n");
    }
}

// Append to high scores
void add_high_score(string name, int score) {
    string entry = sprintf("%-20s %10d\n", name, score);
    write_file("/data/highscores.txt", entry);
}
```

#### SEE ALSO
fwrite(3), read_file(3), ferase(3), remove(3)

---

### remove

#### NAME
remove()  -  delete a file (LPC standard)

#### SYNOPSIS
```c
int remove(string path);
```

#### DESCRIPTION
Permanently deletes the specified file from the filesystem. This is the standard LPC function for file deletion, compatible with MudOS, DGD, and LDMud drivers.

The file is permanently removed with no recovery option. This function is typically restricted to privileged objects for security reasons.

**Returns**: 0 on success, 1 on error (file doesn't exist, permission denied, file is protected, etc.).

**Alias**: `rm()` is deprecated and maps to `remove()` for compatibility.

#### EXAMPLE
```c
// Clean up temporary files
void cleanup_temp_files() {
    string *files = get_dir("/tmp");
    foreach (string file in files) {
        if (remove("/tmp/" + file) == 0) {
            write("Removed: " + file + "\n");
        }
    }
}

// Delete old player file
void delete_character(string name) {
    string path = "/data/players/" + name + ".o";
    if (remove(path) == 0) {
        write("Character " + name + " deleted\n");
        log_event("Deleted character: " + name);
    } else {
        write("Failed to delete character\n");
    }
}
```

#### SEE ALSO
rm(3), ferase(3), write_file(3), rename(3)

---

### rename

#### NAME
rename()  -  rename or move a file (LPC standard)

#### SYNOPSIS
```c
int rename(string old_path, string new_path);
```

#### DESCRIPTION
Renames or moves a file from `old_path` to `new_path`. This is the standard LPC function for file renaming/moving, compatible with MudOS, DGD, and LDMud drivers.

Can be used to rename a file in the same directory or move it to a different directory. If `new_path` already exists, the behavior is system-dependent (may fail or overwrite).

**Returns**: 0 on success, 1 on error (source doesn't exist, permission denied, destination already exists, etc.).

**Alias**: `mv()` is deprecated and maps to `rename()` for compatibility.

#### EXAMPLE
```c
// Rename a file
void rename_player(string old_name, string new_name) {
    string old_path = "/data/players/" + old_name + ".o";
    string new_path = "/data/players/" + new_name + ".o";
    
    if (rename(old_path, new_path) == 0) {
        write("Player renamed from " + old_name + " to " + new_name + "\n");
    } else {
        write("Rename failed\n");
    }
}

// Move file to backup
void backup_file(string path) {
    string backup_path = path + ".bak";
    if (rename(path, backup_path) == 0) {
        write("File backed up to " + backup_path + "\n");
    }
}

// Archive old logs
void archive_log(string log_name) {
    string old_path = "/log/" + log_name + ".log";
    string new_path = "/log/archive/" + log_name + "_" + ctime(time()) + ".log";
    rename(old_path, new_path);
}
```

#### SEE ALSO
mv(3), remove(3), write_file(3)

---

### file_size

#### NAME
file_size()  -  get file size in bytes (LPC standard)

#### SYNOPSIS
```c
int file_size(string path);
```

#### DESCRIPTION
Returns the size of the specified file in bytes. This is the standard LPC function for querying file size, compatible with MudOS, DGD, and LDMud drivers.

Unlike `fstat()` which returns filesystem flags, `file_size()` returns the actual byte size of the file content, obtained via system `stat()` call.

**Returns**: File size in bytes, or -1 on error (file doesn't exist, permission denied, etc.).

#### EXAMPLE
```c
// Check file size before loading
void safe_load_file(string path) {
    int size = file_size(path);
    if (size < 0) {
        write("File not found\n");
        return;
    }
    if (size > 1000000) {
        write("File too large: " + size + " bytes\n");
        return;
    }
    
    string content = read_file(path);
    process_content(content);
}

// Display file information
void file_info(string path) {
    int size = file_size(path);
    if (size < 0) {
        write("File does not exist\n");
        return;
    }
    
    write("File: " + path + "\n");
    write("Size: " + size + " bytes\n");
    
    if (size < 1024) {
        write("     (" + size + " bytes)\n");
    } else if (size < 1048576) {
        write("     (" + (size / 1024) + " KB)\n");
    } else {
        write("     (" + (size / 1048576) + " MB)\n");
    }
}

// Check disk usage
int calculate_directory_size(string path) {
    int total = 0;
    string *files = get_dir(path);
    
    foreach (string file in files) {
        int size = file_size(path + "/" + file);
        if (size > 0) total += size;
    }
    
    return total;
}
```

#### SEE ALSO
fstat(3), read_file(3), get_dir(3)

---

### ferase

#### NAME
ferase()  -  truncate a file to zero length

#### SYNOPSIS
```c
int ferase(string path);
```

#### DESCRIPTION
Truncates a file to zero length, effectively erasing its contents. If the file doesn't exist, it creates an empty file. Unlike `rm()`, the file itself remains—it's just empty.

This is useful when you want to clear a log file, reset a data file, or ensure a file exists but is empty.

**Returns**: 0 on success, 1 on error (permission denied, path is a directory, etc.).

#### EXAMPLE
```c
// Clear a log file
void clear_log() {
    if (ferase("/logs/debug.log") == 0) {
        write("Log file cleared\n");
    }
}

// Reset player data
void reset_player_stats(string name) {
    string path = "/data/players/" + name + "/stats.dat";
    ferase(path);  // Clear old stats
    // Now write new default stats
    fwrite(path, save_value(([ "hp": 100, "level": 1 ])));
}
```

#### SEE ALSO
rm(3), fwrite(3), fread(3)

---

### cp

#### NAME
cp()  -  copy a file

#### SYNOPSIS
```c
int cp(string src, string dst);
```

#### DESCRIPTION
Copies a file from `src` to `dst`. The source file remains unchanged, and a new file is created at the destination with identical contents.

If the destination already exists, it's overwritten. If the destination path includes directories that don't exist, the operation fails—create the directories first with `mkdir()`.

**Returns**: 0 on success, 1 on error (source doesn't exist, permission denied, destination path invalid, etc.).

#### EXAMPLE
```c
// Backup a file
void backup_file(string path) {
    string backup = path + ".bak";
    if (cp(path, backup) == 0) {
        write("Backup created: " + backup + "\n");
    } else {
        write("Backup failed\n");
    }
}

// Copy template to new file
void create_from_template(string name) {
    string template = "/templates/room.c";
    string newfile = "/world/rooms/" + name + ".c";
    if (cp(template, newfile) == 0) {
        write("Created " + newfile + " from template\n");
    }
}
```

#### SEE ALSO
mv(3), rm(3), fwrite(3)

---

### mv

#### NAME
mv()  -  move or rename a file

#### SYNOPSIS
```c
int mv(string src, string dst);
```

#### DESCRIPTION
Moves a file from `src` to `dst`, or renames it if both paths are in the same directory. The source file is removed after the move—this is different from `cp()` which leaves the original.

If the destination exists, it's overwritten. This is an atomic operation when renaming within the same directory, but may not be atomic across different filesystems.

**Returns**: 0 on success, 1 on error (source doesn't exist, permission denied, cross-filesystem move failed, etc.).

#### EXAMPLE
```c
// Rename a file
void rename_file(string old_name, string new_name) {
    if (mv(old_name, new_name) == 0) {
        write("Renamed to " + new_name + "\n");
    } else {
        write("Rename failed\n");
    }
}

// Move file to archive
void archive_log() {
    string current = "/logs/current.log";
    string archive = "/logs/archive/" + itoa(time()) + ".log";
    if (mv(current, archive) == 0) {
        write("Log archived\n");
    }
}
```

#### SEE ALSO
cp(3), rm(3), mkdir(3)

---

### mkdir

#### NAME
mkdir()  -  create a directory

#### SYNOPSIS
```c
int mkdir(string path);
```

#### DESCRIPTION
Creates a new directory at the specified path. Parent directories must already exist—this doesn't create intermediate directories like `mkdir -p` in Unix.

This is essential for organizing your MUD's file structure, creating player directories, setting up log hierarchies, or any time you need to organize files.

**Returns**: 0 on success, 1 on error (parent directory doesn't exist, permission denied, directory already exists, etc.).

#### EXAMPLE
```c
// Create a player directory
void create_player_dir(string name) {
    string dir = "/data/players/" + name;
    if (mkdir(dir) == 0) {
        write("Created player directory: " + dir + "\n");
    } else {
        write("Could not create directory\n");
    }
}

// Set up log directory structure
void setup_logs() {
    mkdir("/logs");
    mkdir("/logs/archive");
    mkdir("/logs/errors");
}
```

#### SEE ALSO
rmdir(3), ls(3), cp(3)

---

### rmdir

#### NAME
rmdir()  -  remove an empty directory

#### SYNOPSIS
```c
int rmdir(string path);
```

#### DESCRIPTION
Removes a directory, but only if it's empty. If the directory contains any files or subdirectories, the operation fails. This is a safety feature—you have to explicitly delete the contents first.

To remove a directory tree, you need to recursively delete all files and subdirectories first, then remove the directory itself.

**Returns**: 0 on success, 1 on error (directory not empty, doesn't exist, permission denied, etc.).

#### EXAMPLE
```c
// Remove empty directory
void cleanup_empty_dir(string path) {
    if (rmdir(path) == 0) {
        write("Removed directory: " + path + "\n");
    } else {
        write("Directory not empty or doesn't exist\n");
    }
}

// Clean up old player directory (after deleting files)
void remove_player_dir(string name) {
    string dir = "/data/players/" + name;
    // First delete all files in the directory
    // ... (file deletion code)
    // Then remove the directory
    rmdir(dir);
}
```

#### SEE ALSO
mkdir(3), rm(3), ls(3)

---

### fread

#### NAME
fread()  -  read lines from a file with position tracking

#### SYNOPSIS
```c
mixed fread(string path, int &pos[, int lines]);
```

#### DESCRIPTION
Reads one or more lines from a file, starting at the given position. This is your file reading workhorse—use it to load configuration files, read player data, parse logs, or implement paged file viewing.

The `pos` parameter is passed by reference (as an lvalue with `&`), and the function automatically updates it to point to the next unread line. This makes it perfect for sequential reading in a loop.

If you provide the optional `lines` parameter, it reads that many lines and returns them as an array. Otherwise, it reads a single line and returns it as a string.

**Returns**: 
- Single line mode: The line as a string, 0 at EOF, or -1 on error
- Multi-line mode: An array of strings, empty array at EOF, or -1 on error

#### EXAMPLE
```c
// Read file line by line
void read_config(string path) {
    int pos = 0;
    string line;
    
    while ((line = fread(path, &pos)) && line != 0) {
        if (line == "") continue;  // Skip blank lines
        write("Config: " + line + "\n");
    }
}

// Read multiple lines at once
void show_file_page(string path, int start) {
    int pos = start;
    mixed *lines = fread(path, &pos, 20);  // Read 20 lines
    
    if (typeof(lines) == T_ARRAY) {
        foreach (string line in lines) {
            write(line + "\n");
        }
        write("\nNext page starts at line " + itoa(pos) + "\n");
    }
}

// Load saved data
mapping load_player_data(string name) {
    string path = "/data/players/" + name + ".o";
    int pos = 0;
    string data = fread(path, &pos);
    
    if (data && data != 0) {
        return restore_value(data);
    }
    return 0;
}
```

#### SEE ALSO
fwrite(3), cat(3), restore_value(3)

---

### fwrite

#### NAME
fwrite()  -  write or append data to a file

#### SYNOPSIS
```c
int fwrite(string path, string data);
```

#### DESCRIPTION
Appends data to a file. If the file doesn't exist, it's created. If it does exist, the new data is added to the end. This is your file writing workhorse—use it to save player data, write logs, store configuration, or persist any information that needs to survive server restarts.

Unlike `ferase()` which clears the file first, `fwrite()` always appends. If you want to replace a file's contents, call `ferase()` first, then `fwrite()`.

The data is written as-is, with no automatic newlines. If you want line-oriented output, add `"\n"` to your strings.

**Returns**: 0 on success, 1 on error (permission denied, disk full, invalid path, etc.).

#### EXAMPLE
```c
// Save player data
void save_player(string name, mapping data) {
    string path = "/data/players/" + name + ".o";
    string serialized = save_value(data);
    
    ferase(path);  // Clear old data
    if (fwrite(path, serialized) == 0) {
        write("Player saved\n");
    } else {
        write("Save failed!\n");
    }
}

// Append to log file
void log_event(string event) {
    string timestamp = ctime(time());
    string entry = sprintf("[%s] %s\n", timestamp, event);
    fwrite("/logs/events.log", entry);
}

// Write multiple lines
void write_config(string path, mixed *lines) {
    ferase(path);  // Start fresh
    foreach (string line in lines) {
        fwrite(path, line + "\n");
    }
}
```

#### SEE ALSO
fread(3), ferase(3), save_value(3)

---

### fstat

#### NAME
fstat()  -  get file status and permissions

#### SYNOPSIS
```c
int fstat(string path);
```

#### DESCRIPTION
Returns the status and permission bits for a file. The return value is a bitmask containing file flags and permissions. If the file doesn't exist, returns -1.

This is useful for checking file permissions before attempting operations, implementing access controls, or displaying file information to users.

The exact bit meanings are driver-specific, but typically include read/write permissions, hidden status, and other file attributes.

**Returns**: Permission/flag bitmask as an integer, or -1 if the file doesn't exist.

#### EXAMPLE
```c
// Check if file exists
int file_exists(string path) {
    return fstat(path) != -1;
}

// Check file permissions
void check_file(string path) {
    int stat = fstat(path);
    if (stat == -1) {
        write("File not found\n");
        return;
    }
    write(sprintf("File permissions: 0x%x\n", stat));
}

// Verify before reading
void safe_read(string path) {
    if (fstat(path) == -1) {
        write("Cannot read: file doesn't exist\n");
        return;
    }
    cat(path);
}
```

#### SEE ALSO
fowner(3), chmod(3), ls(3)

---

### fowner

#### NAME
fowner()  -  get the owner of a file

#### SYNOPSIS
```c
object fowner(string path);
```

#### DESCRIPTION
Returns the object that owns a file. File ownership is used for access control—typically, only the owner (or privileged objects) can modify or delete a file.

If the file doesn't exist or has no owner, returns 0.

**Returns**: The owner object, or 0 if the file doesn't exist or has no owner.

#### EXAMPLE
```c
// Check file ownership
void show_owner(string path) {
    object owner = fowner(path);
    if (owner) {
        write("File owned by: " + otoa(owner) + "\n");
    } else {
        write("File has no owner or doesn't exist\n");
    }
}

// Verify ownership before deletion
void delete_if_owned(string path) {
    object owner = fowner(path);
    if (owner == this_object()) {
        rm(path);
        write("File deleted\n");
    } else {
        write("You don't own this file\n");
    }
}

// List files with owners
void list_with_owners(string dir) {
    // Iterate through directory
    // For each file:
    object owner = fowner(file_path);
    write(sprintf("%s (owner: %s)\n", file_path, otoa(owner)));
}
```

#### SEE ALSO
fstat(3), chown(3), priv(3)

---

### chmod

#### NAME
chmod()  -  change file permissions

#### SYNOPSIS
```c
int chmod(string path, int mode);
```

#### DESCRIPTION
Changes the permission bits on a file. The `mode` parameter is a bitmask that sets read/write permissions and other file attributes.

This is typically restricted to privileged objects or the file's owner. Use it to implement access controls, protect sensitive files, or manage file visibility.

**Returns**: 0 on success, 1 on error (permission denied, file doesn't exist, etc.).

#### EXAMPLE
```c
// Set file permissions
void protect_file(string path) {
    int mode = 0644;  // Read/write for owner, read for others
    if (chmod(path, mode) == 0) {
        write("Permissions updated\n");
    } else {
        write("Permission change failed\n");
    }
}

// Make file read-only
void make_readonly(string path) {
    chmod(path, 0444);  // Read-only for everyone
}

// Restrict access
void restrict_file(string path) {
    chmod(path, 0600);  // Owner only
}
```

#### SEE ALSO
chown(3), fstat(3), priv(3)

---

### chown

#### NAME
chown()  -  change file ownership

#### SYNOPSIS
```c
int chown(string path, object owner);
```

#### DESCRIPTION
Changes the owner of a file. File ownership determines who can modify, delete, or change permissions on the file.

This is typically restricted to privileged objects. You can't just steal ownership of someone else's files—you need the PRIV flag to use this function.

**Returns**: 0 on success, 1 on error (permission denied, file doesn't exist, invalid owner, etc.).

#### EXAMPLE
```c
// Transfer file ownership
void transfer_file(string path, object new_owner) {
    if (!priv(this_object())) {
        write("Insufficient privileges\n");
        return;
    }
    
    if (chown(path, new_owner) == 0) {
        write("Ownership transferred\n");
    } else {
        write("Transfer failed\n");
    }
}

// Claim ownership of new file
void create_owned_file(string path) {
    fwrite(path, "Initial content\n");
    chown(path, this_object());
}
```

#### SEE ALSO
chmod(3), fowner(3), set_priv(3)

---

### hide

#### NAME
hide()  -  hide a file from directory listings

#### SYNOPSIS
```c
int hide(string path);
```

#### DESCRIPTION
Marks a file as hidden, removing it from normal directory listings. The file still exists and can be accessed if you know its path, but it won't show up in `ls()` output.

This requires the PRIV flag. Use it to hide system files, protect sensitive data, or implement secret areas that players shouldn't easily discover.

**Returns**: 0 on success, 1 on error (permission denied, file doesn't exist, etc.).

#### EXAMPLE
```c
// Hide a system file
void hide_system_file(string path) {
    if (!priv(this_object())) {
        write("Insufficient privileges\n");
        return;
    }
    
    if (hide(path) == 0) {
        write("File hidden\n");
    } else {
        write("Hide failed\n");
    }
}

// Hide player data from casual browsing
void protect_player_data(string name) {
    string path = "/data/players/" + name + ".o";
    hide(path);
}
```

#### SEE ALSO
unhide(3), fstat(3), priv(3)

---

### unhide

#### NAME
unhide()  -  restore a hidden file to visibility

#### SYNOPSIS
```c
int unhide(string path, object owner, int flags);
```

#### DESCRIPTION
Restores a hidden file to normal visibility, making it appear in directory listings again. You must specify the owner and permission flags when unhiding.

This requires the PRIV flag. Use it to restore files that were previously hidden, or to correct accidental hiding.

**Returns**: 0 on success, 1 on error (permission denied, file doesn't exist, etc.).

#### EXAMPLE
```c
// Unhide a file
void unhide_file(string path, object new_owner) {
    if (!priv(this_object())) {
        write("Insufficient privileges\n");
        return;
    }
    
    int flags = 0644;  // Standard permissions
    if (unhide(path, new_owner, flags) == 0) {
        write("File restored to visibility\n");
    } else {
        write("Unhide failed\n");
    }
}

// Restore system file
void restore_system_file(string path) {
    object master = atoo("/boot");
    unhide(path, master, 0644);
}
```

#### SEE ALSO
hide(3), chown(3), chmod(3)

---

### edit

#### NAME
edit()  -  open the line editor on a file

#### SYNOPSIS
```c
int edit(string path);
```

#### DESCRIPTION
Opens the built-in line editor for editing a file. The player enters editor mode, where their input goes to the editor instead of normal command processing. This is how wizards edit code, players write mail, or anyone composes multi-line text.

The editor provides commands for inserting, deleting, and modifying lines, searching and replacing text, and saving or aborting changes. When the player exits the editor, normal command processing resumes.

**Returns**: 0 on success, 1 on error (file can't be opened, player already in editor, etc.).

#### EXAMPLE
```c
// Edit a file
void do_edit(string filename) {
    if (edit(filename) == 0) {
        write("Entering editor...\n");
    } else {
        write("Could not open editor\n");
    }
}

// Edit wizard's home file
void edit_home() {
    string path = "/wiz/" + query_name() + "/workroom.c";
    edit(path);
}
```

#### SEE ALSO
in_editor(3), cat(3), fwrite(3)

---

### typeof

#### NAME
typeof()  -  get the runtime type of a value

#### SYNOPSIS
```c
int typeof(mixed value);
```

#### DESCRIPTION
Returns a numeric type code identifying what kind of value you have. This is essential for writing generic functions that handle different types, validating input, or implementing type-specific behavior.

The type codes are defined as constants:
- `T_INTEGER` - Integer
- `T_STRING` - String  
- `T_OBJECT` - Object reference
- `T_ARRAY` - Array
- `T_MAPPING` - Mapping

And potentially others for internal types.

**Returns**: An integer type code.

#### EXAMPLE
```c
// Handle different types
void process_value(mixed value) {
    if (typeof(value) == T_STRING) {
        write("String: " + value + "\n");
    } else if (typeof(value) == T_INTEGER) {
        write("Number: " + itoa(value) + "\n");
    } else if (typeof(value) == T_ARRAY) {
        write("Array with " + itoa(sizeof(value)) + " elements\n");
    } else if (typeof(value) == T_MAPPING) {
        write("Mapping with " + itoa(sizeof(value)) + " keys\n");
    } else if (typeof(value) == T_OBJECT) {
        write("Object: " + otoa(value) + "\n");
    }
}

// Validate function argument
void set_property(string key, mixed value) {
    if (typeof(key) != T_STRING) {
        write("Error: key must be a string\n");
        return;
    }
    // Store the property
}

// Type-safe array operations
mixed *filter_strings(mixed *arr) {
    mixed *result = ({ });
    foreach (mixed item in arr) {
        if (typeof(item) == T_STRING) {
            result += ({ item });
        }
    }
    return result;
}
```

#### SEE ALSO
sizeof(3), restore_value(3)

---

### strlen

#### NAME
strlen()  -  get the length of a string

#### SYNOPSIS
```c
int strlen(string s);
```

#### DESCRIPTION
Returns the number of characters in a string. This counts actual characters, not bytes (though in ASCII they're the same).

Use this to validate input length, truncate strings, or check if a string is empty.

**Returns**: The length of the string as an integer.

#### EXAMPLE
```c
// Validate password length
int check_password(string pass) {
    if (strlen(pass) < 8) {
        write("Password must be at least 8 characters\n");
        return 0;
    }
    return 1;
}

// Check for empty string
if (strlen(input) == 0) {
    write("Please enter something\n");
}

// Truncate long names
string truncate(string name, int max_len) {
    if (strlen(name) > max_len) {
        return leftstr(name, max_len) + "...";
    }
    return name;
}
```

#### SEE ALSO
leftstr(3), sizeof(3)

---

### leftstr

#### NAME
leftstr()  -  extract characters from the left side of a string

#### SYNOPSIS
```c
string leftstr(string s, int len);
```

#### DESCRIPTION
Returns the first `len` characters from a string. If `len` is greater than the string length, returns the entire string. If `len` is negative or zero, returns an empty string.

This is perfect for truncating strings, extracting prefixes, or implementing "show first N characters" displays.

**Returns**: A substring containing the leftmost characters.

#### EXAMPLE
```c
// Get first 10 characters
string preview = leftstr(description, 10);
// "This is a ..." becomes "This is a "

// Extract filename prefix
string name = "player_data.o";
string prefix = leftstr(name, 6);  // "player"

// Safe truncation
string truncate_left(string s, int max) {
    if (strlen(s) > max) {
        return leftstr(s, max) + "...";
    }
    return s;
}
```

#### SEE ALSO
rightstr(3), midstr(3), strlen(3)

---

### rightstr

#### NAME
rightstr()  -  extract characters from the right side of a string

#### SYNOPSIS
```c
string rightstr(string s, int len);
```

#### DESCRIPTION
Returns the last `len` characters from a string. If `len` is greater than the string length, returns the entire string. If `len` is negative or zero, returns an empty string.

Use this to extract file extensions, get suffixes, or show the end of long strings.

**Returns**: A substring containing the rightmost characters.

#### EXAMPLE
```c
// Get file extension
string filename = "player.c";
string ext = rightstr(filename, 2);  // ".c"

// Show last N characters
string msg = "This is a very long message";
string end = rightstr(msg, 10);  // "ng message"

// Extract suffix
string get_extension(string path) {
    int dot_pos = instr(path, 0, ".");
    if (dot_pos >= 0) {
        int ext_len = strlen(path) - dot_pos;
        return rightstr(path, ext_len);
    }
    return "";
}
```

#### SEE ALSO
leftstr(3), midstr(3), strlen(3)

---

### midstr

#### NAME
midstr()  -  extract a substring from the middle of a string

#### SYNOPSIS
```c
string midstr(string s, int pos, int len);
```

#### DESCRIPTION
Returns `len` characters starting at position `pos` (0-indexed). If the position is beyond the string length, returns an empty string. If the length extends past the end, returns characters up to the end.

This is your general-purpose substring extractor—use it to parse structured data, extract fields, or implement string slicing.

**Returns**: A substring starting at the specified position.

#### EXAMPLE
```c
// Extract middle portion
string text = "Hello World";
string middle = midstr(text, 6, 5);  // "World"

// Parse fixed-width fields
string record = "John    Doe     30";
string first = midstr(record, 0, 8);   // "John    "
string last = midstr(record, 8, 8);    // "Doe     "
string age = midstr(record, 16, 2);    // "30"

// Extract between markers
string extract_between(string s, int start, int end) {
    int len = end - start;
    return midstr(s, start, len);
}
```

#### SEE ALSO
leftstr(3), rightstr(3), instr(3)

---

### instr

#### NAME
instr()  -  find the position of a substring

#### SYNOPSIS
```c
int instr(string haystack, int start, string needle);
```

#### DESCRIPTION
Searches for the first occurrence of `needle` in `haystack`, starting at position `start` (0-indexed). Returns the position where the substring was found, or -1 if it's not present.

This is essential for parsing text, finding delimiters, implementing search functionality, or checking if a string contains a pattern.

**Returns**: The 0-based index of the first match, or -1 if not found.

#### EXAMPLE
```c
// Find a substring
string text = "Hello World";
int pos = instr(text, 0, "World");  // 6

// Check if string contains something
if (instr(text, 0, "Hello") >= 0) {
    write("Found 'Hello'\n");
}

// Find all occurrences
void find_all(string text, string search) {
    int pos = 0;
    while ((pos = instr(text, pos, search)) >= 0) {
        write("Found at position " + itoa(pos) + "\n");
        pos++;  // Move past this match
    }
}

// Extract between delimiters
string extract_quoted(string s) {
    int start = instr(s, 0, "\"");
    if (start < 0) return "";
    
    int end = instr(s, start + 1, "\"");
    if (end < 0) return "";
    
    return midstr(s, start + 1, end - start - 1);
}

// Case-insensitive search
int find_ignore_case(string text, string search) {
    return instr(downcase(text), 0, downcase(search));
}
```

#### SEE ALSO
midstr(3), explode(3), sscanf(3)

---

### subst

#### NAME
subst()  -  substitute a portion of a string

#### SYNOPSIS
```c
string subst(string s, int pos, int len, string replacement);
```

#### DESCRIPTION
Returns a new string where `len` characters starting at position `pos` are replaced with `replacement`. The original string is unchanged (strings are immutable).

This is useful for editing strings, implementing find-and-replace, correcting text, or building strings piece by piece.

**Returns**: A new string with the substitution applied.

#### EXAMPLE
```c
// Replace a portion
string text = "Hello World";
string result = subst(text, 6, 5, "Universe");
// result is "Hello Universe"

// Delete characters (replace with empty string)
string remove_middle(string s) {
    return subst(s, 5, 3, "");
}

// Insert text (replace 0 characters)
string insert_at(string s, int pos, string insert) {
    return subst(s, pos, 0, insert);
}

// Replace found substring
string replace_first(string text, string find, string replace) {
    int pos = instr(text, 0, find);
    if (pos >= 0) {
        return subst(text, pos, strlen(find), replace);
    }
    return text;
}

// Censor bad words
string censor(string text) {
    int pos = instr(text, 0, "badword");
    if (pos >= 0) {
        return subst(text, pos, 7, "*******");
    }
    return text;
}
```

#### SEE ALSO
instr(3), replace_string(3), midstr(3)

---

### sprintf

#### NAME
sprintf() - Format strings with C-style and LPC-specific format specifiers

#### SYNOPSIS
```c
string sprintf(string format, ...);
```

#### DESCRIPTION
The sprintf() efun provides powerful string formatting with both C-style and LPC-specific format specifiers. It supports variadic arguments and returns a formatted string based on the format string and provided arguments.

#### FORMAT SPECIFIERS

##### Basic Types
- **%s** - String (truncated by precision if specified)
- **%d** / **%i** - Signed decimal integer
- **%o** - Octal integer
- **%x** - Hexadecimal (lowercase)
- **%X** - Hexadecimal (uppercase)
- **%c** - Character (integer to ASCII)
- **%%** - Literal percent sign

##### LPC Extensions
- **%O** - Pretty-print LPC values (arrays, mappings, nested structures)
- **%@**_spec_ - Apply format _spec_ to each element of an array
- **%\*** - Dynamic width from next argument
- **%.\*** - Dynamic precision from next argument
- **%|** - Center alignment

#### MODIFIERS

##### Width and Precision
- **%10s** - Minimum field width (right-aligned)
- **%-10s** - Left-aligned in field
- **%.5s** - Maximum precision (truncate to 5 chars)
- **%10.5s** - Width 10, precision 5

##### Flags
- **-** - Left-align (default is right-align)
- **0** - Zero-pad numbers (e.g., %05d → "00042")
- **+** - Always show sign for numbers
- **(space)** - Prefix positive numbers with space
- **@** - Array iteration
- **|** - Center alignment

#### EXAMPLES

##### Basic Formatting
```c
// String interpolation
sprintf("Hello, %s!", "world")                    → "Hello, world!"
sprintf("Player %s has %d gold", name, gold)      → "Player Bob has 150 gold"

// Integer formatting
sprintf("%d", 42)                                 → "42"
sprintf("%05d", 42)                               → "00042"
sprintf("%+d", 42)                                → "+42"
sprintf("% d", 42)                                → " 42"

// Width and alignment
sprintf("%10s", "test")                           → "      test"
sprintf("%-10s", "test")                          → "test      "
sprintf("%5d", 42)                                → "   42"

// Precision (truncation)
sprintf("%.3s", "truncate")                       → "tru"
sprintf("%10.3s", "truncate")                     → "       tru"

// Number bases
sprintf("%d %o %x %X", 255, 255, 255, 255)        → "255 377 ff FF"

// Character conversion
sprintf("%c%c%c", 72, 105, 33)                    → "Hi!"

// Real-world MUD example
sprintf("%-20s%5s gold", item_name, itoa(price))  → "Sword of Flames       150 gold"
sprintf("HP: %3d/%3d  MP: %3d/%3d", 
        curr_hp, max_hp, curr_mp, max_mp)         → "HP: 100/100  MP:  50/ 80"
```

##### LPC Extensions
```c
// %O - Pretty-print LPC values
sprintf("%O", ({ 1, 2, 3 }))                      → "({1,2,3})"
sprintf("%O", ([ "hp": 100, "mp": 50 ]))          → "([\"hp\":100,\"mp\":50])"
sprintf("%O", ({ ({ 1, 2 }), ({ 3, 4 }) }))      → "({({1,2}),({3,4})})"

// %O with width/alignment
sprintf("%15O", ({ 1, 2 }))                       → "        ({1,2})"
sprintf("%-15O", ({ 1, 2 }))                      → "({1,2})        "

// %@ - Array iteration
sprintf("%@d", ({ 1, 2, 3, 4 }))                  → "1234"
sprintf("%@5d", ({ 1, 22, 333 }))                 → "    1   22  333"
sprintf("%@-5s", ({ "a", "bb", "ccc" }))          → "a    bb   ccc  "
sprintf("%@04X", ({ 10, 255 }))                   → "000A00FF"

// %* - Dynamic width
sprintf("%*s", 10, "test")                        → "      test"
sprintf("%-*s", 10, "test")                       → "test      "
sprintf("%*d", 5, 42)                             → "   42"

// %.* - Dynamic precision
sprintf("%.*s", 3, "truncate")                    → "tru"
sprintf("%*.*s", 10, 3, "truncate")               → "       tru"

// %| - Center alignment
sprintf("%|10s", "test")                          → "   test   "
sprintf("%|10s", "hello")                         → "  hello   "
sprintf("%|8d", 42)                               → "   42   "

// Combined features
sprintf("%@*d", 5, ({ 1, 22, 333 }))              → "    1   22  333"
sprintf("%*O", 15, ({ 1, 2 }))                    → "        ({1,2})"
```

#### NOTES

**Missing Arguments**: If not enough arguments are provided for format specifiers, the output will contain `<?>` placeholders.

**Array Iteration Behavior**: When using %@, literal text after the format spec is output once after ALL iterations, not after each element:
```c
sprintf("%@d ", ({ 1, 2, 3 }))  → "123 "   // space after all, not "1 2 3 "
```

**Empty Strings**: The integer `0` is treated as an empty string when used with %s, following NetCI convention.

**Performance**: sprintf() is implemented in C and is significantly faster than manual string concatenation for complex formatting.

**Type Safety**: Type mismatches (e.g., passing string to %d) will attempt conversion or output `<?>`.

#### PLANNED FEATURES

The following advanced features are planned for future implementation:

- **%=** - Column mode (word wrapping to width)
- **%#** - Table mode (multi-column layout)
- **%$** - Justify mode (even spacing between words)
- **Custom padding** - `%'.'10s` (pad with custom characters)

#### SEE ALSO
sscanf(3), itoa(3), explode(3), implode(3), save_value(3)

---

### sscanf

#### NAME
sscanf()  -  parse and extract values from a string

#### SYNOPSIS
```c
int sscanf(string input, string format, ...);
```

#### DESCRIPTION
Parses a string according to a format template and extracts values into variables. This is the inverse of sprintf()—use it to parse player commands, read configuration files, extract data from formatted text, or any time you need to pull structured data out of a string.

Variables are passed directly (not with `&` like in C). The function uses NLPC's internal lvalue mechanism to assign the parsed values. The function returns the number of successful assignments.

Supported format specifiers:
- `%s` - Match and extract a string (matches until next literal text or end of string)
- `%d` - Match and extract a decimal integer (skips leading whitespace)
- `%x` - Match and extract a hexadecimal integer (handles optional 0x prefix)
- `%*s`, `%*d`, `%*x` - Match but don't assign (skip the value)
- `%%` - Match a literal percent sign

Literal text in the format string must match exactly. Parsing stops at the first mismatch.

**Returns**: The number of values successfully parsed and assigned (not counting skipped values).

#### EXAMPLE
```c
// Parse a command (note: no & needed)
string cmd = "get 5 coins from chest";
string verb, item, container;
int count;

if (sscanf(cmd, "%s %d %s from %s", verb, count, item, container) == 4) {
    // Successfully parsed: verb="get", count=5, item="coins", container="chest"
    write(sprintf("Getting %d %s from %s\n", count, item, container));
}

// Parse coordinates
string pos = "x:10 y:25";
int x, y;
if (sscanf(pos, "x:%d y:%d", x, y) == 2) {
    write(sprintf("Position: (%d, %d)\n", x, y));
}

// Parse with skip - extract level but skip HP
string data = "Player: Gandalf Level: 50 HP: 100";
string name;
int level;
// %*s skips "HP:", %*d skips the 100
if (sscanf(data, "Player: %s Level: %d HP: %*d", name, level) == 2) {
    write(sprintf("%s is level %d\n", name, level));
}

// Check if parse succeeded
string input = "attack goblin";
string action, target;
if (sscanf(input, "%s %s", action, target) != 2) {
    write("Invalid command format\n");
    return;
}

// Extract verb and args
string verb, args;
if (sscanf(input, "%s %s", verb, args) == 2) {
    // Got both verb and args
} else if (sscanf(input, "%s", verb) == 1) {
    // Got only verb, no args
    args = "";
}
```

#### SEE ALSO
sprintf(3), explode(3), atoi(3)

---

### upcase

#### NAME
upcase()  -  convert a string to uppercase

#### SYNOPSIS
```c
string upcase(string s);
```

#### DESCRIPTION
Converts all lowercase letters in a string to uppercase. Non-alphabetic characters are unchanged. This is useful for case-insensitive comparisons, formatting output, or normalizing user input.

**Returns**: A new string with all letters converted to uppercase.

#### EXAMPLE
```c
string name = "gandalf";
string upper = upcase(name);  // "GANDALF"

// Case-insensitive comparison
string input = "Yes";
if (upcase(input) == "YES") {
    write("Confirmed!\n");
}

// Format headers
string title = upcase("important message");
write(title + "\n");  // "IMPORTANT MESSAGE"
```

#### SEE ALSO
downcase(3), asc(3)

---

### downcase

#### NAME
downcase()  -  convert a string to lowercase

#### SYNOPSIS
```c
string downcase(string s);
```

#### DESCRIPTION
Converts all uppercase letters in a string to lowercase. Non-alphabetic characters are unchanged. Perfect for normalizing user input, creating case-insensitive keys, or formatting text.

**Returns**: A new string with all letters converted to lowercase.

#### EXAMPLE
```c
string input = "HELLO WORLD";
string lower = downcase(input);  // "hello world"

// Normalize command input
string cmd = downcase("LOOK");  // "look"

// Create case-insensitive mapping keys
mapping data = ([ ]);
data[downcase("PlayerName")] = player_obj;
// Key is "playername"
```

#### SEE ALSO
upcase(3), is_legal(3)

---

### is_legal

#### NAME
is_legal()  -  check if a string is a valid NLPC identifier

#### SYNOPSIS
```c
int is_legal(string s);
```

#### DESCRIPTION
Tests whether a string is a valid NLPC identifier (variable or function name). Valid identifiers must start with a letter or underscore, and contain only letters, digits, and underscores.

This is useful for validating user input before using it as a function name, checking player names, or ensuring data keys are safe to use as identifiers.

**Returns**: 1 if the string is a valid identifier, 0 if not.

#### EXAMPLE
```c
// Validate player name
string name = "Gandalf_123";
if (is_legal(name)) {
    write("Valid player name\n");
} else {
    write("Invalid name - use only letters, numbers, and underscores\n");
}

// Check before dynamic function call
string func_name = "do_something";
if (is_legal(func_name)) {
    call_other(this_object(), func_name);
} else {
    write("Invalid function name\n");
}

// Invalid examples
is_legal("123abc");      // 0 (starts with digit)
is_legal("my-function"); // 0 (contains hyphen)
is_legal("hello world"); // 0 (contains space)
is_legal("_private");    // 1 (valid)
```

#### SEE ALSO
downcase(3), call_other(3)

---

### remove_verb

#### NAME
remove_verb()  -  unregister a verb from this object

#### SYNOPSIS
```c
int remove_verb(string action);
```

#### DESCRIPTION
Removes a previously registered verb from this object. If the verb was added with `add_verb()` or `add_xverb()`, this removes it from the verb list, and players will no longer be able to use that command on this object.

This is useful for temporary verbs (like "read" on a book that gets removed when finished), conditional verbs (like "unlock" that only works when you have the key), or cleaning up when an object changes state.

**Returns**: 0 on success, 1 if the verb wasn't registered.

#### EXAMPLE
```c
// Remove a verb when an item is used up
void consume_potion() {
    write("You drink the potion.\n");
    remove_verb("drink");
    remove_verb("quaff");
    // Potion is empty, can't drink anymore
}

// Conditional verb based on state
void update_door_verbs() {
    if (query_property("locked")) {
        remove_verb("open");
        add_verb("unlock", "do_unlock");
    } else {
        remove_verb("unlock");
        add_verb("open", "do_open");
    }
}

// Clean up all verbs
void remove_all_verbs() {
    remove_verb("get");
    remove_verb("drop");
    remove_verb("examine");
}
```

#### SEE ALSO
add_verb(3), add_xverb(3), next_verb(3)

---

### set_localverbs

#### NAME
set_localverbs()  -  enable or disable local verb checking

#### SYNOPSIS
```c
void set_localverbs(int enabled);
```

#### DESCRIPTION
Controls whether this object's verbs are checked during command processing. When enabled (1), the object's verbs are active and can be matched. When disabled (0), the object's verbs are ignored.

This is useful for temporarily disabling an object's commands without removing the verbs, or for implementing objects that only respond to commands in certain states.

**Returns**: Nothing (void).

#### EXAMPLE
```c
// Disable verbs when object is broken
void break_item() {
    set_localverbs(0);
    set_property("broken", 1);
    write("The item breaks and becomes unusable!\n");
}

// Re-enable when repaired
void repair_item() {
    set_localverbs(1);
    set_property("broken", 0);
    write("The item is repaired!\n");
}
```

#### SEE ALSO
localverbs(3), add_verb(3)

---

### localverbs

#### NAME
localverbs()  -  check if an object has local verbs enabled

#### SYNOPSIS
```c
int localverbs(object obj);
```

#### DESCRIPTION
Tests whether an object has local verb checking enabled. Returns 1 if the object's verbs are active, 0 if they're disabled.

**Returns**: 1 if local verbs are enabled, 0 if disabled.

#### EXAMPLE
```c
// Check before processing
if (localverbs(item)) {
    write("Item can respond to commands\n");
} else {
    write("Item is inactive\n");
}
```

#### SEE ALSO
set_localverbs(3), next_verb(3)

---

### next_verb

#### NAME
next_verb()  -  iterate through an object's registered verbs

#### SYNOPSIS
```c
string next_verb(object obj, string verb);
```

#### DESCRIPTION
Iterates through the verbs registered on an object. Pass 0 as the `verb` parameter to get the first verb, then pass each returned verb to get the next one. When there are no more verbs, it returns 0.

This is useful for listing all commands an object responds to, debugging verb registration, or implementing help systems that show available actions.

**Returns**: The next verb name as a string, or 0 when iteration is complete.

#### EXAMPLE
```c
// List all verbs on an object
void show_verbs(object obj) {
    string verb = next_verb(obj, 0);
    
    write("Available commands:\n");
    while (verb) {
        write("  " + verb + "\n");
        verb = next_verb(obj, verb);
    }
}

// Check if object has a specific verb
int has_verb(object obj, string target) {
    string verb = next_verb(obj, 0);
    while (verb) {
        if (verb == target) return 1;
        verb = next_verb(obj, verb);
    }
    return 0;
}

// Count verbs
int count_verbs(object obj) {
    int count = 0;
    string verb = next_verb(obj, 0);
    while (verb) {
        count++;
        verb = next_verb(obj, verb);
    }
    return count;
}
```

#### SEE ALSO
add_verb(3), remove_verb(3), localverbs(3)

---

### iterate

#### NAME
iterate()  -  iterate over a collection with optional callback

#### SYNOPSIS
```c
mixed iterate(object obj[, string func, ...]);
```

#### DESCRIPTION
Iterates over a collection (typically an object that implements an iteration protocol). If you provide a function name, it calls that function for each element. If you don't provide a function, it returns an iterator token you can use for manual iteration.

The exact behavior depends on the object being iterated. Common uses include iterating over custom data structures, processing all elements in a container, or implementing foreach-style loops.

**Returns**: If a function is provided, returns the number of elements processed. If no function is provided, returns an iterator token (implementation-specific).

#### EXAMPLE
```c
// Iterate with callback
void process_item(mixed item) {
    write("Processing: " + save_value(item) + "\n");
}

void process_all(object container) {
    int count = iterate(container, "process_item");
    write("Processed " + itoa(count) + " items\n");
}

// Manual iteration (if supported)
mixed token = iterate(container);
// Use token for manual iteration...
```

#### SEE ALSO
next_who(3), contents(3), next_object(3)

---

### next_who

#### NAME
next_who()  -  iterate through connected players

#### SYNOPSIS
```c
object next_who(object prev);
```

#### DESCRIPTION
Iterates through all currently connected players (interactive objects). Call with 0 to get the first player, then pass each returned player to get the next one. When there are no more players, it returns 0.

This is essential for implementing "who" commands, broadcasting messages to all players, finding players by name, or any operation that needs to process all connected users.

**Returns**: The next connected player object, or 0 when iteration is complete.

#### EXAMPLE
```c
// Show all connected players
void do_who() {
    object player = next_who(0);
    int count = 0;
    
    write("Connected players:\n");
    while (player) {
        string name = player->query_name();
        write("  " + name + "\n");
        count++;
        player = next_who(player);
    }
    write("Total: " + itoa(count) + " players\n");
}

// Broadcast message to all players
void shout(string msg) {
    object player = next_who(0);
    while (player) {
        player->receive_message(msg);
        player = next_who(player);
    }
}

// Find player by name
object find_player(string name) {
    object player = next_who(0);
    name = downcase(name);
    
    while (player) {
        if (downcase(player->query_name()) == name) {
            return player;
        }
        player = next_who(player);
    }
    return 0;
}

// Count connected players
int count_players() {
    object player = next_who(0);
    int count = 0;
    while (player) {
        count++;
        player = next_who(player);
    }
    return count;
}
```

#### SEE ALSO
interactive(3), connected(3), this_player(3)

---


### get_devport

#### NAME
get_devport()  -  get the port number of a connection

#### SYNOPSIS
```c
int get_devport(object obj);
```

#### DESCRIPTION
Returns the port number that a connected object is using. This is useful for logging connection details, implementing port-based access controls, or debugging network issues.

**Returns**: The port number as an integer, or 0 if the object isn't connected.

#### EXAMPLE
```c
// Log connection details
void log_connection(object player) {
    int port = get_devport(player);
    string ip = query_ip_number(player);
    write(sprintf("Connected from %s:%d\n", ip, port));
}
```

#### SEE ALSO
get_devconn(3), connected(3)

---

### get_devnet

#### NAME
get_devnet()  -  get network information for a connection

#### SYNOPSIS
```c
mixed get_devnet(object obj);
```

#### DESCRIPTION
Returns driver-specific network information about a connected object. The format is implementation-defined but typically includes details like the network interface, protocol, or other low-level connection metadata.

**Returns**: Network information (format varies), or 0 if not connected.

#### EXAMPLE
```c
// Get network details
mixed net_info = get_devnet(player);
if (net_info) {
    write("Network info: " + save_value(net_info) + "\n");
}
```

#### SEE ALSO
get_devconn(3), get_devport(3)

---

### get_devidle

#### NAME
get_devidle()  -  get idle time for a connected object

#### SYNOPSIS
```c
int get_devidle(object obj);
```

#### DESCRIPTION
Returns the number of seconds since the object last sent input. This is perfect for implementing idle timeouts, auto-away systems, or activity monitoring.

The idle time resets whenever the player types a command or sends any input to the server.

**Returns**: Idle time in seconds, or 0 if not connected.

#### EXAMPLE
```c
// Check for idle players
void check_idle() {
    object player = next_who(0);
    while (player) {
        int idle = get_devidle(player);
        if (idle > 300) {  // 5 minutes
            player->receive_message("You've been idle for " + itoa(idle) + " seconds.\n");
        }
        player = next_who(player);
    }
}

// Auto-disconnect idle players
void timeout_idle_players() {
    object player = next_who(0);
    while (player) {
        if (get_devidle(player) > 1800) {  // 30 minutes
            write("Disconnecting due to inactivity.\n");
            disconnect_device();
        }
        player = next_who(player);
    }
}
```

#### SEE ALSO
get_conntime(3), interactive(3)

---

### get_conntime

#### NAME
get_conntime()  -  get connection duration

#### SYNOPSIS
```c
int get_conntime(object obj);
```

#### DESCRIPTION
Returns the Unix timestamp when the object connected. Subtract this from `time()` to get how long they've been connected.

Useful for tracking session length, implementing time-based rewards, or displaying "online since" information.

**Returns**: Unix timestamp of connection time, or 0 if not connected.

#### EXAMPLE
```c
// Show how long player has been connected
void show_uptime(object player) {
    int conn_time = get_conntime(player);
    int duration = time() - conn_time;
    int hours = duration / 3600;
    int minutes = (duration % 3600) / 60;
    
    write(sprintf("Connected for %d hours, %d minutes\n", hours, minutes));
}

// Track total play time
void update_playtime(object player) {
    int session_start = get_conntime(player);
    int session_length = time() - session_start;
    player->add_playtime(session_length);
}
```

#### SEE ALSO
get_devidle(3), time(3), connected(3)

---

### sysctl

#### NAME
sysctl()  -  perform privileged system control operations

#### SYNOPSIS
```c
int sysctl(int oper, ...);
```

#### DESCRIPTION
Performs low-level system control operations. This is a privileged function that only works for objects with the PRIV flag set. The specific operations available depend on the `oper` code.

This is typically used by the master object and system daemons to control driver behavior, manage resources, or perform administrative tasks that normal objects shouldn't access.

**Returns**: Operation-specific return value, typically 0 on success or an error code.

#### EXAMPLE
```c
// System control (requires privilege)
if (priv(this_object())) {
    int result = sysctl(SOME_OPERATION, args);
    if (result == 0) {
        write("System operation completed\n");
    }
}
```

#### SEE ALSO
priv(3), set_priv(3), syslog(3)

---

### syslog

#### NAME
syslog()  -  write a message to the system log

#### SYNOPSIS
```c
void syslog(string msg);
```

#### DESCRIPTION
Writes a message to the system log file. This is for logging important events, errors, security issues, or debugging information that should be preserved outside of normal game output.

Unlike `write()` which sends messages to players, `syslog()` writes to a persistent log file that administrators can review later. Use it for audit trails, error tracking, or recording significant game events.

**Returns**: Nothing (void).

#### EXAMPLE
```c
// Log security events
void log_security(string event) {
    string timestamp = ctime(time());
    string caller = otoa(caller_object());
    syslog(sprintf("[SECURITY] %s: %s (from %s)", timestamp, event, caller));
}

// Log errors
void log_error(string msg) {
    syslog("[ERROR] " + msg);
}

// Log player actions
void log_player_action(object player, string action) {
    string name = player->query_name();
    syslog(sprintf("[PLAYER] %s: %s", name, action));
}
```

#### SEE ALSO
write(3), sysctl(3)

---


### table_set

#### NAME
table_set()  -  store a value in the global table

#### SYNOPSIS
```c
void table_set(string key, string val);
```

#### DESCRIPTION
Stores a key-value pair in the global process-wide table. This is persistent storage that survives object destruction and is accessible from anywhere in the MUD. Think of it as a global mapping that all objects can access.

Because it's global, you should namespace your keys to avoid conflicts. A good practice is to prefix keys with your object's path or a unique identifier.

**Returns**: Nothing (void).

#### EXAMPLE
```c
// Store global configuration
void set_config(string option, string value) {
    table_set("config:" + option, value);
}

// Store per-object data
void save_state() {
    string prefix = otoa(this_object()) + ":";
    table_set(prefix + "state", "active");
    table_set(prefix + "count", itoa(query_count()));
}
```

#### SEE ALSO
table_get(3), table_delete(3)

---

### table_get

#### NAME
table_get()  -  retrieve a value from the global table

#### SYNOPSIS
```c
string table_get(string key);
```

#### DESCRIPTION
Retrieves a value from the global table. If the key doesn't exist, returns 0.

This is useful for sharing data between objects, implementing global flags, or accessing configuration that needs to be available everywhere.

**Returns**: The stored value as a string, or 0 if the key doesn't exist.

#### EXAMPLE
```c
// Get global configuration
string get_config(string option) {
    string value = table_get("config:" + option);
    return value ? value : "default";
}

// Check global flag
int is_shutdown_pending() {
    return table_get("system:shutdown") ? 1 : 0;
}

// Load saved state
void restore_state() {
    string prefix = otoa(this_object()) + ":";
    string state = table_get(prefix + "state");
    if (state) {
        set_state(state);
    }
}
```

#### SEE ALSO
table_set(3), table_delete(3)

---

### table_delete

#### NAME
table_delete()  -  remove a key from the global table

#### SYNOPSIS
```c
void table_delete(string key);
```

#### DESCRIPTION
Removes a key-value pair from the global table. If the key doesn't exist, this does nothing.

Use this to clean up temporary global data, remove obsolete configuration, or free up space in the global table.

**Returns**: Nothing (void).

#### EXAMPLE
```c
// Clean up global state
void cleanup() {
    string prefix = otoa(this_object()) + ":";
    table_delete(prefix + "state");
    table_delete(prefix + "count");
}

// Remove global flag
void clear_shutdown() {
    table_delete("system:shutdown");
}
```

#### SEE ALSO
table_set(3), table_get(3)

---

### attach

#### NAME
attach()  -  attach a component object to this object

#### SYNOPSIS
```c
void attach(object obj);
```

#### DESCRIPTION
Attaches another object as a component of this object, creating a composition relationship. The attached object's functions become available through this object, allowing you to build complex objects from simpler components.

This is NetCI's component system—use it to add behaviors to objects dynamically, implement mixins, or build modular object hierarchies.

**Returns**: Nothing (void).

#### EXAMPLE
```c
// Attach a combat component
void init() {
    object combat = compile_object("/sys/components/combat");
    attach(combat);
}

// Attach multiple components
void setup_player() {
    attach(compile_object("/sys/components/inventory"));
    attach(compile_object("/sys/components/stats"));
    attach(compile_object("/sys/components/skills"));
}
```

#### SEE ALSO
detach(3), this_component(3)

---

### this_component

#### NAME
this_component()  -  get the active component in the call chain

#### SYNOPSIS
```c
object this_component();
```

#### DESCRIPTION
Returns the currently active component object in the attach chain. When a function is called through attached components, this returns which component is actually executing.

This is useful for components that need to know their own identity, or for debugging component interactions.

**Returns**: The active component object.

#### EXAMPLE
```c
// In a component
void component_function() {
    object me = this_component();
    write("Called from component: " + otoa(me) + "\n");
}
```

#### SEE ALSO
attach(3), this_object(3)

---

### detach

#### NAME
detach()  -  remove an attached component

#### SYNOPSIS
```c
void detach(object obj);
```

#### DESCRIPTION
Removes a previously attached component from this object. The component's functions are no longer accessible through this object.

Use this to dynamically remove behaviors, clean up when components are no longer needed, or implement temporary effects.

**Returns**: Nothing (void).

#### EXAMPLE
```c
// Remove a component
void remove_combat() {
    object combat = find_component("/sys/components/combat");
    if (combat) {
        detach(combat);
    }
}

// Clean up all components
void cleanup_components() {
    // Detach each component
    // (implementation depends on how you track components)
}
```

#### SEE ALSO
attach(3), this_component(3)

---

### get_master

#### NAME
get_master()  -  get the top of the attach chain

#### SYNOPSIS
```c
object get_master(object obj);
```

#### DESCRIPTION
Returns the topmost object in the attach chain. When objects are attached to each other (component system), they form a chain. This function walks up that chain to find the "master" object at the top.

If the object has no attacher (it's already the master), returns the object itself.

This is useful for finding the main object when working with components, or ensuring you're operating on the master object rather than an attached component.

**Returns**: The master object (top of attach chain).

#### EXAMPLE
```c
// Find the master object
object component = this_component();
object master = get_master(component);
write("Master: " + otoa(master) + "\n");

// Ensure we're working with the master
void operate_on_master(object obj) {
    object master = get_master(obj);
    master->do_something();
}

// Check attach chain
void show_attach_chain(object obj) {
    object master = get_master(obj);
    if (master == obj) {
        write("This object is the master\n");
    } else {
        write("Master is: " + otoa(master) + "\n");
    }
}
```

#### SEE ALSO
is_master(3), attach(3), this_component(3)

---

### is_master

#### NAME
is_master()  -  check if an object is a master (not attached)

#### SYNOPSIS
```c
int is_master(object obj);
```

#### DESCRIPTION
Tests whether an object is a master object (has no attacher). In the attach/component system, objects can be attached to other objects. A "master" is an object that isn't attached to anything—it's at the top of its attach chain.

**Returns**: 1 if the object is a master (no attacher), 0 if it's attached to something.

#### EXAMPLE
```c
// Check if object is a master
if (is_master(this_object())) {
    write("I am a master object\n");
} else {
    write("I am attached to something\n");
}

// Only allow masters to perform action
void privileged_action() {
    if (!is_master(this_object())) {
        write("Only master objects can do this\n");
        return;
    }
    // Perform action
}

// Filter for master objects
void process_masters_only(object obj) {
    if (is_master(obj)) {
        obj->do_something();
    }
}
```

#### SEE ALSO
get_master(3), attach(3), detach(3)

---

# Privileged Wrappers

Wrappers provided by `/boot.c` for privileged operations and safe delegation.

### compile_object

#### NAME
compile_object()  -  compile a source file and return the prototype

#### SYNOPSIS
```c
object compile_object(string path);
```

#### DESCRIPTION
Compiles an NLPC source file and returns the resulting prototype object. If the file is already compiled and loaded, returns the existing prototype. If there are compilation errors, returns 0.

This is the fundamental way to load code into the MUD. Every object starts as a prototype created by compile_object(). You then clone instances from that prototype.

Note: This is typically wrapped by a privileged function in `/boot.c` for security. Direct access may be restricted.

**Returns**: The prototype object, or 0 on compilation failure.

#### EXAMPLE
```c
// Compile a room
object room_proto = compile_object("/world/tavern");
if (room_proto) {
    write("Room compiled successfully\n");
} else {
    write("Compilation failed\n");
}

// Reload after editing
void reload_object(string path) {
    // Note: May need to destruct old version first
    object proto = compile_object(path);
    if (proto) {
        write("Reloaded: " + path + "\n");
    }
}
```

#### SEE ALSO
get_object(3), clone_object(3), atoo(3)

---

### get_object

#### NAME
get_object()  -  get or compile a prototype object

#### SYNOPSIS
```c
object get_object(string path);
```

#### DESCRIPTION
A convenience function (typically a simulated efun in auto.c) that ensures a prototype exists. If the object is already loaded, returns it. If not, compiles it first, then returns the prototype.

This is safer than `atoo()` alone because it handles the "not yet compiled" case automatically. Use this when you need a prototype and don't care whether it's freshly compiled or already loaded.

**Returns**: The prototype object, or 0 if compilation fails.

#### EXAMPLE
```c
// Get a prototype, compiling if needed
object room = get_object("/world/tavern");
if (room) {
    // Use the prototype
}

// Safe object loading
void load_helper(string path) {
    object helper = get_object(path);
    if (helper) {
        helper->initialize();
    }
}
```

#### SEE ALSO
compile_object(3), atoo(3), clone_object(3)

---

### clone

#### NAME
clone()  -  create a new instance of an object

#### SYNOPSIS
```c
object clone(string path);
```

#### DESCRIPTION
A convenience wrapper (typically in auto.c) that ensures the prototype exists and creates a new clone from it. This combines `get_object()` and `clone_object()` into one step.

Use this when you want to create an instance and don't want to worry about whether the prototype is loaded. It's the most common way to create new objects in game code.

**Returns**: A new clone of the object, or 0 if the prototype can't be loaded or cloning fails.

#### EXAMPLE
```c
// Create a new sword
object sword = clone("/items/weapons/sword");
if (sword) {
    move_object(sword, this_player());
    write("You get a sword\n");
}

// Spawn multiple monsters
void spawn_goblins(int count) {
    for (int i = 0; i < count; i++) {
        object goblin = clone("/npc/goblin");
        move_object(goblin, this_object());
    }
}
```

#### SEE ALSO
clone_object(3), get_object(3), new(3)

---

### write

#### NAME
write()  -  send a message to the current player

#### SYNOPSIS
```c
void write(string msg);
```

#### DESCRIPTION
Sends a message to the current player (or current object if no player). This is the fundamental output function in NetCI—every message you display to players goes through write().

The message is delivered by calling the `listen()` function on the target object, which handles formatting, wrapping, and actual output to the player's connection.

If you need to send a message to a specific object (not the current player), use `tell_object()` or call their `receive_message()` function directly.

**Returns**: Nothing (void).

#### EXAMPLE
```c
// Basic output
write("Hello, world!\n");

// Formatted messages
write("You have " + itoa(gold) + " gold coins.\n");
write(sprintf("HP: %d/%d\n", hp, max_hp));

// Multi-line output
write("Welcome to the tavern!\n");
write("The room is warm and inviting.\n");
write("A fire crackles in the hearth.\n");

// Conditional output
if (is_wizard(this_player())) {
    write("[Wizard] You see hidden exits.\n");
}
```

#### SEE ALSO
syswrite(3), tell_object(3), shout(3)

---
## Object Management Efuns

### contents

#### NAME
contents()  -  get the first object in a container's inventory

#### SYNOPSIS
```c
object contents(object container);
```

#### DESCRIPTION
Returns the first object in a container's inventory. Note: this returns a single object (the first one), not an array. To get all objects, you need to iterate using `next_object()`.

This is the driver's low-level inventory system. The objects are stored in a linked list, and this function gives you the head of that list. If the container is empty, returns 0.

Most game code uses higher-level functions like `query_inventory()` from the mudlib, but this efun is essential for implementing those functions or when you need guaranteed accuracy.

**Returns**: The first object in the container, or 0 if empty.

#### EXAMPLE
```c
// Iterate through inventory
void list_inventory(object container) {
    object item = contents(container);
    while (item) {
        write("  " + item->query_name() + "\n");
        item = next_object(item);
    }
}

// Count items
int count_contents(object container) {
    int count = 0;
    object item = contents(container);
    while (item) {
        count++;
        item = next_object(item);
    }
    return count;
}

// Find specific item
object find_item(object container, string name) {
    object item = contents(container);
    while (item) {
        if (item->query_name() == name) {
            return item;
        }
        item = next_object(item);
    }
    return 0;
}
```

#### SEE ALSO
next_object(3), location(3), move_object(3)

---

### next_object

#### NAME
next_object()  -  get the next object in an inventory list

#### SYNOPSIS
```c
object next_object(object current);
```

#### DESCRIPTION
Returns the next object in a container's inventory linked list. You use this with `contents()` to iterate through all objects in a container.

The pattern is: start with `contents(container)` to get the first object, then repeatedly call `next_object()` on each object to walk the list. When you reach the end, it returns 0.

**Returns**: The next object in the list, or 0 if this is the last object.

#### EXAMPLE
```c
// Standard iteration pattern
object room = this_object();
for (object item = contents(room); item; item = next_object(item)) {
    write("Found: " + item->query_name() + "\n");
}

// Process all items
void process_inventory(object container) {
    object item = contents(container);
    while (item) {
        object next = next_object(item);  // Save next before processing
        process_item(item);  // Might move or destruct item
        item = next;
    }
}

// Build array from linked list
mixed *get_all_contents(object container) {
    mixed *result = ({ });
    object item = contents(container);
    while (item) {
        result += ({ item });
        item = next_object(item);
    }
    return result;
}
```

#### SEE ALSO
contents(3), location(3)

---

### location

#### NAME
location()  -  get an object's container or environment

#### SYNOPSIS
```c
object location(object obj);
```

#### DESCRIPTION
Returns the object that contains `obj`. For items, this is their inventory container. For players, this is the room they're in. For rooms, this is typically 0 (rooms aren't contained).

This is the inverse of `contents()`—while contents() tells you what's inside an object, location() tells you what object something is inside.

**Returns**: The container object, or 0 if the object isn't in anything.

#### EXAMPLE
```c
// Find what room a player is in
object player = this_player();
object room = location(player);
write("You are in: " + room->query_name() + "\n");

// Check if item is in player's inventory
object sword = /* ... */;
if (location(sword) == this_player()) {
    write("You're carrying the sword\n");
}

// Walk up the containment chain
void show_location_chain(object obj) {
    while (obj) {
        write(otoa(obj) + "\n");
        obj = location(obj);
    }
}

// Check if two objects are in the same room
int same_room(object obj1, object obj2) {
    return location(obj1) == location(obj2);
}
```

#### SEE ALSO
contents(3), move_object(3), next_object(3)

---

### move_object

#### NAME
move_object()  -  move an object to a new container (low-level)

#### SYNOPSIS
```c
int move_object(object item, object dest);
```

#### DESCRIPTION
Moves an object from its current location into a new container. This is the driver's low-level move function—it updates the internal inventory lists but doesn't call any mudlib hooks.

**Important**: Most game code should use the mudlib's `move()` function instead, which calls `release_object()` and `receive_object()` hooks for policy enforcement. Use move_object() directly only when you need to bypass those hooks or when implementing the hooks themselves.

The function prevents cyclic moves (moving a room into itself or its contents) and validates that both objects exist.

**Returns**: 0 on success, 1 on failure (invalid objects, cyclic move, etc.).

#### EXAMPLE
```c
// Low-level move (bypasses hooks)
if (move_object(sword, player) == 0) {
    write("Sword moved to player\n");
}

// Teleport player to new room
object new_room = atoo("/world/temple");
if (move_object(this_player(), new_room) == 0) {
    write("You are teleported!\n");
    new_room->describe();
}

// Implementation of mudlib move() function
int move(object dest) {
    object old_env = location(this_object());
    
    // Call hooks for policy
    if (old_env && !old_env->release_object(this_object())) {
        return 0;  // Move rejected
    }
    if (!dest->receive_object(this_object())) {
        return 0;  // Move rejected
    }
    
    // Actually move
    return !move_object(this_object(), dest);
}
```

#### SEE ALSO
location(3), contents(3), next_object(3)

---

### clone_object

#### NAME
clone_object()  -  create a new instance from a prototype

#### SYNOPSIS
```c
object clone_object(string path);
object clone_object(object proto);
```

#### DESCRIPTION
Creates a new clone (instance) of an object. You can pass either a file path (which gets compiled if needed) or an existing prototype object.

Each clone is a separate object with its own variables, but they all share the same code (functions) from the prototype. This is how you create multiple swords, multiple goblins, or multiple instances of any object.

The `new()` efun is an alias for `clone_object()`.

**Returns**: The newly created clone, or 0 on failure.

#### EXAMPLE
```c
// Clone from path
object sword = clone_object("/items/weapons/sword");
move_object(sword, this_player());

// Clone from prototype
object proto = atoo("/npc/goblin");
object goblin1 = clone_object(proto);
object goblin2 = clone_object(proto);
// goblin1 and goblin2 are separate instances

// Spawn loot
void drop_loot() {
    object gold = clone_object("/items/gold");
    gold->set_amount(100);
    move_object(gold, location(this_object()));
}
```

#### SEE ALSO
compile_object(3), new(3), destruct(3)

---

### destruct

#### NAME
destruct()  -  destroy an object

#### SYNOPSIS
```c
int destruct(object obj);
```

#### DESCRIPTION
Queues an object for destruction. The object is removed from the game at the end of the current execution cycle. All references to it become invalid, and it's removed from any containers.

Security restrictions apply:
- Non-privileged objects can only destruct themselves
- Privileged objects can destruct others
- Prototype objects (root objects) are protected and can't be destructed

The object's `destruct()` apply function is called before destruction, allowing cleanup.

**Returns**: 0 on success, 1 on failure (permission denied, protected object, etc.).

#### EXAMPLE
```c
// Self-destruct
void consume_potion() {
    write("You drink the potion.\n");
    this_player()->heal(50);
    destruct(this_object());  // Potion is consumed
}

// Cleanup old objects
void cleanup_corpses(object room) {
    object item = contents(room);
    while (item) {
        object next = next_object(item);
        if (item->is_corpse() && item->query_age() > 300) {
            destruct(item);
        }
        item = next;
    }
}

// Privileged cleanup
void admin_cleanup(object target) {
    if (!priv(this_object())) {
        write("Permission denied\n");
        return;
    }
    if (destruct(target) == 0) {
        write("Object destructed\n");
    }
}
```

#### SEE ALSO
clone_object(3), priv(3)

---

### this_object

#### NAME
this_object()  -  get the current executing object

#### SYNOPSIS
```c
object this_object();
```

#### DESCRIPTION
Returns the object whose code is currently executing. This is the object that contains the function you're in right now.

If objects are attached (component system), this returns the top of the attach chain—the main object, not the component.

This is one of the most commonly used efuns. You use it to refer to "myself" in object code.

**Returns**: The current object.

#### EXAMPLE
```c
// Refer to self
void init() {
    move_object(this_object(), start_room);
}

// Get my own properties
string get_description() {
    return this_object()->query_property("description");
}

// Pass self to other functions
void register_with_daemon() {
    object daemon = atoo("/sys/daemons/combat_d");
    daemon->register_combatant(this_object());
}

// Check my location
object my_room = location(this_object());
```

#### SEE ALSO
this_player(3), caller_object(3), this_component(3)

---

### this_player

#### NAME
this_player()  -  get the player who initiated this action

#### SYNOPSIS
```c
object this_player();
```

#### DESCRIPTION
Returns the player object (interactive user) who initiated the current chain of execution. This is set when a player types a command and persists through the call stack.

If the current execution wasn't initiated by a player (e.g., a heartbeat, alarm, or daemon action), returns 0.

This is essential for commands and interactions—it tells you who is performing the action.

**Returns**: The player object, or 0 if no player is involved.

#### EXAMPLE
```c
// Send message to the acting player
void do_look() {
    object player = this_player();
    object room = location(player);
    player->receive_message(room->query_description());
}

// Check player permissions
int cmd_teleport(string dest) {
    if (!this_player()->is_wizard()) {
        write("You don't have permission\n");
        return 0;
    }
    // Perform teleport
}

// Give item to player
void give_reward() {
    object reward = clone_object("/items/gold");
    move_object(reward, this_player());
    write("You receive gold!\n");
}

// Check if player is present
if (this_player()) {
    write("A player is involved\n");
} else {
    syslog("Background process running");
}
```

#### SEE ALSO
this_object(3), interactive(3), write(3)

---

### caller_object

#### NAME
caller_object()  -  get the object that called this function

#### SYNOPSIS
```c
object caller_object();
```

#### DESCRIPTION
Returns the object that called the currently executing function. This walks back one step in the call stack to find who invoked you.

This is crucial for security checks—you can verify that only authorized objects are calling sensitive functions. It's also useful for implementing callbacks where you need to know who to respond to.

If called from the top level (no caller), returns 0.

**Returns**: The calling object, or 0 if called from top level.

#### EXAMPLE
```c
// Security check
void privileged_function() {
    object caller = caller_object();
    if (!caller || !priv(caller)) {
        write("Access denied\n");
        return;
    }
    // Perform privileged operation
}

// Verify caller is a specific object
void daemon_only_function() {
    object caller = caller_object();
    if (caller != atoo("/sys/daemons/security_d")) {
        syslog("Unauthorized call from " + otoa(caller));
        return;
    }
    // Proceed
}

// Callback pattern
void request_data(object callback_obj) {
    // Process request
    mixed *data = gather_data();
    // Call back to requester
    callback_obj->receive_data(data);
}

// Log who's calling
void log_access() {
    object caller = caller_object();
    if (caller) {
        syslog("Function called by: " + otoa(caller));
    }
}
```

#### SEE ALSO
this_object(3), priv(3)

---

### parent

#### NAME
parent()  -  get the prototype of a clone

#### SYNOPSIS
```c
object parent(object obj);
```

#### DESCRIPTION
Returns the prototype object that a clone was created from. Every clone in NetCI is an instance of a prototype (the compiled program object). This function tells you which prototype an object came from.

If the object is already a prototype (not a clone), returns 0.

This is useful for checking object types, finding all clones of a specific prototype, or implementing type-based logic.

**Returns**: The prototype object, or 0 if the object is a prototype itself.

#### EXAMPLE
```c
// Check what prototype a clone came from
object sword = clone_object("/items/weapons/sword");
object proto = parent(sword);
string path = otoa(proto);  // "/items/weapons/sword"

// Check if two objects are the same type
int same_type(object obj1, object obj2) {
    return parent(obj1) == parent(obj2);
}

// Find all instances of a specific type
void count_goblins() {
    object goblin_proto = atoo("/npc/goblin");
    int count = 0;
    object obj = next_child(goblin_proto);
    while (obj) {
        count++;
        obj = next_child(obj);
    }
    write("There are " + itoa(count) + " goblins\n");
}
```

#### SEE ALSO
prototype(3), next_child(3), clone_object(3)

---

### next_child

#### NAME
next_child()  -  iterate through clones of a prototype

#### SYNOPSIS
```c
object next_child(object obj);
```

#### DESCRIPTION
Returns the next clone in the linked list of all clones created from the same prototype. This lets you iterate through all instances of a particular object type.

To start iteration, call it on the prototype itself. To continue, call it on each clone. When there are no more clones, returns 0.

This is essential for finding all instances of an object type, implementing cleanup routines, or broadcasting messages to all objects of a specific class.

**Returns**: The next clone object, or 0 if there are no more clones.

#### EXAMPLE
```c
// Iterate through all clones of a prototype
object proto = atoo("/items/weapons/sword");
object clone = next_child(proto);
while (clone) {
    write("Found sword: " + otoa(clone) + "\n");
    clone = next_child(clone);
}

// Count all instances
int count_instances(string path) {
    object proto = atoo(path);
    if (!proto) return 0;
    
    int count = 0;
    object clone = next_child(proto);
    while (clone) {
        count++;
        clone = next_child(clone);
    }
    return count;
}

// Cleanup all instances
void cleanup_all_goblins() {
    object proto = atoo("/npc/goblin");
    object goblin = next_child(proto);
    while (goblin) {
        object next = next_child(goblin);  // Save before destruct
        destruct(goblin);
        goblin = next;
    }
}
```

#### SEE ALSO
parent(3), prototype(3), clone_object(3)

---

### next_proto

#### NAME
next_proto()  -  iterate through all loaded prototypes

#### SYNOPSIS
```c
object next_proto(object obj);
```

#### DESCRIPTION
Returns the next prototype in the global list of all loaded prototypes. The driver maintains a linked list of every compiled program (prototype object), and this function walks that list.

This is useful for implementing system-wide operations like finding all loaded objects, memory analysis, or administrative commands that need to inspect every program in the MUD.

**Returns**: The next prototype object, or 0 if there are no more prototypes.

#### EXAMPLE
```c
// List all loaded prototypes
void list_all_prototypes() {
    object proto = atoo("/boot");  // Start with boot
    while (proto) {
        write(otoa(proto) + "\n");
        proto = next_proto(proto);
    }
}

// Count total loaded objects
int count_prototypes() {
    int count = 0;
    object proto = atoo("/boot");
    while (proto) {
        count++;
        proto = next_proto(proto);
    }
    return count;
}

// Find prototypes matching a pattern
void find_prototypes(string pattern) {
    object proto = atoo("/boot");
    while (proto) {
        string path = otoa(proto);
        if (instr(path, 1, pattern) > 0) {
            write("Found: " + path + "\n");
        }
        proto = next_proto(proto);
    }
}
```

#### SEE ALSO
prototype(3), parent(3), next_child(3)

---

### prototype

#### NAME
prototype()  -  check if an object is a prototype

#### SYNOPSIS
```c
int prototype(object obj);
```

#### DESCRIPTION
Tests whether an object is a prototype (compiled program) or a clone (instance). Prototypes are the original compiled objects that clones are created from. They have the PROTOTYPE flag set.

This is useful for distinguishing between the "class" (prototype) and "instances" (clones), implementing different behavior for prototypes vs clones, or validating object types.

**Returns**: 1 if the object is a prototype, 0 if it's a clone.

#### EXAMPLE
```c
// Check object type
object obj = this_object();
if (prototype(obj)) {
    write("This is a prototype\n");
} else {
    write("This is a clone\n");
}

// Prevent operations on prototypes
void take_damage(int amount) {
    if (prototype(this_object())) {
        write("Error: Can't damage a prototype!\n");
        return;
    }
    hp -= amount;
}

// Filter for clones only
void process_clones_only(object obj) {
    if (!prototype(obj)) {
        // This is a clone, process it
        obj->do_something();
    }
}
```

#### SEE ALSO
parent(3), next_child(3), clone_object(3)

---

### otoa

#### NAME
otoa()  -  convert an object reference to its file path

#### SYNOPSIS
```c
string otoa(object obj);
```

#### DESCRIPTION
Returns the canonical file path for an object. Every object in NetCI corresponds to a source file (for prototypes) or derives from one (for clones). This function gives you that path as a string.

This is essential for logging, debugging, saving object references to disk (you can't serialize object pointers, but you can save paths), or any time you need to identify what an object is.

For clones, this returns the path of the prototype they were cloned from, not a unique path for the clone itself. If you need a unique identifier for clones, use `otoi()` instead.

**Returns**: The file path as a string (e.g., "/world/npc/goblin"), or 0 if the object is invalid.

#### EXAMPLE
```c
object room = location(this_player());
string path = otoa(room);
write("You are in: " + path + "\n");
// Might print: "You are in: /world/start/tavern"

// Save object reference to a file
mapping save_data = ([
    "home": otoa(this_player()->query_home()),
    "location": otoa(location(this_player()))
]);
string data = save_value(save_data);
```

#### SEE ALSO
atoo(3), otoi(3), itoo(3)

---

### atoo

#### NAME
atoo()  -  convert a file path to an object reference

#### SYNOPSIS
```c
object atoo(string path);
```

#### DESCRIPTION
Takes a file path and returns the corresponding object, if it's currently loaded in the game. This only works for prototypes (program objects)—you can't use it to get clones.

If the file hasn't been compiled yet, this returns 0. It doesn't automatically compile the file for you—use `compile_object()` or the simulated efun `get_object()` if you want that behavior.

This is your go-to function for getting references to system objects, rooms, or any other prototype you need to interact with.

**Returns**: The object reference if loaded, or 0 if the path doesn't correspond to a loaded object.

#### EXAMPLE
```c
// Get the master object
object master = atoo("/boot");
if (master) {
    master->some_function();
}

// Check if a room is loaded
object tavern = atoo("/world/start/tavern");
if (!tavern) {
    write("Tavern not loaded yet.\n");
}

// Load saved object references
mapping save_data = restore_value(fread("/data/player.o", 0));
string home_path = save_data["home"];
object home = atoo(home_path);
if (!home) {
    // Compile it if needed
    home = compile_object(home_path);
}
```

#### SEE ALSO
otoa(3), compile_object(3), get_object(3)

---

### otoi

#### NAME
otoi()  -  get an object's unique numeric identifier

#### SYNOPSIS
```c
int otoi(object obj);
```

#### DESCRIPTION
Returns a unique numeric ID for an object. Unlike `otoa()`, which returns the same path for all clones of a prototype, `otoi()` gives each object (prototype or clone) its own distinct number.

These IDs are assigned sequentially as objects are created and are never reused during a server session. They're perfect for tracking specific clones, implementing object caches with integer keys, or any situation where you need a lightweight unique identifier.

Note: IDs are not persistent across server restarts. Don't save them to disk expecting them to work later.

**Returns**: A unique integer ID for the object.

#### EXAMPLE
```c
object sword1 = clone_object("/items/sword");
object sword2 = clone_object("/items/sword");

int id1 = otoi(sword1);  // e.g., 1234
int id2 = otoi(sword2);  // e.g., 1235

// They're different objects, different IDs
if (id1 != id2) {
    write("Two distinct swords!\n");
}

// Use IDs as mapping keys
mapping object_cache = ([ ]);
object_cache[otoi(sword1)] = "Player's sword";
object_cache[otoi(sword2)] = "Merchant's sword";
```

#### SEE ALSO
itoo(3), otoa(3), clone_object(3)

---

### itoo

#### NAME
itoo()  -  convert a numeric ID back to an object reference

#### SYNOPSIS
```c
object itoo(int id);
```

#### DESCRIPTION
Takes an object ID (from `otoi()`) and returns the corresponding object, if it still exists. If the object has been destructed or the ID is invalid, you get 0.

This is useful for maintaining temporary object references by ID, implementing undo systems, or any time you've stored an ID and need to get the object back.

Remember: IDs are only valid for the current server session. After a restart, all IDs are invalid.

**Returns**: The object reference if it exists, or 0 if the ID is invalid or the object has been destructed.

#### EXAMPLE
```c
object sword = clone_object("/items/sword");
int id = otoi(sword);

// Later, get it back
object same_sword = itoo(id);
if (same_sword) {
    write("Found the sword!\n");
}

// Check if an object still exists
int saved_id = /* ... */;
object obj = itoo(saved_id);
if (!obj) {
    write("That object doesn't exist anymore.\n");
}

// Implement an object cache
mapping cache = ([ ]);
void cache_object(object obj) {
    cache[otoi(obj)] = time();
}
object get_cached(int id) {
    if (member(cache, id)) {
        return itoo(id);
    }
    return 0;
}
```

#### SEE ALSO
otoi(3), atoo(3)

---

# Efuns

Efuns are the core system functions provided by the NetCI driver. They're implemented in C for performance and provide the primitive operations that everything else builds on.

## Efuns Index
- Object management
  - contents(obj)
  - next_object(obj)
  - location(obj)
  - move_object(item, dest)
  - clone_object(path|object) (alias: new(path))
  - destruct(obj)
  - this_object()
  - this_player()
  - caller_object()
  - parent(obj)
  - next_child(obj)
  - next_proto(obj)
  - prototype(obj)
  - otoa(obj) / atoo(str)
  - otoi(obj) / itoo(int)
- Verb/Command system
  - add_verb(verb, func)
  - add_xverb(verb, func)
  - remove_verb(verb)
  - set_localverbs(int)
  - localverbs(obj)
  - next_verb(obj, verb)
- Interactive/Device
  - set_interactive(int)
  - interactive(obj)
  - connected(obj)
  - get_devconn(obj)
  - send_device(str)
  - reconnect_device(obj)
  - disconnect_device()
  - get_devidle(obj)
  - get_conntime(obj)
  - get_devport(obj)
  - get_devnet(obj)
  - connect_device(address, port)
  - flush_device()
  - redirect_input(func)
  - input_to(obj, func)
  - get_input_func()
- Scheduling/Time
  - alarm(seconds, func)
  - remove_alarm([func])
  - time()
  - mktime(...)
  - random(max)
- Compilation/Execution
  - compile_object(path)
  - call_other(obj, func, ...)
  - command(str)
- Filesystem
  - cat(path), ls(path), rm(path), cp(src, dst), mv(src, dst)
  - mkdir(path), rmdir(path)
  - fread(path, pos[, lines]), fwrite(path, str), ferase(path)
  - fstat(path), fowner(path)
  - hide(path), unhide(path, owner, flags), chown(path, owner)
  - edit(path), in_editor(obj)
- Strings
  - strlen(s), leftstr(s, len), rightstr(s, len), midstr(s, pos, len)
  - instr(s, start, search), subst(s, pos, len, s2)
  - upcase(s), downcase(s)
  - sprintf(fmt, ...), sscanf(input, format, ...)
  - replace_string(str, search, repl)
- Conversion/Type
  - itoa(int), atoi(str), chr(int), asc(str)
  - typeof(x), is_legal(str)
- Arrays
  - sizeof(array|mapping)
  - implode(arr, sep), explode(str, sep)
  - member_array(elem, arr), sort_array(arr), reverse(arr), unique_array(arr)
- Mappings
  - keys(mapping), values(mapping)
  - map_delete(mapping, key), member(mapping, key)
- Table Storage (Global)
  - table_get(key), table_set(key, value), table_delete(key)
- Misc/System
  - iterate(obj[, func, ...]), next_who(obj)
  - get_hostname(ip), get_address(host)
  - get_master(obj), is_master(obj)
  - sysctl(cmd, ...), syslog(str), syswrite(str)
  - set_priv(obj, int), priv(obj)
  - attach(obj), this_component(), detach(obj)

## Detailed Efuns Reference

### keys

#### NAME
keys()  -  return an array of the keys from the (key, value) pairs in a mapping

#### SYNOPSIS
```c
mixed *keys(mapping m);
```

#### DESCRIPTION
keys() returns an array of keys (indices) corresponding to the keys in the (key, value) pairs stored in the mapping m.

For example, if:

```c
mapping m;
m = (["hp" : 35, "sp" : 42, "mass" : 100]);
```

then

```c
keys(m) == ({"hp", "sp", "mass"})
```

Note: the keys will not be returned in any apparent order. However, they will be returned in the same order as the corresponding values (returned by the values() efun). This is because both functions iterate through the same hash table structure—the order is unpredictable but consistent between the two calls.

**Returns**: An array of mixed type containing all keys from the mapping. Returns an empty array `({})` if the mapping is empty.

#### SEE ALSO
values(3), sizeof(3), member(3), map_delete(3)

---

### values

#### NAME
values()  -  return an array of the values from the (key, value) pairs in a mapping

#### SYNOPSIS
mixed *values(mapping m);

#### DESCRIPTION
values() returns an array of values corresponding to the pairs stored in mapping m.

Order: Returned in the same iteration order as keys(), so ks[i] and vs[i] correspond to the same pair.

#### EXAMPLE
```c
mapping m;
m = (["hp":35, "sp":42, "mass":100]);
mixed *vs = values(m); /* e.g., ({ 35, 42, 100 }) in some order */
```

#### SEE ALSO
keys(3), sizeof(3), member(3)

---

### map_delete

#### NAME
map_delete()  -  remove a key-value pair from a mapping

#### SYNOPSIS
```c
void map_delete(mapping m, mixed key);
```

#### DESCRIPTION
Removes a key and its associated value from a mapping. If the key doesn't exist, this does nothing (no error). The mapping is modified in place.

This is how you delete entries from mappings—use it to remove obsolete data, clean up temporary keys, or implement expiring caches.

**Returns**: Nothing (void).

#### EXAMPLE
```c
// Remove a key
mapping stats = ([ "hp": 100, "mp": 50, "stamina": 75 ]);
map_delete(stats, "stamina");
// stats now has only "hp" and "mp"

// Safe deletion (no error if key doesn't exist)
map_delete(stats, "nonexistent_key");  // Does nothing

// Clean up expired cache entries
void cleanup_cache(mapping cache, int max_age) {
    mixed *keys_to_delete = ({ });
    foreach (string key in keys(cache)) {
        if (time() - cache[key] > max_age) {
            keys_to_delete += ({ key });
        }
    }
    foreach (string key in keys_to_delete) {
        map_delete(cache, key);
    }
}
```

#### SEE ALSO
member(3), keys(3), values(3)

---

### member

#### NAME
member()  -  check if a key exists in a mapping

#### SYNOPSIS
```c
int member(mapping m, mixed key);
```

#### DESCRIPTION
Tests whether a key exists in a mapping. This is faster than checking if the value is 0, because 0 might be a valid value stored in the mapping.

Use this before accessing mapping values to avoid getting 0 for both "key doesn't exist" and "key exists with value 0".

**Returns**: 1 if the key exists, 0 if not.

#### EXAMPLE
```c
mapping stats = ([ "hp": 100, "mp": 0 ]);

// Check before access
if (member(stats, "hp")) {
    int hp = stats["hp"];  // Safe to access
}

// Distinguish between missing and zero
if (member(stats, "mp")) {
    write("MP exists and is: " + itoa(stats["mp"]) + "\n");  // "MP exists and is: 0"
}

if (!member(stats, "stamina")) {
    write("Stamina not set\n");
}

// Conditional initialization
if (!member(cache, key)) {
    cache[key] = compute_value(key);
}
```

#### SEE ALSO
map_delete(3), keys(3), values(3)

---

### sizeof

#### NAME
sizeof()  -  get the number of elements in an array or mapping

#### SYNOPSIS
```c
int sizeof(mixed *array);
int sizeof(mapping m);
```

#### DESCRIPTION
Returns the number of elements in an array or the number of key-value pairs in a mapping. This is one of the most frequently used efuns—you'll use it constantly for bounds checking, iteration, and validation.

For arrays, it returns the element count. For mappings, it returns the number of keys. For the integer 0 (which NLPC treats as an empty array/mapping in some contexts), it returns 0.

**Returns**: The number of elements/pairs as an integer.

#### EXAMPLE
```c
// Array size
mixed *items = ({ "sword", "shield", "potion" });
int count = sizeof(items);  // 3

// Mapping size
mapping stats = ([ "hp": 100, "mp": 50, "level": 5 ]);
int num_stats = sizeof(stats);  // 3

// Bounds checking
void process_array(mixed *arr) {
    for (int i = 0; i < sizeof(arr); i++) {
        process_element(arr[i]);
    }
}

// Check if empty
if (sizeof(inventory) == 0) {
    write("Your inventory is empty\n");
}

// Validate input
void set_stats(mapping new_stats) {
    if (sizeof(new_stats) > 10) {
        write("Too many stats!\n");
        return;
    }
    stats = new_stats;
}
```

#### SEE ALSO
keys(3), values(3), strlen(3)

---

### implode

#### NAME
implode()  -  join array elements into a string

#### SYNOPSIS
```c
string implode(mixed *array, string separator);
```

#### DESCRIPTION
Joins the elements of an array into a single string, with the separator inserted between each element. Elements are converted to strings: integers become decimal strings, strings are used as-is.

This is the inverse of `explode()`—use it to build comma-separated lists, format output, create file paths, or any time you need to combine array elements into a string.

**Returns**: A string with all elements joined by the separator.

#### EXAMPLE
```c
// Create comma-separated list
mixed *names = ({ "Alice", "Bob", "Charlie" });
string list = implode(names, ", ");
// "Alice, Bob, Charlie"

// Build a path
mixed *parts = ({ "world", "areas", "forest", "clearing" });
string path = "/" + implode(parts, "/");
// "/world/areas/forest/clearing"

// Format numbers
mixed *scores = ({ 100, 95, 87, 92 });
string score_str = implode(scores, " | ");
// "100 | 95 | 87 | 92"

// Create sentence
mixed *words = ({ "The", "quick", "brown", "fox" });
string sentence = implode(words, " ") + ".";
// "The quick brown fox."
```

#### SEE ALSO
explode(3), sprintf(3)

---

### explode

#### NAME
explode()  -  split a string into an array

#### SYNOPSIS
```c
mixed *explode(string str, string separator);
```

#### DESCRIPTION
Splits a string into an array of substrings, breaking at each occurrence of the separator. The separator itself is not included in the results.

If the separator is empty (""), returns an array with the original string as the only element. If the string is empty, returns an empty array.

This is the inverse of `implode()`—use it to parse comma-separated values, split sentences into words, break paths into components, or tokenize any delimited text.

**Returns**: An array of strings.

#### EXAMPLE
```c
// Parse CSV
string csv = "apple,banana,cherry";
mixed *fruits = explode(csv, ",");
// ({ "apple", "banana", "cherry" })

// Split into words
string sentence = "The quick brown fox";
mixed *words = explode(sentence, " ");
// ({ "The", "quick", "brown", "fox" })

// Parse path
string path = "/world/areas/forest/clearing";
mixed *parts = explode(path, "/");
// ({ "", "world", "areas", "forest", "clearing" })
// Note: first element is empty because path starts with /

// Split on newlines
string text = "Line 1\nLine 2\nLine 3";
mixed *lines = explode(text, "\n");
// ({ "Line 1", "Line 2", "Line 3" })

// Handle multiple delimiters
string data = "a::b::c";
mixed *items = explode(data, "::");
// ({ "a", "b", "c" })
```

#### SEE ALSO
implode(3), sscanf(3)

---

### replace_string

#### NAME
replace_string()  -  replace all occurrences of a substring

#### SYNOPSIS
```c
string replace_string(string str, string search, string replacement);
```

#### DESCRIPTION
Returns a new string with all occurrences of the search string replaced by the replacement string. Unlike `subst()` which replaces by position, this searches for the pattern and replaces every match.

The original string is unchanged (strings are immutable). If the search string isn't found, returns the original string. If the search string is empty, returns the original string.

**Returns**: A new string with all replacements made.

#### EXAMPLE
```c
// Remove all spaces
string text = "hello world foo bar";
string compact = replace_string(text, " ", "");
// "helloworldfoobar"

// Replace all occurrences
string msg = "The cat sat on the cat mat";
string fixed = replace_string(msg, "cat", "dog");
// "The dog sat on the dog mat"

// Censor profanity
string clean = replace_string(player_input, "badword", "****");

// Normalize line endings
string normalized = replace_string(text, "\r\n", "\n");

// Replace multiple times
string s = "foo";
s = replace_string(s, "o", "a");  // "faa"
s = replace_string(s, "f", "b");  // "baa"
```

#### SEE ALSO
subst(3), instr(3), sscanf(3)

---

### member_array

#### NAME
member_array()  -  find the position of an element in an array

#### SYNOPSIS
```c
int member_array(mixed element, mixed *array);
```

#### DESCRIPTION
Searches for an element in an array and returns its index (0-based). If the element appears multiple times, returns the index of the first occurrence. If not found, returns -1.

This is essential for checking if an array contains a value, finding positions, or implementing set operations.

**Returns**: The 0-based index of the element, or -1 if not found.

#### EXAMPLE
```c
// Find element
mixed *items = ({ "sword", "shield", "potion" });
int pos = member_array("shield", items);  // 1

// Check if present
if (member_array("key", inventory) >= 0) {
    write("You have the key!\n");
}

// Not found
int idx = member_array("dragon", items);  // -1

// Find and remove
void remove_item(mixed *arr, mixed item) {
    int pos = member_array(item, arr);
    if (pos >= 0) {
        arr = arr[0..pos-1] + arr[pos+1..];
    }
}

// Check multiple items
int has_any(mixed *inventory, mixed *required) {
    foreach (mixed item in required) {
        if (member_array(item, inventory) >= 0) {
            return 1;
        }
    }
    return 0;
}
```

#### SEE ALSO
sort_array(3), sizeof(3), member(3)

---

### sort_array

#### NAME
sort_array()  -  sort an array in ascending order

#### SYNOPSIS
```c
mixed *sort_array(mixed *array);
```

#### DESCRIPTION
Returns a new array with the elements sorted in ascending order. Works with integers and strings. Integers are sorted numerically, strings are sorted alphabetically (case-sensitive).

The original array is unchanged. The sort is stable and uses a simple comparison.

**Returns**: A new sorted array.

#### EXAMPLE
```c
// Sort numbers
mixed *nums = ({ 5, 2, 8, 1, 9 });
mixed *sorted = sort_array(nums);
// ({ 1, 2, 5, 8, 9 })

// Sort strings
mixed *names = ({ "Charlie", "Alice", "Bob" });
mixed *sorted_names = sort_array(names);
// ({ "Alice", "Bob", "Charlie" })

// Display sorted inventory
void show_inventory() {
    mixed *items = query_inventory();
    mixed *names = ({ });
    foreach (object item in items) {
        names += ({ item->query_name() });
    }
    names = sort_array(names);
    write("Inventory:\n");
    foreach (string name in names) {
        write("  " + name + "\n");
    }
}
```

#### SEE ALSO
reverse(3), unique_array(3), member_array(3)

---

### reverse

#### NAME
reverse()  -  reverse the order of array elements

#### SYNOPSIS
```c
mixed *reverse(mixed *array);
```

#### DESCRIPTION
Returns a new array with the elements in reverse order. The first element becomes the last, the last becomes the first, and so on.

The original array is unchanged.

**Returns**: A new array with elements in reverse order.

#### EXAMPLE
```c
// Reverse numbers
mixed *nums = ({ 1, 2, 3, 4, 5 });
mixed *rev = reverse(nums);
// ({ 5, 4, 3, 2, 1 })

// Reverse strings
mixed *words = ({ "first", "second", "third" });
mixed *backwards = reverse(words);
// ({ "third", "second", "first" })

// Reverse sort (descending)
mixed *sorted_desc = reverse(sort_array(array));

// Process in reverse order
void process_backwards(mixed *items) {
    foreach (mixed item in reverse(items)) {
        process(item);
    }
}
```

#### SEE ALSO
sort_array(3), unique_array(3)

---

### unique_array

#### NAME
unique_array()  -  remove duplicate elements from an array

#### SYNOPSIS
```c
mixed *unique_array(mixed *array);
```

#### DESCRIPTION
Returns a new array with all duplicate elements removed. When duplicates are found, only the first occurrence is kept. The order of remaining elements is preserved.

This is useful for cleaning up lists, implementing set operations, or ensuring uniqueness in collections.

**Returns**: A new array with duplicates removed.

#### EXAMPLE
```c
// Remove duplicates
mixed *items = ({ "apple", "banana", "apple", "cherry", "banana" });
mixed *unique = unique_array(items);
// ({ "apple", "banana", "cherry" })

// Clean up list
mixed *numbers = ({ 1, 2, 2, 3, 1, 4, 3 });
mixed *clean = unique_array(numbers);
// ({ 1, 2, 3, 4 })

// Combine and deduplicate
mixed *list1 = ({ "a", "b", "c" });
mixed *list2 = ({ "b", "c", "d" });
mixed *combined = unique_array(list1 + list2);
// ({ "a", "b", "c", "d" })

// Get unique player names
void show_unique_visitors(mixed *log) {
    mixed *names = ({ });
    foreach (mapping entry in log) {
        names += ({ entry["player"] });
    }
    names = unique_array(names);
    write("Unique visitors: " + implode(names, ", ") + "\n");
}
```

#### SEE ALSO
sort_array(3), member_array(3), reverse(3)

---

### redirect_input

#### NAME
redirect_input()  -  capture the next line of player input

#### SYNOPSIS
```c
void redirect_input(string funcname);
```

#### DESCRIPTION
Redirects the next line of input from the player to a specific function on this object. Instead of going through normal command processing, the input is passed directly to the named function as a string argument.

This is perfect for implementing prompts, getting player responses, reading multi-line input, or any time you need to capture raw input without command parsing.

After the function is called, input handling returns to normal unless you call `redirect_input()` again.

**Returns**: Nothing (void).

#### EXAMPLE
```c
// Ask for confirmation
void ask_delete_character() {
    write("Are you sure you want to delete your character? (yes/no): ");
    redirect_input("handle_delete_confirm");
}

void handle_delete_confirm(string answer) {
    if (downcase(answer) == "yes") {
        write("Character deleted.\n");
        delete_character();
    } else {
        write("Cancelled.\n");
    }
}

// Get player name
void prompt_for_name() {
    write("Enter your character name: ");
    redirect_input("receive_name");
}

void receive_name(string name) {
    if (strlen(name) < 3) {
        write("Name too short. Try again: ");
        redirect_input("receive_name");
        return;
    }
    set_name(name);
    write("Welcome, " + name + "!\n");
}
```

#### SEE ALSO
input_to(3), get_input_func(3)

---

### input_to

#### NAME
input_to()  -  redirect input to a function on another object

#### SYNOPSIS
```c
void input_to(object target, string funcname);
```

#### DESCRIPTION
Redirects the next line of player input to a function on a different object. This is like `redirect_input()` but sends the input to another object instead of this one.

Use this for delegation patterns, implementing shared input handlers, or when a daemon needs to collect input from players.

**Returns**: Nothing (void).

#### EXAMPLE
```c
// Delegate to a daemon
void ask_question() {
    object quiz_daemon = atoo("/sys/daemons/quiz_d");
    write("What is 2 + 2? ");
    input_to(quiz_daemon, "check_answer");
}

// In quiz_daemon:
void check_answer(string answer) {
    if (answer == "4") {
        tell_object(this_player(), "Correct!\n");
    } else {
        tell_object(this_player(), "Wrong!\n");
    }
}

// Password entry to security object
void login() {
    object security = atoo("/sys/security");
    write("Password: ");
    input_to(security, "verify_password");
}
```

#### SEE ALSO
redirect_input(3), get_input_func(3), this_player(3)

---

### get_input_func

#### NAME
get_input_func()  -  check if input is being redirected

#### SYNOPSIS
```c
string get_input_func();
```

#### DESCRIPTION
Returns the name of the function that will receive the next line of input, if input has been redirected with `redirect_input()` or `input_to()`. If input is not being redirected, returns 0.

Use this to check if a player is in the middle of answering a prompt, or to debug input handling.

**Returns**: The function name as a string, or 0 if no redirect is active.

#### EXAMPLE
```c
// Check if player is busy
int is_busy() {
    return get_input_func() != 0;
}

// Cancel pending input
void cancel_prompt() {
    if (get_input_func()) {
        write("Prompt cancelled.\n");
        // Clear the redirect somehow
    }
}

// Debug
void show_status() {
    string func = get_input_func();
    if (func) {
        write("Waiting for input to: " + func + "\n");
    } else {
        write("Normal command mode\n");
    }
}
```

#### SEE ALSO
redirect_input(3), input_to(3)

---

### call_other

#### NAME
call_other()  -  call a function on another object

#### SYNOPSIS
```c
mixed call_other(object obj, string funcname, ...);
```

#### DESCRIPTION
Calls a function on another object and returns its result. This is the fundamental inter-object communication mechanism in NLPC. You can also use the `->` operator as syntactic sugar: `obj->func(args)` is equivalent to `call_other(obj, "func", args)`.

If the function doesn't exist or the object is invalid, returns 0. If the function exists but returns void, you still get 0.

**Returns**: The return value of the called function, or 0 on error.

#### EXAMPLE
```c
// Call a function
object player = this_player();
string name = call_other(player, "query_name");

// Using -> syntax (preferred)
string name = player->query_name();

// With arguments
int hp = player->query_stat("hp");

// Check if function exists
mixed result = obj->some_function();
if (result == 0) {
    // Either function doesn't exist, or it returned 0
    // Can't distinguish without other checks
}

// Call with multiple arguments
object room = location(this_player());
room->receive_message("say", this_player(), "Hello!");

// Dynamic function calls
string func_name = "do_" + action;
mixed result = call_other(this_object(), func_name, args);
```

#### SEE ALSO
this_object(3), caller_object(3)

---

### random

#### NAME
random()  -  generate a random number

#### SYNOPSIS
```c
int random(int max);
```

#### DESCRIPTION
Returns a random integer in the range 0 to max-1 (inclusive). This is your random number generator for dice rolls, random encounters, loot drops, or any game mechanic that needs unpredictability.

The distribution is uniform—each number in the range has an equal chance of being selected.

**Returns**: A random integer from 0 to max-1.

#### EXAMPLE
```c
// Simulate a six-sided die (1-6)
int roll = random(6) + 1;

// Random chance (50%)
if (random(2) == 0) {
    write("Heads!\n");
} else {
    write("Tails!\n");
}

// Random percentage (0-99)
int percent = random(100);
if (percent < 25) {
    write("25% chance event!\n");
}

// Pick random element from array
mixed *items = ({ "sword", "shield", "potion" });
mixed random_item = items[random(sizeof(items))];

// Random damage range
int damage = 10 + random(11);  // 10-20 damage

// Random encounter
if (random(100) < 10) {  // 10% chance
    spawn_monster();
}
```

#### SEE ALSO
time(3)

---

### time

#### NAME
time()  -  get the current Unix timestamp

#### SYNOPSIS
```c
int time();
```

#### DESCRIPTION
Returns the current time as a Unix timestamp (seconds since January 1, 1970 00:00:00 UTC). This is the standard way to work with time in NLPC.

Use this for timestamps, measuring durations, implementing cooldowns, scheduling events, or any time-based game mechanics.

**Returns**: Current Unix timestamp as an integer.

#### EXAMPLE
```c
// Record when something happened
int event_time = time();

// Check elapsed time
int start_time = time();
// ... do something ...
int elapsed = time() - start_time;
write("That took " + itoa(elapsed) + " seconds\n");

// Cooldown system
mapping cooldowns = ([ ]);
void set_cooldown(string ability, int duration) {
    cooldowns[ability] = time() + duration;
}

int can_use(string ability) {
    if (!member(cooldowns, ability)) return 1;
    return time() >= cooldowns[ability];
}

// Timestamp data
mapping player_data = ([ ]);
player_data["last_login"] = time();
player_data["created"] = time();

// Check if expired
int is_expired(int timestamp, int duration) {
    return time() > timestamp + duration;
}
```

#### SEE ALSO
ctime(3), mktime(3), get_conntime(3)

---

### mktime

#### NAME
mktime()  -  convert date/time components to Unix timestamp

#### SYNOPSIS
```c
int mktime(int year, int month, int day, int hour, int minute, int second);
```

#### DESCRIPTION
Converts individual date and time components into a Unix timestamp. This is the inverse of breaking down a timestamp—use it to create specific dates, schedule future events, or work with calendar dates.

The year should be the full year (e.g., 2025, not 25). Month is 1-12, day is 1-31, hour is 0-23, minute and second are 0-59.

**Returns**: Unix timestamp as an integer.

#### EXAMPLE
```c
// Create a specific date
int new_years = mktime(2025, 1, 1, 0, 0, 0);

// Schedule an event
int event_time = mktime(2025, 12, 25, 12, 0, 0);  // Christmas noon

// Check if date has passed
int deadline = mktime(2025, 6, 1, 0, 0, 0);
if (time() > deadline) {
    write("Deadline has passed!\n");
}

// Calculate age
int birth_time = mktime(2000, 5, 15, 0, 0, 0);
int age_seconds = time() - birth_time;
int age_years = age_seconds / (365 * 24 * 60 * 60);
```

#### SEE ALSO
time(3), ctime(3)

---

### compile_object (efun)

#### NAME
compile_object()  -  compile a source file to a prototype object

#### SYNOPSIS
object compile_object(string path);

#### DESCRIPTION
Parses and compiles the program at `path`. Returns the prototype object or 0 on error.

#### EXAMPLE
```c
object proto = compile_object("/world/room.c");
```

#### SEE ALSO
compile_string(3), atoo(3), otoa(3)

---

### compile_string

#### NAME
compile_string()  -  compile LPC code from a string and return an eval object

#### SYNOPSIS
```c
object compile_string(string code);
```

#### DESCRIPTION
Compiles LPC code from a string and returns a temporary eval object with the compiled code installed. This enables dynamic code execution, interactive development, and runtime code generation.

The code is wrapped in a temporary prototype object that can have its functions called normally. The object should be destructed when no longer needed to free resources.

Security limits:
- Maximum code length: 10KB
- Compilation cycle limit: 50,000
- Only privileged objects can call this (check may be disabled for development)

The eval object's `init()` function is automatically called after compilation if it exists.

**Returns**: Eval object on success, 0 on compilation failure.

#### EXAMPLE
```c
// Evaluate an expression
object eval_obj = compile_string("eval() { return 5 + 3; }");
int result = eval_obj.eval();  // Returns 8
destruct(eval_obj);

// Execute statements
string code = "test() { write(\"Hello from eval!\\n\"); }";
eval_obj = compile_string(code);
eval_obj.test();
destruct(eval_obj);

// Interactive calculator
void calculate(string expr) {
    string code = "result() { return " + expr + "; }";
    object eval = compile_string(code);
    if (eval) {
        write("Result: " + eval.result() + "\n");
        destruct(eval);
    } else {
        write("Syntax error\n");
    }
}
```

#### SEE ALSO
compile_object(3), destruct(3), eval (admin command)

---

### command

#### NAME
command()  -  execute a command programmatically

#### SYNOPSIS
```c
int command(string cmd);
```

#### DESCRIPTION
Executes a command string as if the current object (usually a player) had typed it. The command goes through normal command processing, including verb matching and argument parsing.

This is useful for implementing aliases, macros, scripted actions, or AI behavior where you want to trigger commands programmatically.

**Returns**: The result of command execution (implementation-specific).

#### EXAMPLE
```c
// Implement an alias
void do_l(string args) {
    command("look " + args);
}

// Scripted NPC behavior
void npc_routine() {
    command("say Hello, traveler!");
    command("emote waves");
}

// Macro system
void execute_macro(string macro_name) {
    mapping macros = query_macros();
    if (member(macros, macro_name)) {
        mixed *commands = macros[macro_name];
        foreach (string cmd in commands) {
            command(cmd);
        }
    }
}

// Auto-actions
void auto_loot() {
    command("get all from corpse");
}
```

#### SEE ALSO
call_other(3), this_player(3)

---

### syswrite

#### NAME
syswrite()  -  write debug output to syswrite.txt

#### SYNOPSIS
```c
void syswrite(string msg);
```

#### DESCRIPTION
Writes a message to the `syswrite.txt` file with automatic context information (calling object, timestamp, etc.). This is a debugging and testing utility—use it to track execution flow, log test results, or debug complex interactions.

Unlike `syslog()` which is for production logging, `syswrite()` is specifically for development and testing.

**Returns**: Nothing (void).

#### EXAMPLE
```c
// Debug output
syswrite("Entering init() function");

// Track execution
void complex_function() {
    syswrite("Starting complex operation");
    // ... do work ...
    syswrite("Complex operation complete");
}

// Test output
void test_feature() {
    syswrite("TEST: Testing feature X");
    int result = do_something();
    syswrite("TEST: Result = " + itoa(result));
}

// Log state changes
void set_state(string new_state) {
    syswrite("State change: " + query_state() + " -> " + new_state);
    state = new_state;
}
```

#### SEE ALSO
syslog(3), write(3)

---

### set_interactive

#### NAME
set_interactive()  -  mark an object as interactive

#### SYNOPSIS
```c
void set_interactive(int enabled);
```

#### DESCRIPTION
Marks the current object as interactive (connected to a player) or non-interactive. Interactive objects can receive input from a player connection.

This is typically called during login when a connection object becomes associated with a player character.

**Returns**: Nothing (void).

#### EXAMPLE
```c
// During login
void become_player() {
    set_interactive(1);
    write("You are now interactive!\n");
}

// Disconnect
void logout() {
    set_interactive(0);
}
```

#### SEE ALSO
interactive(3), connected(3), this_player(3)

---

### interactive

#### NAME
interactive()  -  check if an object is interactive

#### SYNOPSIS
```c
int interactive(object obj);
```

#### DESCRIPTION
Tests whether an object is marked as interactive (has the interactive flag set). Interactive objects are typically player characters or connection objects that can receive player input.

This doesn't necessarily mean the object is currently connected—use `connected()` for that.

**Returns**: 1 if the object is interactive, 0 if not.

#### EXAMPLE
```c
// Check if object is a player
if (interactive(obj)) {
    write("This is a player object\n");
}

// Send message only to interactive objects
void broadcast(string msg) {
    object player = next_who(0);
    while (player) {
        if (interactive(player)) {
            player->receive_message(msg);
        }
        player = next_who(player);
    }
}

// Filter for players
mixed *get_players_in_room(object room) {
    mixed *players = ({ });
    object obj = contents(room);
    while (obj) {
        if (interactive(obj)) {
            players += ({ obj });
        }
        obj = next_object(obj);
    }
    return players;
}
```

#### SEE ALSO
set_interactive(3), connected(3), next_who(3)

---

### connected

#### NAME
connected()  -  check if an object has an active network connection

#### SYNOPSIS
```c
int connected(object obj);
```

#### DESCRIPTION
Tests whether an object has an active network connection (device). An object can be interactive but not connected (e.g., after link-death), or connected but not interactive (e.g., during login).

Use this to check if you can actually send output to a player, or to detect link-dead players.

**Returns**: 1 if the object has an active connection, 0 if not.

#### EXAMPLE
```c
// Check before sending output
if (connected(player)) {
    player->receive_message("Hello!\n");
} else {
    // Player is link-dead, queue message
    queue_message(player, "Hello!\n");
}

// Find link-dead players
void check_linkdead() {
    object player = next_who(0);
    while (player) {
        if (interactive(player) && !connected(player)) {
            write(player->query_name() + " is link-dead\n");
        }
        player = next_who(player);
    }
}

// Auto-logout after timeout
void check_idle_disconnect() {
    if (!connected(this_object())) {
        if (time() - query_last_connected() > 300) {
            destruct(this_object());
        }
    }
}
```

#### SEE ALSO
interactive(3), get_devconn(3), get_devidle(3)

# Simulated Efuns

Common utilities provided by `/sys/auto.c` that are automatically available to all objects. These are written in NLPC and build on top of the driver efuns to provide higher-level functionality.

## Detailed Simulated Efuns Reference

### find_object

#### NAME
find_object()  -  locate an object by name, path, id, or context

#### SYNOPSIS
object find_object(string name);

#### DESCRIPTION
Smart lookup supporting:
- "me" => this_player()
- "/path" or "#num" => absolute object reference
- "*name" => player by name (TODO placeholder)
- plain name => present() in player inventory or environment
- "here" => location(this_player())

#### EXAMPLE
```c
object o = find_object("/boot");
object me = find_object("me");
```

#### SEE ALSO
present(3), atoo(3), otoa(3)

---

### present

#### NAME
present()  -  find an object by id in a container

#### SYNOPSIS
object present(string name, object container);

#### DESCRIPTION
Returns the first object in container's inventory whose `id(name)` returns true.

#### EXAMPLE
```c
object ring = present("ring", this_player());
```

#### SEE ALSO
find_object(3)

---

### compile_object (sefun)

#### NAME
compile_object()  -  compile an object via boot wrapper

#### SYNOPSIS
object compile_object(string path);

#### DESCRIPTION
Delegates to `/boot.compile_object`. Returns the prototype object or 0.

#### EXAMPLE
```c
object proto = compile_object("/world/room.c");
```

#### SEE ALSO
get_object(3), clone_object(3), compile_object(efun)

---

### get_object (sefun)

#### NAME
get_object()  -  ensure a prototype exists and return it

#### SYNOPSIS
object get_object(string path);

#### DESCRIPTION
Delegates to `/boot.get_object`. Compiles the path if not already compiled, then returns the prototype.

#### EXAMPLE
```c
object proto = get_object("/sys/user.c");
```

#### SEE ALSO
compile_object(3), clone_object(3)

---

### clone_object (sefun)

#### NAME
clone_object()  -  clone an object via boot wrapper

#### SYNOPSIS
object clone_object(string|object x);

#### DESCRIPTION
If x is a string path, clones that path. If x is an object, clones its program. Uses `/boot.clone`.

#### EXAMPLE
```c
object room = clone_object("/world/room.c");
```

#### SEE ALSO
compile_object(3), get_object(3)

---

### capitalize

#### NAME
capitalize()  -  uppercase the first character of a string

#### SYNOPSIS
```c
string capitalize(string s);
```

#### DESCRIPTION
Returns a new string with the first character converted to uppercase and the rest unchanged. Useful for formatting names, titles, or sentence beginnings.

**Returns**: String with first character uppercased.

#### EXAMPLE
```c
// Capitalize names
string name = capitalize("gandalf");  // "Gandalf"

// Format player names
void greet_player(string name) {
    write("Welcome, " + capitalize(name) + "!\n");
}

// Capitalize first word
string title = capitalize("the lord of the rings");
// "The lord of the rings"
```

#### SEE ALSO
upcase(3), downcase(3)

---

### contains

#### NAME
contains()  -  check if an array contains an element

#### SYNOPSIS
```c
int contains(*arr, elem);
```

#### DESCRIPTION
Returns 1 if the element is found in the array, 0 otherwise. This is a convenience wrapper around `member_array()` that returns a boolean instead of an index.

**Returns**: 1 if element is in array, 0 if not.

#### EXAMPLE
```c
// Check for item
*inventory = ({ "sword", "shield", "potion" });
if (contains(inventory, "sword")) {
    write("You have a sword!\n");
}

// Validate input
int is_valid_direction(string dir) {
    *valid = ({ "north", "south", "east", "west" });
    return contains(valid, dir);
}

// Check permissions
if (contains(admin_list, player_name)) {
    grant_admin_access();
}
```

#### SEE ALSO
member_array(3), sizeof(3)

---

### remove_element

#### NAME
remove_element()  -  remove all occurrences of an element from an array

#### SYNOPSIS
```c
*remove_element(*arr, elem);
```

#### DESCRIPTION
Returns a new array with all occurrences of the specified element removed. The original array is unchanged.

Unlike removing by index, this removes **all** matching elements, no matter where they appear in the array.

**Returns**: New array with element removed.

#### EXAMPLE
```c
// Remove duplicates
*numbers = ({ 1, 2, 2, 3, 2, 4 });
*clean = remove_element(numbers, 2);
// ({ 1, 3, 4 })

// Remove player from list
*players = ({ "alice", "bob", "charlie" });
players = remove_element(players, "bob");
// ({ "alice", "charlie" })

// Clean up inventory
void drop_all(string item_name) {
    *inv = query_inventory();
    inv = remove_element(inv, item_name);
    set_inventory(inv);
}
```

#### SEE ALSO
member_array(3), contains(3), unique_array(3)

---

### tell

#### NAME
tell()  -  send a message to a specific player

#### SYNOPSIS
```c
void tell(object player, string msg);
```

#### DESCRIPTION
Sends a message to a specific player by calling their `listen()` function. This is the standard way to send output to a player object.

Use this when you need to send a message to a specific player, as opposed to `write()` which sends to `this_player()`.

**Returns**: Nothing (void).

#### EXAMPLE
```c
// Send to specific player
object target = find_player("gandalf");
tell(target, "You receive a message.\n");

// Notify all players in room
void tell_room(object room, string msg) {
    object obj = contents(room);
    while (obj) {
        if (interactive(obj)) {
            tell(obj, msg);
        }
        obj = next_object(obj);
    }
}

// Private message
void whisper(object from, object to, string msg) {
    tell(to, from->query_name() + " whispers: " + msg + "\n");
    tell(from, "You whisper to " + to->query_name() + ": " + msg + "\n");
}
```

#### SEE ALSO
write(3), this_player(3)

---

### write (sefun)

#### NAME
write()  -  send a message to this_player() or syswrite

#### SYNOPSIS
void write(string msg);

#### DESCRIPTION
Calls `listen(msg)` on this_player() if present, else falls back to `syswrite(msg)`.

#### EXAMPLE
```c
write("Saved.\n");
```

#### SEE ALSO
syswrite(3), tell(3)

---

### set_heart_beat

#### NAME
set_heart_beat()  -  start a periodic heartbeat timer

#### SYNOPSIS
```c
void set_heart_beat(int interval);
```

#### DESCRIPTION
Starts a periodic timer that calls `do_heart_beat()` every `interval` seconds. The `do_heart_beat()` function then calls your object's `heart_beat()` function if it exists.

This is the standard way to implement periodic actions like NPC AI, regeneration, environmental effects, or any recurring behavior.

To stop the heartbeat, call `remove_alarm()` on the scheduled alarm.

**Returns**: Nothing (void).

#### EXAMPLE
```c
// Start heartbeat in init
void init() {
    set_heart_beat(5);  // Every 5 seconds
}

// Implement heart_beat
void heart_beat() {
    // This gets called every 5 seconds
    regenerate_hp();
    check_environment();
}

// NPC behavior
void init() {
    set_heart_beat(10);  // Every 10 seconds
}

void heart_beat() {
    if (random(2) == 0) {
        do_emote("looks around");
    }
    if (query_hp() < query_max_hp() / 2) {
        flee();
    }
}
```

#### SEE ALSO
do_heart_beat(3), alarm(3), remove_alarm(3)

---

### do_heart_beat

#### NAME
do_heart_beat()  -  heartbeat callback function

#### SYNOPSIS
```c
void do_heart_beat();
```

#### DESCRIPTION
This is the callback function scheduled by `set_heart_beat()`. It calls your object's `heart_beat()` function if it exists, then reschedules itself.

You don't normally call this directly - it's called automatically by the alarm system. Just implement `heart_beat()` in your object and call `set_heart_beat()` to start it.

**Returns**: Nothing (void).

#### EXAMPLE
```c
// You implement this:
void heart_beat() {
    // Your periodic code here
}

// set_heart_beat() schedules do_heart_beat()
// do_heart_beat() calls your heart_beat()
// Then reschedules itself
```

#### SEE ALSO
set_heart_beat(3), alarm(3)

---

# Applies / Callbacks

## Master Object

### connect

#### NAME
connect()  -  called by the driver when a new connection is established

#### SYNOPSIS
```c
void connect();
```

#### DESCRIPTION
Called on the boot object (`/boot.c`) when a new network connection is established. This is the entry point for handling new player connections.

The boot object should create a user/connection object and associate it with the connection. This is where the login process begins.

**Called on**: `/boot.c` (boot object)  
**When**: Immediately after a TCP connection is accepted  
**Arguments**: None

#### EXAMPLE
```c
// In /boot.c
void connect() {
    object user;
    
    // Create a new user connection object
    user = clone_object("/sys/user");
    
    // The user object will handle login
    user->start_login();
}
```

#### SEE ALSO
disconnect(3)

---

### disconnect

#### NAME
disconnect()  -  called by the driver when a connection closes

#### SYNOPSIS
```c
void disconnect();
```

#### DESCRIPTION
Called on the object associated with a connection when that connection is closed (either by the client disconnecting or by the server closing it).

Use this to clean up the player's state, save their data, notify other players, or handle link-death recovery.

**Called on**: The object associated with the connection (usually a user or player object)  
**When**: When the TCP connection closes  
**Arguments**: None

#### EXAMPLE
```c
// In user.c or player.c
void disconnect() {
    object player_body;
    
    // Save player data
    save_player();
    
    // Notify others
    if (player_body = query_player()) {
        tell_room(location(player_body), 
                  player_body->query_name() + " has disconnected.\n");
    }
    
    // Clean up
    if (player_body) {
        destruct(player_body);
    }
}
```

#### SEE ALSO
connect(3), connected(3)

---

### valid_read

#### NAME
valid_read()  -  authorize file read operations

#### SYNOPSIS
```c
int valid_read(string path, string operation, object caller, int owner, int flags);
```

#### DESCRIPTION
**Master object security callback** - Called on the master object (first loaded object, typically `/boot.c`) to authorize file read operations. The driver calls this before allowing any file read operation.

**Parameters**:
- `path` - The file path being accessed
- `operation` - The operation being performed (e.g., "fread", "cat", "ls")
- `caller` - The object requesting the read
- `owner` - The numeric ID of the file's owner (-1 if file doesn't exist)
- `flags` - File flags (permissions, directory bit, etc., -1 if file doesn't exist)

**Returns**: 1 to allow the operation, 0 to deny it.

**Implementation Note**: Currently, if this function doesn't exist in the master object, all reads are allowed by default. In the future, the driver will enforce this callback more strictly.

#### EXAMPLE
```c
// In /boot.c (master object)
int valid_read(string path, string operation, object caller, int owner, int flags) {
    object caller_obj = caller;
    int caller_priv;
    
    // Master object can read anything (prevent recursion)
    if (caller_obj == this_object()) {
        return 1;
    }
    
    // Get caller's privilege level
    caller_priv = caller_obj->query_priv();
    
    // Admins can read anything
    if (caller_priv >= PRIV_ADMIN) {
        return 1;
    }
    
    // /data directory is restricted
    if (instr(path, 1, "/data/") == 1) {
        // Only system objects and admins
        if (caller_priv >= PRIV_WIZARD) {
            return 1;
        }
        return 0;
    }
    
    // Everyone can read /world, /doc, /include
    if (instr(path, 1, "/world/") == 1 ||
        instr(path, 1, "/doc/") == 1 ||
        instr(path, 1, "/include/") == 1) {
        return 1;
    }
    
    // Wizards can read their own directories
    if (caller_priv >= PRIV_WIZARD) {
        string wiz_dir = "/wiz/" + caller_obj->query_name() + "/";
        if (instr(path, 1, wiz_dir) == 1) {
            return 1;
        }
    }
    
    // Default: allow
    return 1;
}
```

#### SEE ALSO
valid_write(3), priv(3), set_priv(3)

---

### valid_write

#### NAME
valid_write()  -  authorize file write operations

#### SYNOPSIS
```c
int valid_write(string path, string operation, object caller, int owner, int flags);
```

#### DESCRIPTION
**Master object security callback** - Called on the master object to authorize file write operations (including create, modify, delete, rename). The driver calls this before allowing any file modification.

**Parameters**:
- `path` - The file path being accessed
- `operation` - The operation being performed (e.g., "fwrite", "mkdir", "rm", "mv")
- `caller` - The object requesting the write
- `owner` - The numeric ID of the file's owner (-1 if file doesn't exist)
- `flags` - File flags (permissions, directory bit, etc., -1 if file doesn't exist)

**Returns**: 1 to allow the operation, 0 to deny it.

**Implementation Note**: Currently, if this function doesn't exist in the master object, all writes are allowed by default. In the future, the driver will enforce this callback more strictly.

#### EXAMPLE
```c
// In /boot.c (master object)
int valid_write(string path, string operation, object caller, int owner, int flags) {
    object caller_obj = caller;
    int caller_priv;
    
    // Master object can write anything (prevent recursion)
    if (caller_obj == this_object()) {
        return 1;
    }
    
    // Get caller's privilege level
    caller_priv = caller_obj->query_priv();
    
    // Admins can write anything
    if (caller_priv >= PRIV_ADMIN) {
        return 1;
    }
    
    // Nobody can write to /sys (system files)
    if (instr(path, 1, "/sys/") == 1) {
        if (caller_priv >= PRIV_ADMIN) {
            return 1;
        }
        return 0;
    }
    
    // /data directory - only system objects
    if (instr(path, 1, "/data/") == 1) {
        if (caller_priv >= PRIV_WIZARD) {
            return 1;
        }
        return 0;
    }
    
    // Wizards can write to their own directories
    if (caller_priv >= PRIV_WIZARD) {
        string wiz_dir = "/wiz/" + caller_obj->query_name() + "/";
        if (instr(path, 1, wiz_dir) == 1) {
            return 1;
        }
        
        // Wizards can also write to /world
        if (instr(path, 1, "/world/") == 1) {
            return 1;
        }
    }
    
    // Players can write to /tmp
    if (instr(path, 1, "/tmp/") == 1) {
        return 1;
    }
    
    // Default: deny
    return 0;
}
```

#### SEE ALSO
valid_read(3), priv(3), set_priv(3)

---

## Object Lifecycle

### init

#### NAME
init()  -  initialize a newly created object

#### SYNOPSIS
```c
void init();
```

#### DESCRIPTION
Called automatically by the driver immediately after an object is cloned or compiled. This is where you initialize your object's state, set up variables, and perform any setup that needs to happen when the object is created.

For the auto object (`/sys/auto.c`), `init()` is called after compilation. For all other objects, it's called after cloning.

**Called on**: Any object after creation  
**When**: Immediately after `clone_object()` or after compilation (for auto object)  
**Arguments**: None

#### EXAMPLE
```c
// Basic initialization
void init() {
    set_name("goblin");
    set_description("A nasty looking goblin.");
    set_hp(50);
}

// Call parent init
inherit "/inherits/living";

void init() {
    ::init();  // Call parent's init first
    
    // Then do our own setup
    set_race("goblin");
    set_level(5);
}

// Conditional initialization
void init() {
    if (prototype(this_object())) {
        // This is the prototype, don't initialize
        return;
    }
    
    // This is a clone, initialize it
    set_unique_id(time() + random(10000));
}
```

#### SEE ALSO
clone_object(3), prototype(3)

---

### allow_attach

#### NAME
allow_attach()  -  control whether this object can be attached to

#### SYNOPSIS
```c
int allow_attach();
```

#### DESCRIPTION
Called by the driver when another object attempts to attach to this object using `attach()`. Return 1 to allow the attachment, 0 to deny it.

If this function doesn't exist, attachments are allowed by default.

Use this to implement security policies, prevent certain objects from being attached, or enforce attachment rules.

**Called on**: The object being attached to  
**When**: Before `attach()` completes  
**Arguments**: None  
**Returns**: 1 to allow, 0 to deny

#### EXAMPLE
```c
// Only allow system objects to attach
int allow_attach() {
    object caller = caller_object();
    
    if (!caller) {
        return 0;  // No caller? Deny
    }
    
    // Check if caller is privileged
    if (caller->query_priv() >= PRIV_WIZARD) {
        return 1;
    }
    
    return 0;
}

// Allow specific objects only
int allow_attach() {
    object caller = caller_object();
    string caller_path = otoa(caller);
    
    // Only allow specific component types
    if (instr(caller_path, 1, "/components/") == 1) {
        return 1;
    }
    
    return 0;
}

// Always allow
int allow_attach() {
    return 1;
}
```

#### SEE ALSO
attach(3), detach(3), caller_object(3)

---

### listen

#### NAME
listen()  -  receive data from system queries

#### SYNOPSIS
```c
void listen(string data);
```

#### DESCRIPTION
Called by various system commands (like `sysctl()`) to deliver query results. The specific format of `data` depends on which system operation triggered the call.

For example, `sysctl(3)` (alarm query) calls `listen()` on the target object with information about each pending alarm.

**Called on**: The object specified in the system query  
**When**: During system information queries  
**Arguments**: `data` - formatted string with query results

#### EXAMPLE
```c
// Handle alarm list from sysctl
void listen(string data) {
    // data format: "objid funcname delay\n"
    // Example: "123 heartbeat 5\n"
    
    write("Alarm: " + data);
}

// Parse and process
void listen(string data) {
    string obj_id, func_name;
    int delay;
    
    if (sscanf(data, "%s %s %d", obj_id, func_name, delay) == 3) {
        write("Object " + obj_id + " has alarm '" + 
              func_name + "' in " + itoa(delay) + " seconds\n");
    }
}
```

#### SEE ALSO
sysctl(3)

---

# Notes & Caveats

## Important Behavioral Notes

### Object Iteration
- `contents()` and `next_object()` form a linked-list iterator, **not** an array
- Always iterate with: `obj = contents(container); while(obj) { ...; obj = next_object(obj); }`
- Do not modify the contents list while iterating (save changes for after the loop)

### Movement and Inventory
- `move_object()` is a **low-level** efun that does NOT call policy hooks
- Use mudlib's `move()` function (in object.c) which calls `receive_object()` and `release_object()` hooks
- The mudlib uses a hybrid system: fast array tracking + driver's linked list for safety

### String Handling
- Empty strings are represented as **INTEGER 0** in NLPC
- All string efuns handle INTEGER 0 as empty string
- Use `strlen(str) == 0` to check for empty, not `str == ""`
- Strings are **immutable** - operations create new strings

### Global Storage
- `table_set()`, `table_get()`, `table_delete()` use **process-global** storage
- Always namespace your keys: `table_set(otoa(this_object()) + "_state", value)`
- Not per-object - shared across entire MUD process

### Type System
- NLPC does **not** have a `mixed` keyword (used in docs for clarity only)
- Types are flexible - you can omit type declarations
- Use `typeof()` to check types at runtime
- Array and mapping literals (`({ })` and `([ ])`) are language syntax, not efuns

### Security
- `valid_read()` and `valid_write()` are called on the **master object** (boot.c)
- Currently optional - if not defined, operations are allowed by default
- Future versions will enforce these more strictly

### Inheritance
- `inherit` statements must be at the **top of the file** before any variables or functions
- Use `::function()` to call parent version of overridden functions
- Variable name conflicts between parents cause compilation errors

---
