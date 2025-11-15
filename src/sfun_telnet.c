/**
 * @file sfun_telnet.c
 * @brief Telnet and terminal capability efuns
 *
 * Provides access to telnet negotiation results and MSSP data
 */

#include "config.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include "object.h"
#include "protos.h"
#include "instr.h"
#include "constrct.h"
#include "interp.h"
#define CMPLNG_INTRFCE  /* Need this to see struct connlist_s definition */
#include "intrface.h"
#include "globals.h"
#include "file.h"

/* External connection list from intrface.c */
extern struct connlist_s *connlist;
extern int num_conns;

/**
 * @brief query_terminal(object player)
 *
 * Returns a mapping containing terminal capability information for a player
 *
 * @param player Player object to query
 * @return Mapping with terminal data or 0 if not connected
 *
 * Returns mapping: ([
 *   "term_type": string,  // Terminal type (e.g. "xterm-256color")
 *   "width": int,         // Window width
 *   "height": int,        // Window height
 *   "naws": int,          // NAWS negotiated? (1/0)
 *   "ttype": int,         // TTYPE negotiated? (1/0)
 *   "echo": int,          // ECHO negotiated? (1/0)
 *   "sga": int            // SGA negotiated? (1/0)
 * ])
 */
int s_query_terminal(struct object *caller, struct object *obj, 
                     struct object *player, struct var_stack **rts) {
    struct var tmp, key, val;
    struct object *target;
    struct heap_mapping *result;
    int devnum;
    
    /* Pop arguments */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != NUM_ARGS) {
        clear_var(&tmp);
        return 1;
    }
    if (tmp.value.num != 1) return 1;
    
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != OBJECT) {
        clear_var(&tmp);
        tmp.type = INTEGER;
        tmp.value.integer = 0;
        push(&tmp, rts);
        return 0;
    }
    
    target = tmp.value.objptr;
    
    /* Check if object has a connection */
    if (!(target->flags & CONNECTED) || target->devnum == -1) {
        clear_var(&tmp);
        tmp.type = INTEGER;
        tmp.value.integer = 0;
        push(&tmp, rts);
        return 0;
    }
    
    devnum = target->devnum;
    
    /* Create result mapping (10 fields now with MTTS support) */
    result = allocate_mapping(10);
    if (!result) {
        clear_var(&tmp);
        tmp.type = INTEGER;
        tmp.value.integer = 0;
        push(&tmp, rts);
        return 0;
    }
    
    /* Add term_client (MTTS round 1) */
    key.type = STRING;
    key.value.string = "term_client";
    if (connlist[devnum].term_client[0] != '\0') {
        val.type = STRING;
        val.value.string = connlist[devnum].term_client;
    } else {
        val.type = STRING;
        val.value.string = "";
    }
    mapping_set(result, &key, &val);
    
    /* Add term_type (normalized: XTERM, ANSI, VT100, DUMB) */
    key.value.string = "term_type";
    if (connlist[devnum].term_type[0] != '\0') {
        val.type = STRING;
        val.value.string = connlist[devnum].term_type;
    } else {
        val.type = STRING;
        val.value.string = "";
    }
    mapping_set(result, &key, &val);
    
    /* Add term_support (MTTS capability bitmask) */
    key.value.string = "term_support";
    val.type = INTEGER;
    val.value.integer = connlist[devnum].term_support;
    mapping_set(result, &key, &val);
    
    /* Add width */
    key.value.string = "width";
    val.type = INTEGER;
    val.value.integer = connlist[devnum].win_width;
    mapping_set(result, &key, &val);
    
    /* Add height */
    key.value.string = "height";
    val.value.integer = connlist[devnum].win_height;
    mapping_set(result, &key, &val);
    
    /* Add naws flag */
    key.value.string = "naws";
    val.value.integer = connlist[devnum].opt_naws;
    mapping_set(result, &key, &val);
    
    /* Add ttype flag */
    key.value.string = "ttype";
    val.value.integer = connlist[devnum].opt_ttype;
    mapping_set(result, &key, &val);
    
    /* Add echo flag */
    key.value.string = "echo";
    val.value.integer = connlist[devnum].opt_echo;
    mapping_set(result, &key, &val);
    
    /* Add sga flag */
    key.value.string = "sga";
    val.value.integer = connlist[devnum].opt_sga;
    mapping_set(result, &key, &val);
    
    /* Return mapping */
    clear_var(&tmp);
    tmp.type = MAPPING;
    tmp.value.mapping_ptr = result;
    push(&tmp, rts);
    return 0;
}

/**
 * @brief get_mssp()
 *
 * Returns the current MSSP data mapping (reconstructed from C structure)
 *
 * @return Mapping with MSSP data or empty mapping if none set
 */
