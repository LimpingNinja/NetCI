# Melville/NetCI Changelog

## 2025-11-05 - Hybrid Inventory Management

### Changed
- **Implemented hybrid approach for inventory management**
  - Maintains `object *inventory` array for fast iteration
  - Uses `move_object()` underneath to keep driver's contents list in sync
  - Both `query_inventory()` and `contents()`/`next_object()` work correctly
  - Added `sync_inventory()` to rebuild from driver if needed
  - Added `query_inventory_safe()` to query directly from driver

### Why Hybrid?
1. **Fast Access**: Array-based iteration is faster than linked list walking
2. **Driver Sync**: Driver's `contents()` list stays accurate for low-level code
3. **Policy Hooks**: Our `receive_object()`/`release_object()` provide move policy
4. **Best of Both**: Get benefits of both approaches!

### How It Works
```c
// object.c move() function:
1. Call old_env.release_object(obj)  // Our hook - can reject
2. Call new_env.receive_object(obj)  // Our hook - can reject
3. Call move_object(obj, new_env)    // Driver's low-level move
4. Both inventory array and driver's contents list are now in sync!
```

### API
- `query_inventory()` - Fast array-based (uses our tracking)
- `query_inventory_safe()` - Slower but guaranteed accurate (uses driver's contents)
- `sync_inventory()` - Rebuild our array from driver's list
- `contents(obj)` + `next_object()` - Driver's iterator (always accurate)

## 2025-11-04 - Major Refactor: Arrays & Mappings

### Changed
- **Refactored all inheritables to use native arrays and mappings**
  - Replaced table_set/get pattern with proper mapping storage
  - Replaced indexed properties with dynamic arrays
  - Much cleaner, more maintainable code

### `/inherits/object.c`
- ✅ Properties now stored in `mapping properties` instead of table_set
- ✅ IDs now stored in `string *id_list` dynamic array
- ✅ Uses `member_array()` for ID checking
- ✅ Simplified cleanup (automatic memory management)
- **Before**: 269 lines with table_set hacks
- **After**: ~250 lines with clean mappings/arrays

### `/inherits/container.c`
- ✅ Inventory now stored in `object *inventory` dynamic array
- ✅ Uses array arithmetic: `inventory + ({ ob })` and `inventory - ({ ob })`
- ✅ Uses `member_array()` for presence checking
- ✅ Simplified iteration with array indexing
- **Before**: 217 lines with indexed properties
- **After**: ~160 lines with clean arrays

### `/inherits/room.c`
- ✅ Exits now stored in `mapping exits` (direction -> destination)
- ✅ Uses `keys()` to get exit directions
- ✅ Uses `implode()` for exit list formatting
- ✅ Uses array slicing: `directions[0..count-2]`
- ✅ Simplified room_tell with `member_array()` for exclusions
- **Before**: 331 lines with indexed properties
- **After**: ~260 lines with clean mappings/arrays

### Benefits
1. **Cleaner Code**: No more `otoa(this_object()) + "_" + key` hacks
2. **Better Performance**: Native data structures vs. global table lookups
3. **More Readable**: Standard LPC patterns instead of workarounds
4. **Easier Maintenance**: Less code, clearer intent
5. **True LPC**: Uses actual LPC features, not table_set workarounds

### Technical Details

**Array Features Used**:
- Dynamic arrays: `string *id_list`, `object *inventory`
- Array literals: `({})`, `({ "item" })`
- Array arithmetic: `arr + ({ item })`, `arr - ({ item })`
- Array slicing: `arr[0..count-2]`
- `member_array(item, arr)` - Check if item in array
- `sizeof(arr)` - Get array size
- `implode(arr, sep)` - Join array elements

**Mapping Features Used**:
- Mapping literals: `([])`, `([ "key": value ])`
- Subscript access: `map["key"] = value`, `value = map["key"]`
- `keys(map)` - Get array of keys
- `values(map)` - Get array of values
- `map_delete(map, key)` - Delete a key
- `sizeof(map)` - Get number of entries
- `member(map, key)` - Check if key exists

### Migration Notes

**Old Pattern (table_set)**:
```c
set_property(key, value) {
    table_set(otoa(this_object()) + "_" + key, value);
}

get_property(key) {
    return table_get(otoa(this_object()) + "_" + key);
}
```

**New Pattern (mappings)**:
```c
mapping properties;

set_property(key, value) {
    if (!properties) properties = ([]);
    properties[key] = value;
}

get_property(key) {
    if (!properties) return NULL;
    return properties[key];
}
```

**Old Pattern (indexed arrays)**:
```c
// Store
set_property("inv_" + itoa(count), ob);
set_property("inv_count", count + 1);

// Retrieve
for (i = 0; i < count; i++) {
    ob = get_property("inv_" + itoa(i));
}
```

**New Pattern (dynamic arrays)**:
```c
object *inventory;

// Store
inventory = inventory + ({ ob });

// Retrieve
for (i = 0; i < sizeof(inventory); i++) {
    ob = inventory[i];
}
```

## 2025-11-04 - Initial Implementation

### Added
- Directory structure for Melville/NetCI
- Documentation (README.md, security.md, driver_integration.md)
- Include files (config.h, roles.h, std.h)
- boot.c with role-based security
- Core inheritables (object.c, container.c, room.c)

### Features
- Role-based security (5 levels)
- TMI-2 style command system (planned)
- User/player split architecture (planned)
- Clean separation of concerns
