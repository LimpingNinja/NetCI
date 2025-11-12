/* void.c - The Void (starting room)
 *
 * This is the default starting location for new players.
 * A simple, safe room to begin their adventure.
 */


#include <std.h>
#include <config.h>
inherit "/inherits/room";

init() {
    /* Call parent init (room, which calls object and container) */
    ::init();
    
    /* Set room descriptions */
    set_short("The Void");
    set_long("You are floating in an endless void. Darkness surrounds you in all "+
             "directions. This appears to be the beginning of your journey.\n");
    
    /* No exits yet - will add when we build more rooms */
}
