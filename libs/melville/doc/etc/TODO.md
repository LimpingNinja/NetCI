# Melville/NetCI TODO List

## Phase 1 Status: ✅ COMPLETE

### Completed Features
- ✅ Core inheritables (object, container, room, living)
- ✅ User/player system with authentication
- ✅ Command daemon (TMI-2 style)
- ✅ Basic player commands (look, go, quit, say, inventory, who)
- ✅ Auto object with simulated efuns
- ✅ Property-based storage system
- ✅ Exit system with custom messages
- ✅ **sscanf() efun** - Implemented and tested (40/43 tests passing)

## Critical Issues

### 1. Directory Scanning
**Problem**: NetCI's `ls()` efun calls `listen()` on each file, making it unsuitable for programmatic directory listing.

**Current Workaround**: cmd_d.c uses a hardcoded list of common verbs and checks for file existence.

**Proper Solution**: Need one of:
- Implement `get_dir()` efun in NetCI driver (returns array of filenames)
- Implement `file_list()` or similar efun
- Use a different pattern for command discovery

**Impact**: Commands must be manually added to the common_verbs list in cmd_d.c until this is fixed.

### 2. Password Hashing
**Problem**: Need proper cryptographic password hashing

**Current State**: 
- ✅ Basic `hash_password()` implemented in auto.c (placeholder)
- ✅ player.c uses hash_password() for set/check
- ⚠️ Current implementation is NOT cryptographically secure!

**Solution**: Implement proper password hashing:
- Add `crypt()` efun to NetCI driver
- Use a secure hashing algorithm (bcrypt, scrypt, argon2)
- Generate random salt per password
- Store salt with hash

**Impact**: Passwords are hashed but not with cryptographic security. Good enough for development, NOT for production.

## Gap Analysis: DGD Melville vs NLPC Melville

### Phase 2: Core Daemons (HIGH PRIORITY)

**Missing System Daemons** (from DGD Melville):
- [ ] **users_d.c** - Track logged-in users, manage this_player()
- [ ] **channel_d.c** - Chat channel system (gossip, wizard channels)
- [ ] **soul_d.c** - Social/emote commands (smile, laugh, wave, etc.)
- [ ] **mail_d.c** - Mail system daemon
- [ ] **mailer.c** - Mail composition/sending object

### Phase 2: Essential Player Commands (HIGH PRIORITY)

**Inventory Commands** (17 missing from DGD):
- [ ] **get.c** - Pick up items
- [ ] **drop.c** - Drop items
- [ ] **give.c** - Give items to others

**Communication Commands**:
- [ ] **tell.c** - Private messaging
- [ ] **gossip.c** - Public channel
- [ ] **tune.c** - Channel management (tune in/out)
- [ ] **show.c** - Show channel listeners
- [ ] **emote.c** - Custom emotes

**Utility Commands**:
- [ ] **help.c** - Help system
- [ ] **finger.c** - User information
- [ ] **describe.c** - Set self description
- [ ] **brief.c** - Toggle brief mode
- [ ] **mail.c** - Mail interface
- [ ] **bug.c** - Bug reporting
- [ ] **date.c** - Show date/time
- [ ] **history.c** - Command history

### Phase 3: Wizard Commands (MEDIUM PRIORITY)

**Object Management** (24 missing from DGD):
- [ ] **clone.c** - Clone objects
- [ ] **dest.c** - Destruct objects
- [ ] **update.c** - Reload objects
- [ ] **goto.c** - Teleport to location

**File Operations**:
- [ ] **cd.c** - Change directory
- [ ] **pwd.c** - Print working directory
- [ ] **ls.c** - List directory
- [ ] **mkdir.c** - Make directory
- [ ] **rmdir.c** - Remove directory
- [ ] **rm.c** - Remove file
- [ ] **cp.c** - Copy file
- [ ] **mv.c** - Move file

**Development Tools**:
- [ ] **ed.c** - Line editor
- [ ] **more.c** - View file
- [ ] **tail.c** - Tail file
- [ ] **status.c** - Object status
- [ ] **filenames.c** - Show object filenames
- [ ] **recreate.c** - Recreate object
- [ ] **task.c** - Task management

