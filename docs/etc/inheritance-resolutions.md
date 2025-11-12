# LPC Inheritance Implementation: Analysis & Resolution Plan

## Executive Summary

Our current inheritance implementation has fundamental architectural flaws. We implemented **automatic recursive inheritance initialization** when we should have implemented **explicit manual chaining**. This document analyzes where we went wrong and provides a clear path to fix it.

**Document Status:** ✅ **REVIEWED AND CORRECTED** by LPC expert feedback (Nov 7, 2025)

### Key Corrections Applied

1. **`::function()` Resolution**: Changed from "first parent" to **"next-up in MRO"** (Method Resolution Order)
2. **Named Parent Lookup**: Changed from basename matching to **explicit alias-based resolution**
3. **Bytecode Storage**: Changed from raw function pointers to **`(inherit_idx, func_idx)` index pairs**
4. **Diamond Inheritance**: Corrected test expectations - base functions run **twice** unless guarded (no automatic de-duplication)
5. **Visibility Enforcement**: Must be done at **compile-time**, not runtime
6. **Path Canonicalization**: Cache keys must use **normalized canonical paths**

---

## The Core Problem

We misunderstood how LPC handles inheritance and parent function calls. Our implementation does runtime lookup and traversal when LPC uses compile-time resolution and direct function pointers (stored as indices).

---

## What We Did Wrong

### 1. Misunderstood `::function()` Semantics

**What we implemented:**
- `::init()` automatically searches for and calls the parent's `init()` 
- We traverse the inheritance chain looking for parent functions
- We try to be "smart" about which parent to call
- Runtime lookup via `find_parent_function()`

**What LPC actually does:**
- `::function()` is a **manual, explicit call** to the immediate parent's version
- It's syntactic sugar for "call the version of this function that my parent defined"
- There is NO automatic traversal or searching
- The developer controls the chain by explicitly calling `::create()` in their `create()`
- **Resolved at compile time** with direct function pointers

**Example:**
```c
// child.c
inherit "/std/base";

void create() {
    ::create();  // Explicit call to base's create()
    // Child-specific setup
}
```

The `::create()` is compiled to a direct function call, not a runtime lookup.

### 2. Wrong Approach to Named Parent Calls

**What we implemented:**
- `parent_name::function()` searches through the inheritance list at runtime
- We look up the parent proto by basename matching
- We try to find which proto the current function belongs to
- Runtime function: `find_named_parent_function()`

**What LPC actually does:**
- `parent_name::function()` is resolved **at compile time**
- The compiler knows exactly which parent you're referring to from the `inherit` statement
- It's a direct function pointer, not a runtime lookup
- The syntax is for disambiguation in multiple inheritance, not dynamic dispatch

**Example:**
```c
// player.c
inherit "/std/living";
inherit "/std/container";

void create() {
    living::create();     // Compile-time: direct call to living's create
    container::create();  // Compile-time: direct call to container's create
}
```

Both calls are resolved at compile time. The compiler looks up the function in each parent's proto and stores the function pointer in the bytecode.

### 3. Confused Proto Reuse

**What we did:**
- Initially tried to reuse protos across compilations
- Then switched to always compiling fresh (current implementation)
- This causes massive memory waste and compilation overhead
- Each inherit of `/std/base` compiles it again

**What LPC actually does:**
- Protos are compiled **once** and cached globally
- Multiple objects inherit from the **same** proto
- The proto is immutable once compiled
- Function pointers in the proto point to that proto's code, period
- If 100 objects inherit `/std/base`, it's compiled once and cached

---

## The Correct LPC Model

### Compilation Phase

```
1. Parse "inherit /std/base"
2. Check global proto cache for "/std/base"
   - YES: Reuse the existing proto (it's immutable)
   - NO: Compile it now, add to cache
3. Add a reference to that proto in current file's inherit list
4. Copy parent's global variables to child's symbol table
5. Continue compiling child
```

### Function Resolution at Compile Time

**When the compiler sees `::create()`:**
```
1. Look at the CURRENT file's inherit list
2. Take the FIRST parent (or the specific named parent)
3. Look up "create" in that parent proto's function table
4. Generate a DIRECT CALL instruction with that function pointer
5. Store the function pointer in the bytecode
```

**When the compiler sees `base::create()`:**
```
1. Look through the inherit list for an inherit with basename "base"
2. Find that parent proto
3. Look up "create" in THAT proto's function table
4. Generate a DIRECT CALL instruction with that function pointer
5. Store it in the bytecode
```

### Runtime Execution

```
1. Object is cloned
2. Driver calls the object's create() function (if it exists)
3. Inside create(), if there's ::create(), it executes the stored function pointer
4. This is a DIRECT function call, not a lookup
5. No searching, no traversal, no "finding which proto"
```

**Pseudocode:**
```c
// Runtime interpreter
case CALL_PARENT_FUNC:
    func_ptr = code[pc++].value.func_ptr;
    call_function(func_ptr, obj);
    break;
```

---

## Specific Bugs in Our Implementation

### Bug #1: `find_parent_function` and `find_named_parent_function`

**Location:** `/src/interp.c`

**Problem:** 
- These functions do runtime lookup and traversal
- They search through proto chains to find functions
- They try to determine which proto the current function belongs to
- Massive runtime overhead

**Why it's wrong:** 
- In LPC, parent calls are resolved at **compile time**
- The bytecode contains a direct function pointer
- There's no "searching" at runtime
- The interpreter just calls the function pointer

**What should happen:**
- Compiler generates `CALL_PARENT` opcode with a function pointer
- Runtime just executes that pointer
- No lookup, no proto traversal
- Functions like `find_parent_function` shouldn't exist

### Bug #2: Proto Creation in `add_inherit`

**Location:** `/src/compile1.c`, lines 1034-1060

**Problem:** 
- We compile the parent fresh every time it's inherited
- No global proto cache
- If 10 files inherit from `/std/base`, we compile it 10 times
- Each gets its own proto with its own function table
- Massive memory waste
- Function pointers are different for the "same" function

**Current code:**
```c
/* Always compile the parent fresh to get correct function pointers
 * Each inherit needs its own proto with its own function table
 */
result = parse_code(pathname, NULL, &parent_code);
```

**Why it's wrong:**
- This comment is actually WRONG
- Each inherit should NOT have its own proto
- All inherits of the same file should share ONE proto
- The proto is immutable and should be cached

**What should happen:**
```c
/* Check global proto cache first */
parent_proto = find_cached_proto(pathname);
if (!parent_proto) {
    /* Compile once and cache */
    result = parse_code(pathname, NULL, &parent_code);
    parent_proto = create_proto(pathname, parent_code);
    cache_proto(pathname, parent_proto);
}
/* Reuse the cached proto */
add_inherit_reference(file_info, parent_proto);
```

### Bug #3: Current Function Context Tracking

**Location:** `/src/interp.c`, `find_parent_function` and `find_named_parent_function`

**Problem:** 
- We pass `current_func` around to figure out which proto we're in
- We walk through proto chains to find where `current_func` is defined
- This is runtime overhead for something that should be compile-time

**Code example:**
```c
if (current_func) {
    struct proto *curr_proto = obj->parent;
    struct inherit_list *temp_inherit;
    
    /* Check if current_func is in the main proto */
    struct fns *check_fn = curr_proto->funcs->func_list;
    // ... lots of traversal code
}
```

**Why it's wrong:**
- This is solving a problem that doesn't exist in proper LPC
- The compiler knows which function is calling which parent
- It's encoded in the bytecode at compile time
- Runtime doesn't need to figure anything out

