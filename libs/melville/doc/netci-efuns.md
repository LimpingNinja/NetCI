# NetCI Efuns Reference

This document lists all available efuns (external functions) in NetCI, based on `scall_array` in `compile1.c`.

## Object Management

### contents(obj)
Returns the **first** object in the contents of `obj`, or NULL if empty.
**Usage**: Iterator pattern with `next_object()`
```c
object curr;
curr = contents(this_object());
while (curr) {
    // Do something with curr
    curr = next_object(curr);
}
```

### next_object(obj)
Returns the next object after `obj` in its location, or NULL if last.
**Usage**: Used with `contents()` to iterate inventory.

### location(obj)
Returns the container/environment of `obj`, or NULL if not contained.

### move_object(item, dest)
Moves `item` to `dest`. **Does NOT call hooks** - you must handle `release_object()` and `receive_object()` yourself.

### clone_object(path) / new(path)
Creates a new instance of an object. `new()` is an alias for `clone_object()`.

### destruct(obj)
Destroys an object.

### this_object()
Returns the current object.

### this_player()
Returns the current player object.

### caller_object()
Returns the object that called the current function.

### parent(obj)
Returns the parent (prototype) of a cloned object.

### next_child(obj)
Returns the next clone of the same prototype.

### next_proto(obj)
Returns the next prototype object.

### prototype(obj)
Returns 1 if `obj` is a prototype, 0 if it's a clone.

### otoa(obj)
Object to string - returns the path/name of an object.

### atoo(str)
String to object - finds/loads an object by path.

### otoi(obj)
Object to integer - returns the object ID number.

### itoo(int)
Integer to object - finds object by ID number.

## Verb/Command System

### add_verb(verb, function)
Registers a verb (command) with a function name.

### add_xverb(verb, function)
Registers an exclusive verb (overrides others).

### remove_verb(verb)
Removes a registered verb.

### set_localverbs(flag)
Enable/disable local verb processing.

### localverbs()
Query local verb status.

### next_verb(verb)
Iterate through registered verbs.

## Interactive/Connection

### set_interactive(obj)
Makes an object interactive (connected to a player).

### interactive(obj)
Returns 1 if object is interactive.

### connected(obj)
Returns 1 if object has an active connection.

### get_devconn(obj)
Get device connection info.

### send_device(str)
Send string to the connected device.

### reconnect_device(obj)
Transfer connection to another object.

### disconnect_device()
Disconnect the current connection.

### connect_device(obj)
Connect a device to an object.

### flush_device()
Flush output buffers.

### redirect_input(function_name)
Redirect next input to a function **in the same object**.
**Signature**: `redirect_input(string function_name)`
**Usage**: `redirect_input("my_input_handler");`
The function will be called with the input string as an argument.

### input_to(object, function_name)
Redirect next input to a function **in another object** (standard LPC pattern).
**Signature**: `input_to(object target_obj, string function_name)`
**Usage**: `input_to(this_object(), "my_input_handler");`
**Important**: Always use `input_to()` for standard LPC compatibility!

### get_input_func()
Get the current input redirect function name (string), or 0 if none set.

### get_devidle()
Get device idle time.

### get_conntime()
Get connection time.

### get_devport()
Get device port number.

### get_devnet()
Get device network info.

### get_hostname()
Get hostname of connection.

### get_address()
Get IP address of connection.

## File System

### cat(path)
Display file contents.

### ls(path)
List directory contents.

### rm(path)
Remove a file.

### cp(src, dest)
Copy a file.

### mv(src, dest)
Move/rename a file.

### mkdir(path)
Create a directory.

### rmdir(path)
Remove a directory.

### fread(path, start, lines)
Read lines from a file.

### fwrite(path, str)
Write string to a file.

### ferase(path)
Erase a file (alias for rm).

### fstat(path)
Get file status/info.

### fowner(path)
Get file owner.

### chmod(path, mode)
Change file permissions.

### chown(path, owner)
Change file owner.

### hide(path)
Hide a file/directory.

### unhide(path, owner, mode)
Unhide and set permissions.

## String Functions

### strlen(str)
Returns length of string.

### leftstr(str, len)
Returns left `len` characters.

### rightstr(str, len)
Returns right `len` characters.

