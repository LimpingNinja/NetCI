/* player.c - Player body object for Melville/NetCI
 *
 * This is the player's in-world body (avatar). NOT the connection object.
 * The connection is handled by user.c.
 *
 * Responsibilities:
 * - In-world representation (location, inventory, description)
 * - Command processing (via cmd_hook pattern)
 * - Player data persistence (save/load)
 * - Movement and interaction
 * - Brief mode, working directory, etc.
 *
 * Architecture:
 * - Inherits from living.c (which inherits from object.c)
 * - Inherits from container.c (which inherits from object.c)
 * - Uses cmd_hook to route commands to command daemon
 * - Stores player data in /sys/data/players/
 */

#include   <std.h>
#include <config.h>
#include <roles.h>

inherit "/inherits/living";
inherit CONTAINER_PATH;

/* Player data (saved) - core fields only */
string player_name;
string player_password;  /* Hashed */
int player_role;

/* Runtime data (not saved) */
object user_ob;  /* Associated user connection object */

/* NOTE: inventory and properties are inherited from container.c and object.c */

/* ========================================================================
 * COMMAND PROCESSING (verb catch-all pattern)
 * ======================================================================== */

/* Catch-all verb handler called when no other verb matches
 * This is invoked via add_xverb("cmd_fallback", "") registered in init()
 * The empty string matches all input, checked last in verb search order
 * DEFINED BEFORE init() so it exists when add_xverb is called
 */
cmd_fallback(input) {
    object cmd_d;
    
    /* Empty input - just return success (prompt handled by engine) */
    if (!input || strlen(input) == 0) {
        return 1;
    }
    
    /* Get command daemon */
    cmd_d = atoo(CMD_D);
    if (!cmd_d) {
        send_device("Error: Command daemon not found.\n");
        return 0;
    }
    
    /* Delegate to command daemon - player context already set by engine */
    cmd_d.process_command(this_object(), input);
    
    /* Return 1 to indicate command was handled (prompt sent by engine) */
    return 1;
}

/* ========================================================================
 * INITIALIZATION
 * ======================================================================== */

static init() {
    /* Call each parent's init explicitly to avoid diamond problem
     * With multiple inheritance (living + container both inherit object),
     * we need to call each parent separately
     */
    
    /* Call living init (which calls object init) */
    living::init();
    
    /* Call container init (which also inherits from object, but won't double-init) */
    container::init();
    
    /* Set default player values */
    player_role = ROLE_PLAYER;
    set_property("type", TYPE_PLAYER);
    set_property("prevent_get", 1);
    set_property("brief_mode", 0);
    set_property("working_dir", "/");
    
    /* Register catch-all verb to route commands through cmd_d
     * Empty string "" matches all input as prefix (checked last in verb search)
     * Only register on clones, not on the prototype
     */
    if (!prototype(this_object())) {
        add_xverb("", "cmd_fallback");
    }
}

/* ========================================================================
 * CONNECTION HANDLING
 * ======================================================================== */

/* Called when connection is transferred from user.c
 * This is AFTER reconnect_device() has transferred the connection
 */
connect() {
    object start_room;
    string entry_point, desc;
    int brief;
    mapping term_info;
    string term_client, term_type, color_support;
    int term_support;
    
    /* Display terminal info (MTTS negotiation complete by now) */
    term_info = query_terminal(this_object());
    if (term_info) {
        term_client = term_info["term_client"];
        term_type = term_info["term_type"];
        term_support = term_info["term_support"];
        
        /* Determine color support */
        if (term_support >= 8) {
            /* MTTS bit 3 = 256 colors */
            color_support = "ANSI 256 colors (MTTS)";
        } else if (term_type == "XTERM") {
            color_support = "ANSI colors (xterm)";
        } else if (term_type == "ANSI") {
            color_support = "ANSI colors";
        } else if (term_type == "VT100") {
            color_support = "ANSI colors (VT100)";
        } else {
            color_support = "unknown";
        }
        
        send_device("\n");
        send_device("[ Terminal ] Client: " + (term_client != "" ? term_client : "unknown") + "\n");
        send_device("[ Terminal ] Type: " + (term_type != "" ? term_type : "unknown") + "\n");
        if (term_support > 0) {
            send_device(sprintf("[ Terminal ] MTTS: %d\n", term_support));
        }
        send_device("[ Terminal ] Colors: " + color_support + "\n");
        send_device("\n");
    }
    
    /* Mark this object as interactive (connected to a player) */
    set_interactive(1);
    
    /* Enable heartbeat for testing (2 second interval) */
    set_heart_beat(2);
    
    /* Set up IDs */
    set_id(({ player_name }));
    
    /* Set short/long descriptions */
    set_short(capitalize(player_name) + " the player");
    desc = query_property("description");
    if (desc) {
        set_long(desc);
    } else {
        set_long("You see " + capitalize(player_name) + ", a player.");
    }
    
    /* Determine entry point */
    entry_point = query_property("quit_location");
    if (!entry_point || entry_point == "") {
        entry_point = START_ROOM;
    }
    
    /* Load start room - get_object() will compile if needed */
    start_room = get_object(entry_point);
    
    /* Fallback to START_ROOM if saved location doesn't exist/compile */
    if (!start_room && entry_point != START_ROOM) {
        start_room = get_object(START_ROOM);
    }
    
    /* Move to room or show error */
    if (start_room) {
        move(start_room);
    } else {
        send_device("Error: Unable to find starting location!\n");
        send_device("The void room at " + START_ROOM + " does not exist.\n");
    }
    
    /* Send initial prompt - commands automatically route through verb system */
    send_prompt("> ");
}

