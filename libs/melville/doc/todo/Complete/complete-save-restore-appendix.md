# APPENDIX: Leveraging Existing Parser for restore_value()

## Deep Dive: How the Parser Actually Works

### Key Discovery: Tokenizer Reads from String!

**Critical Code** (`/src/token.c` lines 17-18):
```c
#define getch() *((file_info->expanded)++)
#define ungetch() --(file_info->expanded)
```

**CRITICAL INSIGHT**: The tokenizer reads from `file_info->expanded` (a **string pointer**), NOT directly from FILE!

### filptr Structure Analysis

**From `/src/token.h` lines 81-93**:
```c
typedef struct {
    FILE *curr_file;                    // File handle (can be NULL!)
    struct file_stack *previous;
    token_t put_back_token;
    int is_put_back;
    char *expanded;                     // <-- Tokenizer reads from HERE!
    unsigned int phys_line;
    struct code *curr_code;
    sym_tab_t *glob_sym;
    struct define *defs;
    int depth;
} filptr;
```

### How Compilation Actually Works

**Normal Compilation Flow**:
1. `preprocess()` reads line from FILE into buffer
2. Sets `file_info->expanded` to point to buffer
3. `get_token()` reads from `file_info->expanded` via `getch()` macro
4. `parse_exp()` calls `get_token()` and generates bytecode via `add_code_instr()`

**Array Literal Parsing** (`/src/compile1.c` lines 504-537):
```c
if (token.type==LARRASGN_TOK) {  // ({
    elem_count = 0;
    do {
        if (parse_exp(file_info,curr_fn,loc_sym,1,0))  // Parse element
            return file_info->phys_line;
        elem_count++;
        get_token(file_info,&token);
    } while (token.type==COMMA_TOK);
    
    add_code_integer(curr_fn,elem_count);      // Push count
    add_code_instr(curr_fn,S_ARRAY_LITERAL);   // Generate opcode
}
```

**Mapping Literal Parsing** (`/src/compile1.c` lines 538-589):
```c
if (token.type==LMAPSGN_TOK) {  // ([
    pair_count = 0;
    do {
        if (parse_exp(file_info,curr_fn,loc_sym,1,0))  // Parse key
        if (token.type!=COLON_TOK) return error;
        if (parse_exp(file_info,curr_fn,loc_sym,1,0))  // Parse value
        pair_count++;
        get_token(file_info,&token);
    } while (token.type==COMMA_TOK);
    
    add_code_integer(curr_fn,pair_count);        // Push count
    add_code_instr(curr_fn,S_MAPPING_LITERAL);   // Generate opcode
}
```

---

## Strategy: Mock filptr for restore_value()

**The Approach**: Create a minimal `filptr` that points to the saved string, then use existing tokenizer!

### What We Reuse vs. What We Don't

**What We're REUSING** ✅:
- Tokenizer (`get_token()`, `unget_token()`)
- Token types (INTEGER_TOK, STRING_TOK, LARRASGN_TOK, LMAPSGN_TOK, etc.)
- String buffer mechanism (`file_info->expanded`)
- String escape sequence handling (`\n`, `\t`, `\"`, `\\`, etc.)
- Integer parsing (negative numbers, etc.)

**What We're NOT Using** ❌:
- Code generation (`add_code_instr()`, `add_code_integer()`, etc.)
- Symbol tables (`glob_sym`, `loc_sym`)
- Function context (`curr_fn`)
- Preprocessor (`#define`, `#include`)
- File stack (include handling)
- File I/O (`curr_file` can be NULL)

---

## Implementation Plan

### Step 1: Create String-Based filptr Mock

