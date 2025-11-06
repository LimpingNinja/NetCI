# System Directory

This directory contains privileged system objects and daemons.

## Core Objects

- `boot.c` - Driver initialization object
- `auto.c` - Auto-inherited base object (requires driver support)
- `user.c` - Connection and authentication object
- `player.c` - In-world player body object
- `security.c` - Security validation functions

## Daemons

- `daemons/cmd_d.c` - Command daemon (TMI-2 style)
- `daemons/channel_d.c` - Channel daemon (future)
- `daemons/soul_d.c` - Soul/emote daemon (future)

## Data Storage

- `data/users/` - User save files (encrypted passwords, email, etc.)
- `data/players/` - Player save files (inventory, stats, etc.)
- `data/mail/` - Mail spools

## Security Notes

Objects in `/sys/` have elevated privileges. Only admins should be able to modify files in this directory.

The security system (implemented in `security.c` or `auto.c`) enforces:
- Role-based command access
- Path-based file permissions
- Object ownership tracking
