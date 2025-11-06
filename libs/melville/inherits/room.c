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
 * NOTE: This automatically attaches object.c and container.c
 */

#include <std.h>
#include <config.h>

/* Exits stored as a mapping: direction -> destination */
mapping exits;

/* Attached functionality */
object object_ob, container_ob;

/* Variables from attached objects */
object *inventory;  /* From container.c */
mapping properties; /* From object.c */

/* ========================================================================
 * INITIALIZATION
 * ======================================================================== */

static init() {
    /* Attach base object functionality */
    object_ob = new(OBJECT_PATH);
    if (object_ob) {
        attach(object_ob);
    }
    
    /* Attach container functionality */
    container_ob = new(CONTAINER_PATH);
    if (container_ob) {
        attach(container_ob);
    }
    
    /* Set room type */
    set_property("type", TYPE_ROOM);
    
    /* Initialize exits mapping */
    exits = ([]);
}

/* ========================================================================
 * EXIT MANAGEMENT
 * ======================================================================== */

/* Add an exit in a direction leading to a destination */
add_exit(direction, destination) {
    if (!direction || !destination) return;
    
    if (!exits) exits = ([]);
    
    exits[direction] = destination;
}

/* Remove an exit */
remove_exit(direction) {
    if (!direction) return;
    if (!exits) return;
    
    map_delete(exits, direction);
}

/* Query exit destination for a direction */
query_exit(direction) {
    if (!direction) return NULL;
    if (!exits) return NULL;
    
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

/* ========================================================================
 * ROOM MESSAGING
 * ======================================================================== */

/* Send a message to all players in the room
 * exclude: array of objects to exclude (optional)
 */
room_tell(msg, exclude) {
    int i;
    object ob;
    
    if (!msg) return;
    if (!inventory || sizeof(inventory) == 0) return;
    
    /* Send to each object in room */
    for (i = 0; i < sizeof(inventory); i++) {
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
    /* Mappings and arrays are automatically freed */
    exits = NULL;
    
    /* Clean up container data */
    destruct_container();
}
