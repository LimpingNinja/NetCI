# Admin Commands

Admin-only commands for NetCI/Melville mudlib.

## Dynamic Code Execution

### eval - Evaluate Expressions

**Usage:** `eval <expression>`

Evaluates an LPC expression and returns the result.

**Examples:**
```
eval 5 + 3
Result: 8

eval sizeof(({1,2,3,4,5}))
Result: 5

eval 42
Result: 42
```

**How it works:**
- Wraps your expression in `eval_expr() { return (<expression>); }`
- Uses `compile_string()` to create a temporary eval object
- Calls the function and displays the result
- Automatically destructs the eval object

### exec - Execute Statements

**Usage:** `exec <statement>`

Executes LPC code statements (not just expressions).

**Examples:**
```
exec syslog("Hello from exec!");

exec write("Line 1"); write("Line 2");

exec int x = 5; write("Value: " + x);
```

**How it works:**
- Wraps your code in `exec_stmt() { <code> }`
- Uses `compile_string()` to create a temporary eval object
- Executes the function
- Automatically destructs the eval object

## Security

Both commands are **admin-only** and use the `compile_string()` efun which:
- Has a 10KB code size limit
- Has a 50,000 cycle compilation limit
- Creates isolated eval objects that are automatically cleaned up

## Implementation

Both commands use the new `compile_string()` efun (Phase 2) which:
1. Compiles LPC code from a string
2. Creates a temporary eval object
3. Installs the compiled code as a prototype
4. Allows calling functions on the eval object
5. Cleans up via `destruct()`

**Files:**
- `/cmds/admin/_eval.c` - Expression evaluator
- `/cmds/admin/_exec.c` - Statement executor
