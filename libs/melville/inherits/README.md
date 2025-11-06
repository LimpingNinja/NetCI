# Inheritables Directory

This directory contains standard inheritable objects that form the foundation of the game world.

## Core Inheritables

### object.c
Base for all physical objects in the game world.

**Features**:
- Short and long descriptions
- ID and adjective handling
- Movement functions (`move_object()`)
- Environment tracking (`query_environment()`)
- Basic object properties

**Inherit this for**: Any object that has a physical presence

### container.c
Adds inventory management to objects.

**Inherits**: `object.c`

**Features**:
- Add/remove objects from inventory
- Query contents
- Prevent/allow object insertion
- Weight and capacity (optional)

**Inherit this for**: Rooms, players, bags, chests, NPCs

### room.c
Specialized container for rooms with exits.

**Inherits**: `container.c`

**Features**:
- Exit handling (`add_exit()`, `query_exit()`)
- Room descriptions with contents listing
- Brief mode support
- Prevent object movement (rooms don't move!)

**Inherit this for**: All rooms

### living.c (Optional)
Base for living creatures.

**Inherits**: `container.c`

**Features**:
- Health and stats
- Combat hooks
- NPC behavior hooks

**Inherit this for**: Players, NPCs, monsters

## Inheritance Hierarchy

```
object.c
  └─ container.c
       ├─ room.c
       └─ living.c
```

## Usage Example

```c
// A simple room
inherit "/inherits/room";

create() {
    set_short("A dark void");
    set_long("You are floating in an endless void of darkness.");
    add_exit("north", "/world/start/start");
}
```
