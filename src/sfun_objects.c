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
