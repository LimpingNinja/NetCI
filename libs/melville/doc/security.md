# Security System - Melville/NetCI

This document describes the security model implemented in Melville/NetCI, including role-based permissions, file access control, and object privileges.

## Overview

Melville/NetCI uses a **role-based security system** with hierarchical privilege levels. Unlike the original DGD Melville (which used file-path-based permissions), this implementation uses roles that determine what actions a player can perform and which files they can access.

## Role Hierarchy

Security is based on five hierarchical roles, defined in `/include/roles.h`:

```c
#define ROLE_PLAYER     0   // Basic player
#define ROLE_BUILDER    1   // Can build areas
#define ROLE_PROGRAMMER 2   // Can write code
#define ROLE_WIZARD     3   // Full wizard powers
#define ROLE_ADMIN      4   // Administrator
```

**Hierarchy Rules:**
- Higher roles inherit all permissions of lower roles
- A WIZARD can do everything a BUILDER can do
- An ADMIN can do everything a WIZARD can do
- Roles are assigned per-player and stored in their player object

## Role Flags

In addition to hierarchical roles, players can have multiple capability flags (bitmask):

```c
#define FLAG_PLAYER     0x0001  // Basic player
#define FLAG_BUILDER    0x0002  // Can build areas
#define FLAG_PROGRAMMER 0x0004  // Can write code
#define FLAG_WIZARD     0x0008  // Full wizard powers
#define FLAG_ADMIN      0x0010  // Administrator
#define FLAG_IMMORTAL   0x0020  // Cannot be killed
#define FLAG_INVISIBLE  0x0040  // Invisible to players
#define FLAG_GUEST      0x0080  // Guest account
```

**Usage:**
- Flags allow fine-grained control (e.g., immortal builder)
- Check flags with: `player.get_flags() & FLAG_ADMIN`
- Helper macros: `IS_ADMIN(obj)`, `IS_WIZARD(obj)`, etc.

## File Access Control

All file operations are controlled by two functions in `/boot.c`:

### valid_read(path, func, caller, owner, flags)

Controls read access to files in the virtual filesystem.

