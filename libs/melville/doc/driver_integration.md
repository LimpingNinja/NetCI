# Driver Integration Notes

This document describes the NetCI driver modifications needed to support Melville mudlib features.

## Required Driver Features

### 1. Auto Object Support

The driver needs to automatically inherit `/sys/auto.c` into every compiled object.

**Implementation Options**:

**Option A: Compile-time inheritance**
- Modify the parser to automatically add `inherit "/sys/auto";` to every object
- Similar to how DGD handles auto objects
- Cleanest approach

**Option B: Runtime wrapping**
- Wrap object creation to inject auto object functions
- More complex, but doesn't require parser changes

**Option C: Simulated efuns only**
- Keep auto.c as a library, not auto-inherited
- Objects explicitly inherit it when needed
- Less automatic, but works with current driver

**Recommendation**: Start with Option C, migrate to Option A when driver supports it.

### 2. Command Hook (cmd_hook)

The `cmd_hook` in `player.c` uses `input_to()` to capture player input and route it to the command daemon.

**Current NetCI Support**: ✅ `input_to()` already exists

**Implementation**:
```c
// In player.c
create() {
    input_to("cmd_hook");
}

cmd_hook(string input) {
    // Route to command daemon
    "/sys/daemons/cmd_d"->execute_command(this_object(), input);
    
    // Re-establish hook for next input
    input_to("cmd_hook");
}
```

**Note**: When other systems need input (editor, mail, etc.), they temporarily override the hook and restore it when done.

### 3. Security Hooks

The security system needs to intercept file operations.

**Required Efuns to Override** (if possible):
- `read_file()`
- `write_file()`
- `save_object()`
- `restore_object()`
- `remove_file()`
- `rename_file()`

**Implementation Options**:

**Option A: Efun overrides in auto.c**
```c
// In auto.c
int read_file(string path, int start, int lines) {
    if (!valid_read(path, this_object())) {
        return 0; // Permission denied
    }
    return ::read_file(path, start, lines); // Call original
}
```

**Option B: Wrapper functions**
```c
// Objects call secure_read_file() instead of read_file()
int secure_read_file(string path, int start, int lines) {
    if (!valid_read(path, this_object())) {
        return 0;
    }
    return read_file(path, start, lines);
}
```

**Recommendation**: Option B for now (doesn't require driver changes), migrate to Option A later.

### 4. User/Player Object Management

The driver needs to handle the user/player split.

**Connection Flow**:
1. Player connects → driver creates `/sys/user.c` clone
2. User logs in → user.c creates `/sys/player.c` clone
3. User.c and player.c are linked (user stores player reference, player stores user reference)
4. Player disconnects → driver notifies user.c, which cleans up player.c

**Driver Callbacks Needed**:
- `connect()` - Called when socket connects → create user object
- `disconnect()` - Called when socket disconnects → notify user object
- `receive_message()` - Called when data arrives → route to user object

**Current NetCI Support**: Need to verify existing callback system

### 5. Object Initialization

Every object needs `create()` called after compilation.

**Current NetCI Support**: ✅ Already supported

**Melville Usage**:
```c
create() {
    // Object initialization
    set_short("a sword");
    set_long("A gleaming steel sword.");
}
```

## Migration Path

### Phase 1: Work with Current Driver
- Use explicit inheritance instead of auto object
- Use wrapper functions for security
- Implement cmd_hook with existing `input_to()`

### Phase 2: Driver Enhancements
- Add auto object support
- Add efun override capability
- Enhance callback system for user/player split

### Phase 3: Optimization
- Optimize security checks
- Add driver-level command routing
- Performance tuning

## Testing Driver Features

### Test auto.c inheritance
```c
// Test object
inherit "/sys/auto";

create() {
    // Should have access to auto.c functions
    object obj = find_object("/sys/boot");
    if (obj) {
        write(this_player(), "Auto inheritance works!\n");
    }
}
```

### Test cmd_hook
```c
// In player.c
cmd_hook(string input) {
    write(this_object(), "You typed: " + input + "\n");
    input_to("cmd_hook");
}
```

### Test security hooks
```c
// Try to read a protected file
if (secure_read_file("/sys/data/users/admin.o")) {
    write(this_player(), "Security bypass!\n");
} else {
    write(this_player(), "Security working.\n");
}
```

## Driver Configuration

Suggested additions to NetCI config:

```
# Auto object
auto_object = /sys/auto

# System directories (privileged)
system_dirs = /sys, /sys/daemons, /sys/data

# Default user object
user_object = /sys/user

# Default player object  
player_object = /sys/player
```

## Questions for Driver Team

1. Does NetCI support auto-inherited objects?
2. Can we override efuns in inherited objects?
3. What callbacks exist for connection/disconnection?
4. How does the current `input_to()` work?
5. Is there a way to mark directories as privileged?

---

**Status**: This is a living document. Update as driver features are implemented.
