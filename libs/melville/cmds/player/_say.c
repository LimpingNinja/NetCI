/* _say.c - Say command
 *
 * Syntax: say <message>
 *         '<message>
 *
 * Say something to everyone in the room.
 */

#include <std.h>
#include <config.h>

/* Main command execution */
do_command(player, args) {
    object env;
    string name, message;
    if (!player) return 0;
    
    if (!args || args == "") {
        write("Say what?\n");
        return 0;
    }
    
    message = args;
    name = capitalize(player.query_name());
    
    /* Get current room */
    env = location(player);
    if (!env) {
        write("You are nowhere!\n");
        return 0;
    }
    
    /* Send message to player */
    write("You say: " + message + "\n");
    
    /* Send message to room (excluding player) */
    env.room_tell(name + " says: " + message + "\n", ({ player }));
    
    return 1;
}

/* Help text */
query_help() {
    return "Speak to everyone in the room\n"+
           "Category: communication\n\n"+
           "Usage:\n"+
           "  say <message>\n"+
           "  '<message>\n\n"+
           "Say something to everyone in the room.\n\n"+
           "Examples:\n"+
           "  say Hello everyone!\n"+
           "  'Hello everyone!\n";
}
