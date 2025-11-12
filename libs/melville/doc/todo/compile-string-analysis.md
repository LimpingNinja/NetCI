# compile_string() / eval() Implementation Analysis

**Date:** November 9, 2025  
**Status:** ANALYSIS / PROPOSAL  
**Complexity:** HIGH (8-12+ hours for minimal implementation, 20-30+ hours for production-ready)

## Overview

Analysis of implementing dynamic LPC code compilation and execution via `compile_string()` or `eval()` efun in NetCI. This would allow end users to compile and execute LPC code from strings at runtime rather than only from files.

## Use Cases

### Common MUD Applications
1. **Admin Commands:** Dynamic object creation/modification without file edits
2. **Interactive Code Testing:** REPL-style development environment
3. **Dynamic Content Generation:** Runtime-generated quest logic, AI behaviors
4. **Scripting System:** User-defined commands, macros, or automation
5. **Debug/Inspection Tools:** Runtime introspection and modification

### Example Usage
```c
// Simple expression evaluation
result = eval("2 + 2 * 3");  // Returns 8

// Function definition and execution
compile_string("int add(int a, int b) { return a + b; }");
result = eval("add(5, 3)");  // Returns 8

// Dynamic object method
code = "void greet() { syswrite(\"Hello from eval!\"); }";
compile_string(code);
greet();  // Executes dynamically compiled function
```

## Current NetCI Compilation Architecture

### File-Based Compilation Flow

**Entry Point:** `compile_object(string path)` in `sys5.c`
1. Calls `parse_code(filename, caller_obj, &result)` in `compile2.c`
2. Opens file with `open_file(filename, FREAD_MODE, NULL)`
3. Creates `filptr` structure with `FILE*` pointer
4. Tokenizes via `get_token(&file_info)` reading from `FILE*`
5. Parses with `top_level_parse(&file_info)` in `compile1.c`
6. Generates bytecode in `struct code` structure
7. Links into proto system
8. Executes with `interp(caller, obj, player, &arg_stack, func)`

### Key Data Structures

**`filptr`** (`token.h` lines 83-96):
```c
typedef struct {
  FILE *curr_file;               // ← FILE POINTER DEPENDENCY
  struct file_stack *previous;
  token_t put_back_token;
  int is_put_back;
  char *expanded;
  unsigned int phys_line;
  struct code *curr_code;
  sym_tab_t *glob_sym;
  struct define *defs;
  int depth;
  int layout_locked;
} filptr;
```

**`struct code`** (holds compiled bytecode):
- Function list (`struct fns *func_list`)
- Global symbol table (`struct var_tab *gst`)
- Inheritance chain (`struct inherit_list *inherits`)
- Bytecode instructions (`struct var *code`)

### Critical Dependencies
- **Tokenizer:** Reads from `FILE*` via `fgetc()`, `ungetc()`, `fgets()`
- **Preprocessor:** Handles `#include`, `#define` from files
- **Inheritance:** Resolves `inherit` directives by loading parent files
- **Error Reporting:** References file paths and line numbers

## Implementation Options

---

## OPTION A: String-to-Memory-File Wrapper (SIMPLEST - 4-6 hours)

### Concept
Use `fmemopen()` (POSIX) or custom memory stream to wrap string as `FILE*`, feed to existing compilation pipeline.

### Implementation Steps

1. **Create String Wrapper** (1 hour)
   - New function: `FILE* string_to_file(char *code)`
   - Use `fmemopen(code, strlen(code), "r")` on BSD/Linux
   - Create temporary file fallback for portability
   - Returns `FILE*` that existing tokenizer can read

2. **Add compile_string() Efun** (2 hours)
   - New efun in `sys5.c`: `s_compile_string()`
   - Similar to `s_compile_object()` but:
     - Takes string argument instead of path
     - Creates memory file with `string_to_file()`
     - Generates temporary pathname (e.g., `"/eval/temp_<timestamp>"`)
     - Calls `parse_code()` with memory file
     - Installs result in temporary proto or caller's context

3. **Context Management** (1-2 hours)
   - Decide execution context:
     - **Option A1:** Install in calling object's proto (modifies caller)
     - **Option A2:** Create temporary eval object (safer, cleaner)
   - Handle variable scoping (globals vs locals)
   - Cleanup: Remove temporary protos after execution

4. **Error Handling** (1 hour)
   - Compilation errors must return gracefully, not crash
   - Clear error messages without file path confusion
   - Prevent infinite loops (eval calling eval)

### Pros
- ✅ Minimal code changes (~200-300 lines)
- ✅ Reuses 100% of existing compilation infrastructure
- ✅ Supports full LPC syntax (functions, variables, control flow)
- ✅ `#define` and preprocessor work automatically
- ✅ Relatively quick to implement (4-6 hours)

