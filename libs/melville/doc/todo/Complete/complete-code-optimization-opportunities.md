# Code Optimization Opportunities - Melville Mudlib

**Date**: November 6, 2025  
**Status**: Analysis Complete  
**Priority**: Code quality and efficiency improvements

## Overview

This document identifies code inefficiencies, redundancies, and optimization opportunities in the Melville mudlib. Focus areas:
- Duplicate code (DRY violations)
- Inefficient loops
- Redundant function calls
- Non-valuable code
- Missing early exits

**Note**: NetCI does not support `switch/case` or `continue` statements.

---

## HIGH PRIORITY - Duplicate Code (DRY Violations)

### 1. capitalize() Function - Duplicated 5 Times

**Problem**: The `capitalize()` function is duplicated in 5 different files with identical implementation.

**Files**:
- `/sys/auto.c` (lines 192-205)
- `/sys/user.c` (lines 291-304)
- `/sys/player.c` (lines 463-476)
- `/inherits/room.c` (lines 271-284)
- `/inherits/living.c` - uses auto.c version

**Current Code** (repeated 4 times):
```c
capitalize(str) {
    string first, rest;
    
    if (!str || strlen(str) == 0) return str;
    
    if (strlen(str) == 1) {
        return upcase(str);
    }
    
    first = leftstr(str, 1);
    rest = rightstr(str, strlen(str) - 1);
    
    return upcase(first) + rest;
}
```

**Solution**: Keep ONLY in `/sys/auto.c` (already there), remove from all other files.

**Why**: auto.c is automatically attached to all objects, so `capitalize()` is already available everywhere.

**Impact**: 
- Remove ~60 lines of duplicate code
- Single source of truth for capitalize logic
- Easier to maintain/update

**Files to Edit**:
- ✅ Keep: `/sys/auto.c` (lines 192-205)
- ❌ Remove: `/sys/user.c` (lines 291-304)
- ❌ Remove: `/sys/player.c` (lines 463-476)
- ❌ Remove: `/inherits/room.c` (lines 271-284)

---

### 2. find_object() Function - Duplicated 2 Times

**Problem**: `find_object()` is duplicated in auto.c and living.c with nearly identical logic.

**Files**:
- `/sys/auto.c` (lines 95-139) - 45 lines
- `/inherits/living.c` (lines 67-106) - 40 lines

**Differences**:
- auto.c version searches `this_player()` inventory/environment
- living.c version also searches `this_player()` inventory/environment
- Both have identical logic for "me", paths, "*name", "here"

**Solution**: Keep ONLY in `/sys/auto.c`, remove from `/inherits/living.c`.

**Impact**:
- Remove ~40 lines of duplicate code
- Single implementation for object finding

**Files to Edit**:
- ✅ Keep: `/sys/auto.c` (lines 95-139)
- ❌ Remove: `/inherits/living.c` (lines 67-106)

---

### 3. present() Function - Duplicated 2 Times

**Problem**: `present()` function is duplicated in auto.c and living.c with identical implementation.

**Files**:
- `/sys/auto.c` (lines 142-161)
- `/inherits/living.c` (lines 109-128)

**Current Code** (identical in both):
```c
present(name, container) {
    object *inv, obj;
    int i;
    
    if (!name || !container) return NULL;
    
    /* Get inventory */
    inv = container.query_inventory();
    if (!inv) return NULL;
    
    /* Search for matching ID */
    for (i = 0; i < sizeof(inv); i++) {
        obj = inv[i];
        if (obj && obj.id(name)) {
            return obj;
        }
    }
    
    return NULL;
}
```

**Solution**: Keep ONLY in `/sys/auto.c`, remove from `/inherits/living.c`.

**Impact**:
- Remove ~20 lines of duplicate code

**Files to Edit**:
- ✅ Keep: `/sys/auto.c` (lines 142-161)
- ❌ Remove: `/inherits/living.c` (lines 109-128)

---

## MEDIUM PRIORITY - Loop Inefficiencies

### 4. room.c - Manual replace_string() - NOW OBSOLETE ✅

**Problem**: Manual `replace_string()` implementation that only replaces first occurrence.

**File**: `/inherits/room.c` (lines 145-159)

**Current Code**:
```c
replace_string(str, find, replace) {
    int pos;
    string before, after;
    
    if (!str || !find) return str;
    if (!replace) replace = "";
    
    pos = instr(str, find);
    if (pos == -1) return str;
    
    before = leftstr(str, pos);
    after = rightstr(str, strlen(str) - pos - strlen(find));
    
    return before + replace + after;
}
```

