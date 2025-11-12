# Implementation Guide: String Indexing by []

**Status**: Not Implemented  
**Priority**: Low (Recommend NOT implementing)  
**Type**: Driver Feature (Language Syntax)  
**Estimated Effort**: 12-16 hours (much more complex than originally estimated)

**CRITICAL ISSUE**: NetCI has **dynamic typing** - variables don't have compile-time types. The parser cannot distinguish between `arr[0]` (array) and `str[0]` (string) at compile time!

---

## Overview

Implement string indexing syntax in the NetCI driver, allowing direct character access using array-like bracket notation: `str[index]` and potentially `str[start..end]` for substring extraction.

---

## Current State

### What Exists
- **String manipulation efuns**: `leftstr()`, `rightstr()`, `midstr()`, `strlen()`, `instr()`
- Manual string extraction using these functions

### Current Approach
```c
/* Get first character */
string first = leftstr(str, 1);

/* Get last character */
string last = rightstr(str, 1);

/* Get character at position 5 */
string char = midstr(str, 5, 1);

/* Get substring */
string sub = midstr(str, 5, 10);
```

### Limitations
- ❌ No direct character access by index
- ❌ Verbose syntax for simple operations
- ❌ Function call overhead
- ❌ Not intuitive for developers from other languages

---

## Proposed Syntax

### Character Access
```c
str[index]             // Get character at index (returns string of length 1)
str[0]                 // First character
str[-1]                // Last character
str[strlen(str)-1]     // Last character (explicit)
```

### Substring Extraction (Optional Extension)
```c
str[start..end]        // Substring from start to end (inclusive)
str[start..]           // Substring from start to end
str[..end]             // Substring from beginning to end
str[..]                // Copy entire string
```

### Examples
```c
string text = "hello world";

string first = text[0];              // "h"
string last = text[-1];              // "d"
string fifth = text[4];              // "o"

/* With slicing extension */
string hello = text[0..4];           // "hello"
string world = text[6..10];          // "world"
string skip_first = text[1..];       // "ello world"
string skip_last = text[..-2];       // "hello worl"
```

---

## Design Decisions

### Question 1: Return Type

**Option A: Return single character string**
```c
string char = str[0];  // Returns "h" (string of length 1)
```
- ✅ Consistent with LPC string type system
- ✅ Can be used directly in string operations
- ✅ Compatible with existing code
- ❌ Not a true "character" type

**Option B: Return integer (ASCII/Unicode value)**
```c
int char_code = str[0];  // Returns 104 (ASCII 'h')
```
- ✅ Efficient for character comparisons
- ✅ Matches C language behavior
- ❌ Requires conversion for string operations
- ❌ Breaking change from string-based model

**Recommendation**: **Option A** - Return single-character string for LPC consistency.

---

### Question 2: Negative Indices

**Option A: Support negative indices (Python-style)**
```c
str[-1]   // Last character
str[-2]   // Second-to-last character
```
- ✅ Intuitive and convenient
- ✅ Matches array slicing behavior
- ❌ Slight performance overhead (index calculation)

**Option B: No negative indices**
```c
str[strlen(str)-1]  // Must calculate explicitly
```
- ✅ Simpler implementation
- ❌ Less convenient

**Recommendation**: **Option A** - Support negative indices for consistency with array slicing.

---

### Question 3: Out-of-Bounds Behavior

**Option A: Return empty string**
```c
str[100]  // Returns "" if index out of bounds
```
- ✅ Safe, no runtime errors
- ✅ Consistent with LPC error handling
- ❌ Silent failures

**Option B: Return NULL**
```c
str[100]  // Returns NULL if index out of bounds
```
- ✅ Explicit failure indication
- ❌ Requires NULL checks

**Option C: Runtime error**
```c
str[100]  // Throws runtime error
```
- ✅ Catches bugs early
- ❌ Can crash code

**Recommendation**: **Option A** - Return empty string for safety and LPC consistency.

---

## Implementation Details

### THE FUNDAMENTAL PROBLEM: Dynamic Typing

#### Deep Dive: NetCI Type System (VERIFIED)