### Cons
- ⚠️ `#include` directives would still try to load files (may fail or need disable)
- ⚠️ `inherit` directives similarly problematic (needs file system)
- ⚠️ Error messages reference fake pathnames
- ⚠️ Temporary protos clutter proto list if not cleaned up
- ⚠️ `fmemopen()` may not be available on all platforms (needs fallback)

### Security Risks
- 🔴 **HIGH:** Full LPC execution capability
- 🔴 Caller can define functions, modify variables, access efuns
- 🔴 No sandboxing - executes with caller's privileges
- 🔴 Potential for abuse if exposed to non-admin users

---

## OPTION B: Expression-Only Evaluator (MODERATE - 6-10 hours)

### Concept
Implement restricted `eval()` that only evaluates expressions, not full statements/functions. Safer and more targeted.

### Implementation Steps

1. **Expression Parser** (3-4 hours)
   - New function: `parse_expression(char *expr, struct var *result)`
   - Reuse `parse_exp()` from `compile1.c` but without statement context
   - Build minimal AST or directly execute expression
   - Support: arithmetic, variables, function calls, string operations

2. **Execution Engine** (2-3 hours)
   - Compile expression to bytecode snippet
   - Execute in caller's context with access to local variables
   - Return result as `struct var`
   - No side effects (no assignments, no function definitions)

3. **Variable Scoping** (1-2 hours)
   - Access caller's local variables by name
   - Read globals from caller's object
   - No ability to modify (read-only evaluation)

4. **Add eval() Efun** (1 hour)
   - Takes string expression
   - Returns evaluated result
   - Example: `eval("hp + 10")` returns integer

### Pros
- ✅ Much safer than full compilation
- ✅ No function definition capability (limited attack surface)
- ✅ Simpler implementation than full compile_string
- ✅ Useful for dynamic calculations, formulas, conditions
- ✅ No temporary protos needed

### Cons
- ⚠️ Limited functionality (expressions only, no loops/conditionals)
- ⚠️ Can't define new functions or modify code
- ⚠️ Still allows function calls (potential for abuse)
- ⚠️ Complex to properly scope variables between contexts

### Security Risks
- 🟡 **MEDIUM:** Limited to expressions
- 🟡 Still allows function calls (can call any efun)
- 🟡 Can read all accessible variables
- 🟡 Potential information disclosure

---

## OPTION C: Full Implementation with Sandboxing (COMPLEX - 20-30 hours)

### Concept
Complete `compile_string()` with proper security boundaries, execution limits, and permission system.

### Implementation Steps

1. **Memory Stream Wrapper** (2 hours) - Same as Option A

2. **Compilation Subsystem** (4-6 hours)
   - Disable `#include` in eval context (flag in `filptr`)
   - Disable `inherit` in eval context
   - Create isolated temporary objects for execution
   - Prevent name collisions with real objects

3. **Sandboxing Layer** (6-8 hours)
   - **Privilege Checks:** Only allow admins/wizards to use eval
   - **Resource Limits:**
     - Max string length (e.g., 10KB)
     - Max execution time (cycle limits)
     - Max memory allocation
   - **Blacklisted Efuns:** Disable dangerous operations
     - `compile_object()` (prevent recursion)
     - File operations (unless explicitly allowed)
     - System commands
   - **Execution Context Isolation:**
     - Separate global variable space
     - Limited access to caller's scope
     - Can't modify protos permanently

4. **Context Management** (3-4 hours)
   - Create temporary eval objects
   - Execute in isolated environment
   - Copy results back to caller
   - Clean up temporary objects immediately
   - Garbage collection for eval artifacts

5. **Error Handling** (2-3 hours)
   - Safe error reporting
   - Prevent error messages from leaking info
   - Compilation errors don't crash driver
   - Runtime errors contained

6. **API Design** (2-3 hours)
   - `compile_string(string code, [mapping options])`
   - Options:
     - `"context": object` - Execute in specific object context
     - `"timeout": int` - Max execution cycles
     - `"allow_efuns": array` - Whitelist of allowed efuns
   - Returns: success/failure + error message

7. **Testing & Hardening** (3-4 hours)
   - Test malicious inputs
   - Test resource exhaustion attempts
   - Test privilege escalation attempts
   - Fuzz testing

### Pros
- ✅ Full LPC compilation capability
- ✅ Proper security boundaries
- ✅ Admin-suitable for production
- ✅ Flexible and powerful
- ✅ Can be exposed safely to end users (with restrictions)

### Cons
- ⚠️ Significant development time (20-30+ hours)
- ⚠️ Complex to maintain
- ⚠️ Performance overhead from sandboxing
- ⚠️ Still potential for subtle security holes

