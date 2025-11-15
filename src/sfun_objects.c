/* sfun_objects.c - Object collection and iteration efuns
 *
 * This file contains efuns for querying and iterating over objects in
 * various collections (all objects, clones, inventory).
 *
 * Functions:
 *   - objects() - Returns array of all loaded prototypes/objects
 *   - children() - Returns array of all clones of a prototype
 *   - all_inventory() - Returns array of all objects in a container
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "object.h"
#include "protos.h"
#include "instr.h"
#include "constrct.h"
#include "globals.h"
#include "file.h"

/* objects() - Return array of all loaded prototypes
 *
 * Returns an object array containing all loaded prototypes in the system.
 * This is a convenience wrapper around next_proto() that builds and returns
 * an array instead of requiring manual iteration.
 *
 * Returns: object * array of all prototypes
 */
int s_objects(struct object *caller, struct object *obj, struct object *player,
              struct var_stack **rts) {
    struct heap_array *arr;
    struct var *elem;
    struct object *proto_obj;
    struct var tmp, result;
    int count = 0;
    int i;
    
    /* Pop NUM_ARGS - objects() takes no arguments */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != NUM_ARGS) {
        clear_var(&tmp);
        return 1;
    }
    if (tmp.value.num != 0) return 1;
    
    /* First pass: count prototypes - iterate through proto linked list */
    proto_obj = obj; /* Start with current object */
    if (proto_obj && proto_obj->parent) {
        proto_obj = proto_obj->parent->proto_obj; /* Get to a real proto */
        struct proto *proto = proto_obj->parent;
        
        /* Iterate through all prototypes */
        while (proto) {
            count++;
            proto = proto->next_proto;
        }
    }
    
    /* Allocate array */
    arr = allocate_array(count, UNLIMITED_ARRAY_SIZE);
    if (!arr) {
        return 1;
    }
    
    /* Second pass: populate array */
    if (count > 0 && proto_obj && proto_obj->parent) {
        elem = arr->elements;
        struct proto *proto = proto_obj->parent;
        i = 0;
        while (proto && i < count) {
            elem[i].type = OBJECT;
            elem[i].value.objptr = proto->proto_obj;
            proto = proto->next_proto;
            i++;
        }
    }
    
    /* Push result onto stack */
    result.type = ARRAY;
    result.value.array_ptr = arr;
    push(&result, rts);
    
    return 0;
}

/* children() - Return array of all clones of a prototype
 *
 * Takes an object (prototype) and returns an array of all its clones.
 * This is a convenience wrapper around next_child() that builds and returns
 * an array instead of requiring manual iteration.
 *
 * Args: object prototype
 * Returns: object * array of all clones (may be empty)
 */
int s_children(struct object *caller, struct object *obj, struct object *player,
               struct var_stack **rts) {
    struct heap_array *arr;
    struct var *elem;
    struct var tmp, result;
    struct object *proto_obj, *clone;
    int count = 0;
    int i;
    
    /* Pop NUM_ARGS */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != NUM_ARGS || tmp.value.num != 1) {
        clear_var(&tmp);
        return 1;
    }
    
    /* Get prototype argument */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != OBJECT) {
        clear_var(&tmp);
        return 1;
    }
    
    proto_obj = tmp.value.objptr;
    clear_var(&tmp);
    
    if (!proto_obj) {
        /* Return empty array for null object */
        arr = allocate_array(0, UNLIMITED_ARRAY_SIZE);
        if (!arr) return 1;
        result.type = ARRAY;
        result.value.array_ptr = arr;
        push(&result, rts);
        return 0;
    }
    
    /* First pass: count children using next_child field */
    clone = proto_obj->next_child;
    while (clone) {
        count++;
        clone = clone->next_child;
    }
    
    /* Allocate array */
    arr = allocate_array(count, UNLIMITED_ARRAY_SIZE);
    if (!arr) {
        return 1;
    }
    
    /* Second pass: populate array */
    elem = arr->elements;
    clone = proto_obj->next_child;
    i = 0;
    while (clone && i < count) {
        elem[i].type = OBJECT;
        elem[i].value.objptr = clone;
        clone = clone->next_child;
        i++;
    }
    
    /* Push result onto stack */
    result.type = ARRAY;
    result.value.array_ptr = arr;
    push(&result, rts);
    
    return 0;
}