**Type Definitions** (`/src/object.h` lines 9-34):
```c
#define INTEGER 0
#define STRING 1
#define OBJECT 2
#define ASM_INSTR 3
#define GLOBAL_L_VALUE 4
#define LOCAL_L_VALUE 5
// ... more types ...
#define ARRAY 17                    /* heap-allocated array pointer */
#define MAPPING 18                  /* heap-allocated mapping pointer */
```

**Variable Structure** (`/src/object.h` lines 51-66):
```c
struct var {
    unsigned char type;  // Runtime type tag (0-18)
    union {
        signed long integer;
        char *string;
        struct object *objptr;
        struct heap_array *array_ptr;   // For ARRAY type
        struct heap_mapping *mapping_ptr; // For MAPPING type
        // ... other types
    } value;
};
```

**Variable Declaration** (`/src/object.h` lines 124-131):
```c
struct var_tab {
    char *name;
    unsigned int base;
    struct array_size *array;  // Used ONLY for arrays
    int is_mapping;  // 1 if declared with 'mapping' keyword
    struct var_tab *next;
};
```

**CRITICAL FINDING**: Variables have TWO type systems:
1. **Compile-time**: `is_mapping` flag (only for mappings!)
2. **Runtime**: `type` field in `struct var` (0-18)

**The Problem**:
```c
string s = "hello";  // var_tab has: is_mapping=0, array=NULL
int *arr = ({1, 2, 3}); // var_tab has: is_mapping=0, array=size_info

s[0]    // Parser sees: is_mapping=0, array=NULL -> assumes STRING? NO!
arr[0]  // Parser sees: is_mapping=0, array=size_info -> array subscript
```

**At compile time**, the parser in `parse_var()` (lines 384-425) ONLY knows:
- `is_mapping` flag (for mappings declared with `mapping` keyword)
- `array` field (for statically-sized arrays like `int arr[10]`)

**It does NOT know** if a variable is a string vs dynamic array!

**Current Implementation** (`compile1.c` lines 384-425):
```c
if (token.type==LARRAY_TOK) {
    if (the_var->is_mapping) {
        // Mapping subscript - KNOWN at compile time
    } else {
        // Array subscript - ASSUMES it's an array!
        // Could actually be a string at runtime!
    }
}
```

**Runtime Type Detection** (`/src/sys4.c` lines 78-96, `s_typeof()`):
```c
int s_typeof(...) {
    if (pop(&tmp,rts,obj)) return 1;
    result=tmp.type;  // Gets runtime type (0-18)
    tmp.value.integer=result;
    push(&tmp,rts);
}
```

**Proof**: `typeof()` returns the RUNTIME type, not compile-time!

### Possible Solutions

#### Option A: Runtime Type Dispatch (RECOMMENDED)

Generate a generic `S_SUBSCRIPT` opcode and let runtime check the type:

**Modify `parse_var()`**:
```c
if (token.type==LARRAY_TOK) {
    // Don't try to determine type at compile time
    // Just generate subscript operation
    if (parse_exp(file_info,curr_fn,loc_sym,0,0))
        return file_info->phys_line;
    add_code_instr(curr_fn,S_SUBSCRIPT);  // Generic subscript
}
```

**Runtime handler** (`interp.c`):
```c
case S_SUBSCRIPT:
    // Pop index and object
    if (pop(&index_var, rts, obj)) goto error;
    if (pop(&obj_var, rts, obj)) goto error;
    
    // Dispatch based on actual type
    if (obj_var.type == ARRAY) {
        // Do array subscript
        result = array_subscript(&obj_var, index_var.value.integer);
    } else if (obj_var.type == MAPPING) {
        // Do mapping lookup
        result = mapping_lookup(&obj_var, &index_var);
    } else if (obj_var.type == STRING) {
        // Do string indexing
        result = string_index(&obj_var, index_var.value.integer);
    } else {
        // Error: cannot subscript this type
        goto error;
    }
    push(&result, rts);
    break;
```

**Problem**: This requires refactoring ALL existing array and mapping subscript code!

**How It Would Work**:

1. **Compile time**: Generate generic `S_SUBSCRIPT` opcode
2. **Runtime**: Check `var.type` field and dispatch:

