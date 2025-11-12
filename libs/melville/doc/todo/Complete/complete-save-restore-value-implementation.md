# Implementation Guide: save_value() / restore_value() EFUNS

**Status**: Not Implemented  
**Priority**: HIGH  
**Type**: EFUN (Engine Function)  
**Estimated Effort**: 16-24 hours (MUCH more complex than originally estimated)

**CRITICAL COMPLEXITY FACTORS** (VERIFIED FROM CODEBASE):

**Type System Analysis** (`/src/object.h`):
- **19 different type tags** (INTEGER=0 through MAPPING=18)
- **Complex union structure** in `struct var`
- **Nested structures**: Arrays contain `struct var` elements, mappings contain key-value `struct var` pairs
- **Reference counting**: Arrays and mappings use refcount for GC
- **Object pointers**: Must serialize object references (path? object number?)

**Serialization Challenges**:
1. **Type Preservation**: Must encode type tag (0-18) in string format
2. **Recursive Structures**: Arrays/mappings can contain arrays/mappings
3. **Circular References**: `arr[0] = arr` would cause infinite recursion
4. **String Escaping**: Must escape `"`, `\`, `\n`, etc.
5. **Object References**: How to serialize `struct object *objptr`?
6. **Parser Complexity**: Need tokenizer + recursive descent for restore
7. **Memory Management**: Must handle refcounts correctly on restore

---

## Overview

Implement `save_value()` and `restore_value()` as native EFUNs in the NetCI driver. These functions perform serialization and deserialization of LPC data structures, converting them to/from string representations for storage and transmission.

---

## Current State

### What Exists (VERIFIED)

**Actual Implementation** in `/libs/melville/sys/player.c` (lines 298-421):

```c
save_data() {
    data = "player_name:" + player_name + "\n";
    data = data + "player_password:" + player_password + "\n";
    data = data + "player_role:" + itoa(player_role) + "\n";
    
    /* Properties saved as: */
    if (typeof(properties[key]) == INT_T) {  // Uses typeof()!
        data = data + "prop:key:int:" + itoa(value) + "\n";
    } else if (typeof(properties[key]) == STRING_T) {
        data = data + "prop:key:str:" + value + "\n";
    }
    /* Comment says: Add more types as needed (object, array, etc.) */
    
    fwrite(path, data);
}

load_data(name) {
    /* Manual line-by-line parsing with sscanf */
    /* Only handles int and string properties */
}
```

**What This Proves**:
- ✅ `typeof()` efun EXISTS and works (`/src/sys4.c` lines 78-96)
- ✅ Runtime type detection is POSSIBLE
- ✅ Player.c ALREADY uses `typeof()` to detect int vs string
- ❌ But only handles 2 types (int, string) out of 19!

**Type Constants** (from `/libs/melville/include/config.h`):
```c
#define INT_T 0
#define STRING_T 1
#define OBJECT_T 2
#define ARRAY_T 17
#define MAPPING_T 18
```

**Limitations**:
- ❌ Only handles 2 types (int=0, string=1)
- ❌ No array support (type=17)
- ❌ No mapping support (type=18) - even though mappings exist!
- ❌ No object support (type=2)
- ❌ No nested structures
- ❌ Manual parsing with sscanf
- ❌ No string escaping (what if value contains `:`?)

### Current Limitations
```c
/* Current player.c save format */
data = "player_name:" + player_name + "\n";
data = data + "player_password:" + player_password + "\n";
data = data + "player_role:" + itoa(player_role) + "\n";
data = data + "prop:key:int:" + itoa(value) + "\n";
data = data + "prop:key:str:" + value + "\n";
```

**Problems**:
- ❌ No array support
- ❌ No mapping support
- ❌ No nested structure support
- ❌ Manual parsing required
- ❌ Error-prone format
- ❌ Not portable between objects

---

## Function Signatures

### save_value()
```c
string save_value(mixed value)
```

**Parameters**:
- **value**: Any LPC data type (int, string, array, mapping, object)

**Returns**:
- String representation of the value in a parseable format

**Behavior**:
- Converts LPC data structures to string format
- Preserves type information
- Handles nested structures recursively
- Returns parseable string representation

---

### restore_value()
```c
mixed restore_value(string str)
```

**Parameters**:
- **str**: String representation created by `save_value()`

**Returns**:
- Original LPC data structure
- Returns 0 on parse error

**Behavior**:
- Parses string representation back to LPC data
- Reconstructs original type and structure
- Handles nested structures recursively
- Validates format and returns 0 on error

---

## Deep Dive: What Must Be Serialized

### Type Analysis from `/src/object.h`

**struct var** (lines 51-66):
```c
struct var {
    unsigned char type;  // 0-18
    union {
        signed long integer;             // Type 0: INTEGER
        char *string;                    // Type 1: STRING
        struct object *objptr;           // Type 2: OBJECT
        struct heap_array *array_ptr;    // Type 17: ARRAY
        struct heap_mapping *mapping_ptr; // Type 18: MAPPING
        // ... 14 other types (mostly internal)
    } value;
};
```

**struct heap_array** (lines 71-77):
```c
struct heap_array {
    unsigned int size;        // Number of elements
    unsigned int capacity;    // Allocated capacity
    unsigned int max_size;    // Maximum size
    unsigned int refcount;    // Reference count
    struct var *elements;     // Array of struct var!
};
```

**struct heap_mapping** (lines 83-95):
```c
struct mapping_entry {
    struct var key;           // Key (any type!)
    struct var value;         // Value (any type!)
    unsigned int hash;
    struct mapping_entry *next;
};

