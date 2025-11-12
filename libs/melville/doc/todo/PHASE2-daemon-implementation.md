# Phase 2: Daemon Implementation Plan

**Date:** November 10, 2025  
**Status:** Ready to Implement  
**Priority:** High

---

## Overview

Phase 2 focuses on implementing essential system daemons to improve user experience. These daemons leverage the newly implemented `users()` and `get_dir()` efuns completed on November 10, 2025.

## Completed Prerequisites ✅

### Engine Efuns
- **users()** - Returns array of connected player objects
- **objects()** - Returns array of all objects  
- **children()** - Returns array of object clones
- **all_inventory()** - Returns array of container contents
- **get_dir()** - Returns array of filenames in directory
- **read_file()** - Read file contents
- **write_file()** - Write to file
- **file_size()** - Get file size

### Mudlib Infrastructure
- **cmd_d.c** - Command daemon with discovery and hashing ✅
- **Dynamic command discovery** - Uses get_dir() to find commands ✅
- **Role-based command access** - PLAYER → WIZARD → ADMIN ✅

## Implementation Order

### 1. users_d.c - User Tracking Daemon
**Estimated Time:** 2-3 hours  
**Spec:** `SPEC-users_d.md`

**Core Features:**
- Track all logged-in users using `users()` efun
- Fast user lookup by name (O(1) via mapping)
- Manage this_player() context
- Connection time and idle time tracking
- User statistics and monitoring

**Integration Points:**
- user.c: Call `user_logged_in()` on login
- user.c/player.c: Call `user_logged_out()` on disconnect
- player.c cmd_hook: Set/clear current player context

**Benefits:**
- Efficient user lookup without iteration
- Centralized user management
- Better debugging (track current player context)
- Admin monitoring (connection stats, idle tracking)

---

### 2. help_d.c - Dynamic Help System
**Estimated Time:** 3-4 hours  
**Spec:** `SPEC-help_d.md`

**Core Features:**
- Dynamic command discovery via get_dir()
- Auto-query query_help() from all commands
- Fuzzy matching "did you mean?" (Levenshtein distance)
- Category organization (communication, movement, etc.)
- Static help file indexing from /doc/help/
- `help reload` to refresh help index

**Integration Points:**
- All commands: Implement `query_help()` and `query_category()`
- cmd_d.c: Use existing command search paths
- _help.c: New player command

**Benefits:**
- Self-documenting system (commands provide own help)
- User-friendly fuzzy matching for typos
- No manual help file maintenance for commands
- Easy to add new commands (auto-discovered)
- Organized by category for easy browsing

---

## Implementation Strategy

### Phase 2a: users_d.c (First)
1. Create `/sys/daemons/users_d.c`
2. Implement core tracking functions
3. Add to `/include/config.h` as `USERS_D`
4. Update `user.c` login/logout flow
5. Update `player.c` cmd_hook for context management
6. Test with multiple concurrent users

### Phase 2b: help_d.c (Second)
1. Create `/sys/daemons/help_d.c`
2. Implement Levenshtein fuzzy matching
3. Implement command discovery and indexing
4. Create `/cmds/player/_help.c`
5. Add `query_help()` to existing commands:
   - _who.c
   - _say.c
   - _go.c
   - _look.c
   - _inventory.c
   - _quit.c
6. Test fuzzy matching with various typos
7. Test `help reload` functionality

### Phase 2c: Static Help Files (Ongoing)
1. Create `/doc/help/` directory
2. Add static help files for concepts:
   - `/doc/help/movement.txt`
   - `/doc/help/communication.txt`
   - `/doc/help/inventory.txt`
   - `/doc/help/commands.txt`
3. Add wizard help:
   - `/doc/help/building.txt`
   - `/doc/help/debugging.txt`

---

## Testing Plan

### users_d Tests
- [ ] Login updates registry
- [ ] Logout removes from registry
- [ ] find_user() case-insensitive
- [ ] find_user() returns correct object
- [ ] rehash_users() rebuilds from users() efun
- [ ] Connection time tracking accurate
- [ ] Idle time tracking accurate
- [ ] Multiple concurrent users tracked
- [ ] Context set/cleared in cmd_hook

### help_d Tests
- [ ] help reload discovers all commands
- [ ] query_help() called successfully
- [ ] Fuzzy matching suggests "who" for "woh"
- [ ] Fuzzy matching suggests "say" for "syy"
- [ ] Categories organized correctly
- [ ] help topics lists all topics
- [ ] Static help files loaded
- [ ] Long help text uses pager
- [ ] No help available message shown for unknown topics

---

## Post-Implementation Tasks

### Documentation
- [ ] Update COMPREHENSIVE-GAP-ANALYSIS.md
- [ ] Add users_d.c to Phase 1 complete list
- [ ] Add help_d.c to Phase 1 complete list
- [ ] Document query_help() pattern for new commands
- [ ] Add fuzzy matching algorithm to ENGINE-EXTENSION-GUIDE.md

### Code Quality
- [ ] Add error handling
- [ ] Add debug logging
- [ ] Optimize fuzzy matching cache
- [ ] Profile performance under load

### User Experience
- [ ] Add "help help" meta-help
- [ ] Add popular topics to main help
- [ ] Add keyboard shortcuts (if applicable)
- [ ] Consider pager for long help text

---

## Future Enhancements (Phase 3)

### users_d Extensions
- User groups/parties
- Friend lists
- Block/ignore lists
- Who filter by role/location
- User presence notifications

### help_d Extensions  
- Full-text search (apropos)
- Help history tracking
- See also links
- Interactive menu navigation
- Multi-language support
- Player-contributed help
- Help voting/rating

---

## Benefits Summary

### For Players
- **users_d**: Fast who list, reliable user tracking
- **help_d**: Easy to find help, forgiving of typos, organized topics

### For Developers
- **users_d**: No manual iteration, centralized user state
- **help_d**: Self-documenting commands, no manual help maintenance

### For Admins
- **users_d**: Connection monitoring, idle tracking, statistics
- **help_d**: Easy to add new topics, dynamic discovery

---

## Success Criteria

✅ **users_d Complete When:**
- All users tracked on login/logout
- find_user() works case-insensitively
- Connection and idle times accurate
- Context management integrated with cmd_hook
- No performance degradation with 10+ concurrent users

✅ **help_d Complete When:**
- All existing commands provide help
- Fuzzy matching suggests correct topics
- help reload rebuilds index
- Categories organize topics logically
- Static help files loaded and displayed
- No crashes or errors in production use

---

## Next Steps

1. Review specs: `SPEC-users_d.md` and `SPEC-help_d.md`
2. Implement users_d.c first (simpler, fewer dependencies)
3. Test users_d.c thoroughly with multiple users
4. Implement help_d.c second (more complex, fuzzy matching)
5. Add query_help() to all existing commands
6. Test help system with real users
7. Update documentation
8. Mark Phase 2 complete in COMPREHENSIVE-GAP-ANALYSIS.md

Let's build some great daemons! 🚀
