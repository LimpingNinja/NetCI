/* _who.c - Who command
 *
 * Syntax: who
 *
 * List all connected players.
 */

#include <std.h>
#include <config.h>

/* Main command execution */
do_command(player, args) {
    object *connected_users;
    string output, name;
    int count, i;
    if (!player) return 0;
    
    output = "Players currently online:\n";
    output = output + "------------------------\n";
    
    count = 0;
    
    /* Use new users() efun to get array of all connected players */
    connected_users = users();
    
    if (connected_users && sizeof(connected_users) > 0) {
        for (i = 0; i < sizeof(connected_users); i++) {
            /* Check if it's a player object (call_other returns 0 on failure) */
            if (call_other(connected_users[i], "query_living")) {
                name = call_other(connected_users[i], "query_name");
                if (name) {
                    string title;
                    
                    output = output + "  " + capitalize(name);
                    
                    /* Add title if they have one */
                    title = call_other(connected_users[i], "query_title");
                    if (title) {
                        output = output + " " + title;
                    }
                    
                    output = output + "\n";
                    count++;
                }
            }
        }
    }
    
    output = output + "------------------------\n";
    output = output + "Total: " + itoa(count) + " player";
    if (count != 1) {
        output = output + "s";
    }
    output = output + " online.\n";
    
    write(output);
    return 1;
}

/* Help text */
query_help() {
    return "List connected players\n"+
           "Category: information\n\n"+
           "Usage:\n"+
           "  who\n\n"+
           "Shows all players currently connected to the game.\n";
}