**Solution**: ✅ **COMPLETED** - Native `replace_string()` efun now available (opcode 150)

**Action**: 
- ❌ Remove manual implementation from `/inherits/room.c` (lines 145-159)
- ✅ Use native `replace_string()` efun instead
- Native version replaces ALL occurrences, not just first

**Impact**:
- Remove ~15 lines of manual code
- Better performance (native C implementation)
- Replaces all occurrences instead of just first

---

### 5. auto.c - trim() Function - Inefficient Loop

**Problem**: `trim()` uses a while loop that repeatedly calls `strlen()`, `leftstr()`, and `rightstr()` on every iteration.

**File**: `/sys/auto.c` (lines 218-232)

**Current Code**:
```c
trim(str) {
    if (!str) return "";
    
    /* Trim leading spaces */
    while (strlen(str) > 0 && leftstr(str, 1) == " ") {
        str = rightstr(str, strlen(str) - 1);
    }
    
    /* Trim trailing spaces */
    while (strlen(str) > 0 && rightstr(str, 1) == " ") {
        str = leftstr(str, strlen(str) - 1);
    }
    
    return str;
}
```

**Issues**:
- Calls `strlen()` on every loop iteration (expensive)
- Creates new strings on every iteration (memory churn)
- For a string with 10 leading spaces, calls strlen() 10 times

**Optimized Version**:
```c
trim(str) {
    int len, start, end;
    
    if (!str) return "";
    
    len = strlen(str);
    if (len == 0) return "";
    
    /* Find first non-space */
    start = 1;  /* NetCI uses 1-based indexing */
    while (start <= len && leftstr(str, 1) == " ") {
        str = rightstr(str, len - 1);
        len = len - 1;
        if (len == 0) return "";
    }
    
    /* Find last non-space */
    while (len > 0 && rightstr(str, 1) == " ") {
        str = leftstr(str, len - 1);
        len = len - 1;
    }
    
    return str;
}
```

**Impact**: 
- Reduces function calls by ~50%
- Clearer intent with cached length

**Note**: This is still not optimal (still creates strings), but better than current. A truly optimal version would need index-based slicing which NetCI may not support efficiently.

---

### 5. room.c - get_exit_description() - Inefficient Array Building

**Problem**: Building array manually in loop when we could use array slicing (if available).

**File**: `/inherits/room.c` (lines 230-237)

**Current Code**:
```c
/* Three or more exits - build array manually since NetCI doesn't support slicing */
string *first_exits;
int j;

first_exits = ({});
for (j = 0; j < count - 1; j++) {
    first_exits = first_exits + ({ directions[j] });
}
```

**Issue**: Array concatenation in a loop creates new arrays on each iteration.

**Analysis**: The comment says "NetCI doesn't support slicing" - need to verify this. If true, this is acceptable. If false, we can optimize.

**Potential Optimization** (if slicing exists):
```c
/* If NetCI supports array slicing */
first_exits = directions[0..count-2];  /* Hypothetical syntax */
```

**Action**: VERIFY if NetCI supports array slicing. If not, current code is acceptable.

---

### 6. container.c - sync_inventory() - Could Use Array Literal

**Problem**: Building array in loop when we could potentially build it more efficiently.

**File**: `/inherits/container.c` (lines 89-100)

**Current Code**:
```c
sync_inventory() {
    object curr;
    
    /* Rebuild from driver's contents list */
    inventory = ({});
    
    curr = contents(this_object());
    while (curr) {
        inventory = inventory + ({ curr });
        curr = next_object(curr);
    }
}
```

**Issue**: Array concatenation in loop (creates new array each time).

**Analysis**: This is unavoidable since we're iterating a linked list. The driver doesn't provide a way to get all contents as an array directly.

**Action**: NO CHANGE - This is the correct approach given NetCI's API.

---

## MEDIUM PRIORITY - Redundant Function Calls

### 7. auto.c - remove_element() - Overcomplicated Logic

**Problem**: Function has unnecessary complexity and redundant checks.

**File**: `/sys/auto.c` (lines 245-268)