### midstr(str, start, len)
Returns substring starting at `start` for `len` characters.

### instr(str, start, search)
Find `search` in `str` starting at position `start`.

### subst(str, old, new)
Substitute `old` with `new` in `str`.

### upcase(str)
Convert to uppercase.

### downcase(str)
Convert to lowercase.

### sprintf(format, ...)
Format a string (like C sprintf).

### sscanf(str, format, ...)
Parse a string (like C sscanf).

### implode(array, separator)
Join array elements with separator.

### explode(str, separator)
Split string into array by separator.

## Conversion Functions

### itoa(int)
Integer to string.

### atoi(str)
String to integer.

### chr(int)
Integer to character.

### asc(str)
Character to integer (ASCII value).

### typeof(var)
Returns type code of variable.

## Array Functions

### sizeof(array)
Returns size of array or mapping.

### member_array(item, array)
Returns index of `item` in `array`, or -1 if not found.

### sort_array(array, function)
Sort array using comparison function.

### reverse(array)
Reverse array order.

### unique_array(array)
Remove duplicates from array.

## Mapping Functions

### keys(mapping)
Returns array of all keys.

### values(mapping)
Returns array of all values.

### map_delete(mapping, key)
Delete a key from mapping.

### member(mapping, key)
Returns 1 if key exists, 0 otherwise.

## Time/Scheduling

### time()
Returns current Unix timestamp.

### mktime(year, month, day, hour, min, sec)
Convert date/time to Unix timestamp.

### alarm(seconds, function)
Schedule a function call after `seconds`.

### remove_alarm(function)
Cancel a scheduled alarm.

## Security/Privileges

### set_priv(obj, flag)
Grant/revoke system privileges.

### priv(obj)
Check if object has system privileges.

### is_legal(str)
Check if string is a legal identifier.

## Editor

### edit(file)
Enter the editor for a file.

### in_editor()
Returns 1 if currently in editor.

## Composition (attach/detach)

### attach(obj)
Attach a shared object (composition pattern).

### detach(obj)
Detach a shared object.

### this_component()
Returns the current component in attach chain.

## Table Storage (Global)

### table_set(key, value)
Store a global key-value pair.

### table_get(key)
Retrieve a global value.

### table_delete(key)
Delete a global key.

**WARNING**: Table storage is GLOBAL! Use namespacing pattern:
```c
table_set(otoa(this_object()) + "_" + key, value);
```

## Miscellaneous

### call_other(obj, function, args...)
Call a function in another object.

### command(str)
Execute a command string.

### compile_object(path)
Compile an object from source.

### random(max)
Returns random number from 0 to max-1.

### sysctl(cmd)
System control function.

### syslog(str)
Write to system log.

### iterate()
Iterate through objects.

### next_who(obj)
Iterate through connected players.

### get_master()
Get the master object (boot.c).

### is_master(obj)
Check if object is the master.

## Important Notes

1. **contents() is an iterator**: Use with `next_object()` in a while loop
2. **move_object() doesn't call hooks**: You must call `release_object()` and `receive_object()` yourself
3. **Table storage is global**: Always namespace your keys
4. **Arrays and mappings are first-class**: Use them instead of table_set hacks!

## Example: Iterating Inventory

```c
// Correct way to iterate contents
object curr;
curr = contents(this_object());
while (curr) {
    // Process curr
    if (curr.get_type() == TYPE_PLAYER) {
        curr.listen("Hello!\n");
    }
    curr = next_object(curr);
}
```

## Example: Moving Objects with Hooks

```c
// Correct way to move with hooks
move_with_hooks(obj, dest) {
    object old_env;
    
    old_env = location(obj);
    
    // Ask old environment if we can leave
    if (old_env && function_exists("release_object", old_env)) {
        if (!old_env.release_object(obj)) {
            return 0;  // Not allowed
        }
    }
    
    // Ask new environment if we can enter
    if (function_exists("receive_object", dest)) {
        if (!dest.receive_object(obj)) {
            // Rejected, try to go back
            if (old_env && function_exists("receive_object", old_env)) {
                old_env.receive_object(obj);
            }
            return 0;
        }
    }
    
    // Actually move
    move_object(obj, dest);
    return 1;
}
```