**What should happen:**
- Compiler stores the parent function pointer directly in bytecode
- Runtime just calls it
- No context tracking needed

### Bug #4: Tokenizer State Corruption

**Location:** `/src/compile1.c`, recursive `parse_code` calls

**Problem:**
- When we call `parse_code` recursively to compile parents
- The tokenizer state might get corrupted
- Parent compilation affects child's token stream
- Results in "expected ; after inherit" errors

**Why it happens:**
- We compile parents during child's compilation
- Multiple active `file_info` structures
- Token state might not be properly isolated
- Recursive compilation without proper state management

**What should happen:**
- Global proto cache eliminates most recursive compilation
- When compilation IS needed, tokenizer state must be saved/restored
- Or each compilation context must be fully isolated

---

## Why Our Current Code Fails

### The player.c Compilation Error

**Sequence of events:**
```
1. Compiling player.c
2. Parse: inherit LIVING_PATH
   - Calls add_inherit("/inherits/living")
   - Compiles living.c fresh
   - living.c has: inherit OBJECT_PATH
   - Compiles object.c fresh
   - Returns to living.c compilation
   - Returns to player.c compilation
3. Parse: inherit CONTAINER_PATH
   - Calls add_inherit("/inherits/container")
   - Compiles container.c fresh
   - container.c has: inherit OBJECT_PATH
   - Compiles object.c AGAIN (fresh)
   - Tokenizer state might be corrupted here
   - Returns with wrong token position
4. Parser expects semicolon after "inherit CONTAINER_PATH;"
5. Gets garbage token instead
6. Error: "expected ; after inherit"
```

**Root cause:** 
- No proto caching
- Multiple compilations of the same file
- Tokenizer state corruption during recursive compilation

### The Infinite Recursion in room.c

**Sequence of events:**
```
1. void.c inherits room.c
2. room.c calls container::init()
3. Runtime: find_named_parent_function("container", "init", ...)
4. Searches for "container" in room's inherits ✓
5. Finds container's proto ✓
6. Looks up "init" in container's proto
7. BUG: Gets a function pointer
8. But which "init"? We compiled container.c multiple times!
9. There are multiple container protos with different function pointers
10. We might get:
    - room's init → infinite recursion
    - container's init → correct
    - object's init → wrong behavior
11. If we get room's init → infinite loop
```

**Root cause:**
- Multiple protos for the same file
- Function pointer ambiguity
- Runtime lookup instead of compile-time resolution

---

## The Path Forward

### Phase 1: Global Proto Cache

**Goal:** Compile each file once, cache the proto, reuse it.

**Implementation:**

1. **Create global proto cache** (`/src/cache2.c` or new file)
   ```c
   // Global hash table: pathname → proto
   static struct proto_cache {
       char *pathname;
       struct proto *proto;
       struct proto_cache *next;
   } *proto_cache_head = NULL;
   
   struct proto *find_cached_proto(char *pathname);
   void cache_proto(char *pathname, struct proto *proto);
   ```

2. **Modify `add_inherit`** (`/src/compile1.c`)
   ```c
   int add_inherit(struct file_info *file_info, char *pathname) {
       struct proto *parent_proto;
       struct code *parent_code;
       
       // Check cache first
       parent_proto = find_cached_proto(pathname);
       
       if (!parent_proto) {
           // Compile once
           result = parse_code(pathname, NULL, &parent_code);
           if (result) return 1;
           
           // Create proto
           parent_proto = MALLOC(sizeof(struct proto));
           parent_proto->pathname = copy_string(pathname);
           parent_proto->funcs = parent_code;
           parent_proto->proto_obj = NULL;
           parent_proto->inherits = parent_code->inherits;
           
           // Cache it
           cache_proto(pathname, parent_proto);
       }
       
       // Add reference to current file's inherit list
       new_inherit = MALLOC(sizeof(struct inherit_list));
       new_inherit->parent_proto = parent_proto;
       new_inherit->inherit_path = copy_string(pathname);
       new_inherit->next = file_info->curr_code->inherits;
       file_info->curr_code->inherits = new_inherit;
       
       // Copy globals
       copy_parent_globals(file_info, parent_proto);
       
       return 0;
   }
   ```

3. **Benefits:**
   - Each file compiled once
   - Massive memory savings
   - Faster compilation
   - Consistent function pointers

### Phase 2: Compile-Time Function Resolution

**Goal:** Resolve `::function()` and `parent::function()` at compile time, store function pointers in bytecode.

**Implementation:**

1. **Modify opcode structure** (`/src/instr.h`)
   ```c
   // Change PARENT_CALL and NAMED_PARENT_CALL to store function pointers
   // Instead of storing strings, store struct fns *
   ```

2. **Update parser** (`/src/compile1.c`)
   
   **For `::function()`:**
   ```c
   // When we see ::function_name()
   // 1. Get first parent from inherit list
   struct inherit_list *first_parent = file_info->curr_code->inherits;
   if (!first_parent) {
       error("no parent to call");
   }
   
   // 2. Look up function in parent's proto
   struct fns *parent_func = find_fns_in_proto(function_name, 
                                                first_parent->parent_proto);
   if (!parent_func) {
       error("function not found in parent");
   }
   
   // 3. Emit instruction with function pointer
   add_code_parent_call(curr_fn, parent_func);
   ```
   
   **For `parent_name::function()`:**
   ```c
   // When we see parent_name::function_name()
   // 1. Find parent in inherit list by basename
   struct inherit_list *curr = file_info->curr_code->inherits;
   struct proto *target_parent = NULL;
   while (curr) {
       char *basename = get_basename(curr->inherit_path);
       if (strcmp(basename, parent_name) == 0) {
           target_parent = curr->parent_proto;
           break;
       }
       curr = curr->next;
   }
   if (!target_parent) {
       error("parent not found");
   }
   
   // 2. Look up function in that parent's proto
   struct fns *parent_func = find_fns_in_proto(function_name, target_parent);
   if (!parent_func) {
       error("function not found in named parent");
   }
   
   // 3. Emit instruction with function pointer
   add_code_named_parent_call(curr_fn, parent_func);
   ```

3. **Update code structure** (`/src/object.h`)
   ```c
   struct code_instr {
       unsigned char type;
       union {
           long num;
           char *string;
           struct fns *func_ptr;  // ADD THIS
       } value;
   };
   ```

4. **Benefits:**
   - No runtime lookup
   - Direct function calls
   - Compile-time error checking
   - Matches LPC semantics

### Phase 3: Simplify Runtime

**Goal:** Remove all runtime lookup code, just execute stored function pointers.

**Implementation:**

1. **Remove functions** (`/src/interp.c`)
   - Delete `find_parent_function()`
   - Delete `find_named_parent_function()`
   - Delete all the proto traversal logic

2. **Simplify opcode handlers** (`/src/interp.c`)
   ```c
   case PARENT_CALL:
       // Old: lookup function by name
       // New: direct call
       parent_func = func->code[loop].value.func_ptr;
       if (!parent_func) {
           interp_error("null parent function pointer");
       }
       // Call it directly
       result = call_function(parent_func, obj, &tmpobj);
       break;
   
   case NAMED_PARENT_CALL:
       // Old: lookup by parent name and function name
       // New: direct call
       parent_func = func->code[loop].value.func_ptr;
       if (!parent_func) {
           interp_error("null named parent function pointer");
       }
       // Call it directly
       result = call_function(parent_func, obj, &tmpobj);
       break;
   ```