**Current Code**:
```c
remove_element(arr, element) {
    int pos, i;
    
    if (!arr) return ({});
    
    pos = member_array(element, arr);
    if (pos == -1) return arr;
    
    /* Build new array without the element by concatenating parts */
    /* We can't use untyped variables, so we'll use a different approach */
    /* Just rebuild the array by filtering out the element */
    if (pos == 0 && sizeof(arr) == 1) {
        return ({});
    }
    
    /* Use array arithmetic to build result */
    if (pos == 0) {
        /* Remove first element - can't slice, so rebuild manually */
        return arr - ({ element });
    }
    
    /* For other positions, use array subtraction */
    return arr - ({ element });
}
```

**Issues**:
- Special case for `pos == 0 && sizeof(arr) == 1` is redundant
- Special case for `pos == 0` is redundant
- All cases just do `arr - ({ element })`
- **IMPORTANT**: `arr - ({ element })` removes ALL occurrences, not just the first
- Function should be renamed or documented to clarify it removes all occurrences

**Optimized Version**:
```c
/* Remove ALL occurrences of element from array */
remove_element(arr, element) {
    if (!arr) return ({});
    
    /* Array subtraction removes all occurrences */
    return arr - ({ element });
}
```

**Impact**: 24 lines → 6 lines, same functionality (removes all occurrences).

---

### 8. Multiple Files - Redundant NULL Checks Before Operations ✅ VERIFIED

**Problem**: Many functions check `if (!properties)` before every operation when properties should be initialized in init().

**Examples**:

**object.c**:
```c
set_property(key, value) {
    if (!properties) properties = ([]);  /* REDUNDANT - init() guaranteed */
    properties[key] = value;
}

query_property(key) {
    if (!properties) return NULL;  /* REDUNDANT - init() guaranteed */
    return properties[key];
}
```

**Analysis**: 
- ✅ **CONFIRMED**: `init()` is guaranteed to run on object creation
- These NULL checks are redundant defensive programming
- Can be safely removed

**Optimized Version**:
```c
set_property(key, value) {
    properties[key] = value;
}

query_property(key) {
    return properties[key];
}
```

**Impact**: Cleaner code, fewer unnecessary checks. Apply to:
- `/inherits/object.c` - set_property, query_property, delete_property
- `/inherits/room.c` - exit management functions
- Other files with similar patterns

**Action**: Remove redundant NULL checks in property/mapping operations.

---

### 9. auto.c - compile_object() Wrapper - CRITICAL BUG 

**Problem**: Function wraps `compile_object()` but just calls itself recursively.

**File**: `/sys/auto.c` (lines 171-185)

**Current Code**:
```c
compile_object(path) {
    object result;
    
    if (!path) return NULL;
    
    /* Try to compile */
    result = compile_object(path);  /* RECURSIVE CALL TO ITSELF! */
    
    if (!result) {
        /* Log compilation failure */
        syslog("Failed to compile: " + path);
    }
    
    return result;
}
```

**Issue**: ⚠️ **INFINITE RECURSION BUG** - This is calling itself recursively!

**Analysis**: This should delegate to boot.c's compile_object() function, not call itself.

**Correct Fix**:
```c
compile_object(path) {
    object boot, result;
    
    if (!path) return NULL;
    
    /* Delegate to boot.c */
    boot = atoo("/boot");
    if (!boot) return NULL;
    
    result = boot.compile_object(path);
    
    if (!result) {
        syslog("Failed to compile: " + path);
    }
    
    return result;
}
```

**Action**: ⚠️ **FIX IMMEDIATELY** - This is a critical bug, not just an optimization.

---

### 10. object.c - Wrapper Functions - Keep for Compatibility 

**Problem**: Many functions are just one-line wrappers.

**File**: `/inherits/object.c`

**Examples**:
```c
get_short() {
    return query_short();
}

get_long() {
    return query_long();
}

get_environment() {
    return query_environment();
}

get_type() {
    return query_type();
}
```

**Analysis**: 
- These provide both `get_*()` and `query_*()` naming conventions
- Support compatibility with different MUD codebases (DGD vs TMI-2 style)
- Properties use mappings for dynamic data (descriptions can change)
- Static data (like name) can use direct variables
- Short/long descriptions may have modifiers and environmental changes

**Reasoning**:
- Dynamic properties → use mapping (`properties["short"]`)
- Static properties → can use direct variables
- Wrappers → provide API compatibility

**Action**: ✅ KEEP - Valuable for compatibility and flexibility.

---

