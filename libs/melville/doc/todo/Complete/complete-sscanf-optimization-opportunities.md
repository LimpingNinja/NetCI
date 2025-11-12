# sscanf() Optimization Opportunities

**Date**: November 6, 2025  
**Status**: Analysis Complete  
**Priority**: MEDIUM (Code quality improvement, not critical)

## Overview

Now that `sscanf()` is implemented in NetCI (40/43 tests passing), we can replace manual string parsing with cleaner sscanf() calls. This document identifies all locations where manual parsing using `instr()`, `leftstr()`, `rightstr()`, and `midstr()` could be simplified.

---

## High Priority Replacements

### 1. cmd_d.c - Command/Argument Splitting (Lines 60-70)

**Current Code:**
```c
/* Parse verb and arguments - simple split on first space */
space_pos = instr(input, 1, " ");  /* NetCI uses 1-based indexing */
if (space_pos == 0) {
    /* No space found, entire input is the verb */
    verb = input;
    args = NULL;
} else {
    /* Split: verb is before space, args is after */
    verb = leftstr(input, space_pos - 1);
    args = rightstr(input, strlen(input) - space_pos);
}
```

**Replacement with sscanf():**
```c
/* Parse verb and arguments using sscanf */
if (sscanf(input, "%s %s", verb, args) == 2) {
    /* Got both verb and args */
} else if (sscanf(input, "%s", verb) == 1) {
    /* Got only verb, no args */
    args = NULL;
} else {
    /* Empty input */
    verb = NULL;
    args = NULL;
}
```

**Benefits:**
- Cleaner, more readable code
- Handles whitespace automatically
- Standard pattern used in many MUDs

**File:** `/sys/daemons/cmd_d.c`  
**Function:** `process_command()`  
**Lines:** 60-70

---

### 2. cmd_d.c - Filename Parsing (Lines 212-217)

**Current Code:**
```c
/* Check if file starts with _ and ends with .c */
if (strlen(file) > 3 && leftstr(file, 1) == "_") {
    /* Remove leading _ and trailing .c */
    if (rightstr(file, 2) == ".c") {
        /* midstr is 1-indexed! pos=2 to skip the _, len=strlen-3 to remove _.c */
        verb = midstr(file, 2, strlen(file) - 3);
        cmds = cmds + ({ verb });
    }
}
```

**Replacement with sscanf():**
```c
/* Extract command name from _verb.c pattern */
if (sscanf(file, "_%s.c", verb) == 1) {
    cmds = cmds + ({ verb });
}
```

**Benefits:**
- Much simpler and clearer
- No manual index calculations
- Pattern matching is explicit

**File:** `/sys/daemons/cmd_d.c`  
**Function:** `rehash()`  
**Lines:** 212-217

---

### 3. boot.c - Path Username Extraction (Lines 360-367)

**Current Code:**
```c
/* Extract username from path */
next_slash = instr(path, 12, "/");  /* 12 = length of "/world/wiz/" */
if (!next_slash) {
    /* Path is exactly /world/wiz/username */
    path_user = rightstr(path, strlen(path) - 11);
} else {
    /* Path is /world/wiz/username/... */
    path_user = midstr(path, 12, next_slash - 12);
}
```

**Replacement with sscanf():**
```c
/* Extract username from path */
if (sscanf(path, "/world/wiz/%s/%s", path_user, rest) == 2) {
    /* Path has subdirectories: /world/wiz/username/... */
} else if (sscanf(path, "/world/wiz/%s", path_user) == 1) {
    /* Path is exactly /world/wiz/username */
} else {
    /* Invalid path format */
    return 0;
}
```

**Benefits:**
- No magic numbers (12, 11)
- Pattern is self-documenting
- Handles both cases cleanly

**File:** `/boot.c`  
**Function:** `is_in_home_dir()`  
**Lines:** 360-367

---

## Medium Priority Replacements

### 4. _look.c - "at" Prefix Removal (Lines 34-37)

**Current Code:**
```c
/* Remove "at" if present */
if (leftstr(args, 3) == "at ") {
    args = rightstr(args, strlen(args) - 3);
    args = trim(args);
}
```

**Replacement with sscanf():**
```c
/* Remove "at" if present */
string target;
if (sscanf(args, "at %s", target) == 1) {
    args = target;
}
```

**Benefits:**
- Simpler
- Handles whitespace automatically
- No manual trimming needed

**File:** `/cmds/player/_look.c`  
**Function:** `execute()`  
**Lines:** 34-37

---

### 5. room.c - String Replacement (Lines 158-164)

**Current Code:**
```c
pos = instr(str, find);
if (pos == -1) return str;

before = leftstr(str, pos);
after = rightstr(str, strlen(str) - pos - strlen(find));

return before + replace + after;
```

