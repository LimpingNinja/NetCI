# Melville/NetCI Implementation Status

## Phase 1: Foundation (IN PROGRESS)

### ✅ Completed

1. **Directory Structure**
   - Created full directory tree
   - Added README files for each major directory
   - Organized by function (cmds, sys, world, inherits, etc.)

2. **Documentation**
   - Main README.md with architecture overview
   - Driver integration documentation
   - Directory-specific README files

3. **Include Files**
   - `config.h` - System configuration and paths
   - `roles.h` - Role definitions and permission flags
   - `std.h` - Standard includes and constants

4. **Boot Object**
   - `/boot.c` - Master object with all required callbacks
   - `init()` - System initialization
   - `finish_init()` - Daemon initialization
   - `connect()` - Connection handling
   - `valid_read()` - File read permissions (role-based)
   - `valid_write()` - File write permissions (role-based)
   - `save_db()` - Autosave functionality
   - Helper functions for security checks

### 🚧 In Progress

None currently.

### ⏳ Pending

1. **Core Inheritables**
   - `/inherits/object.c` - Base object with descriptions, IDs, movement
   - `/inherits/container.c` - Inventory handling
   - `/inherits/room.c` - Rooms with exits

2. **System Objects**
   - `/sys/user.c` - Connection and authentication
   - `/sys/player.c` - In-world player body with cmd_hook
   - `/sys/login.c` - Login flow handler

3. **Daemons**
   - `/sys/daemons/cmd_d.c` - Command daemon (TMI-2 style)
   - `/sys/daemons/user_d.c` - User management
   - `/sys/daemons/log_d.c` - Logging system

4. **Auto Object**
   - `/sys/auto.c` - Auto-inherited base with sefuns

5. **Starter World**
   - `/world/start/void.c` - Starting room
   - `/world/start/start.c` - First real room

6. **Basic Commands**
   - Player commands (look, say, quit, who)
   - Wizard commands (goto, clone, dest)
   - Admin commands (shutdown, promote)

## Key Design Decisions

### Architecture
- **TMI-2 Style Commands**: Using command daemon instead of DGD's add_command()
- **User/Player Split**: Separate objects for connection and in-world presence
- **Role-Based Security**: PLAYER → BUILDER → PROGRAMMER → WIZARD → ADMIN
- **Composition Over Inheritance**: Using attach() instead of inherit (when driver supports)

### Security Model
- Role-based permissions (5 levels)
- Path-based file access control
- Protected directories (/sys, /boot.c)
- Home directory isolation for players

### Command System
- Command daemon for discovery and routing
- cmd_hook using input_to() for command processing
- Commands organized by role in /cmds/
- Standard interface: main(player, arg)

## NetCI Driver Requirements

### Currently Working
- ✅ Basic callbacks (connect, valid_read, valid_write)
- ✅ input_to() for command routing
- ✅ Object creation and management
- ✅ File system operations

### Needs Implementation
- ⏳ Auto object support (auto-inherit /sys/auto.c)
- ⏳ Efun overrides in auto object
- ⏳ Enhanced callback system for user/player split

### Workarounds
- Boot.c at root level (will move to /sys when driver updated)
- Explicit inheritance instead of auto object (for now)
- Wrapper functions for security instead of efun overrides

## Next Steps

1. Implement core inheritables (object.c, container.c, room.c)
2. Create system objects (user.c, player.c, login.c)
3. Implement command daemon
4. Create basic commands
5. Build starter world
6. Test login flow
7. Test command execution
8. Document usage

## Testing Plan

### Unit Tests
- Security functions (valid_read, valid_write)
- Path helper functions
- Role checking

### Integration Tests
- Login flow (connect → login → player)
- Command execution (input → cmd_d → command)
- File permissions (read/write by role)
- Object movement and inventory

### System Tests
- Full player session (login → commands → logout)
- Multiple concurrent players
- Autosave functionality
- Daemon initialization

## Notes

- Boot.c is currently at `/boot.c` but should move to `/sys/boot.c` when driver is updated
- Auto object support requires driver modifications
- Command system is TMI-2 style, not DGD style
- Security is role-based, not file-based like original Melville
- Using NLPC patterns (no inheritance, no mappings, composition via attach)

## References

- Original Melville: `.kiro/tmp/MelvilleMudlib/`
- TMI-2 cmd_d: https://github.com/Hobbitron/tmi2/blob/master/lib/adm/daemons/cmd_d.c
- NetCI Guide: `/docs/MODERN-MUDLIB-GUIDE.md`
- Architecture: `/libs/melville/README.md`