/* all_inventory() - Return array of all objects in a container
 *
 * Takes a container object and returns an array of all objects in its
 * inventory. This is a convenience wrapper around contents() and next_object()
 * that builds and returns an array instead of requiring manual iteration.
 *
 * Args: object container
 * Returns: object * array of inventory items (may be empty)
 */
int s_all_inventory(struct object *caller, struct object *obj, struct object *player,
                    struct var_stack **rts) {
    struct heap_array *arr;
    struct var *elem;
    struct var tmp, result;
    struct object *container, *item;
    int count = 0;
    int i;
    
    /* Pop NUM_ARGS */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != NUM_ARGS || tmp.value.num != 1) {
        clear_var(&tmp);
        return 1;
    }
    
    /* Get container argument */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != OBJECT) {
        clear_var(&tmp);
        return 1;
    }
    
    container = tmp.value.objptr;
    clear_var(&tmp);
    
    if (!container) {
        /* Return empty array for null object */
        arr = allocate_array(0, UNLIMITED_ARRAY_SIZE);
        if (!arr) return 1;
        result.type = ARRAY;
        result.value.array_ptr = arr;
        push(&result, rts);
        return 0;
    }
    
    /* First pass: count inventory items using contents and next_object */
    item = container->contents;
    while (item) {
        count++;
        item = item->next_object;
    }
    
    /* Allocate array */
    arr = allocate_array(count, UNLIMITED_ARRAY_SIZE);
    if (!arr) {
        return 1;
    }
    
    /* Second pass: populate array */
    elem = arr->elements;
    item = container->contents;
    i = 0;
    while (item && i < count) {
        elem[i].type = OBJECT;
        elem[i].value.objptr = item;
        item = item->next_object;
        i++;
    }
    
    /* Push result onto stack */
    result.type = ARRAY;
    result.value.array_ptr = arr;
    push(&result, rts);
    
    return 0;
}

/* ========================================================================
 * OBJECT PERSISTENCE - save_object/restore_object/restore_map
 * ======================================================================== */

#include "save_adapter.h"

/* Forward declaration */
extern struct object *ref_to_obj(signed long refno);

/* Helper: Get variable name from global symbol table by index */
static char *get_global_var_name_by_index(struct var_tab *gst, int index) {
    struct var_tab *curr = gst;
    
    while (curr) {
        if (curr->base == index) {
            return curr->name;
        }
        curr = curr->next;
    }
    
    return NULL;
}

/* Helper: Serialize object reference as string "#refno:pathname" */
static struct var serialize_object_ref(struct object *obj) {
    struct var result;
    char *str;
    
    if (!obj) {
        /* NULL object = INTEGER 0 */
        result.type = INTEGER;
        result.value.integer = 0;
        return result;
    }
    
    /* Format: "#1234:/path/to/object.c" */
    str = MALLOC(strlen(obj->parent->pathname) + 20);
    sprintf(str, "#%ld:%s", (long)obj->refno, obj->parent->pathname);
    
    result.type = STRING;
    result.value.string = str;
    return result;
}

/* Helper: Deserialize object reference from string "#refno:pathname" */
static struct object *deserialize_object_ref(char *str) {
    long refno;
    
    if (!str || *str != '#') return NULL;
    
    /* Parse refno */
    refno = atol(str + 1);
    
    /* Get object by refno */
    return ref_to_obj(refno);
}

/* Helper: Build mapping from object's global variables */
static struct heap_mapping *serialize_object_globals(struct object *obj) {
    struct heap_mapping *result;
    char *var_name;
    struct var key, value;
    int i;
    