**Note:** ✅ **UPDATE AVAILABLE** - This manual implementation is now obsolete! Native `replace_string()` efun is available (opcode 150).

**Action:** ❌ REMOVE MANUAL IMPLEMENTATION - Use native `replace_string()` efun

**Current Code** (OBSOLETE):
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

**Replacement:**
```c
/* Just use the native efun - no manual implementation needed! */
string result = replace_string(str, find, replace);
```

**Benefits:**
- Native C implementation (faster)
- Replaces ALL occurrences (not just first)
- No manual string manipulation needed
- Handles edge cases (empty strings, etc.)

**File:** `/inherits/room.c`  
**Function:** `replace_string()`  
**Lines:** 145-159

---

## Low Priority / Keep As-Is

### 6. auto.c - trim() Function (Lines 222-229)

**Current Code:**
```c
/* Trim leading spaces */
while (strlen(str) > 0 && leftstr(str, 1) == " ") {
    str = rightstr(str, strlen(str) - 1);
}

/* Trim trailing spaces */
while (strlen(str) > 0 && rightstr(str, 1) == " ") {
    str = leftstr(str, strlen(str) - 1);
}
```

**Analysis:** This is a utility function that needs to handle all whitespace characters, not just spaces. sscanf() wouldn't improve this - it's fine as-is.

**Action:** NO CHANGE NEEDED

**File:** `/sys/auto.c`  
**Function:** `trim()`  
**Lines:** 222-229

---

### 7. Multiple capitalize() Functions

**Current Code (appears in auto.c, user.c, player.c, room.c):**
```c
first = leftstr(str, 1);
rest = rightstr(str, strlen(str) - 1);
return upcase(first) + rest;
```

**Analysis:** This is simple character manipulation, not parsing. sscanf() wouldn't help here.

**Action:** NO CHANGE NEEDED (but consider consolidating into auto.c only)

**Files:** `/sys/auto.c`, `/sys/user.c`, `/sys/player.c`, `/inherits/room.c`

---

### 8. Object Name Prefix Checking

**Current Code (appears in auto.c, living.c):**
```c
if (leftstr(name, 1) == "/" || leftstr(name, 1) == "#") {
    result = atoo(name);
    ...
}

if (leftstr(name, 1) == "*") {
    /* Player reference */
    ...
}
```

**Analysis:** These are simple prefix checks, not parsing. sscanf() would be overkill.

**Action:** NO CHANGE NEEDED

**Files:** `/sys/auto.c`, `/inherits/living.c`

---

### 9. boot.c - path_starts_with() (Line 346)

**Current Code:**
```c
return (leftstr(path, prefix_len) == prefix);
```

**Analysis:** This is a utility function for prefix checking. sscanf() wouldn't improve it.

**Action:** NO CHANGE NEEDED

**File:** `/boot.c`  
**Function:** `path_starts_with()`  
**Line:** 346

---

## Implementation Priority

### Phase 1: High Impact (Do First)
1. ✅ **cmd_d.c command parsing** - Most frequently called, clearest improvement
2. ✅ **cmd_d.c filename parsing** - Much simpler with sscanf
3. ✅ **boot.c path parsing** - Removes magic numbers, clearer logic

### Phase 2: Nice to Have
4. **_look.c "at" removal** - Minor improvement, rarely called

### Phase 3: No Action Needed
- trim() function - works fine as-is
- capitalize() functions - simple operations
- Prefix checking - appropriate use of leftstr()
- replace_string() - not a parsing operation

---

## Testing Requirements

Offer test ideas after each replacement but do not create unless explicitly asked:

1. **Unit Test**: Verify the function works with various inputs
2. **Integration Test**: Test the command/feature in-game
3. **Edge Cases**: Test with:
   - Empty strings
   - Single words
   - Multiple spaces
   - Special characters
   - Very long strings

---

## Notes

- sscanf() in NetCI returns the number of successful conversions
- NetCI uses 1-based indexing for strings (important for instr, midstr)
- sscanf() handles whitespace automatically (space, tab, newline)
- Current sscanf() test suite: 40/43 tests passing

---

## Summary

**Total Opportunities Found**: 9  
**High Priority Replacements**: 3  
**Medium Priority Replacements**: 1  
**Keep As-Is**: 5

**Estimated Effort**: 2-3 hours for all high priority replacements  
**Estimated Benefit**: Cleaner, more maintainable code with fewer manual index calculations

---

**Next Steps:**
1. Review this document with team
2. Implement high priority replacements
3. Test thoroughly
4. Update documentation to show sscanf() usage patterns