```c
/* In /src/save_restore.c */

/* Create a mock filptr that reads from a string instead of file */
filptr *create_string_filptr(char *str) {
    filptr *fp;
    
    fp = MALLOC(sizeof(filptr));
    if (!fp) return NULL;
    
    /* Initialize required fields */
    fp->curr_file = NULL;              // No file needed!
    fp->previous = NULL;
    fp->expanded = str;                // Point to our string!
    fp->is_put_back = 0;
    fp->phys_line = 1;
    fp->curr_code = NULL;              // Don't need code generation
    fp->glob_sym = NULL;               // Don't need symbol table
    fp->defs = NULL;                   // Don't need #defines
    fp->depth = 0;
    
    return fp;
}

void free_string_filptr(filptr *fp) {
    if (fp) FREE(fp);
}
```

### Step 2: Create Value-Building Parser (NOT Code-Generating)

**Problem**: `parse_exp()` calls `add_code_instr()` which generates bytecode. We need to build VALUES instead!

**Solution**: Create parallel "eval" functions that build values directly:

```c
/* Forward declarations */
int parse_value(filptr *file_info, struct var *result);
int parse_array_value(filptr *file_info, struct var *result);
int parse_mapping_value(filptr *file_info, struct var *result);

/* Parse and BUILD a value (not generate code) */
int parse_value(filptr *file_info, struct var *result) {
    token_t token;
    
    get_token(file_info, &token);
    
    /* Integer literal */
    if (token.type == INTEGER_TOK) {
        result->type = INTEGER;
        result->value.integer = token.token_data.integer;
        return 0;
    }
    
    /* String literal */
    if (token.type == STRING_TOK) {
        result->type = STRING;
        result->value.string = copy_string(token.token_data.name);
        return 0;
    }
    
    /* Array literal: ({...}) */
    if (token.type == LARRASGN_TOK) {
        return parse_array_value(file_info, result);
    }
    
    /* Mapping literal: ([...]) */
    if (token.type == LMAPSGN_TOK) {
        return parse_mapping_value(file_info, result);
    }
    
    /* Unknown token */
    return 1;  // Error
}

/* Parse array literal and build heap_array directly */
int parse_array_value(filptr *file_info, struct var *result) {
    token_t token;
    struct heap_array *arr;
    struct var elem;
    int elem_count = 0;
    struct var *temp_elements;
    int capacity = 16;
    int i;
    
    /* Allocate temporary storage for elements */
    temp_elements = MALLOC(sizeof(struct var) * capacity);
    if (!temp_elements) return 1;
    
    get_token(file_info, &token);
    
    /* Empty array: ({}) */
    if (token.type == RARRASGN_TOK) {
        arr = array_create(0, UNLIMITED_ARRAY_SIZE);
        if (!arr) {
            FREE(temp_elements);
            return 1;
        }
        result->type = ARRAY;
        result->value.array_ptr = arr;
        FREE(temp_elements);
        return 0;
    }
    
    /* Parse elements */
    unget_token(file_info, &token);
    do {
        /* Grow temp storage if needed */
        if (elem_count >= capacity) {
            capacity *= 2;
            temp_elements = REALLOC(temp_elements, sizeof(struct var) * capacity);
            if (!temp_elements) return 1;
        }
        
        /* Parse element recursively */
        if (parse_value(file_info, &elem)) {
            /* Clean up on error */
            for (i = 0; i < elem_count; i++) {
                clear_var(&temp_elements[i]);
            }
            FREE(temp_elements);
            return 1;
        }
        
        temp_elements[elem_count++] = elem;
        
        get_token(file_info, &token);
    } while (token.type == COMMA_TOK);
    
    if (token.type != RARRASGN_TOK) {
        /* Clean up on error */
        for (i = 0; i < elem_count; i++) {
            clear_var(&temp_elements[i]);
        }
        FREE(temp_elements);
        return 1;  // Expected })
    }
    
    /* Create heap array and copy elements */
    arr = array_create(elem_count, UNLIMITED_ARRAY_SIZE);
    if (!arr) {
        for (i = 0; i < elem_count; i++) {
            clear_var(&temp_elements[i]);
        }
        FREE(temp_elements);
        return 1;
    }
    
    for (i = 0; i < elem_count; i++) {
        arr->elements[i] = temp_elements[i];
    }
    
    FREE(temp_elements);
    
    result->type = ARRAY;
    result->value.array_ptr = arr;
    return 0;
}

/* Parse mapping literal and build heap_mapping directly */
int parse_mapping_value(filptr *file_info, struct var *result) {
    token_t token;
    struct heap_mapping *map;
    struct var key, value;
    
    /* Create empty mapping */
    map = mapping_create();
    if (!map) return 1;
    
    get_token(file_info, &token);
    
    /* Empty mapping: ([]) */
    if (token.type == RARRAY_TOK) {
        get_token(file_info, &token);  // Consume )
        if (token.type != RPAR_TOK) {
            mapping_free(map);
            return 1;
        }
        result->type = MAPPING;
        result->value.mapping_ptr = map;
        return 0;
    }
    
    /* Parse key:value pairs */
    unget_token(file_info, &token);
    do {
        /* Parse key */
        if (parse_value(file_info, &key)) {
            mapping_free(map);
            return 1;
        }
        
        /* Expect colon */
        get_token(file_info, &token);
        if (token.type != COLON_TOK) {
            clear_var(&key);
            mapping_free(map);
            return 1;
        }
        
        /* Parse value */
        if (parse_value(file_info, &value)) {
            clear_var(&key);
            mapping_free(map);
            return 1;
        }
        
        /* Insert into mapping */
        mapping_insert(map, &key, &value);
        
        /* Clean up key/value (mapping made copies) */
        clear_var(&key);
        clear_var(&value);
        
        get_token(file_info, &token);
    } while (token.type == COMMA_TOK);
    
    if (token.type != RARRAY_TOK) {
        mapping_free(map);
        return 1;  // Expected ]
    }
    
    get_token(file_info, &token);  // Consume )
    if (token.type != RPAR_TOK) {
        mapping_free(map);
        return 1;
    }
    
    result->type = MAPPING;
    result->value.mapping_ptr = map;
    return 0;
}
```

