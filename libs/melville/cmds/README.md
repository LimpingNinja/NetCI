# Commands Directory

This directory contains all command objects, organized by role.

## Structure

- `admin/` - Admin-only commands (shutdown, promote, demote, etc.)
- `wizard/` - Wizard commands (clone, dest, update, goto, etc.)
- `builder/` - Builder commands (dig, @create, @link, etc.)
- `player/` - Player commands (look, say, tell, inventory, etc.)

## Command Interface

Each command object **must** implement:

```c
/* Main command execution - REQUIRED */
int execute(object player, string args) {
    // Command logic here
    // player.write("You did something!\n");
    return 1; // 1 = success, 0 = failure
}

/* Help text - OPTIONAL */
string query_help() {
    return "Help text for this command";
}

/* Required role - OPTIONAL (defaults to ROLE_PLAYER) */
int query_role() {
    return ROLE_PLAYER; // or ROLE_BUILDER, ROLE_WIZARD, ROLE_ADMIN
}
```

**Note**: The `execute()` function is called by cmd_d.c. It receives the player object and optional arguments string.

## Command Registration

Commands are registered with the command daemon (`/sys/daemons/cmd_d.c`) at startup or when added dynamically.

The command daemon maps command names to command objects and handles permission checking.