## LOW PRIORITY - Missing Early Exits

### 11. room.c - room_tell() - Missing Early Exit

**Problem**: Function checks inventory size but doesn't exit early if empty.

**File**: `/inherits/room.c` (lines 293-311)

**Current Code**:
```c
room_tell(msg, exclude) {
    int i;
    object ob;
    
    if (!msg) return;
    if (!inventory || sizeof(inventory) == 0) return;  /* Good early exit */
    
    /* Send to each object in room */
    for (i = 0; i < sizeof(inventory); i++) {  /* Could cache sizeof */
        ob = inventory[i];
        if (ob) {
            /* Check if this object is excluded */
            if (!exclude || member_array(ob, exclude) == -1) {
                /* Send message if object can receive it */
                call_other(ob, "listen", msg);
            }
        }
    }
}
```

**Optimization**:
```c
room_tell(msg, exclude) {
    int i, count;
    object ob;
    
    if (!msg) return;
    if (!inventory) return;
    
    count = sizeof(inventory);
    if (count == 0) return;
    
    /* Send to each object in room */
    for (i = 0; i < count; i++) {  /* Use cached count */
        ob = inventory[i];
        if (ob) {
            if (!exclude || member_array(ob, exclude) == -1) {
                call_other(ob, "listen", msg);
            }
        }
    }
}
```

**Impact**: Minor - caches `sizeof()` result instead of calling it every loop iteration.

---

### 12. player.c - load_data() - File Reading ✅ VERIFIED

**Problem**: Reads file line by line in a loop, building string incrementally.

**File**: `/sys/player.c` (lines 372-378)

**Current Code**:
```c
/* Read entire file - fread reads one line at a time, so loop */
pos = 0;
data = "";
line = fread(path, pos);
while (line && line != 0 && line != -1) {
    data = data + line;
    line = fread(path, pos);
}
```

**Issue**: String concatenation in loop creates new strings on each iteration.

**Analysis**: 
- ✅ **VERIFIED**: NetCI only has `fread()` which uses `fgets()` internally
- `fread()` reads one line at a time by design
- No "read entire file" function exists in NetCI
- `cat()` exists but calls `listen()` on each line (not suitable for data loading)

**Conclusion**: Current implementation is the correct approach given NetCI's API.

**Action**: ✅ NO CHANGE - This is the proper way to read files in NetCI.

---

## SUMMARY

### High Priority (Do First)
1. ✅ **Remove duplicate capitalize()** - 4 files, ~60 lines saved
2. ✅ **Remove duplicate find_object()** - 1 file, ~40 lines saved
3. ✅ **Remove duplicate present()** - 1 file, ~20 lines saved
4. ⚠️ **Fix auto.c compile_object() recursion bug** - CRITICAL BUG

**Total Lines Saved**: ~120 lines of duplicate code

### Medium Priority
5. **Optimize auto.c trim()** - Reduce function calls
6. **Simplify auto.c remove_element()** - 24 lines → 5 lines
7. **Verify array slicing support** - Could optimize room.c

### Low Priority
8. **Cache sizeof() in loops** - Minor performance gain
9. **Verify file reading API** - Check for better alternatives

### Keep As-Is (Good Reasons)
- Wrapper functions (get_*/query_*) - Compatibility
- sync_inventory() loop - No better alternative
- Defensive NULL checks - Safe programming

---

## Implementation Priority

**Phase 1: Critical Fixes**
1. Fix auto.c compile_object() recursion bug
2. Remove duplicate capitalize() from 4 files
3. Remove duplicate find_object() from living.c
4. Remove duplicate present() from living.c

**Phase 2: Simplifications**
5. Simplify remove_element() function
6. Optimize trim() function

**Phase 3: Verification & Optimization**
7. Verify object init() guarantees
8. Verify array slicing support
9. Verify file reading API
10. Cache sizeof() in hot loops

---

## Testing Requirements

After all changes, offer test ideas but do not create unless explicitly asked:
1. **Verify compilation** - All files must compile
2. **Test affected commands** - Ensure functionality unchanged
3. **Test login flow** - user.c and player.c changes
4. **Test room descriptions** - room.c changes
5. **Test inventory** - container.c changes

---

**Estimated Effort**: 3-4 hours for Phase 1  
**Estimated Benefit**: 
- ~120 lines of code removed
- 1 critical bug fixed
- Improved maintainability
- Single source of truth for common functions