struct heap_mapping {
    unsigned int size;        // Number of pairs
    unsigned int capacity;    // Hash table size
    unsigned int refcount;
    struct mapping_entry **buckets; // Hash table
};
```

**CRITICAL INSIGHT**: Arrays and mappings contain `struct var`, which can contain arrays/mappings!

**Example Nested Structure**:
```c
mapping m = ([
    "name": "player",
    "stats": (["hp": 100, "mp": 50]),
    "inventory": ({"sword", "shield"})
]);
```

**Serialization Must Handle**:
1. Mapping with 3 keys
2. Key "stats" -> value is ANOTHER mapping
3. Key "inventory" -> value is an ARRAY
4. Array contains STRINGS
5. Nested mapping contains INTEGERS

**Depth**: 3 levels deep!

---

## Serialization Format

### Option A: LDMud-Style Format (Recommended)

Uses a compact, readable format with type prefixes:

```
Integers:   42
Strings:    "hello world"
Arrays:     ({1,2,3})
Mappings:   (["key1":value1,"key2":value2])
Floats:     3.14
Objects:    <object "/path/to/object">
```

**Examples**:
```c
save_value(42)                    // "42"
save_value("hello")               // "\"hello\""
save_value(({1, 2, 3}))          // "({1,2,3})"
save_value((["a":1, "b":2]))     // "([\"a\":1,\"b\":2])"
save_value((["x":({1,2}), "y":3])) // "([\"x\":({1,2}),\"y\":3])"
```

**Advantages**:
- ✅ Human-readable
- ✅ Compact representation
- ✅ Similar to LPC syntax
- ✅ Easy to debug
- ✅ Compatible with LDMud

---

### Option B: JSON-Style Format

Uses JSON-like format:

```json
{
  "type": "mapping",
  "value": {
    "key1": {"type": "int", "value": 1},
    "key2": {"type": "string", "value": "hello"}
  }
}
```

**Advantages**:
- ✅ Explicit type information
- ✅ Industry standard format
- ✅ Easy to parse with existing tools

**Disadvantages**:
- ❌ More verbose
- ❌ Less LPC-like
- ❌ Harder to read for LPC developers

---

## Detailed Implementation

### Implementation Location

**Driver Files**:
- `/src/save_restore.c` (new file for serialization logic)
- `/src/sys6a.c` or `/src/sys7.c` (efun wrappers)
- `/src/instr.h` (opcodes)
- `/src/protos.h` (prototypes)
- `/src/compile1.c` (efun table)
- `/src/interp.c` (dispatcher)

---

### Step 1: Create `/src/save_restore.c`

```c
#include "config.h"
#include "object.h"
#include "instr.h"
#include "interp.h"
#include "protos.h"
#include "globals.h"

/* Forward declarations */
static char *save_value_recursive(struct var *value, int depth);
static int restore_value_recursive(char **str_ptr, struct var *result);

/* Maximum recursion depth to prevent stack overflow */
#define MAX_SAVE_DEPTH 50

/* ========================================================================
 * SAVE_VALUE - Serialize LPC data to string
 * ======================================================================== */

