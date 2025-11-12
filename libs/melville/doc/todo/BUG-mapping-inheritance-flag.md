# Bug: Mapping Variables Treated as Arrays in Inherited Objects

**Status:** Unresolved  
**Priority:** Medium  
**Component:** Compiler / Object System  
**Reported:** November 10, 2025

## Description

When a mapping variable is declared in a parent object and inherited by a child object, the variable may lose its `is_mapping` flag during inheritance resolution. This causes the runtime to treat the mapping as an array, leading to type errors and incorrect behavior.

## Symptoms

- Mapping variables work correctly in the object where they're declared
- Same mapping variables fail when accessed through inherited code
- Runtime errors like "Bad argument 1 to keys()" or "Bad type for subscript"
- Variable appears to have array type instead of mapping type

## Root Cause (Suspected)

The `is_mapping` flag in `struct var_tab` (defined in `src/object.h`) is not being properly propagated during:
1. Inheritance flattening (`src/compile2.c`)
2. Variable table merging when child objects inherit parent variables
3. Object cloning/instantiation

## Relevant Code Locations

- **Flag Definition:** `src/object.h` - `struct var_tab` has `is_mapping` field
- **Variable Declaration:** `src/compile1.c` - `add_var()`, `parse_var()` set the flag
- **Inheritance:** `src/compile2.c` - Variable table flattening and merging
- **Object Creation:** `src/cache1.c`, `src/cache2.c` - Object instantiation

## Example Case

```c
// parent.c
mapping properties;

void create() {
    properties = ([]);
}

// child.c
inherit "parent";

void test() {
    // This might fail if is_mapping flag lost during inheritance
    string *keys = keys(properties);  // Error: Bad argument 1 to keys()
}
```

## Workaround

Re-declare mapping variables in child objects instead of relying on inheritance:
```c
// child.c
inherit "parent";
mapping properties;  // Re-declare to ensure flag is set

void create() {
    ::create();  // Call parent create
}
```

## Investigation Steps

1. Add debug logging to inheritance code in `compile2.c`
2. Verify `is_mapping` flag is copied during variable table merging
3. Check if flag persists through object cloning
4. Review how inherited variable metadata is stored and accessed
5. Compare with how `is_array` flag (if any) is handled

## Related Issues

- Comma-separated variable declarations bug (fixed) - similar flag propagation issue
- L_VALUE resolution for mapping keys (fixed) - different but related mapping issue

## Notes

This issue was mentioned during implementation of collection efuns (users, objects, etc.) but not fully investigated at the time. The fix for comma-separated declarations suggests similar patterns might exist in the inheritance code.
