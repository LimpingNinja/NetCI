# World Directory

This directory contains the game world: rooms, objects, and areas.

## Structure

- `start/` - Starting area (void, first room)
- `wiz/` - Wizard home directories
- Additional areas can be added as subdirectories

## Starting Area

The starting area (`/world/start/`) contains:
- `void.c` - The void (default spawn location for new players)
- `start.c` - First real room (connected to void)

## Wizard Homes

Each wizard gets a personal directory in `/world/wiz/{name}/`:
- `home.c` - Wizard's home room
- Additional rooms and objects as they build

The `xyzzy` command returns wizards to their home room.

## Building Guidelines

### For Builders
- Create new areas as subdirectories (e.g., `/world/forest/`, `/world/castle/`)
- Use descriptive file names
- Document your areas with comments
- Connect areas via exits in rooms

### For Wizards
- Your home is `/world/wiz/{yourname}/home.c`
- You can build anywhere in `/world/` except other wizards' homes
- Consider creating your own area subdirectory

## File Naming

- Rooms: descriptive names (e.g., `throne_room.c`, `dark_forest.c`)
- Objects: noun-based (e.g., `sword.c`, `torch.c`, `chest.c`)
- NPCs: name-based (e.g., `guard.c`, `shopkeeper.c`)

## Example Room

```c
inherit "/inherits/room";

create() {
    set_short("A peaceful meadow");
    set_long("Tall grass sways gently in the breeze. Wildflowers "
             "dot the landscape with splashes of color.");
    
    add_exit("north", "/world/forest/entrance");
    add_exit("south", "/world/start/start");
}
```
