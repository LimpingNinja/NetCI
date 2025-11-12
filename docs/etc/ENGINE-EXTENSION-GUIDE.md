# NetCI Engine Extension Guide

**For C Developers Extending the NetCI Engine**

This guide provides comprehensive documentation for safely adding functionality to the NetCI engine. It emphasizes safety patterns learned from real bugs and provides working examples from the codebase.

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Adding System Calls](#2-adding-system-calls)
3. [Implementing Callbacks](#3-implementing-callbacks)
4. [Memory Management](#4-memory-management)
5. [Security Considerations](#5-security-considerations)
6. [Working Examples](#6-working-examples)
7. [Testing and Debugging](#7-testing-and-debugging)
8. [Best Practices](#8-best-practices)
9. [Reference](#9-reference)
10. [Case Studies](#10-case-studies)

**Appendices:**
- [Appendix A: System Call Checklist](#appendix-a-system-call-checklist)
- [Appendix B: Callback Checklist](#appendix-b-callback-checklist)
- [Appendix C: Safety Checklist](#appendix-c-safety-checklist)
- [Appendix D: Common Mistakes](#appendix-d-common-mistakes)
- [Appendix E: Additional Resources](#appendix-e-additional-resources)

---

## 1. Introduction

### 1.1 About This Guide

This guide is for C developers who need to extend the NetCI engine by adding new functionality. You'll learn:

- How to add system calls that NLPC code can invoke
- How to implement callbacks from C to NLPC safely
- Critical memory management patterns
- Security considerations
- Real-world examples and bug case studies

**Prerequisites:**
- Strong C programming knowledge
- Understanding of NetCI architecture basics
- Familiarity with stack-based execution models

**Why Safety Matters:**

This guide emphasizes safety because small mistakes in engine code can cause:
- Memory corruption and crashes
- Security vulnerabilities
- Data loss
- Difficult-to-debug issues

We'll show you the correct patterns and explain common pitfalls with real examples from bugs we've encountered and fixed.

### 1.2 NetCI Architecture Overview

```
┌─────────────────────────────────────┐
│   NLPC Code (Mudlib)                │  ← Your game/application code
├─────────────────────────────────────┤
│   NLPC Runtime (Interpreter)        │  ← Executes NLPC bytecode
├─────────────────────────────────────┤
│   System Calls (sys*.c)             │  ← C functions callable from NLPC
├─────────────────────────────────────┤
│   Core Engine (file.c, cache.c)    │  ← Core functionality
├─────────────────────────────────────┤
│   Network Layer (intrface.c)        │  ← TCP/IP handling
└─────────────────────────────────────┘
```

**Where Extensions Fit:**
- **System Calls**: Add new functions NLPC can call (most common)
- **Callbacks**: Call NLPC functions from C code (advanced)
- **Core Engine**: Modify fundamental behavior (rare, complex)

---

## 2. Adding System Calls

### 2.1 What Are System Calls?

System calls are C functions that NLPC code can invoke. They bridge the gap between high-level NLPC code and low-level C functionality.

**Example:**
```c
// NLPC code
int len = strlen("hello");  // Calls C function s_strlen()

// C implementation in sys6a.c
int s_strlen(struct object *caller, struct object *obj, 
             struct object *player, struct var_stack **rts) {
  // Implementation
}
```

**The Runtime Stack:**

System calls communicate via a runtime stack:
- NLPC pushes arguments onto the stack
- C function pops arguments from the stack
- C function pushes return value onto the stack
- NLPC pops the return value

**Stack Order (LIFO - Last In, First Out):**
```
NLPC: strlen("hello")

Stack before C function:
  [NUM_ARGS: 1]      ← Top (pop first)
  ["hello"]          ← Bottom (pop last)

C function pops in reverse order:
  1. Pop NUM_ARGS (1)
  2. Pop string ("hello")
  3. Calculate length (5)
  4. Push result (5)

Stack after C function:
  [5]                ← Return value
```

**Important:** When there are arguments, they're pushed first, then NUM_ARGS last. When there are NO arguments (0), NUM_ARGS is pushed alone.

### 2.2 System Call Anatomy

Every system call follows this pattern:

```c
int s_function_name(struct object *caller,    // Who called this
                    struct object *obj,        // Object being executed
                    struct object *player,     // Interactive player (if any)
                    struct var_stack **rts)    // Runtime stack
{
  struct var tmp;
  
  // 1. Pop NUM_ARGS and validate
  //    (NUM_ARGS is always on top of stack)
  if (pop(&tmp, rts, obj)) return 1;
  if (tmp.type != NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num != EXPECTED_ARGS) return 1;
  
  // 2. Pop arguments (in reverse order they were pushed)
  //    If there are arguments, they were pushed before NUM_ARGS
  // 3. Validate argument types
  // 4. Perform operation
  // 5. Push return value
  // 6. Return 0 for success, 1 for error
  
  return 0;
}
```

**Parameters:**
- `caller` - The object that called this function (may be NULL)
- `obj` - The object whose code is executing
- `player` - The interactive player object (may be NULL)
- `rts` - Pointer to runtime stack pointer

**Return Values:**
- `0` - Success
- `1` - Error (stack will be cleaned up by caller)

### 2.3 Step-by-Step: Adding a System Call

Let's add a simple system call: `reverse(string)` that reverses a string.

#### Step 1: Choose the Right File

**New Pattern (Recommended):** `sfun_[category].c` for new efuns

NetCI is migrating to a cleaner organization where new efuns go into category-specific files:
- `sfun_arrays.c` - Array operations (implode, explode, sort_array, etc.)
- `sfun_mappings.c` - Mapping operations (keys, values, map_delete, etc.)
- `sfun_strings.c` - String operations (replace_string, etc.)
- `sfun_files.c` - File operations (read_file, write_file, get_dir, etc.)
- `sfun_interactive.c` - Interactive/player operations (users, etc.)
- `sfun_objects.c` - Object collection operations (objects, children, all_inventory)

**Legacy Pattern:** `sys*.c` files

Older efuns are in these files:
- `sys1.c` - Object/verb operations
- `sys2.c` - String operations
- `sys3.c` - File operations
- `sys4.c` - Object operations
- `sys5.c` - Utility operations
- `sys6a.c`, `sys6b.c` - String operations (continued)
- `sys7.c` - Network operations
- `sys8.c` - Miscellaneous operations

**For new features:** Create or use an appropriate `sfun_[category].c` file.  
**For our reverse() example:** We'll use `sys6b.c` for this guide, but `sfun_strings.c` would be preferred for new code.

#### Step 2: Write the C Function

```c
// In src/sys6b.c

int s_reverse(struct object *caller, struct object *obj, 
              struct object *player, struct var_stack **rts) {
  struct var tmp;
  char *result, *src;
  int len, i;
  
  /* Pop NUM_ARGS */
  if (pop(&tmp, rts, obj)) return 1;
  if (tmp.type != NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  
  /* Expect 1 argument */
  if (tmp.value.num != 1) return 1;
  
  /* Pop string argument */
  if (pop(&tmp, rts, obj)) return 1;
  
  /* Handle NULL/0 as empty string */
  if (tmp.type == INTEGER && tmp.value.integer == 0) {
    tmp.type = STRING;
    tmp.value.string = copy_string("");
  }
  
  /* Validate it's a string */
  if (tmp.type != STRING) {
    clear_var(&tmp);
    return 1;
  }
  
  /* Perform the operation */
  src = tmp.value.string;
  len = strlen(src);
  result = MALLOC(len + 1);
  
  for (i = 0; i < len; i++) {
    result[i] = src[len - 1 - i];
  }
  result[len] = '\0';
  
  /* Clean up input */
  clear_var(&tmp);
  
  /* Push result */
  tmp.type = STRING;
  tmp.value.string = result;
  push(&tmp, rts);
  
  return 0;
}
```

**Key Points:**
1. Always pop NUM_ARGS first
2. Validate argument count
3. Pop arguments in reverse order
4. Handle NULL/0 as empty string (common pattern)
5. Validate types
6. Clean up input variables
7. Push result
8. Return 0 for success

#### Step 3: Register the Function Name

**⚠️ CRITICAL:** You must register the function in **THREE** places!

**3a. Add function name to `scall_array` in `src/compile1.c`:**

```c
// In src/compile1.c, around line 133
char *scall_array[NUM_SCALLS]={ "add_verb","add_xverb","call_other",
  // ... existing functions ...
  "get_dir","file_size","reverse"  // Add your function name here
};
```

**3b. Add function pointer to the interpreter table in `src/interp.c`:**

```c
// In src/interp.c, around line 20-42
// This is an array of function pointers
int (*functions[])(struct object *, struct object *, struct object *, 
                   struct var_stack **) = {
  // ... existing function pointers ...
  s_remove,s_rename,s_get_dir,s_file_size,s_reverse  // Add your function pointer here
};
```

**⚠️ THIS STEP IS CRITICAL** - Missing this causes segfaults because the interpreter can't find your function!

**3c. Update `NUM_SCALLS` in `src/instr.h`:**

```c
// In src/instr.h
#define NUM_SCALLS     125  // Increment by 1 for each new function
```

**The three arrays must match:**
- `scall_array` in `compile1.c` - function names (for compilation)
- Function pointer array in `interp.c` - actual C functions (for execution)
- `NUM_SCALLS` in `instr.h` - count of all functions

If these don't match, you'll get segfaults or "undefined function" errors.

#### Step 4: Add Function Prototype

Add the prototype to `src/protos.h`:

```c
// In src/protos.h

int s_reverse(struct object *, struct object *, struct object *, 
              struct var_stack **);
```

#### Step 5: Compile and Test

```bash
make clean
make
```

Test in NLPC:
```c
string result = reverse("hello");
// result should be "olleh"
```

### 2.4 Complete Working Example: strlen()

Here's a real example from the codebase (`src/sys6a.c`):

```c
int s_strlen(struct object *caller, struct object *obj, struct object *player,
             struct var_stack **rts) {
  struct var tmp;
  long retval;

  /* Pop NUM_ARGS */
  if (pop(&tmp, rts, obj)) return 1;
  if (tmp.type != NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  
  /* Expect 1 argument */
  if (tmp.value.num != 1) return 1;
  
  /* Pop string argument */
  if (pop(&tmp, rts, obj)) return 1;
  
  /* Handle NULL/0 as empty string */
  if (tmp.type == INTEGER && tmp.value.integer == 0) {
    tmp.type = STRING;
    *(tmp.value.string = MALLOC(1)) = '\0';
  }
  
  /* Validate it's a string */
  if (tmp.type != STRING) {
    clear_var(&tmp);
    return 1;
  }
  
  /* Calculate length */
  retval = strlen(tmp.value.string);
  
  /* Clean up input */
  clear_var(&tmp);
  
  /* Push result */
  tmp.type = INTEGER;
  tmp.value.integer = retval;
  push(&tmp, rts);
  
  return 0;
}
```

**This is the pattern to follow for all system calls.**

### 2.5 Type Handling

#### Integers

**Popping:**
```c
struct var tmp;
if (pop(&tmp, rts, obj)) return 1;
if (tmp.type != INTEGER) {
  clear_var(&tmp);
  return 1;
}
int value = tmp.value.integer;
```

**Pushing:**
```c
struct var tmp;
tmp.type = INTEGER;
tmp.value.integer = 42;
push(&tmp, rts);
```

#### Strings

**Popping:**
```c
struct var tmp;
if (pop(&tmp, rts, obj)) return 1;

/* Handle NULL/0 as empty string (common pattern) */
if (tmp.type == INTEGER && tmp.value.integer == 0) {
  tmp.type = STRING;
  tmp.value.string = copy_string("");
}

if (tmp.type != STRING) {
  clear_var(&tmp);
  return 1;
}
char *str = tmp.value.string;
```

**Pushing:**
```c
struct var tmp;
tmp.type = STRING;
tmp.value.string = copy_string("hello");  // Always copy!
push(&tmp, rts);
```

**⚠️ CRITICAL: Never allocate strings yourself when pushing!**

See Section 3.3 for the critical string handling bug.

#### Objects

**Popping:**
```c
struct var tmp;
if (pop(&tmp, rts, obj)) return 1;
if (tmp.type != OBJECT) {
  clear_var(&tmp);
  return 1;
}
struct object *obj_ref = tmp.value.object;

/* Always check for NULL */
if (!obj_ref) {
  clear_var(&tmp);
  return 1;
}
```

**Pushing:**
```c
struct var tmp;
tmp.type = OBJECT;
tmp.value.object = some_object;  // May be NULL
push(&tmp, rts);
```

### 2.6 Error Handling

**Return Value Conventions:**
- `return 0` - Success, return value on stack
- `return 1` - Error, stack will be cleaned up

**Always clean up on error:**
```c
if (error_condition) {
  clear_var(&tmp);      // Clean up any popped variables
  clear_var(&tmp2);     // Clean up all variables
  return 1;             // Return error
}
```

**Logging errors:**
```c
#include "logger.h"

if (error_condition) {
  logger(LOG_ERROR, "reverse: Invalid argument type");
  clear_var(&tmp);
  return 1;
}
```

### 2.7 Argument Order Patterns

**Critical Understanding:** The order arguments are pushed depends on whether there are arguments:

**Pattern 1: With Arguments (most common)**
```c
// Pushing (from NLPC or C calling interp):
push(arg1);
push(arg2);
push(NUM_ARGS: 2);  // ← NUM_ARGS pushed LAST

// Popping (in C system call):
pop(NUM_ARGS);      // ← NUM_ARGS popped FIRST
pop(arg2);          // ← Arguments popped in reverse
pop(arg1);
```

**Pattern 2: No Arguments**
```c
// Pushing (from intrface.c example):
tmp.type = NUM_ARGS;
tmp.value.num = 0;
rts = NULL;
push(&tmp, &rts);   // ← NUM_ARGS pushed alone

// Popping:
pop(NUM_ARGS);      // ← Just pop NUM_ARGS, no arguments to pop
```

**Why This Matters:**
- Always pop NUM_ARGS first (it's always on top)
- Then pop arguments in reverse order they were pushed
- When building callbacks, push arguments first, then NUM_ARGS last

**Example from ls_dir() in file.c:**
```c
// Has 1 argument
tmp.type = STRING;
tmp.value.string = buf;
push(&tmp, &rts);        // ← Argument pushed first

tmp.type = NUM_ARGS;
tmp.value.integer = 1;
push(&tmp, &rts);        // ← NUM_ARGS pushed last

interp(uid, tmpobj, player, &rts, listen_func);
```

**Example from intrface.c (boot connect):**
```c
// Has 0 arguments
tmp.type = NUM_ARGS;
tmp.value.num = 0;
rts = NULL;
push(&tmp, &rts);        // ← NUM_ARGS pushed alone

interp(NULL, boot, NULL, &rts, func);
```

### 2.8 Zero-Argument Functions

**⚠️ CRITICAL:** Even functions with NO arguments must handle NUM_ARGS!

#### The Pattern

```c
int s_users(struct object *caller, struct object *obj, struct object *player,
            struct var_stack **rts) {
  struct var tmp, result;
  
  /* Pop NUM_ARGS - users() takes no arguments */
  if (pop(&tmp, rts, obj)) return 1;
  if (tmp.type != NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num != 0) return 1;  // ← Must check for 0!
  
  /* ... do work ... */
  
  /* Push result */
  result.type = ARRAY;
  result.value.array_ptr = arr;
  push(&result, rts);
  
  return 0;
}
```

**Why This Matters:**

The interpreter ALWAYS pushes NUM_ARGS before calling any efun, even if the count is 0. If you don't pop it, the stack becomes corrupted and causes segfaults.

**Real-World Example:** `s_this_player()` from `src/sys1.c`:

```c
int s_this_player(struct object *caller, struct object *obj, struct object *player,
                   struct var_stack **rts) {
  struct var tmp;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=0) return 1;  // ← Expects 0 arguments
  tmp.type=OBJECT;
  tmp.value.objptr=player;
  if (!player) {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  }
  push(&tmp,rts);
  return 0;
}
```

**Common Mistake:**
```c
// ❌ WRONG - Will cause segfault!
int s_users(struct object *caller, struct object *obj, struct object *player,
            struct var_stack **rts) {
  // Missing NUM_ARGS pop!
  struct heap_array *arr = allocate_array(count, UNLIMITED_ARRAY_SIZE);
  // ... segfault when interpreter tries to clean up stack
}
```

### 2.9 More Common Patterns

#### Multiple Arguments

```c
/* Pop NUM_ARGS (always first) */
if (pop(&tmp, rts, obj)) return 1;
if (tmp.type != NUM_ARGS) {
  clear_var(&tmp);
  return 1;
}
if (tmp.value.num != 2) return 1;  // Expect 2 arguments

/* Pop arguments in REVERSE order they were pushed */
if (pop(&tmp2, rts, obj)) return 1;  // Second argument (pushed second)
if (pop(&tmp1, rts, obj)) {          // First argument (pushed first)
  clear_var(&tmp2);
  return 1;
}

/* Validate and use tmp1 and tmp2 */
```

#### Optional Arguments

```c
/* Pop NUM_ARGS */
if (pop(&tmp, rts, obj)) return 1;
if (tmp.type != NUM_ARGS) {
  clear_var(&tmp);
  return 1;
}

int num_args = tmp.value.num;
if (num_args < 1 || num_args > 2) return 1;  // 1 or 2 arguments

/* Pop required argument */
if (pop(&tmp1, rts, obj)) return 1;

/* Pop optional argument if present */
if (num_args == 2) {
  if (pop(&tmp2, rts, obj)) {
    clear_var(&tmp1);
    return 1;
  }
} else {
  /* Use default value */
  tmp2.type = INTEGER;
  tmp2.value.integer = 0;
}
```

#### Returning Multiple Values

NLPC doesn't support multiple return values directly, but you can:

**Option 1: Return a string with delimiters**
```c
/* Return "value1,value2" */
char *result = MALLOC(100);
sprintf(result, "%d,%d", val1, val2);
tmp.type = STRING;
tmp.value.string = result;
push(&tmp, rts);
```

**Option 2: Modify object properties**
```c
/* Set properties on an object passed as argument */
set_object_property(obj, "result1", val1);
set_object_property(obj, "result2", val2);

/* Return success code */
tmp.type = INTEGER;
tmp.value.integer = 0;
push(&tmp, rts);
```

---

## 3. Implementing Callbacks

### 3.1 What Are Callbacks?

Callbacks allow C code to call NLPC functions. This is more complex than system calls because you're invoking the interpreter from within C code.

**Example:**
```c
// C code in file.c
int check_master_permission(char *path, char *operation, struct object *uid) {
  // Call valid_read() or valid_write() in boot.c
  interp(NULL, master, NULL, &rts, callback_func);
  // Check result
}
```

**When to Use Callbacks:**
- Security checks (valid_read, valid_write)
- Event notifications
- Extensible behavior
- Plugin systems

**When NOT to Use Callbacks:**
- Simple operations (use system calls instead)
- Performance-critical code (callbacks have overhead)
- During bootstrap (object system may not be ready)

### 3.2 The Critical Problem: Global Variable Corruption

**⚠️ THIS IS THE MOST IMPORTANT SECTION IN THIS GUIDE ⚠️**

When calling `interp()` from C code while another `interp()` is executing, you MUST save and restore global execution state variables.

#### 3.2.1 Understanding the Bug

The NetCI interpreter uses global variables to track execution state:

```c
// In src/globals.h
extern unsigned int num_locals;  // Number of local variables
extern struct var *locals;       // Array of local variables
```

**The Problem:**

When `interp()` is called recursively from C code (not from within LPC), these globals are overwritten, corrupting the outer execution context.

**What Happens:**
```
1. Outer interp() executing, locals = 0x12345000 (valid memory)
2. C code calls check_master_permission()
3. check_master_permission() calls interp() for callback
4. Inner interp() OVERWRITES locals = 0x67890000 (new allocation)
5. Inner interp() tries to allocate more memory
6. Malloc sees corrupted metadata (references old locals pointer)
7. CRASH: "Corruption of tiny freelist"
```

**Stack Trace When This Happens:**
```
frame #8: copy_string() - trying to allocate memory
frame #9: resolve_var() - resolving a variable
frame #10: gen_stack() - building argument stack
frame #11: interp() - INNER execution (callback)
frame #12: interp() - OUTER execution (original LPC code)
```

The crash happens in the INNER `interp()` because the execution environment is corrupted.

#### 3.2.2 The Solution Pattern

**ALWAYS do this when calling interp() from C code:**

```c
/* CRITICAL: Save global state before calling interp() */
extern unsigned int num_locals;
extern struct var *locals;
unsigned int old_num_locals = num_locals;
struct var *old_locals = locals;

/* Your interp() call */
interp(NULL, tmpobj, NULL, &rts, callback_func);

/* CRITICAL: Restore global state */
num_locals = old_num_locals;
locals = old_locals;
```

**Complete Example from file.c:**

```c
int check_master_permission(char *path, char *operation, 
                            struct object *uid, int is_write) {
  struct object *master, *tmpobj;
  struct fns *callback_func;
  struct var_stack *rts;
  struct var tmp;
  char *func_name;
  int result;
  
  /* ... setup code ... */
  
  /* CRITICAL: Save globals */
  extern unsigned int num_locals;
  extern struct var *locals;
  unsigned int old_num_locals = num_locals;
  struct var *old_locals = locals;
  
  /* Call callback */
  rts = NULL;
  
  /* Push arguments */
  tmp.type = STRING;
  tmp.value.string = path;
  push(&tmp, &rts);
  
  /* ... push more arguments ... */
  
  tmp.type = NUM_ARGS;
  tmp.value.integer = 5;
  push(&tmp, &rts);
  
  /* Call interp() */
  interp(NULL, tmpobj, NULL, &rts, callback_func);
  
  /* CRITICAL: Restore globals */
  num_locals = old_num_locals;
  locals = old_locals;
  
  /* Pop result */
  if (pop(&tmp, &rts, NULL)) {
    free_stack(&rts);
    return 1;
  }
  
  result = (tmp.type == INTEGER && tmp.value.integer != 0);
  clear_var(&tmp);
  free_stack(&rts);
  
  return result;
}
```

#### 3.2.3 When This Is Required

**REQUIRED (must save/restore globals):**
- File operations called from LPC code
- System calls that trigger callbacks
- Any C function that might be called during LPC execution
- Basically: any `interp()` call from C that isn't the first/top-level call

**NOT REQUIRED (safe without save/restore):**
- Main event loop calling `interp()` for top-level commands
- Boot sequence calling `interp()` for initialization
- Network handlers calling `interp()` for new connections

These are safe because they're the **first** `interp()` call, not nested.

**How to Tell:**
- If your C function might be called while LPC code is executing → REQUIRED
- If your C function is only called from the main loop → NOT REQUIRED
- **When in doubt, always save/restore** - it's safe even when not strictly needed

#### 3.2.4 Why interp() Doesn't Handle This Automatically

`interp()` DOES save and restore globals when called recursively **from within LPC code**:

```c
// In src/interp.c, FUNC_CALL case:
old_locals = locals;
old_num_locals = num_locals;
if (interp(obj, obj, player, &stack1, func->code[loop].value.func_call)) {
  locals = old_locals;
  num_locals = old_num_locals;
}
locals = old_locals;
num_locals = old_num_locals;
```

But when you call `interp()` directly from C code, you're bypassing this mechanism, so you must do it yourself.

### 3.3 String Handling in Callbacks

**⚠️ CRITICAL: String Allocation Bug ⚠️**

When pushing strings as arguments to callbacks, do NOT allocate memory yourself.

#### 3.3.1 The Bug

```c
/* WRONG - causes malloc corruption */
tmp.type = STRING;
tmp.value.string = MALLOC(strlen(path) + 1);
strcpy(tmp.value.string, path);
push(&tmp, &rts);
FREE(tmp.value.string);  // ❌ Freeing too early!
```

**Why This Fails:**

The `push()` function internally calls `copy_string()` which:
1. Allocates its own memory
2. Copies the string

By freeing immediately after `push()`, you corrupt malloc's metadata.

#### 3.3.2 The Correct Pattern

```c
/* CORRECT - let push() handle allocation */
tmp.type = STRING;
tmp.value.string = path;  // ✅ Just point to existing string
push(&tmp, &rts);         // push() makes its own copy
```

**How push() Works:**

```c
// From src/constrct.c
void push(struct var *data, struct var_stack **rts) {
  struct var_stack *tmp;
  tmp = MALLOC(sizeof(struct var_stack));
  tmp->data.type = data->type;
  switch (data->type) {
    case STRING:
      tmp->data.value.string = copy_string(data->value.string);  // Makes copy
      break;
```

And `copy_string()`:

```c
char *copy_string(char *s) {
  return strcpy((char *) MALLOC(strlen(s)+1), s);  // Allocates and copies
}
```

**Key Insight:** `push()` handles all memory management for strings. Callers should just pass pointers to existing strings.

#### 3.3.3 Complete Example

```c
/* Pushing string arguments to callback */
struct var tmp;
struct var_stack *rts = NULL;

/* Argument 1: path (string) */
tmp.type = STRING;
tmp.value.string = path;  // ✅ Just point, don't allocate
push(&tmp, &rts);

/* Argument 2: operation (string) */
tmp.type = STRING;
tmp.value.string = operation;  // ✅ Just point, don't allocate
push(&tmp, &rts);

/* Argument 3: caller (object) */
tmp.type = OBJECT;
tmp.value.object = uid;
push(&tmp, &rts);

/* NUM_ARGS */
tmp.type = NUM_ARGS;
tmp.value.integer = 3;
push(&tmp, &rts);

/* Call interp() */
interp(NULL, tmpobj, NULL, &rts, callback_func);

/* Clean up */
free_stack(&rts);
```

### 3.4 Re-entry Guards

Even with globals saved, callbacks might trigger more callbacks, causing infinite recursion:

```
check_master_permission() 
  → interp(valid_read) 
  → fread() 
  → open_file() 
  → check_master_permission() 
  → ...
```

**Solution: Re-entry Guard**

```c
static int in_master_callback = 0;

int check_master_permission(...) {
  /* Re-entry guard */
  if (in_master_callback) {
    return 1;  // Allow without callback
  }
  
  /* Set flag */
  in_master_callback = 1;
  
  /* ... save globals ... */
  
  /* Call interp() */
  interp(NULL, tmpobj, NULL, &rts, callback_func);
  
  /* ... restore globals ... */
  
  /* Clear flag */
  in_master_callback = 0;
  
  return result;
}
```

This prevents infinite recursion by allowing operations during callbacks without triggering more callbacks.

### 3.5 Bootstrap Considerations

During system boot, the object system may not be fully initialized:

```c
int check_master_permission(...) {
  /* Bootstrap check: object system not ready */
  if (!obj_list) return 1;
  
  /* NULL caller during boot */
  if (!uid) return 1;
  
  /* Master object check */
  master = db_ref_to_obj(0);
  if (!master) return 1;
  
  /* Master recursion check */
  if (uid == master) return 1;
  
  /* ... rest of function ... */
}
```

**Always check:**
1. Object system initialized (`obj_list`)
2. Caller is not NULL (unless system operation)
3. Master object exists
4. Not recursing into master itself

### 3.6 Complete Callback Example

Here's the complete `check_master_permission()` from `src/file.c`:

```c
static int in_master_callback = 0;

int check_master_permission(char *path, char *operation, 
                            struct object *uid, int is_write) {
  struct object *master, *tmpobj;
  struct fns *callback_func;
  struct var_stack *rts;
  struct var tmp;
  char *func_name;
  int result;
  struct file_entry *fe;
  
  /* Re-entry guard */
  if (in_master_callback) return 1;
  
  /* Bootstrap checks */
  if (!obj_list) return 1;
  if (!uid) return 1;
  
  /* Get master object */
  master = db_ref_to_obj(0);
  if (!master) return 1;
  
  /* Prevent master recursion */
  if (uid == master) return 1;
  
  /* Find callback function */
  func_name = is_write ? "valid_write" : "valid_read";
  callback_func = find_function(func_name, master, &tmpobj);
  if (!callback_func) return 1;  // Fallback to allow
  
  /* Get file info */
  fe = find_entry(path);
  int owner = fe ? fe->owner : -1;
  int flags = fe ? fe->flags : -1;
  
  /* CRITICAL: Save globals */
  extern unsigned int num_locals;
  extern struct var *locals;
  unsigned int old_num_locals = num_locals;
  struct var *old_locals = locals;
  
  /* Set re-entry flag */
  in_master_callback = 1;
  
  /* Build argument stack */
  rts = NULL;
  
  /* Arg 1: path */
  tmp.type = STRING;
  tmp.value.string = path;
  push(&tmp, &rts);
  
  /* Arg 2: operation */
  tmp.type = STRING;
  tmp.value.string = operation;
  push(&tmp, &rts);
  
  /* Arg 3: caller */
  tmp.type = OBJECT;
  tmp.value.object = uid;
  push(&tmp, &rts);
  
  /* Arg 4: owner */
  tmp.type = INTEGER;
  tmp.value.integer = owner;
  push(&tmp, &rts);
  
  /* Arg 5: flags */
  tmp.type = INTEGER;
  tmp.value.integer = flags;
  push(&tmp, &rts);
  
  /* NUM_ARGS */
  tmp.type = NUM_ARGS;
  tmp.value.integer = 5;
  push(&tmp, &rts);
  
  /* Call callback */
  interp(NULL, tmpobj, NULL, &rts, callback_func);
  
  /* Clear re-entry flag */
  in_master_callback = 0;
  
  /* CRITICAL: Restore globals */
  num_locals = old_num_locals;
  locals = old_locals;
  
  /* Pop result */
  if (pop(&tmp, &rts, NULL)) {
    free_stack(&rts);
    return 1;
  }
  
  /* Check result (non-zero = allow) */
  result = (tmp.type == INTEGER && tmp.value.integer != 0);
  
  /* Log denial */
  if (!result) {
    logger(LOG_WARNING, "Permission denied: %s by %s", 
           path, otoa(uid));
  }
  
  /* Clean up */
  clear_var(&tmp);
  free_stack(&rts);
  
  return result;
}
```

**This example demonstrates all the safety patterns:**
1. ✅ Re-entry guard
2. ✅ Bootstrap checks
3. ✅ Global variable save/restore
4. ✅ Correct string handling (no allocation)
5. ✅ Proper cleanup
6. ✅ Error logging

---

## Quick Reference

### System Call Checklist

**Implementation:**
- [ ] Function signature matches pattern
- [ ] Pop NUM_ARGS first (even for zero-argument functions!)
- [ ] Validate argument count (check for 0 if no arguments)
- [ ] Pop arguments in reverse order
- [ ] Handle NULL/0 as empty string for strings
- [ ] Validate all argument types
- [ ] Clean up on error
- [ ] Push return value
- [ ] Return 0 for success, 1 for error

**Registration (CRITICAL - Missing any causes segfaults!):**
- [ ] Add function name to `scall_array` in `src/compile1.c`
- [ ] **Add function pointer to interpreter table in `src/interp.c`** ⚠️
- [ ] Increment `NUM_SCALLS` in `src/instr.h`
- [ ] Add prototype with `OPER_PROTO()` macro in `src/protos.h`
- [ ] If new file, add `.o` file to `OFILES` in `src/Makefile` and `src/autoconf.in`
- [ ] If new file, add build rule in `src/Makefile` and `src/autoconf.in`

### Appendix B: Callback Checklist

- [ ] Check if re-entry guard needed
- [ ] Add bootstrap checks
- [ ] **SAVE num_locals and locals globals**
- [ ] Set re-entry flag (if using)
- [ ] Push arguments (strings: just point, don't allocate!)
- [ ] Call interp()
- [ ] Clear re-entry flag (if using)
- [ ] **RESTORE num_locals and locals globals**
- [ ] Pop and check result
- [ ] Clean up stack
- [ ] Log errors/denials

### Appendix C: Safety Checklist

- [ ] No malloc/free around push() for strings
- [ ] Globals saved/restored for interp() calls
- [ ] Re-entry guard if recursion possible
- [ ] Bootstrap checks if called early
- [ ] All variables cleaned up on error
- [ ] NULL checks for all pointers
- [ ] Bounds checking for arrays/strings
- [ ] Error logging for debugging

---

**See Also:**
- [Mudlib Development Guide](MUDLIB-DEVELOPMENT-GUIDE.md) - For NLPC developers
- [Technical Reference](ci200fs-library/TECHNICAL-REFERENCE.md) - For system reference
- [Memory Corruption Case Study](../specs/3-Automatic-Virtual-Filesystem/phase2-memory-corruption-fix.md) - Complete bug analysis


#### Optional Arguments

```c
/* Pop NUM_ARGS */
if (pop(&tmp, rts, obj)) return 1;
if (tmp.type != NUM_ARGS) {
  clear_var(&tmp);
  return 1;
}

int num_args = tmp.value.num;
if (num_args < 1 || num_args > 2) return 1;  // 1 or 2 arguments

/* Pop required argument */
if (pop(&tmp1, rts, obj)) return 1;

/* Pop optional argument if present */
if (num_args == 2) {
  if (pop(&tmp2, rts, obj)) {
    clear_var(&tmp1);
    return 1;
  }
} else {
  /* Use default value */
  tmp2.type = INTEGER;
  tmp2.value.integer = 0;
}
```

---

## 4. Memory Management

### 4.1 MALLOC and FREE

**Basic Rules:**
- Always `MALLOC()` before use
- Always `FREE()` when done
- Never `FREE()` twice
- Never use after `FREE()`

**Pattern:**
```c
char *buffer = MALLOC(size);
// Use buffer
FREE(buffer);
buffer = NULL;  // Good practice
```

### 4.2 String Memory Management

**Rule 1: copy_string() allocates**
```c
char *str = copy_string("hello");  // Allocates memory
// Use str
FREE(str);  // Must free
```

**Rule 2: push() copies strings**
```c
tmp.type = STRING;
tmp.value.string = some_string;  // Just point
push(&tmp, &rts);                // push() makes copy
// Don't free some_string here!
```

**Rule 3: clear_var() frees**
```c
struct var tmp;
tmp.type = STRING;
tmp.value.string = copy_string("hello");
clear_var(&tmp);  // Frees the string
```

### 4.3 Stack Management

**Always clean up the stack:**
```c
struct var_stack *rts = NULL;

// Push arguments
// Call interp()

// Always free the stack
free_stack(&rts);
```

**On error, clean up variables:**
```c
if (error) {
  clear_var(&tmp1);
  clear_var(&tmp2);
  free_stack(&rts);
  return 1;
}
```

---

## 5. Security Considerations

### 5.1 Path Validation

**Always validate paths before file operations:**

```c
int validate_path(char *path) {
  char *resolved;
  int result;
  
  /* Check for NULL or empty */
  if (!path || !*path) {
    logger(LOG_ERROR, "Security: NULL or empty path");
    return 0;
  }
  
  /* Must be absolute */
  if (path[0] != '/') {
    logger(LOG_ERROR, "Security: Relative path rejected: %s", path);
    return 0;
  }
  
  /* Resolve .. components */
  resolved = resolve_path(path);
  
  /* Check if escaped mudlib root */
  if (strncmp(resolved, fs_path, strlen(fs_path)) != 0) {
    logger(LOG_ERROR, "Security: Path escape attempt: %s", path);
    FREE(resolved);
    return 0;
  }
  
  FREE(resolved);
  return 1;
}
```

**Threats Prevented:**
- Directory traversal: `../../etc/passwd`
- Absolute paths outside mudlib
- Symlink escapes

### 5.2 Permission Checking

**Pattern:**
```c
/* Check if operation is allowed */
if (!check_master_permission(path, "read_file", uid, 0)) {
  logger(LOG_WARNING, "Permission denied: %s by %s", path, otoa(uid));
  return -2;  // Permission denied error code
}
```

### 5.3 Privilege Checks

**Always check privileges for sensitive operations:**
```c
if (!priv(caller)) {
  logger(LOG_WARNING, "Privilege required for operation");
  return 1;  // Error
}
```

---

## 6. Working Examples

### 6.1 Simple System Call: strlen()

Complete implementation from `src/sys6a.c`:

```c
int s_strlen(struct object *caller, struct object *obj, struct object *player,
             struct var_stack **rts) {
  struct var tmp;
  long retval;

  /* Pop NUM_ARGS */
  if (pop(&tmp, rts, obj)) return 1;
  if (tmp.type != NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num != 1) return 1;
  
  /* Pop string argument */
  if (pop(&tmp, rts, obj)) return 1;
  
  /* Handle NULL/0 as empty string */
  if (tmp.type == INTEGER && tmp.value.integer == 0) {
    tmp.type = STRING;
    *(tmp.value.string = MALLOC(1)) = '\0';
  }
  
  /* Validate type */
  if (tmp.type != STRING) {
    clear_var(&tmp);
    return 1;
  }
  
  /* Perform operation */
  retval = strlen(tmp.value.string);
  
  /* Clean up input */
  clear_var(&tmp);
  
  /* Push result */
  tmp.type = INTEGER;
  tmp.value.integer = retval;
  push(&tmp, rts);
  
  return 0;
}
```

### 6.2 Callback with All Safety Patterns

Complete implementation from `src/file.c`:

See Section 3.6 for the complete `check_master_permission()` example with all safety patterns.

---

## 7. Testing and Debugging

### 7.1 Testing System Calls

**Create test NLPC code:**

```c
// test_reverse.c
void test_reverse() {
  string result;
  
  result = reverse("hello");
  if (result != "olleh") {
    write("FAIL: reverse('hello') = '" + result + "', expected 'olleh'\n");
  } else {
    write("PASS: reverse('hello')\n");
  }
  
  result = reverse("");
  if (result != "") {
    write("FAIL: reverse('') = '" + result + "', expected ''\n");
  } else {
    write("PASS: reverse('')\n");
  }
}
```

### 7.2 Debugging with gdb

```bash
# Compile with debug symbols
make CFLAGS="-g -O0"

# Run under gdb
gdb ./netci
(gdb) set args -f mudlib
(gdb) run

# When it crashes:
(gdb) bt          # Stack trace
(gdb) info locals # Local variables
(gdb) print *rts  # Examine stack
```

### 7.3 Adding Debug Logging

```c
#include "logger.h"

int s_debug_function(...) {
  logger(LOG_DEBUG, "debug_function called by %s", otoa(caller));
  
  /* ... function body ... */
  
  logger(LOG_DEBUG, "debug_function returning %d", result);
  return result;
}
```

---

## 8. Best Practices

### 8.1 Code Organization

- **One system call per function**
- **Group related functions in same sys*.c file**
- **Use descriptive function names**
- **Add comprehensive comments**

### 8.2 Error Handling

- **Always check return values**
- **Log errors with context**
- **Clean up on all error paths**
- **Return consistent error codes**

### 8.3 Performance

- **Minimize memory allocations**
- **Cache frequently-used values**
- **Avoid expensive operations in loops**
- **Profile before optimizing**

---

## 9. Reference

### 9.1 Stack Functions

```c
int push(struct var *data, struct var_stack **rts);
int pop(struct var *data, struct var_stack **rts, struct object *obj);
void free_stack(struct var_stack **rts);
void clear_var(struct var *var);
```

### 9.2 String Functions

```c
char *copy_string(char *src);           // Allocate and copy
void free_string(char *str);            // Free string
int strlen(char *str);                  // String length
char *strcpy(char *dest, char *src);    // Copy string
```

### 9.3 Object Functions

```c
struct object *db_ref_to_obj(int ref);  // Get object by reference
char *otoa(struct object *obj);         // Object to string
int otoi(struct object *obj);           // Object to integer
int priv(struct object *obj);           // Check privileges
```

### 9.4 Logging Functions

```c
void logger(int level, char *format, ...);

#define LOG_ERROR   1
#define LOG_WARNING 2
#define LOG_INFO    3
#define LOG_DEBUG   4
```

---

## 10. Case Studies

### 10.1 The String Allocation Bug

**Problem:** Memory corruption when pushing strings to stack

**Cause:** Allocating and freeing strings around push() calls

**Solution:** Let push() handle all string allocation

**Lesson:** Trust the abstractions - push() is designed to handle memory management

### 10.2 The Global Variable Corruption Bug

**Problem:** Malloc corruption when calling interp() from C

**Cause:** Global execution state variables overwritten by recursive interp() calls

**Solution:** Save and restore num_locals and locals around interp() calls

**Lesson:** Understand the execution model - globals are used for execution state

### 10.3 Automatic Virtual Filesystem Implementation

**The Challenge:**
Implement automatic file discovery with master object security callbacks.

**The Bugs Encountered:**
1. String allocation bug (malloc/free pattern)
2. NULL pointer dereference (bootstrap)
3. Global variable corruption (main bug)
4. Re-entry recursion

**The Solutions:**
- Correct string handling (just point, don't allocate)
- NULL checks before use
- Save/restore globals around interp()
- Re-entry guard flag

**Complete Analysis:**
See `.kiro/specs/3-Automatic-Virtual-Filesystem/phase2-memory-corruption-fix.md`

**Key Lessons:**
- Small mistakes in engine code cause hard-to-debug crashes
- Stack traces are critical for diagnosis
- Incremental testing isolates issues
- Understanding the execution model is essential
- Following established patterns prevents bugs

---

## Appendices

### Appendix A: System Call Checklist

When adding a system call:

- [ ] Choose appropriate sys*.c file
- [ ] Write function with standard signature
- [ ] Pop NUM_ARGS first (always on top)
- [ ] Validate argument count
- [ ] Pop arguments in reverse order
- [ ] Handle NULL/0 as empty string for strings
- [ ] Validate all argument types
- [ ] Clean up on error
- [ ] Perform operation
- [ ] Push return value
- [ ] Return 0 for success, 1 for error
- [ ] Add to function table in interp.c
- [ ] Add prototype to protos.h
- [ ] Compile and test

### Callback Checklist

When implementing callbacks:

- [ ] Determine if re-entry guard needed
- [ ] Add bootstrap checks (obj_list, NULL caller, master exists)
- [ ] **CRITICAL: Save num_locals and locals globals**
- [ ] Set re-entry flag (if using)
- [ ] Initialize stack: `rts = NULL`
- [ ] Push arguments (strings: just point, don't allocate!)
- [ ] Push NUM_ARGS last
- [ ] Call interp()
- [ ] Clear re-entry flag (if using)
- [ ] **CRITICAL: Restore num_locals and locals globals**
- [ ] Pop and validate result
- [ ] Clean up stack with free_stack()
- [ ] Log errors/denials

### Safety Checklist

Critical safety patterns:

- [ ] **Never malloc/free strings around push()** - just point to existing strings
- [ ] **Always save/restore globals** when calling interp() from C
- [ ] Re-entry guard if recursion possible
- [ ] Bootstrap checks if called during boot
- [ ] All variables cleaned up on error paths
- [ ] NULL checks for all pointers
- [ ] Bounds checking for arrays/strings
- [ ] Error logging for debugging
- [ ] Path validation for file operations
- [ ] Permission checks for sensitive operations

### Appendix D: Common Mistakes

#### Mistake 1: Allocating Strings for push()

**Wrong:**
```c
tmp.value.string = MALLOC(strlen(str) + 1);
strcpy(tmp.value.string, str);
push(&tmp, &rts);
FREE(tmp.value.string);  // ❌ Causes corruption
```

**Right:**
```c
tmp.value.string = str;  // ✅ Just point
push(&tmp, &rts);        // push() handles it
```

#### Mistake 2: Forgetting to Save Globals

**Wrong:**
```c
interp(NULL, obj, NULL, &rts, func);  // ❌ Corrupts globals
```

**Right:**
```c
extern unsigned int num_locals;
extern struct var *locals;
unsigned int old_num_locals = num_locals;
struct var *old_locals = locals;

interp(NULL, obj, NULL, &rts, func);

num_locals = old_num_locals;
locals = old_locals;
```

#### Mistake 3: Not Cleaning Up on Error

**Wrong:**
```c
if (pop(&tmp, rts, obj)) return 1;  // ❌ Leaks tmp
if (error) return 1;                // ❌ Leaks tmp
```

**Right:**
```c
if (pop(&tmp, rts, obj)) return 1;
if (error) {
  clear_var(&tmp);  // ✅ Clean up
  return 1;
}
```

#### Mistake 4: Wrong Argument Order

**Wrong:**
```c
pop(&tmp1, rts, obj);  // ❌ Should pop NUM_ARGS first
pop(&tmp2, rts, obj);
pop(&num_args, rts, obj);
```

**Right:**
```c
pop(&num_args, rts, obj);  // ✅ NUM_ARGS always first
pop(&tmp2, rts, obj);      // ✅ Then arguments in reverse
pop(&tmp1, rts, obj);
```

### Appendix E: Additional Resources

#### Source Code References

**System Calls:**
- `src/sys1.c` - Object/verb operations
- `src/sys2.c` - String operations
- `src/sys6a.c` - More string operations (strlen, leftstr, etc.)
- `src/intrface.c` - Network operations with callbacks

**Callbacks:**
- `src/file.c` - check_master_permission() complete example
- `src/intrface.c` - Boot connect() callback

**Core Functions:**
- `src/constrct.c` - push(), pop(), copy_string()
- `src/interp.c` - interp() and execution engine
- `src/globals.h` - Global variables

#### Documentation References

**Bug Analysis:**
- `.kiro/specs/3-Automatic-Virtual-Filesystem/phase2-memory-corruption-fix.md` - Complete bug analysis

**Implementation Details:**
- `.kiro/specs/3-Automatic-Virtual-Filesystem/design.md` - Architecture and design
- `.kiro/specs/3-Automatic-Virtual-Filesystem/phase2-summary.md` - Implementation summary

**Related Guides:**
- `docs/MUDLIB-DEVELOPMENT-GUIDE.md` - For NLPC developers building mudlibs
- `.kiro/ci200fs-library/TECHNICAL-REFERENCE.md` - ci200fs system reference

---

## Conclusion

Extending the NetCI engine requires careful attention to safety patterns, especially when implementing callbacks. The two critical rules are:

1. **Never allocate strings yourself when pushing** - let push() handle it
2. **Always save/restore globals when calling interp()** - prevent corruption

Follow the patterns in this guide, study the working examples in the codebase, and test thoroughly. When in doubt, err on the side of caution - add extra checks, save globals even if you think you don't need to, and log liberally for debugging.

The bugs documented in this guide are real bugs we encountered and fixed. Learn from them, and your extensions will be safe and reliable.

---

**Document Version:** 1.0  
**Last Updated:** 2025-10-29  
**Status:** Complete

**Feedback:** If you find errors or have suggestions, please update this document.
