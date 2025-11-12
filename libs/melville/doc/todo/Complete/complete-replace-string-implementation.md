# Implementation Guide: replace_string() EFUN

**Status**: Not Implemented  
**Priority**: Medium  
**Type**: EFUN (Engine Function) or SFUN (Simulated EFUN)  
**Estimated Effort**: 2-4 hours

---

## Overview

Implement `replace_string()` as either a native EFUN in the NetCI driver or as an SFUN in the mudlib. This function performs pattern-based string replacement, replacing all occurrences of a substring with another string.

---

## Current State

### What Exists
- **Manual implementation** in `/inherits/room.c` (lines 151-165)
- Uses `instr()`, `leftstr()`, `rightstr()` for single replacement
- Only replaces **first occurrence**

### Current Implementation
```c
/* Simple string replacement helper */
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

### Limitations
- Only replaces first occurrence
- Inefficient for multiple replacements
- Duplicated across files if needed elsewhere
- Not available as a global efun

---

## Function Signature

```c
string replace_string(string str, string pattern, string replacement)
```

### Parameters
- **str**: The source string to search within
- **pattern**: The substring to find and replace
- **replacement**: The string to replace pattern with

### Returns
- New string with all occurrences of `pattern` replaced by `replacement`
- Returns original string if `pattern` not found
- Returns original string if `str` is NULL or empty

### Behavior
- Replaces **ALL occurrences** of pattern (not just first)
- Case-sensitive matching
- Empty pattern returns original string (avoid infinite loops)
- NULL replacement treated as empty string ""

---

## Implementation Options

### Option A: Native EFUN (Recommended)

Implement in the NetCI driver as a native C function.

**Advantages**:
- ✅ Fastest performance (native C)
- ✅ Available to all LPC code automatically
- ✅ No memory churn from LPC string operations
- ✅ Consistent with other string efuns

**Disadvantages**:
- ❌ Requires driver modification
- ❌ Requires driver recompilation

**Implementation Location**: `/src/sys6a.c` (with other string efuns)

**Verified Facts**:
- ✅ `/src/sys6a.c` EXISTS and contains: `s_midstr`, `s_strlen`, `s_leftstr`, `s_rightstr`, `s_subst`, `s_instr`
- ✅ String efuns are registered in `scall_array` in `/src/compile1.c` (lines 37-55)
- ✅ Efun dispatch table in `/src/interp.c` (line 30) includes all string efuns
- ✅ Current highest opcode: `S_MAPPING_LITERAL` (147) in `/src/instr.h`

**Steps**:
1. Add `OPER_PROTO(s_replace_string)` to `/src/protos.h`
2. Add `#define S_REPLACE_STRING 152` to `/src/instr.h` (after line 186)
3. Implement `s_replace_string()` in `/src/sys6a.c` (after `s_instr`, around line 350)
4. Add `"replace_string"` to `scall_array` in `/src/compile1.c` (line 45, after "instr")
5. Add `s_replace_string` to dispatch table in `/src/interp.c` (line 30, after `s_instr`)
6. Update `NUM_SCALLS` in `/src/instr.h` (currently 110, increment to 111)
7. Create help file `/libs/ci200fs/help/help.7/replace_.string`
8. Update both `/src/Makefile` AND `/src/autoconf.in` if creating new file

---

### Option B: SFUN in auto.c (QUICK START)

Implement as a simulated efun in `/sys/auto.c`.

**Advantages**:
- ✅ No driver changes needed
- ✅ No recompilation required (LPC is interpreted)
- ✅ Can be updated instantly
- ✅ Easier to test and debug
- ✅ Can implement in 15-30 minutes

**Disadvantages**:
- ❌ Slower than native implementation
- ❌ More memory allocation overhead (string concatenation in loop)
- ❌ Creates intermediate strings

**Implementation Location**: `/libs/melville/sys/auto.c`

**Actual Implementation**:
```c
/* Replace all occurrences of pattern in string with replacement
 * Returns new string with replacements made
 */
replace_string(str, pattern, replacement) {
    string result;
    int pos, pattern_len;
    
    if (!str || !pattern) return str;
    if (!replacement) replacement = "";
    
    pattern_len = strlen(pattern);
    if (pattern_len == 0) return str;
    
    result = str;
    
    /* Keep replacing until no more matches found */
    while (1) {
        pos = instr(result, pattern);
        if (pos == -1) break;
        
        /* Split and rebuild */
        result = leftstr(result, pos) + replacement + 
                 rightstr(result, strlen(result) - pos - pattern_len);
    }
    
    return result;
}
```

