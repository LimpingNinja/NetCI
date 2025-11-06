/* living.c - Living object inheritable for Melville/NetCI
 *
 * This inheritable provides functionality for "living" objects
 * (players, NPCs, monsters, etc.)
 *
 * Responsibilities:
 * - Mark object as living (query_living)
 * - Communication (listen, catch_tell)
 * - Object finding (find_object helper)
 * - Present checking
 *
 * Usage:
 * - Player objects should attach() this
 * - NPC objects should attach() this
 * - Monsters should attach() this
 */

#include <std.h>
#include <config.h>

/* ========================================================================
 * LIVING STATUS
 * ======================================================================== */

/* Allow this object to be attached */
allow_attach() {
    return 1;
}

/* Mark this as a living object */
query_living() {
    return 1;
}

/* ========================================================================
 * COMMUNICATION
 * ======================================================================== */

/* Receive a message */
listen(msg) {
    if (!msg) return;
    send_device(msg);
}

/* Alias for listen (standard LPC) */
catch_tell(msg) {
    listen(msg);
}

/* Write to this living (convenience) */
write(msg) {
    listen(msg);
}

/* ========================================================================
 * OBJECT FINDING
 * ======================================================================== */

/* Find an object by name/reference
 * Supports:
 * - "me" - this_player()
 * - "/path" or "#num" - absolute object reference
 * - "*name" - find player by name
 * - "name" - search inventory and environment
 * - "here" - current location
 */
find_object(name) {
    object result;
    
    if (!name) return NULL;
    
    /* Special: "me" */
    if (name == "me") {
        return this_player();
    }
    
    /* Absolute path or object number */
    if (leftstr(name, 1) == "/" || leftstr(name, 1) == "#") {
        result = atoo(name);
        if (result) {
            return result;
        }
        return NULL;
    }
    
    /* Player reference: *name */
    if (leftstr(name, 1) == "*") {
        /* TODO: Implement find_player via user_d */
        return NULL;
    }
    
    /* Search in inventory */
    result = present(name, this_player());
    if (result) return result;
    
    /* Search in environment */
    result = present(name, location(this_player()));
    if (result) return result;
    
    /* Special: "here" */
    if (name == "here") {
        return location(this_player());
    }
    
    return NULL;
}

/* Check if object is present in container */
present(name, container) {
    object *inv, obj;
    int i;
    
    if (!name || !container) return NULL;
    
    /* Get inventory */
    inv = container.query_inventory();
    if (!inv) return NULL;
    
    /* Search for matching ID */
    for (i = 0; i < sizeof(inv); i++) {
        obj = inv[i];
        if (obj && obj.id(name)) {
            return obj;
        }
    }
    
    return NULL;
}