```c
case S_SUBSCRIPT:
    pop(&index_var, rts, obj);
    pop(&obj_var, rts, obj);
    
    switch (obj_var.type) {  // Runtime type check!
        case ARRAY:
            // obj_var.value.array_ptr->elements[index]
            break;
        case MAPPING:
            // mapping_lookup(obj_var.value.mapping_ptr, &index_var)
            break;
        case STRING:
            // obj_var.value.string[index] (new!)
            break;
        default:
            // Error: cannot subscript this type
            break;
    }
```

**Refactoring Required**:
- Current array subscript code in `interp.c` (LOCAL_REF/GLOBAL_REF)
- Current mapping subscript code in `interp.c`
- Would need to unify into single dispatch mechanism
- Estimated 8-12 hours just for refactoring

#### Option B: Separate Syntax

Don't use `[]` for strings. Use a function or method:
```c
char = charAt(str, index);
char = str->charAt(index);  // If method call syntax exists
```

**Advantages**:
- ✅ No parser changes needed
- ✅ Clear intent
- ✅ Can implement as SFUN in auto.c

**Disadvantages**:
- ❌ Less intuitive than `str[0]`
- ❌ Inconsistent with other languages

#### Option C: Don't Implement (RECOMMENDED)

**Existing efuns work fine**:
```c
char = leftstr(str, 1);           // First char
char = rightstr(str, 1);          // Last char  
char = midstr(str, pos, 1);       // Char at pos
substr = midstr(str, start, len); // Substring
```

