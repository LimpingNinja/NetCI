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
        /* Start autosave timer */
        alarm(SAVE_INTERVAL, "save_db");
        
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
    object daemon, wizobj, room;
    
    syslog("Compiling objects required for system operation...");
    
    /* Initialize command daemon */
    syslog("...cmd_d.c");
    daemon = new(CMD_D);
    if (daemon) {
        set_priv(daemon, 1);
    }
    syslog("...auto.c");
    new(OBJECT_PATH);

    syslog("...object.c");
    new(OBJECT_PATH);
    
    syslog("...living.c");
    new(LIVING_PATH);
    
    syslog("...user.c");
    new(USER_OB);

    syslog("...room.c");
    new(ROOM_PATH);

    syslog("...player.c");
    
    /* Check if wizard player file exists */
    if (fstat("/sys/data/players/wizard.o") == -1) {
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
    }
    
    /* Note: log_d and user_d don't exist yet - we'll add them later */
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
    int next_slash;
    string path_user;
    
    if (!path || !username) return 0;
    
    /* Check if path starts with /world/wiz/ */
    if (!path_starts_with(path, "/world/wiz/")) return 0;
    
    /* Extract username from path */
    next_slash = instr(path, 12, "/");  /* 12 = length of "/world/wiz/" */
    if (!next_slash) {
        /* Path is exactly /world/wiz/username */
        path_user = rightstr(path, strlen(path) - 11);
    } else {
        /* Path is /world/wiz/username/... */
        path_user = midstr(path, 12, next_slash - 12);
    }
    
    return (downcase(path_user) == downcase(username));
}

/* ========================================================================
 * AUTOSAVE
 * ======================================================================== */

/* Called periodically by alarm() to save the database */
save_db() {
    object curr;
    
    /* Notify all connected players */
    curr = NULL;
    while (curr = next_who(curr)) {
        call_other(curr, "listen", "\n*** Autosaving Database ***\n");
    }
    
    /* Flush output buffers */
    flush_device();
    
    /* Trigger database save */
    sysctl(0);
    
    /* Schedule next save */
    alarm(SAVE_INTERVAL, "save_db");
}

/* ========================================================================
 * UTILITY FUNCTIONS
 * ======================================================================== */

/* For debugging - allow boot.c to receive messages */
listen(arg) {
    send_device(arg);
}

/* Query functions for system information */
get_mud_name() {
    return MUD_NAME;
}

get_version() {
    return MUD_VERSION;
}
