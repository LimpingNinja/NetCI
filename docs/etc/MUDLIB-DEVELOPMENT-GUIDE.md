# NetCI Mudlib Development Guide

**For NLPC Developers Building Mudlibs**

This guide provides comprehensive documentation for building mudlibs (MUD libraries) using NLPC. It covers both technical foundations (how the system works) and practical implementation (how to build a working mudlib from scratch).

---

## Table of Contents

**PART I: TECHNICAL FOUNDATIONS**

1. [Introduction](#1-introduction)
2. [Object System Deep Dive](#2-object-system-deep-dive)
3. [Boot Object Requirements](#3-boot-object-requirements)
4. [Connection Handling](#4-connection-handling)
5. [Security Policies](#5-security-policies)
6. [Object Hierarchies](#6-object-hierarchies)

**PART II: PRACTICAL GUIDE**

7. [Building a Skeleton Mudlib](#7-building-a-skeleton-mudlib)
8. [Command System](#8-command-system)
9. [Best Practices](#9-best-practices)
10. [Testing and Debugging](#10-testing-and-debugging)
11. [Advanced Topics](#11-advanced-topics)
12. [Complete Examples](#12-complete-examples)

**Appendices:**
- [Appendix A: Required Functions Reference](#appendix-a-required-functions-reference)
- [Appendix B: System Calls Reference](#appendix-b-system-calls-reference)
- [Appendix C: Quick Start Checklist](#appendix-c-quick-start-checklist)
- [Appendix D: Glossary](#appendix-d-glossary)

---

# PART I: TECHNICAL FOUNDATIONS

## 1. Introduction

### 1.1 About This Guide

This guide is for developers who want to build mudlibs (MUD libraries) using NLPC. You'll learn:

- How the NetCI object system works
- What the boot object must provide
- How to handle player connections
- How to build a complete mudlib from scratch
- Best practices and patterns

**Prerequisites:**
- Basic NLPC syntax knowledge
- Understanding of object-oriented concepts
- Familiarity with MUD concepts (rooms, players, objects)

**What is a Mudlib?**

A mudlib is the application layer that runs on top of the NetCI engine. It provides:
- The boot object (master object) that initializes the system
- Login and authentication
- Player objects and commands
- Room and object hierarchies
- Security policies
- Game-specific functionality

```
┌─────────────────────────────────────┐
│   Your Mudlib (NLPC Code)          │  ← What you build
├─────────────────────────────────────┤
│   NetCI Engine (C Code)            │  ← Provided by NetCI
└─────────────────────────────────────┘
```

### 1.2 NLPC Language Primer

NLPC (NetCI LPC) is inspired by LPC (Lars Pensjö C), the language created for LPMud in 1989.

**Basic Syntax:**
```c
// Data types
int count;           // Integer
string name;         // String
object player;       // Object reference

// Control structures
if (condition) {
  // code
} else {
  // code
}

while (condition) {
  // code
}

// Functions
static my_function(arg1, arg2) {
  return arg1 + arg2;
}

// Object operations
obj = new("/obj/player");        // Create instance
obj.set_name("Bob");             // Call function
ref = atoo("/obj/player#15");    // Get by reference
destruct(obj);                   // Destroy object
```

**Key Concepts:**
- **Prototypes**: Original objects defined by source files (e.g., `/obj/player`)
- **Instances**: Clones of prototypes with unique reference numbers (e.g., `/obj/player#15`)
- **Shared Objects**: Objects attached to provide common functionality
- **Static functions**: Internal to object (not callable from outside)
- **Public functions**: Callable from other objects

### 1.3 Architecture Overview

```
TCP Connection
      │
      ▼
NetCI Engine (intrface.c)
      │
      ▼
boot.c::connect()
      │
      ▼
/sys/login.c
      │
      ▼
/obj/player.c
      │
      ▼
Game World (rooms, objects, NPCs)
```

**Responsibilities:**

**NetCI Engine (C code):**
- TCP/IP networking
- NLPC interpreter
- Database management
- System calls (file I/O, object management, etc.)

**Mudlib (NLPC code):**
- Boot object (master object)
- Connection handling
- Authentication
- Player commands
- Game logic
- Security policies

---

## 2. Object System Deep Dive

### 2.1 Objects in NetCI

#### 2.1.1 Prototypes vs Instances

**Prototypes** are the original objects defined by source code files:
- Created by compiling source files
- Serve as templates for instances
- Have `prototype(this_object())` return `1`
- Example: `/obj/player`

**Instances** are clones of prototypes:
- Created with `new(prototype_path)`
- Have unique reference numbers
- Inherit functions from prototype
- Example: `/obj/player#15`

**Checking:**
```c
if (prototype(this_object())) {
  write("I am a prototype\n");
} else {
  write("I am an instance\n");
}
```

#### 2.1.2 Object Creation

**Creating a Prototype (Compilation):**
```c
compile_object("/obj/weapon");
```

This:
1. Reads `/obj/weapon.c` from the virtual filesystem
2. Parses and validates the NLPC code
3. Generates bytecode
4. Stores in database
5. Creates the prototype object `/obj/weapon`

**Creating an Instance (Cloning):**
```c
object sword = new("/obj/weapon");
```

This:
1. Finds the prototype `/obj/weapon`
2. Creates a copy with a unique reference number
3. Calls the `init()` function
4. Returns the new instance (e.g., `/obj/weapon#42`)

#### 2.1.3 Object Storage in Database

Objects are stored in the NetCI database with:
- **Reference number**: Unique ID (e.g., `15`)
- **Prototype path**: Parent object (e.g., `/obj/player`)
- **Properties**: Name, description, flags, etc.
- **Variables**: All object variables and their values
- **Location**: Container object reference

**Object References:**
```c
object player = atoo("/obj/player#15");  // Get by full path
int ref = otoi(player);                  // Get reference number (15)
string path = otoa(player);              // Get full path ("/obj/player#15")
```

### 2.2 Compilation and Updates

#### 2.2.1 How Compilation Works

```
Source File (/obj/weapon.c)
      ↓
compile_object("/obj/weapon")
      ↓
Parse & Validate Syntax
      ↓
Generate Bytecode
      ↓
Store in Database
      ↓
Prototype Object (/obj/weapon)
```

**Important:** Compilation creates or updates the prototype, not instances.

#### 2.2.2 Updating Prototypes

**Critical Question: What happens to existing instances when you recompile?**

```c
// You have instances
object sword1 = new("/obj/weapon");
object sword2 = new("/obj/weapon");

// You recompile the prototype
compile_object("/obj/weapon");

// What happens to sword1 and sword2?
```

**Answer:**
- ✅ Instances are **NOT** automatically updated
- ✅ Instances keep their old code
- ✅ New instances get new code
- ❌ Must manually update existing instances

**Why This Matters:**
- Bug fixes don't automatically apply to existing objects
- New features don't appear in old instances
- Must implement update mechanisms

#### 2.2.3 Manual Update Pattern

**Option 1: Update Function**
```c
// In your object
void update() {
  object new_proto = load_object(prototype_path(this_object()));
  
  // Strategy 1: Call sync() to reload
  sync();
  
  // Strategy 2: Copy new functions manually
  // (Implementation depends on your needs)
}
```

**Option 2: Update All Instances**
```c
// Utility function to update all instances of a prototype
void update_all_instances(string prototype_path) {
  object curr = first_object();
  
  while (curr) {
    if (prototype_path(curr) == prototype_path && !prototype(curr)) {
      // This is an instance of the prototype
      if (function_exists("update", curr)) {
        curr.update();
      }
    }
    curr = next_object(curr);
  }
}

// Usage
update_all_instances("/obj/player");
```

**Option 3: Destruct and Recreate**
```c
// For non-player objects, sometimes easier to recreate
void refresh_object(object old_obj) {
  string proto = prototype_path(old_obj);
  object location = get_link(old_obj);
  
  // Create new instance
  object new_obj = new(proto);
  
  // Copy important properties
  new_obj.set_name(old_obj.get_name());
  new_obj.set_desc(old_obj.get_desc());
  
  // Move to same location
  move_object(new_obj, location);
  
  // Destroy old
  destruct(old_obj);
}
```

### 2.3 The attach() System

NLPC uses `attach()` instead of traditional inheritance. This is a key difference from other LPC implementations.

#### 2.3.1 What attach() Does

`attach()` makes another object's functions available to your object:

```c
// Create shared object
object sharobj = new("/obj/share/common");

// Attach it
attach(sharobj);

// Now can call functions from common.c
set_name("Bob");  // Actually calls sharobj.set_name("Bob")
get_name();       // Actually calls sharobj.get_name()
```

**How It Works:**
1. When you call a function that doesn't exist in your object
2. NetCI checks attached objects
3. If found, calls the function in the attached object
4. The function runs in the attached object's context

#### 2.3.2 attach() vs inherit

**Traditional LPC inherit:**
```c
// base_living.c
int hp;
void set_hp(int amount) { hp = amount; }

// player.c
inherit "/obj/base_living";
void create() {
  set_hp(100);  // Runs in player.c context, sets player's hp
}
```

**NLPC attach:**
```c
// /obj/share/living.c
int hp;
void set_hp(int amount) { hp = amount; }

// /obj/player.c
object sharobj;
void init() {
  sharobj = new("/obj/share/living");
  attach(sharobj);
  set_hp(100);  // Runs in sharobj context, sets sharobj's hp!
}
```

**Key Difference:**
- `inherit`: Code runs in child object's context
- `attach()`: Code runs in attached object's context

#### 2.3.3 Context and Scope

**Critical Understanding:**

When you call an attached function:
- `this_object()` returns the **attached object**, not your object
- Variables accessed are in the **attached object**
- Functions called are in the **attached object**

**Example:**
```c
// /obj/share/common.c
string name;

void set_name(string n) {
  name = n;  // Sets common.c's name variable
}

string get_name() {
  return name;  // Returns common.c's name variable
}

// /obj/player.c
object sharobj;
string player_name;  // Different variable!

void init() {
  sharobj = new("/obj/share/common");
  attach(sharobj);
  
  set_name("Bob");  // Sets sharobj.name, not player_name
  
  write(get_name());  // Prints "Bob" (from sharobj)
  write(player_name); // Prints "" (empty, never set)
}
```

**Workaround Pattern:**

To access your object from attached code, pass `this_object()`:

```c
// /obj/share/common.c
void set_name_on(object target, string n) {
  // Now we can set the target's variable
  target->name = n;
}

// /obj/player.c
string name;
object sharobj;

void init() {
  sharobj = new("/obj/share/common");
  attach(sharobj);
  
  set_name_on(this_object(), "Bob");  // Sets player's name
}
```

#### 2.3.4 Multiple attach() Calls

You can attach multiple objects:

```c
object sharobj1, sharobj2;

void init() {
  sharobj1 = new("/obj/share/common");
  attach(sharobj1);
  
  sharobj2 = new("/obj/share/living");
  attach(sharobj2);
  
  // Can call functions from both
  set_name("Bob");      // From common.c
  set_hp(100);          // From living.c
}
```

**Function Resolution Order:**
1. Check local object
2. Check attached objects in reverse order (last attached first)
3. If not found, error

**Name Conflicts:**
If multiple attached objects have the same function, the last attached wins:

```c
attach(sharobj1);  // Has set_name()
attach(sharobj2);  // Also has set_name()

set_name("Bob");  // Calls sharobj2.set_name(), not sharobj1
```

#### 2.3.5 Common Shared Objects

**`/obj/share/common.c`** - Basic object functionality:
- Name and description management
- Flag management
- Contents and location management
- Basic examination

**`/obj/share/living.c`** - Living being functionality:
- Gender system
- Communication helpers
- Living-specific functions

**Pattern:**
```c
// Most objects use common
object sharobj;

void init() {
  sharobj = new("/obj/share/common");
  attach(sharobj);
  set_type(TYPE_ROOM);  // or TYPE_OBJECT, TYPE_PLAYER
}

// Players and NPCs also use living
object sharobj, sharobj2;

void init() {
  sharobj = new("/obj/share/common");
  attach(sharobj);
  
  sharobj2 = new("/obj/share/living");
  attach(sharobj2);
  
  set_type(TYPE_PLAYER);
}
```

---

## 3. Boot Object Requirements

The boot object (usually `/boot.c`) is the master object that initializes and manages the system. It's always object #0 and has special privileges.

### 3.1 The Master Object

**What is the Master Object?**
- The first object loaded by NetCI
- Always has reference number 0
- Has UID "root" (unrestricted access)
- Manages system initialization
- Handles new connections
- Implements security policies

**Special Privileges:**
- Can access any file
- Can modify any object
- Can grant privileges to other objects
- Bypasses security checks

### 3.2 Required Callbacks

The boot object **MUST** implement these functions for the system to work:

#### 3.2.1 connect()

**Called when:** A TCP connection is established

**Signature:**
```c
static connect()
```

**Purpose:** Create and transfer control to a login object

**Responsibilities:**
1. Create a login object
2. Mark it as secure (optional but recommended)
3. Transfer the connection to it
4. Grant privileges if needed
5. Start the login process

**Complete Implementation:**
```c
static connect() {
  object login_obj;
  
  // Create login object
  login_obj = new("/sys/login");
  if (!login_obj) {
    send_device("Unable to create login object\n");
    disconnect_device();
    return;
  }
  
  // Mark as secure (prevents tampering)
  login_obj.set_secure();
  
  // Transfer connection
  if (reconnect_device(login_obj)) {
    send_device("Unable to transfer connection\n");
    disconnect_device();
    destruct(login_obj);
    return;
  }
  
  // Grant privileges (allows login object to create players)
  set_priv(login_obj, 1);
  
  // Start login process
  login_obj.connect();
}
```

**Key Points:**
- Must create a new object for each connection
- Must use `reconnect_device()` to transfer connection
- Login object needs `listen()` function to receive input
- Error handling is critical (connection might fail)

**Common Mistakes:**
- ❌ Forgetting to call `login_obj.connect()` at the end
- ❌ Not checking if `new()` succeeded
- ❌ Not handling `reconnect_device()` failure
- ❌ Forgetting to grant privileges

#### 3.2.2 valid_read(path, func, caller, owner, flags)

**Called when:** Any read operation on the virtual filesystem

**Signature:**
```c
int valid_read(string path, string func, object caller, int owner, int flags)
```

**Parameters:**
- `path` - Virtual filesystem path being accessed (e.g., "/home/wizard/test.c")
- `func` - Operation name (e.g., "read_file", "stat", "get_dir", "fread")
- `caller` - Object attempting the operation (NULL for system operations)
- `owner` - Object reference number of file owner (-1 if doesn't exist)
- `flags` - File permission flags (-1 if file doesn't exist)

**Returns:**
- `1` - Allow the operation
- `0` - Deny the operation

**Purpose:** Control who can read which files

**Default Implementation:**
```c
int valid_read(string path, string func, object caller, int owner, int flags) {
  // System operations - always allow
  if (!caller) return 1;
  
  // Privileged objects - always allow
  if (priv(caller)) return 1;
  
  // Non-existent files - allow read attempts
  // (Allows checking if file exists)
  if (flags == -1) return 1;
  
  // Owner - always allow
  if (owner == otoi(caller)) return 1;
  
  // READ_OK flag set - allow
  if (flags & 1) return 1;
  
  // Otherwise deny
  return 0;
}
```

**Permission Flags:**
- `1` (bit 0) - Read permission
- `2` (bit 1) - Write permission
- `3` (both bits) - Read and write

**Common Patterns:**

**Allow all reads:**
```c
int valid_read(string path, string func, object caller, int owner, int flags) {
  return 1;  // Permissive
}
```

**Protect system files:**
```c
int valid_read(string path, string func, object caller, int owner, int flags) {
  if (!caller) return 1;
  if (priv(caller)) return 1;
  
  // Protect boot.c
  if (path == "/boot.c") return 0;
  
  // Protect /sys directory
  if (path == "/sys" || leftstr(path, 5) == "/sys/") {
    return 0;
  }
  
  // Allow everything else
  return 1;
}
```

**Role-based access:**
```c
int valid_read(string path, string func, object caller, int owner, int flags) {
  int caller_flags;
  
  if (!caller) return 1;
  if (priv(caller)) return 1;
  
  caller_flags = caller.get_flags();
  
  // Wizards can read everything except boot.c
  if (caller_flags & FLAG_WIZARD) {
    if (path == "/boot.c") return 0;
    return 1;
  }
  
  // Programmers can read their own files and public files
  if (caller_flags & FLAG_PROGRAMMER) {
    if (owner == otoi(caller)) return 1;
    if (flags & 1) return 1;
    return 0;
  }
  
  // Regular users - owner or public only
  if (owner == otoi(caller)) return 1;
  if (flags & 1) return 1;
  return 0;
}
```

**Audit logging:**
```c
int valid_read(string path, string func, object caller, int owner, int flags) {
  int result;
  
  // Determine if allowed
  if (!caller) result = 1;
  else if (priv(caller)) result = 1;
  else if (owner == otoi(caller)) result = 1;
  else if (flags & 1) result = 1;
  else result = 0;
  
  // Log denials
  if (!result && caller) {
    write_file("/var/log/access.log", 
      sprintf("[%s] READ DENIED: %s by %s (func: %s)\n",
        ctime(time()), path, otoa(caller), func));
  }
  
  return result;
}
```

#### 3.2.3 valid_write(path, func, caller, owner, flags)

**Called when:** Any write operation on the virtual filesystem

**Signature:**
```c
int valid_write(string path, string func, object caller, int owner, int flags)
```

**Parameters:** Same as `valid_read()`

**Returns:**
- `1` - Allow the operation
- `0` - Deny the operation

**Purpose:** Control who can write/modify which files

**Default Implementation:**
```c
int valid_write(string path, string func, object caller, int owner, int flags) {
  // System operations - always allow
  if (!caller) return 1;
  
  // Privileged objects - always allow
  if (priv(caller)) return 1;
  
  // Owner - always allow
  if (owner == otoi(caller)) return 1;
  
  // WRITE_OK flag set - allow
  if (flags & 2) return 1;
  
  // For new files (flags == -1), check parent directory
  if (flags == -1) {
    // Only privileged can create new files
    if (priv(caller)) return 1;
    return 0;
  }
  
  // Otherwise deny
  return 0;
}
```

**Common Patterns:**

**Protect system files:**
```c
int valid_write(string path, string func, object caller, int owner, int flags) {
  if (!caller) return 1;
  if (priv(caller)) return 1;
  
  // Protect boot.c
  if (path == "/boot.c") return 0;
  
  // Protect /sys directory
  if (path == "/sys" || leftstr(path, 5) == "/sys/") {
    return 0;
  }
  
  // Owner can write their files
  if (owner == otoi(caller)) return 1;
  
  // WRITE_OK flag
  if (flags & 2) return 1;
  
  return 0;
}
```

**Backup before write:**
```c
int valid_write(string path, string func, object caller, int owner, int flags) {
  string backup_path;
  
  // Check basic permissions first
  if (!caller) return 1;
  if (!priv(caller) && owner != otoi(caller) && !(flags & 2)) {
    return 0;
  }
  
  // Backup important files before modification
  if (leftstr(path, 5) == "/obj/" || leftstr(path, 5) == "/sys/") {
    backup_path = path + ".bak";
    if (fstat(path) != -1) {  // File exists
      // Copy to backup
      fwrite(backup_path, fread(path));
    }
  }
  
  return 1;
}
```

**Quota enforcement:**
```c
int valid_write(string path, string func, object caller, int owner, int flags) {
  int total_size, file_count;
  string user_dir;
  
  if (!caller) return 1;
  if (priv(caller)) return 1;
  
  // Check quota for non-privileged users
  user_dir = "/home/" + caller.get_name();
  if (leftstr(path, strlen(user_dir)) == user_dir) {
    // Calculate current usage
    total_size = calculate_directory_size(user_dir);
    file_count = count_files(user_dir);
    
    // Enforce limits
    if (total_size > 1000000) {  // 1MB limit
      caller.listen("Quota exceeded: disk space\n");
      return 0;
    }
    
    if (file_count > 100) {  // 100 files limit
      caller.listen("Quota exceeded: file count\n");
      return 0;
    }
  }
  
  // Allow if owner or write flag
  if (owner == otoi(caller)) return 1;
  if (flags & 2) return 1;
  
  return 0;
}
```

### 3.3 Optional Callbacks

These callbacks are not required but provide useful functionality:

#### 3.3.1 get_uid()

**Purpose:** Return the UID (user ID) for the boot object

```c
string get_uid() {
  return "root";
}
```

This identifies the boot object as "root" with full privileges.

#### 3.3.2 init()

**Purpose:** Initialize the system on first boot

```c
static init() {
  object room, wizobj;
  int me;
  
  me = otoi(this_object());
  
  // Set up auto-save (every 3600 seconds = 1 hour)
  alarm(3600, "save_db");
  
  // Make boot.c readable
  chmod("/boot.c", 1);
  
  // Create initial directories
  unhide("/etc", me, 5);
  unhide("/etc/pkg", me, 5);
  unhide("/home", me, 5);
  unhide("/obj", me, 5);
  unhide("/sys", me, 5);
  
  // Install core package
  add_pkg("core");
  
  // Schedule finish_init
  alarm(0, "finish_init");
}
```

#### 3.3.3 finish_init()

**Purpose:** Complete initialization after packages are loaded

```c
static finish_init() {
  object room, wizobj;
  
  // Grant privileges to system objects
  set_priv(atoo("/sys/sys"), 1);
  set_priv(atoo("/sys/ban"), 1);
  
  // Create Limbo room
  room = atoo("/obj/room");
  if (!room) {
    room = new("/obj/room");
  }
  room.set_name("Limbo");
  room.set_desc("A formless void. The beginning of all things.");
  room.set_flags(19);  // LINK_OK | JUMP_OK | STICKY
  
  // Create Wizard character
  wizobj = new("/obj/player");
  wizobj.set_secure();
  wizobj.set_name("Wizard");
  wizobj.set_password("potrzebie");  // Change this!
  wizobj.set_flags(64);  // WIZARD flag
  
  // Register Wizard in player table
  table_set("wizard", itoa(otoi(wizobj)));
  
  // Set up Wizard's home
  wizobj.set_link(room);
  move_object(wizobj, room);
  
  // Grant Wizard privileges
  set_priv(wizobj, 1);
}
```

#### 3.3.4 save_db()

**Purpose:** Periodic database save

```c
static save_db() {
  // Notify all connected players
  broadcast("*** Auto-save in progress ***\n");
  
  // Trigger database save
  sync();
  
  // Schedule next save
  alarm(3600, "save_db");
}
```

### 3.4 Complete Minimal boot.c

Here's a complete minimal boot object:

```c
// /boot.c - Minimal boot object

#include <sys.h>

// Auto-save interval (seconds)
#define SAVE_DELAY 3600

// Required: Handle new connections
static connect() {
  object login_obj;
  
  login_obj = new("/sys/login");
  if (!login_obj) {
    send_device("Unable to create login object\n");
    disconnect_device();
    return;
  }
  
  login_obj.set_secure();
  
  if (reconnect_device(login_obj)) {
    send_device("Unable to transfer connection\n");
    disconnect_device();
    destruct(login_obj);
    return;
  }
  
  set_priv(login_obj, 1);
  login_obj.connect();
}

// Required: File read permissions
int valid_read(string path, string func, object caller, int owner, int flags) {
  if (!caller) return 1;
  if (priv(caller)) return 1;
  if (flags == -1) return 1;
  if (owner == otoi(caller)) return 1;
  if (flags & 1) return 1;
  return 0;
}

// Required: File write permissions
int valid_write(string path, string func, object caller, int owner, int flags) {
  if (!caller) return 1;
  if (priv(caller)) return 1;
  if (owner == otoi(caller)) return 1;
  if (flags & 2) return 1;
  if (flags == -1 && priv(caller)) return 1;
  return 0;
}

// Optional: Return UID
string get_uid() {
  return "root";
}

// Optional: Initialize system
static init() {
  int me = otoi(this_object());
  
  alarm(SAVE_DELAY, "save_db");
  chmod("/boot.c", 1);
  
  unhide("/etc", me, 5);
  unhide("/sys", me, 5);
  unhide("/obj", me, 5);
  
  alarm(0, "finish_init");
}

// Optional: Finish initialization
static finish_init() {
  object room;
  
  set_priv(atoo("/sys/sys"), 1);
  
  room = new("/obj/room");
  room.set_name("Limbo");
  room.set_desc("The void.");
  room.set_flags(19);
}

// Optional: Auto-save
static save_db() {
  broadcast("*** Auto-save ***\n");
  sync();
  alarm(SAVE_DELAY, "save_db");
}
```

This is the absolute minimum needed for a working boot object. In the next sections, we'll build the rest of the mudlib.

---

## 4. Connection Handling

Understanding how connections flow from TCP to player objects is essential for building a mudlib.

### 4.1 The Complete Flow

```
1. TCP Connection Established
         │
         ▼
2. NetCI Engine (intrface.c)
   - Accepts connection
   - Calls boot.c::connect()
         │
         ▼
3. boot.c::connect()
   - Creates /sys/login object
   - Transfers connection
   - Grants privileges
         │
         ▼
4. /sys/login.c
   - Displays welcome message
   - Handles authentication
   - Creates/loads player
         │
         ▼
5. /obj/player.c
   - Connection transferred
   - Player enters game world
   - Commands processed
```

### 4.2 Login Object Implementation

The login object handles authentication and character creation.

#### 4.2.1 Required Functions

**connect()** - Start the login process
```c
connect() {
  // Display welcome message
  listen("Welcome to NetCI MUD!\n");
  listen("Commands: connect <name> <password>, create <name> <password>, WHO, quit\n");
  listen("\n> ");
}
```

**listen(input)** - Handle user input
```c
listen(input) {
  string cmd, arg;
  int pos;
  
  // Parse command
  pos = instr(input, 1, " ");
  if (pos) {
    cmd = downcase(leftstr(input, pos-1));
    arg = rightstr(input, strlen(input)-pos);
  } else {
    cmd = downcase(input);
    arg = "";
  }
  
  // Execute command
  if (cmd == "connect") {
    do_connect(arg);
  } else if (cmd == "create") {
    do_create(arg);
  } else if (cmd == "who") {
    do_who();
  } else if (cmd == "quit") {
    disconnect_device();
    destruct(this_object());
  } else {
    send_device("Unknown command. Try: connect, create, WHO, quit\n> ");
  }
}
```

#### 4.2.2 Complete Login Object

```c
// /sys/login.c - Complete login handler

#include <sys.h>

// Display welcome message
connect() {
  send_device("\n");
  send_device("╔════════════════════════════════════════╗\n");
  send_device("║     Welcome to NetCI MUD!              ║\n");
  send_device("╚════════════════════════════════════════╝\n");
  send_device("\n");
  send_device("Commands:\n");
  send_device("  connect <name> <password> - Login\n");
  send_device("  create <name> <password>  - Create character\n");
  send_device("  WHO                       - List players\n");
  send_device("  quit                      - Disconnect\n");
  send_device("\n> ");
}

// Handle user input
listen(input) {
  string cmd, arg;
  int pos;
  
  // Remove trailing newline
  if (rightstr(input, 1) == "\n") {
    input = leftstr(input, strlen(input)-1);
  }
  
  // Parse command and arguments
  pos = instr(input, 1, " ");
  if (pos) {
    cmd = downcase(leftstr(input, pos-1));
    arg = rightstr(input, strlen(input)-pos);
  } else {
    cmd = downcase(input);
    arg = "";
  }
  
  // Execute command
  if (cmd == "connect") {
    do_connect(arg);
  } else if (cmd == "create") {
    do_create(arg);
  } else if (cmd == "who" || cmd == "WHO") {
    do_who();
  } else if (cmd == "quit") {
    send_device("Goodbye!\n");
    disconnect_device();
    destruct(this_object());
  } else {
    send_device("Unknown command.\n> ");
  }
}

// Handle login
static do_connect(arg) {
  string name, password;
  object player;
  int pos;
  
  // Parse name and password
  pos = instr(arg, 1, " ");
  if (!pos) {
    send_device("Usage: connect <name> <password>\n> ");
    return;
  }
  
  name = leftstr(arg, pos-1);
  password = rightstr(arg, strlen(arg)-pos);
  
  // Validate name
  if (!is_legal_name(name)) {
    send_device("Invalid name.\n> ");
    return;
  }
  
  // Find player
  player = find_player(name);
  if (!player) {
    send_device("No such player.\n> ");
    return;
  }
  
  // Check password
  if (!player.check_password(password)) {
    send_device("Incorrect password.\n> ");
    return;
  }
  
  // Check if already connected
  if (interactive(player)) {
    send_device("That player is already connected.\n> ");
    return;
  }
  
  // Transfer connection
  if (reconnect_device(player)) {
    send_device("Unable to transfer connection.\n");
    disconnect_device();
    destruct(this_object());
    return;
  }
  
  // Start player session
  player.connect();
  
  // Self-destruct
  destruct(this_object());
}

// Handle character creation
static do_create(arg) {
  string name, password;
  object player;
  int pos;
  
  // Parse name and password
  pos = instr(arg, 1, " ");
  if (!pos) {
    send_device("Usage: create <name> <password>\n> ");
    return;
  }
  
  name = leftstr(arg, pos-1);
  password = rightstr(arg, strlen(arg)-pos);
  
  // Validate name
  if (!is_legal_name(name)) {
    send_device("Invalid name. Use letters only, 3-14 characters.\n> ");
    return;
  }
  
  // Check if name exists
  if (find_player(name)) {
    send_device("That name is already taken.\n> ");
    return;
  }
  
  // Create player
  player = new("/obj/player");
  if (!player) {
    send_device("Unable to create player.\n");
    disconnect_device();
    destruct(this_object());
    return;
  }
  
  // Initialize player
  player.set_secure();
  player.set_name(name);
  player.set_password(password);
  player.set_flags(0);  // No special flags
  
  // Register player
  add_new_player(name, player);
  
  // Set starting location
  player.set_link(atoo("/obj/room"));  // Limbo
  move_object(player, atoo("/obj/room"));
  
  // Transfer connection
  if (reconnect_device(player)) {
    send_device("Unable to transfer connection.\n");
    disconnect_device();
    destruct(player);
    destruct(this_object());
    return;
  }
  
  // Start player session
  player.connect();
  
  // Self-destruct
  destruct(this_object());
}

// Show connected players
static do_who() {
  object curr;
  int count;
  
  send_device("\nConnected Players:\n");
  send_device("------------------\n");
  
  curr = NULL;
  count = 0;
  
  while (curr = next_who(curr)) {
    send_device(curr.get_name() + "\n");
    count++;
  }
  
  send_device("\nTotal: " + itoa(count) + " player");
  if (count != 1) send_device("s");
  send_device("\n\n> ");
}
```

### 4.3 Player Object Implementation

The player object is the most complex part of the mudlib. It must handle:
- Connection transfer
- User input
- Commands
- Communication
- Disconnection

#### 4.3.1 Required Functions

**connect()** - Handle connection transfer from login
```c
connect() {
  object location;
  
  // Get player's location
  location = get_link();
  if (!location) {
    location = atoo("/obj/room");  // Default to Limbo
    set_link(location);
  }
  
  // Move player to location
  move_object(this_object(), location);
  
  // Announce arrival
  tell_room_except(location, this_object(), NULL, HEAR_ENTER, "", "", "");
  
  // Show room
  do_look("");
  
  // Store connection info
  conn_addr = get_devconn(this_object());
}
```

**listen(input)** - Handle user input
```c
listen(input) {
  string cmd, arg;
  int pos;
  
  // Remove trailing newline
  if (rightstr(input, 1) == "\n") {
    input = leftstr(input, strlen(input)-1);
  }
  
  // Parse command and arguments
  pos = instr(input, 1, " ");
  if (pos) {
    cmd = downcase(leftstr(input, pos-1));
    arg = rightstr(input, strlen(input)-pos);
  } else {
    cmd = downcase(input);
    arg = "";
  }
  
  // Execute command
  if (!execute_verb(cmd, arg)) {
    do_huh(cmd);
  }
}
```

**hear(actor, actee, type, message, pre, post)** - Handle environmental messages
```c
hear(actor, actee, type, message, pre, post) {
  // Don't hear our own actions
  if (actor == this_object()) return;
  
  switch(type) {
    case HEAR_SAY:
      send_device(actor.get_name() + " says, \"" + message + "\"\n");
      break;
    case HEAR_POSE:
      send_device(actor.get_name() + " " + message + "\n");
      break;
    case HEAR_ENTER:
      send_device(actor.get_name() + " has arrived.\n");
      break;
    case HEAR_LEAVE:
      send_device(actor.get_name() + " has left.\n");
      break;
  }
}
```

**disconnect()** - Handle disconnection
```c
disconnect() {
  object location;
  
  location = get_link();
  if (location) {
    tell_room_except(location, this_object(), NULL, HEAR_LEAVE, "", "", "");
  }
  
  // Don't destruct - player persists
}
```

**check_password(password)** - Validate password
```c
check_password(password) {
  return (crypt(password, stored_password) == stored_password);
}
```

**set_password(password)** - Set new password
```c
set_password(password) {
  stored_password = crypt(password, 0);
}
```

#### 4.3.2 Minimal Player Object

```c
// /obj/player.c - Minimal player object

#include <sys.h>
#include <hear.h>

// State variables
string stored_password;
string conn_addr;
object sharobj;

// Initialize
static init() {
  // Attach shared functionality
  sharobj = new("/obj/share/common");
  attach(sharobj);
  
  set_type(TYPE_PLAYER);
  
  // Add basic verbs
  add_verb("say", "do_say");
  add_verb("look", "do_look");
  add_verb("quit", "do_quit");
}

// Handle connection from login
connect() {
  object location;
  
  location = get_link();
  if (!location) {
    location = atoo("/obj/room");
    set_link(location);
  }
  
  move_object(this_object(), location);
  tell_room_except(location, this_object(), NULL, HEAR_ENTER, "", "", "");
  
  do_look("");
  
  conn_addr = get_devconn(this_object());
}

// Handle user input
listen(input) {
  string cmd, arg;
  int pos;
  
  if (rightstr(input, 1) == "\n") {
    input = leftstr(input, strlen(input)-1);
  }
  
  pos = instr(input, 1, " ");
  if (pos) {
    cmd = downcase(leftstr(input, pos-1));
    arg = rightstr(input, strlen(input)-pos);
  } else {
    cmd = downcase(input);
    arg = "";
  }
  
  if (!execute_verb(cmd, arg)) {
    send_device("Huh?\n");
  }
}

// Handle environmental messages
hear(actor, actee, type, message, pre, post) {
  if (actor == this_object()) return;
  
  switch(type) {
    case HEAR_SAY:
      send_device(actor.get_name() + " says, \"" + message + "\"\n");
      break;
    case HEAR_POSE:
      send_device(actor.get_name() + " " + message + "\n");
      break;
    case HEAR_ENTER:
      send_device(actor.get_name() + " has arrived.\n");
      break;
    case HEAR_LEAVE:
      send_device(actor.get_name() + " has left.\n");
      break;
  }
}

// Handle disconnection
disconnect() {
  object location = get_link();
  if (location) {
    tell_room_except(location, this_object(), NULL, HEAR_LEAVE, "", "", "");
  }
}

// Password functions
check_password(password) {
  return (crypt(password, stored_password) == stored_password);
}

set_password(password) {
  stored_password = crypt(password, 0);
}

// Basic commands
static do_say(arg) {
  if (!arg || arg == "") {
    send_device("Say what?\n");
    return 1;
  }
  
  tell_room(get_link(), this_object(), NULL, HEAR_SAY, arg, "", "");
  send_device("You say, \"" + arg + "\"\n");
  return 1;
}

static do_look(arg) {
  object location, *contents, curr;
  string desc;
  int i;
  
  location = get_link();
  if (!location) {
    send_device("You are nowhere.\n");
    return 1;
  }
  
  // Show room name
  send_device("\n" + location.get_name() + "\n");
  
  // Show description
  desc = location.get_desc();
  if (desc && desc != "") {
    send_device(desc + "\n");
  }
  
  // Show contents
  contents = get_contents(location);
  if (contents && sizeof(contents) > 0) {
    send_device("\nYou see:\n");
    for (i = 0; i < sizeof(contents); i++) {
      curr = contents[i];
      if (curr != this_object()) {
        send_device("  " + curr.get_name() + "\n");
      }
    }
  }
  
  send_device("\n");
  return 1;
}

static do_quit(arg) {
  send_device("Goodbye!\n");
  disconnect_device();
  return 1;
}
```

### 4.4 Connection Transfer Rules

**Critical Rules:**
1. Only one connection per object at a time
2. Must use `reconnect_device(target)` to transfer
3. Old object should self-destruct after transfer
4. Target must have `listen()` function
5. Caller must have PRIV flag or be the boot object

**Common Mistakes:**
- ❌ Forgetting to destruct login object after transfer
- ❌ Not checking `reconnect_device()` return value
- ❌ Transferring to object without `listen()` function
- ❌ Not granting privileges to login object

**Correct Pattern:**
```c
// In boot.c::connect()
login_obj = new("/sys/login");
login_obj.set_secure();
set_priv(login_obj, 1);  // Grant privileges

if (reconnect_device(login_obj)) {
  // Handle error
  disconnect_device();
  destruct(login_obj);
  return;
}

login_obj.connect();

// In login.c::do_connect()
if (reconnect_device(player)) {
  // Handle error
  disconnect_device();
  destruct(this_object());
  return;
}

player.connect();
destruct(this_object());  // Clean up login object
```

---

## 5. Security Policies

Security is implemented through the `valid_read()` and `valid_write()` callbacks in boot.c. Here are common patterns:

### 5.1 Role-Based Access Control

Define roles using flags:

```c
// In /include/flags.h
#define FLAG_WIZARD     64
#define FLAG_PROGRAMMER 32
#define FLAG_BUILDER    8
```

Implement role checking:

```c
// In boot.c
static has_role(caller, role_name) {
  string name;
  if (!caller) return 0;
  name = caller.get_name();
  if (!name) return 0;
  return (downcase(name) == downcase(role_name));
}

int valid_read(string path, string func, object caller, int owner, int flags) {
  int caller_flags;
  
  if (!caller) return 1;
  if (priv(caller)) return 1;
  
  caller_flags = caller.get_flags();
  
  // Wizard: Full read access except boot.c
  if (has_role(caller, "Wizard")) {
    if (path == "/boot.c") return 0;
    return 1;
  }
  
  // Programmer: Read all except boot.c and /sys
  if (caller_flags & FLAG_PROGRAMMER) {
    if (path == "/boot.c") return 0;
    if (path == "/sys" || leftstr(path, 5) == "/sys/") return 0;
    return 1;
  }
  
  // Regular user
  if (owner == otoi(caller)) return 1;
  if (flags & 1) return 1;
  return 0;
}
```

### 5.2 Path-Based Restrictions

Protect system directories:

```c
int valid_write(string path, string func, object caller, int owner, int flags) {
  if (!caller) return 1;
  if (priv(caller)) return 1;
  
  // Protect system files
  if (path == "/boot.c") return 0;
  if (leftstr(path, 5) == "/sys/") return 0;
  if (leftstr(path, 5) == "/etc/") return 0;
  
  // Allow home directory access
  if (leftstr(path, 6) == "/home/") {
    string user_dir = "/home/" + caller.get_name();
    if (leftstr(path, strlen(user_dir)) == user_dir) {
      return 1;
    }
  }
  
  // Owner or write flag
  if (owner == otoi(caller)) return 1;
  if (flags & 2) return 1;
  
  return 0;
}
```

---

## 6. Object Hierarchies

### 6.1 Room Objects

Rooms are containers for players and objects:

```c
// /obj/room.c
object sharobj;

static init() {
  sharobj = new("/obj/share/common");
  attach(sharobj);
  set_type(TYPE_ROOM);
  set_owner(caller_object());
}
```

All functionality comes from `/obj/share/common.c`:
- Name and description
- Contents list
- Exit list
- Flags

### 6.2 Shared Objects

**`/obj/share/common.c`** provides basic object functionality:

```c
// /obj/share/common.c - Basic object functionality
string name;
string desc;
int type;
int flags;
object owner;
object link;

void set_name(string n) { name = n; }
string get_name() { return name; }

void set_desc(string d) { desc = d; }
string get_desc() { return desc; }

void set_type(int t) { type = t; }
int get_type() { return type; }

void set_flags(int f) { flags = f; }
int get_flags() { return flags; }

void set_owner(object o) { owner = o; }
object get_owner() { return owner; }

void set_link(object l) { link = l; }
object get_link() { return link; }
```

---

# PART II: PRACTICAL GUIDE

## 7. Building a Skeleton Mudlib

This section provides step-by-step instructions for building a minimal working mudlib from scratch.

### 7.1 Minimal Mudlib Structure

```
/
├── boot.c              # Master object (REQUIRED)
├── sys/
│   ├── login.c         # Login handler (REQUIRED)
│   └── sys.c           # Utility functions
├── obj/
│   ├── player.c        # Player object (REQUIRED)
│   ├── room.c          # Room object
│   └── share/
│       └── common.c    # Shared functionality (REQUIRED)
└── include/
    ├── sys.h           # System includes
    ├── types.h         # Object types
    ├── flags.h         # Object flags
    └── hear.h          # Communication types
```

### 7.2 Step-by-Step Implementation

#### Step 1: Create Include Files

**`/include/types.h`** - Object types:
```c
// Object types
#define TYPE_PLAYER  1
#define TYPE_OBJECT  2
#define TYPE_EXIT    3
#define TYPE_ROOM    4
```

**`/include/flags.h`** - Object flags:
```c
// Object flags
#define FLAG_BUILDER     1
#define FLAG_DARK        2
#define FLAG_JUMP_OK     4
#define FLAG_LINK_OK     8
#define FLAG_PROGRAMMER  16
#define FLAG_STICKY      32
#define FLAG_WIZARD      64
```

**`/include/hear.h`** - Communication types:
```c
// Hear types
#define HEAR_SAY     1
#define HEAR_POSE    2
#define HEAR_EMOTE   3
#define HEAR_ENTER   4
#define HEAR_LEAVE   5
```

**`/include/sys.h`** - System includes:
```c
// System includes
#include <types.h>
#include <flags.h>
#include <hear.h>
```

#### Step 2: Create boot.c

```c
// /boot.c - Master object

#include <sys.h>

#define SAVE_DELAY 3600

// REQUIRED: Handle new connections
static connect() {
  object login_obj;
  
  login_obj = new("/sys/login");
  if (!login_obj) {
    send_device("Unable to create login object\n");
    disconnect_device();
    return;
  }
  
  login_obj.set_secure();
  
  if (reconnect_device(login_obj)) {
    send_device("Unable to transfer connection\n");
    disconnect_device();
    destruct(login_obj);
    return;
  }
  
  set_priv(login_obj, 1);
  login_obj.connect();
}

// REQUIRED: File read permissions
int valid_read(string path, string func, object caller, int owner, int flags) {
  if (!caller) return 1;
  if (priv(caller)) return 1;
  if (flags == -1) return 1;
  if (owner == otoi(caller)) return 1;
  if (flags & 1) return 1;
  return 0;
}

// REQUIRED: File write permissions
int valid_write(string path, string func, object caller, int owner, int flags) {
  if (!caller) return 1;
  if (priv(caller)) return 1;
  if (owner == otoi(caller)) return 1;
  if (flags & 2) return 1;
  if (flags == -1 && priv(caller)) return 1;
  return 0;
}

// Optional: Return UID
string get_uid() {
  return "root";
}

// Optional: Initialize system
static init() {
  int me = otoi(this_object());
  
  alarm(SAVE_DELAY, "save_db");
  chmod("/boot.c", 1);
  
  unhide("/sys", me, 5);
  unhide("/obj", me, 5);
  unhide("/include", me, 5);
  
  alarm(0, "finish_init");
}

// Optional: Finish initialization
static finish_init() {
  object room, wizobj;
  
  set_priv(atoo("/sys/sys"), 1);
  
  room = new("/obj/room");
  room.set_name("Limbo");
  room.set_desc("A formless void. The beginning of all things.");
  room.set_flags(FLAG_LINK_OK | FLAG_JUMP_OK | FLAG_STICKY);
  
  wizobj = new("/obj/player");
  wizobj.set_secure();
  wizobj.set_name("Wizard");
  wizobj.set_password("changeme");
  wizobj.set_flags(FLAG_WIZARD);
  
  table_set("wizard", itoa(otoi(wizobj)));
  
  wizobj.set_link(room);
  move_object(wizobj, room);
  
  set_priv(wizobj, 1);
}

// Optional: Auto-save
static save_db() {
  broadcast("*** Auto-save ***\n");
  sync();
  alarm(SAVE_DELAY, "save_db");
}
```

#### Step 3: Create /obj/share/common.c

```c
// /obj/share/common.c - Shared object functionality

#include <sys.h>

// Properties
string name;
string desc;
int type;
int flags;
object owner;
object link;

// Name
void set_name(string n) { name = n; }
string get_name() { return name ? name : "something"; }

// Description
void set_desc(string d) { desc = d; }
string get_desc() { return desc ? desc : "You see nothing special."; }

// Type
void set_type(int t) { type = t; }
int get_type() { return type; }

// Flags
void set_flags(int f) { flags = f; }
int get_flags() { return flags; }

// Owner
void set_owner(object o) { owner = o; }
object get_owner() { return owner; }

// Link (home/destination)
void set_link(object l) { link = l; }
object get_link() { return link; }

// Secure flag
void set_secure() {
  flags = flags | 128;  // SECURE flag
}

int is_secure() {
  return (flags & 128);
}
```

#### Step 4: Create /obj/room.c

```c
// /obj/room.c - Room object

#include <sys.h>

object sharobj;

static init() {
  sharobj = new("/obj/share/common");
  attach(sharobj);
  set_type(TYPE_ROOM);
  set_owner(caller_object());
}
```

#### Step 5: Create /sys/sys.c

```c
// /sys/sys.c - Utility functions

#include <sys.h>

// Add player to table
add_new_player(name, o) {
  table_set(downcase(name), itoa(otoi(o)));
}

// Find player by name
find_player(name) {
  string ref;
  object curr;
  
  if (downcase(name) == "me") return this_player();
  
  ref = table_get(downcase(name));
  if (ref) return atoo(ref);
  
  // Try partial match on connected players
  curr = NULL;
  while (curr = next_who(curr)) {
    if (leftstr(downcase(curr.get_name()), strlen(name)) == downcase(name)) {
      return curr;
    }
  }
  
  return NULL;
}

// Validate name
is_legal_name(s) {
  int i, len, c;
  
  s = downcase(s);
  len = strlen(s);
  
  if (len < 3 || len > 14) return 0;
  
  // Check for illegal characters
  for (i = 1; i <= len; i++) {
    c = ascii(midstr(s, i, 1));
    if (c < 97 || c > 122) return 0;  // Not a-z
  }
  
  // Check for reserved names
  if (s == "root" || s == "void") return 0;
  if (leftstr(s, 5) == "guest") return 0;
  
  return 1;
}
```

#### Step 6: Create /sys/login.c

```c
// /sys/login.c - Login handler

#include <sys.h>

connect() {
  send_device("\nWelcome to NetCI MUD!\n\n");
  send_device("Commands:\n");
  send_device("  connect <name> <password>\n");
  send_device("  create <name> <password>\n");
  send_device("  WHO\n");
  send_device("  quit\n");
  send_device("\n> ");
}

listen(input) {
  string cmd, arg;
  int pos;
  
  if (rightstr(input, 1) == "\n") {
    input = leftstr(input, strlen(input)-1);
  }
  
  pos = instr(input, 1, " ");
  if (pos) {
    cmd = downcase(leftstr(input, pos-1));
    arg = rightstr(input, strlen(input)-pos);
  } else {
    cmd = downcase(input);
    arg = "";
  }
  
  if (cmd == "connect") {
    do_connect(arg);
  } else if (cmd == "create") {
    do_create(arg);
  } else if (cmd == "who" || cmd == "WHO") {
    do_who();
  } else if (cmd == "quit") {
    send_device("Goodbye!\n");
    disconnect_device();
    destruct(this_object());
  } else {
    send_device("Unknown command.\n> ");
  }
}

static do_connect(arg) {
  string name, password;
  object player;
  int pos;
  
  pos = instr(arg, 1, " ");
  if (!pos) {
    send_device("Usage: connect <name> <password>\n> ");
    return;
  }
  
  name = leftstr(arg, pos-1);
  password = rightstr(arg, strlen(arg)-pos);
  
  if (!is_legal_name(name)) {
    send_device("Invalid name.\n> ");
    return;
  }
  
  player = find_player(name);
  if (!player) {
    send_device("No such player.\n> ");
    return;
  }
  
  if (!player.check_password(password)) {
    send_device("Incorrect password.\n> ");
    return;
  }
  
  if (interactive(player)) {
    send_device("That player is already connected.\n> ");
    return;
  }
  
  if (reconnect_device(player)) {
    send_device("Unable to transfer connection.\n");
    disconnect_device();
    destruct(this_object());
    return;
  }
  
  player.connect();
  destruct(this_object());
}

static do_create(arg) {
  string name, password;
  object player;
  int pos;
  
  pos = instr(arg, 1, " ");
  if (!pos) {
    send_device("Usage: create <name> <password>\n> ");
    return;
  }
  
  name = leftstr(arg, pos-1);
  password = rightstr(arg, strlen(arg)-pos);
  
  if (!is_legal_name(name)) {
    send_device("Invalid name.\n> ");
    return;
  }
  
  if (find_player(name)) {
    send_device("That name is already taken.\n> ");
    return;
  }
  
  player = new("/obj/player");
  if (!player) {
    send_device("Unable to create player.\n");
    disconnect_device();
    destruct(this_object());
    return;
  }
  
  player.set_secure();
  player.set_name(name);
  player.set_password(password);
  player.set_flags(0);
  
  add_new_player(name, player);
  
  player.set_link(atoo("/obj/room"));
  move_object(player, atoo("/obj/room"));
  
  if (reconnect_device(player)) {
    send_device("Unable to transfer connection.\n");
    disconnect_device();
    destruct(player);
    destruct(this_object());
    return;
  }
  
  player.connect();
  destruct(this_object());
}

static do_who() {
  object curr;
  int count;
  
  send_device("\nConnected Players:\n");
  send_device("------------------\n");
  
  curr = NULL;
  count = 0;
  
  while (curr = next_who(curr)) {
    send_device(curr.get_name() + "\n");
    count++;
  }
  
  send_device("\nTotal: " + itoa(count) + "\n\n> ");
}
```

#### Step 7: Create /obj/player.c

```c
// /obj/player.c - Player object

#include <sys.h>

string stored_password;
string conn_addr;
object sharobj;

static init() {
  sharobj = new("/obj/share/common");
  attach(sharobj);
  
  set_type(TYPE_PLAYER);
  
  add_verb("say", "do_say");
  add_verb("'", "do_say");
  add_verb("look", "do_look");
  add_verb("l", "do_look");
  add_verb("quit", "do_quit");
  add_verb("who", "do_who");
}

connect() {
  object location;
  
  location = get_link();
  if (!location) {
    location = atoo("/obj/room");
    set_link(location);
  }
  
  move_object(this_object(), location);
  tell_room_except(location, this_object(), NULL, HEAR_ENTER, "", "", "");
  
  send_device("\nWelcome, " + get_name() + "!\n\n");
  do_look("");
}

listen(input) {
  string cmd, arg;
  int pos;
  
  if (rightstr(input, 1) == "\n") {
    input = leftstr(input, strlen(input)-1);
  }
  
  pos = instr(input, 1, " ");
  if (pos) {
    cmd = downcase(leftstr(input, pos-1));
    arg = rightstr(input, strlen(input)-pos);
  } else {
    cmd = downcase(input);
    arg = "";
  }
  
  if (!execute_verb(cmd, arg)) {
    send_device("Huh?\n");
  }
}

hear(actor, actee, type, message, pre, post) {
  if (actor == this_object()) return;
  
  switch(type) {
    case HEAR_SAY:
      send_device(actor.get_name() + " says, \"" + message + "\"\n");
      break;
    case HEAR_POSE:
      send_device(actor.get_name() + " " + message + "\n");
      break;
    case HEAR_ENTER:
      send_device(actor.get_name() + " has arrived.\n");
      break;
    case HEAR_LEAVE:
      send_device(actor.get_name() + " has left.\n");
      break;
  }
}

disconnect() {
  object location = get_link();
  if (location) {
    tell_room_except(location, this_object(), NULL, HEAR_LEAVE, "", "", "");
  }
}

check_password(password) {
  return (crypt(password, stored_password) == stored_password);
}

set_password(password) {
  stored_password = crypt(password, 0);
}

static do_say(arg) {
  if (!arg || arg == "") {
    send_device("Say what?\n");
    return 1;
  }
  
  tell_room(get_link(), this_object(), NULL, HEAR_SAY, arg, "", "");
  send_device("You say, \"" + arg + "\"\n");
  return 1;
}

static do_look(arg) {
  object location, curr;
  string desc;
  
  location = get_link();
  if (!location) {
    send_device("You are nowhere.\n");
    return 1;
  }
  
  send_device("\n" + location.get_name() + "\n");
  
  desc = location.get_desc();
  if (desc && desc != "") {
    send_device(desc + "\n");
  }
  
  send_device("\nYou see:\n");
  curr = first_inventory(location);
  while (curr) {
    if (curr != this_object()) {
      send_device("  " + curr.get_name() + "\n");
    }
    curr = next_inventory(curr);
  }
  
  send_device("\n");
  return 1;
}

static do_quit(arg) {
  send_device("Goodbye!\n");
  disconnect_device();
  return 1;
}

static do_who(arg) {
  object curr;
  int count;
  
  send_device("\nConnected Players:\n");
  send_device("------------------\n");
  
  curr = NULL;
  count = 0;
  
  while (curr = next_who(curr)) {
    send_device(curr.get_name() + "\n");
    count++;
  }
  
  send_device("\nTotal: " + itoa(count) + "\n\n");
  return 1;
}
```

### 7.3 Testing Your Skeleton Mudlib

**Step 1: Start NetCI**
```bash
./netci -f mudlib
```

**Step 2: Connect**
```bash
telnet localhost 6666
```

**Step 3: Create a character**
```
create alice password123
```

**Step 4: Test commands**
```
look
say Hello world!
who
quit
```

**Step 5: Reconnect**
```
connect alice password123
```

**Congratulations!** You now have a working skeleton mudlib.

---

## 8. Command System

### 8.1 The Verb System

Commands are implemented using verbs:

```c
add_verb("say", "do_say");      // Normal verb
add_xverb("@create", "do_create");  // Extended verb (gets full input)
```

**Verb Lookup Order:**
1. Local verbs on player
2. Local verbs on location
3. Local verbs on objects in location
4. Global verbs on player

### 8.2 Adding Commands

**Basic Command Pattern:**
```c
static do_command(arg) {
  // Validate arguments
  if (!arg || arg == "") {
    send_device("Usage: command <argument>\n");
    return 1;
  }
  
  // Perform action
  // ...
  
  // Return 1 for success
  return 1;
}

// In init():
add_verb("command", "do_command");
```

**Communication Commands:**
```c
static do_say(arg) {
  if (!arg || arg == "") {
    send_device("Say what?\n");
    return 1;
  }
  
  tell_room(get_link(), this_object(), NULL, HEAR_SAY, arg, "", "");
  send_device("You say, \"" + arg + "\"\n");
  return 1;
}

static do_pose(arg) {
  if (!arg || arg == "") {
    send_device("Pose what?\n");
    return 1;
  }
  
  tell_room(get_link(), this_object(), NULL, HEAR_POSE, arg, "", "");
  send_device(get_name() + " " + arg + "\n");
  return 1;
}
```

---

## 9. Best Practices

### 9.1 Code Organization

- Keep related functions together
- Use meaningful variable names
- Comment complex logic
- Follow consistent style

### 9.2 Security

- Always check `priv()` for privileged commands
- Validate all user input
- Sanitize file paths
- Log security events

### 9.3 Performance

- Minimize object creation
- Cache frequently-used values
- Avoid nested loops
- Use efficient string operations

### 9.4 Error Handling

- Always check return values
- Provide helpful error messages
- Clean up on errors
- Log errors for debugging

---

## 10. Testing and Debugging

### 10.1 Testing Commands

Test each command thoroughly:
```
say hello
look
who
quit
```

### 10.2 Common Issues

**"Permission Denied"**
- Check `valid_read()` / `valid_write()`
- Check file ownership
- Check privilege flag

**"Unable to clone"**
- Object doesn't exist
- Syntax error in code
- Missing dependencies

**"Reconnect failed"**
- Target object doesn't exist
- Target already has connection
- Target missing `listen()` function

### 10.3 Debugging Techniques

**Check compilation:**
```
compile_object("/obj/player")
```

**Examine objects:**
```
@examine #42
```

**Call functions directly:**
```
@call /obj/player do_say hello
```

---

## 11. Advanced Topics

### 11.1 Package System

Create package files to install multiple files:

```
# /etc/pkg/mypack.pkg
version 1.0.0

directory rw /home/wizard/mypack
file r /home/wizard/mypack/myobject.c

compile /home/wizard/mypack/myobject
```

Install:
```
@package add mypack
```

### 11.2 Database Management

**Auto-save:**
- Configured in boot.c
- Default: every 3600 seconds (1 hour)
- Notifies all players

**Manual save:**
```
@save
```

**Emergency save:**
```
@panic
```

---

## 12. Complete Examples

### 12.1 Complete Minimal Mudlib

All files shown in Section 7.2 constitute a complete minimal mudlib.

**Features:**
- ✅ Boot object with required callbacks
- ✅ Login system with authentication
- ✅ Player object with basic commands
- ✅ Room system
- ✅ Shared object functionality
- ✅ Security policies

**Commands Available:**
- `say` / `'` - Say something
- `look` / `l` - Look around
- `who` - List players
- `quit` - Disconnect

### 12.2 Adding More Commands

**Movement commands:**
```c
static do_north(arg) {
  object exit, dest;
  
  exit = find_exit(get_link(), "north");
  if (!exit) {
    send_device("You can't go that way.\n");
    return 1;
  }
  
  dest = exit.get_link();
  if (!dest) {
    send_device("That exit leads nowhere.\n");
    return 1;
  }
  
  tell_room_except(get_link(), this_object(), NULL, HEAR_LEAVE, "", "", "");
  move_object(this_object(), dest);
  tell_room_except(dest, this_object(), NULL, HEAR_ENTER, "", "", "");
  
  do_look("");
  return 1;
}

// In init():
add_verb("north", "do_north");
add_verb("n", "do_north");
```

---

# Appendices

## Appendix A: Required Functions Reference

### Boot Object (boot.c)

**Required:**
- `connect()` - Handle new TCP connections
- `valid_read(path, func, caller, owner, flags)` - File read permissions
- `valid_write(path, func, caller, owner, flags)` - File write permissions

**Optional:**
- `get_uid()` - Return UID ("root")
- `init()` - Initialize system
- `finish_init()` - Complete initialization
- `save_db()` - Periodic save

### Login Object (/sys/login.c)

**Required:**
- `connect()` - Display welcome message
- `listen(input)` - Handle user input

**Typical:**
- `do_connect(arg)` - Handle login
- `do_create(arg)` - Handle character creation
- `do_who()` - Show connected players

### Player Object (/obj/player.c)

**Required:**
- `init()` - Initialize player
- `connect()` - Handle connection transfer
- `listen(input)` - Handle user input
- `hear(actor, actee, type, message, pre, post)` - Handle messages
- `disconnect()` - Handle disconnection
- `check_password(password)` - Validate password
- `set_password(password)` - Set password

**Typical Commands:**
- `do_say(arg)` - Say something
- `do_look(arg)` - Look around
- `do_quit(arg)` - Disconnect
- `do_who(arg)` - List players

### Shared Object (/obj/share/common.c)

**Typical:**
- `set_name(string)` / `get_name()` - Name management
- `set_desc(string)` / `get_desc()` - Description management
- `set_type(int)` / `get_type()` - Type management
- `set_flags(int)` / `get_flags()` - Flag management
- `set_owner(object)` / `get_owner()` - Owner management
- `set_link(object)` / `get_link()` - Link management
- `set_secure()` / `is_secure()` - Secure flag

---

## Appendix B: System Calls Reference

### Object Operations

```c
object new(string path)              // Create instance
void destruct(object obj)            // Destroy object
object atoo(string path)             // Get object by path
string otoa(object obj)              // Object to string
int otoi(object obj)                 // Object to reference number
int prototype(object obj)            // Check if prototype
string prototype_path(object obj)    // Get prototype path
object caller_object()               // Get calling object
object this_object()                 // Get current object
object this_player()                 // Get current player
```

### File Operations

```c
string fread(string path)            // Read file
int fwrite(string path, string data) // Write file
int fstat(string path)               // Get file flags
int fowner(string path)              // Get file owner
int chmod(string path, int flags)    // Change permissions
int chown(string path, object owner) // Change owner
int unhide(string path, int owner, int flags) // Make visible
```

### String Operations

```c
int strlen(string s)                 // String length
string leftstr(string s, int n)     // Left substring
string rightstr(string s, int n)    // Right substring
string midstr(string s, int pos, int len) // Middle substring
int instr(string s, int start, string search) // Find position
string upcase(string s)              // To uppercase
string downcase(string s)            // To lowercase
string sprintf(string fmt, ...)      // Format string
```

### Communication

```c
void send_device(string msg)         // Send to connection
void disconnect_device()             // Disconnect
int reconnect_device(object target)  // Transfer connection
string get_devconn(object obj)       // Get connection info
int interactive(object obj)          // Check if connected
object next_who(object obj)          // Next connected player
void broadcast(string msg)           // Send to all players
void tell_room(object room, object actor, object actee, int type, string msg, string pre, string post)
void tell_room_except(object room, object actor, object actee, int type, string msg, string pre, string post)
```

### Object Management

```c
void move_object(object obj, object dest) // Move object
object first_inventory(object container)  // First contained object
object next_inventory(object obj)         // Next contained object
object get_link(object obj)               // Get link/home
void set_priv(object obj, int flag)       // Set privilege
int priv(object obj)                      // Check privilege
```

### Compilation

```c
int compile_object(string path)      // Compile object
object load_object(string path)      // Load/compile object
int function_exists(string func, object obj) // Check function
```

### Utility

```c
int time()                           // Current time
string ctime(int time)               // Format time
int atoi(string s)                   // String to int
string itoa(int i)                   // Int to string
string crypt(string password, string salt) // Encrypt password
void alarm(int seconds, string func) // Schedule callback
void sync()                          // Save database
```

### Tables

```c
void table_set(string key, string value) // Set table entry
string table_get(string key)             // Get table entry
```

---

## Appendix C: Quick Start Checklist

### Creating a New Mudlib

- [ ] Create directory structure
- [ ] Create include files (types.h, flags.h, hear.h, sys.h)
- [ ] Create boot.c with required callbacks
- [ ] Create /obj/share/common.c
- [ ] Create /obj/room.c
- [ ] Create /sys/sys.c with utility functions
- [ ] Create /sys/login.c
- [ ] Create /obj/player.c
- [ ] Test connection and login
- [ ] Test basic commands
- [ ] Add more commands as needed

### Testing Checklist

- [ ] Can connect to server
- [ ] Can create character
- [ ] Can login with existing character
- [ ] Can see room description
- [ ] Can say something
- [ ] Can see other players
- [ ] Can quit and reconnect
- [ ] Auto-save works
- [ ] Passwords are encrypted
- [ ] File permissions work

---

## Appendix D: Glossary

**attach()** - Make another object's functions available to your object

**Boot Object** - The master object (object #0) that initializes the system

**Clone** - An instance of a prototype object

**Instance** - A copy of a prototype with a unique reference number

**Mudlib** - The application layer that runs on top of NetCI

**NLPC** - NetCI LPC, the scripting language for NetCI

**Prototype** - The original object defined by a source file

**Shared Object** - An object attached to provide common functionality

**UID** - User ID, identifies object ownership and privileges

**Virtual Filesystem** - The file system managed by NetCI for mudlib files

---

## Conclusion

You now have everything needed to build a working mudlib from scratch. The skeleton mudlib provides:

- ✅ Complete boot object with security
- ✅ Login and authentication system
- ✅ Player objects with basic commands
- ✅ Room system
- ✅ Shared object functionality
- ✅ Extensible command system

**Next Steps:**
1. Add more commands (movement, inventory, building)
2. Implement more object types (items, NPCs, exits)
3. Add help system
4. Implement save/load for players
5. Add more sophisticated security
6. Build your game world!

**Resources:**
- [Engine Extension Guide](ENGINE-EXTENSION-GUIDE.md) - For C developers
- [Technical Reference](.kiro/ci200fs-library/TECHNICAL-REFERENCE.md) - ci200fs reference
- [Modernization Guide](modernization.md) - NLPC vs modern LPC

---

**Document Version:** 1.0  
**Last Updated:** 2025-10-29  
**Status:** Complete

**Feedback:** If you find errors or have suggestions, please update this document.

