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

/* Hash a password using bcrypt (via crypt() efun)
 * Returns: secure hash string (includes salt)
 */
hash_password(password) {
    if (!password) return "";
    
    /* Use crypt() efun with single argument to generate hash with random salt */
    return crypt(password);
}

/* Verify a password against a stored hash
 * Returns: 1 if password matches, 0 if not
 */
verify_password(password, hash) {
    if (!password || !hash) return 0;
    
    /* Use crypt() efun with two arguments to verify */
    return crypt(password, hash);
}

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
/* compile_object wrapper removed - call the efun directly
 * No privilege needed anymore, so no delegation required
 */

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

/* Remove ALL occurrences of element from array
 * Note: Array subtraction removes all matching elements, not just the first
 */
remove_element(arr, element) {
    if (!arr) return ({});
    
    /* Array subtraction removes all occurrences */
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

/* ========================================================================
 * NLPC-STYLE APIS (Delegate to boot.c privileged wrappers)
 * These provide Melville-style convenience functions that call through
 * to the privileged boot.c wrappers for actual implementation.
 * ======================================================================== */

/* Get or compile an object - ensures prototype exists
 * No privilege needed - compilation is safe, security is file-based
 */
get_object(path) {
    object obj;
    
    if (!path) return 0;
    
    obj = atoo(path);
    if (!obj) {
        compile_object(path);
        obj = atoo(path);
    }
    
    return obj;
}

/* Clone an object - creates a new instance
 * Delegates to boot.c for privileged access
 * Can accept either a path string or an object
 */
clone_object(path_or_obj) {
    object boot;
    string path;
    
    if (!path_or_obj) return 0;
    
    boot = atoo("/boot");
    if (!boot) {
        syslog("auto.clone_object(): ERROR - boot object not found!");
        return 0;
    }
    
    /* If it's an object, get its path */
    if (typeof(path_or_obj) == "object") {
        path = otoa(path_or_obj);
    } else {
        path = path_or_obj;
    }
    
    return boot.clone(path);
}

/* Say something to everyone in the room
 * Sends HEAR_SAY event to location's contents
 */
say(msg) {
    object env, *inv;
    int i;
    
    if (!msg) return;
    
    env = location(this_object());
    if (!env) return;
    
    /* Get all objects in the room */
    inv = env.query_inventory();
    if (!inv) return;
    
    /* Send HEAR_SAY to each object except this one */
    for (i = 0; i < sizeof(inv); i++) {
        if (inv[i] && inv[i] != this_object()) {
            /* Call hear() if it exists */
            inv[i].hear(this_object(), inv[i], HEAR_SAY, msg, 0, 0);
        }
    }
    
    /* Also send to this object for local echo */
    this_object().listen(msg + "\n");
}

/* Write a message to an object or this_object()
 * Simple wrapper that calls listen() on the target
 * Falls back to syslog if no player context
 */
write(arg1) {
    object player;
    
    player = this_player();
    if (player) {
        player.listen(arg1);
    } else {
        /* No player context - use syswrite for formatted output */
        syswrite(arg1);
    }
}

