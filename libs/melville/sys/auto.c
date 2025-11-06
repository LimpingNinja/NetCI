/* auto.c - Auto object for Melville/NetCI
 *
 * This object is automatically inherited/attached to all objects in the mudlib.
 * It provides simulated efuns (sefuns) and utility functions.
 *
 * Responsibilities:
 * - Simulated efuns (find_object, compile_object, etc.)
 * - Password hashing
 * - String utilities
 * - Object utilities
 * - Security helpers
 *
 * Note: In NetCI, this should be configured in boot.c to be auto-inherited
 * or attached to all objects.
 */

#include <std.h>
#include <config.h>

/* ========================================================================
 * ATTACHMENT
 * ======================================================================== */

/* Allow this object to be attached */
allow_attach() {
    return 1;
}

/* ========================================================================
 * PASSWORD HASHING
 * ======================================================================== */

/* Simple password hashing function
 * TODO: Replace with proper cryptographic hash (bcrypt, scrypt, argon2)
 * For now, uses a simple XOR-based hash with salt
 * 
 * SECURITY WARNING: This is NOT secure for production!
 * It's a placeholder until we implement proper hashing.
 */
hash_password(password) {
    string salt, hash;
    int i, len, hash_val, char_val;
    
    if (!password) return "";
    
    /* Use a fixed salt for now (INSECURE!) */
    /* TODO: Generate random salt per password and store it */
    salt = "MelvilleNetCI2025";
    
    len = strlen(password);
    hash = "";
    hash_val = 5381; /* DJB2 hash initial value */
    
    /* Simple DJB2 hash algorithm */
    for (i = 0; i < len; i++) {
        /* Get character value (NetCI doesn't have direct char access) */
        char_val = i; /* Placeholder - need proper char-to-int conversion */
        hash_val = ((hash_val * 33) + char_val) % 2147483647;
    }
    
    /* Convert hash to string */
    hash = "HASH_" + itoa(hash_val);
    
    return hash;
}

/* TODO: Implement proper password hashing when crypt() efun is available
 * Proper implementation would be:
 * 
 * hash_password(password) {
 *     string salt;
 *     
 *     // Generate random salt
 *     salt = generate_salt();
 *     
 *     // Use crypt with bcrypt or similar
 *     return crypt(password, "$2b$10$" + salt);
 * }
 */

/* ========================================================================
 * OBJECT FINDING (Simulated Efun)
 * ======================================================================== */

/* Find an object by name/reference
 * This is a simulated efun that searches intelligently
 * 
 * Supports:
 * - "me" - this_player()
 * - "/path" or "#num" - absolute object reference
 * - "*name" - find player by name (via user_d)
 * - "name" - search inventory and environment
 * - "here" - current location
 */