### Security Risks
- 🟢 **LOW-MEDIUM:** With proper implementation
- 🟡 Complexity increases bug potential
- 🟡 Requires ongoing security review
- 🟡 Sandboxing must be thorough

---

## Security Considerations

### Critical Risks

1. **Arbitrary Code Execution**
   - `eval()` is inherently dangerous
   - Must restrict to admin/wizard privileges
   - Consider whitelist of allowed callers

2. **Resource Exhaustion**
   - Infinite loops: `eval("while(1);")`
   - Memory bombs: `eval("array a = allocate(1000000);")`
   - Needs cycle/memory limits

3. **Information Disclosure**
   - Can read any variable in scope
   - Can call functions to probe system state
   - Need careful permission checks

4. **Privilege Escalation**
   - Caller might gain access to privileged functions
   - Must maintain privilege levels through eval context
   - Prevent impersonation attacks

5. **Code Injection**
   - If eval input comes from user input → severe vulnerability
   - Must validate and sanitize all inputs
   - Never eval user-provided strings directly

### Recommended Security Measures

```c
// Permission check (minimum)
if (!priv(caller) && !is_wizard(caller)) {
    error("eval: Permission denied - admin/wizard only");
}

// Length limit
if (strlen(code) > MAX_EVAL_LENGTH) {
    error("eval: Code too long (max 10KB)");
}

// Cycle limit during execution
old_hard_cycles = hard_cycles;
hard_cycles = EVAL_MAX_CYCLES;  // e.g., 10000
result = execute_eval(code);
hard_cycles = old_hard_cycles;

// Efun whitelist (for restricted eval)
if (is_blacklisted_efun(efun_name)) {
    error("eval: Efun not allowed in eval context");
}
```

## Recommendation

### For Quick Prototype/Admin Use: **Option A**
- Implement basic `compile_string()` with `fmemopen()` wrapper
- Restrict to admins only (`priv()` check)
- Add basic cycle limits
- **Time:** 4-6 hours
- **Risk:** High, but acceptable for admin-only tool

### For Production/End User: **Option C**
- Full sandboxing implementation
- Proper permission system
- Resource limits
- **Time:** 20-30 hours
- **Risk:** Medium (if implemented correctly)

### Alternative Approach: **Domain-Specific Language**
Instead of full LPC eval, consider implementing a restricted DSL:
- Formula language for calculations only
- No function definitions or control flow
- Limited efun access
- Much safer and simpler
- **Time:** 8-12 hours
- **Risk:** Low

## Implementation Priority

**Suggested Phased Approach:**

### Phase 1: Admin-Only Prototype (4-6 hours)
- Option A: Memory file wrapper
- Basic `compile_string()` efun
- Admin privilege check only
- Cycle limits
- Useful for debugging and development

### Phase 2: Security Hardening (8-12 hours)
- Add efun blacklist
- Execution context isolation
- Resource limits (memory, cycles)
- Better error handling
- Prevent eval recursion

### Phase 3: Production Ready (6-10 hours)
- Whitelist-based permission system
- Per-caller privilege configuration
- Comprehensive testing
- Documentation
- Security audit

### Total: 18-28 hours for production-ready eval system

## References

### Similar Implementations
- **LDMud:** `funcall()` for callback execution, no direct eval
- **FluffOS:** `evaluate()` efun with sandboxing
- **MudOS:** `call_out()` for deferred execution, limited eval

### Files to Modify (Option A)
1. `/src/sys5.c` - Add `s_compile_string()` efun (~100 lines)
2. `/src/compile2.c` - Add `parse_string()` wrapper (~50 lines)
3. `/src/compile.h` - Add prototype
4. `/src/instr.h` - Add `S_COMPILE_STRING` opcode
5. `/src/compile1.c` - Add scall to array
6. `/src/interp.c` - Add efun dispatch
7. `/src/protos.h` - Add OPER_PROTO

### Estimated File Changes (Option A)
- Lines added: ~300-400
- Files modified: 7
- New files: 0
- Complexity: MODERATE

## Conclusion

`compile_string()` / `eval()` is **feasible** but **high-risk**. 

- **Simplest:** Option A (4-6 hours) - Admin use only
- **Safest:** Option B (6-10 hours) - Expression evaluation only  
- **Best:** Option C (20-30 hours) - Production-ready with sandboxing

**Recommendation:** Start with Option A for admin tools, then evaluate if full production version (Option C) is worth the investment based on actual use cases.

**Critical:** Whatever option is chosen, must include:
1. Privilege checks (`priv()` minimum)
2. Cycle/resource limits
3. No eval recursion
4. Clear documentation of risks
5. Audit log of eval usage
