/* bedroom.c - Wizard's Bedroom
 *
 * A peaceful sleeping chamber with simple comforts. An ancient wardrobe
 * stands sentinel against one wall, holding the wizard's modest
 * collection of robes and personal effects.
 */

#include <std.h>
#include <config.h>
inherit ROOM_PATH;

init() {
    /* Call parent init */
    ::init();
    
    /* Set room descriptions */
    set_short("A Simple Bedroom");
    
    set_long(
        "This chamber offers a stark contrast to the organized chaos of the main "+
        "dwelling. The space is small and spare, carved from the same living stone "+
        "but softened with carefully chosen comforts. A narrow bed rests against the "+
        "southern wall, covered with a patchwork quilt that shows the careful mending "+
        "of many years. The pillow is modest but well-worn, shaped by countless nights "+
        "of contemplation before sleep.\n\n"+
        
        "An ancient wardrobe dominates the western wall, its dark wood carved with "+
        "protective symbols that have nearly faded with age. The craftsmanship suggests "+
        "it's far older than the dwelling itself - perhaps salvaged from some previous "+
        "life or inherited from a long-dead master. Its brass hinges are well-oiled, "+
        "and it opens with barely a whisper. Within hang several robes of different "+
        "weights and seasons: heavy wool for winter, light linen for summer, and a "+
        "midnight blue ceremonial robe reserved for rare occasions.\n\n"+
        
        "A small window, barely more than an arrow slit, admits a shaft of natural "+
        "light that tracks across the floor with the sun's passage. On the narrow "+
        "nightstand sits a candle, a book of meditative poetry, and a clay cup that "+
        "still holds the dregs of yesterday's tea. The air here is cool and still, "+
        "scented faintly with lavender from a small sachet tucked beneath the pillow.\n\n"+
        
        "This is a room for rest and restoration, free from the demands of scholarship "+
        "and experimentation. A place where even a hermit wizard can simply be human.\n"
    );
    
    /* Add exit back to home */
    add_exit("east", "/world/home/wizard/home");
    
    /* Custom exit message */
    set_exit_message("east", "$N pushes aside the tapestry and returns to the main chamber.");
}
