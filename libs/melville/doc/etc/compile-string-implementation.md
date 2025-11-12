# compile_string() Implementation

**Date:** November 9, 2025  
**Status:** ✅ COMPLETE - Option A (Admin Tool with Limits)  
**Implementation Time:** ~2 hours

## Overview

Implemented `compile_string(string code)` efun that allows privileged users (admins/wizards) to compile and validate LPC code from strings at runtime.

## Implementation Approach

**Option A:** Quick admin tool with security limits
- Temporary file approach (writes string to temp file, compiles, cleans up)
- Privilege checks (`PRIV` flag required)
- Resource limits (10KB max, 50K compilation cycles)
- MVP: Validates compilation only (doesn't install functions yet)

## Security Features

### 1. Privilege Check
```c
if (!(obj->flags & PRIV)) {
    // DENIED - admin/wizard only
    return 0;
}
```
Only objects with `PRIV` flag can use `compile_string()`.

### 2. Length Limit
```c
#define MAX_EVAL_CODE_LENGTH 10240  // 10KB max
```
Prevents memory exhaustion attacks.

### 3. Compilation Cycle Limit
```c
#define EVAL_COMPILE_CYCLES 50000
```
Limits CPU time during compilation to prevent DoS.

### 4. Logging
All compile_string attempts are logged with:
- Caller object and refno
- Code length
- Success/failure status
- Compilation errors

## API

### Function Signature
```c
int compile_string(string code)
```

### Parameters
- **code**: String containing LPC source code to compile

### Returns
- **1**: Compilation successful
- **0**: Compilation failed (error logged)

### Example Usage
```c
// Simple function
result = compile_string("int add(int a, int b) { return a + b; }");

// Multiple functions
code = "int square(int x) { return x * x; }\n";
code += "int cube(int x) { return x * x * x; }";
result = compile_string(code);

// With variables
code = "int counter;\nvoid increment() { counter++; }";
result = compile_string(code);
```

## Implementation Details

### Files Modified

1. **`/src/sys5.c`** (~220 lines added)
   - `string_to_file()` - Helper to create FILE* from string
   - `s_compile_string()` - Main efun implementation
   - Includes: `<unistd.h>`, `<time.h>`

2. **`/src/instr.h`**
   - Added `S_COMPILE_STRING` (154)
   - Updated `NUM_SCALLS` to 117

3. **`/src/compile1.c`**
   - Added `"compile_string"` to `scall_array`

4. **`/src/protos.h`**
   - Added `OPER_PROTO(s_compile_string)`

5. **`/src/interp.c`**
   - Added `s_compile_string` to dispatch table

### Workflow

1. User calls `compile_string(code_string)`
2. Privilege check (must be admin/wizard)
3. Length validation (<= 10KB)
4. Generate unique temp pathname `/eval/tmp_<timestamp>_<pid>`
5. Write code to temporary file in filesystem
6. Set compilation cycle limit (50K cycles)
7. Call `parse_code()` with temp pathname
8. Restore cycle limit
9. Clean up temp file
10. Check compilation result
11. Return success/failure

### Current Limitations (MVP)

- ✅ Validates compilation
- ✅ Returns success/failure
- ❌ Does NOT install compiled functions into object
- ❌ Does NOT execute compiled code
- ❌ Compiled code is discarded after validation

This is intentional for the MVP - proves the concept safely before adding execution capabilities.

## Testing

Test suite created: `/libs/melville/test/test_compile_string.c`

**Tests included:**
1. Simple function definition
2. Function with efun call (syswrite)
3. Multiple functions
4. Global variables
5. Syntax error handling
6. Empty code string
7. Arrays and mappings
8. Control flow structures

**To run:**
```c
// In NetCI as admin/wizard:
compile_object("/test/test_compile_string")
/test/test_compile_string->run_tests()
```

## Security Assessment

### Risks Addressed
- ✅ Privilege escalation: PRIV check enforced
- ✅ Resource exhaustion: Length + cycle limits
- ✅ Memory bombs: 10KB max code size
- ✅ Audit trail: All attempts logged
- ✅ Compilation errors: Safe error handling

### Remaining Risks
- ⚠️ Admin-only (high trust level required)
- ⚠️ No execution sandboxing (not needed for MVP)
- ⚠️ Temporary files in filesystem (cleaned up)

### Recommended Usage
- Development and debugging by admins
- Code validation before file writes
- Interactive testing of LPC snippets
- Educational tool for learning LPC

## Future Enhancements

### Phase 2: Function Installation (4-6 hours)
- Install compiled functions into caller's object
- Allow calling dynamically compiled functions
- Proper cleanup and lifecycle management

### Phase 3: Eval Context (6-8 hours)
- Create temporary eval objects
- Execute code in isolated context
- Return results to caller
- Automatic cleanup

### Phase 4: Full Sandboxing (8-12 hours)
- Efun whitelist/blacklist
- Memory limits during execution
- Execution timeout
- Per-caller configuration

## Known Issues

None currently. The implementation is stable for validation use.

## Documentation

See also:
- **Analysis:** `/libs/melville/doc/todo/compile-string-analysis.md`
- **Test Suite:** `/libs/melville/test/test_compile_string.c`

## Examples

### Valid Code
```c
// Function definition
compile_string("int multiply(int a, int b) { return a * b; }");

// Variables and functions
compile_string("int score; void add_score(int n) { score += n; }");

// Arrays
compile_string("int process(int a) { return ({1,2,3})[a]; }");

// Control flow
compile_string("int max(int a, int b) { if (a > b) return a; return b; }");
```

### Invalid Code (Returns 0)
```c
// Syntax errors
compile_string("int broken( { }");  // Missing parameter

// Too long (> 10KB)
compile_string(very_long_string);  // Fails length check

// Non-privileged caller
// Regular user calls compile_string() → DENIED
```

## Changelog

**November 9, 2025** - Initial Implementation
- Option A: Admin tool with limits
- Privilege checks
- Resource limits
- Compilation validation
- Test suite created
- Documentation complete

## Conclusion

`compile_string()` is now available for admin/wizard use in NetCI. It provides a safe, limited way to validate LPC code compilation at runtime. The MVP implementation proves the concept while maintaining security boundaries. Future enhancements can add execution capabilities if needed.

**Status:** ✅ Production-ready for admin use