**Complexity**: O(n * m) where n = string length, m = number of replacements  
**Memory**: Creates new string on each replacement (inefficient but functional)

---

## Detailed Implementation

### Recommendation: Start with Option B (SFUN), Upgrade to Option A Later

**Rationale**:
1. SFUN can be implemented in 15-30 minutes
2. Provides immediate functionality
3. No driver recompilation needed
4. Can upgrade to native EFUN later for performance

---

### Option A: Native EFUN Implementation (For Performance)

#### 1. Add to `/src/instr.h`
```c
#define REPLACE_STRING   93  /* replace_string(string s, string pattern, string repl) */
```

#### 2. Add to `/src/protos.h`
```c
OPER_PROTO(s_replace_string)
```

#### 3. Add to `/src/compile1.c` efun_table
```c
"midstr","strlen","leftstr","rightstr","subst","instr","otoa","itoa",
"replace_string",  /* Add here */
```

#### 4. Implement in `/src/sys6a.c`
```c
int s_replace_string(struct object *caller, struct object *obj, 
                     struct object *player, struct var_stack **rts) {
    struct var tmp1, tmp2, tmp3;
    char *str, *pattern, *replacement;
    char *result, *pos, *write_pos;
    int pattern_len, replacement_len, count, result_len;
    
    /* Pop arguments: NUM_ARGS, replacement, pattern, str */
    if (pop(&tmp3, rts, obj)) return 1;
    if (tmp3.type != NUM_ARGS || tmp3.value.num != 3) {
        clear_var(&tmp3);
        return 1;
    }
    
    /* Get replacement string */
    if (pop(&tmp3, rts, obj)) return 1;
    if (resolve_var(&tmp3, obj)) return 1;
    if (tmp3.type != STRING) {
        clear_var(&tmp3);
        return 1;
    }
    replacement = tmp3.value.string;
    replacement_len = strlen(replacement);
    
    /* Get pattern string */
    if (pop(&tmp2, rts, obj)) return 1;
    if (resolve_var(&tmp2, obj)) return 1;
    if (tmp2.type != STRING) {
        clear_var(&tmp2);
        clear_var(&tmp3);
        return 1;
    }
    pattern = tmp2.value.string;
    pattern_len = strlen(pattern);
    
    /* Get source string */
    if (pop(&tmp1, rts, obj)) return 1;
    if (resolve_var(&tmp1, obj)) return 1;
    if (tmp1.type != STRING) {
        clear_var(&tmp1);
        clear_var(&tmp2);
        clear_var(&tmp3);
        return 1;
    }
    str = tmp1.value.string;
    
    /* Handle edge cases */
    if (pattern_len == 0) {
        /* Empty pattern - return original string */
        push(&tmp1, rts);
        clear_var(&tmp2);
        clear_var(&tmp3);
        return 0;
    }
    
    /* Count occurrences to calculate result size */
    count = 0;
    pos = str;
    while ((pos = strstr(pos, pattern)) != NULL) {
        count++;
        pos += pattern_len;
    }
    
    /* If no matches, return original */
    if (count == 0) {
        push(&tmp1, rts);
        clear_var(&tmp2);
        clear_var(&tmp3);
        return 0;
    }
    
    /* Allocate result buffer */
    result_len = strlen(str) + (count * (replacement_len - pattern_len));
    result = (char *)MALLOC(result_len + 1);
    
    /* Perform replacements */
    write_pos = result;
    pos = str;
    while (1) {
        char *next = strstr(pos, pattern);
        if (next == NULL) {
            /* Copy remaining string */
            strcpy(write_pos, pos);
            break;
        }
        
        /* Copy text before pattern */
        int copy_len = next - pos;
        strncpy(write_pos, pos, copy_len);
        write_pos += copy_len;
        
        /* Copy replacement */
        strcpy(write_pos, replacement);
        write_pos += replacement_len;
        
        /* Move past pattern */
        pos = next + pattern_len;
    }
    
    /* Push result and clean up */
    tmp1.type = STRING;
    tmp1.value.string = result;
    push(&tmp1, rts);
    
    clear_var(&tmp2);
    clear_var(&tmp3);
    
    return 0;
}
```