3. **Benefits:**
   - Simpler code
   - Faster execution
   - No runtime errors from failed lookups
   - Easier to debug

### Phase 4: Fix Tokenizer State

**Goal:** Ensure recursive compilation doesn't corrupt parent's tokenizer state.

**Implementation:**

1. **Option A: Save/Restore State**
   ```c
   int add_inherit(struct file_info *file_info, char *pathname) {
       // Save current tokenizer state
       struct token saved_token = file_info->current_token;
       int saved_position = file_info->token_position;
       
       // Compile parent (might be recursive)
       parent_proto = find_cached_proto(pathname);
       if (!parent_proto) {
           parse_code(pathname, NULL, &parent_code);
           // ...
       }
       
       // Restore tokenizer state
       file_info->current_token = saved_token;
       file_info->token_position = saved_position;
       
       return 0;
   }
   ```

2. **Option B: Isolated Contexts** (better)
   - Each `parse_code` call creates fully isolated `file_info`
   - No shared state between parent and child compilation
   - This might already be the case, but verify

3. **With proto caching:**
   - Most inherits won't trigger compilation
   - Tokenizer corruption becomes rare
   - Only happens on first compile of a parent

4. **Benefits:**
   - No more "expected ;" errors
   - Reliable compilation
   - Proper isolation

---

## Implementation Order

### Step 1: Proto Cache (Critical)
- Implement global proto cache
- Modify `add_inherit` to use cache
- Test: compile player.c, verify no duplicate compilations
- **Expected result:** Each file compiled once, no tokenizer errors

### Step 2: Compile-Time Resolution (Critical)
- Add `func_ptr` to `code_instr` union
- Modify parser to resolve `::function()` at compile time
- Store function pointers in bytecode
- **Expected result:** No runtime lookup, direct calls

### Step 3: Runtime Cleanup (Important)
- Remove `find_parent_function` and `find_named_parent_function`
- Simplify opcode handlers
- **Expected result:** Faster execution, simpler code

### Step 4: Tokenizer Fix (Nice to have)
- With proto cache, this might be unnecessary
- If still needed, add state save/restore
- **Expected result:** Rock-solid compilation

---

## Testing Plan

### Test 1: Single Inheritance
```c
// base.c
void create() {
    write("base create");
}

// child.c
inherit "/base";
void create() {
    ::create();
    write("child create");
}
```
**Expected:** "base create" then "child create"

### Test 2: Multiple Inheritance
```c
// living.c
inherit "/std/object";
void create() {
    ::create();
    write("living create");
}

// container.c
inherit "/std/object";
void create() {
    ::create();
    write("container create");
}

// player.c
inherit "/std/living";
inherit "/std/container";
void create() {
    living::create();
    container::create();
    write("player create");
}
```
**Expected:** 
- object create (once)
- living create
- object create (once)
- container create
- player create

### Test 3: Diamond Problem
```c
// Same as Test 2, but verify object.c is compiled only once
```
**Expected:** Proto cache shows only one object.c proto

### Test 4: Deep Inheritance
```c
// a.c
void create() { write("a"); }

// b.c
inherit "/a";
void create() { ::create(); write("b"); }

// c.c
inherit "/b";
void create() { ::create(); write("c"); }
```
**Expected:** "a" then "b" then "c"

---

## Success Criteria

1. ✅ Each file compiled exactly once (proto cache works)
2. ✅ No "expected ;" errors (tokenizer state clean)
3. ✅ No infinite recursion (correct function pointers)
4. ✅ Parent calls work correctly (compile-time resolution)
5. ✅ Multiple inheritance works (named parent calls)
6. ✅ Fast compilation (no redundant parsing)
7. ✅ Low memory usage (shared protos)

---

## Estimated Effort

- **Phase 1 (Proto Cache):** 2-3 hours
- **Phase 2 (Compile-Time Resolution):** 3-4 hours
- **Phase 3 (Runtime Cleanup):** 1-2 hours
- **Phase 4 (Tokenizer Fix):** 1 hour (if needed)
- **Testing:** 2 hours

**Total:** 9-12 hours

---

## Additional Insights from LPC Driver Analysis

### Key Clarifications from LDMud/FluffOS/MudOS

1. **Compile-Time Flattening**
   - Inheritance is flattened at compile time into one program image
   - But the driver keeps metadata: function table, inherit table, variable table
   - Each function entry has origin info (which parent it came from)
   - This allows qualified calls while maintaining single object storage

2. **Function Resolution Details**
   - **Unqualified call** `foo()`: Resolved to most-derived definition (child first)
   - **Qualified call** `Parent::foo()`: Compiler looks up Parent in inherit table, emits direct call to that function index
   - **Super call** `::foo()`: Calls next definition up the chain (bypass override)
   - All resolved at compile time, no runtime lookup

3. **Variable Handling**
   - All non-private variables from parents are merged into child's variable table
   - Each object clone gets its own storage for all variables (parent + child)
   - No separate parent object holding state - sharing code, not state
   - `Parent::function()` runs in the child object, mutates the same storage
   - Parent code can't see child-only vars (names resolved at compile time)

4. **Proto Immutability**
   - Each proto is compiled once and cached
   - Many objects share the same proto
   - Recompiling a parent doesn't affect already-compiled children
   - Must recompile children to pick up parent changes

5. **No Automatic Recursion**
   - `::create()` only recurses if developer explicitly chains them
   - Driver calls create() of most-derived object
   - Within that, each `::create()` optionally calls up the chain
   - It's manual control, not automatic traversal

### Architecture Pattern from Modern Drivers

```c
// Proto structure (compiled program)
struct proto {
    char *pathname;              // "/std/base"
    struct function_table *funcs; // All functions (with origin metadata)
    struct inherit_table *inherits; // Parent protos
    struct variable_table *vars;  // Variable layout
    void *bytecode;              // Compiled code
};

// Function table entry
struct function_entry {
    char *name;                  // "create"
    int index;                   // Position in table
    struct proto *origin;        // Which proto defined this
    int visibility;              // public/private/protected
    void *code_ptr;              // Direct pointer to bytecode
};

// Inherit table entry
struct inherit_entry {
    char *name;                  // "base" (basename)
    struct proto *proto;         // Pointer to parent proto
    int func_offset;             // Where parent's functions start
    int var_offset;              // Where parent's vars start
};
```

### Compilation Flow

```
1. Parse "inherit /std/base"
2. proto = get_cached_proto("/std/base")
   - If not cached: compile it, cache it
   - If cached: reuse it
3. Add proto to current file's inherit table
4. Copy parent's variable layout to child (merge)
5. Continue parsing child

When parsing "::create()":
1. Look at first entry in inherit table
2. Find "create" in that proto's function table
3. Get function index
4. Emit: CALL_PARENT_FUNC <index>

When parsing "base::create()":
1. Find "base" in inherit table
2. Find "create" in that proto's function table
3. Get function index
4. Emit: CALL_NAMED_PARENT <index>

Runtime:
case CALL_PARENT_FUNC:
    index = bytecode[pc++];
    func = current_proto->inherits[0]->funcs[index];
    call_function(func);
```

---

## Conclusion

Our current implementation has fundamental flaws because we misunderstood LPC's inheritance model. We implemented runtime lookup when we should have implemented compile-time resolution. We compile files multiple times when we should cache them once.

