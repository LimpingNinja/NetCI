# Efun Validation Checklist

Before using any efun in Melville/NetCI code, **VERIFY** against source code!

## Validation Process

1. **Check help file**: `/libs/ci200fs/help/help.7/<efun_name>`
2. **Check source**: `grep -r "s_<efun_name>" src/`
3. **Check signature**: Look at argument parsing in `sys*.c`
4. **Document correctly**: Update this list with verified signatures

## Verified Efuns

### Input/Output
- ✅ `redirect_input(string function_name)` - Same object only
- ✅ `input_to(object target, string function_name)` - Standard LPC, use this!
- ✅ `get_input_func()` - Returns string or 0

### Object Management
- ✅ `contents(object obj)` - Returns FIRST object (iterator)
- ✅ `next_object(object obj)` - Returns next in linked list
- ✅ `location(object obj)` - Returns container
- ✅ `move_object(object item, object dest)` - Low-level move (no hooks)
- ✅ `this_object()` - Current object
- ✅ `this_player()` - Current player
- ✅ `caller_object()` - Calling object

### Conversions
- ✅ `otoa(object obj)` - Object to string path
- ✅ `atoo(string path)` - String to object
- ✅ `otoi(object obj)` - Object to integer ID
- ✅ `itoo(int id)` - Integer to object

### Arrays
- ✅ `sizeof(array)` - Array/mapping size
- ✅ `member_array(item, array)` - Returns index or -1
- ✅ `implode(array, separator)` - Join array to string
- ✅ `explode(string, separator)` - Split string to array
- ✅ `sort_array(array, function)` - Sort with comparator
- ✅ `reverse(array)` - Reverse array
- ✅ `unique_array(array)` - Remove duplicates

### Mappings
- ✅ `keys(mapping)` - Returns array of keys
- ✅ `values(mapping)` - Returns array of values
- ✅ `map_delete(mapping, key)` - Delete key
- ✅ `member(mapping, key)` - Check if key exists (1/0)

## To Verify Before Use

### String Functions
- ✅ `strlen(string)` - Returns length of string
- ✅ `leftstr(string, len)` - Returns left len characters
- ✅ `rightstr(string, len)` - Returns right len characters
- ✅ `instr(string, start, search)` - Find search in string starting at start, returns position or -1
- ✅ `upcase(string)` - Convert to uppercase
- ✅ `downcase(string)` - Convert to lowercase
- ✅ `explode(string, separator)` - Split string into array
- ✅ `implode(array, separator)` - Join array into string
- ✅ `midstr(string, pos, len)` - Returns substring starting at pos (1-indexed!) for len characters
- ⚠️ `subst(string, old, new)` - Need to verify
- ⚠️ `sprintf(format, ...)` - Need to verify
- ❌ `sscanf(string, format, ...)` - **NOT IMPLEMENTED!** Use instr/leftstr/rightstr instead

### File System
- ✅ `fread(path, pos)` - Read from file starting at position pos (updates pos), returns string or 0/−1
- ✅ `fwrite(path, string)` - Append string to file, returns 0 on success, non-zero on failure
- ✅ `fstat(path)` - Check if file exists, returns truthy if exists
- ⚠️ `chmod(path, mode)` - Need to verify mode format
- ⚠️ `chown(path, owner)` - Need to verify

### Interactive
- ✅ `send_device(string msg)` - Send message to connected device
- ✅ `disconnect_device()` - Disconnect the current connection
- ✅ `reconnect_device(object obj)` - Transfer connection to obj (returns 0 on success, non-zero on failure, requires PRIV)
- ⚠️ `set_interactive(obj)` - Need to verify
- ⚠️ `flush_device()` - Need to verify

### Verbs
- ⚠️ `add_verb(verb, function)` - Need to verify signature
- ⚠️ `add_xverb(verb, function)` - Need to verify signature
- ⚠️ `remove_verb(verb)` - Need to verify

### Time/Scheduling
- ⚠️ `alarm(seconds, function)` - Need to verify signature
- ⚠️ `remove_alarm(function)` - Need to verify

## Common Mistakes to Avoid

### ❌ WRONG: Assuming redirect_input takes object
```c
redirect_input(this_object(), "handler");  // WRONG!
```

### ✅ CORRECT: Use input_to for cross-object
```c
input_to(this_object(), "handler");  // Correct!
```

### ❌ WRONG: Treating contents() as array
```c
object *inv = contents(room);  // WRONG! Returns first object, not array
```

### ✅ CORRECT: Use iterator pattern
```c
object curr = contents(room);
while (curr) {
    // process curr
    curr = next_object(curr);
}
```

### ❌ WRONG: Assuming move_object calls hooks
```c
move_object(obj, room);  // Hooks NOT called!
```

### ✅ CORRECT: Call hooks yourself or use move()
```c
obj.move(room);  // Calls hooks then move_object
```

## Validation Template

When adding a new efun to code:

1. **Find help file**: `cat libs/ci200fs/help/help.7/<efun>`
2. **Find implementation**: `grep -r "s_<efun>" src/`
3. **Check argument count**: Look for `tmp.value.num!=X` in source
4. **Check argument types**: Look for type checks (STRING, OBJECT, INTEGER)
5. **Document signature**: Add to this file with ✅
6. **Write test**: Create test in `/test/` to verify behavior

## Example Verification: input_to

```c
// From src/sys8.c line 366:
int s_input_to(struct object *caller, struct object *obj,
               struct object *player, struct var_stack **rts) {
  // ...
  if (tmp.value.num!=2) return 1;  // Takes 2 arguments!
  
  // Pop function name (STRING)
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=STRING && !(tmp.type==INTEGER && tmp.value.integer==0)) {
    // ...
  }
  
  // Pop target object (OBJECT)
  if (pop(&tmp2,rts,obj)) {
    // ...
  }
  if (tmp2.type!=OBJECT) {
    // ...
  }
```

**Conclusion**: `input_to(object target, string function_name)` ✅
