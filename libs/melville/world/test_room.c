/* test_room.c - Minimal test */

#include <std.h>
#include <config.h>
inherit ROOM_PATH;

init() {
    ::init();
    set_short("Test Room");
    set_long("A simple test room.\n");
    add_exit("east", "/world/start/void");
}