find_object(name) {
    object result, player;
    
    if (!name) return NULL;
    
    player = this_player();
    
    /* Special: "me" */
    if (name == "me") {
        return player;
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
        /* For now, return NULL */
        return NULL;
    }
    
    /* Search in player's inventory */
    if (player) {
        result = present(name, player);
        if (result) return result;
        
        /* Search in player's environment */
        result = present(name, location(player));
        if (result) return result;
    }
    
    /* Special: "here" */
    if (name == "here") {
        return location(player);
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

/* ========================================================================
 * OBJECT COMPILATION (Simulated Efun)
 * ======================================================================== */

/* Compile an object from source
 * This is a wrapper around the compile_object efun
 * Adds error handling and logging
 */
compile_object(path) {
    object result;
    
    if (!path) return NULL;
    
    /* Try to compile */
    result = compile_object(path);
    
    if (!result) {
        /* Log compilation failure */
        syslog("Failed to compile: " + path);
    }
    
    return result;
}

/* ========================================================================
 * STRING UTILITIES
 * ======================================================================== */

/* Capitalize first letter of string */
capitalize(str) {
    string first, rest;
    
    if (!str || strlen(str) == 0) return str;
    
    if (strlen(str) == 1) {
        return upcase(str);
    }
    
    first = leftstr(str, 1);
    rest = rightstr(str, strlen(str) - 1);
    
    return upcase(first) + rest;
}

/* Lowercase entire string */
lowercase(str) {
    return downcase(str);
}

/* Uppercase entire string */
uppercase(str) {
    return upcase(str);
}

/* Trim whitespace from string */
trim(str) {
    if (!str) return "";
    
    /* Trim leading spaces */
    while (strlen(str) > 0 && leftstr(str, 1) == " ") {
        str = rightstr(str, strlen(str) - 1);
    }
    
    /* Trim trailing spaces */
    while (strlen(str) > 0 && rightstr(str, 1) == " ") {
        str = leftstr(str, strlen(str) - 1);
    }
    
    return str;
}

/* ========================================================================
 * ARRAY UTILITIES
 * ======================================================================== */

/* Check if array contains element */
contains(arr, element) {
    if (!arr) return 0;
    return (member_array(element, arr) != -1);
}

/* Remove element from array */
remove_element(arr, element) {
    int pos, i;
    
    if (!arr) return ({});
    
    pos = member_array(element, arr);
    if (pos == -1) return arr;
    
    /* Build new array without the element by concatenating parts */
    /* We can't use untyped variables, so we'll use a different approach */
    /* Just rebuild the array by filtering out the element */
    if (pos == 0 && sizeof(arr) == 1) {
        return ({});
    }
    
    /* Use array arithmetic to build result */
    if (pos == 0) {
        /* Remove first element - can't slice, so rebuild manually */
        return arr - ({ element });
    }
    
    /* For other positions, use array subtraction */
    return arr - ({ element });
}

/* ========================================================================
 * OBJECT UTILITIES
 * ======================================================================== */

/* Check if object is a player */
is_player(obj) {
    if (!obj) return 0;
    return (obj.query_property("type") == TYPE_PLAYER);
}

/* Check if object is living */
is_living(obj) {
    if (!obj) return 0;
    return obj.query_living();
}

/* Get object's short description */
get_short(obj) {
    if (!obj) return "nothing";
    return obj.query_short();
}

/* Get object's long description */
get_long(obj) {
    if (!obj) return "You see nothing special.";
    return obj.query_long();
}

/* ========================================================================
 * SECURITY HELPERS
 * ======================================================================== */

/* Check if caller has privilege */
has_privilege(level) {
    object caller;
    int caller_role;
    
    caller = caller_object();
    if (!caller) return 0;
    
    caller_role = caller.query_role();
    if (!caller_role) return 0;
    
    return (caller_role >= level);
}

/* Check if caller is wizard+ */
is_wizard(obj) {
    if (!obj) obj = caller_object();
    if (!obj) return 0;
    return obj.query_wizard();
}

/* Check if caller is admin */
is_admin(obj) {
    if (!obj) obj = caller_object();
    if (!obj) return 0;
    return obj.query_admin();
}

/* ========================================================================
 * MESSAGING UTILITIES
 * ======================================================================== */

/* Send message to player */
tell(player, msg) {
    if (!player || !msg) return;
    player.listen(msg);
}

/* Send message to all in room except exclusions */
tell_room(room, msg, exclude) {
    if (!room || !msg) return;
    room.room_tell(msg, exclude);
}

/* ========================================================================
 * DEBUGGING UTILITIES
 * ======================================================================== */

/* Debug print (logs to syslog) */
debug(msg) {
    syslog("[DEBUG] " + msg);
}

/* Dump object info */
dump_object(obj) {
    string output;
    
    if (!obj) {
        debug("dump_object: NULL object");
        return;
    }
    
    output = "Object: " + otoa(obj) + "\n";
    output = output + "  Short: " + obj.query_short() + "\n";
    output = output + "  Location: " + otoa(location(obj)) + "\n";
    
    debug(output);
}