    if (!obj || !obj->parent || !obj->parent->funcs) {
        return NULL;
    }
    
    /* Allocate mapping with capacity for all globals */
    result = allocate_mapping(obj->parent->funcs->num_globals * 2);
    if (!result) return NULL;
    
    /* Iterate through all global variables */
    for (i = 0; i < obj->parent->funcs->num_globals; i++) {
        /* Get variable name from symbol table */
        var_name = get_global_var_name_by_index(obj->parent->funcs->gst, i);
        if (!var_name) continue;
        
        /* Create string key for variable name */
        key.type = STRING;
        key.value.string = var_name;
        
        /* Get variable value */
        value = obj->globals[i];
        
        /* Handle OBJECT type - serialize as string reference */
        if (value.type == OBJECT) {
            value = serialize_object_ref(value.value.objptr);
        }
        
        /* Add to mapping: mapping[varname] = value */
        mapping_set(result, &key, &value);
    }
    
    return result;
}

/* Helper: Restore object globals from mapping */
static int deserialize_object_globals(struct object *obj, struct heap_mapping *data) {
    struct var key, value;
    char *var_name;
    int i, found;
    char logbuf[256];
    
    if (!obj || !obj->parent || !obj->parent->funcs || !data) {
        return -1;
    }
    
    /* Iterate through all global variables */
    for (i = 0; i < obj->parent->funcs->num_globals; i++) {
        /* Get variable name */
        var_name = get_global_var_name_by_index(obj->parent->funcs->gst, i);
        if (!var_name) continue;
        
        /* Create string key */
        key.type = STRING;
        key.value.string = var_name;
        
        /* Get value from mapping */
        found = mapping_get(data, &key, &value);
        if (!found) {
            sprintf(logbuf, "  sfun_objects: variable '%s' not found in save file", var_name);
            logger(LOG_ERROR, logbuf);
            continue;
        }
        
        /* Clear existing value */
        clear_var(&obj->globals[i]);
        
        /* Handle object references (stored as strings) */
        if (value.type == STRING && value.value.string[0] == '#') {
            obj->globals[i].type = OBJECT;
            obj->globals[i].value.objptr = deserialize_object_ref(value.value.string);
        } else {
            /* Copy value */
            copy_var(&obj->globals[i], &value);  /* dest, src */
        }
        
        /* Note: Don't clear 'value' - it's a reference from mapping_get, not owned by us */
    }
    
    return 0;
}

/* save_object([string key])
 * Serializes caller's global variables to storage
 * Key defaults to object pathname
 * Returns: 1 on success, 0 on failure
 */
int s_save_object(struct object *caller, struct object *obj, struct object *player,
                  struct var_stack **rts) {
    struct heap_mapping *data;
    struct var tmp;
    char *key = NULL;
    int num_args, result;
    save_adapter_t *adapter;
    
    /* Pop NUM_ARGS */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != NUM_ARGS) {
        clear_var(&tmp);
        return 1;
    }
    num_args = tmp.value.num;
    
    /* Get optional key argument */
    if (num_args == 1) {
        if (pop(&tmp, rts, obj)) return 1;
        
        /* Handle INTEGER 0 as empty string */
        if (tmp.type == INTEGER && tmp.value.integer == 0) {
            tmp.type = STRING;
            *(tmp.value.string = MALLOC(1)) = '\0';
        }
        
        if (tmp.type == STRING) {
            key = copy_string(tmp.value.string);
        }
        clear_var(&tmp);
    } else if (num_args != 0) {
        return 1;
    }
    
    /* Default key is object pathname */
    if (!key || !*key) {
        if (key) FREE(key);
        key = copy_string(obj->parent->pathname);
    }
    
    /* Serialize caller's globals to mapping */
    data = serialize_object_globals(obj);
    if (!data) {
        FREE(key);
        tmp.type = INTEGER;
        tmp.value.integer = 0;
        push(&tmp, rts);
        return 0;
    }
    
    /* Save via adapter */
    adapter = get_save_adapter();
    result = adapter->save_map(key, data, obj);
    
    /* Cleanup */
    mapping_release(data);
    FREE(key);
    
    /* Return success/failure */
    tmp.type = INTEGER;
    tmp.value.integer = (result == 0) ? 1 : 0;
    push(&tmp, rts);
    return 0;
}