The fix is clear:
1. **Global proto cache** - compile once, reuse everywhere
2. **Compile-time function resolution** - store function pointers/indices in bytecode
3. **Simplified runtime** - just execute the pointers
4. **Proper state management** - isolate compilation contexts
5. **Metadata tracking** - keep origin info for qualified calls

This brings us in line with how LDMud, FluffOS, and MudOS implement LPC inheritance. It's the correct model, and it will fix all our current bugs.

---

## Complete Implementation Plan

### Task 1: Global Proto Cache (CRITICAL - DO FIRST)
**Estimated Time:** 2-3 hours

**Files to modify:**
- `/src/cache2.c` - Add proto cache implementation
- `/src/cache2.h` or `/src/protos.h` - Add function prototypes
- `/src/compile1.c` - Modify `add_inherit` to use cache

**Implementation:**

```c
// In cache2.c or new proto_cache.c

// Global proto cache
static struct proto_cache_entry {
    char *pathname;
    struct proto *proto;
    struct proto_cache_entry *next;
} *proto_cache_head = NULL;

struct proto *find_cached_proto(char *pathname) {
    struct proto_cache_entry *curr = proto_cache_head;
    while (curr) {
        if (strcmp(curr->pathname, pathname) == 0) {
            return curr->proto;
        }
        curr = curr->next;
    }
    return NULL;
}

void cache_proto(char *pathname, struct proto *proto) {
    struct proto_cache_entry *entry = MALLOC(sizeof(struct proto_cache_entry));
    entry->pathname = copy_string(pathname);
    entry->proto = proto;
    entry->next = proto_cache_head;
    proto_cache_head = entry;
}

void clear_proto_cache() {
    // For development/testing - clear cache
    struct proto_cache_entry *curr = proto_cache_head;
    while (curr) {
        struct proto_cache_entry *next = curr->next;
        FREE(curr->pathname);
        // Don't free proto - it might still be in use
        FREE(curr);
        curr = next;
    }
    proto_cache_head = NULL;
}
```

**Modify add_inherit in compile1.c:**

```c
int add_inherit(struct file_info *file_info, char *pathname) {
    struct proto *parent_proto;
    struct code *parent_code;
    struct inherit_list *new_inherit;
    int result;
    char logbuf[256];
    
    // Check for duplicate inherit in current file
    struct inherit_list *curr = file_info->curr_code->inherits;
    while (curr) {
        if (!strcmp(curr->inherit_path, pathname)) {
            sprintf(logbuf, "add_inherit: '%s' already inherited", pathname);
            logger(LOG_WARNING, logbuf);
            return 0;
        }
        curr = curr->next;
    }
    
    // Check global proto cache first
    parent_proto = find_cached_proto(pathname);
    
    if (parent_proto) {
        sprintf(logbuf, "add_inherit: using cached proto for '%s'", pathname);
        logger(LOG_DEBUG, logbuf);
    } else {
        // Compile once and cache
        sprintf(logbuf, "add_inherit: compiling and caching '%s'", pathname);
        logger(LOG_DEBUG, logbuf);
        
        result = parse_code(pathname, NULL, &parent_code);
        if (result) {
            sprintf(logbuf, "add_inherit: failed to compile '%s'", pathname);
            logger(LOG_ERROR, logbuf);
            return 1;
        }
        
        // Create proto
        parent_proto = MALLOC(sizeof(struct proto));
        parent_proto->pathname = copy_string(pathname);
        parent_proto->funcs = parent_code;
        parent_proto->proto_obj = NULL;
        parent_proto->next_proto = NULL;
        parent_proto->inherits = parent_code->inherits;
        
        // Cache it globally
        cache_proto(pathname, parent_proto);
    }
    
    // Add reference to current file's inherit list
    new_inherit = MALLOC(sizeof(struct inherit_list));
    new_inherit->parent_proto = parent_proto;
    new_inherit->inherit_path = copy_string(pathname);
    new_inherit->next = file_info->curr_code->inherits;
    file_info->curr_code->inherits = new_inherit;
    
    // Copy parent's global variables to child
    copy_parent_globals(file_info, parent_proto);
    
    return 0;
}
```

**Testing:**
- Compile player.c and verify each parent is compiled only once
- Check logs for "using cached proto" messages
- Verify no "expected ;" errors

---

### Task 2: Add Inherit Entry Structure with Alias Support (CRITICAL)
**Estimated Time:** 2-3 hours

**Files to modify:**
- `/src/object.h` - Add `struct inherit_entry` and modify `struct inherit_list`
- `/src/compile1.c` - Add alias parsing to `inherit` statement

**Implementation:**

```c
// In object.h, add inherit entry structure
struct inherit_entry {
    char *alias;              // Explicit alias (e.g., "Container") or NULL
    char *canon_path;         // Canonical absolute path (normalized)
    struct proto *proto;      // Compiled/cached program
    uint16_t func_offset;     // Optional: offset in flat function table
    uint16_t var_offset;      // Optional: offset in flat variable table
    struct inherit_entry *next;
};

// Modify struct inherit_list to use inherit_entry
struct inherit_list {
    struct inherit_entry *entry;  // Use entry instead of direct proto
    struct inherit_list *next;
};

// Add to struct fns
struct fns {
    char *funcname;
    unsigned char is_static;
    unsigned char num_args;
    unsigned char num_locals;
    unsigned long num_instr;
    struct code_instr *code;
    struct local_sym_table *lst;
    struct fns *next;
    uint16_t func_index;      // ADD THIS - position in function table
    uint8_t visibility;       // ADD THIS - public/protected/private
    struct proto *origin_proto;  // ADD THIS - which proto defined this
};
```

**Modify parser to support aliases:**

```c
// In compile1.c, modify inherit parsing
case INHERIT_TOK:
    get_token(file_info, &token);
    
    // Handle macro expansion
    if (token.type == NAME_TOK) {
        // ... macro expansion code ...
    }
    
    if (token.type != STRING_TOK) {
        set_c_err_msg("expected string after inherit");
        return file_info->phys_line;
    }
    
    char *inherit_path = copy_string(token.token_data.name);
    char *alias = NULL;
    
    // Check for "as Alias" syntax
    get_token(file_info, &token);
    if (token.type == NAME_TOK && strcmp(token.token_data.name, "as") == 0) {
        get_token(file_info, &token);
        if (token.type != NAME_TOK) {
            set_c_err_msg("expected alias name after 'as'");
            return file_info->phys_line;
        }
        alias = copy_string(token.token_data.name);
        get_token(file_info, &token);
    }
    
    // If no alias provided, default to basename
    if (!alias) {
        char *last_slash = strrchr(inherit_path, '/');
        char *basename = last_slash ? last_slash + 1 : inherit_path;
        // Remove .c extension if present
        char *dot = strrchr(basename, '.');
        if (dot) {
            alias = MALLOC(dot - basename + 1);
            strncpy(alias, basename, dot - basename);
            alias[dot - basename] = '\0';
        } else {
            alias = copy_string(basename);
        }
    }
    
    // Add to inheritance list with alias
    if (add_inherit(file_info, inherit_path, alias)) {
        set_c_err_msg("failed to load inherited file");
        return file_info->phys_line;
    }
    
    if (token.type != SEMI_TOK) {
        set_c_err_msg("expected ; after inherit");
        return file_info->phys_line;
    }
    break;
```

**Add path canonicalization function:**

