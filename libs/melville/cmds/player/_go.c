/* _go.c - Movement command
 *
 * Syntax: go <direction>
 *         <direction>
 *
 * Move in a direction.
 */

#include <std.h>
#include <config.h>

/* Main command execution */
do_command(player, args) {
    object env, dest;
    string direction;
    if (!player) return 0;
    
    if (!args || args == "") {
        write("Go where?\n");
        return 0;
    }
    
    direction = args;
    
    /* Get current room */
    env = location(player);
    if (!env) {
        write("You are nowhere!\n");
        return 0;
    }
    
    /* Try to find exit */
    dest = env.query_exit(direction);
    if (!dest) {
        write("You can't go that way.\n");
        return 0;
    }
    
    /* Attempt movement */
    if (!player.move(dest)) {
        write("You can't go that way.\n");
        return 0;
    }
    
    /* Announce departure to old room (use custom message if set) */
    env.room_tell(env.get_departure_message(player.query_name(), direction),
                  ({ player }));
    
    /* Announce arrival to new room */
    dest = location(player);
    if (dest) {
        dest.room_tell(capitalize(player.query_name()) + " arrives.\n",
                      ({ player }));
    }
    
    return 1;
}

/* Help text */
query_help() {
    return "Move in a direction\n"+
           "Category: movement\n\n"+
           "Usage:\n"+
           "  go <direction>\n"+
           "  <direction>\n\n"+
           "Move in a specified direction.\n\n"+
           "Valid directions:\n"+
           "  north, south, east, west, up, down\n"+
           "  n, s, e, w, u, d\n\n"+
           "Examples:\n"+
           "  go north\n"+
           "  n\n";
}