/* restore_object([string key])
 * Deserializes from storage and sets caller's global variables
 * Key defaults to object pathname
 * Returns: 1 on success, 0 on failure
 */
int s_restore_object(struct object *caller, struct object *obj, struct object *player,
                     struct var_stack **rts) {
    struct heap_mapping *data;
    struct var tmp;
    char *key = NULL;
    int num_args, result;
    save_adapter_t *adapter;
    
    /* Pop NUM_ARGS */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != NUM_ARGS) {
        clear_var(&tmp);
        return 1;
    }
    num_args = tmp.value.num;
    
    /* Get optional key argument */
    if (num_args == 1) {
        if (pop(&tmp, rts, obj)) return 1;
        
        /* Handle INTEGER 0 as empty string */
        if (tmp.type == INTEGER && tmp.value.integer == 0) {
            tmp.type = STRING;
            *(tmp.value.string = MALLOC(1)) = '\0';
        }
        
        if (tmp.type == STRING) {
            key = copy_string(tmp.value.string);
        }
        clear_var(&tmp);
    } else if (num_args != 0) {
        return 1;
    }
    
    /* Default key is object pathname */
    if (!key || !*key) {
        if (key) FREE(key);
        key = copy_string(obj->parent->pathname);
    }
    
    /* Restore via adapter */
    adapter = get_save_adapter();
    data = adapter->restore_map(key, obj);
    FREE(key);
    
    if (!data) {
        /* File not found or error */
        tmp.type = INTEGER;
        tmp.value.integer = 0;
        push(&tmp, rts);
        return 0;
    }
    
    /* Deserialize mapping to object globals */
    result = deserialize_object_globals(obj, data);
    mapping_release(data);
    
    /* Return success/failure */
    tmp.type = INTEGER;
    tmp.value.integer = (result == 0) ? 1 : 0;
    push(&tmp, rts);
    return 0;
}

/* restore_map(string key)
 * Retrieves raw mapping from storage without setting variables
 * Returns: mapping on success, 0 on failure
 */
int s_restore_map(struct object *caller, struct object *obj, struct object *player,
                  struct var_stack **rts) {
    struct heap_mapping *data;
    struct var tmp;
    char *key;
    int num_args;
    save_adapter_t *adapter;
    
    /* Pop NUM_ARGS */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != NUM_ARGS) {
        clear_var(&tmp);
        return 1;
    }
    num_args = tmp.value.num;
    
    /* Expect 1 argument */
    if (num_args != 1) return 1;
    
    /* Pop key argument */
    if (pop(&tmp, rts, obj)) return 1;
    
    /* Handle INTEGER 0 as empty string */
    if (tmp.type == INTEGER && tmp.value.integer == 0) {
        tmp.type = STRING;
        *(tmp.value.string = MALLOC(1)) = '\0';
    }
    
    if (tmp.type != STRING) {
        clear_var(&tmp);
        return 1;
    }
    
    key = copy_string(tmp.value.string);
    clear_var(&tmp);
    
    /* Restore via adapter */
    adapter = get_save_adapter();
    data = adapter->restore_map(key, obj);
    FREE(key);
    
    if (!data) {
        /* File not found or error - return 0 */
        tmp.type = INTEGER;
        tmp.value.integer = 0;
        push(&tmp, rts);
        return 0;
    }
    
    /* Return mapping */
    tmp.type = MAPPING;
    tmp.value.mapping_ptr = data;
    push(&tmp, rts);
    return 0;
}

