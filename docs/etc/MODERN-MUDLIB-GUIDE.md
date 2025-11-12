# Modern Mudlib Development Guide for NLPC

**Target Audience:** Mudlib developers building on NetCI/NLPC  
**Purpose:** Provide a modern mudlib skeleton using NLPC-native patterns  
**Scope:** Complete working implementation with daemon-based architecture

**IMPORTANT:** This guide is specifically for NLPC (NetCI LPC), which differs significantly from traditional LPC. NLPC uses composition instead of inheritance, tables instead of mappings, and has different system functions.

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [NLPC vs Traditional LPC](#2-nlpc-vs-traditional-lpc)
3. [Architecture Overview](#3-architecture-overview)
4. [Boot Object (Master Object)](#4-boot-object-master-object)
5. [Table Storage Patterns](#5-table-storage-patterns)
6. [Command Discovery System](#6-command-discovery-system)
7. [Login System with redirect_input()](#7-login-system-with-redirect_input)
8. [Daemon Architecture](#8-daemon-architecture)
9. [Object Composition Pattern](#9-object-composition-pattern)
10. [Standard Objects](#10-standard-objects)
11. [Configuration System](#11-configuration-system)
12. [Complete Implementation](#12-complete-implementation)
13. [Usage Guide](#13-usage-guide)
14. [Benefits and Trade-offs](#14-benefits-and-trade-offs)
15. [Migration Guide](#15-migration-guide)
16. [Conclusion](#16-conclusion)

---

## 1. Introduction

### 1.1 Why Modern Architecture?

Traditional LPMud 2.4.5 style mudlibs suffer from several problems:

**Monolithic player.c:**
- 100+ commands hardcoded in player.c
- 2000+ lines of code in a single file
- Every command requires modifying player.c
- Difficult to maintain and extend
- Hard for teams to work on simultaneously

**Example of the problem:**
```c
// Traditional player.c - 2000+ lines
static init() {
  add_verb("say", "do_say");
  add_verb("look", "do_look");
  add_verb("get", "do_get");
  add_verb("drop", "do_drop");
  // ... 100+ more commands
}

static do_say(arg) { /* 50 lines */ }
static do_look(arg) { /* 100 lines */ }
static do_get(arg) { /* 80 lines */ }
// ... 100+ more functions
```

**Modern architecture solves this:**
- Separation of concerns
- Small, focused files (20-50 lines each)
- Easy to add/modify commands
- Team-friendly development
- Maintainable codebase

### 1.2 What Makes It Modern?

**Key Features:**

1. **Command Discovery** - Commands auto-discovered from /cmds directory
2. **Daemon Services** - Singleton services (command_d, user_d, log_d)
3. **redirect_input()** - Clean login flow using NLPC's native feature
4. **Composition Pattern** - Objects use attach() for shared functionality
5. **Table Storage** - Global key-value storage with namespacing
6. **Configuration-Driven** - Centralized configuration
7. **MVC-like Separation** - Clear separation of concerns

### 1.3 Comparison with Basic Skeleton

**Basic Skeleton (Traditional style):**
```c
// player.c - 2000+ lines
add_verb("say", "do_say");
add_verb("look", "do_look");
// ... 100+ more commands

static do_say(arg) { ... }
static do_look(arg) { ... }
// ... 100+ more functions
```

**Modern Skeleton:**
```c
// player.c - 200 lines
listen(input) {
  string cmd, arg;
  int pos;
  
  // Parse input
  pos = instr(input, 1, " ");
  if (pos) {
    cmd = leftstr(input, pos - 1);
    arg = rightstr(input, strlen(input) - pos);
  } else {
    cmd = input;
    arg = NULL;
  }
  
  // Delegate to command_d
  atoo(COMMAND_D).execute_command(this_object(), cmd, arg);
}

// cmds/player/say.c - 20 lines
main(player, arg) {  // No return type in NLPC!
  if (!arg) {
    player.listen("Say what?\n");
    return 0;
  }
  player.listen("You say: " + arg + "\n");
  tell_room(location(player), player, NULL, HEAR_SAY,
            player.get_name() + " says: " + arg + "\n");
  return 1;
}

// cmds/player/look.c - 30 lines
main(player, arg) {
  object env;
  
  env = location(player);
  if (!env) {
    player.listen("You are in the void.\n");
    return 1;
  }
  player.listen(env.get_desc() + "\n");
  return 1;
}
```

**Benefits:**
- 20-50 lines per file vs 2000+ lines
- Add commands without touching player.c
- Easy to find and modify code
- Team can work on different commands simultaneously
- NLPC-native patterns (no LPC-isms)

---

## 2. NLPC vs Traditional LPC

### 2.1 Critical Differences

**NLPC is NOT traditional LPC!** If you're coming from LPMud, DGD, or other LPC drivers, you need to understand these fundamental differences:

| Feature | Traditional LPC | NLPC |
|---------|----------------|------|
| **Inheritance** | `inherit OBJECT;` | NO - Use composition with `attach()` |
| **Mappings** | `mapping m = ([]);` | NO - Use `table_set/get()` |
| **Return Types** | `void create() { }` | NO - Omit return type |
| **Player Output** | `write("text\n");` | `listen("text\n");` or `send_device()` |
| **Room Broadcast** | `say("text\n");` | `tell_room(room, from, to, type, msg)` |
| **Get Player** | `previous_object()` | `this_player()` or pass as parameter |
| **Arrays** | Dynamic `string *arr` | Fixed-size `string arr[100]` |
| **Generic Type** | `mixed` | `var` |

### 2.2 NLPC System Functions

**Available in NLPC:**
- `this_player()` - Current player object
- `this_object()` - Current object
- `caller_object()` - Calling object
- `add_verb(verb, func)` - Register command
- `listen(str)` - Send to player
- `send_device(str)` - Send to device
- `tell_room(room, from, to, type, msg)` - Broadcast to room
- `tell_player(player, from, to, type, msg)` - Send to player
- `redirect_input(func)` - Chain input to function
- `atoo(path)` - Load/find object
- `otoa(obj)` - Object to string
- `new(path)` - Clone object
- `destruct(obj)` - Destroy object
- `move_object(obj, dest)` - Move object
- `location(obj)` - Get container
- `present(name, container)` - Find object
- `find_player(name)` - Find player
- `find_object(name)` - Find object by name
- `attach(obj)` - Attach shared object (composition!)
- `detach(obj)` - Detach shared object
- `table_set(key, value)` - Store key-value (GLOBAL!)
- `table_get(key)` - Retrieve value (GLOBAL!)
- `table_delete(key)` - Delete key (GLOBAL!)
- `priv(obj)` - Check privileges
- `get_flags()` - Get player flags

**NOT available in NLPC:**
- `previous_object()` - Use `this_player()` or pass as param
- `this_user()` - Use `this_player()`
- `write()` - Use `listen()` or `send_device()`
- `say()` - Use `tell_room()`
- `add_command()` - Use `add_verb()`
- `mapping` type - Use `table_set/get()`
- `inherit` keyword - Use `attach()`
- `void` return type - Just omit it
- `mixed` type - Use `var`

### 2.3 Composition vs Inheritance

**Traditional LPC (Inheritance):**
```c
// object.c
string short_desc;
void set_short(string str) { short_desc = str; }
string query_short() { return short_desc; }

// room.c
inherit "/std/object";
mapping exits;
void add_exit(string dir, string dest) {
  exits += ([ dir : dest ]);
}
```

**NLPC (Composition):**
```c
// obj/object.c
object sharobj;

static init() {
  sharobj = new("/obj/share/common");
  attach(sharobj);  // Composition!
  set_type(TYPE_OBJECT);
}

// obj/room.c
object sharobj;

static init() {
  sharobj = new("/obj/share/common");
  attach(sharobj);  // Reuse common functionality
  set_type(TYPE_ROOM);
}
```

**Key Point:** NLPC objects use `attach()` to share functionality, not `inherit`. The shared object provides common methods that become available to the attaching object.

### 2.4 Table Storage Pattern

**CRITICAL:** `table_set/get` is GLOBAL! All objects share the same namespace. You MUST namespace your keys carefully.

**Pattern for Object-Specific Data:**
```c
// Store data specific to this object
set_property(key, value) {
  table_set(otoa(this_object()) + "_" + key, value);
}

get_property(key) {
  return table_get(otoa(this_object()) + "_" + key);
}

delete_property(key) {
  table_delete(otoa(this_object()) + "_" + key);
}
```

**Pattern for Daemon/Global Data:**
```c
// daemons/command_d.c - Singleton daemon
static init() {
  // Daemon-specific prefix
  table_set("cmd_d_initialized", 1);
  discover_commands();
}

register_command(cmd, path) {
  // Use daemon prefix for global command registry
  table_set("cmd_d_cmd_" + cmd, path);
}

get_command_path(cmd) {
  return table_get("cmd_d_cmd_" + cmd);
}
```

**Naming Convention:**
- Object data: `otoa(this_object()) + "_" + key`
- Daemon data: `daemon_name + "_" + key`
- Global data: `"global_" + key` (use sparingly!)

---

## 3. Architecture Overview

### 3.1 Directory Structure

```
/
├── boot.c                    # Master object
├── daemons/
│   ├── command_d.c          # Command discovery and routing
│   ├── user_d.c             # User management
│   ├── log_d.c              # Error logging and debugging
│   └── help_d.c             # Help system and documentation
├── cmds/
│   ├── player/              # Level 0 commands
│   │   ├── say.c
│   │   ├── look.c
│   │   ├── who.c
│   │   └── quit.c
│   ├── builder/             # Level 1 commands
│   │   ├── create.c
│   │   └── dig.c
│   └── admin/               # Level 2 commands
│       ├── shutdown.c
│       └── save.c
├── obj/
│   ├── object.c             # Base object (uses composition)
│   ├── player.c             # Base player
│   ├── room.c               # Base room
│   └── share/
│       └── common.c         # Shared functionality
├── rooms/
│   └── start/               # Starting area
│       ├── limbo.c
│       └── void.c
├── include/
│   ├── config.h             # Configuration
│   ├── daemons.h            # Daemon paths
│   ├── types.h              # Type constants
│   ├── flags.h              # Flag constants
│   ├── hear.h               # Hear type constants
│   └── log.h                # Log levels
└── sys/
    ├── login.c              # Login object (cloned per connection)
    └── sys.c                # Utilities
```

### 3.2 Component Responsibilities

**boot.c** - Master object
- System initialization
- Security policies (valid_read/valid_write)
- Connection handling (calls new("/sys/login"))
- Daemon initialization
- Required callbacks: connect(), valid_read(), valid_write()
- Optional callbacks: init(), finish_init(), save_db()

**Daemons** - Singleton services
- **command_d**: Command discovery and routing
- **user_d**: Player registry, user management
- **log_d**: Error logging, debugging, log levels
- **help_d**: Help system, command documentation, searchable help
- All daemons use table storage with daemon-specific prefixes

**Commands** - Individual command files
- One file per command
- Standard interface: `main(player, arg)`
- No return type declaration (NLPC style)
- Player passed as first parameter
- Return 1 for success, 0 for failure

**Objects** - Composition-based
- obj/object.c: Base object using attach()
- obj/player.c: Player-specific (minimal, delegates to command_d)
- obj/room.c: Room-specific
- obj/share/common.c: Shared functionality for all objects
- NO inheritance - uses composition via attach()

**Rooms** - Game areas
- Self-contained areas
- Own objects and rooms
- Can have area-specific commands

**Login System**
- sys/login.c: Per-connection login object (cloned)
- Uses redirect_input() for input flow
- Transfers to player object after authentication
- Each connection gets its own login object

### 3.3 Data Flow

**Connection Flow:**
```
TCP Connection
      ↓
boot.c::connect()
      ↓
new("/sys/login")  // Clone login object
      ↓
sys/login.c::connect()
      ↓
redirect_input("get_name")
      ↓
User enters name
      ↓
sys/login.c::get_name(input)
      ↓
redirect_input("get_password")
      ↓
User enters password
      ↓
sys/login.c::get_password(input)
      ↓
Authenticate
      ↓
reconnect_device(player)
      ↓
player.c::connect()
```

**Command Flow:**
```
Player types "say hello"
      ↓
player.c::listen("say hello")
      ↓
Parse: cmd="say", arg="hello"
      ↓
atoo(COMMAND_D).execute_command(player, "say", "hello")
      ↓
command_d looks up: "say" → "/cmds/player/say.c"
      ↓
load_object("/cmds/player/say.c")
      ↓
cmd_ob.main(player, "hello")
      ↓
Return result to player
```

---

## 3. Boot Object (Master Object)

### 3.1 What is boot.c?

The boot object (`/boot.c`) is the **master object** - the first object loaded by NetCI. It's always object #0 and has special system privileges.

**Responsibilities:**
- Initialize the system
- Handle new TCP connections
- Enforce security policies (valid_read/valid_write)
- Manage filesystem permissions
- Initialize daemons
- Provide system-wide services

**Key Point:** boot.c is like DGD's master object or LPMud's master.c - it's the interface between the engine and your mudlib.

### 3.2 Required Callbacks

The NetCI engine calls these functions in boot.c. You **MUST** implement them:

#### 3.2.1 connect()

**Called when:** A TCP connection is established

**Signature:**
```c
static connect()
```

**What it must do:**
1. Create a login object
2. Transfer the connection to it
3. Grant privileges
4. Start the login process

**Complete Implementation:**
```c
static connect() {
  object login_obj;
  
  // Create login object (cloned per connection)
  login_obj = new("/sys/login");
  if (!login_obj) {
    send_device("Unable to create login object.\n");
    atoo(LOG_D).log_error("Failed to create /sys/login");
    disconnect_device();
    return;
  }
  
  // Grant system privileges
  login_obj.set_secure();
  
  // Transfer connection from boot.c to login object
  if (reconnect_device(login_obj)) {
    send_device("Unable to transfer connection.\n");
    atoo(LOG_D).log_error("reconnect_device failed in boot.c::connect()");
    disconnect_device();
    destruct(login_obj);
    return;
  }
  
  // Grant privileges and start login
  set_priv(login_obj, 1);
  login_obj.connect();
}
```

**Common Mistakes:**
- ❌ Forgetting to call `login_obj.connect()` at the end
- ❌ Not checking if `new()` succeeded
- ❌ Not handling `reconnect_device()` failure
- ❌ Forgetting to grant privileges with `set_priv()`

#### 3.2.2 valid_read(path, func, caller, owner, flags)

**Called when:** Any read operation on the virtual filesystem (fread, cat, get_dir, etc.)

**Signature:**
```c
int valid_read(string path, string func, object caller, int owner, int flags)
```

**Parameters:**
- `path` - File path being accessed
- `func` - Function name that triggered the read (e.g., "fread", "cat")
- `caller` - Object attempting the read (0 for system)
- `owner` - Object ID of file owner
- `flags` - File permissions (-1 if file doesn't exist, otherwise bitmask: 1=READ_OK, 2=WRITE_OK)

**Returns:**
- 1 = Allow read
- 0 = Deny read

**Basic Implementation:**
```c
int valid_read(string path, string func, object caller, int owner, int flags) {
  // System operations - always allow
  if (!caller) return 1;
  
  // Privileged objects - always allow
  if (priv(caller)) return 1;
  
  // Non-existent files - allow read attempts
  if (flags == -1) return 1;
  
  // Owner can always read their files
  if (owner == otoi(caller)) return 1;
  
  // Files marked READ_OK (flag & 1)
  if (flags & 1) return 1;
  
  // Deny by default
  return 0;
}
```

**Role-Based Security (Modern Approach):**
```c
int valid_read(string path, string func, object caller, int owner, int flags) {
  int caller_flags;
  string caller_name;
  
  // System access - always allow
  if (!caller) return 1;
  if (priv(caller)) return 1;
  if (flags == -1) return 1;
  
  // Get caller info
  caller_flags = caller.get_flags();
  caller_name = caller.get_name();
  
  // Admin: Full read access except boot.c
  if (caller_flags & FLAG_ADMIN) {
    if (path == "/boot.c") return 0;
    return 1;
  }
  
  // Builder: Read all except boot.c and /sys
  if (caller_flags & FLAG_BUILDER) {
    if (path == "/boot.c") return 0;
    if (path == "/sys" || path_starts_with(path, "/sys/")) return 0;
    return 1;
  }
  
  // Regular user: owner, home directory, or READ_OK flag
  if (owner == otoi(caller)) return 1;
  if (caller_name && is_in_home_dir(path, caller_name)) return 1;
  if (flags & 1) return 1;
  
  return 0;
}
```

**Helper Functions:**
```c
// Check if path starts with prefix
static path_starts_with(path, prefix) {
  int prefix_len;
  if (!path || !prefix) return 0;
  prefix_len = strlen(prefix);
  if (strlen(path) < prefix_len) return 0;
  return (leftstr(path, prefix_len) == prefix);
}

// Check if path is in user's home directory
static is_in_home_dir(path, username) {
  int next_slash;
  string path_user;
  if (!path || !username) return 0;
  if (!path_starts_with(path, "/home/")) return 0;
  next_slash = instr(path, 7, "/");
  if (!next_slash) return 0;
  path_user = midstr(path, 7, next_slash - 7);
  return (downcase(path_user) == downcase(username));
}
```

#### 3.2.3 valid_write(path, func, caller, owner, flags)

**Called when:** Any write operation on the virtual filesystem (fwrite, mkdir, rm, etc.)

**Signature:**
```c
int valid_write(string path, string func, object caller, int owner, int flags)
```

**Parameters:** Same as valid_read()

**Returns:**
- 1 = Allow write
- 0 = Deny write

**Basic Implementation:**
```c
int valid_write(string path, string func, object caller, int owner, int flags) {
  // System operations - always allow
  if (!caller) return 1;
  
  // Privileged objects - always allow
  if (priv(caller)) return 1;
  
  // Owner can write their files
  if (owner == otoi(caller)) return 1;
  
  // Files marked WRITE_OK (flag & 2)
  if (flags & 2) return 1;
  
  // Deny by default
  return 0;
}
```

**Role-Based Security (Modern Approach):**
```c
int valid_write(string path, string func, object caller, int owner, int flags) {
  int caller_flags, caller_id;
  string caller_name;
  
  // System access - always allow
  if (!caller) return 1;
  if (priv(caller)) return 1;
  
  // Get caller info
  caller_flags = caller.get_flags();
  caller_name = caller.get_name();
  caller_id = otoi(caller);
  
  // Admin: Full write access except boot.c
  if (caller_flags & FLAG_ADMIN) {
    if (path == "/boot.c") return 0;
    return 1;
  }
  
  // Builder: Write only in /domains and /etc
  if (caller_flags & FLAG_BUILDER) {
    if (path == "/boot.c") return 0;
    if (path == "/sys" || path_starts_with(path, "/sys/")) return 0;
    if (!path_starts_with(path, "/domains/") && 
        !path_starts_with(path, "/etc/")) return 0;
    // Allow creating new files, owner files, or WRITE_OK files
    if (flags == -1) return 1;
    if (owner == caller_id) return 1;
    if (flags & 2) return 1;
    return 0;
  }
  
  // Regular user: Write only in home directory
  if (caller_name && is_in_home_dir(path, caller_name)) {
    if (flags == -1) return 1;
    if (owner == caller_id) return 1;
    if (flags & 2) return 1;
    return 0;
  }
  
  return 0;
}
```

### 3.3 Optional Callbacks

These are commonly implemented but not strictly required:

#### 3.3.1 init()

**Called when:** boot.c is first loaded

**Use for:**
- Initial system setup
- Setting file permissions
- Starting autosave
- Loading initial packages

```c
static init() {
  object me;
  
  me = this_object();
  
  // Only run on prototype (not clones)
  if (prototype(this_object())) {
    // Start autosave
    alarm(SAVE_DELAY, "save_db");
    
    // Make boot.c readable
    chmod("/boot.c", 1);
    
    // Create initial directories
    unhide("/etc", me, 5);
    unhide("/data", me, 7);
    unhide("/home", me, 7);
    
    // Initialize daemons after a moment
    alarm(0, "finish_init");
  } else {
    // Clones should self-destruct
    destruct(this_object());
  }
}
```

#### 3.3.2 finish_init()

**Called by:** alarm() from init()

**Use for:**
- Initializing daemons
- Creating initial objects
- Setting up admin account

```c
static finish_init() {
  object daemon, admin, room;
  
  // Initialize daemons
  daemon = load_object(LOG_D);
  set_priv(daemon, 1);
  
  daemon = load_object(COMMAND_D);
  set_priv(daemon, 1);
  
  daemon = load_object(USER_D);
  set_priv(daemon, 1);
  
  // Create starting room
  room = atoo("/domains/start/limbo");
  if (!room) {
    room = new("/std/room");
    room.set_name("Limbo");
    room.set_short("Limbo");
    room.set_long("You are floating in a gray void.");
  }
  
  // Create admin account
  admin = new("/std/player");
  admin.set_secure();
  admin.set_name("Admin");
  admin.set_password("changeme");
  admin.set_level(2);  // Admin level
  admin.set_flags(FLAG_ADMIN);
  admin.set_link(room);
  move_object(admin, room);
  set_priv(admin, 1);
  
  atoo(USER_D).add_player("Admin", admin);
  
  atoo(LOG_D).log_info("System initialization complete");
}
```

#### 3.3.3 save_db()

**Called by:** alarm() periodically

**Use for:**
- Autosaving the database
- Notifying players

```c
static save_db() {
  object curr;
  
  // Notify all players
  while (curr = next_who(curr)) {
    curr.send_device("\n*** Autosaving Database ***\n");
  }
  
  flush_device();
  
  // Trigger database save
  sysctl(0);
  
  // Schedule next save
  alarm(SAVE_DELAY, "save_db");
}
```

### 3.4 Complete Modern boot.c

Here's a complete boot.c for the modern mudlib:

```c
// /boot.c - Modern mudlib master object

#include <config.h>
#include <daemons.h>

#define SAVE_DELAY 3600  // Autosave every hour

// Initialize system
static init() {
  object me;
  
  me = this_object();
  
  if (prototype(this_object())) {
    // Start autosave
    alarm(SAVE_DELAY, "save_db");
    
    // Make boot.c readable
    chmod("/boot.c", 1);
    
    // Create initial directories
    unhide("/etc", me, 5);
    unhide("/data", me, 7);
    unhide("/home", me, 7);
    unhide("/domains", me, 5);
    unhide("/cmds", me, 5);
    
    // Initialize daemons after a moment
    alarm(0, "finish_init");
  } else {
    destruct(this_object());
  }
}

// Initialize daemons and create initial objects
static finish_init() {
  object daemon, admin, room;
  
  // Initialize daemons (log_d first!)
  daemon = load_object(LOG_D);
  set_priv(daemon, 1);
  
  daemon = load_object(COMMAND_D);
  set_priv(daemon, 1);
  
  daemon = load_object(USER_D);
  set_priv(daemon, 1);
  
  // Create starting room
  room = new("/std/room");
  room.set_name("Limbo");
  room.set_short("Limbo");
  room.set_long("You are floating in a gray void.");
  
  // Create admin account
  admin = new("/std/player");
  admin.set_secure();
  admin.set_name("Admin");
  admin.set_password("changeme");
  admin.set_level(2);
  admin.set_link(room);
  move_object(admin, room);
  set_priv(admin, 1);
  
  atoo(USER_D).add_player("Admin", admin);
  
  atoo(LOG_D).log_info("System initialization complete");
}

// Handle new connections
static connect() {
  object login_obj;
  
  login_obj = new("/sys/login");
  if (!login_obj) {
    send_device("Unable to create login object.\n");
    atoo(LOG_D).log_error("Failed to create /sys/login");
    disconnect_device();
    return;
  }
  
  login_obj.set_secure();
  
  if (reconnect_device(login_obj)) {
    send_device("Unable to transfer connection.\n");
    atoo(LOG_D).log_error("reconnect_device failed");
    disconnect_device();
    destruct(login_obj);
    return;
  }
  
  set_priv(login_obj, 1);
  login_obj.connect();
}

// File read permissions
int valid_read(string path, string func, object caller, int owner, int flags) {
  int caller_level;
  string caller_name;
  
  // System access
  if (!caller) return 1;
  if (priv(caller)) return 1;
  if (flags == -1) return 1;
  
  // Get caller info
  if (function_exists("query_level", caller)) {
    caller_level = caller.query_level();
  } else {
    caller_level = 0;
  }
  
  if (function_exists("query_name", caller)) {
    caller_name = caller.query_name();
  }
  
  // Admin (level 2): Full read except boot.c
  if (caller_level >= 2) {
    if (path == "/boot.c") return 0;
    return 1;
  }
  
  // Builder (level 1): Read all except boot.c and /sys
  if (caller_level >= 1) {
    if (path == "/boot.c") return 0;
    if (path == "/sys" || path_starts_with(path, "/sys/")) return 0;
    return 1;
  }
  
  // Regular user
  if (owner == otoi(caller)) return 1;
  if (caller_name && is_in_home_dir(path, caller_name)) return 1;
  if (flags & 1) return 1;
  
  return 0;
}

// File write permissions
int valid_write(string path, string func, object caller, int owner, int flags) {
  int caller_level, caller_id;
  string caller_name;
  
  // System access
  if (!caller) return 1;
  if (priv(caller)) return 1;
  
  // Get caller info
  caller_id = otoi(caller);
  
  if (function_exists("query_level", caller)) {
    caller_level = caller.query_level();
  } else {
    caller_level = 0;
  }
  
  if (function_exists("query_name", caller)) {
    caller_name = caller.query_name();
  }
  
  // Admin (level 2): Full write except boot.c
  if (caller_level >= 2) {
    if (path == "/boot.c") return 0;
    return 1;
  }
  
  // Builder (level 1): Write in /domains and /etc
  if (caller_level >= 1) {
    if (path == "/boot.c") return 0;
    if (path == "/sys" || path_starts_with(path, "/sys/")) return 0;
    if (!path_starts_with(path, "/domains/") && 
        !path_starts_with(path, "/etc/")) return 0;
    if (flags == -1) return 1;
    if (owner == caller_id) return 1;
    if (flags & 2) return 1;
    return 0;
  }
  
  // Regular user: home directory only
  if (caller_name && is_in_home_dir(path, caller_name)) {
    if (flags == -1) return 1;
    if (owner == caller_id) return 1;
    if (flags & 2) return 1;
    return 0;
  }
  
  return 0;
}

// Helper: Check if path starts with prefix
static path_starts_with(path, prefix) {
  int prefix_len;
  if (!path || !prefix) return 0;
  prefix_len = strlen(prefix);
  if (strlen(path) < prefix_len) return 0;
  return (leftstr(path, prefix_len) == prefix);
}

// Helper: Check if path is in user's home directory
static is_in_home_dir(path, username) {
  int next_slash;
  string path_user;
  if (!path || !username) return 0;
  if (!path_starts_with(path, "/home/")) return 0;
  next_slash = instr(path, 7, "/");
  if (!next_slash) return 0;
  path_user = midstr(path, 7, next_slash - 7);
  return (downcase(path_user) == downcase(username));
}

// Autosave database
static save_db() {
  object curr;
  
  while (curr = next_who(curr)) {
    curr.send_device("\n*** Autosaving Database ***\n");
  }
  
  flush_device();
  sysctl(0);
  alarm(SAVE_DELAY, "save_db");
}

// For debugging
listen(arg) {
  send_device(arg);
}
```

### 3.5 Security Best Practices

**DO:**
- ✅ Always protect boot.c from modification
- ✅ Protect /sys directory from non-privileged access
- ✅ Use level-based or role-based access control
- ✅ Log security violations
- ✅ Validate all inputs
- ✅ Use helper functions for common checks

**DON'T:**
- ❌ Allow write access to boot.c
- ❌ Trust user input in security checks
- ❌ Forget to check function_exists() before calling
- ❌ Use hardcoded names (use query_name())
- ❌ Assume caller is always valid

**Example Security Violation Logging:**
```c
int valid_write(string path, string func, object caller, int owner, int flags) {
  // ... security checks ...
  
  // If denied, log it
  if (result == 0) {
    atoo(LOG_D).log_warn("Write denied: " + 
                         caller.query_name() + 
                         " attempted to write " + path);
  }
  
  return result;
}
```

---

## 5. Table Storage Patterns

### 5.1 Why Tables?

NLPC doesn't have mappings like traditional LPC. Instead, it provides **global table storage** via `table_set()`, `table_get()`, and `table_delete()`.

**CRITICAL:** Tables are GLOBAL! All objects share the same namespace. You MUST namespace your keys carefully to avoid collisions.

### 5.2 Namespacing Patterns

**Object-Specific Data:**
```c
// Pattern: otoa(this_object()) + "_" + key
set_property(key, value) {
  table_set(otoa(this_object()) + "_" + key, value);
}

get_property(key) {
  return table_get(otoa(this_object()) + "_" + key);
}

delete_property(key) {
  table_delete(otoa(this_object()) + "_" + key);
}
```

**Daemon-Specific Data:**
```c
// Pattern: "daemon_name_" + key

// daemons/command_d.c
register_command(cmd, path) {
  table_set("cmd_d_cmd_" + cmd, path);
}

get_command_path(cmd) {
  return table_get("cmd_d_cmd_" + cmd);
}

// daemons/user_d.c
register_user(name, obj) {
  table_set("user_d_player_" + downcase(name), otoa(obj));
}

find_user(name) {
  string obj_str;
  obj_str = table_get("user_d_player_" + downcase(name));
  if (!obj_str) return NULL;
  return atoo(obj_str);
}

// daemons/help_d.c
register_help(topic, text) {
  table_set("help_d_topic_" + downcase(topic), text);
}

get_help(topic) {
  return table_get("help_d_topic_" + downcase(topic));
}
```

**Global Data (Use Sparingly):**
```c
// Pattern: "global_" + key
table_set("global_mud_name", "My MUD");
table_set("global_uptime", time());
```

### 5.3 Iteration Patterns

Since NLPC doesn't have mapping iteration, you need to track keys separately:

**Pattern 1: Counter-Based**
```c
// Store count
register_command(cmd, path) {
  int count;
  
  table_set("cmd_d_cmd_" + cmd, path);
  
  // Increment count
  count = table_get("cmd_d_count");
  if (!count) count = 0;
  count++;
  table_set("cmd_d_count", count);
  
  // Store in indexed list
  table_set("cmd_d_list_" + itoa(count), cmd);
}

// Iterate
list_commands() {
  int i, count;
  string cmd;
  
  count = table_get("cmd_d_count");
  if (!count) return;
  
  for (i = 1; i <= count; i++) {
    cmd = table_get("cmd_d_list_" + itoa(i));
    if (cmd) {
      // Process command
    }
  }
}
```

**Pattern 2: Fixed-Size Array**
```c
// Store in array
string cmd_list[100];
int cmd_count;

register_command(cmd, path) {
  if (cmd_count >= 100) {
    atoo(LOG_D).log_error("Command list full!");
    return 0;
  }
  
  table_set("cmd_d_cmd_" + cmd, path);
  cmd_list[cmd_count] = cmd;
  cmd_count++;
  return 1;
}

// Iterate
list_commands() {
  int i;
  
  for (i = 0; i < cmd_count; i++) {
    // Process cmd_list[i]
  }
}
```

### 5.4 Cleanup Patterns

**Delete Object Data:**
```c
// When object is destroyed
recycle() {
  string prefix;
  int i;
  string keys[50];  // List of keys to delete
  int key_count;
  
  prefix = otoa(this_object()) + "_";
  
  // Build list of keys (you need to track these!)
  keys[0] = prefix + "name";
  keys[1] = prefix + "desc";
  keys[2] = prefix + "level";
  key_count = 3;
  
  // Delete all
  for (i = 0; i < key_count; i++) {
    table_delete(keys[i]);
  }
  
  destruct(this_object());
}
```

**Delete Daemon Data:**
```c
// daemons/command_d.c
unregister_command(cmd) {
  table_delete("cmd_d_cmd_" + cmd);
  
  // Also remove from list
  // (Implementation depends on your iteration pattern)
}
```

### 5.5 Best Practices

**DO:**
- ✅ Always use consistent prefixes
- ✅ Use `otoa(this_object())` for object-specific data
- ✅ Use daemon name prefix for daemon data
- ✅ Track keys for iteration and cleanup
- ✅ Check for NULL returns from `table_get()`
- ✅ Use `downcase()` for case-insensitive keys

**DON'T:**
- ❌ Use bare keys without prefixes
- ❌ Assume `table_get()` returns valid data
- ❌ Forget to clean up when objects are destroyed
- ❌ Use spaces or special characters in keys
- ❌ Store objects directly (use `otoa()` first)

**Example - Complete Object with Properties:**
```c
// obj/object.c
object sharobj;

static init() {
  sharobj = new("/obj/share/common");
  attach(sharobj);
  set_type(TYPE_OBJECT);
}

set_name(name) {
  table_set(otoa(this_object()) + "_name", name);
}

get_name() {
  return table_get(otoa(this_object()) + "_name");
}

set_desc(desc) {
  table_set(otoa(this_object()) + "_desc", desc);
}

get_desc() {
  return table_get(otoa(this_object()) + "_desc");
}

recycle() {
  // Clean up our table entries
  table_delete(otoa(this_object()) + "_name");
  table_delete(otoa(this_object()) + "_desc");
  
  sharobj.recycle();
  destruct(this_object());
}
```

---

## 6. Command Discovery System

### 6.1 How It Works

**Discovery Process:**

1. command_d scans /cmds directory on boot
2. Stores commands in table: `"cmd_d_cmd_" + name → path`
3. Stores aliases in table: `"cmd_d_alias_" + alias → command`
4. Tracks command list for iteration
5. Reloads on demand or schedule

**Execution Flow:**

```
Player types "say hello"
      ↓
player.c::listen("say hello")
      ↓
Parse: cmd="say", arg="hello"
      ↓
atoo(COMMAND_D).execute_command(player, "say", "hello")
      ↓
Look up: table_get("cmd_d_cmd_say") → "/cmds/player/say.c"
      ↓
Load: load_object("/cmds/player/say.c")
      ↓
Execute: cmd_ob.main(player, "hello")
      ↓
Return result to player
```

### 6.2 Command File Pattern

**Standard Interface:**

Every command file must implement this interface:

```c
// cmds/player/say.c

// Main entry point - REQUIRED
// No return type declaration in NLPC!
main(player, arg) {
  if (!arg || arg == "") {
    player.listen("Say what?\n");
    return 0;
  }
  
  // Send to player
  player.listen("You say: " + arg + "\n");
  
  // Broadcast to room (NLPC uses tell_room, not say!)
  tell_room(location(player), player, NULL, HEAR_SAY,
            player.get_name() + " says: " + arg + "\n");
  
  return 1;
}

// Help text - OPTIONAL
query_help() {
  return "Usage: say <message>\n" +
         "Say something to everyone in the room.\n" +
         "\n" +
         "Example: say Hello everyone!";
}

// Command aliases - OPTIONAL
query_aliases() {
  string aliases[2];
  aliases[0] = "'";
  aliases[1] = "\"";
  return aliases;
}

// Access level - OPTIONAL (default: 0)
// 0=player, 1=builder, 2=admin
query_level() {
  return 0;
}
```

**Key Points:**
- `main(player, arg)` is REQUIRED - no return type!
- Returns 1 for success, 0 for failure
- Player object is passed as first parameter
- Argument string is passed as second parameter
- Use `player.listen()` not `write()`
- Use `tell_room()` not `say()`
- All other functions are optional

### 6.3 command_d Implementation

**Complete Implementation:**

```c
// daemons/command_d.c

#include <config.h>
#include <daemons.h>
#include <log.h>

// Track commands for iteration
string cmd_list[200];
int cmd_count;

// Prevent cloning
static init() {
  if (!prototype(this_object())) {
    destruct(this_object());
    return;
  }
  
  table_set("cmd_d_initialized", 1);
  discover_commands();
}

// Scan /cmds directory and register commands
discover_commands() {
  string dirs[3];
  string dir, file, cmd_name, path;
  int i, j, k, file_count;
  object cmd_ob;
  string alias_list[10];
  int alias_count;
  
  // Reset
  cmd_count = 0;
  
  // Scan each level directory
  dirs[0] = "player";
  dirs[1] = "builder";
  dirs[2] = "admin";
  
  for (i = 0; i < 3; i++) {
    dir = "/cmds/" + dirs[i];
    
    // Get .c files (you'll need to implement file listing)
    // For now, manually register known commands
    if (dirs[i] == "player") {
      register_command("say", "/cmds/player/say.c");
      register_command("look", "/cmds/player/look.c");
      register_command("who", "/cmds/player/who.c");
      register_command("quit", "/cmds/player/quit.c");
      register_command("help", "/cmds/player/help.c");
    }
  }
  
  atoo(LOG_D).log_info("Discovered " + itoa(cmd_count) + " commands");
}

// Register a command
register_command(cmd, path) {
  object cmd_ob;
  string alias_list[10];
  int alias_count, i;
  
  if (cmd_count >= 200) {
    atoo(LOG_D).log_error("Command list full!");
    return 0;
  }
  
  // Store command
  table_set("cmd_d_cmd_" + cmd, path);
  cmd_list[cmd_count] = cmd;
  cmd_count++;
  
  // Load and check for aliases
  cmd_ob = load_object(path);
  if (!cmd_ob) return 1;
  
  if (function_exists("query_aliases", cmd_ob)) {
    alias_list = cmd_ob.query_aliases();
    if (alias_list) {
      alias_count = sizeof(alias_list);
      for (i = 0; i < alias_count; i++) {
        if (alias_list[i]) {
          table_set("cmd_d_alias_" + alias_list[i], cmd);
        }
      }
    }
  }
  
  return 1;
}

// Execute a command
execute_command(player, cmd, arg) {
  string file_path, real_cmd;
  object cmd_ob;
  int result, level, player_level;
  
  if (!player || !cmd) return 0;
  
  // Check for alias
  real_cmd = table_get("cmd_d_alias_" + cmd);
  if (!real_cmd) real_cmd = cmd;
  
  // Look up command
  file_path = table_get("cmd_d_cmd_" + real_cmd);
  if (!file_path) {
    return 0;  // Command not found
  }
  
  // Load command object
  cmd_ob = load_object(file_path);
  if (!cmd_ob) {
    atoo(LOG_D).log_error("Failed to load command: " + file_path);
    return 0;
  }
  
  // Check access level
  if (function_exists("query_level", cmd_ob)) {
    level = cmd_ob.query_level();
    player_level = player.query_level();
    if (level > player_level) {
      player.listen("You don't have permission to use that command.\n");
      return 0;
    }
  }
  
  // Execute command
  if (!function_exists("main", cmd_ob)) {
    atoo(LOG_D).log_error("Command missing main(): " + file_path);
    return 0;
  }
  
  result = cmd_ob.main(player, arg);
  return result;
}

// Get help for a command
query_help(cmd) {
  string file_path, real_cmd;
  object cmd_ob;
  
  // Check for alias
  real_cmd = table_get("cmd_d_alias_" + cmd);
  if (!real_cmd) real_cmd = cmd;
  
  file_path = table_get("cmd_d_cmd_" + real_cmd);
  if (!file_path) return NULL;
  
  cmd_ob = load_object(file_path);
  if (!cmd_ob) return NULL;
  
  if (function_exists("query_help", cmd_ob)) {
    return cmd_ob.query_help();
  }
  
  return "No help available for " + cmd + ".\n";
}

// List all commands (for help system)
list_commands() {
  int i;
  string result, cmd;
  
  result = "Available commands:\n";
  
  for (i = 0; i < cmd_count; i++) {
    cmd = cmd_list[i];
    if (cmd) {
      result = result + "  " + cmd + "\n";
    }
  }
  
  return result;
}
  
  // Scan each level directory
  dirs = ({ "player", "builder", "admin" });
  
  for (i = 0; i < sizeof(dirs); i++) {
    dir = "/cmds/" + dirs[i];
    files = get_dir(dir + "/*.c");
    
    if (!files) continue;
    
    for (j = 0; j < sizeof(files); j++) {
      // Remove .c extension
      cmd_name = leftstr(files[j], strlen(files[j]) - 2);
      file = dir + "/" + files[j];
      
      // Register command
      commands[cmd_name] = file;
      
      // Load to check for aliases
      cmd_ob = load_object(file);
      if (cmd_ob && function_exists("query_aliases", cmd_ob)) {
        cmd_aliases = cmd_ob.query_aliases();
        if (cmd_aliases) {
          for (int k = 0; k < sizeof(cmd_aliases); k++) {
            aliases[cmd_aliases[k]] = cmd_name;
          }
        }
      }
    }
  }
  
  atoo(LOG_D).log_info("Discovered " + itoa(sizeof(commands)) + " commands");
}

// Execute a command
int execute_command(object player, string cmd, string arg) {
  string file_path, real_cmd;
  object cmd_ob;
  int result, level;
  
  if (!player || !cmd) return 0;
  
  // Check for alias
  if (aliases[cmd]) {
    real_cmd = aliases[cmd];
  } else {
    real_cmd = cmd;
  }
  
  // Look up command
  file_path = commands[real_cmd];
  if (!file_path) {
    return 0;  // Command not found
  }
  
  // Load command object
  cmd_ob = load_object(file_path);
  if (!cmd_ob) {
    atoo(LOG_D).log_error("Failed to load command: " + file_path);
    return 0;
  }
  
  // Check access level
  if (function_exists("query_level", cmd_ob)) {
    level = cmd_ob.query_level();
    if (level > player.query_level()) {
      player.send_device("You don't have permission to use that command.\n");
      return 0;
    }
  }
  
  // Execute command
  if (!function_exists("main", cmd_ob)) {
    atoo(LOG_D).log_error("Command missing main(): " + file_path);
    return 0;
  }
  
  result = cmd_ob.main(player, arg);
  return result;
}

// Get help for a command
string query_help(string cmd) {
  string file_path, real_cmd;
  object cmd_ob;
  
  // Check for alias
  if (aliases[cmd]) {
    real_cmd = aliases[cmd];
  } else {
    real_cmd = cmd;
  }
  
  file_path = commands[real_cmd];
  if (!file_path) return 0;
  
  cmd_ob = load_object(file_path);
  if (!cmd_ob) return 0;
  
  if (function_exists("query_help", cmd_ob)) {
    return cmd_ob.query_help();
  }
  
  return "No help available for " + cmd + ".\n";
}

// List available commands for a player
string *query_commands(int level) {
  string *cmd_list, *keys;
  int i;
  object cmd_ob;
  int cmd_level;
  
  cmd_list = ({});
  keys = map_indices(commands);
  
  for (i = 0; i < sizeof(keys); i++) {
    cmd_ob = load_object(commands[keys[i]]);
    if (!cmd_ob) continue;
    
    if (function_exists("query_level", cmd_ob)) {
      cmd_level = cmd_ob.query_level();
    } else {
      cmd_level = 0;
    }
    
    if (cmd_level <= level) {
      cmd_list += ({ keys[i] });
    }
  }
  
  return cmd_list;
}

// Reload commands (for development)
void reload_commands() {
  commands = ([]);
  aliases = ([]);
  discover_commands();
}
```

**Benefits:**
- Commands auto-discovered on boot
- No need to modify player.c
- Easy to add new commands
- Caching for performance
- Alias support built-in

---

## 4. Login System with redirect_input()

### 4.1 Understanding redirect_input()

NetCI provides `redirect_input(function_name)` which redirects the next line of input to a specified function. This is similar to MudOS/FluffOS `input_to()`.

**Important Limitation:** `redirect_input()` can only redirect to functions in the **same object** that called it. You cannot redirect to functions in a different object (like a daemon).

**How it works:**

```c
// In sys/login.c
connect() {
  send_device("Welcome!\nEnter your name: ");
  redirect_input("get_name");
}

get_name(input) {
  // Process name
  send_device("Enter password: ");
  redirect_input("get_password");
}

get_password(input) {
  // Process password
  // Transfer to player or continue
}
```

**Benefits:**
- Simpler than state machines
- Each handler is a separate function
- Easy to follow flow
- Native NetCI feature
- Non-blocking (execution continues after redirect_input)

### 4.2 Login Flow

```
connect()
   ↓ redirect_input("get_name")
get_name(input)
   ↓ redirect_input("get_password") [existing player]
   ↓ redirect_input("get_new_password") [new player]
get_password(input)
   ↓ authenticate and transfer to player
get_new_password(input)
   ↓ redirect_input("confirm_password")
confirm_password(input)
   ↓ create player and transfer
```

### 4.3 sys/login.c Implementation

**Complete working implementation:**

```c
// sys/login.c

#include <daemons.h>
#include <config.h>

string player_name;
string temp_password;

// Called when connection is established
connect() {
  send_device("\n");
  send_device("========================================\n");
  send_device("  Welcome to " + MUD_NAME + "\n");
  send_device("========================================\n");
  send_device("\n");
  send_device("Enter your name (or 'new' for new character): ");
  redirect_input("get_name");
}

// Handle name input
get_name(input) {
  input = downcase(input);
  
  // Check for new player
  if (input == "new") {
    send_device("Choose a name: ");
    redirect_input("get_new_name");
    return;
  }
  
  // Validate name
  if (!is_legal_name(input)) {
    send_device("Invalid name. Names must be 3-12 letters only.\n");
    send_device("Enter your name: ");
    redirect_input("get_name");
    return;
  }
  
  // Check if player exists
  if (!atoo(USER_D).find_player(input)) {
    send_device("No such player. Try again or type 'new': ");
    redirect_input("get_name");
    return;
  }
  
  player_name = input;
  send_device("Password: ");
  redirect_input("get_password");
}

// Handle password for existing player
get_password(input) {
  object player;
  
  player = atoo(USER_D).find_player(player_name);
  if (!player) {
    send_device("Player not found.\n");
    disconnect_device();
    destruct(this_object());
    return;
  }
  
  if (!player.check_password(input)) {
    send_device("Incorrect password.\n");
    atoo(LOG_D).log_warn("Failed login attempt for " + player_name);
    disconnect_device();
    destruct(this_object());
    return;
  }
  
  // Successful login
  atoo(LOG_D).log_info(player_name + " logged in");
  
  // Transfer connection to player
  if (reconnect_device(player)) {
    send_device("Unable to transfer connection.\n");
    disconnect_device();
    destruct(this_object());
    return;
  }
  
  player.connect();
  destruct(this_object());
}

// Handle name input for new player
get_new_name(input) {
  input = downcase(input);
  
  if (!is_legal_name(input)) {
    send_device("Invalid name. Names must be 3-12 letters only.\n");
    send_device("Choose a name: ");
    redirect_input("get_new_name");
    return;
  }
  
  if (atoo(USER_D).find_player(input)) {
    send_device("That name is already taken.\n");
    send_device("Choose a name: ");
    redirect_input("get_new_name");
    return;
  }
  
  player_name = input;
  send_device("Choose a password (at least 6 characters): ");
  redirect_input("get_new_password");
}

// Handle password for new player
get_new_password(input) {
  if (strlen(input) < 6) {
    send_device("Password too short. At least 6 characters required.\n");
    send_device("Choose a password: ");
    redirect_input("get_new_password");
    return;
  }
  
  temp_password = input;
  send_device("Confirm password: ");
  redirect_input("confirm_password");
}

// Confirm password for new player
confirm_password(input) {
  object player;
  
  if (input != temp_password) {
    send_device("Passwords don't match.\n");
    send_device("Choose a password: ");
    redirect_input("get_new_password");
    return;
  }
  
  // Create new player
  player = new("/std/player");
  if (!player) {
    send_device("Failed to create player.\n");
    atoo(LOG_D).log_error("Failed to create player: " + player_name);
    disconnect_device();
    destruct(this_object());
    return;
  }
  
  player.set_secure();
  player.set_name(player_name);
  player.set_password(temp_password);
  player.set_level(0);  // Player level
  
  // Add to user registry
  atoo(USER_D).add_player(player_name, player);
  
  // Move to starting room
  player.set_link(atoo("/domains/start/limbo"));
  move_object(player, atoo("/domains/start/limbo"));
  
  atoo(LOG_D).log_info("New player created: " + player_name);
  
  // Transfer connection
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

// Handle disconnection
disconnect() {
  destruct(this_object());
}

// Validate name
int is_legal_name(string name) {
  int i, len;
  
  if (!name) return 0;
  
  len = strlen(name);
  if (len < 3 || len > 12) return 0;
  
  for (i = 1; i <= len; i++) {
    if (!is_alpha(midstr(name, i, 1))) {
      return 0;
    }
  }
  
  return 1;
}
```

**Key Points:**
- Each input handler is a separate function
- Use `redirect_input()` to chain handlers
- Always validate input before proceeding
- Use `reconnect_device()` to transfer connection
- Destruct login object after transfer
- Log important events (logins, failures, new players)

### 4.4 Error Handling

**Common Issues:**

1. **reconnect_device() fails:**
   - Always check return value
   - Disconnect and destruct on failure
   - Log the error

2. **Player object creation fails:**
   - Check if new() returns valid object
   - Clean up and disconnect on failure
   - Log the error

3. **Invalid input:**
   - Always validate before processing
   - Give clear error messages
   - Loop back to same input handler

**Example:**
```c
if (reconnect_device(player)) {
  send_device("Unable to transfer connection.\n");
  atoo(LOG_D).log_error("reconnect_device failed for " + player_name);
  disconnect_device();
  destruct(this_object());
  return;
}
```

---

## 7. Daemon Architecture

### 7.1 Singleton Pattern

Daemons are **singleton** objects - only one instance exists. They provide services to all players and objects.

**Daemon Access:**

```c
// In include/daemons.h
#define COMMAND_D "/daemons/command_d"
#define USER_D "/daemons/user_d"
#define LOG_D "/daemons/log_d"
#define HELP_D "/daemons/help_d"

// Usage anywhere (note: use dot notation, not arrow)
atoo(COMMAND_D).execute_command(this_player(), cmd, arg);

// Or cache the daemon object
object cmd_d;
cmd_d = atoo(COMMAND_D);
cmd_d.execute_command(this_player(), cmd, arg);
```

**Important:** Use **dot notation** (`.`) not arrow notation (`->`) for method calls in NLPC.

### 7.2 log_d Implementation

**Purpose:** Centralized error logging and debugging

```c
// daemons/log_d.c

#include <config.h>
#include <log.h>

#define LOG_FILE "/data/logs/mud.log"

int log_level;  // Current log level
string log_buffer[100];  // Fixed-size buffer
int log_count;

// Prevent cloning
static init() {
  if (!prototype(this_object())) {
    destruct(this_object());
    return;
  }
  
  log_level = LOG_INFO;  // Default level
  log_count = 0;
  table_set("log_d_initialized", 1);
}

// Set log level
set_log_level(level) {
  log_level = level;
}

// Log error message
log_error(msg) {
  if (log_level <= LOG_ERROR) {
    write_log("ERROR", msg);
  }
}

// Log warning message
log_warn(msg) {
  if (log_level <= LOG_WARN) {
    write_log("WARN", msg);
  }
}

// Log info message
log_info(msg) {
  if (log_level <= LOG_INFO) {
    write_log("INFO", msg);
  }
}

// Log debug message
log_debug(msg) {
  if (log_level <= LOG_DEBUG) {
    write_log("DEBUG", msg);
  }
}

// Write to log file
static write_log(level, msg) {
  string timestamp, line;
  
  timestamp = mktime(time());
  line = "[" + timestamp + "] " + level + ": " + msg + "\n";
  
  // Add to buffer
  if (log_count < 100) {
    log_buffer[log_count] = line;
    log_count++;
  }
  
  // Write to file (if available)
  cat(LOG_FILE, line);
}

// Get recent log entries
get_recent_logs(count) {
  string result;
  int i, start;
  
  if (count > log_count) count = log_count;
  start = log_count - count;
  
  result = "";
  for (i = start; i < log_count; i++) {
    result = result + log_buffer[i];
  }
  
  return result;
}
```

**Log Levels (include/log.h):**
```c
// include/log.h
#define LOG_DEBUG 0
#define LOG_INFO 1
#define LOG_WARN 2
#define LOG_ERROR 3
```

**Usage:**
```c
// In any object
#include <daemons.h>

if (!valid_input(input)) {
  atoo(LOG_D).log_error("Invalid input from " + get_name() + ": " + input);
  return 0;
}

atoo(LOG_D).log_debug("Player " + get_name() + " entered room");
```

### 7.3 user_d Implementation

**Purpose:** Player registry and management using table storage

```c
// daemons/user_d.c

#include <config.h>
#include <daemons.h>

// Track players for iteration
string player_list[100];
int player_count;

// Prevent cloning
static init() {
  if (!prototype(this_object())) {
    destruct(this_object());
    return;
  }
  
  player_count = 0;
  table_set("user_d_initialized", 1);
}

// Add player to registry
add_player(name, player) {
  if (!name || !player) return 0;
  
  name = downcase(name);
  
  // Store in table
  table_set("user_d_player_" + name, otoa(player));
  
  // Add to list if not full
  if (player_count < 100) {
    player_list[player_count] = name;
    player_count++;
  }
  
  atoo(LOG_D).log_debug("Added player to registry: " + name);
  return 1;
}

// Remove player from registry
remove_player(name) {
  int i;
  
  if (!name) return 0;
  
  name = downcase(name);
  
  // Remove from table
  table_delete("user_d_player_" + name);
  
  // Remove from list
  for (i = 0; i < player_count; i++) {
    if (player_list[i] == name) {
      // Shift remaining entries
      while (i < player_count - 1) {
        player_list[i] = player_list[i + 1];
        i++;
      }
      player_count--;
      break;
    }
  }
  
  atoo(LOG_D).log_debug("Removed player from registry: " + name);
  return 1;
}

// Find player by name
find_player(name) {
  string obj_str;
  
  if (!name) return NULL;
  
  name = downcase(name);
  obj_str = table_get("user_d_player_" + name);
  
  if (!obj_str) return NULL;
  return atoo(obj_str);
}

// Get all connected players
query_players() {
  object result[100];
  int result_count, i;
  object player;
  
  result_count = 0;
  
  for (i = 0; i < player_count; i++) {
    player = find_player(player_list[i]);
    if (player && connected(player)) {
      result[result_count] = player;
      result_count++;
    }
  }
  
  return result;
}

// Get player count
query_player_count() {
  int i, count;
  object player;
  
  count = 0;
  for (i = 0; i < player_count; i++) {
    player = find_player(player_list[i]);
    if (player && connected(player)) {
      count++;
    }
  }
  
  return count;
}

// Check if player exists
player_exists(name) {
  if (!name) return 0;
  
  name = downcase(name);
  return (table_get("user_d_player_" + name) != NULL);
}
```

### 7.4 help_d Implementation

**Purpose:** Help system and command documentation

```c
// daemons/help_d.c

#include <config.h>
#include <daemons.h>

string topic_list[200];
int topic_count;

// Prevent cloning
static init() {
  if (!prototype(this_object())) {
    destruct(this_object());
    return;
  }
  
  topic_count = 0;
  table_set("help_d_initialized", 1);
  load_help_files();
}

// Load help files from /help directory
load_help_files() {
  // Register basic help topics
  register_help("commands", "Type 'commands' to see available commands.\n");
  register_help("say", "Usage: say <message>\nSay something to everyone in the room.\n");
  register_help("look", "Usage: look [at <object>]\nLook at your surroundings or an object.\n");
  register_help("who", "Usage: who\nSee who is currently online.\n");
  register_help("quit", "Usage: quit\nDisconnect from the game.\n");
  
  atoo(LOG_D).log_info("Loaded " + itoa(topic_count) + " help topics");
}

// Register a help topic
register_help(topic, text) {
  if (!topic || !text) return 0;
  if (topic_count >= 200) return 0;
  
  topic = downcase(topic);
  
  // Store in table
  table_set("help_d_topic_" + topic, text);
  
  // Add to list
  topic_list[topic_count] = topic;
  topic_count++;
  
  return 1;
}

// Get help for a topic
get_help(topic) {
  string text;
  
  if (!topic) return NULL;
  
  topic = downcase(topic);
  text = table_get("help_d_topic_" + topic);
  
  if (!text) {
    // Try to get help from command
    text = atoo(COMMAND_D).query_help(topic);
  }
  
  return text;
}

// List all help topics
list_topics() {
  string result;
  int i;
  
  result = "Available help topics:\n";
  
  for (i = 0; i < topic_count; i++) {
    result = result + "  " + topic_list[i] + "\n";
  }
  
  return result;
}

// Search help topics
search_topics(query) {
  string result;
  int i;
  
  if (!query) return NULL;
  
  query = downcase(query);
  result = "Matching topics:\n";
  
  for (i = 0; i < topic_count; i++) {
    if (instr(topic_list[i], 1, query)) {
      result = result + "  " + topic_list[i] + "\n";
    }
  }
  
  return result;
}
```

### 7.5 Daemon Initialization

**In boot.c::finish_init():**

```c
static finish_init() {
  object daemon;
  
  // Initialize daemons in order
  // log_d first so other daemons can log
  daemon = load_object(LOG_D);
  set_priv(daemon, 1);
  
  daemon = load_object(USER_D);
  set_priv(daemon, 1);
  
  daemon = load_object(COMMAND_D);
  set_priv(daemon, 1);
  
  daemon = load_object(HELP_D);
  set_priv(daemon, 1);
  
  // Log that initialization is complete
  atoo(LOG_D).log_info("All daemons initialized successfully");
  atoo(LOG_D).log_info("System ready");
}
```

**Key Points:**
- Initialize log_d first (so other daemons can log)
- Set privilege on daemons with set_priv()
- Log successful initialization
- Daemons are loaded once and persist

### 7.6 Daemon Best Practices

**DO:**
- ✅ Use table storage with daemon-specific prefixes
- ✅ Use fixed-size arrays for iteration
- ✅ Prevent cloning in init()
- ✅ Log important events
- ✅ Validate all inputs
- ✅ Handle errors gracefully

**DON'T:**
- ❌ Use mappings (NLPC doesn't have them!)
- ❌ Store player-specific data in daemons
- ❌ Assume daemons are always loaded
- ❌ Forget to check return values
- ❌ Mix concerns (keep daemons focused)

**Example:**
```c
// GOOD - Daemon provides service
object player;
player = atoo(USER_D).find_player(name);
if (player) {
  player.listen("Hello!\n");
}

// BAD - Daemon stores player state
atoo(USER_D).set_player_hp(name, 100);  // Don't do this!
```

---

## 8. Object Composition Pattern

### 8.1 Why Composition?

NLPC doesn't have inheritance (`inherit` keyword). Instead, it uses **composition** via the `attach()` function.

**Traditional LPC (Inheritance):**
```c
// object.c
string name;
void set_name(string n) { name = n; }

// player.c
inherit "/std/object";  // Get set_name() automatically
```

**NLPC (Composition):**
```c
// obj/share/common.c - Shared functionality
// obj/player.c - Uses attach() to get shared functionality
object sharobj;
static init() {
  sharobj = new("/obj/share/common");
  attach(sharobj);  // Now we have common's functions!
}
```

### 8.2 How attach() Works

When you call `attach(obj)`, the attached object's functions become available to your object. It's like inheritance, but using composition.

**Example:**
```c
// obj/share/common.c
set_name(name) {
  table_set(otoa(caller_object()) + "_name", name);
}

get_name() {
  return table_get(otoa(caller_object()) + "_name");
}

// obj/player.c
object sharobj;

static init() {
  sharobj = new("/obj/share/common");
  attach(sharobj);
}

// Now player can call set_name() and get_name()!
// player.set_name("Bob");  // Works!
```

### 8.3 Complete Shared Object

**obj/share/common.c - Provides common functionality:**

```c
// obj/share/common.c

#include <types.h>

// Property management using table storage
set_property(key, value) {
  table_set(otoa(caller_object()) + "_" + key, value);
}

get_property(key) {
  return table_get(otoa(caller_object()) + "_" + key);
}

delete_property(key) {
  table_delete(otoa(caller_object()) + "_" + key);
}

// Name
set_name(name) {
  set_property("name", name);
}

get_name() {
  return get_property("name");
}

// Description
set_desc(desc) {
  set_property("desc", desc);
}

get_desc() {
  string desc;
  desc = get_property("desc");
  if (!desc) return "You see nothing special.\n";
  return desc;
}

// Short description
set_short(short) {
  set_property("short", short);
}

get_short() {
  string short;
  short = get_property("short");
  if (!short) return get_name();
  return short;
}

// Long description
set_long(long) {
  set_property("long", long);
}

get_long() {
  string long;
  long = get_property("long");
  if (!long) return get_desc();
  return long;
}

// Type
set_type(type) {
  set_property("type", itoa(type));
}

get_type() {
  string type_str;
  type_str = get_property("type");
  if (!type_str) return TYPE_OBJECT;
  return atoi(type_str);
}

// Owner
set_owner(owner) {
  set_property("owner", otoa(owner));
}

get_owner() {
  string owner_str;
  owner_str = get_property("owner");
  if (!owner_str) return NULL;
  return atoo(owner_str);
}

// Cleanup
recycle() {
  // Clean up properties
  delete_property("name");
  delete_property("desc");
  delete_property("short");
  delete_property("long");
  delete_property("type");
  delete_property("owner");
}
```

### 8.4 Using Composition in Objects

**obj/object.c - Base object:**

```c
// obj/object.c

#include <types.h>

object sharobj;

id(arg) {
  string name;
  name = get_name();
  if (!name) return 0;
  return (instr(downcase(name), 1, downcase(arg)) > 0);
}

recycle() {
  if (!priv(caller_object()) && get_owner() != caller_object()) return 0;
  if (prototype(this_object())) return 0;
  sharobj.recycle();
  destruct(this_object());
  return 1;
}

static init() {
  sharobj = new("/obj/share/common");
  attach(sharobj);
  set_type(TYPE_OBJECT);
  set_owner(caller_object());
}
```

**obj/room.c - Room object:**

```c
// obj/room.c

#include <types.h>

object sharobj;

id(arg) {
  return 0;  // Rooms don't match id
}

recycle() {
  if (!priv(caller_object()) && get_owner() != caller_object()) return 0;
  if (prototype(this_object())) return 0;
  sharobj.recycle();
  destruct(this_object());
  return 1;
}

static init() {
  sharobj = new("/obj/share/common");
  attach(sharobj);
  set_type(TYPE_ROOM);
  set_owner(caller_object());
}

// Room-specific functions
add_exit(direction, destination) {
  table_set(otoa(this_object()) + "_exit_" + direction, destination);
}

get_exit(direction) {
  return table_get(otoa(this_object()) + "_exit_" + direction);
}

list_exits() {
  // Would need to track exits in an array
  // For now, return a simple message
  return "Exits: north, south, east, west\n";
}
```

**obj/player.c - Player object:**

```c
// obj/player.c

#include <types.h>
#include <daemons.h>
#include <flags.h>

object sharobj;
string password;
int player_level;
int player_flags;

static init() {
  sharobj = new("/obj/share/common");
  attach(sharobj);
  set_type(TYPE_PLAYER);
  player_level = 0;
  player_flags = 0;
}

// Password management
set_password(pass) {
  if (caller_object() != this_object() && !priv(caller_object())) return 0;
  password = pass;
  return 1;
}

check_password(pass) {
  return (pass == password);
}

// Level management
set_level(level) {
  if (!priv(caller_object())) return 0;
  player_level = level;
  return 1;
}

query_level() {
  return player_level;
}

// Flags management
set_flags(flags) {
  if (!priv(caller_object())) return 0;
  player_flags = flags;
  return 1;
}

query_flags() {
  return player_flags;
}

// Connection handling
connect() {
  listen("\nWelcome back, " + get_name() + "!\n");
  
  if (location(this_object())) {
    listen(location(this_object()).get_long() + "\n");
  }
  
  atoo(USER_D).add_player(get_name(), this_object());
  atoo(LOG_D).log_info(get_name() + " connected");
}

disconnect() {
  atoo(USER_D).remove_player(get_name());
  atoo(LOG_D).log_info(get_name() + " disconnected");
}

// Input handling
listen(input) {
  string cmd, arg;
  int pos;
  
  // Parse input
  pos = instr(input, 1, " ");
  if (pos) {
    cmd = leftstr(input, pos - 1);
    arg = rightstr(input, strlen(input) - pos);
  } else {
    cmd = input;
    arg = NULL;
  }
  
  // Execute via command_d
  if (atoo(COMMAND_D).execute_command(this_object(), cmd, arg)) {
    return;
  }
  
  send_device("What?\n");
}

recycle() {
  if (!priv(caller_object())) return 0;
  if (prototype(this_object())) return 0;
  sharobj.recycle();
  destruct(this_object());
  return 1;
}
```

### 8.5 Composition Benefits

**Advantages:**
- ✅ Reuse code without inheritance
- ✅ Multiple objects can share functionality
- ✅ Easy to update shared code
- ✅ Cleaner than copying code
- ✅ NLPC-native pattern

**Pattern:**
1. Create shared object with common functions
2. In your object, create instance: `sharobj = new("/obj/share/common")`
3. Attach it: `attach(sharobj)`
4. Now you have access to all shared functions!

---

## 9. Standard Objects

### 9.1 Object Architecture

NLPC uses **composition**, not inheritance. All objects attach a shared object for common functionality.

**Architecture:**
```
obj/share/common.c (shared functionality)
    ↑ attached by
    ├── obj/object.c (base object)
    ├── obj/room.c (rooms)
    └── obj/player.c (players)
```

### 9.2 Complete obj/player.c

**Minimal player implementation that delegates to command_d:**

```c
// obj/player.c

#include <types.h>
#include <daemons.h>
#include <flags.h>

object sharobj;
string password;
int player_level;
int player_flags;

static init() {
  sharobj = new("/obj/share/common");
  attach(sharobj);
  set_type(TYPE_PLAYER);
  player_level = 0;
  player_flags = 0;
}

// Password management
set_password(pass) {
  if (caller_object() != this_object() && !priv(caller_object())) return 0;
  password = pass;
  return 1;
}

check_password(pass) {
  return (pass == password);
}

// Level management
set_level(level) {
  if (!priv(caller_object())) return 0;
  player_level = level;
  return 1;
}

query_level() {
  return player_level;
}

// Flags management
set_flags(flags) {
  if (!priv(caller_object())) return 0;
  player_flags = flags;
  return 1;
}

query_flags() {
  return player_flags;
}

// Connection handling
connect() {
  listen("\nWelcome back, " + get_name() + "!\n");
  
  if (location(this_object())) {
    listen(location(this_object()).get_long() + "\n");
  }
  
  atoo(USER_D).add_player(get_name(), this_object());
  atoo(LOG_D).log_info(get_name() + " connected");
}

disconnect() {
  atoo(USER_D).remove_player(get_name());
  atoo(LOG_D).log_info(get_name() + " disconnected");
}

// Input handling - Delegates to command_d
listen(input) {
  string cmd, arg;
  int pos;
  
  // Parse input
  pos = instr(input, 1, " ");
  if (pos) {
    cmd = leftstr(input, pos - 1);
    arg = rightstr(input, strlen(input) - pos);
  } else {
    cmd = input;
    arg = NULL;
  }
  
  // Execute via command_d
  if (atoo(COMMAND_D).execute_command(this_object(), cmd, arg)) {
    return;
  }
  
  send_device("What?\n");
}

recycle() {
  if (!priv(caller_object())) return 0;
  if (prototype(this_object())) return 0;
  sharobj.recycle();
  destruct(this_object());
  return 1;
}
}

// Handle player input - DELEGATE TO COMMAND_D
void listen(string input) {
  string cmd, arg;
  int pos;
  
  if (!input || input == "") return;
  
  // Parse input
  pos = instr(input, 1, " ");
  if (pos) {
    cmd = leftstr(input, pos - 1);
    arg = rightstr(input, strlen(input) - pos);
  } else {
    cmd = input;
    arg = "";
  }
  
  // Route to command daemon
  if (!atoo(COMMAND_D).execute_command(this_object(), cmd, arg)) {
    send_device("Huh?\n");
  }
}

// Called when player disconnects
void disconnect() {
  atoo(USER_D).remove_player(query_name());
  atoo(LOG_D).log_info(query_name() + " disconnected");
}

// Set/query player level
void set_level(int level) {
  player_level = level;
}

int query_level() {
  return player_level;
}
```

**That's it!** Player.c is ~50 lines instead of 2000+.

---

## 7. Configuration System

### 7.1 config.h

```c
// include/config.h

// Server configuration
#define MUD_NAME "NetCI MUD"
#define MUD_PORT 6666
#define SAVE_INTERVAL 3600
#define MAX_IDLE_TIME 1800

// Paths
#define DIR_DAEMONS "/daemons"
#define DIR_CMDS "/cmds"
#define DIR_STD "/std"
#define DIR_DOMAINS "/domains"
#define DIR_DATA "/data"

// Limits
#define MAX_PLAYERS 100
#define MAX_INVENTORY 50
#define MAX_COMMAND_LENGTH 1000

// Features
#define ENABLE_AUTOSAVE 1
#define ENABLE_IDLE_TIMEOUT 1
#define ENABLE_COMMAND_LOGGING 0
```

### 7.2 daemons.h

```c
// include/daemons.h

#define COMMAND_D "/daemons/command_d"
#define USER_D "/daemons/user_d"
#define LOG_D "/daemons/log_d"
```

---

## 8. Complete Implementation

### 8.1 File Listing

**Total: 19 files**

**Core (5 files):**
1. boot.c
2. include/config.h
3. include/daemons.h
4. include/commands.h
5. include/log.h

**Daemons (3 files):**
6. daemons/log_d.c
7. daemons/command_d.c
8. daemons/user_d.c

**Standard Objects (4 files):**
9. std/object.c
10. std/living.c
11. std/player.c
12. std/room.c

**System (2 files):**
13. sys/login.c
14. sys/sys.c

**Commands (4 files):**
15. cmds/player/say.c
16. cmds/player/look.c
17. cmds/player/who.c
18. cmds/player/quit.c

**Domains (1 file):**
19. domains/start/limbo.c

---

## 9. Usage Guide

### 9.1 Adding a New Command

**Step 1:** Create command file
```bash
# Create cmds/player/emote.c
```

**Step 2:** Implement standard interface
```c
int main(object player, string arg) {
  // Implementation
  return 1;
}

string query_help() {
  return "Usage: emote <action>";
}
```

**Step 3:** Reload commands
```
@call /daemons/command_d reload_commands
```

**That's it!** No need to modify player.c.

### 9.2 Testing the Skeleton

**Start the MUD:**
```bash
./netci -l libs/modern_mudlib
```

**Connect:**
```
telnet localhost 6666
```

**Create a character:**
```
Enter your name: new
Choose a name: bob
Choose a password: secret123
Confirm password: secret123
```

**Try commands:**
```
look
say Hello!
who
quit
```

---

## 10. Benefits and Trade-offs

### 10.1 Benefits

**Maintainability:**
- Small, focused files (20-50 lines vs 2000+)
- Easy to find code
- Clear responsibilities
- Team-friendly

**Extensibility:**
- Add commands without touching core
- Plug-in architecture
- Easy to add features

**Performance:**
- Commands loaded on-demand
- Caching for frequently-used commands
- Daemon-based services are efficient

**Modern UX:**
- Professional login experience
- Better prompts and feedback
- Configurable behavior

### 10.2 Trade-offs

**Complexity:**
- More files to manage
- Need to understand daemon pattern
- Slightly more initial setup

**Learning Curve:**
- Different from traditional LPMud
- Need to learn new patterns
- More abstraction

**Debugging:**
- More indirection (daemons)
- Need to trace through layers
- But: smaller files easier to debug

---

## 11. Migration Guide

### 11.1 From Basic Skeleton

**Commands:**
1. Extract each do_* function to separate file
2. Implement standard interface (main function)
3. Remove from player.c
4. Let command_d discover

**Login:**
1. Move logic to sys/login.c
2. Use redirect_input() for input flow
3. Update boot.c to use new("/sys/login")

**Objects:**
1. Move shared code to std/object.c
2. Use inheritance instead of attach()
3. Update existing objects

---

## 12. Conclusion

The modern mudlib architecture for NLPC provides a production-ready foundation for building MUDs on NetCI.

### 12.1 What You've Learned

**NLPC-Specific Patterns:**
- ✅ **Table Storage with Namespacing** - Global tables with proper prefixing
- ✅ **Composition over Inheritance** - Using `attach()` instead of `inherit`
- ✅ **No Mappings** - Table-based key-value storage
- ✅ **No Return Types** - NLPC function declarations
- ✅ **Proper System Functions** - `listen()`, `tell_room()`, `this_player()`

**Architecture:**
- ✅ **Command Discovery** - Auto-discover commands from /cmds
- ✅ **Four Core Daemons** - log_d, user_d, command_d, help_d
- ✅ **redirect_input()** - Clean login flow
- ✅ **Minimal player.c** - Delegates to command_d
- ✅ **Composition Pattern** - Shared functionality via attach()

**Key Innovations:**
1. **Table Namespacing Pattern:**
   - Object data: `otoa(this_object()) + "_" + key`
   - Daemon data: `"daemon_name_" + key`
   - Global data: `"global_" + key`

2. **Fixed-Size Arrays for Iteration:**
   - Track items in arrays
   - Use counters for iteration
   - Clean and efficient

3. **Composition via attach():**
   - Shared object pattern
   - Reusable functionality
   - NLPC-native approach

### 12.2 File Count

**Total: 20 files**
- 1 boot.c (master object)
- 4 daemons (log_d, user_d, command_d, help_d)
- 1 login system (sys/login.c)
- 4 base objects (obj/object.c, obj/room.c, obj/player.c, obj/share/common.c)
- 5+ commands (say, look, who, quit, help)
- 5 include files (config.h, daemons.h, types.h, flags.h, log.h)

### 12.3 Production Readiness

This architecture is **production-ready** with:
- ✅ Proper security (valid_read/valid_write)
- ✅ Error logging (log_d)
- ✅ User management (user_d)
- ✅ Command routing (command_d)
- ✅ Help system (help_d)
- ✅ Clean login flow (redirect_input)
- ✅ Extensible design (easy to add commands)

### 12.4 Next Steps

**Immediate:**
1. Set up your NetCI environment
2. Create the directory structure
3. Implement boot.c and daemons
4. Test with basic commands

**Short-term:**
5. Add more commands (get, drop, inventory, etc.)
6. Implement rooms and areas
7. Add more help topics
8. Test with multiple players

**Long-term:**
9. Add game-specific features
10. Implement combat/skills/quests
11. Build your world
12. Launch your MUD!

### 12.5 Resources

**NetCI Documentation:**
- **MUDLIB-DEVELOPMENT-GUIDE.md** - Basic mudlib development
- **ENGINE-EXTENSION-GUIDE.md** - Extending NetCI in C
- **TECHNICAL-REFERENCE.md** - Complete system reference

**Key Differences from Traditional LPC:**
- NO `mapping` - use `table_set/get`
- NO `void` - omit return types
- NO `inherit` - use `attach()`
- NO `previous_object()` - use `this_player()` or pass as parameter
- NO `write()` - use `listen()` or `send_device()`
- NO `say()` - use `tell_room()`

### 12.6 Final Thoughts

NLPC is different from traditional LPC, but it's powerful and efficient. The patterns in this guide provide a solid foundation for building modern MUDs.

**Key Principles:**
1. **Use composition, not inheritance**
2. **Namespace your table keys carefully**
3. **Keep daemons focused and stateless**
4. **Delegate command handling to command_d**
5. **Use fixed-size arrays for iteration**
6. **Follow NLPC syntax (no void, no mappings)**

**Remember:**
- Table storage is GLOBAL - always namespace!
- Objects use attach() for shared functionality
- Commands are stateless and reusable
- Daemons are singletons with table storage

Good luck building your MUD!

---

**End of Modern Mudlib Development Guide for NLPC**

*This guide provides NLPC-native patterns for building production-ready MUDs on NetCI.*