**Why this is the best option**:
- ✅ No complexity added to parser
- ✅ No runtime overhead
- ✅ Works today
- ✅ Standard LPC approach (LDMud, DGD don't have string indexing)
- ✅ Avoids the dynamic typing problem entirely

---

### New Opcodes

#### Location: `/src/instr.h`

```c
#define STRING_INDEX        100  /* string_index(string, index) */
#define STRING_SLICE        101  /* string_slice(string, start, end) */
#define STRING_SLICE_START  102  /* string_slice_start(string, start) */
#define STRING_SLICE_END    103  /* string_slice_end(string, end) */
#define STRING_SLICE_ALL    104  /* string_slice_all(string) */
```

---

### Runtime Implementation

#### Location: `/src/string_ops.c` (new file)

```c
#include "config.h"
#include "object.h"
#include "instr.h"
#include "interp.h"
#include "protos.h"
#include "globals.h"

/* ========================================================================
 * STRING INDEXING IMPLEMENTATION
 * ======================================================================== */

/* Normalize negative index to positive
 * -1 becomes len-1, -2 becomes len-2, etc.
 */
static int normalize_string_index(int index, int len) {
    if (index < 0) {
        index = len + index;
    }
    /* Clamp to valid range */
    if (index < 0) index = 0;
    if (index >= len) index = len - 1;
    return index;
}

/* Get single character from string at index
 * Returns single-character string or empty string if out of bounds
 */
char *string_index_internal(char *str, int index) {
    int len;
    char *result;
    
    if (!str) {
        result = MALLOC(1);
        result[0] = '\0';
        return result;
    }
    
    len = strlen(str);
    if (len == 0) {
        result = MALLOC(1);
        result[0] = '\0';
        return result;
    }
    
    /* Normalize negative index */
    index = normalize_string_index(index, len);
    
    /* Check bounds */
    if (index < 0 || index >= len) {
        result = MALLOC(1);
        result[0] = '\0';
        return result;
    }
    
    /* Extract single character */
    result = MALLOC(2);
    result[0] = str[index];
    result[1] = '\0';
    
    return result;
}

/* Extract substring from start to end (inclusive)
 * Returns new string or empty string if invalid range
 */
char *string_slice_internal(char *str, int start, int end) {
    int len, slice_len;
    char *result;
    
    if (!str) {
        result = MALLOC(1);
        result[0] = '\0';
        return result;
    }
    
    len = strlen(str);
    if (len == 0) {
        result = MALLOC(1);
        result[0] = '\0';
        return result;
    }
    
    /* Normalize indices */
    start = normalize_string_index(start, len);
    end = normalize_string_index(end, len);
    
    /* Ensure start <= end */
    if (start > end) {
        result = MALLOC(1);
        result[0] = '\0';
        return result;
    }
    
    /* Calculate slice length */
    slice_len = end - start + 1;
    
    /* Allocate and copy */
    result = MALLOC(slice_len + 1);
    strncpy(result, str + start, slice_len);
    result[slice_len] = '\0';
    
    return result;
}

/* ========================================================================
 * OPCODE HANDLERS
 * ======================================================================== */

/* str[index] */
int op_string_index(struct object *caller, struct object *obj,
                    struct object *player, struct var_stack **rts) {
    struct var str_var, index_var, result_var;
    char *str, *result;
    int index;
    
    /* Pop index */
    if (pop(&index_var, rts, obj)) return 1;
    if (resolve_var(&index_var, obj)) return 1;
    if (index_var.type != INTEGER) {
        clear_var(&index_var);
        return 1;
    }
    index = index_var.value.integer;
    
    /* Pop string */
    if (pop(&str_var, rts, obj)) return 1;
    if (resolve_var(&str_var, obj)) return 1;
    if (str_var.type != STRING) {
        clear_var(&str_var);
        clear_var(&index_var);
        return 1;
    }
    str = str_var.value.string;
    
    /* Get character */
    result = string_index_internal(str, index);
    
    /* Clean up and push result */
    clear_var(&str_var);
    clear_var(&index_var);
    
    result_var.type = STRING;
    result_var.value.string = result;
    push(&result_var, rts);
    
    return 0;
}

/* str[start..end] */
int op_string_slice(struct object *caller, struct object *obj,
                    struct object *player, struct var_stack **rts) {
    struct var str_var, start_var, end_var, result_var;
    char *str, *result;
    int start, end;
    
    /* Pop end index */
    if (pop(&end_var, rts, obj)) return 1;
    if (resolve_var(&end_var, obj)) return 1;
    if (end_var.type != INTEGER) {
        clear_var(&end_var);
        return 1;
    }
    end = end_var.value.integer;
    
    /* Pop start index */
    if (pop(&start_var, rts, obj)) return 1;
    if (resolve_var(&start_var, obj)) return 1;
    if (start_var.type != INTEGER) {
        clear_var(&start_var);
        clear_var(&end_var);
        return 1;
    }
    start = start_var.value.integer;
    
    /* Pop string */
    if (pop(&str_var, rts, obj)) return 1;
    if (resolve_var(&str_var, obj)) return 1;
    if (str_var.type != STRING) {
        clear_var(&str_var);
        clear_var(&start_var);
        clear_var(&end_var);
        return 1;
    }
    str = str_var.value.string;
    
    /* Extract substring */
    result = string_slice_internal(str, start, end);
    
    /* Clean up and push result */
    clear_var(&str_var);
    clear_var(&start_var);
    clear_var(&end_var);
    
    result_var.type = STRING;
    result_var.value.string = result;
    push(&result_var, rts);
    
    return 0;
}

/* str[start..] */
int op_string_slice_start(struct object *caller, struct object *obj,
                          struct object *player, struct var_stack **rts) {
    struct var str_var, start_var, result_var;
    char *str, *result;
    int start, len;
    
    /* Pop start index */
    if (pop(&start_var, rts, obj)) return 1;
    if (resolve_var(&start_var, obj)) return 1;
    if (start_var.type != INTEGER) {
        clear_var(&start_var);
        return 1;
    }
    start = start_var.value.integer;
    
    /* Pop string */
    if (pop(&str_var, rts, obj)) return 1;
    if (resolve_var(&str_var, obj)) return 1;
    if (str_var.type != STRING) {
        clear_var(&str_var);
        clear_var(&start_var);
        return 1;
    }
    str = str_var.value.string;
    len = strlen(str);
    
    /* Slice from start to end */
    result = string_slice_internal(str, start, len - 1);
    
    /* Clean up and push result */
    clear_var(&str_var);
    clear_var(&start_var);
    
    result_var.type = STRING;
    result_var.value.string = result;
    push(&result_var, rts);
    
    return 0;
}

/* str[..end] */
int op_string_slice_end(struct object *caller, struct object *obj,
                        struct object *player, struct var_stack **rts) {
    struct var str_var, end_var, result_var;
    char *str, *result;
    int end;
    
    /* Pop end index */
    if (pop(&end_var, rts, obj)) return 1;
    if (resolve_var(&end_var, obj)) return 1;
    if (end_var.type != INTEGER) {
        clear_var(&end_var);
        return 1;
    }
    end = end_var.value.integer;
    
    /* Pop string */
    if (pop(&str_var, rts, obj)) return 1;
    if (resolve_var(&str_var, obj)) return 1;
    if (str_var.type != STRING) {
        clear_var(&str_var);
        clear_var(&end_var);
        return 1;
    }
    str = str_var.value.string;
    
    /* Slice from beginning to end */
    result = string_slice_internal(str, 0, end);
    
    /* Clean up and push result */
    clear_var(&str_var);
    clear_var(&end_var);
    
    result_var.type = STRING;
    result_var.value.string = result;
    push(&result_var, rts);
    
    return 0;
}

/* str[..] - copy entire string */
int op_string_slice_all(struct object *caller, struct object *obj,
                        struct object *player, struct var_stack **rts) {
    struct var str_var, result_var;
    char *str, *result;
    int len;
    
    /* Pop string */
    if (pop(&str_var, rts, obj)) return 1;
    if (resolve_var(&str_var, obj)) return 1;
    if (str_var.type != STRING) {
        clear_var(&str_var);
        return 1;
    }
    str = str_var.value.string;
    len = strlen(str);
    
    /* Copy entire string */
    result = string_slice_internal(str, 0, len - 1);
    
    /* Clean up and push result */
    clear_var(&str_var);
    
    result_var.type = STRING;
    result_var.value.string = result;
    push(&result_var, rts);
    
    return 0;
}
```

---

### Interpreter Integration

#### Location: `/src/interp.c`

Add to opcode dispatch table:
```c
case STRING_INDEX:
    if (op_string_index(caller, obj, player, &rts)) goto error;
    break;

case STRING_SLICE:
    if (op_string_slice(caller, obj, player, &rts)) goto error;
    break;

case STRING_SLICE_START:
    if (op_string_slice_start(caller, obj, player, &rts)) goto error;
    break;

case STRING_SLICE_END:
    if (op_string_slice_end(caller, obj, player, &rts)) goto error;
    break;

case STRING_SLICE_ALL:
    if (op_string_slice_all(caller, obj, player, &rts)) goto error;
    break;
```

---

## Usage Examples

### Before (Current Code)
```c
/* Get first character */
string first = leftstr(text, 1);

/* Get last character */
string last = rightstr(text, 1);

/* Get character at position 5 */
string char = midstr(text, 5, 1);

/* Get substring "hello" from "hello world" */
string hello = midstr(text, 0, 5);

/* Check if first character is uppercase */
if (leftstr(name, 1) == upcase(leftstr(name, 1))) {
    write("Name starts with uppercase\n");
}
```

### After (With String Indexing)
```c
/* Get first character */
string first = text[0];

/* Get last character */
string last = text[-1];

/* Get character at position 5 */
string char = text[5];

/* Get substring "hello" from "hello world" */
string hello = text[0..4];

/* Check if first character is uppercase */
if (name[0] == upcase(name[0])) {
    write("Name starts with uppercase\n");
}
```

---

### More Examples

```c
string text = "Hello, World!";

/* Character access */
string h = text[0];              // "H"
string exclaim = text[-1];       // "!"
string comma = text[5];          // ","

/* Substring extraction */
string hello = text[0..4];       // "Hello"
string world = text[7..11];      // "World"
string skip_first = text[1..];   // "ello, World!"
string skip_last = text[..-2];   // "Hello, World"

/* Iteration */
for (int i = 0; i < strlen(text); i++) {
    write("Character " + itoa(i) + ": " + text[i] + "\n");
}

/* Character comparison */
if (text[0] >= "A" && text[0] <= "Z") {
    write("Starts with uppercase\n");
}
```

---

## Testing Requirements

### Unit Tests

Create `/libs/ci200fs/test/test_string_indexing.c`:

```c
#include <std.h>

test_basic_index() {
    string text = "hello";
    
    if (text[0] != "h") return 0;
    if (text[1] != "e") return 0;
    if (text[4] != "o") return 0;
    
    return 1;
}

test_negative_index() {
    string text = "hello";
    
    if (text[-1] != "o") return 0;
    if (text[-2] != "l") return 0;
    if (text[-5] != "h") return 0;
    
    return 1;
}

test_out_of_bounds() {
    string text = "hello";
    
    if (text[100] != "") return 0;
    if (text[-100] != "") return 0;
    
    return 1;
}

test_empty_string() {
    string text = "";
    
    if (text[0] != "") return 0;
    if (text[-1] != "") return 0;
    
    return 1;
}

test_substring() {
    string text = "hello world";
    
    if (text[0..4] != "hello") return 0;
    if (text[6..10] != "world") return 0;
    if (text[0..-1] != "hello world") return 0;
    
    return 1;
}

test_open_slices() {
    string text = "hello world";
    
    if (text[6..] != "world") return 0;
    if (text[..4] != "hello") return 0;
    if (text[..] != "hello world") return 0;
    
    return 1;
}

test_special_chars() {
    string text = "a\nb\tc";
    
    if (text[0] != "a") return 0;
    if (text[1] != "\n") return 0;
    if (text[2] != "b") return 0;
    if (text[3] != "\t") return 0;
    if (text[4] != "c") return 0;
    
    return 1;
}

run_tests() {
    int passed = 0;
    int total = 7;
    
    write("Testing string indexing...\n");
    
    if (test_basic_index()) passed++;
    if (test_negative_index()) passed++;
    if (test_out_of_bounds()) passed++;
    if (test_empty_string()) passed++;
    if (test_substring()) passed++;
    if (test_open_slices()) passed++;
    if (test_special_chars()) passed++;
    
    write("Passed " + itoa(passed) + "/" + itoa(total) + " tests\n");
    return (passed == total);
}
```

---

## Migration Plan

### Phase 1: Implement Character Access
1. Add string indexing opcode
2. Implement runtime handler
3. Test with unit tests
4. Document feature

### Phase 2: Implement Slicing (Optional)
1. Add string slicing opcodes
2. Implement runtime handlers
3. Test with unit tests
4. Document feature

### Phase 3: Update Existing Code (Optional)
1. Identify uses of `leftstr(str, 1)`, `rightstr(str, 1)`, etc.
2. Replace with index syntax where appropriate
3. Keep existing functions for compatibility

---

## Performance Considerations

- **Character Access**: O(1) for ASCII, O(n) for UTF-8 (if supported)
- **Substring**: O(n) where n = substring length
- **Memory**: Creates new string (does not modify original)
- **Optimization**: Consider string interning for single characters

---

## Compatibility

### Similar Features in Other Languages

**Python**: `str[index]`, `str[start:end]`  
**Ruby**: `str[index]`, `str[start..end]`  
**JavaScript**: `str[index]`, `str.substring(start, end)`  
**C**: `str[index]` (returns char, not string)  
**LDMud**: No native string indexing (uses efuns)  
**DGD**: No native string indexing (uses kfuns)  
**FluffOS**: Has string indexing support

---

## Alternative: Keep Existing Efuns

**Arguments Against Implementation**:
- Existing efuns work fine
- Not standard in LPC
- Adds complexity to compiler
- May confuse developers familiar with other LPC drivers

**Arguments For Implementation**:
- More intuitive syntax
- Matches modern language conventions
- Reduces function call overhead
- Easier to read and write

**Recommendation**: Implement as **optional enhancement** - keep existing efuns for compatibility.

---

## References

- **Current Code**: Uses `leftstr()`, `rightstr()`, `midstr()` throughout
- **FluffOS**: Has string indexing support
- **Python**: String indexing reference
- **Ruby**: String indexing reference

---

**Priority**: LOW-MEDIUM - Nice to have, but existing efuns work fine.

**Next Steps**:
1. Decide if feature is worth the implementation effort
2. If yes, implement character access first
3. Consider substring slicing as separate enhancement
4. Keep existing efuns for backward compatibility
