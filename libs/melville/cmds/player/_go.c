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
execute(args) {
    object player, env, dest;
    string direction;
    
    player = this_player();
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
    
    /* Announce departure to old room */
    env.room_tell(capitalize(player.query_name()) + " leaves " + direction + ".\n",
                  ({ player }));
    
    /* Show new room */
    dest = location(player);
    if (dest) {
        write(dest.query_long(player.query_brief()));
        
        /* Announce arrival to new room */
        dest.room_tell(capitalize(player.query_name()) + " arrives.\n",
                      ({ player }));
    }
    
    return 1;
}

/* Help text */
query_help() {
    return "Syntax: go <direction>\n"+
           "        <direction>\n\n"+
           "Move in a direction.\n\n"+
           "Common directions: north, south, east, west, up, down, "+
           "northeast, northwest, southeast, southwest\n\n"+
           "Examples:\n"+
           "  go north\n"+
           "  north\n"+
           "  n\n";
}