int s_get_mssp(struct object *caller, struct object *obj,
               struct object *player, struct var_stack **rts) {
    struct var tmp, key, val;
    struct heap_mapping *result;
    extern struct mssp_var *mssp_vars;
    extern int mssp_var_count;
    
    /* Pop arguments */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != NUM_ARGS) {
        clear_var(&tmp);
        return 1;
    }
    if (tmp.value.num != 0) return 1;
    
    /* Create mapping from C structure */
    result = allocate_mapping(mssp_var_count);
    if (!result) {
        tmp.type = INTEGER;
        tmp.value.integer = 0;
        push(&tmp, rts);
        return 0;
    }
    
    /* Populate mapping from C array */
    key.type = STRING;
    val.type = STRING;
    for (int i = 0; i < mssp_var_count; i++) {
        key.value.string = mssp_vars[i].name;
        val.value.string = mssp_vars[i].value;
        mapping_set(result, &key, &val);
    }
    
    tmp.type = MAPPING;
    tmp.value.mapping_ptr = result;
    push(&tmp, rts);
    return 0;
}

/**
 * @brief set_mssp(mapping data)
 *
 * Sets custom MSSP variables to be sent to clients
 * Converts LPC mapping to C structure for use in telnet negotiation
 *
 * @param data Mapping of MSSP variable names to values
 * @return 1 on success, 0 on failure
 */
int s_set_mssp(struct object *caller, struct object *obj,
               struct object *player, struct var_stack **rts) {
    struct var tmp;
    struct heap_mapping *map;
    struct heap_array *keys_arr, *vals_arr;
    extern struct mssp_var *mssp_vars;
    extern int mssp_var_count;
    int i;
    
    /* Pop arguments */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != NUM_ARGS) {
        clear_var(&tmp);
        return 1;
    }
    if (tmp.value.num != 1) return 1;
    
    if (pop(&tmp, rts, obj)) return 1;
    
    /* Accept mapping or 0 (to clear) */
    if (tmp.type == INTEGER && tmp.value.integer == 0) {
        /* Clear MSSP data */
        if (mssp_vars) {
            for (i = 0; i < mssp_var_count; i++) {
                if (mssp_vars[i].name) FREE(mssp_vars[i].name);
                if (mssp_vars[i].value) FREE(mssp_vars[i].value);
            }
            FREE(mssp_vars);
            mssp_vars = NULL;
            mssp_var_count = 0;
        }
        clear_var(&tmp);
        tmp.type = INTEGER;
        tmp.value.integer = 1;
        push(&tmp, rts);
        return 0;
    }
    
    if (tmp.type != MAPPING) {
        clear_var(&tmp);
        tmp.type = INTEGER;
        tmp.value.integer = 0;
        push(&tmp, rts);
        return 0;
    }
    
    map = tmp.value.mapping_ptr;
    
    /* Clear existing MSSP data */
    if (mssp_vars) {
        for (i = 0; i < mssp_var_count; i++) {
            if (mssp_vars[i].name) FREE(mssp_vars[i].name);
            if (mssp_vars[i].value) FREE(mssp_vars[i].value);
        }
        FREE(mssp_vars);
    }
    
    /* Convert mapping to C structure */
    keys_arr = mapping_keys_array(map);
    vals_arr = mapping_values_array(map);
    
    if (!keys_arr || !vals_arr) {
        clear_var(&tmp);
        tmp.type = INTEGER;
        tmp.value.integer = 0;
        push(&tmp, rts);
        return 0;
    }
    
    mssp_var_count = keys_arr->size;
    mssp_vars = MALLOC(sizeof(struct mssp_var) * mssp_var_count);
    
    /* Copy strings from arrays to C structure */
    for (i = 0; i < mssp_var_count; i++) {
        if (keys_arr->elements[i].type == STRING && 
            vals_arr->elements[i].type == STRING) {
            mssp_vars[i].name = copy_string(keys_arr->elements[i].value.string);
            mssp_vars[i].value = copy_string(vals_arr->elements[i].value.string);
        } else if (keys_arr->elements[i].type == STRING && 
                   vals_arr->elements[i].type == INTEGER) {
            char buf[32];
            mssp_vars[i].name = copy_string(keys_arr->elements[i].value.string);
            sprintf(buf, "%ld", vals_arr->elements[i].value.integer);
            mssp_vars[i].value = copy_string(buf);
        } else {
            mssp_vars[i].name = copy_string("");
            mssp_vars[i].value = copy_string("");
        }
    }
    
    array_release(keys_arr);
    array_release(vals_arr);
    
    clear_var(&tmp);
    tmp.type = INTEGER;
    tmp.value.integer = 1;
    push(&tmp, rts);
    return 0;
}
