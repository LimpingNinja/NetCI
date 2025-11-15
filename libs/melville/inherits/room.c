/* room.c - Room object for Melville/NetCI
 *
 * This inheritable provides room functionality for objects that contain
 * players and other objects. Features:
 * - Exit handling (directions and destinations)
 * - Room descriptions with exit listing
 * - Content listing (objects and players in room)
 * - Room-wide messaging
 * - Brief mode support
 *
 * Rooms should NOT be movable (they are the environment).
 *
 * NOTE: This inherits from object.c and container.c
 */

#include <std.h>
#include <config.h>

/* Only inherit container - it already inherits from obj */
inherit CONTAINER_PATH;

/* Exits stored as a mapping: direction -> destination */
mapping exits;

/* Custom exit messages: direction -> message
 * If set, overrides default "leaves [direction]" message
 * Use $N for player name placeholder
 */
mapping exit_messages;

/* ========================================================================
 * INITIALIZATION
 * ======================================================================== */

static init() {
    /* Call parent init explicitly */
    container::init();
    
    /* Set room type */
    set_property("type", TYPE_ROOM);
    
    /* Initialize exits mapping */
    exits = ([ ]);
    exit_messages = ([]);
    
    /* Log room initialization for testing */
    syslog(sprintf("Room init: %O created at time %d\n", this_object(), time()));
}

/* ========================================================================
 * EXIT MANAGEMENT
 * ======================================================================== */

/* Add an exit in a direction leading to a destination */
add_exit(direction, destination) {
    if (!direction || !destination) return;
    
    exits[direction] = destination;
}

/* Remove an exit */
remove_exit(direction) {
    if (!direction) return;
    
    map_delete(exits, direction);
}

/* Query exit destination for a direction
 * Returns destination (string path or object)
 * move() will handle compilation if it's a string
 */
query_exit(direction) {
    if (!direction) return NULL;
    
    return exits[direction];
}

/* Query all exits - returns mapping */
query_exits() {
    if (!exits) return ([]);
    return exits;
}

/* Get exit directions as array */
query_exit_directions() {
    if (!exits) return ({});
    return keys(exits);
}

/* Get exit count */
query_exit_count() {
    if (!exits) return 0;
    return sizeof(exits);
}

/* Set custom exit message for a direction
 * Use $N for player name placeholder
 * Example: set_exit_message("down", "$N climbs down the ladder.")
 */
set_exit_message(direction, message) {
    if (!direction || !message) return;
    exit_messages[direction] = message;
}

/* Query exit message for a direction */
query_exit_message(direction) {
    if (!direction) return NULL;
    return exit_messages[direction];
}

/* Get the departure message for a player leaving in a direction
 * Returns formatted message with player name substituted
 */
get_departure_message(player_name, direction) {
    string msg;
    
    if (!player_name || !direction) return NULL;
    
    /* Check for custom exit message */
    if (exit_messages) {
        msg = exit_messages[direction];
        if (msg) {
            /* Replace $N with player name */
            msg = replace_string(msg, "$N", capitalize(player_name));
            /* Ensure it ends with newline */
            if (rightstr(msg, 1) != "\n") {
                msg = msg + "\n";
            }
            return msg;
        }
    }
    
    /* Default message */
    return capitalize(player_name) + " leaves " + direction + ".\n";
}

/* ========================================================================
 * ROOM DESCRIPTIONS
 * ======================================================================== */

/* Override query_long to include exits and contents
 * brief: 1 for brief mode (short desc only), 0 for full description
 */
query_long(brief) {
    string desc, exit_desc, content_desc;
    int exit_count;
    
    /* Get base description */
    if (brief) {
        desc = get_property("short");
        if (desc) desc = desc + "\n";
    } else {
        desc = get_property("long");
        if (!desc) desc = "You see nothing special.\n";
    }
    
    /* Add exit description */
    exit_desc = get_exit_description();
    if (exit_desc) {
        desc = desc + exit_desc;
    }
    
    /* Add contents description (if not brief) */
    if (!brief) {
        content_desc = get_content_description();
        if (content_desc) {
            desc = desc + content_desc;
        }
    }

    return desc;
}

