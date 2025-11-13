/* boot.c - Melville/NetCI Master Object
 *
 * This is the master object - the first object loaded by NetCI.
 * It handles system initialization, connections, and security.
 */

#include <std.h>
#include <config.h>
#include <roles.h>

/* ========================================================================
 * SECURITY CALLBACKS (Master Object)
 * ======================================================================== */

/* Called by driver to validate file write operations
 * Arguments: path, func, caller, owner, flags
 * Return: 1 to allow, 0 to deny
 */
valid_write(path, func, caller, owner, flags) {
    string caller_name, expected_path;
    int instr_result, priv_result;
    
    syslog("boot.c: valid_write() CALLED");
    syslog("boot.c:   path = " + path);
    syslog("boot.c:   func = " + func);
    syslog("boot.c:   caller = " + otoa(caller));
    
    /* System operations (caller is NULL) - always allow */
    if (!caller) {
        syslog("boot.c:   ALLOW: caller is NULL (system operation)");
        return 1;
    }
    
    /* Privileged objects - always allow */
    priv_result = priv(caller);
    syslog("boot.c:   priv(caller) = " + itoa(priv_result));
    if (priv_result) {
        syslog("boot.c:   ALLOW: caller has PRIV flag");
        return 1;
    }
    
    /* Boot object itself - always allow */
    if (caller == this_object()) {
        syslog("boot.c:   ALLOW: caller is boot object");
        return 1;
    }
    
    /* Players can only write to their own save file */
    caller_name = caller.query_name();
    if (caller_name) {
        expected_path = "/sys/data/players/" + caller_name + ".o";
        syslog("boot.c:   caller_name = " + caller_name);
        syslog("boot.c:   expected_path = " + expected_path);
        if (path == expected_path) {
            syslog("boot.c:   ALLOW: player writing own save file");
            return 1;
        }
    }
    
    /* Deny all other writes */
    syslog("boot.c:   DENY: no matching rule for path");
    return 0;
}

/* Called by driver to validate file read operations
 * Arguments: path, func, caller, owner, flags
 * Return: 1 to allow, 0 to deny
 */
valid_read(path, func, caller, owner, flags) {
    /* System operations - always allow */
    if (!caller) return 1;
    
    /* Privileged objects - always allow */
    if (priv(caller)) return 1;
    
    /* Allow all reads for now - we can tighten this later */
    return 1;
}

/* ========================================================================
 * SYSTEM INITIALIZATION
 * ======================================================================== */

/* Called when boot.c is first loaded */
static init() {
    object me;
    
    syslog("boot.c: init() called");
    
    me = this_object();
    
    /* Only run initialization on the prototype, not clones */
    if (prototype(this_object())) {
        syslog("boot.c: prototype check passed, scheduling finish_init");
        /* Autosave removed - use save_object() in mudlib for persistence */
        
        /* Make boot.c readable (but not writable) */
        chmod("/boot.c", PERM_READ);
        
        /* Initialize daemons after a brief delay */
        alarm(0, "finish_init");
    } else {
        /* Clones of boot.c should self-destruct */
        destruct(this_object());
    }
}

/* Called by alarm() to initialize daemons */
static finish_init() {
    object daemon, wizobj, room, test_obj;
    
    syslog("Compiling objects required for system operation...");
    
    /* Compile (don't clone) system prototypes */
    syslog("...object.c");
    compile_object(OBJECT_PATH);
    
    syslog("...living.c");
    compile_object(LIVING_PATH);
    
    syslog("...user.c");
    compile_object(USER_OB);

    syslog("...room.c");
    compile_object(ROOM_PATH);

    syslog("...player.c");
    compile_object(PLAYER_OB);
    
    /* Initialize command daemon - we DO want a clone here since it's a singleton daemon */
    syslog("...cmd_d.c");
    compile_object(CMD_D);
    daemon = atoo(CMD_D);
    if (daemon) {
        set_priv(daemon, 1);
    }
    
    /* Initialize users tracking daemon */
    syslog("...users_d.c");
    compile_object(USERS_D);
    daemon = atoo(USERS_D);
    if (daemon) {
        set_priv(daemon, 1);
    }
    
    /* Create initial wizard if needed */
    if (file_size("/sys/data/players/wizard.o") < 0) {
        wizobj = new(PLAYER_PATH);
        if (wizobj) {
            wizobj.set_name("wizard");
            wizobj.set_password("potrzebie");
            wizobj.set_role(ROLE_ADMIN);
            wizobj.save_data();
            syslog("Created initial wizard character (wizard/potrzebie)");
        } else {
            syslog("ERROR: Failed to create wizard character");
        }
    } else {
        syslog("Wizard character already exists");
    }
    
    /* Boot sequence complete */
    syslog("Boot sequence complete - all systems initialized");
    
    /* Note: Test suites can be run manually via eval command:
     *   eval new("/test/test_compile_string").run_tests();
     *   eval new("/test/test_eval_lifecycle").run_tests();
     *   eval new("/test/test_crypt").run_tests();
     *   eval new("/test/test_users_d").run_tests();
     */
}