### Step 3: Implement restore_value() EFUN

```c
/* EFUN: restore_value(string saved_data) -> mixed */
int s_restore_value(struct object *caller, struct object *obj,
                    struct object *player, struct var_stack **rts) {
    struct var tmp, result;
    filptr *fp;
    char *saved_str;
    
    /* Pop NUM_ARGS */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != NUM_ARGS || tmp.value.num != 1) {
        clear_var(&tmp);
        return 1;
    }
    
    /* Pop string argument */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != STRING) {
        clear_var(&tmp);
        return 1;
    }
    saved_str = tmp.value.string;
    
    /* Create mock filptr pointing to string */
    fp = create_string_filptr(saved_str);
    if (!fp) {
        clear_var(&tmp);
        /* Return 0 on error */
        result.type = INTEGER;
        result.value.integer = 0;
        push(&result, rts);
        return 0;
    }
    
    /* Parse the value using existing tokenizer! */
    if (parse_value(fp, &result)) {
        free_string_filptr(fp);
        clear_var(&tmp);
        
        /* Return 0 on parse error */
        result.type = INTEGER;
        result.value.integer = 0;
        push(&result, rts);
        return 0;
    }
    
    /* Clean up */
    free_string_filptr(fp);
    clear_var(&tmp);
    
    /* Push result */
    push(&result, rts);
    return 0;
}
```

---

## Why This Approach Works

### Advantages

1. ✅ **Reuses existing tokenizer** - `get_token()` already handles all token types
2. ✅ **Reuses string parsing** - String escape sequences (`\n`, `\t`, `\"`, `\\`) already work
3. ✅ **Reuses integer parsing** - Handles negative numbers, hex, etc.
4. ✅ **Minimal new code** - Only need value-building functions, not tokenization
5. ✅ **Handles nested structures** - Recursive `parse_value()` handles any depth
6. ✅ **No file I/O needed** - `curr_file` can be NULL, tokenizer uses `expanded` pointer
7. ✅ **Battle-tested** - Tokenizer is already used for all LPC compilation
8. ✅ **Consistent syntax** - Saved format matches LPC literal syntax exactly