get_long(brief) {
    return query_long(brief);
}

/* Build exit description string */
get_exit_description() {
    int count, i;
    string *directions, list;
    
    if (!exits) return "There are no obvious exits.\n";
    
    count = sizeof(exits);
    if (count == 0) {
        return "There are no obvious exits.\n";
    }
    
    directions = keys(exits);
    
    if (count == 1) {
        return "The only obvious exit is " + directions[0] + ".\n";
    }
    
    if (count == 2) {
        return "Obvious exits are " + directions[0] + " and " + directions[1] + ".\n";
    }
    
    /* Three or more exits - build array manually since NetCI doesn't support slicing */
    string *first_exits;
    int j;
    
    first_exits = ({});
    for (j = 0; j < count - 1; j++) {
        first_exits = first_exits + ({ directions[j] });
    }
    
    list = implode(first_exits, ", ");
    list = list + ", and " + directions[count-1];
    
    return "Obvious exits are " + list + ".\n";
}

/* Build content description (objects and players in room) */
get_content_description() {
    int i;
    object ob;
    string desc, short;
    
    if (!inventory || sizeof(inventory) == 0) return NULL;
    
    desc = "";
    
    for (i = 0; i < sizeof(inventory); i++) {
        ob = inventory[i];
        if (ob && ob != this_player()) {
            /* Get object's short description */
            short = call_other(ob, "query_short");
            if (short && typeof(short) == STRING_T) {
                /* Capitalize first letter */
                desc = desc + "   " + capitalize(short) + "\n";
            }
        }
    }
    
    return desc;
}

/* NOTE: capitalize() is available from auto.c (automatically attached) */

/* ========================================================================
 * ROOM MESSAGING
 * ======================================================================== */

/* Send a message to all players in the room
 * exclude: array of objects to exclude (optional)
 */
room_tell(msg, exclude) {
    int i, count;
    object ob;
    
    if (!msg) return;
    if (!inventory) return;
    
    count = sizeof(inventory);
    if (count == 0) return;
    
    /* Send to each object in room */
    for (i = 0; i < count; i++) {
        ob = inventory[i];
        if (ob) {
            /* Check if this object is excluded */
            if (!exclude || member_array(ob, exclude) == -1) {
                /* Send message if object can receive it */
                call_other(ob, "listen", msg);
            }
        }
    }
}

/* ========================================================================
 * ROOM RESTRICTIONS
 * ======================================================================== */

/* Rooms cannot be picked up */
get() {
    return 0;
}

prevent_get() {
    return 1;
}

/* Rooms cannot be dropped */
drop() {
    return 0;
}

/* Rooms should not move */
move(dest) {
    /* Rooms are stationary */
    return 0;
}

/* ========================================================================
 * CLEANUP
 * ======================================================================== */

/* Clean up room data when destructed */
destruct_room() {
    /* Clean up container data */
    destruct_container();
}

/* ========================================================================
 * PERIODIC APPLIES (for testing periodic systems)
 * ======================================================================== */

/* reset() - Called every 800 seconds (13 minutes) by the driver
 * Use this to respawn items, reset puzzles, restore room state, etc.
 */
reset() {
    int idle;
    
    idle = query_idle_time();
    syslog(sprintf("Room reset: %O called at time %d (idle: %d seconds)\n", 
                   this_object(), time(), idle));
}

/* clean_up() - Called every 1200 seconds (20 minutes) on idle rooms
 * Return 1 to allow destruction, 0 to stay loaded
 * We opt-in (return 0) to prevent destruction during testing
 */
clean_up(refs) {
    int idle;
    object *inv;
    
    idle = query_idle_time();
    syslog(sprintf("Room clean_up: %O called at time %d (idle: %d seconds, refs: %d))\n", 
                   this_object(), time(), idle, refs));   
    /* Return 0 = opt-in, stay loaded (for testing) */
    /* Don't clean up rooms with players */
    inv = query_inventory();
    if (sizeof(inv) > 0) {
        syslog("Room clean_up: has inventory, staying loaded");
        return 0;  /* Keep loaded */
    }
    syslog("Room clean_up: We are flagged for cleanup.");
    return 1;
}
