/* _quit.c - Quit command
 *
 * Syntax: quit
 *
 * Disconnect from the game.
 */

#include <std.h>
#include <config.h>

/* Main command execution */
do_command(player, args) {
    object env;
    if (!player) return 0;
    
    /* Get current location */
    env = location(player);
    
    /* Announce departure */
    if (env) {
        env.room_tell(capitalize(player.query_name()) + " leaves the game.\n",
                     ({ player }));
    }
    
    /* Save player data */
    player.save_data();
    
    /* Send goodbye message */
    write("\nGoodbye! Thanks for playing.\n\n");
    
    /* Disconnect */
    disconnect_device();
    
    return 1;
}

/* Help text */
query_help() {
    return "Disconnect from the game\n"+
           "Category: general\n\n"+
           "Usage:\n"+
           "  quit\n\n"+
           "Disconnects you from the game.\n"+
           "Your character will be saved automatically.\n";
}