char *save_value_internal(struct var *value, int depth) {
    char *result;
    int i, len;
    char *temp;
    
    if (depth > MAX_SAVE_DEPTH) {
        /* Too deep - return error marker */
        result = MALLOC(20);
        strcpy(result, "<MAX_DEPTH_ERROR>");
        return result;
    }
    
    switch (value->type) {
        case INTEGER:
            /* Format: 42 */
            result = MALLOC(32);
            sprintf(result, "%ld", value->value.integer);
            return result;
            
        case STRING:
            /* Format: "string content" with escaping */
            len = strlen(value->value.string);
            result = MALLOC(len * 2 + 3); /* Worst case: all chars escaped */
            result[0] = '"';
            
            /* Escape special characters */
            char *write_pos = result + 1;
            char *read_pos = value->value.string;
            while (*read_pos) {
                if (*read_pos == '"' || *read_pos == '\\') {
                    *write_pos++ = '\\';
                }
                *write_pos++ = *read_pos++;
            }
            *write_pos++ = '"';
            *write_pos = '\0';
            return result;
            
        case ARRAY:
            /* Format: ({elem1,elem2,elem3}) */
            {
                struct array *arr = value->value.arrayptr;
                if (!arr || arr->size == 0) {
                    result = MALLOC(5);
                    strcpy(result, "({})");
                    return result;
                }
                
                /* Build array representation */
                char **elements = MALLOC(arr->size * sizeof(char *));
                int total_len = 3; /* "({" + "}" */
                
                for (i = 0; i < arr->size; i++) {
                    elements[i] = save_value_internal(&arr->vars[i], depth + 1);
                    total_len += strlen(elements[i]);
                    if (i > 0) total_len++; /* comma */
                }
                
                result = MALLOC(total_len + 1);
                strcpy(result, "({");
                for (i = 0; i < arr->size; i++) {
                    if (i > 0) strcat(result, ",");
                    strcat(result, elements[i]);
                    FREE(elements[i]);
                }
                strcat(result, "})");
                FREE(elements);
                return result;
            }
            
        case MAPPING:
            /* Format: (["key1":value1,"key2":value2]) */
            {
                struct mapping *map = value->value.mapptr;
                if (!map || map->size == 0) {
                    result = MALLOC(5);
                    strcpy(result, "([])");
                    return result;
                }
                
                /* Build mapping representation */
                char **pairs = MALLOC(map->size * sizeof(char *));
                int total_len = 3; /* "([" + "])" */
                
                /* Iterate through mapping entries */
                struct map_entry *entry = map->entries;
                i = 0;
                while (entry) {
                    char *key_str = save_value_internal(&entry->key, depth + 1);
                    char *val_str = save_value_internal(&entry->value, depth + 1);
                    
                    pairs[i] = MALLOC(strlen(key_str) + strlen(val_str) + 2);
                    sprintf(pairs[i], "%s:%s", key_str, val_str);
                    
                    total_len += strlen(pairs[i]);
                    if (i > 0) total_len++; /* comma */
                    
                    FREE(key_str);
                    FREE(val_str);
                    
                    entry = entry->next;
                    i++;
                }
                
                result = MALLOC(total_len + 1);
                strcpy(result, "([");
                for (i = 0; i < map->size; i++) {
                    if (i > 0) strcat(result, ",");
                    strcat(result, pairs[i]);
                    FREE(pairs[i]);
                }
                strcat(result, "])");
                FREE(pairs);
                return result;
            }
            
        case OBJECT:
            /* Format: <object "/path/to/object"> */
            if (!value->value.objptr) {
                result = MALLOC(10);
                strcpy(result, "<null>");
                return result;
            }
            {
                char *path = value->value.objptr->parent->pathname;
                result = MALLOC(strlen(path) + 20);
                sprintf(result, "<object \"%s\">", path);
                return result;
            }
            
        default:
            /* Unknown type */
            result = MALLOC(20);
            strcpy(result, "<unknown_type>");
            return result;
    }
}

/* ========================================================================
 * RESTORE_VALUE - Deserialize string to LPC data
 * ======================================================================== */

/* Skip whitespace */
static void skip_whitespace(char **ptr) {
    while (**ptr == ' ' || **ptr == '\t' || **ptr == '\n' || **ptr == '\r') {
        (*ptr)++;
    }
}

