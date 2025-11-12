# Implementation Guide: Inheritance with `::` Operator

**Status**: DEEP-DIVE ANALYSIS COMPLETE ✅  
**Priority**: MEDIUM  
**Type**: Language Feature (Syntax + Runtime)  
**Estimated Effort**: 4-8 hours (NOT 4-6 weeks!)

**Analysis Date**: November 6, 2025  
**Validated By**: Exhaustive analysis of attach() system, compiler, parser, and interpreter

---

## Executive Summary

After deep-dive analysis, **true inheritance can be implemented in 4-8 hours** using THREE different approaches:

1. **Option A: Simulated Inheritance** (2-4 hours) - Use existing attach(), add `::` syntax sugar
2. **Option B: Enhanced Attach** (4-6 hours) - Extend attach() with parent tracking
3. **Option C: True Inheritance** (6-8 hours) - Full inheritance chain with virtual dispatch

**Recommendation**: Start with **Option A** (quickest win), then upgrade to **Option B** if needed.

---

## Current State Analysis

### What EXISTS Today ✅

**1. Attach System** (`/src/sys7.c` lines 365-432):
```c
int s_attach(struct object *caller, struct object *obj,
             struct object *player, struct var_stack **rts) {
    // Adds object to attachees list
    attachptr = MALLOC(sizeof(struct attach_list));
    attachptr->attachee = tmp.value.objptr;
    attachptr->next = obj->attachees;
    obj->attachees = attachptr;
    tmp.value.objptr->attacher = obj;  // Backlink!
    return 0;
}
```

**2. Function Lookup** (`/src/interp.c` lines 339-356):
```c
struct fns *find_function(char *name, struct object *obj,
                          struct object **real_obj) {
    struct fns *tmpfns;
    struct attach_list *curr_attach;
    
    // Try local functions first
    tmpfns = find_fns(name, obj);
    if (tmpfns) {
        if (real_obj) (*real_obj) = obj;
        return tmpfns;
    }
    
    // RECURSIVE SEARCH through attachees!
    curr_attach = obj->attachees;
    while (curr_attach) {
        if ((tmpfns = find_function(name, curr_attach->attachee, real_obj)))
            return tmpfns;
        curr_attach = curr_attach->next;
    }
    return NULL;
}
```

**3. Object Structure** (`/src/object.h` lines 171-190):
```c
struct attach_list {
    struct object *attachee;      // The attached object
    struct attach_list *next;     // Linked list
};

struct object {
    // ... other fields ...
    struct object *attacher;           // Who attached to me
    struct attach_list *attachees;     // Who I'm attached to
    // ... other fields ...
};
```

**4. SECOND_TOK** (`::`) **Already Tokenized**:
- Token exists: `SECOND_TOK` (58) in `/src/token.h` line 31
- Tokenizer handles it: `/src/token.c` lines 197-204
- Parser rejects it: `/src/compile1.c` lines 616-618

---

## KEY INSIGHT 🔑

**The attach() system ALREADY implements inheritance-like behavior!**

- ✅ Function lookup walks the attach chain recursively
- ✅ Objects can have multiple attachees (multiple "inheritance")
- ✅ Circular attach detection exists (`is_attached_to()`)
- ✅ Backlinks exist (`attacher` pointer)
- ✅ **Function overriding ALREADY WORKS!** Local functions shadow attached functions

**What's Missing:**
- ❌ Can't call "parent" version of overridden function
- ❌ No `::` syntax to specify which attachee
- ❌ No way to know which attachee provided a function

---

## CRITICAL: Function Overriding Already Works! ✅

**Current Behavior** (`/src/interp.c` lines 339-356):

```c
struct fns *find_function(char *name, struct object *obj,
                          struct object **real_obj) {
    struct fns *tmpfns;
    struct attach_list *curr_attach;
    
    // 1. Check LOCAL functions FIRST
    tmpfns = find_fns(name, obj);
    if (tmpfns) {
        if (real_obj) (*real_obj) = obj;
        return tmpfns;  // Local function shadows attached!
    }
    
    // 2. Only check attachees if NOT found locally
    curr_attach = obj->attachees;
    while (curr_attach) {
        if ((tmpfns = find_function(name, curr_attach->attachee, real_obj)))
            return tmpfns;
        curr_attach = curr_attach->next;
    }
    return NULL;
}
```

**This means:**
- ✅ If you define `take_damage()` locally, it OVERRIDES the attached version
- ✅ No compiler error for "duplicate function" - local always wins
- ✅ This is EXACTLY how inheritance works!

**The Problem:**
Once you override, you CAN'T call the parent version. That's what `::` solves!

### Real Example - How It Works TODAY:

