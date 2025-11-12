/* _inventory.c - Inventory command
 *
 * Syntax: inventory
 *         inv
 *         i
 *
 * List what you're carrying.
 */

#include <std.h>
#include <config.h>

/* Main command execution */
do_command(player, args) {
    object *inv;
    int i;
    string output;
    if (!player) return 0;
    
    /* Get inventory */
    inv = player.query_inventory();
    
    if (!inv || sizeof(inv) == 0) {
        write("You aren't carrying anything.\n");
        return 1;
    }
    
    /* Build inventory list */
    output = "You are carrying:\n";
    for (i = 0; i < sizeof(inv); i++) {
        if (inv[i]) {
            output = output + "  " + inv[i].query_short() + "\n";
        }
    }
    
    write(output);
    return 1;
}

/* Help text */
query_help() {
    return "List your items\n"+
           "Category: inventory\n\n"+
           "Usage:\n"+
           "  inventory\n"+
           "  i\n\n"+
           "Shows all items you are currently carrying.\n";
}