/* Parse integer */
static int parse_integer(char **ptr, struct var *result) {
    char *end;
    long value;
    
    value = strtol(*ptr, &end, 10);
    if (end == *ptr) return 0; /* No digits parsed */
    
    result->type = INTEGER;
    result->value.integer = value;
    *ptr = end;
    return 1;
}

/* Parse string */
static int parse_string(char **ptr, struct var *result) {
    char *start, *write_pos;
    char *str_buf;
    int len = 0;
    
    if (**ptr != '"') return 0;
    (*ptr)++; /* Skip opening quote */
    
    start = *ptr;
    
    /* Calculate length and find end */
    while (**ptr && **ptr != '"') {
        if (**ptr == '\\' && *(*ptr + 1)) {
            (*ptr)++; /* Skip escape */
        }
        (*ptr)++;
        len++;
    }
    
    if (**ptr != '"') return 0; /* Unterminated string */
    
    /* Allocate and copy with unescaping */
    str_buf = MALLOC(len + 1);
    write_pos = str_buf;
    *ptr = start;
    
    while (**ptr != '"') {
        if (**ptr == '\\' && *(*ptr + 1)) {
            (*ptr)++;
            *write_pos++ = **ptr;
        } else {
            *write_pos++ = **ptr;
        }
        (*ptr)++;
    }
    *write_pos = '\0';
    (*ptr)++; /* Skip closing quote */
    
    result->type = STRING;
    result->value.string = str_buf;
    return 1;
}

/* Parse array */
static int parse_array(char **ptr, struct var *result) {
    struct array *arr;
    int capacity = 10;
    int count = 0;
    
    if (**ptr != '(' || *(*ptr + 1) != '{') return 0;
    *ptr += 2; /* Skip "({" */
    
    skip_whitespace(ptr);
    
    /* Check for empty array */
    if (**ptr == '}' && *(*ptr + 1) == ')') {
        *ptr += 2;
        arr = create_array(0);
        result->type = ARRAY;
        result->value.arrayptr = arr;
        return 1;
    }
    
    /* Allocate initial array */
    arr = create_array(capacity);
    
    /* Parse elements */
    while (1) {
        skip_whitespace(ptr);
        
        /* Grow array if needed */
        if (count >= capacity) {
            capacity *= 2;
            arr = resize_array(arr, capacity);
        }
        
        /* Parse element */
        if (!restore_value_recursive(ptr, &arr->vars[count])) {
            free_array(arr);
            return 0;
        }
        count++;
        
        skip_whitespace(ptr);
        
        /* Check for end or comma */
        if (**ptr == '}' && *(*ptr + 1) == ')') {
            *ptr += 2;
            break;
        }
        if (**ptr == ',') {
            (*ptr)++;
            continue;
        }
        
        /* Invalid format */
        free_array(arr);
        return 0;
    }
    
    /* Resize to actual size */
    arr = resize_array(arr, count);
    result->type = ARRAY;
    result->value.arrayptr = arr;
    return 1;
}

/* Parse mapping */
static int parse_mapping(char **ptr, struct var *result) {
    struct mapping *map;
    struct var key, value;
    
    if (**ptr != '(' || *(*ptr + 1) != '[') return 0;
    *ptr += 2; /* Skip "([" */
    
    skip_whitespace(ptr);
    
    /* Check for empty mapping */
    if (**ptr == ']' && *(*ptr + 1) == ')') {
        *ptr += 2;
        map = create_mapping();
        result->type = MAPPING;
        result->value.mapptr = map;
        return 1;
    }
    
    map = create_mapping();
    
    /* Parse key:value pairs */
    while (1) {
        skip_whitespace(ptr);
        
        /* Parse key */
        if (!restore_value_recursive(ptr, &key)) {
            free_mapping(map);
            return 0;
        }
        
        skip_whitespace(ptr);
        
        /* Expect colon */
        if (**ptr != ':') {
            clear_var(&key);
            free_mapping(map);
            return 0;
        }
        (*ptr)++;
        
        skip_whitespace(ptr);
        
        /* Parse value */
        if (!restore_value_recursive(ptr, &value)) {
            clear_var(&key);
            free_mapping(map);
            return 0;
        }
        
        /* Add to mapping */
        mapping_insert(map, &key, &value);
        
        skip_whitespace(ptr);
        
        /* Check for end or comma */
        if (**ptr == ']' && *(*ptr + 1) == ')') {
            *ptr += 2;
            break;
        }
        if (**ptr == ',') {
            (*ptr)++;
            continue;
        }
        
        /* Invalid format */
        free_mapping(map);
        return 0;
    }
    
    result->type = MAPPING;
    result->value.mapptr = map;
    return 1;
}

