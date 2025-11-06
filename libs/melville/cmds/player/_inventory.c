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
execute(player, args) {
    object *inv;
    int i;
    string output;
    
    if (!player) return 0;
    
    /* Get inventory */
    inv = player.query_inventory();
    
    if (!inv || sizeof(inv) == 0) {
        player.write("You aren't carrying anything.\n");
        return 1;
    }
    
    /* Build inventory list */
    output = "You are carrying:\n";
    for (i = 0; i < sizeof(inv); i++) {
        if (inv[i]) {
            output = output + "  " + inv[i].query_short() + "\n";
        }
    }
    
    player.write(output);
    return 1;
}

/* Help text */
query_help() {
    return "Syntax: inventory\n"
           "        inv\n"
           "        i\n\n"
           "List what you're carrying.\n";
}
