# Implementation Spec: users_d.c - User Tracking Daemon

**Priority:** High (Phase 2)  
**Status:** Not Started  
**Estimated Time:** 2-3 hours  
**Dependencies:** users() efun ✅ (completed Nov 10, 2025)

---

## Overview

The users daemon (`/sys/daemons/users_d.c`) provides centralized user tracking and management. It integrates with the new `users()` efun to efficiently track all connected players without manual iteration.

## Core Requirements

### 1. Track All Logged-In Users
- Maintain registry of all connected player objects
- Use `users()` efun for efficient player discovery
- Update registry on login/logout events
- Provide fast user lookup by name

### 2. Manage this_player() Context
- Track which player is currently executing commands
- Provide `query_this_player()` - alternative to efun `this_player()`
- Handle edge cases (disconnected players, background tasks)

### 3. User Lookup by Name
- `find_user(string name)` - Get player object by name
- Case-insensitive search
- Return player object or 0 if not found

### 4. Connection Registry
- Track connection time for each user
- Track idle time
- Provide statistics (total users, longest connection, etc.)

## API Functions

```c
// Core tracking
void user_logged_in(object player);
void user_logged_out(object player);
object *query_users();
int query_user_count();

// User lookup
object find_user(string name);
object query_this_player();

// Statistics
int query_connection_time(object player);
int query_idle_time(object player);
mapping query_user_stats();

// Administration
void rehash_users();  // Rebuild from users() efun
string *query_user_names();
```

## Implementation Details

### Data Structures

```c
// Main user registry (mapping for O(1) lookup)
mapping user_registry;  // ([ player_name : player_object ])

// Connection metadata
mapping connection_times;  // ([ player_name : timestamp ])
mapping last_command;      // ([ player_name : timestamp ])

// Current context
object current_player;
```

### Integration with users() Efun

```c
void rehash_users() {
    object *all_users;
    int i;
    
    // Get all connected players from driver
    all_users = users();
    
    // Rebuild registry
    user_registry = ([]);
    
    for (i = 0; i < sizeof(all_users); i++) {
        string name;
        
        if (!all_users[i]) continue;
        
        name = all_users[i]->query_name();
        if (!name) continue;
        
        user_registry[name] = all_users[i];
        
        // Initialize metadata if missing
        if (!connection_times[name]) {
            connection_times[name] = time();
        }
        if (!last_command[name]) {
            last_command[name] = time();
        }
    }
}
```

### User Lookup

```c
object find_user(string name) {
    object player;
    
    if (!name || name == "") return 0;
    
    // Normalize name (lowercase)
    name = lowercase(name);
    
    // Check registry first (fast)
    player = user_registry[name];
    if (player && player->query_name() == name) {
        return player;
    }
    
    // Fallback: search through users() (slower but accurate)
    object *all_users = users();
    int i;
    
    for (i = 0; i < sizeof(all_users); i++) {
        if (!all_users[i]) continue;
        if (lowercase(all_users[i]->query_name()) == name) {
            // Update registry
            user_registry[name] = all_users[i];
            return all_users[i];
        }
    }
    
    return 0;
}
```

### this_player() Context Management

```c
object query_this_player() {
    // Return current player context (set by cmd_hook)
    return current_player;
}

void set_this_player(object player) {
    // Called by cmd_hook before command execution
    current_player = player;
    
    // Update last command timestamp
    string name = player->query_name();
    if (name) {
        last_command[name] = time();
    }
}

void clear_this_player() {
    // Called after command execution
    current_player = 0;
}
```

### Event Handlers

```c
void user_logged_in(object player) {
    string name;
    
    if (!player) return;
    
    name = player->query_name();
    if (!name) return;
    
    // Add to registry
    user_registry[name] = player;
    connection_times[name] = time();
    last_command[name] = time();
    
    // Log event
    logger(LOG_INFO, sprintf("User logged in: %s", name));
}

void user_logged_out(object player) {
    string name;
    
    if (!player) return;
    
    name = player->query_name();
    if (!name) return;
    
    // Remove from registry
    map_delete(user_registry, name);
    map_delete(connection_times, name);
    map_delete(last_command, name);
    
    // Clear context if this was current player
    if (current_player == player) {
        current_player = 0;
    }
    
    // Log event
    logger(LOG_INFO, sprintf("User logged out: %s", name));
}
```

### Statistics

```c
mapping query_user_stats() {
    mapping stats = ([]);
    object *all_users = users();
    string *names = keys(user_registry);
    int i, total_time, total_idle;
    
    stats["total_users"] = sizeof(all_users);
    stats["registered_users"] = sizeof(names);
    
    // Calculate average connection and idle times
    for (i = 0; i < sizeof(names); i++) {
        int conn_time = time() - connection_times[names[i]];
        int idle_time = time() - last_command[names[i]];
        
        total_time += conn_time;
        total_idle += idle_time;
    }
    
    if (sizeof(names) > 0) {
        stats["avg_connection_time"] = total_time / sizeof(names);
        stats["avg_idle_time"] = total_idle / sizeof(names);
    }
    
    return stats;
}

int query_connection_time(object player) {
    string name;
    
    if (!player) return 0;
    name = player->query_name();
    if (!name || !connection_times[name]) return 0;
    
    return time() - connection_times[name];
}

int query_idle_time(object player) {
    string name;
    
    if (!player) return 0;
    name = player->query_name();
    if (!name || !last_command[name]) return 0;
    
    return time() - last_command[name];
}
```

## Integration Points

### 1. user.c Login Flow
```c
// In user.c after successful login
void finish_login() {
    // ... create player object ...
    
    // Notify users_d
    USERS_D->user_logged_in(player);
    
    // ... transfer connection ...
}
```

### 2. user.c/player.c Disconnect
```c
void disconnect() {
    // Notify users_d
    USERS_D->user_logged_out(this_object());
    
    // ... rest of disconnect logic ...
}
```

### 3. player.c cmd_hook
```c
void cmd_hook(string input) {
    // Set current player context
    USERS_D->set_this_player(this_object());
    
    // Process command
    CMD_D->process_command(input);
    
    // Clear context
    USERS_D->clear_this_player();
}
```

## Testing Plan

### Unit Tests
1. `find_user()` with valid/invalid names
2. `query_users()` matches `users()` efun
3. `rehash_users()` rebuilds registry correctly
4. `this_player()` context set/cleared properly

### Integration Tests
1. Login flow updates registry
2. Logout flow removes from registry
3. Multiple users tracked correctly
4. Statistics accurate

### Edge Cases
1. Player disconnects during command execution
2. Duplicate login attempts
3. Player name changes
4. Registry out of sync with reality (rehash fixes)

## Performance Considerations

- **O(1) lookup** via mapping for `find_user()`
- **Lazy updates** - only rehash when needed
- **users() efun** - efficient driver-level iteration vs manual next_who() loops
- **Minimal overhead** - event-driven updates vs polling

## File Location
`/Users/kcmorgan/source/NetCI/libs/melville/sys/daemons/users_d.c`

## Configuration
```c
// In /include/config.h
#define USERS_D "/sys/daemons/users_d"
```

## Notes

- This daemon leverages the new `users()` efun for performance
- Provides both fast lookup (mapping) and accurate fallback (users() scan)
- Context management helps with debugging and logging
- Statistics useful for admin monitoring
