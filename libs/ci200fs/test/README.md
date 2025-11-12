# NetCI Test Suite

This directory contains interactive test suites for the NetCI LPC interpreter.

## Test Files

### test_array_efuns.c
Tests for array manipulation functions (efuns):
- `implode()` - Join array elements into string
- `explode()` - Split string into array
- `member_array()` - Find element in array
- `sort_array()` - Sort array in place
- `reverse()` - Reverse array in place
- `unique_array()` - Remove duplicates

**Usage:**
```
@call /test/test_array_efuns create
```

### test_types.c
Tests for type system and array features:
- `typeof()` - Type detection
- `sizeof()` - Array size
- Dynamic array growth (unlimited arrays)
- Array bounds checking
- Pointer syntax (`int *arr` vs `int arr[]`)

**Usage:**
```
@call /test/test_types create
```

### test_driver.c
Tests for driver features (placeholder):
- Error tracebacks
- Memory management
- Call stack

**Usage:**
```
@call /test/test_driver create
```

### traceback_test.c
Comprehensive traceback and error handling tests:
- Line-level error detection
- Call chain tracebacks
- Stack overflow detection
- Array bounds errors
- Unknown function calls

**Usage:**
```
@call /test/traceback_test start_menu    # Interactive mode
@call /test/traceback_test run 1         # Run specific test
@call /test/traceback_test run all       # Run all tests
```

## Interactive Menus

All test suites use `input_to()` for interactive menu-driven testing. You can:
- Run individual tests
- Run all tests
- View test descriptions
- Exit cleanly

## Notes

- Tests are designed to be run in a live NetCI environment
- Some tests intentionally trigger errors to verify traceback functionality
- Array tests verify both heap-allocated arrays and bounds checking