#### 5. Add to `/src/interp.c` dispatch table
```c
s_midstr,s_strlen,s_leftstr,s_rightstr,s_subst,s_instr,s_otoa,s_itoa,
s_replace_string,  /* Add here */
```

#### 6. Create help file `/libs/ci200fs/help/help.7/replace_.string`
```
Section 7 - replace_string

replace_string(str, pattern, replacement)

string str;
string pattern;
string replacement;

Replaces all occurrences of 'pattern' in 'str' with 'replacement'.
Returns a new string with all replacements made.

If pattern is not found, returns the original string unchanged.
If pattern is empty, returns the original string unchanged.
If replacement is NULL, treats it as an empty string.

Examples:
    replace_string("hello world", "world", "there")  // "hello there"
    replace_string("foo foo foo", "foo", "bar")      // "bar bar bar"
    replace_string("test", "x", "y")                 // "test" (no match)

See Also: instr(7), subst(7)
```

---

### Option B: SFUN Implementation in auto.c

Add to `/sys/auto.c`:

```c
/* Replace all occurrences of pattern in string with replacement
 * Returns new string with replacements made
 */
replace_string(str, pattern, replacement) {
    string result, before, after;
    int pos, pattern_len;
    
    if (!str || !pattern) return str;
    if (!replacement) replacement = "";
    
    pattern_len = strlen(pattern);
    if (pattern_len == 0) return str;
    
    result = str;
    
    /* Keep replacing until no more matches found */
    while (1) {
        pos = instr(result, pattern);
        if (pos == -1) break;
        
        /* Split and rebuild */
        before = leftstr(result, pos);
        after = rightstr(result, strlen(result) - pos - pattern_len);
        result = before + replacement + after;
    }
    
    return result;
}
```

**Note**: This SFUN version is less efficient due to repeated string operations in a loop.

---

## Testing Requirements

### Unit Tests

Create `/libs/melville/test/test_replace_string.c`:

```c
#include <std.h>

test_basic_replacement() {
    string result;
    
    result = replace_string("hello world", "world", "there");
    if (result != "hello there") {
        write("FAIL: Basic replacement\n");
        return 0;
    }
    return 1;
}

test_multiple_replacements() {
    string result;
    
    result = replace_string("foo foo foo", "foo", "bar");
    if (result != "bar bar bar") {
        write("FAIL: Multiple replacements\n");
        return 0;
    }
    return 1;
}

test_no_match() {
    string result;
    
    result = replace_string("hello world", "xyz", "abc");
    if (result != "hello world") {
        write("FAIL: No match should return original\n");
        return 0;
    }
    return 1;
}

test_empty_pattern() {
    string result;
    
    result = replace_string("hello", "", "x");
    if (result != "hello") {
        write("FAIL: Empty pattern should return original\n");
        return 0;
    }
    return 1;
}

test_empty_replacement() {
    string result;
    
    result = replace_string("hello world", "world", "");
    if (result != "hello ") {
        write("FAIL: Empty replacement (deletion)\n");
        return 0;
    }
    return 1;
}

test_overlapping_pattern() {
    string result;
    
    result = replace_string("aaa", "aa", "b");
    /* Should replace first "aa", leaving "ba" */
    if (result != "ba") {
        write("FAIL: Overlapping pattern\n");
        return 0;
    }
    return 1;
}

run_tests() {
    int passed = 0;
    int total = 6;
    
    write("Testing replace_string()...\n");
    
    if (test_basic_replacement()) passed++;
    if (test_multiple_replacements()) passed++;
    if (test_no_match()) passed++;
    if (test_empty_pattern()) passed++;
    if (test_empty_replacement()) passed++;
    if (test_overlapping_pattern()) passed++;
    
    write("Passed " + itoa(passed) + "/" + itoa(total) + " tests\n");
    return (passed == total);
}
```

### Integration Tests

1. Test in room.c exit messages (already uses replace_string)
2. Test with special characters: `\n`, `\t`, quotes
3. Test with very long strings (performance)
4. Test with NULL inputs (error handling)

---

## Migration Plan

### Phase 1: Implement EFUN (if choosing Option A)
1. Add to driver source files
2. Recompile driver
3. Test with unit tests
4. Create help documentation

### Phase 2: Update Existing Code
1. Remove manual implementation from `/inherits/room.c`
2. Update any other files using manual string replacement
3. Add note that `replace_string()` is now available globally

### Phase 3: Documentation
1. Update mudlib documentation
2. Add examples to help files
3. Note in STATUS.md or CHANGES.md

---