```c
// In compile1.c or new file_utils.c
char *canonicalize_path(char *path) {
    // Normalize path: resolve .., ., collapse multiple slashes
    // Return newly allocated canonical path
    char *result = MALLOC(strlen(path) + 1);
    char *src = path;
    char *dst = result;
    
    // Simple implementation - expand as needed
    while (*src) {
        if (*src == '/' && *(src+1) == '/') {
            src++;  // Skip duplicate slash
        } else if (*src == '/' && *(src+1) == '.' && *(src+2) == '/') {
            src += 2;  // Skip ./
        } else if (*src == '/' && *(src+1) == '.' && *(src+2) == '.' && *(src+3) == '/') {
            // Handle ../  - backtrack in dst
            if (dst > result) {
                dst--;
                while (dst > result && *dst != '/') dst--;
            }
            src += 3;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
    return result;
}
```

**Testing:**
- Test `inherit "/std/container.c" as Container;` syntax
- Verify alias is stored correctly
- Test default basename aliasing
- Verify path canonicalization works

---

### Task 3: Compile-Time Resolution of ::function() with MRO (CRITICAL)
**Estimated Time:** 4-5 hours

**Goal:** Resolve `::func()` to the **next definition up the linearized inheritance chain** (MRO), not "first parent". Store as `(inherit_idx, func_idx)` pair for O(1) runtime dispatch.

**Files to modify:**
- `/src/compile1.c` - Parser for `::function()` with MRO resolution
- `/src/object.h` - Modify `code_instr` to store index pairs
- `/src/interp.c` - Simple indexed dispatch for `CALL_SUPER`

**Implementation:**

**Modify code_instr structure to store indices:**

```c
// In object.h
struct code_instr {
    unsigned char type;
    union {
        long num;
        char *string;
        struct {
            uint16_t inherit_idx;  // Index into program's inherit table
            uint16_t func_idx;     // Index into that program's function table
        } parent_call;
    } value;
};
```

**Add MRO computation function:**

```c
// In compile1.c - compute linearized inheritance chain
// This is a simplified C3 linearization or declaration-order traversal
struct proto **compute_mro(struct code *current_code, int *mro_length) {
    // Allocate array for MRO (child first, then ancestors)
    struct proto **mro = MALLOC(sizeof(struct proto*) * 32);  // Max depth
    int count = 0;
    
    // Add current program first
    mro[count++] = current_code->proto;  // Assuming code has back-pointer to proto
    
    // Traverse inherits in declaration order (depth-first)
    struct inherit_list *curr = current_code->inherits;
    while (curr) {
        // Check if already in MRO (for diamond inheritance)
        int already_added = 0;
        for (int i = 0; i < count; i++) {
            if (mro[i] == curr->entry->proto) {
                already_added = 1;
                break;
            }
        }
        
        if (!already_added) {
            mro[count++] = curr->entry->proto;
            
            // Recursively add that proto's parents
            struct inherit_list *parent_inherits = curr->entry->proto->funcs->inherits;
            while (parent_inherits) {
                // Check for duplicates
                int dup = 0;
                for (int i = 0; i < count; i++) {
                    if (mro[i] == parent_inherits->entry->proto) {
                        dup = 1;
                        break;
                    }
                }
                if (!dup) {
                    mro[count++] = parent_inherits->entry->proto;
                }
                parent_inherits = parent_inherits->next;
            }
        }
        curr = curr->next;
    }
    
    *mro_length = count;
    return mro;
}
```

**Resolve ::function() to next-up in MRO:**

```c
// In compile1.c, when parsing ::function_name()
if (token.type == SECOND_TOK) {
    get_token(file_info, &token);
    if (token.type != NAME_TOK) {
        set_c_err_msg("expected function name after ::");
        return file_info->phys_line;
    }
    
    char *func_name = copy_string(token.token_data.name);
    
    // Compute MRO for current program
    int mro_length;
    struct proto **mro = compute_mro(file_info->curr_code, &mro_length);
    
    // Find current function's defining proto in MRO
    int current_idx = 0;  // Start at 0 (current program)
    
    // Search for next definition up the chain
    struct fns *next_func = NULL;
    struct proto *next_proto = NULL;
    int next_inherit_idx = -1;
    
    for (int i = current_idx + 1; i < mro_length; i++) {
        struct proto *candidate = mro[i];
        struct fns *candidate_func = find_fns_in_proto(func_name, candidate);
        
        if (candidate_func) {
            // Check visibility - must be accessible
            if (candidate_func->visibility == VISIBILITY_PRIVATE && 
                candidate->pathname != file_info->curr_code->proto->pathname) {
                continue;  // Private to different program, skip
            }
            
            next_func = candidate_func;
            next_proto = candidate;
            
            // Find inherit_idx for this proto
            struct inherit_list *inh = file_info->curr_code->inherits;
            int idx = 0;
            while (inh) {
                if (inh->entry->proto == next_proto) {
                    next_inherit_idx = idx;
                    break;
                }
                idx++;
                inh = inh->next;
            }
            break;
        }
    }
    
    FREE(mro);
    
    if (!next_func) {
        char errbuf[256];
        sprintf(errbuf, "no super definition for '%s' in inheritance chain", func_name);
        set_c_err_msg(errbuf);
        return file_info->phys_line;
    }
    
    // Emit CALL_SUPER with (inherit_idx, func_idx)
    {
        unsigned long x = curr_fn->num_code;
        make_new(curr_fn);
        curr_fn->code[x].type = CALL_SUPER;
        curr_fn->code[x].value.parent_call.inherit_idx = next_inherit_idx;
        curr_fn->code[x].value.parent_call.func_idx = next_func->func_index;
    }
    
    // Continue parsing (expect parentheses, args, etc.)
    // ...
}
```

**Runtime handler with indexed dispatch:**

```c
// In interp.c
case CALL_SUPER: {
    uint16_t i_idx = func->code[loop].value.parent_call.inherit_idx;
    uint16_t f_idx = func->code[loop].value.parent_call.func_idx;
    
    // Get the inherit entry
    struct inherit_list *inh = obj->parent->funcs->inherits;
    for (int i = 0; i < i_idx && inh; i++) {
        inh = inh->next;
    }
    
    if (!inh) {
        interp_error_with_trace("invalid inherit index", player, obj, func, line);
        break;
    }
    
    // Get the target program and function
    struct proto *target_proto = inh->entry->proto;
    struct fns *target_func = target_proto->funcs->func_list;
    
    // Find function by index
    for (int i = 0; i < f_idx && target_func; i++) {
        target_func = target_func->next;
    }
    
    if (!target_func) {
        interp_error_with_trace("invalid function index", player, obj, func, line);
        break;
    }
    
    // Call the function (this is simplified - needs proper call stack)
    // The function runs in the context of the current object
    struct fns *saved_func = func;
    func = target_func;
    
    // Execute (actual implementation needs proper frame management)
    result = execute_function(target_func, obj, &tmpobj);
    
    func = saved_func;
    loop++;
    break;
}
```

**Testing:**
- Test single inheritance: `::init()` resolves to immediate parent
- Test deep inheritance: verify next-up in MRO, not first parent
- Test visibility: private functions not accessible via `::`
- Test error case: `::func()` with no parent definition

---

### Task 4: Compile-Time Resolution of Alias::function() (CRITICAL)
**Estimated Time:** 3-4 hours

**Goal:** Resolve `Alias::func()` by looking up the **explicit alias** in the inherit table (not basename matching). Store as `(inherit_idx, func_idx)` pair.

**Files to modify:**
- `/src/compile1.c` - Parser for `Alias::function()` with alias-based resolution
- `/src/interp.c` - Indexed dispatch for `CALL_PARENT_NAMED`