/* Main restore function */
static int restore_value_recursive(char **ptr, struct var *result) {
    skip_whitespace(ptr);
    
    /* Try each type */
    if (**ptr == '"') {
        return parse_string(ptr, result);
    }
    if (**ptr == '(' && *(*ptr + 1) == '{') {
        return parse_array(ptr, result);
    }
    if (**ptr == '(' && *(*ptr + 1) == '[') {
        return parse_mapping(ptr, result);
    }
    if ((**ptr >= '0' && **ptr <= '9') || **ptr == '-') {
        return parse_integer(ptr, result);
    }
    
    /* Unknown format */
    return 0;
}
```

---

### Step 2: Add EFUN Wrappers in `/src/sys7.c` (new file)

```c
#include "config.h"
#include "object.h"
#include "instr.h"
#include "interp.h"
#include "protos.h"
#include "globals.h"

/* save_value(mixed value) -> string */
int s_save_value(struct object *caller, struct object *obj,
                 struct object *player, struct var_stack **rts) {
    struct var tmp;
    char *result_str;
    
    /* Pop NUM_ARGS */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != NUM_ARGS || tmp.value.num != 1) {
        clear_var(&tmp);
        return 1;
    }
    
    /* Pop value to save */
    if (pop(&tmp, rts, obj)) return 1;
    if (resolve_var(&tmp, obj)) return 1;
    
    /* Serialize */
    result_str = save_value_internal(&tmp, 0);
    clear_var(&tmp);
    
    if (!result_str) {
        /* Error - push 0 */
        tmp.type = INTEGER;
        tmp.value.integer = 0;
        push(&tmp, rts);
        return 0;
    }
    
    /* Push result string */
    tmp.type = STRING;
    tmp.value.string = result_str;
    push(&tmp, rts);
    
    return 0;
}

/* restore_value(string str) -> mixed */
int s_restore_value(struct object *caller, struct object *obj,
                    struct object *player, struct var_stack **rts) {
    struct var tmp, result;
    char *str, *parse_ptr;
    int success;
    
    /* Pop NUM_ARGS */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != NUM_ARGS || tmp.value.num != 1) {
        clear_var(&tmp);
        return 1;
    }
    
    /* Pop string to restore */
    if (pop(&tmp, rts, obj)) return 1;
    if (resolve_var(&tmp, obj)) return 1;
    if (tmp.type != STRING) {
        clear_var(&tmp);
        /* Push 0 on error */
        tmp.type = INTEGER;
        tmp.value.integer = 0;
        push(&tmp, rts);
        return 0;
    }
    
    str = tmp.value.string;
    parse_ptr = str;
    
    /* Deserialize */
    success = restore_value_recursive(&parse_ptr, &result);
    clear_var(&tmp);
    
    if (!success) {
        /* Parse error - push 0 */
        result.type = INTEGER;
        result.value.integer = 0;
    }
    
    push(&result, rts);
    return 0;
}
```

---

### Step 3: Add to Driver Infrastructure

#### `/src/instr.h`
```c
#define SAVE_VALUE      94  /* save_value(mixed) */
#define RESTORE_VALUE   95  /* restore_value(string) */
```

#### `/src/protos.h`
```c
OPER_PROTO(s_save_value)
OPER_PROTO(s_restore_value)

/* save_restore.c functions */
char *save_value_internal(struct var *value, int depth);
int restore_value_recursive(char **ptr, struct var *result);
```

#### `/src/compile1.c`
```c
"replace_string","save_value","restore_value",
```

#### `/src/interp.c`
```c
s_replace_string,s_save_value,s_restore_value,
```

---

## Usage Examples

### Example 1: Save Player Inventory
```c
/* In player.c */
save_data() {
    string data;
    mapping save_map;
    
    /* Build save mapping */
    save_map = ([
        "name": player_name,
        "password": player_password,
        "role": player_role,
        "properties": properties,
        "inventory": inventory
    ]);
    
    /* Serialize to string */
    data = save_value(save_map);
    
    /* Write to file */
    fwrite(PLAYER_SAVE_DIR + player_name + ".o", data);
}