**Wizard Utilities**:
- [ ] **people.c** - List all players
- [ ] **wiz.c** - Wizard channel
- [ ] **emote.c** (wizard) - Wizard emotes
- [ ] **priv.c** - Check privileges

### Phase 3: Admin Commands (MEDIUM PRIORITY)

**Server Management** (6 missing from DGD):
- [ ] **shutdown.c** - Shutdown server
- [ ] **users.c** - List all users

**User Management**:
- [ ] **disconnect.c** - Disconnect user
- [ ] **force.c** - Force user command
- [ ] **makewiz.c** - Promote to wizard
- [ ] **nuke.c** - Delete user

### Phase 3: Additional Features (LOW PRIORITY)

**Inheritables**:
- [ ] **board.c** - Bulletin board system

**World Content**:
- [ ] Example objects (clock, items, etc.)
- [ ] Help file system and content
- [ ] Soul data file (emote definitions)
- [ ] News system
- [ ] More starting rooms

**System Improvements**:
- [ ] Command discovery and rehashing
- [ ] Command aliasing system
- [ ] Save/load improvements (compression, backups)
- [ ] Logging system with rotation

## Driver Integration Needed

### 1. Auto Object Support
- NetCI needs to call auto.c for all objects
- Currently objects must manually inherit/attach

### 2. Command Hook
- NetCI needs to call cmd_hook() on player input
- Currently using redirect_input() workaround

### 3. Security Hooks
- Implement proper privilege checking
- Integrate with boot.c valid_read/valid_write

### 4. Object Initialization
- Ensure create() is called properly
- Ensure init() is called when objects meet

## Testing Needed

### 1. Unit Tests
- Test all inheritables
- Test user/player flow
- Test command processing
- Test save/load

### 2. Integration Tests
- Test full login flow
- Test command execution
- Test movement
- Test communication

### 3. Load Tests
- Test with multiple players
- Test with many objects
- Test memory usage

## Documentation Needed

### 1. Builder's Guide
- How to create rooms
- How to create objects
- How to create NPCs

### 2. Wizard's Guide
- How to create commands
- How to use the editor
- How to debug

### 3. Admin's Guide
- How to manage players
- How to backup/restore
- How to update the mudlib

### 4. API Documentation
- Document all efuns
- Document all inheritables
- Document all daemons

## Nice to Have

### 1. Web Interface
- Web-based client
- Admin panel
- Statistics dashboard

### 2. Scripting Language
- Embedded scripting for quests
- Event system

### 3. Database Integration
- PostgreSQL/MySQL support
- Persistent storage

### 4. Clustering
- Multi-server support
- Load balancing

## Priority Order (Post-Phase 1)

### Immediate (Phase 2 Start)
1. **users_d.c** - Foundation for multi-user features
2. **get.c, drop.c** - Basic inventory manipulation
3. **help.c** - Essential for usability
4. **tell.c** - Basic communication

### High Priority (Phase 2 Core)
5. **channel_d.c + gossip.c, tune.c, show.c** - Social features
6. **soul_d.c + emote.c** - Social commands
7. **give.c, finger.c, describe.c, brief.c** - QoL improvements
8. **mail_d.c + mailer.c + mail.c** - Mail system

### Medium Priority (Phase 3 Start)
9. **Wizard object tools** - clone.c, dest.c, update.c, goto.c
10. **Wizard file tools** - cd.c, pwd.c, ls.c, mkdir.c, etc.
11. **Admin commands** - shutdown.c, users.c, disconnect.c, force.c
12. **Development tools** - ed.c, more.c, tail.c, status.c

### Low Priority (Phase 3+)
13. **board.c** - Bulletin boards
14. **Additional world content** - More rooms, objects, help files
15. **System improvements** - Command discovery, aliasing, logging

### Critical Blockers
- **Directory scanning** - Needed for ls.c, command discovery
- **Password hashing** - Needed for production security

## Notes

- ✅ Phase 1 complete - core infrastructure working
- 🎯 Phase 2 focus: User experience and social features
- 🔧 Phase 3 focus: Builder/wizard tools
- 📚 Document as you build
- 🧪 Test each feature before moving on
- 🚫 Don't over-engineer - keep it simple
