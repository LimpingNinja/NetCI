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
 * - Inherits from /inherits/container.c for inventory
 * - Uses cmd_hook to route commands to command daemon
 * - Stores player data in /sys/data/players/
 */

#include <std.h>
#include <config.h>
#include <roles.h>

/* Player data (saved) - core fields only */
string player_name;
string player_password;  /* Hashed */
int player_role;

/* Runtime data (not saved) */
object user_ob;  /* Associated user connection object */
object auto_ob;   /* Auto functionality (utilities) */
object object_ob; /* Base object functionality */
object living_ob; /* Living functionality */
object *inventory; /* Inventory array for container functionality */
mapping properties; /* Property storage (from object.c pattern) */

/* ========================================================================
 * INITIALIZATION
 * ======================================================================== */

static init() {
    /* Initialize container and properties */
    if (!inventory) inventory = ({});
    if (!properties) properties = ([]);
    
    /* Attach auto functionality (for hash_password, etc.) */
    auto_ob = new(AUTO_OB);
    if (auto_ob) {
        if (attach(auto_ob)) {
            syslog("player.c: ERROR - attach(auto.c) failed");
        }
    } else {
        syslog("player.c: ERROR - failed to create auto.c");
    }
    
    /* Attach base object functionality (for properties, etc.) */
    object_ob = new(OBJECT_PATH);
    if (object_ob) {
        attach(object_ob);
    } else {
        syslog("player.c: ERROR - failed to create object.c");
    }
    
    /* Attach living functionality */
    living_ob = new(LIVING_PATH);
    if (living_ob) {
        attach(living_ob);
    } else {
        syslog("player.c: ERROR - failed to create living.c");
    }
    
    /* Set default values via properties */
    player_role = ROLE_PLAYER;
    set_property("type", TYPE_PLAYER);
    set_property("prevent_get", 1);
    set_property("brief_mode", 0);
    set_property("working_dir", "/");
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
    
    /* Mark this object as interactive (connected to a player) */
    set_interactive(1);
    
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
    
    /* Load and move to start room */
    start_room = atoo(entry_point);
    if (!start_room) {
        /* Try to compile it */
        start_room = new(entry_point);
    }
    if (!start_room) {
        /* Fallback to void if saved location doesn't exist */
        start_room = atoo(START_ROOM);
        if (!start_room) {
            start_room = new(START_ROOM);
        }
    }
    
    if (start_room) {
        string desc;
        
        move(start_room);
        
        /* Show room to player */
        brief = query_property("brief_mode");
        desc = call_other(start_room, "query_long", brief);
        if (desc && typeof(desc) == STRING_T) {
            send_device(desc);
            send_device("\n");
        } else {
            send_device("You are in the void.\n");
        }
    } else {
        send_device("Error: Unable to find starting location!\n");
        send_device("The void room at " + START_ROOM + " does not exist.\n");
    }
    
    /* Send prompt and start accepting commands */
    /* Use redirect_input() here because this_player() is still the user object */
    send_device("> ");
    redirect_input("cmd_hook");
}

