/* alchemy.c - Alchemical Laboratory
 *
 * A well-organized alchemical workshop filled with bubbling retorts,
 * carefully labeled ingredients, and the systematic pursuit of
 * transmutation. The domain of careful experimentation and
 * methodical research.
 */

#include <std.h>
#include <config.h>
inherit ROOM_PATH;

init() {
    /* Call parent init */
    ::init();
    
    /* Set room descriptions */
    set_short("An Alchemical Laboratory");
    
    set_long(
        "The laboratory is a testament to centuries of methodical experimentation. "+
        "Unlike the cluttered living quarters to the west, here everything has its "+
        "proper place. Stone workbenches line the curved walls, their surfaces stained "+
        "with the evidence of countless experiments. Glass alembics and copper retorts "+
        "bubble quietly over measured flames, their contents slowly transforming through "+
        "patient distillation.\n\n"+
        
        "The northern wall holds floor-to-ceiling shelves, each level meticulously "+
        "organized with hundreds of glass vials and ceramic jars. Handwritten labels "+
        "identify their contents in a precise, flowing script: 'Essence of Moonlight', "+
        "'Powdered Dragon Scale', 'Tears of the Willow', 'Mercury, Twice Purified'. "+
        "A large brass mortar and pestle sits prominently on the central workbench, "+
        "surrounded by measuring instruments of bewildering precision.\n\n"+
        
        "Careful diagrams cover the eastern wall - geometric patterns, planetary "+
        "correspondences, and alchemical formulae written in silver ink that seems "+
        "to shimmer in the firelight. A tall glass vessel in the corner contains "+
        "a slowly pulsing green luminescence, the product of some ongoing experiment. "+
        "The air smells of sulfur, roses, and something metallic.\n\n"+
        
        "Despite the obvious power contained here, everything speaks of patience, "+
        "precision, and the slow unfolding of natural law. This is not the domain "+
        "of hasty conjuration, but of systematic understanding.\n"
    );
    
    /* Add exit back to home */
    add_exit("west", "/world/home/wizard/home");
    
    /* Custom exit message */
    set_exit_message("west", "$N carefully sets down their tools and returns to the living quarters.");
}