/* query_idle_time() - Get idle time of an object
 *
 * Returns the number of seconds since the object was last accessed.
 * Used by clean_up() logic to determine if an object should be destructed.
 *
 * Syntax: int query_idle_time()
 * Returns: number of seconds since last access
 */
int s_query_idle_time(struct object *caller, struct object *obj, 
                      struct object *player, struct var_stack **rts) {
    struct var tmp;
    long idle_seconds;
    
    /* Pop NUM_ARGS - query_idle_time() takes no arguments */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != NUM_ARGS) {
        clear_var(&tmp);
        return 1;
    }
    if (tmp.value.num != 0) return 1;
    
    /* Calculate idle time: now - last_access_time */
    idle_seconds = now_time - caller->last_access_time;
    if (idle_seconds < 0) idle_seconds = 0;  /* Sanity check */
    
    /* Return idle time in seconds */
    tmp.type = INTEGER;
    tmp.value.integer = idle_seconds;
    push(&tmp, rts);
    return 0;
}

/* query_config() - Get driver configuration values
 *
 * Returns a mapping of driver configuration from netci.ini:
 *   ([ "save_path": "data/save/",
 *      "save_type": "file",
 *      "auto_object": "/sys/auto",
 *      "time_cleanup": 1200,
 *      "time_reset": 800,
 *      "time_heartbeat": 2000 ])
 *
 * Syntax: mapping query_config()
 * Returns: mapping of config keys to values
 */
int s_query_config(struct object *caller, struct object *obj,
                   struct object *player, struct var_stack **rts) {
    struct var tmp, key, value;
    struct heap_mapping *config;

    /* Pop NUM_ARGS - query_config() takes no arguments */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != NUM_ARGS) {
        clear_var(&tmp);
        return 1;
    }
    if (tmp.value.num != 0) return 1;

    /* Create new mapping for config (6 config items) */
    config = allocate_mapping(6);
    if (!config) {
        tmp.type = INTEGER;
        tmp.value.integer = 0;
        push(&tmp, rts);
        return 0;
    }

    /* Add save_path */
    if (save_path && *save_path) {
        key.type = STRING;
        key.value.string = copy_string("save_path");
        value.type = STRING;
        value.value.string = copy_string(save_path);
        mapping_set(config, &key, &value);
        clear_var(&key);
        clear_var(&value);
    }

    /* Add save_type */
    if (save_type && *save_type) {
        key.type = STRING;
        key.value.string = copy_string("save_type");
        value.type = STRING;
        value.value.string = copy_string(save_type);
        mapping_set(config, &key, &value);
        clear_var(&key);
        clear_var(&value);
    }

    /* Add auto_object */
    if (auto_object_path && *auto_object_path) {
        key.type = STRING;
        key.value.string = copy_string("auto_object");
        value.type = STRING;
        value.value.string = copy_string(auto_object_path);
        mapping_set(config, &key, &value);
        clear_var(&key);
        clear_var(&value);
    }

    /* Add time_cleanup */
    key.type = STRING;
    key.value.string = copy_string("time_cleanup");
    value.type = INTEGER;
    value.value.integer = time_cleanup;
    mapping_set(config, &key, &value);
    clear_var(&key);

    /* Add time_reset */
    key.type = STRING;
    key.value.string = copy_string("time_reset");
    value.type = INTEGER;
    value.value.integer = time_reset;
    mapping_set(config, &key, &value);
    clear_var(&key);

    /* Add time_heartbeat */
    key.type = STRING;
    key.value.string = copy_string("time_heartbeat");
    value.type = INTEGER;
    value.value.integer = time_heartbeat;
    mapping_set(config, &key, &value);
    clear_var(&key);

    /* Return the mapping */
    tmp.type = MAPPING;
    tmp.value.mapping_ptr = config;
    push(&tmp, rts);
    return 0;
}
