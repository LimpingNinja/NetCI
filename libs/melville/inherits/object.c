/* object.c - Base object for Melville/NetCI
 *
 * This is the base inheritable for all physical objects in the game world.
 * It provides:
 * - Short and long descriptions
 * - ID and adjective handling
 * - Environment tracking
 * - Movement functions
 * - Basic object properties
 *
 * NOTE: NLPC uses composition via attach() instead of traditional inheritance.
 * For now, we use explicit inheritance until auto.c is implemented.
 */

#include <std.h>

/* Object properties stored in a mapping */
mapping properties;
string *id_list;

/* ========================================================================
 * INITIALIZATION
 * ======================================================================== */

static init() {
    properties = ([]);
    id_list = ({});
}

/* Allow this object to be attached */
allow_attach() {
    return 1;  /* Return non-zero to allow attachment */
}

/* ========================================================================
 * PROPERTY STORAGE (using mappings)
 * ======================================================================== */

/* Store a property for this object */
set_property(key, value) {
    if (!properties) properties = ([]);
    properties[key] = value;
}

/* Retrieve a property for this object */
get_property(key) {
    if (!properties) return NULL;
    return properties[key];
}

/* Query a property for this object */
query_property(key) {
    if (!properties) return NULL;
    return properties[key];
}

/* Get the entire properties mapping */
query_properties() {
    return properties;
}

/* Delete a property for this object */
delete_property(key) {
    if (!properties) return;
    map_delete(properties, key);
}

/* ========================================================================
 * DESCRIPTIONS
 * ======================================================================== */

/* Set short description (shown in inventories and room contents) */
set_short(str) {
    set_property("short", str);
}

/* Query short description */
query_short() {
    return get_property("short");
}

get_short() {
    return query_short();
}

/* Set long description (shown when object is examined) */
set_long(str) {
    set_property("long", str);
}

/* Query long description */
query_long() {
    return get_property("long");
}

get_long() {
    return query_long();
}

/* ========================================================================
 * IDENTIFICATION
 * ======================================================================== */

/* Set ID array - words that identify this object */
set_id(id_array) {
    if (!id_array) {
        id_list = ({});
        return;
    }
    
    id_list = id_array;
}

/* Add a single ID to the list */
add_id(str) {
    if (!str) return;
    
    if (!id_list) id_list = ({});
    
    id_list = id_list + ({ str });
}

/* Check if string matches one of this object's IDs */
id(str) {
    if (!str) return 0;
    if (!id_list) return 0;
    
    return (member_array(str, id_list) != -1);
}

/* Query all IDs (returns array) */
query_id() {
    return id_list;
}

/* ========================================================================
 * ENVIRONMENT AND MOVEMENT
 * ======================================================================== */

/* Query current environment (container) */
query_environment() {
    return location(this_object());
}

get_environment() {
    return query_environment();
}

/* Move this object to a new environment
 * Returns: 1 on success, 0 on failure
 */
move(dest) {
    object old_env, new_env;
    
    if (!dest) return 0;
    
    /* Convert string to object if needed */
    if (typeof(dest) == STRING_T) {
        new_env = atoo(dest);
        if (!new_env) return 0;
    } else {
        new_env = dest;
    }
    
    /* Get current environment */
    old_env = location(this_object());
    
    /* Ask old environment if we can leave */
    if (old_env) {
        if (!call_other(old_env, "release_object", this_object())) {
            return 0;  /* Not allowed to leave */
        }
    }
    
    /* Ask new environment if we can enter */
    if (!call_other(new_env, "receive_object", this_object())) {
        /* New environment rejected us, try to go back */
        if (old_env) {
            call_other(old_env, "receive_object", this_object());
            /* If that fails too, object is in limbo */
        }
        return 0;
    }
    
    /* Perform the actual move */
    move_object(this_object(), new_env);
    
    return 1;
}

/* ========================================================================
 * OBJECT TYPE
 * ======================================================================== */

/* Set object type */
set_type(type) {
    set_property("type", type);
}

/* Query object type */
query_type() {
    return get_property("type");
}

get_type() {
    return query_type();
}

/* ========================================================================
 * VISIBILITY AND PRESENCE
 * ======================================================================== */

/* Check if this object is visible to observer */
visible_to(observer) {
    /* By default, all objects are visible */
    /* Override this in subclasses for invisibility, darkness, etc. */
    return 1;
}

/* Check if this object can be taken */
get() {
    /* By default, objects cannot be taken */
    /* Override this in item objects */
    return 0;
}

/* Check if this object can be dropped */
drop() {
    /* By default, objects cannot be dropped */
    /* Override this in item objects */
    return 0;
}

/* ========================================================================
 * UTILITY FUNCTIONS
 * ======================================================================== */

/* Get a descriptive name for this object */
get_name() {
    string short;
    
    short = query_short();
    if (short) return short;
    
    /* Fall back to object path */
    return otoa(this_object());
}

/* Clean up when object is destructed */
destruct_object() {
    /* Mappings and arrays are automatically freed */
    properties = NULL;
    id_list = NULL;
}