/* Called when connection is lost */
disconnect() {
    object env;
    object users_d;
    
    /* Notify users daemon of logout */
    users_d = atoo(USERS_D);
    if (users_d) {
        call_other(users_d, "user_logged_out", this_object());
    }
    
    /* Save current location */
    env = location(this_object());
    if (env) {
        set_property("quit_location", otoa(env));
        
        /* Announce departure */
        env.room_tell(capitalize(player_name) + " leaves the game.\n",
                     ({ this_object() }));
    }
    
    /* Save player data */
    save_data();
    
    /* Destruct player body */
    destruct(this_object());
}

/* ========================================================================
 * OUTPUT CALLBACKS
 * ======================================================================== */

/* listen() - Required callback for receiving output from engine and commands */
listen(msg) {
    send_device(msg);
}

/* ========================================================================
 * PLAYER DATA MANAGEMENT
 * ======================================================================== */

/* Set player name */
set_name(name) {
    player_name = name;
    set_id(({ name }));
}

/* Query player name */
query_name() {
    return player_name;
}

/* Set password (hashed) - uses hash_password from auto.c */
set_password(password) {
    player_password = hash_password(password);
}

/* Check password */
check_password(password) {
    /* Verify password using bcrypt (from auto.c) */
    return verify_password(password, player_password);
}

/* Set role */
set_role(role) {
    player_role = role;
}

/* Query role */
query_role() {
    return player_role;
}

/* Check if player is wizard+ */
query_wizard() {
    return (player_role >= ROLE_WIZARD);
}

/* Check if player is admin */
query_admin() {
    return (player_role >= ROLE_ADMIN);
}

/* Title - use properties */
set_title(title) {
    set_property("title", title);
}

query_title() {
    return query_property("title");
}

/* Description - use properties and set_long */
set_description(desc) {
    set_property("description", desc);
    set_long(desc);
}

query_description() {
    return query_property("description");
}

/* Brief mode - use properties */
set_brief(flag) {
    set_property("brief_mode", flag);
}

query_brief() {
    return query_property("brief_mode");
}

/* Working directory - use properties */
set_cwd(dir) {
    set_property("working_dir", dir);
}

query_cwd() {
    return query_property("working_dir");
}

/* ========================================================================
 * PERSISTENCE (SAVE/LOAD)
 * ======================================================================== */

/* Save player data using save_object() */
save_data() {
    object env;
    
    if (!player_name) {
        syslog("player.c: ERROR - no player_name set");
        return 0;
    }
    
    /* Save current location as property before saving */
    env = location(this_object());
    if (env) {
        set_property("quit_location", otoa(env));
    }
    
    /* Use save_object() efun to serialize all globals */
    if (!save_object("/players/" + player_name)) {
        syslog("player.c: ERROR - save_object() failed for " + player_name);
        return 0;
    }
    return 1;
}

/* Load player data using restore_object() */
load_data(name) {
    if (!name) {
        syslog("player.c: load_data() - no name provided");
        return 0;
    }
    
    /* Set name first so restore_object can use it */
    player_name = name;
    
    /* Use restore_object() efun to deserialize all globals */
    if (!restore_object("/players/" + name)) {
        syslog("player.c: load_data() - restore_object() failed for " + name);
        return 0;
    }
    
    /* Register catch-all verb (restore_object doesn't call init()) */
    add_xverb("", "cmd_fallback");
    
    return 1;
}

/* ========================================================================
 * MOVEMENT
 * ======================================================================== */

/* Override move to add player-specific behavior */
move(dest) {
    object old_env, new_env;
    int result, brief;
    
    if (!dest) return 0;
    
    /* Convert string to object if needed */
    if (typeof(dest) == STRING_T) {
        new_env = get_object(dest);
        if (!new_env) return 0;
    } else {
        new_env = dest;
    }
    
    old_env = location(this_object());
    
    /* Move to new environment using driver efun */
    result = move_object(this_object(), new_env);
    
    if (!result) {
        /* Show new room (move_object returns 0 on success) */
        brief = query_property("brief_mode");
        send_device(new_env.query_long(brief));
        send_device("\n");
        return 1;
    }
    
    return 0;  /* move_object returns non-zero on failure */
}

/* ========================================================================
 * UTILITY FUNCTIONS
 * ======================================================================== */

/* NOTE: capitalize() is available from auto.c (automatically attached) */

/* Note: listen(), catch_tell(), write(), query_living() provided by living.c */

/* ========================================================================
 * RESTRICTIONS
 * ======================================================================== */

/* Players can't be picked up */
prevent_get() {
    return 1;
}

get() {
    return 0;
}

/* ========================================================================
 * HEARTBEAT (for testing periodic systems)
 * ======================================================================== */

heart_beat() {
   /* just a placeholder */
}