/* ========================================================================
 * CONNECTION HANDLING
 * ======================================================================== */

/* Called when a new TCP connection is established
 * REQUIRED CALLBACK - NetCI driver calls this
 */
static connect() {
    object login_obj;
    
    /* Create a new user object for this connection */
    login_obj = new(USER_OB);
    if (!login_obj) {
        send_device("System error: Unable to create login object.\n");
        disconnect_device();
        return;
    }
    
    /* Transfer the connection from boot.c to the login object */
    if (reconnect_device(login_obj)) {
        send_device("System error: Unable to transfer connection.\n");
        disconnect_device();
        destruct(login_obj);
        return;
    }
    
    /* Grant privileges and start the login process */
    set_priv(login_obj, 1);
    
    /* Call connect() on the user object to start login flow */
    call_other(login_obj, "connect");
}

/* ========================================================================
 * SECURITY - FILE PERMISSIONS
 * ======================================================================== */

/* Called when any object attempts to read a file
 * REQUIRED CALLBACK - NetCI driver calls this
 *
 * Parameters:
 *   path   - File path being accessed
 *   func   - Function that triggered the read (e.g., "fread", "cat")
 *   caller - Object attempting the read (NULL for system)
 *   owner  - Object ID of file owner
 *   flags  - File permissions (-1 if doesn't exist, else bitmask)
 *
 * Returns: 1 to allow, 0 to deny
 */
valid_read(path, func, caller, owner, flags) {
    int caller_role;
    string caller_name;
    
    /* System operations - always allow */
    if (!caller) return 1;
    
    /* Privileged objects - always allow */
    if (priv(caller)) return 1;
    
    /* Non-existent files - allow read attempts (for error messages) */
    if (flags == -1) return 1;
    
    /* Get caller information */
    caller_role = call_other(caller, "query_role");
    if (!caller_role) {
        caller_role = ROLE_PLAYER;
    }
    
    caller_name = call_other(caller, "query_name");
    
    /* Admin: Full read access */
    if (caller_role >= ROLE_ADMIN) return 1;
    
    /* Wizard: Read all except /sys/data */
    if (caller_role >= ROLE_WIZARD) {
        if (path_starts_with(path, "/sys/data/")) return 0;
        return 1;
    }
    
    /* Programmer/Builder: Read all except /sys */
    if (caller_role >= ROLE_BUILDER) {
        if (path_starts_with(path, "/sys/")) return 0;
        return 1;
    }
    
    /* All players: Read access to /world and /etc */
    if (path_starts_with(path, "/world/")) return 1;
    if (path_starts_with(path, "/etc/")) return 1;
    
    /* Regular player: owner, home directory, or READ_OK flag */
    if (owner == otoi(caller)) return 1;
    if (caller_name && is_in_home_dir(path, caller_name)) return 1;
    if (flags & PERM_READ) return 1;
    
    /* Deny by default */
    return 0;
}

/* Called when any object attempts to write a file
 * REQUIRED CALLBACK - NetCI driver calls this
 *
 * Parameters: Same as valid_read()
 * Returns: 1 to allow, 0 to deny
 */