/* Called when connection is lost */
disconnect() {
    object env;
    
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
 * COMMAND PROCESSING (cmd_hook pattern)
 * ======================================================================== */

/* This is called for each line of input from the player
 * We route it to the command daemon for processing
 */
cmd_hook(input) {
    object cmd_d;
    
    if (!input || strlen(input) == 0) {
        write("SHUT IT UP!\n");
        write("> ");
        input_to(this_object(), "cmd_hook");
        return;
    }
    
    /* Get command daemon */
    cmd_d = atoo(CMD_D);
    if (!cmd_d) {
        send_device("Error: Command daemon not found.\n");
        send_device("> ");
        input_to(this_object(), "cmd_hook");
        return; 
    }
    
    /* Let command daemon process the command */
    cmd_d.process_command(this_object(), input);
    
    /* Send prompt and wait for next command (unless we're quitting) */
    /* Use input_to() here because this_player() is now correctly the player object */
    if (input != "quit") {
        send_device("> ");
        input_to(this_object(), "cmd_hook");
    }
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
    /* Compare hashed passwords */
    return (hash_password(password) == player_password);
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

/* Save player data to file */
save_data() {
    string path, data, *prop_keys, key;
    object env;
    int i, value_int, result;
    string value_str;
    
    syslog("player.c: save_data() called for " + player_name);
    
    if (!player_name) {
        syslog("player.c: ERROR - no player_name set");
        return 0;
    }
    
    path = PLAYER_SAVE_DIR + player_name + ".o";
    syslog("player.c: saving to " + path);
    
    /* Build save data - core fields first */
    data = "";
    data = data + "player_name:" + player_name + "\n";
    data = data + "player_password:" + player_password + "\n";
    data = data + "player_role:" + itoa(player_role) + "\n";
    
    /* Save current location as property */
    env = location(this_object());
    if (env) {
        set_property("quit_location", otoa(env));
    }
    
    /* Iterate through all properties and save them */
    if (properties) {
        prop_keys = keys(properties);
        if (prop_keys && sizeof(prop_keys) > 0) {
            for (i = 0; i < sizeof(prop_keys); i++) {
                key = prop_keys[i];
                
                /* Get property value - could be int or string */
                if (typeof(properties[key]) == INT_T) {
                    value_int = properties[key];
                    data = data + "prop:" + key + ":int:" + itoa(value_int) + "\n";
                } else if (typeof(properties[key]) == STRING_T) {
                    value_str = properties[key];
                    data = data + "prop:" + key + ":str:" + value_str + "\n";
                }
                /* Add more types as needed (object, array, etc.) */
            }
        }
    }
    
    /* Write to file */
    syslog("player.c: writing " + itoa(strlen(data)) + " bytes to file");
    result = fwrite(path, data);
    if (result) {
        syslog("player.c: ERROR - fwrite failed with code " + itoa(result));
        return 0;  /* fwrite returns non-zero on error */
    }
    
    syslog("player.c: save_data() completed successfully");
    return 1;
}

/* Load player data from file */
load_data(name) {
    string path, data, *lines, line, *parts, key, type, value;
    int i, pos, colon_pos;
    
    if (!name) {
        syslog("player.c: load_data() - no name provided");
        return 0;
    }
    
    path = PLAYER_SAVE_DIR + name + ".o";
    syslog("player.c: load_data() - loading from " + path);
    
    /* Read entire file - fread reads one line at a time, so loop */
    pos = 0;
    data = "";
    line = fread(path, pos);
    while (line && line != 0 && line != -1) {
        data = data + line;
        line = fread(path, pos);
    }
    
    if (!data || strlen(data) == 0) {
        return 0;
    }
    
    /* Parse lines */
    lines = explode(data, "\n");
    if (!lines) return 0;
    
    for (i = 0; i < sizeof(lines); i++) {
        line = lines[i];
        if (line && strlen(line) > 0) {
            /* Split by colon */
            parts = explode(line, ":");
            if (parts && sizeof(parts) >= 2) {
                key = parts[0];
                
                /* Core fields */
                if (key == "player_name") {
                    player_name = parts[1];
                } else if (key == "player_password") {
                    player_password = parts[1];
                } else if (key == "player_role") {
                    player_role = atoi(parts[1]);
                } else if (key == "prop" && sizeof(parts) >= 4) {
                    /* Property format: prop:key:type:value */
                    key = parts[1];
                    type = parts[2];
                    value = parts[3];
                    
                    if (type == "int") {
                        set_property(key, atoi(value));
                    } else if (type == "str") {
                        set_property(key, value);
                    }
                    /* Add more types as needed */
                }
            }
        }
    }
    
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
        new_env = atoo(dest);
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

/* Capitalize first letter */
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