**Implementation:**

**Resolve Alias::function() by explicit alias:**

```c
// In compile1.c, when parsing Alias::function_name()
// This happens when we see NAME_TOK followed by SECOND_TOK

if (peek_next_token(file_info) == SECOND_TOK) {
    char *alias_name = copy_string(token.token_data.name);
    
    get_token(file_info, &token);  // Consume ::
    get_token(file_info, &token);  // Get function name
    
    if (token.type != NAME_TOK) {
        set_c_err_msg("expected function name after ::");
        return file_info->phys_line;
    }
    
    char *func_name = copy_string(token.token_data.name);
    
    // Find parent in inherit list by ALIAS (not basename!)
    struct inherit_list *curr_inherit = file_info->curr_code->inherits;
    struct proto *target_parent = NULL;
    int target_inherit_idx = -1;
    int idx = 0;
    
    while (curr_inherit) {
        if (curr_inherit->entry->alias && 
            strcmp(curr_inherit->entry->alias, alias_name) == 0) {
            target_parent = curr_inherit->entry->proto;
            target_inherit_idx = idx;
            break;
        }
        idx++;
        curr_inherit = curr_inherit->next;
    }
    
    if (!target_parent) {
        char errbuf[256];
        sprintf(errbuf, "parent alias '%s' not found in inherit list", alias_name);
        set_c_err_msg(errbuf);
        return file_info->phys_line;
    }
    
    // Find function in that parent's proto
    struct fns *parent_func = find_fns_in_proto(func_name, target_parent);
    if (!parent_func) {
        char errbuf[256];
        sprintf(errbuf, "function '%s' not found in parent '%s'", func_name, alias_name);
        set_c_err_msg(errbuf);
        return file_info->phys_line;
    }
    
    // Check visibility
    if (parent_func->visibility == VISIBILITY_PRIVATE) {
        // Private functions can only be called from within their own program
        // From a child, this is an error
        char errbuf[256];
        sprintf(errbuf, "function '%s' is private in parent '%s'", func_name, alias_name);
        set_c_err_msg(errbuf);
        return file_info->phys_line;
    }
    
    // Emit CALL_PARENT_NAMED with (inherit_idx, func_idx)
    {
        unsigned long x = curr_fn->num_code;
        make_new(curr_fn);
        curr_fn->code[x].type = CALL_PARENT_NAMED;
        curr_fn->code[x].value.parent_call.inherit_idx = target_inherit_idx;
        curr_fn->code[x].value.parent_call.func_idx = parent_func->func_index;
    }
    
    // Continue parsing (expect parentheses, args, etc.)
    // ...
}
```

**Check for ambiguous aliases at inherit time:**

```c
// In add_inherit(), after adding new inherit entry
// Check for duplicate aliases
struct inherit_list *check = file_info->curr_code->inherits;
int alias_count = 0;
while (check) {
    if (check->entry->alias && strcmp(check->entry->alias, alias) == 0) {
        alias_count++;
    }
    check = check->next;
}

if (alias_count > 1) {
    char errbuf[256];
    sprintf(errbuf, "ambiguous inherit alias '%s' - use explicit 'as' to disambiguate", alias);
    set_c_err_msg(errbuf);
    return 1;
}
```

**Runtime handler with indexed dispatch:**

```c
// In interp.c
case CALL_PARENT_NAMED: {
    uint16_t i_idx = func->code[loop].value.parent_call.inherit_idx;
    uint16_t f_idx = func->code[loop].value.parent_call.func_idx;
    
    // Get the inherit entry
    struct inherit_list *inh = obj->parent->funcs->inherits;
    for (int i = 0; i < i_idx && inh; i++) {
        inh = inh->next;
    }
    
    if (!inh) {
        interp_error_with_trace("invalid inherit index for named parent call", 
                               player, obj, func, line);
        break;
    }
    
    // Get the target program and function
    struct proto *target_proto = inh->entry->proto;
    struct fns *target_func = target_proto->funcs->func_list;
    
    // Find function by index
    for (int i = 0; i < f_idx && target_func; i++) {
        target_func = target_func->next;
    }
    
    if (!target_func) {
        interp_error_with_trace("invalid function index for named parent call", 
                               player, obj, func, line);
        break;
    }
    
    // Call the function in current object's context
    struct fns *saved_func = func;
    func = target_func;
    
    result = execute_function(target_func, obj, &tmpobj);
    
    func = saved_func;
    loop++;
    break;
}
```

**Testing:**
- Test `Container::init()` with explicit alias
- Test default basename aliasing
- Test ambiguous alias detection (compile-time error)
- Test visibility enforcement (private function error)
- Test alias with same basename but different paths (Test 4 from test suite)

---

### Task 4.5: Proper LPC Variable Layout with Linearization (CRITICAL - BUG FIX)
**Estimated Time:** 4-6 hours
**Status:** ⚠️ **REQUIRED** - Current implementation has diamond inheritance variable duplication bug

