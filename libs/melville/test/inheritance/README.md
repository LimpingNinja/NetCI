# Inheritance Test Suite

Comprehensive tests for NetCI's LPC-style inheritance system.

## Test Files

### Working Tests (Should Compile Successfully)

- **base.c** - Base class with a single variable and methods
- **child.c** - Simple inheritance test, accesses parent's variables and methods
- **left.c** - Left branch of diamond inheritance
- **right.c** - Right branch of diamond inheritance
- **diamond.c** - Diamond inheritance test, verifies single shared storage

### Error Tests (Should Fail to Compile)

- **late_inherit_error.c** - Tests that `inherit` after code produces error
- **conflict_error.c** - Tests that duplicate variable names in different parents error
- **shadowing_error.c** - Tests that child cannot redeclare parent's variable

## Running the Tests

### Option 1: Use the test runner (recommended)

```bash
./netci
> new("/test/inheritance/run_tests");
```

This will compile and run all tests, showing PASS/FAIL for each.

### Option 2: Manual testing

Compile individual test files:

```bash
./netci
> new("/test/inheritance/child");
> call_other(that, "test_parent_methods");
```

## Expected Results

### Test 1: Basic Inheritance
- ✓ Child can access and modify parent's variables
- ✓ `::parent_method()` calls work correctly

### Test 2: Diamond Inheritance
- ✓ Base variable is shared across both inheritance paths (single storage)
- ✓ All variables accessible (base, left, right, diamond)

### Test 3: Late Inherit Error
- ✓ Correctly fails to compile with error about inherit after code

### Test 4: Variable Name Conflict
- ✓ Correctly fails to compile with error about duplicate variable names

### Test 5: Variable Shadowing Error
- ✓ Correctly fails to compile with error about redeclaring parent's variable

## What These Tests Verify

1. **Variable Access**: Child can read/write parent's variables
2. **Method Calls**: Child can call parent's methods
3. **Parent Calls**: `::method()` syntax works
4. **Diamond Inheritance**: Base variables merged once (single storage)
5. **Linearization**: Proper MRO (Method Resolution Order)
6. **Barrier Enforcement**: Inherits must come before code
7. **Conflict Detection**: Duplicate variable names caught at compile time
8. **Shadowing Prevention**: Child cannot redeclare parent's variables

## Architecture Tested

- ✓ `own_vars` vs `all_vars` separation
- ✓ Unique linearization algorithm
- ✓ `var_offset` tracking per inherit
- ✓ `origin_prog` tracking per variable
- ✓ Barrier-based layout construction
- ✓ Compile-time error detection
