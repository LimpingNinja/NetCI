/* _who.c - Who command
 *
 * Syntax: who
 *
 * List all connected players.
 */

#include <std.h>
#include <config.h>

/* Main command execution */
execute(args) {
    object player, curr;
    string output, name;
    int count;
    
    player = this_player();
    if (!player) return 0;
    
    output = "Players currently online:\n";
    output = output + "------------------------\n";
    
    count = 0;
    
    /* Iterate through all connected objects using next_who() */
    curr = next_who(NULL);  /* NULL returns first connected object */
    
    while (curr) {
        /* Check if it's a player object */
        if (curr.query_living && curr.query_living()) {
            name = curr.query_name();
            if (name) {
                output = output + "  " + capitalize(name);
                
                /* Add title if they have one */
                if (curr.query_title && curr.query_title()) {
                    output = output + " " + curr.query_title();
                }
                
                output = output + "\n";
                count++;
            }
        }
        
        /* Get next connected object */
        curr = next_who(curr);
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
    return "Syntax: who\n\n"+
           "List all players currently connected to the game.\n";
}
