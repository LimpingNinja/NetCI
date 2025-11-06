# Melville/NetCI TODO List

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

**Current Implementation** (auto.c):
```c
hash_password(password) {
    // Simple DJB2 hash - NOT SECURE!
    // Placeholder until crypt() efun is available
    return "HASH_" + itoa(hash_value);
}
```

**Proper Implementation** (when crypt() available):
```c
hash_password(password) {
    string salt = generate_salt();
    return crypt(password, "$2b$10$" + salt);
}
```

**Impact**: Passwords are hashed but not with cryptographic security. Good enough for development, NOT for production.

### 3. sscanf() Not Implemented
**Problem**: NetCI doesn't implement `sscanf()` for string parsing

**Current Workaround**: Use `instr()`, `leftstr()`, `rightstr()` for manual parsing

**Solution**: Either:
- Implement `sscanf()` in NetCI driver
- Continue using manual string parsing (works but verbose)

**Impact**: More verbose parsing code, but functional.

## Missing Features

### 1. Command Discovery
- Need proper directory scanning (see #1 above)
- Need command rehashing when new commands are added
- Need command aliasing system

### 2. Soul Daemon
- Implement social commands (smile, laugh, etc.)
- Integrate with cmd_d.c

### 3. Channel System
- Implement chat channels
- Integrate with player.c

### 4. Help System
- Implement help command
- Create help files for all commands
- Auto-generate help from command files

### 5. Editor
- Implement line editor for building
- Integrate with player.c

### 6. Save/Load Improvements
- Implement proper serialization format
- Add compression
- Add backup/restore functionality

### 7. Logging System
- Implement log daemon
- Add log levels
- Add log rotation

### 8. User Daemon
- Track all connected players
- Implement who list
- Implement tell/page commands

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

## Priority Order

1. **Critical**: Directory scanning (#1)
2. **Critical**: Password hashing (#2)
3. **High**: Command discovery improvements
4. **High**: User daemon
5. **High**: Help system
6. **Medium**: Soul daemon
7. **Medium**: Channel system
8. **Medium**: Editor
9. **Low**: Everything else

## Notes

- Focus on getting Phase 1 working first
- Don't over-engineer
- Keep it simple and maintainable
- Document as you go