### Example Usage

```c
/* Save a complex structure */
mapping data = ([
    "name": "player",
    "level": 10,
    "stats": (["hp": 100, "mp": 50]),
    "inventory": ({"sword", "shield", "potion"})
]);

string saved = save_value(data);
// Result: (["name":"player","level":10,"stats":(["hp":100,"mp":50]),"inventory":({"sword","shield","potion"})])

/* Later, restore it */
mapping restored = restore_value(saved);
// restored is identical to original data!
```

---

## Implementation Effort

### Revised Estimate: 10-14 hours (down from 16-24)

**Breakdown**:

1. **save_value() implementation**: 3-4 hours
   - Recursive serialization with `typeof()`
   - String escaping
   - Type detection
   
2. **Mock filptr infrastructure**: 1-2 hours
   - `create_string_filptr()`
   - `free_string_filptr()`
   - Minimal initialization
   
3. **Value-building parser**: 4-6 hours
   - `parse_value()` - main dispatcher
   - `parse_array_value()` - array builder
   - `parse_mapping_value()` - mapping builder
   - Error handling
   - Memory management
   
4. **Integration & testing**: 2-3 hours
   - Add to efun table in `/src/compile1.c`
   - Add to interpreter dispatch in `/src/interp.c`
   - Add opcodes to `/src/instr.h`
   - Test nested structures
   - Test edge cases
   - Test error conditions

### Why Less Effort Than Originally Estimated

- ✅ Don't need to write tokenizer from scratch (already exists!)
- ✅ Don't need to handle escape sequences (already works!)
- ✅ Don't need to parse operators, variables, function calls, etc.
- ✅ Only need to handle data literals (int, string, array, mapping)
- ✅ Tokenizer is already battle-tested and handles edge cases

---

## Testing Plan

### Unit Tests

```c
/* Test basic types */
assert(restore_value(save_value(42)) == 42);
assert(restore_value(save_value("hello")) == "hello");
assert(restore_value(save_value("with\nnewline")) == "with\nnewline");

/* Test arrays */
assert(restore_value(save_value(({1,2,3}))) == ({1,2,3}));
assert(restore_value(save_value(({}))) == ({}));

/* Test mappings */
assert(restore_value(save_value((["a":1]))) == (["a":1]));
assert(restore_value(save_value(([:]))) == ([:]))

/* Test nested structures */
mixed nested = (["arr": ({1,2,3}), "map": (["x":10])]);
assert(restore_value(save_value(nested)) == nested);

/* Test error handling */
assert(restore_value("invalid syntax") == 0);
assert(restore_value("({1,2,") == 0);  // Incomplete
```

---

## Comparison to Writing from Scratch

### If We Wrote a New Parser

**Would Need**:
- Character-by-character tokenization
- String escape sequence handling (`\n`, `\t`, `\"`, `\\`, etc.)
- Integer parsing (negative, hex, octal)
- Token buffering (lookahead)
- Error recovery
- Testing all edge cases

**Estimated Effort**: 20-30 hours

### By Reusing Existing Parser

**Only Need**:
- Mock filptr (1-2 hours)
- Value-building functions (4-6 hours)
- Integration (2-3 hours)

**Estimated Effort**: 10-14 hours

**Savings**: 10-16 hours (40-50% reduction!)

---

## Conclusion

By leveraging the existing tokenizer and parser infrastructure, we can implement `restore_value()` with significantly less effort while maintaining consistency with LPC syntax and reusing battle-tested code.

The key insight is that `file_info->expanded` is just a string pointer, so we can point it at our saved data string and use the existing tokenizer without any file I/O!
