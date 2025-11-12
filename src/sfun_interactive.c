/* sfun_interactive.c - Interactive/player management efuns
 *
 * This file contains efuns for managing and querying interactive (connected)
 * players and connections.
 *
 * Functions:
 *   - users() - Returns array of all connected players
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "object.h"
#include "protos.h"
#include "instr.h"
#include "constrct.h"
#include "intrface.h"
#include "file.h"

/* users() - Return array of all connected players
 *
 * Returns an object array containing all currently connected interactive
 * objects. This is a convenience wrapper around next_who() that builds
 * and returns an array instead of requiring manual iteration.
 *
 * Returns: object * array of connected players (may be empty array)
 */
int s_users(struct object *caller, struct object *obj, struct object *player,
            struct var_stack **rts) {
    struct heap_array *arr;
    struct var *elem;
    struct object *conn_obj;
    struct var tmp, result;
    int count = 0;
    int i;
    
    /* Pop NUM_ARGS - users() takes no arguments */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != NUM_ARGS) {
        clear_var(&tmp);
        return 1;
    }
    if (tmp.value.num != 0) return 1;
    
    /* First pass: count connected players */
    conn_obj = next_who(NULL);
    while (conn_obj) {
        count++;
        conn_obj = next_who(conn_obj);
    }
    
    /* Allocate array */
    arr = allocate_array(count, UNLIMITED_ARRAY_SIZE);
    if (!arr) {
        logger(LOG_ERROR, "users(): Failed to allocate array");
        return 1;
    }
    
    /* Second pass: populate array */
    elem = arr->elements;
    conn_obj = next_who(NULL);
    i = 0;
    while (conn_obj && i < count) {
        elem[i].type = OBJECT;
        elem[i].value.objptr = conn_obj;
        i++;
        conn_obj = next_who(conn_obj);
    }
    
    /* Push result onto stack */
    result.type = ARRAY;
    result.value.array_ptr = arr;
    push(&result, rts);
    
    return 0;
}