## Performance Considerations

### Native EFUN (Option A)
- **Time Complexity**: O(n) where n = length of string
- **Space Complexity**: O(n) for result string
- **Optimizations**: 
  - Single pass through string
  - Pre-calculate result size
  - Use `strstr()` for pattern matching

### SFUN (Option B)
- **Time Complexity**: O(n * m) where m = number of replacements
- **Space Complexity**: O(n * m) due to intermediate strings
- **Limitations**:
  - Creates new string on each replacement
  - Multiple passes through string

---

## Recommendation

**Implement as Native EFUN (Option A)** because:
1. Better performance for common use case
2. Consistent with other string efuns (`instr`, `leftstr`, etc.)
3. Available globally without imports
4. Standard feature in most MUD drivers (LDMud, DGD, FluffOS)

---

## References

- **LDMud**: `replace_string()` is a standard efun
- **DGD**: Uses `implode(explode(str, pattern), replacement)` pattern
- **FluffOS**: Has native `replace_string()` efun
- **Current Implementation**: `/inherits/room.c` lines 151-165

---

**Next Steps**: Decide on Option A or B, then implement and test.

---

## IMPLEMENTATION COMPLETE ✅

### Implementation Summary

**Chosen Approach:** Native EFUN (Option A)

**New EFUN:** `replace_string(string str, string search, string replace)`
- Replaces all occurrences of `search` with `replace` in `str`
- Returns new string with replacements
- Returns original string if no matches found
- Returns original string if search is empty
- Returns 0 on memory allocation failure

### Files Modified:

1. ✅ `/src/sfun_strings.c` - Added `s_replace_string()` implementation (145 lines)
   - Efficient two-pass algorithm: count occurrences, then build result
   - Uses `strstr()` for finding matches
   - Pre-calculates result length to avoid reallocation
   - Handles edge cases (empty search, no matches, allocation failure)
   - Proper memory management with cleanup on all paths

2. ✅ `/src/instr.h` - Added `S_REPLACE_STRING` opcode (150), incremented `NUM_SCALLS` to 113

3. ✅ `/src/compile1.c` - Added "replace_string" to `scall_array`

4. ✅ `/src/interp.c` - Added `s_replace_string` to dispatch table

5. ✅ `/src/protos.h` - Added `OPER_PROTO(s_replace_string)`

### Test Suite Created:

**File:** `/libs/melville/sys/test_replace_string.c`

**Tests:**
- ✅ Basic replacement (single occurrence)
- ✅ Multiple occurrences
- ✅ No matches (returns original)
- ✅ Empty search string (returns original)
- ✅ Empty replace string (removes matches)
- ✅ Case sensitivity
- ✅ Size changes (longer/shorter replacements)

**Test Integration:** Added to `/libs/melville/boot.c` for automatic testing on startup

### Usage Examples:

```lpc
// Basic replacement
string result = replace_string("hello world", "world", "NetCI");
// result = "hello NetCI"

// Multiple occurrences
result = replace_string("foo bar foo baz foo", "foo", "qux");
// result = "qux bar qux baz qux"

// Remove substring (empty replace)
result = replace_string("hello world", "o", "");
// result = "hell wrld"

// No match (returns original)
result = replace_string("hello world", "xyz", "abc");
// result = "hello world"
```

### Performance Characteristics:

- **Time Complexity**: O(n) where n = length of string
- **Space Complexity**: O(n) for result string
- **Algorithm**: Two-pass (count matches, then build result)
- **Optimization**: Pre-calculates result size to avoid reallocation

### Test Results:
**All tests passed (6/6)** ✅
- ✅ Basic replacement
- ✅ Multiple occurrences  
- ✅ No matches
- ✅ Empty strings (handled as INTEGER 0)
- ✅ Case sensitivity
- ✅ Size changes

### Important Implementation Notes:

**Empty String Handling:**
In NetCI, empty strings are represented as INTEGER 0, not STRING "". The implementation handles this by checking:
```c
if (var.type == INTEGER && var.value.integer == 0) {
    /* Handle 0 as empty string */
    str = "";
    len = 0;
}
```

This matches the pattern used in other NetCI string functions like `s_leftstr()`, `s_rightstr()`, and `s_midstr()`.

**Empty String Comparison:**
Direct comparison with `""` doesn't work in NLPC. Use `strlen(str) != 0` to check for empty strings.

### Status: 
**Production-ready** ✅

**Opcode Used:** 150