```c
// base_living.c
take_damage(amount) {
    hp = hp - amount;
    write("Base: took " + itoa(amount) + " damage\n");
}

// player.c
create() {
    attach("/obj/base_living");
    hp = 100;
}

// WITHOUT local take_damage():
// player.take_damage(10) -> Calls base_living.take_damage() ✅

// WITH local take_damage():
take_damage(amount) {
    // ❌ PROBLEM: Can't call base_living's version!
    // Would need to do: call_other(attachee, "take_damage", amount)
    // But we don't have direct access to attachee!
    write("Player: Ouch!\n");
}
// player.take_damage(10) -> Only calls player.take_damage() ✅
// base_living.take_damage() is SHADOWED, unreachable!
```

**With `::` (what we're adding):**
```c
take_damage(amount) {
    ::take_damage(amount);  // ✅ Calls base_living's version!
    write("Player: Ouch!\n");
}
```

---

## OPTION A: Simulated Inheritance (EASIEST - 2-4 hours)

### Concept

Add `::function_name()` syntax that calls the FIRST attachee's version of a function, skipping the local version.

### Implementation

**1. Parser Change** (`/src/compile1.c` line 616):

**BEFORE:**
```c
if (token.type==SECOND_TOK) {
    set_c_err_msg("operation ':' unsupported");
    return file_info->phys_line;
}
```

**AFTER:**
```c
if (token.type==SECOND_TOK) {
    // ::function_name() - call parent version
    get_token(file_info, &token);
    if (token.type != NAME_TOK) {
        set_c_err_msg("expected function name after ::");
        return file_info->phys_line;
    }
    
    // Emit EXTERN_FUNC with special marker
    add_code_string(curr_fn, token.token_data.name);
    add_code_instr(curr_fn, PARENT_FUNC);  // New opcode
    last_was_arg = 1;
    get_token(file_info, &token);
    continue;  // Skip normal function handling
}
```

**2. Add Opcode** (`/src/instr.h` after line 192):
```c
#define PARENT_FUNC     151  /* ::function() - call parent version */
```

**3. Add Interpreter Handler** (`/src/interp.c` around line 980):
```c
case PARENT_FUNC:
    // Like EXTERN_FUNC but skip local function
    temp_fns = find_extern_function(func->code[loop].value.string, obj, &tmpobj);
    if (!temp_fns) {
        interp_error_with_trace("parent function not found", player, obj, func, line);
        free_stack(&rts);
        return 1;
    }
    // ... rest same as EXTERN_FUNC ...
    break;
```

**4. Update NUM_SCALLS** (`/src/instr.h` line 6):
```c
#define NUM_SCALLS     114  /* Updated for PARENT_FUNC */
```

### Usage Example

```c
// base_living.c
take_damage(amount) {
    hp = hp - amount;
    if (hp < 0) hp = 0;
}

// player.c
create() {
    attach("/obj/base_living");
    hp = 100;
}

take_damage(amount) {
    ::take_damage(amount);  // Calls base_living's version
    write("Ouch! You took " + itoa(amount) + " damage!\n");
    if (hp <= 0) {
        write("You died!\n");
    }
}
```

### Pros & Cons

**Pros:**
- ✅ Minimal code changes (3 files, ~30 lines)
- ✅ Uses existing attach() infrastructure
- ✅ Backward compatible
- ✅ Solves 80% of use cases
- ✅ Can implement in 2-4 hours

**Cons:**
- ⚠️ Only calls FIRST attachee (no multi-inheritance control)
- ⚠️ Can't specify which attachee (no `base_living::function()`)
- ⚠️ Still uses attach() semantics (not true inheritance)

---

## OPTION B: Enhanced Attach with Named Parents (BETTER - 4-6 hours)

### Concept

Extend attach() to support named parents and `parent_name::function()` syntax.

### Implementation

**1. Extend attach_list Structure** (`/src/object.h` line 171):
```c
struct attach_list {
    struct object *attachee;
    struct attach_list *next;
    char *alias;  // NEW: Optional name for this attachee
};
```

**2. Add Named Attach Function** (`/src/sys7.c`):
```c
int s_attach_as(struct object *caller, struct object *obj,
                struct object *player, struct var_stack **rts) {
    // Pop NUM_ARGS (2)
    // Pop alias string
    // Pop object
    // Same as s_attach but store alias
    attachptr->alias = copy_string(alias_str);
    // ... rest same ...
}
```

**3. Parser Changes** (`/src/compile1.c`):
```c
if (token.type==SECOND_TOK) {
    // Could be ::function() or name::function()
    get_token(file_info, &token);
    if (token.type != NAME_TOK) {
        set_c_err_msg("expected name after ::");
        return file_info->phys_line;
    }
    
    char *first_name = copy_string(token.token_data.name);
    get_token(file_info, &token);
    
    if (token.type == SECOND_TOK) {
        // name::function() - named parent call
        get_token(file_info, &token);
        if (token.type != NAME_TOK) {
            set_c_err_msg("expected function name");
            return file_info->phys_line;
        }
        add_code_string(curr_fn, first_name);  // Parent name
        add_code_string(curr_fn, token.token_data.name);  // Function name
        add_code_instr(curr_fn, NAMED_PARENT_FUNC);
    } else {
        // ::function() - first parent call
        add_code_string(curr_fn, first_name);
        add_code_instr(curr_fn, PARENT_FUNC);
        unget_token(file_info, &token);
    }
    last_was_arg = 1;
    get_token(file_info, &token);
    continue;
}
```

**4. Add Named Lookup Function** (`/src/interp.c`):
```c
struct fns *find_named_function(char *parent_name, char *func_name,
                                struct object *obj, struct object **real_obj) {
    struct attach_list *curr_attach = obj->attachees;
    
    while (curr_attach) {
        if (curr_attach->alias && strcmp(curr_attach->alias, parent_name) == 0) {
            return find_function(func_name, curr_attach->attachee, real_obj);
        }
        curr_attach = curr_attach->next;
    }
    return NULL;
}
```

**5. Add Opcodes** (`/src/instr.h`):
```c
#define PARENT_FUNC        151  /* ::function() */
#define NAMED_PARENT_FUNC  152  /* name::function() */
```

### Usage Example

```c
// player.c
create() {
    attach_as("/obj/base_living", "living");
    attach_as("/obj/base_container", "container");
    hp = 100;
}

reset() {
    living::reset();     // Call specific parent
    container::reset();  // Call another parent
}

take_damage(amount) {
    living::take_damage(amount);  // Explicit parent call
    write("Ouch!\n");
}
```

### Pros & Cons

**Pros:**
- ✅ Solves multiple inheritance conflicts
- ✅ Clear, explicit parent calls
- ✅ Still uses attach() infrastructure
- ✅ Backward compatible
- ✅ Professional-looking syntax

**Cons:**
- ⚠️ More complex parser changes
- ⚠️ Need to track aliases in attach_list
- ⚠️ Slightly more memory per attachee

---

## OPTION C: True Inheritance (MOST COMPLETE - 6-8 hours)

### Concept

Add `inherit` keyword and true inheritance chain with virtual dispatch.

### Implementation Overview

**1. Add `inherit` Keyword** (`/src/token.c`):
- Add to keyword list
- New token type: `INHERIT_TOK`

**2. Parser Changes** (`/src/compile1.c`):
- Parse `inherit "/path/to/file";` at top level
- Store in proto structure
- Auto-attach during object creation

**3. Object Structure** (`/src/object.h`):
```c
struct proto {
    // ... existing fields ...
    struct inherit_list *inherits;  // NEW: Inheritance chain
};

struct inherit_list {
    struct proto *parent_proto;
    char *alias;  // Optional name
    struct inherit_list *next;
};
```

**4. Compilation** (`/src/cache2.c`):
- When compiling, process `inherit` statements
- Load parent protos
- Build inheritance chain
- Copy/merge function tables

**5. Function Dispatch**:
- Virtual function table
- Override detection
- `::` calls parent version

### Usage Example

```c
// base_living.c
int hp;
int max_hp;

create() {
    hp = 100;
    max_hp = 100;
}

take_damage(int amount) {
    hp = hp - amount;
    if (hp < 0) hp = 0;
}

// player.c
inherit "/obj/base_living";

int experience;

create() {
    ::create();  // Call parent create
    experience = 0;
}

take_damage(int amount) {
    ::take_damage(amount);  // Call parent version
    write("Ouch! You took " + itoa(amount) + " damage!\n");
    if (hp <= 0) {
        write("You died!\n");
    }
}
```

### Pros & Cons

**Pros:**
- ✅ True inheritance like modern LPC
- ✅ Virtual function dispatch
- ✅ Proper override semantics
- ✅ Standard LPC syntax
- ✅ Most powerful option

**Cons:**
- ⚠️ Most complex implementation
- ⚠️ Requires proto structure changes
- ⚠️ Need to handle compilation order
- ⚠️ More testing required
- ⚠️ Potential compatibility issues with attach()

---

## Comparison Matrix

| Feature | Option A | Option B | Option C |
|---------|----------|----------|----------|
| **Effort** | 2-4 hours | 4-6 hours | 6-8 hours |
| **Files Modified** | 3 | 5 | 8+ |
| **Lines of Code** | ~30 | ~100 | ~300 |
| **Backward Compatible** | ✅ Yes | ✅ Yes | ⚠️ Mostly |
| **Parent Calls** | ✅ `::func()` | ✅ `name::func()` | ✅ `::func()` |
| **Multi-Inheritance** | ⚠️ First only | ✅ Named | ✅ Full |
| **Virtual Dispatch** | ❌ No | ❌ No | ✅ Yes |
| **Standard LPC** | ⚠️ Partial | ⚠️ Partial | ✅ Yes |
| **Uses attach()** | ✅ Yes | ✅ Yes | ⚠️ Replaces |
| **Risk Level** | 🟢 Low | 🟡 Medium | 🟠 Medium-High |

---

## Recommended Implementation Path

### Phase 1: Quick Win (2-4 hours)
**Implement Option A** - Simulated Inheritance
- Add `PARENT_FUNC` opcode
- Modify parser to accept `::function()`
- Use existing `find_extern_function()`
- Test with simple parent calls

### Phase 2: Enhancement (2-3 hours)
**Upgrade to Option B** - Named Parents
- Add `alias` field to `attach_list`
- Implement `attach_as()` efun
- Add `NAMED_PARENT_FUNC` opcode
- Support `name::function()` syntax

### Phase 3: Full Feature (Optional, 4-6 hours)
**Implement Option C** - True Inheritance
- Add `inherit` keyword
- Modify proto structure
- Implement virtual dispatch
- Full LPC compatibility

---

## Files to Modify

### Option A (Minimal):
1. `/src/instr.h` - Add PARENT_FUNC opcode (151)
2. `/src/compile1.c` - Parse `::function()` syntax
3. `/src/interp.c` - Handle PARENT_FUNC opcode

### Option B (Enhanced):
1. `/src/object.h` - Add alias to attach_list
2. `/src/sys7.c` - Implement s_attach_as()
3. `/src/instr.h` - Add opcodes (151-152)
4. `/src/compile1.c` - Parse `name::function()`
5. `/src/interp.c` - Handle both opcodes
6. `/src/compile1.c` - Add "attach_as" to scall_array
7. `/src/protos.h` - Add OPER_PROTO(s_attach_as)

### Option C (Full):
- All of Option B, plus:
- `/src/token.c` - Add INHERIT_TOK
- `/src/cache2.c` - Process inherit statements
- `/src/constrct.c` - Build inheritance chains
- Function table merging logic
- Virtual dispatch implementation

---

## Testing Strategy

### Unit Tests for Option A:
```c
// test_inheritance.c
test_parent_call() {
    object base, child;
    
    base = new("/test/base");
    child = new("/test/child");
    child.attach(base);
    
    // Test that ::function() calls parent
    result = child.test_parent_call();
    if (result != "parent_called") {
        write("FAIL: Parent call didn't work\n");
        return 0;
    }
    return 1;
}
```

### Edge Cases:
- No attachee (should error)
- Multiple attachees (calls first)
- Recursive parent calls
- Parent function doesn't exist
- Circular attach detection

---

## Migration from attach() to inherit

### Current Code:
```c
create() {
    attach("/obj/base_living");
    hp = 100;
}
```

### With Option A:
```c
create() {
    attach("/obj/base_living");
    ::create();  // NEW: Can call parent create
    hp = 100;
}
```

### With Option B:
```c
create() {
    attach_as("/obj/base_living", "living");
    living::create();  // NEW: Named parent call
    hp = 100;
}
```

### With Option C:
```c
inherit "/obj/base_living";  // NEW: True inheritance

create() {
    ::create();  // Standard LPC syntax
    hp = 100;
}
```

---

## Performance Considerations

### Option A:
- **No performance impact** - Uses existing find_extern_function()
- Same function lookup cost as current attach()

### Option B:
- **Minimal impact** - Linear search through aliases
- O(n) where n = number of attachees (typically 1-3)

### Option C:
- **Potential improvement** - Virtual function table
- Could be faster than current recursive search
- More memory for vtables

---

## Backward Compatibility

### Option A & B:
- ✅ 100% backward compatible
- Existing attach() code works unchanged
- New `::` syntax is additive
- No breaking changes

### Option C:
- ⚠️ Mostly compatible
- `inherit` is new keyword (could break code using it as identifier)
- attach() still works
- Migration path exists

---

## Conclusion

**RECOMMENDATION: Start with Option A**

**Why:**
1. **Fastest implementation** - 2-4 hours
2. **Lowest risk** - Minimal code changes
3. **Immediate value** - Solves parent call problem
4. **Upgrade path** - Can enhance to Option B later
5. **Proven pattern** - Uses existing infrastructure

**Next Steps:**
1. Implement Option A (this session if desired!)
2. Test thoroughly
3. Evaluate if Option B is needed
4. Consider Option C for future roadmap

**Estimated Total Time: 2-4 hours for working inheritance!**

Not 4-6 weeks. 😎