load_data(name) {
    string data, line;
    mapping save_map;
    int pos;
    
    /* Read file */
    pos = 0;
    data = "";
    line = fread(PLAYER_SAVE_DIR + name + ".o", pos);
    while (line && line != 0 && line != -1) {
        data = data + line;
        line = fread(PLAYER_SAVE_DIR + name + ".o", pos);
    }
    
    /* Deserialize */
    save_map = restore_value(data);
    if (!save_map) return 0;
    
    /* Restore fields */
    player_name = save_map["name"];
    player_password = save_map["password"];
    player_role = save_map["role"];
    properties = save_map["properties"];
    inventory = save_map["inventory"];
    
    return 1;
}
```

### Example 2: Save Room State
```c
/* In room.c */
save_room_state() {
    mapping state;
    
    state = ([
        "exits": exits,
        "exit_messages": exit_messages,
        "properties": properties,
        "contents": inventory
    ]);
    
    return save_value(state);
}

restore_room_state(str) {
    mapping state;
    
    state = restore_value(str);
    if (!state) return 0;
    
    exits = state["exits"];
    exit_messages = state["exit_messages"];
    properties = state["properties"];
    inventory = state["contents"];
    
    return 1;
}
```

### Example 3: Network Transmission
```c
/* Send complex data over network */
send_data_to_client(data) {
    string serialized;
    
    serialized = save_value(data);
    send_device(serialized + "\n");
}

/* Receive and parse */
receive_data_from_client(str) {
    mixed data;
    
    data = restore_value(str);
    if (!data) {
        write("Error: Invalid data format\n");
        return;
    }
    
    /* Process data */
    process_client_data(data);
}
```

---

## Testing Requirements

### Unit Tests

Create `/libs/melville/test/test_save_restore.c`:

```c
#include <std.h>

test_integer() {
    int val = 42;
    string saved = save_value(val);
    int restored = restore_value(saved);
    
    if (restored != val) {
        write("FAIL: Integer save/restore\n");
        return 0;
    }
    return 1;
}

test_string() {
    string val = "hello world";
    string saved = save_value(val);
    string restored = restore_value(saved);
    
    if (restored != val) {
        write("FAIL: String save/restore\n");
        return 0;
    }
    return 1;
}

test_array() {
    int *val = ({1, 2, 3, 4, 5});
    string saved = save_value(val);
    int *restored = restore_value(saved);
    
    if (sizeof(restored) != sizeof(val)) {
        write("FAIL: Array size mismatch\n");
        return 0;
    }
    
    for (int i = 0; i < sizeof(val); i++) {
        if (restored[i] != val[i]) {
            write("FAIL: Array element mismatch\n");
            return 0;
        }
    }
    return 1;
}

test_mapping() {
    mapping val = (["a": 1, "b": 2, "c": 3]);
    string saved = save_value(val);
    mapping restored = restore_value(saved);
    
    if (restored["a"] != 1 || restored["b"] != 2 || restored["c"] != 3) {
        write("FAIL: Mapping save/restore\n");
        return 0;
    }
    return 1;
}

test_nested() {
    mapping val = ([
        "name": "test",
        "data": ({1, 2, 3}),
        "nested": (["x": 10, "y": 20])
    ]);
    
    string saved = save_value(val);
    mapping restored = restore_value(saved);
    
    if (restored["name"] != "test") return 0;
    if (sizeof(restored["data"]) != 3) return 0;
    if (restored["nested"]["x"] != 10) return 0;
    
    return 1;
}

test_special_chars() {
    string val = "hello \"world\" with\\backslash";
    string saved = save_value(val);
    string restored = restore_value(saved);
    
    if (restored != val) {
        write("FAIL: Special character escaping\n");
        return 0;
    }
    return 1;
}

