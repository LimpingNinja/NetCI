/* living.c - Living object inheritable for Melville/NetCI
 *
 * This inheritable provides functionality for "living" objects
 * (players, NPCs, monsters, etc.)
 *
 * Responsibilities:
 * - Mark object as living (query_living)
 * - Communication (listen, catch_tell)
 * - Object finding (find_object helper)
 * - Present checking
 *
 * Usage:
 * - Player objects should inherit this
 * - NPC objects should inherit this
 * - Monsters should inherit this
 */

#include <std.h>
#include <config.h>

inherit OBJECT_PATH;

/* ========================================================================
 * INITIALIZATION
 * ======================================================================== */

static init() {
    /* Call parent init */
    ::init();
    
    /* Set living type */
    set_property("living", 1);
}

/* ========================================================================
 * LIVING STATUS
 * ======================================================================== */

/* Mark this as a living object */
query_living() {
    return 1;
}

/* ========================================================================
 * COMMUNICATION
 * ======================================================================== */

/* Receive a message */
listen(msg) {
    if (!msg) return;
    send_device(msg);
}

/* Alias for listen (standard LPC) */
catch_tell(msg) {
    listen(msg);
}

/* Write to this living (convenience) */
write(msg) {
    listen(msg);
}

/* ========================================================================
 * OBJECT FINDING
 * ======================================================================== */

/* NOTE: find_object() and present() are available from auto.c (automatically attached) */
