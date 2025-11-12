/* home.c - Hermit Wizard's Home
 *
 * A cozy, cluttered dwelling of a wise hermit wizard living in quiet
 * contemplation. Ancient books, curious artifacts, and the scent of
 * herbal tea fill the air. To the east lies the alchemical laboratory,
 * and to the west, the private bedroom.
 */

#include <std.h>
#include <config.h>
inherit ROOM_PATH;

init() {
    /* Call parent init */
    ::init();
    
    /* Set room descriptions */
    set_short("A Hermit Wizard's Home");
    
    set_long(
        "You stand in the heart of a hermit wizard's dwelling, a circular chamber "+
        "carved from living stone. Warm amber light emanates from crystalline orbs "+
        "suspended by unseen forces near the domed ceiling. Ancient tomes are stacked "+
        "in precarious towers along the curved walls, their leather spines cracked "+
        "with age. A worn reading chair sits beside a small iron stove, where a "+
        "copper kettle perpetually steams with fragrant herbal tea.\n\n"+
        
        "Curious artifacts clutter every available surface: an astrolabe of unknown "+
        "origin, dried herbs hanging from brass hooks, star charts marked with cryptic "+
        "annotations, and a collection of river stones arranged in mysterious patterns. "+
        "A faded tapestry depicting celestial movements hangs askew on the western wall, "+
        "partially concealing the entrance to the sleeping quarters. To the east, through "+
        "an arched doorway, you can see the flickering glow of alchemical flames.\n\n"+
        
        "The air carries the mingled scents of old parchment, dried lavender, and "+
        "something indefinably magical. Despite the clutter, there's a sense of careful "+
        "arrangement - chaos that conceals a deeper order.\n"
    );
    
    /* Add exits */
    add_exit("east", "/world/home/wizard/alchemy");
    add_exit("west", "/world/home/wizard/bedroom");
    
    /* Custom exit messages */
    set_exit_message("east", "$N carefully steps through the arched doorway into the laboratory.");
    set_exit_message("west", "$N pulls aside the tapestry and slips into the bedroom.");
}
