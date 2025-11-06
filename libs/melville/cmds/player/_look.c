/* _look.c - Look command
 *
 * Syntax: look [at <object>]
 *
 * Look at the room or a specific object.
 */

#include <std.h>
#include <config.h>

/* Main command execution */
execute(player, args) {
    object env, target;
    string desc;
    
    if (!player) return 0;
    
    /* No arguments - look at room */
    if (!args || args == "") {
        env = location(player);
        if (!env) {
            player.listen("You are nowhere!\n");
            return 1;
        }
        
        /* Get room description (respects brief mode) */
        desc = env.query_long(player.query_brief());
        player.listen(desc);
        return 1;
    }
    
    /* Remove "at" if present */
    if (leftstr(args, 3) == "at ") {
        args = rightstr(args, strlen(args) - 3);
        args = trim(args);
    }
    
    /* Look at specific object */
    target = find_object(args);
    if (!target) {
        player.listen("You don't see that here.\n");
        return 0;
    }
    
    desc = target.query_long();
    if (!desc) {
        desc = "You see nothing special.\n";
    }
    
    player.listen(desc);
    return 1;
}

/* Help text */
query_help() {
    return "Syntax: look [at <object>]\n\n"
           "Look at your surroundings or a specific object.\n\n"
           "Examples:\n"
           "  look          - Look at the room\n"
           "  look sword    - Look at a sword\n"
           "  look at box   - Look at a box\n";
}
