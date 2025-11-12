# NetCI LPC Coding Facts

## Control Flow
* `continue` and `break` statements do not exist
* Use nested `if` statements for flow control instead of `continue`
* Use loop condition flags instead of `break`

## Function Declarations
* Functions do not have return type decorators
* Function parameters do not have type declarations
* `*` prefix decorates an array parameter: `function(*array_param, scalar_param)`
* Example: `sort_by_distance(*topics, *scored)` - both parameters are arrays

## Variable Declarations
* All variable declarations must be at the top of the function (C89 style)
* Variables cannot be declared inside loops or conditional blocks
* Multiple variables of the same type can be declared on one line: `int i, j, n;`
* Do not mix array and scalar declarations on the same line

## Object Method Calls
* Do not use `object->method()` syntax
* Use `call_other(object, "method_name", arg1, arg2, ...)` instead
* Example: `call_other(player, "query_name")` not `player->query_name()`

## Array and Mapping Operations
* Nested subscripts ARE supported: `mapping[array[i]]`
* Arithmetic in subscripts IS supported: `array[i + 1]`
* String character indexing is NOT supported: `string[i]` does not work
* Use `leftstr()`, `midstr()`, `rightstr()` for string manipulation
* **Mapping lookups for missing keys**: Use `member(mapping, key)` to check existence before accessing
* Accessing non-existent mapping keys may cause system call errors

## NULL and Zero Handling
* There is no `NULL` keyword - use `0` for null values
* Empty/missing values are represented as `0` (integer zero)
* Check for null with: `if (!variable)` or `if (variable == 0)`
* Example: `if (!player)` checks if player is null/0

## Operators
* Ternary operator IS supported: `condition ? true_value : false_value`
* Standard arithmetic and comparison operators work as expected

## String Operations and Type Conversion
* **Cannot concatenate integers directly to strings**: `"text" + 5` will fail
* **Use `itoa()` to convert integers to strings**: `"text" + itoa(5)` works
* Example: `syslog("Count: " + itoa(count))` not `syslog("Count: " + count)`
* Use `sprintf()` for formatted strings: `sprintf("Count: %d", count)`

## Best Practice
* Always look at relevant code examples in `/libs/melville` and `/libs/ci200fs` first
* If something doesn't have an example, examine the source in `/src` engine files
* Use `grep_search` to find existing patterns before implementing new code