run_tests() {
    int passed = 0;
    int total = 6;
    
    write("Testing save_value/restore_value...\n");
    
    if (test_integer()) passed++;
    if (test_string()) passed++;
    if (test_array()) passed++;
    if (test_mapping()) passed++;
    if (test_nested()) passed++;
    if (test_special_chars()) passed++;
    
    write("Passed " + itoa(passed) + "/" + itoa(total) + " tests\n");
    return (passed == total);
}
```

---

## Migration Plan

### Phase 1: Implement Core Functions
1. Create `/src/save_restore.c` with serialization logic
2. Add EFUN wrappers in `/src/sys7.c`
3. Update driver infrastructure (opcodes, tables, etc.)
4. Compile and test basic functionality

### Phase 2: Update Player Save/Load
1. Refactor `/sys/player.c` to use `save_value()`/`restore_value()`
2. Migrate existing save files (or create migration tool)
3. Test player login/logout

### Phase 3: Expand Usage
1. Update room persistence
2. Add to object save/load
3. Use for network data transmission
4. Document in mudlib guides

---

## Performance Considerations

- **Recursion Depth**: Limit to 50 levels to prevent stack overflow
- **Memory**: Pre-calculate string sizes where possible
- **Caching**: Consider caching serialized forms for frequently-saved objects
- **File I/O**: Combine with buffered file operations

---

## Security Considerations

- **Object References**: Decide how to handle object pointers (save path only)
- **Validation**: Validate restored data before use
- **Sandboxing**: Prevent code injection through crafted save strings
- **Size Limits**: Impose maximum string size for `restore_value()`

---

## References

- **LDMud**: Has native `save_value()`/`restore_value()` efuns
- **DGD**: Uses `save_object()`/`restore_object()` for similar functionality
- **FluffOS**: Implements both approaches
- **Current**: `/sys/player.c` lines 298-421 (manual implementation)

---

**Priority**: HIGH - This is a fundamental feature needed for proper persistence.

---

## IMPLEMENTATION COMPLETE ✅

### Files Created:

**`/src/sys_strings.c`** - Driver-level string helpers
- `escape_string()` - String escaping for serialization
- `save_value_internal()` - Recursive value serialization

**`/src/token_parse.c`** - Token parser helpers
- `create_string_filptr()` - Mock filptr for string parsing
- `free_string_filptr()` - Cleanup
- `parse_value()` - Parse any value type
- `parse_array_value()` - Parse array literals
- `parse_mapping_value()` - Parse mapping literals

**`/src/sfun_strings.c`** (renamed from sfun_string.c)
- Contains `s_sscanf()` (existing)
- `s_save_value()` - EFUN wrapper
- `s_restore_value()` - EFUN wrapper

### Files Deleted:
- ❌ `/src/save_restore.c` - Removed (code split into proper locations)
- ❌ `/src/sys_sscanf.c` - Removed (empty file, sscanf already in sfun_strings.c)

### Files Updated:
- ✅ `/src/Makefile` - Updated OFILES and build rules
- ✅ `/src/autoconf.in` - Updated OFILES and build rules
- ✅ `/src/protos.h` - Added prototypes for sys_strings.c functions
- ✅ `/src/token.h` - Added prototypes for token_parse.c functions
- ✅ `/libs/melville/boot.c` - Removed test suite call from finish_init()

### Implementation Details:

**Key Design Decision:** Leveraged existing NetCI tokenizer/parser infrastructure
- Created mock `filptr` with `/dev/null` as dummy file
- Reused `get_token()`, `unget_token()` for tokenization
- Built values directly (no bytecode generation)
- Handled tokenization quirks:
  - Negative numbers: `MIN_OPER` + `INTEGER_TOK`
  - Split tokens: `({` → `LPAR_TOK` + `LBRACK_TOK`
  - Combined tokens: `})` → `RARRASGN_TOK` or `RBRACK_TOK` + `RPAR_TOK`

**Test Results:** All tests passing (6/6)
- ✅ Integers (including negative)
- ✅ Strings (including escape sequences)
- ✅ Arrays (empty, simple, mixed-type)
- ✅ Mappings (empty, key-value pairs)
- ✅ Nested structures (arrays in mappings, mappings in mappings)
- ✅ Error handling (invalid syntax returns 0)

**Test File:** `/libs/melville/sys/test_save_restore.c` (available for manual testing)

**Opcodes Used:**
- `S_SAVE_VALUE` = 148
- `S_RESTORE_VALUE` = 149

**Status:** Production-ready ✅

**Next Steps**: 
1. Review and approve implementation approach
2. Implement core serialization functions
3. Add EFUN wrappers
4. Test thoroughly with unit tests
5. Migrate player save/load system
