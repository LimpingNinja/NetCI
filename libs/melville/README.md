# Melville Mudlib for NetCI

A clean, minimalist mudlib for NetCI/NLPC, inspired by the DGD Melville mudlib but adapted for NetCI's architecture and LPC dialect.

## Philosophy

Melville/NetCI provides the **minimum necessary infrastructure** to let a group of administrators and wizards log in, communicate, and begin building a multiuser environment. It focuses on:

- **Simplicity**: Easy to understand and modify
- **Modularity**: Clean separation of concerns
- **Extensibility**: Provides hooks and patterns for expansion
- **Efficiency**: Lightweight core, add features as needed

This is NOT a complete game. It's a **framework** for building games, meeting spaces, virtual realities, or any other multiuser environment you envision.

## Architecture Overview

### Core System Objects (in `/sys`)

1. **boot.c** - Driver initialization object
   - Handles system startup
   - Loads essential daemons
   - Maps to DGD's "driver object"

2. **auto.c** - Auto-inherited base object
   - Automatically inherited by all objects (requires driver support)
   - Contains simulated efuns (sefuns) like `find_object()`, `present()`
   - Provides security hooks (`valid_read()`, `valid_write()`)
   - Similar to DGD's auto object

3. **user.c** - Connection object
   - Manages socket connection
   - Handles authentication and login
   - Stores real-person data (name, email, role)
   - One per connected player

4. **player.c** - In-world body object
   - Player's physical presence in the game world
   - Handles inventory, descriptions, gender
   - Command parsing via `cmd_hook`
   - Editor sessions
   - Associated 1:1 with user object

### Security Model

Role-based security with path permissions:

**Roles**: `player`, `builder`, `programmer`, `wizard`, `admin`

**Permission System**:
- `valid_read(path, caller)` - Controls file read access
- `valid_write(path, caller)` - Controls file write access
- Path-based: `/world/wiz/{name}/` owned by that wizard
- Role-based: admins can write anywhere, wizards in their directory

**Implementation**: `/sys/security.c` or `/sys/auto/security.c`

### Command System (TMI-2 Style)

Unlike DGD's `add_command()`, we use a **command daemon** pattern:

1. **Command Daemon** (`/sys/daemons/cmd_d.c`)
   - Central registry of all commands
   - Maps command names to command objects
   - Handles command lookup and execution

2. **cmd_hook** (in `player.c`)
   - Uses `input_to()` to capture player input
   - Routes commands to command daemon
   - Falls back to local command handling if needed

3. **Command Objects** (in `/cmds/`)
   - Each command is a separate object
   - Organized by role: `admin/`, `wizard/`, `builder/`, `player/`
   - Standard interface: `execute(player, args)`

### Core Inheritables (in `/inherits`)

1. **object.c** - Base for all physical objects
   - Short/long descriptions
   - ID and adjective handling
   - Movement and environment functions
   - `move_object()`, `query_environment()`, etc.

2. **container.c** - Inventory management
   - Inherits `object.c`
   - Add/remove objects from inventory
   - Query contents
   - Weight/capacity (optional)

3. **room.c** - Rooms with exits
   - Inherits `container.c`
   - Exit handling (add_exit, query_exit)
   - Room descriptions with contents
   - Brief mode support

4. **living.c** - Living creatures (optional)
   - Inherits `container.c`
   - Health, stats, combat hooks
   - For future expansion

## Directory Structure

```
/cmds/              Command objects by role
  admin/            Admin-only commands (shutdown, promote, etc.)
  wizard/           Wizard commands (clone, dest, update, etc.)
  builder/          Builder commands (dig, @create, etc.)
  player/           Player commands (look, say, tell, etc.)

/sys/               System objects (privileged)
  boot.c            Driver initialization
  auto.c            Auto-inherited base with sefuns
  user.c            Connection/authentication object
  player.c          In-world player body
  security.c        Security validation functions
  data/             Privileged data storage
    users/          User save files
    players/        Player save files
    mail/           Mail spools
  daemons/          System daemons
    cmd_d.c         Command daemon

/doc/               Documentation
  README.md         This file
  architecture.md   Detailed architecture docs
  security.md       Security system details
  commands.md       Command system details
  functions/        Function reference docs

/include/           Header files
  config.h          System configuration
  roles.h           Role definitions and flags
  std.h             Standard includes
  paths.h           Standard path definitions

/inherits/          Standard inheritables
  object.c          Base object (descriptions, IDs, movement)
  container.c       Inventory handling
  room.c            Rooms with exits
  living.c          Living creatures (optional)

/world/             Game world
  start/            Starting area (void, first room)
    void.c          The void (default start location)
    start.c         First real room
  wiz/              Wizard home directories
    {name}/         Per-wizard directory
      home.c        Wizard's home room
```

## Key Differences from DGD Melville

1. **Command System**: TMI-2 style command daemon instead of DGD's `add_command()`
2. **Driver Integration**: Adapted for NetCI's driver callbacks and architecture
3. **Security**: Uses NetCI's existing UID system with role-based extensions
4. **Language**: NLPC dialect with arrays, mappings, and NetCI-specific features

## Key Differences from ci200fs

1. **Clean Architecture**: Proper separation of user/player, system/world code
2. **Command Daemon**: Centralized command handling vs. verb files
3. **Inheritables**: Standard object hierarchy (object→container→room)
4. **Security**: Formal role system with path-based permissions
5. **Extensibility**: Designed for modification and expansion

## Getting Started

### For Administrators

1. Edit `/include/config.h` to set admin names
2. Start NetCI with Melville mudlib
3. Create your admin character
4. Read `/doc/architecture.md` for detailed system info
5. Add wizards and builders as needed

### For Wizards

1. Your home directory is `/world/wiz/{yourname}/`
2. Default home room is `home.c`
3. Use `xyzzy` command to return home
4. Build your areas in your directory or `/world/`
5. See `/doc/commands.md` for available commands

### For Builders

1. You have write access to `/world/` (except `/world/wiz/`)
2. Use builder commands to create rooms and objects
3. See `/doc/commands.md` for builder command reference

## Development Status

**Phase 1** (In Progress):
- [ ] Directory structure
- [ ] Core inheritables (object.c, container.c, room.c)
- [ ] System objects (user.c, player.c)
- [ ] Command daemon
- [ ] Auto object with sefuns
- [ ] Include files
- [ ] Starter world

**Future Phases**:
- Phase 2: Communication (say, tell, channels)
- Phase 3: Bulletin boards and mail
- Phase 4: Builder tools
- Phase 5: Advanced features (soul, emotes, etc.)

## Credits

- **Original Melville**: Created for DGD by George Reese (Descartes)
- **TMI-2**: Command daemon pattern inspiration
- **NetCI/NLPC**: Driver and language by [NetCI team]
- **Melville/NetCI**: Adapted by [Your Name]

## License

[To be determined - likely similar to original Melville license]

## References

- DGD Melville: `.kiro/tmp/MelvilleMudlib/`
- TMI-2 cmd_d: https://github.com/Hobbitron/tmi2/blob/master/lib/adm/daemons/cmd_d.c
- NetCI Documentation: `/docs/`

---

**Note**: This mudlib requires NetCI driver modifications for auto.c support. See `/doc/driver_integration.md` for details.