**Problem Discovered (Nov 8, 2025):**
When `player.c` inherits both `living.c` and `container.c`, and both inherit from `object.c`, we were copying `object.c`'s variables (`properties`, `id_list`) **twice** into `player.c`:
1. Once when inheriting `living.c` (which contains object's vars)
2. Again when inheriting `container.c` (which also contains object's vars)

This causes:
- Duplicate variable slots in the child's variable table
- Corrupted symbol table state
- Blank compile errors on subsequent inherit statements
- Runtime crashes due to misaligned variable access

**Root Cause:**
We do **per-inherit blind copying** of parent variables without tracking which ancestor programs have already been merged into the child's layout.

**LPC Correct Behavior:**
- Each ancestor program appears **exactly once** in the child's variable layout
- All inheritance paths to the same program share the **same `var_offset`**
- Diamond inheritance (A→B, A→C, B&C→D) means A's variables appear once in D
- If two **different** parents define the same variable name, it's a **compile-time conflict error**

**Goal:** Implement proper LPC-style variable layout construction:
1. Build a **unique linearization** of ancestor programs (base → … → child)
2. Merge each program's **own defined variables** exactly once
3. Track `var_offset` per program (where its variables live in child's storage)
4. All inherit entries pointing to the same program share the same `var_offset`
5. Detect and error on variable name conflicts from different parents

**Files to modify:**
- `/src/compile1.c` - `add_inherit()` and new layout construction function
- `/src/object.h` - Add `var_offset` field to `struct inherit_entry`

**Implementation Steps:**

**Step 1: Add var_offset tracking to inherit_entry**
```c
// In object.h
struct inherit_entry {
    char *alias;
    char *canon_path;
    struct proto *proto;
    uint16_t func_offset;   // Existing
    uint16_t var_offset;    // NEW: Where this parent's vars start in child
};
```

**Step 2: Compute unique linearization**
```c
// In compile1.c - Build base→child ordered list of unique programs
struct proto **compute_unique_linearization(struct code *current_code, int *count) {
    struct proto **order = MALLOC(sizeof(struct proto*) * 32);
    int num = 0;
    
    // DFS through inherits, adding each proto only once
    void add_unique(struct proto *p) {
        if (!p) return;
        
        // Check if already added
        for (int i = 0; i < num; i++) {
            if (order[i] == p) return;  // Already in list
        }
        
        // Add parent's ancestors first (depth-first)
        if (p->funcs && p->funcs->inherits) {
            struct inherit_list *inh = p->funcs->inherits;
            while (inh) {
                add_unique(inh->parent_proto);
                inh = inh->next;
            }
        }
        
        // Then add this program
        order[num++] = p;
    }
    
    // Process all direct inherits
    struct inherit_list *curr = current_code->inherits;
    while (curr) {
        add_unique(curr->parent_proto);
        curr = curr->next;
    }
    
    *count = num;
    return order;
}
```

**Step 3: Build variable layout from linearization**
```c
// In compile1.c - Called after all inherits are parsed
int build_variable_layout(struct file_info *file_info) {
    int order_count;
    struct proto **order = compute_unique_linearization(file_info->curr_code, &order_count);
    
    // Track which programs we've merged and their var_offset
    struct {
        struct proto *prog;
        uint16_t var_offset;
    } merged[32];
    int merged_count = 0;
    
    uint16_t next_slot = 0;
    
    // Process each unique program in order
    for (int i = 0; i < order_count; i++) {
        struct proto *prog = order[i];
        
        // Check if already merged
        int already_merged = 0;
        uint16_t existing_offset = 0;
        for (int j = 0; j < merged_count; j++) {
            if (merged[j].prog == prog) {
                already_merged = 1;
                existing_offset = merged[j].var_offset;
                break;
            }
        }
        
        if (!already_merged) {
            // Record this program's var_offset
            merged[merged_count].prog = prog;
            merged[merged_count].var_offset = next_slot;
            merged_count++;
            
            // Copy only THIS program's own defined variables
            // (not its ancestors - they're handled separately)
            struct var_tab *var = prog->funcs ? prog->funcs->gst : NULL;
            while (var) {
                // Check for name conflict with different origin
                struct var_tab *existing = file_info->glob_sym ? 
                                          file_info->glob_sym->varlist : NULL;
                while (existing) {
                    if (!strcmp(existing->name, var->name)) {
                        // Same name - check if from same program
                        // (Need to track origin_prog in var_tab for this)
                        // For now, just skip if name exists
                        goto skip_var;
                    }
                    existing = existing->next;
                }
                
                // Add variable to child's table
                struct var_tab *new_var = MALLOC(sizeof(struct var_tab));
                new_var->name = copy_string(var->name);
                new_var->base = next_slot++;
                new_var->array = copy_array_size(var->array);
                new_var->is_mapping = var->is_mapping;
                new_var->next = file_info->glob_sym ? 
                               file_info->glob_sym->varlist : NULL;
                
                if (file_info->glob_sym) {
                    file_info->glob_sym->varlist = new_var;
                    file_info->glob_sym->num++;
                } else {
                    file_info->glob_sym = MALLOC(sizeof(sym_tab_t));
                    file_info->glob_sym->num = 1;
                    file_info->glob_sym->varlist = new_var;
                }
                file_info->curr_code->num_globals++;
                
            skip_var:
                var = var->next;
            }
        }
    }
    
    // Attach var_offset to all inherit entries
    struct inherit_list *inh = file_info->curr_code->inherits;
    while (inh) {
        for (int i = 0; i < merged_count; i++) {
            if (merged[i].prog == inh->parent_proto) {
                inh->entry->var_offset = merged[i].var_offset;
                break;
            }
        }
        inh = inh->next;
    }
    
    FREE(order);
    return 0;
}
```

**Step 4: Modify add_inherit to defer variable copying**
```c
// In compile1.c - Remove immediate variable copying from add_inherit()
int add_inherit(struct file_info *file_info, char *pathname) {
    // ... existing proto loading/caching code ...
    
    // Create inherit entry
    struct inherit_entry *entry = MALLOC(sizeof(struct inherit_entry));
    entry->alias = /* ... */;
    entry->canon_path = copy_string(pathname);
    entry->proto = parent_proto;
    entry->func_offset = 0;
    entry->var_offset = 0;  // Will be set by build_variable_layout()
    
    // Add to inherit list
    struct inherit_list *new_inherit = MALLOC(sizeof(struct inherit_list));
    new_inherit->entry = entry;
    new_inherit->parent_proto = parent_proto;
    new_inherit->inherit_path = copy_string(pathname);
    new_inherit->next = file_info->curr_code->inherits;
    file_info->curr_code->inherits = new_inherit;
    
    // DO NOT copy variables here - defer to build_variable_layout()
    
    return 0;
}
```

**Step 5: Call build_variable_layout after all inherits parsed**
```c
// In compile2.c - top_level_parse() or parse_code()
// After all inherit statements are processed:
if (build_variable_layout(&file_info)) {
    // Error during layout construction
    return file_info->phys_line;
}
```

**Testing:**
- Test diamond inheritance: `player.c` inherits `living.c` and `container.c`, both inherit `object.c`
- Verify `object.c` variables appear exactly once in `player.c`
- Verify both `living::` and `container::` paths access the same `properties` variable
- Test variable name conflict detection (two different parents define same var)
- Test deep diamond: A→B→D, A→C→D, B&C→E
- Verify `var_offset` is same for all inherit entries pointing to same program

**Success Criteria:**
- ✅ No duplicate variables in child's variable table
- ✅ `player.c` compiles without blank errors
- ✅ All inheritance paths to same program share same `var_offset`
- ✅ Runtime variable access works correctly through all paths
- ✅ Diamond inheritance test cases pass

---

### Task 5: Remove Runtime Lookup Functions (CLEANUP)
**Estimated Time:** 1 hour

**Files to modify:**
- `/src/interp.c` - Remove `find_parent_function` and `find_named_parent_function`

**Implementation:**

```c
// DELETE these entire functions:
// - find_parent_function()
// - find_named_parent_function()
// - Any helper functions only used by them

// Remove all debug logging related to these functions
```

**Testing:**
- Verify code compiles without these functions
- Run full test suite

---

### Task 6: Fix Tokenizer State Isolation (IMPORTANT)
**Estimated Time:** 1-2 hours

**Files to modify:**
- `/src/compile1.c` - Save/restore tokenizer state in `add_inherit`

**Implementation:**

```c
int add_inherit(struct file_info *file_info, char *pathname) {
    // ... existing code ...
    
    // If we need to compile (not cached)
    if (!parent_proto) {
        // Save tokenizer state before recursive compilation
        struct token saved_token = file_info->current_token;
        // Save any other relevant state
        
        result = parse_code(pathname, NULL, &parent_code);
        
        // Restore tokenizer state
        file_info->current_token = saved_token;
        // Restore other state
        
        if (result) {
            // ... error handling
        }
        
        // ... create and cache proto
    }
    
    // ... rest of function
}
```

**Note:** With proto caching, this might not be necessary since most inherits won't trigger compilation. Test first with caching alone.

**Testing:**
- Compile player.c with multiple inherits
- Verify no "expected ;" errors
- Check that tokenizer position is correct after each inherit

---

### Task 7: Remove Debug Logging (CLEANUP)
**Estimated Time:** 30 minutes

**Files to modify:**
- `/src/interp.c` - Remove all debug logging from parent call functions
- `/src/compile1.c` - Remove excessive logging

**Implementation:**

```c
// Remove or comment out all logger() calls that were added for debugging:
// - "find_named_parent: looking for..."
// - "find_named_parent: current_func found in..."
// - "find_named_parent: found parent..."
// - etc.

// Keep only essential error logging
```

---

### Task 8: Comprehensive Testing (CRITICAL)
**Estimated Time:** 3-4 hours

**Test Cases:**

**Test 1: Single Inheritance + Super Call**
```c
// object.c
void create() { write("object\n"); }

// container.c
inherit "/object.c";
void create() { ::create(); write("container\n"); }

// room.c
inherit "/container.c";
void create() { ::create(); write("room\n"); }
```
**Expected output:**
```
object
container
room
```
**Why:** room::create() calls next-up = container::create(), which calls next-up = object::create().

---

**Test 2: Multiple Inheritance with Named Parents (Un-guarded)**
```c
// object.c
void create() { write("object\n"); }

// living.c
inherit "/object.c";
void create() { ::create(); write("living\n"); }

// container.c
inherit "/object.c";
void create() { ::create(); write("container\n"); }

// player.c
inherit "/living.c" as Living;
inherit "/container.c" as Container;

void create() {
    Living::create();
    Container::create();
    write("player\n");
}
```
**Expected output (un-guarded base):**
```
object
living
object
container
player
```
**Note:** object::create() runs **twice** (no virtual inheritance). This is expected LPC behavior!

---

**Test 2b: Multiple Inheritance with Guarded Base (Best Practice)**
```c
// object.c
static int __inited;
void create() {
    if (__inited) return;
    __inited = 1;
    write("object\n");
}

// living.c and container.c same as above
// player.c same as above
```
**Expected output (guarded):**
```
object
living
container
player
```
**Note:** This is the recommended mudlib pattern for diamond inheritance.

---

**Test 3: ::func() Resolves to Next-Up in MRO, Not First Parent**
```c
// base.c
void init() { write("base\n"); }

// mid1.c
inherit "/base.c";
void init() { ::init(); write("mid1\n"); }

// mid2.c
inherit "/base.c";
void init() { ::init(); write("mid2\n"); }

// top.c
inherit "/mid1.c";
inherit "/mid2.c";
void init() { ::init(); write("top\n"); }
```
**Expected output:**
```
base
mid2
top
```
**Why:** top::init() calls ::init() → next definition in MRO is mid2::init() (closest in linearization), not mid1. Then mid2::init() calls ::init() → base::init().

**Key point:** Validates that :: uses next-up in MRO, not "first parent".

---

**Test 4: Alias Resolution (No Basename Reliance)**
```c
// /std/container_v2.c
void create() { write("container_v2\n"); }

// /std/container.c
void create() { write("container\n"); }

// test.c
inherit "/std/container_v2.c" as Bin;
inherit "/std/container.c" as Box;

void create() {
    Bin::create();
    Box::create();
    write("done\n");
}
```
**Expected output:**
```
container_v2
container
done
```
**Why:** Both parents called distinctly with no ambiguity, proving alias-based resolution works even if basenames collide.

---

**Test 5: Visibility Enforcement at Compile Time**
```c
// secret.c
private void hidden() { write("hidden\n"); }
public void hello() { write("hello\n"); }

// child.c
inherit "/secret.c" as Secret;
void test() {
    Secret::hello();   // OK
    Secret::hidden();  // MUST BE COMPILE-TIME ERROR
}
```
**Expected:** Compilation **fails** on `Secret::hidden()` with clear visibility error message.

---

**Test 6: No ::func() Available**
```c
// only.c
void go() { write("only\n"); }

// child.c
inherit "/only.c";
void go() {
    ::go();  // COMPILE-TIME ERROR: no next-up definition
}
```
**Expected:** Compilation **fails** with error: "no super definition for 'go' in inheritance chain"

---

**Test 7: Program Cache Canonicalization**
```c
// Test that these all resolve to the same cached proto:
// file1.c
inherit "/std/./object.c";

// file2.c
inherit "/std/dir/../object.c";

// file3.c
inherit "/std/object.c";
```
**Expected:** Only **one** program instance in cache (verify via log counters or introspection). Proves canonicalization works.

---

**Test 8: Ambiguous Alias Detection**
```c
// test.c
inherit "/std/container.c";      // Default alias: "container"
inherit "/lib/container.c";      // Default alias: "container" - CONFLICT!

void create() {
    container::create();  // Which one?
}
```
**Expected:** Compilation **fails** with error: "ambiguous inherit alias 'container' - use explicit 'as' to disambiguate"

---

**Test 9: Proto Cache Efficiency**
```c
// Compile 10 files that all inherit from /std/object.c
// Check that object.c is compiled only once
```
**Expected:** 
- Log shows "compiling and caching '/std/object.c'" once
- Log shows "using cached proto for '/std/object.c'" 9 times
- Compilation is fast (no redundant parsing)

---

**Success Criteria:**
- ✅ All test cases pass with expected output
- ✅ Compile-time errors caught correctly (Tests 5, 6, 8)
- ✅ No runtime errors
- ✅ No infinite recursion
- ✅ Proto cache shows single compilation per file (Test 7, 9)
- ✅ MRO resolution correct (Test 3)
- ✅ Alias resolution correct (Test 4)
- ✅ Diamond inheritance behavior documented (Test 2, 2b)
- ✅ Fast compilation times
- ✅ Low memory usage

---

## Implementation Checklist

- [ ] **Task 1:** Global Proto Cache (2-3 hours)
  - Implement `find_cached_proto()` and `cache_proto()`
  - Modify `add_inherit()` to check cache first
  - Add path canonicalization
  
- [ ] **Task 2:** Inherit Entry Structure with Alias Support (2-3 hours)
  - Add `struct inherit_entry` with alias field
  - Modify parser to support `inherit "path" as Alias;` syntax
  - Implement default basename aliasing
  - Add ambiguous alias detection
  
- [ ] **Task 3:** Compile-Time Resolution of ::function() with MRO (4-5 hours)
  - Implement MRO computation (linearized inheritance chain)
  - Resolve `::func()` to next-up in MRO
  - Store `(inherit_idx, func_idx)` in bytecode
  - Implement `CALL_SUPER` opcode handler
  - Add visibility enforcement
  
- [ ] **Task 4:** Compile-Time Resolution of Alias::function() (3-4 hours)
  - Resolve `Alias::func()` by explicit alias lookup
  - Store `(inherit_idx, func_idx)` in bytecode
  - Implement `CALL_PARENT_NAMED` opcode handler
  - Add visibility enforcement
  - Add compile-time error for ambiguous aliases
  
- [ ] **Task 5:** Remove Runtime Lookup Functions (1 hour)
  - Delete `find_parent_function()`
  - Delete `find_named_parent_function()`
  - Remove all related helper functions
  
- [ ] **Task 6:** Fix Tokenizer State Isolation (1-2 hours, if needed)
  - Test if proto cache eliminates the issue
  - If needed, add save/restore of tokenizer state
  
- [ ] **Task 7:** Remove Debug Logging (30 minutes)
  - Clean up all debug logger() calls
  - Keep only essential error logging
  
- [ ] **Task 8:** Comprehensive Testing (3-4 hours)
  - Implement all 9 test cases
  - Verify compile-time error detection
  - Verify runtime behavior
  - Verify proto cache efficiency
  - Document diamond inheritance pattern

**Total Estimated Time:** 16-22 hours

**Critical Path:** Tasks 1-4 must be done in order. Tasks 5-7 can be done anytime after Task 4.

---

## Ready to Change the World! 🚀

This implementation plan is:
- ✅ Based on proven LPC driver architecture (LDMud, FluffOS, MudOS)
- ✅ Incremental and testable at each step
- ✅ Fixes all current bugs
- ✅ Dramatically improves performance
- ✅ Reduces memory usage
- ✅ Simplifies the codebase
- ✅ Matches standard LPC semantics

Let's do this!