valid_write(path, func, caller, owner, flags) {
    int caller_role, caller_id;
    string caller_name;
    
    /* System operations - always allow */
    if (!caller) return 1;
    
    /* Privileged objects - always allow */
    if (priv(caller)) return 1;
    
    /* NEVER allow modification of boot.c */
    if (PROTECT_BOOT && path == "/boot.c") return 0;
    
    /* Get caller information */
    caller_id = otoi(caller);
    
    caller_role = call_other(caller, "query_role");
    if (!caller_role) {
        caller_role = ROLE_PLAYER;
    }
    
    caller_name = call_other(caller, "query_name");
    
    /* Admin: Full write access (except boot.c) */
    if (caller_role >= ROLE_ADMIN) {
        return 1;
    }
    
    /* Wizard: Write in /world and /cmds */
    if (caller_role >= ROLE_WIZARD) {
        if (PROTECT_SYS && path_starts_with(path, "/sys/")) return 0;
        if (path_starts_with(path, "/world/")) return 1;
        if (path_starts_with(path, "/cmds/")) return 1;
        /* Allow if owner or WRITE_OK */
        if (flags == -1) return 1;  /* New file */
        if (owner == caller_id) return 1;
        if (flags & PERM_WRITE) return 1;
        return 0;
    }
    
    /* Builder: Write in /world only */
    if (caller_role >= ROLE_BUILDER) {
        if (!path_starts_with(path, "/world/")) return 0;
        if (flags == -1) return 1;  /* New file */
        if (owner == caller_id) return 1;
        if (flags & PERM_WRITE) return 1;
        return 0;
    }
    
    /* Regular player: Write only in home directory */
    if (caller_name && is_in_home_dir(path, caller_name)) {
        if (flags == -1) return 1;  /* New file */
        if (owner == caller_id) return 1;
        if (flags & PERM_WRITE) return 1;
        return 0;
    }
    
    /* Deny by default */
    return 0;
}

/* ========================================================================
 * HELPER FUNCTIONS
 * ======================================================================== */

/* Check if path starts with prefix */
static path_starts_with(path, prefix) {
    int prefix_len;
    
    if (!path || !prefix) return 0;
    
    prefix_len = strlen(prefix);
    if (strlen(path) < prefix_len) return 0;
    
    return (leftstr(path, prefix_len) == prefix);
}

/* Check if path is in user's home directory */
static is_in_home_dir(path, username) {
    string path_user, rest;
    
    if (!path || !username) return 0;
    
    /* Check if path starts with /world/wiz/ */
    if (!path_starts_with(path, "/world/wiz/")) return 0;
    
    /* Extract username from path using sscanf */
    if (sscanf(path, "/world/wiz/%s/%s", path_user, rest) == 2) {
        /* Path has subdirectories: /world/wiz/username/... */
    } else if (sscanf(path, "/world/wiz/%s", path_user) == 1) {
        /* Path is exactly /world/wiz/username */
    } else {
        /* Invalid path format */
        return 0;
    }
    
    return (downcase(path_user) == downcase(username));
}

/* ========================================================================
 * DATABASE PERSISTENCE REMOVED - November 12, 2025
 * Use save_object()/restore_object() for selective persistence
 * See database-investigation.md for details
 * ======================================================================== */

/* ========================================================================
 * UTILITY FUNCTIONS
 * ======================================================================== */

/* For debugging - allow boot.c to receive messages */
listen(arg) {
    syswrite(arg);
}

/* Query functions for system information */
get_mud_name() {
    return MUD_NAME;
}

get_version() {
    return MUD_VERSION;
}

/* ========================================================================
 * PRIVILEGED WRAPPERS FOR AUTO OBJECT
 * These functions provide safe, centralized access to low-level efuns
 * for the auto object and other system code.
 * ======================================================================== */

/* Compile an object and return its prototype
 * Does NOT clone - just compiles and returns the prototype
 */
compile(path) {
    object caller;
    
    if (!path) return 0;
    
    caller = caller_object();
    
    /* Only allow privileged callers or auto.c */
    if (!priv(caller) && caller != this_object() && caller != atoo("/sys/auto")) {
        syslog("boot.compile(): DENIED - caller not privileged: " + otoa(caller));
        return 0;
    }
    
    compile_object(path);
    return atoo(path);
}

/* Clone an object - ensures prototype exists first, then clones
 * Returns a new instance
 */
clone(path) {
    object proto, caller;
    
    if (!path) return 0;
    
    caller = caller_object();
    
    /* Only allow privileged callers or auto.c */
    if (!priv(caller) && caller != this_object() && caller != atoo("/sys/auto")) {
        syslog("boot.clone(): DENIED - caller not privileged: " + otoa(caller));
        return 0;
    }
    
    /* Ensure prototype exists */
    proto = get_object(path);
    if (!proto) return 0;
    
    /* Clone it */
    return new(path);
}

/* Send a message to an object via its listen() function
 * Provides safe message delivery
 */
write(obj, msg) {
    object caller;
    
    if (!obj || !msg) return;
    
    caller = caller_object();
    
    /* Only allow privileged callers or auto.c */
    if (!priv(caller) && caller != this_object() && caller != atoo("/sys/auto")) {
        syslog("boot.write(): DENIED - caller not privileged: " + otoa(caller));
        return;
    }
    
    /* Call listen() on the target object */
    if (obj) {
        obj.listen(msg);
    }
}