**Parameters:**
- `path` - File path being accessed
- `func` - Function that triggered the read (e.g., "fread", "cat")
- `caller` - Object attempting the read (NULL for system)
- `owner` - Object ID of file owner
- `flags` - File permissions (-1 if doesn't exist, else bitmask)

**Returns:** 1 to allow, 0 to deny

**Permission Rules:**

| Role | Read Access |
|------|-------------|
| **System** | Everything (caller is NULL) |
| **Privileged Objects** | Everything (priv() returns true) |
| **Admin** | Everything |
| **Wizard** | Everything except `/sys/data/` |
| **Programmer/Builder** | Everything except `/sys/` |
| **Player** | Own files, home directory, or files marked READ_OK |

**Special Cases:**
- Non-existent files (flags == -1) are readable (for error messages)
- Owner can always read their own files
- Files with PERM_READ flag (flags & 1) are world-readable

### valid_write(path, func, caller, owner, flags)

Controls write access to files in the virtual filesystem.

**Parameters:** Same as valid_read()

**Returns:** 1 to allow, 0 to deny

**Permission Rules:**

| Role | Write Access |
|------|-------------|
| **System** | Everything (caller is NULL) |
| **Privileged Objects** | Everything (priv() returns true) |
| **Admin** | Everything except `/boot.c` |
| **Wizard** | `/world/` and `/cmds/`, plus owned files |
| **Builder** | `/world/` only, plus owned files |
| **Player** | Home directory (`/world/wiz/{name}/`) only |

**Protected Paths:**
- `/boot.c` - **NEVER** writable (even by admins)
- `/sys/` - Only admins can write (if PROTECT_SYS is enabled)
- `/sys/data/` - Only system and privileged objects

**Special Cases:**
- New files (flags == -1) can be created if directory is writable
- Owner can always write their own files
- Files with PERM_WRITE flag (flags & 2) are world-writable

## Object Privileges

Objects can be granted special privileges using `set_priv(obj, 1)`.

**Privileged Objects:**
- Bypass all file security checks
- Can read and write anywhere
- Typically granted to system daemons and login objects

**Objects with Privileges:**
- `/sys/login` - Login object (needs to read user data)
- `/sys/daemons/cmd_d` - Command daemon
- `/sys/daemons/user_d` - User daemon
- `/sys/daemons/log_d` - Log daemon

**Granting Privileges:**
```c
object daemon;
daemon = load_object("/sys/daemons/cmd_d");
set_priv(daemon, 1);  // Grant system privileges
```

**Checking Privileges:**
```c
if (priv(caller)) {
    // Object has system privileges
    return 1;
}
```

## Home Directories

Each player has a home directory in `/world/wiz/{name}/`:

**Rules:**
- Players can read and write in their own home directory
- Other players cannot access another player's home
- Wizards and admins can access all home directories
- Home directory is created when player is promoted to builder/wizard

**Path Format:**
- `/world/wiz/alice/` - Alice's home directory
- `/world/wiz/alice/home.c` - Alice's home room
- `/world/wiz/alice/myarea/` - Alice's custom area

**Implementation:**
The `is_in_home_dir(path, username)` helper function checks if a path is within a player's home directory.

## Security Best Practices

### For Administrators

**DO:**
- ✅ Protect `/boot.c` from modification (PROTECT_BOOT)
- ✅ Protect `/sys/` from non-admin access (PROTECT_SYS)
- ✅ Grant privileges only to trusted system objects
- ✅ Use role hierarchy (don't give admin to everyone)
- ✅ Log security violations
- ✅ Regularly audit privileged objects

**DON'T:**
- ❌ Allow write access to `/boot.c`
- ❌ Grant privileges to player-created objects
- ❌ Trust user input in security checks
- ❌ Hardcode player names (use query_name())
- ❌ Assume caller is always valid (check for NULL)

### For Builders/Wizards

**DO:**
- ✅ Keep your code in your home directory or `/world/`
- ✅ Use proper file permissions (chmod)
- ✅ Test code before deploying to production areas
- ✅ Document your code

**DON'T:**
- ❌ Try to access `/sys/` or `/boot.c`
- ❌ Create objects that grant themselves privileges
- ❌ Modify other players' home directories
- ❌ Create security backdoors

### For Programmers

**DO:**
- ✅ Always check `function_exists()` before calling functions
- ✅ Validate all user input
- ✅ Use helper functions for common security checks
- ✅ Handle NULL callers gracefully

**DON'T:**
- ❌ Assume objects have specific functions
- ❌ Trust data from untrusted sources
- ❌ Bypass security checks
- ❌ Create privilege escalation vulnerabilities

## Configuration

Security settings are defined in `/include/config.h`:

```c
/* Security Configuration */
#define PROTECT_BOOT    1       /* Protect boot.c from modification */
#define PROTECT_SYS     1       /* Protect /sys from non-admin access */

/* Admin Configuration */
#define ADMIN_NAMES     ({ "admin", "root" })
```

**PROTECT_BOOT:**
- When enabled (1), `/boot.c` cannot be modified by anyone
- Recommended: Always enabled

**PROTECT_SYS:**
- When enabled (1), `/sys/` is read-only for non-admins
- Recommended: Always enabled

**ADMIN_NAMES:**
- Array of player names who are automatically granted admin role
- Edit this to add your admin characters

## Security Violations

When a security check fails, the operation is denied. Optionally, violations can be logged:

```c
int valid_write(string path, string func, object caller, int owner, int flags) {
    // ... security checks ...
    
    if (denied) {
        // Log the violation
        if (function_exists("log_warn", atoo(LOG_D))) {
            atoo(LOG_D).log_warn("Write denied: " + 
                                 caller.get_name() + 
                                 " attempted to write " + path);
        }
        return 0;
    }
    
    return 1;
}
```

**Logged Information:**
- Player name
- Attempted operation (read/write)
- Target path
- Timestamp (via log daemon)

## Helper Functions

The following helper functions are defined in `/boot.c`:

### path_starts_with(path, prefix)

Checks if a path starts with a given prefix.

```c
if (path_starts_with(path, "/sys/")) {
    // Path is in /sys directory
}
```

### is_in_home_dir(path, username)

Checks if a path is within a player's home directory.

```c
if (is_in_home_dir(path, "alice")) {
    // Path is in /world/wiz/alice/
}
```

## Comparison with Original Melville

| Feature | DGD Melville | Melville/NetCI |
|---------|--------------|----------------|
| **Permission Model** | File-path based | Role-based |
| **Privilege Strings** | "admin", "wizard", "player" | ROLE_ADMIN, ROLE_WIZARD, etc. |
| **Creator Tracking** | Tracks who created object | Uses owner ID |
| **File Permissions** | Per-file privileges/creator | Role-based with owner |
| **Home Directories** | `/users/{name}/` | `/world/wiz/{name}/` |
| **Protected Dirs** | `/system/` | `/sys/` and `/boot.c` |

**Key Differences:**
- NetCI uses numeric roles instead of string-based privileges
- Simpler permission model (role hierarchy)
- No separate "privileges" and "creator" strings
- More straightforward to understand and modify

## Examples

### Checking Player Role

```c
// In a command or object
int player_role;

player_role = this_player().get_role();

if (player_role >= ROLE_ADMIN) {
    // Admin-only code
} else if (player_role >= ROLE_WIZARD) {
    // Wizard-only code
} else {
    // Regular player code
}
```

### Checking Player Flags

```c
// Check if player is an admin
if (IS_ADMIN(this_player())) {
    // Admin code
}

// Check if player is invisible
if (HAS_FLAG(this_player(), FLAG_INVISIBLE)) {
    // Don't show in who list
}
```

### Setting File Permissions

```c
// Make a file world-readable
chmod("/world/help/readme.txt", PERM_READ);

// Make a file readable and writable
chmod("/world/wiz/alice/notes.txt", PERM_READ | PERM_WRITE);
```

### Granting Object Privileges

```c
// In boot.c or privileged code
object daemon;

daemon = load_object("/sys/daemons/my_daemon");
if (daemon) {
    set_priv(daemon, 1);  // Grant system privileges
}
```

## Future Enhancements

Potential security improvements for future versions:

1. **Capability-Based Security**: Fine-grained capabilities beyond roles
2. **Access Control Lists**: Per-file/directory ACLs
3. **Audit Logging**: Comprehensive security event logging
4. **Sandboxing**: Isolated environments for untrusted code
5. **Resource Limits**: CPU/memory quotas per role
6. **Two-Factor Authentication**: Enhanced login security
7. **IP-Based Restrictions**: Limit access by IP address

---

**Note:** This security model is designed for simplicity and clarity. For production environments with untrusted users, additional hardening may be required.
