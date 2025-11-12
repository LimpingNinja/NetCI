/* test_eval_lifecycle.c - Test compile_string() Phase 2: Eval object lifecycle */

run_tests() {
    object eval_obj;
    string code;
    int result, sq, cb, val1, val2, val3;
    
    syswrite("\n===============================================");
    syswrite("compile_string() Phase 2: Eval Object Lifecycle Tests");
    syswrite("===============================================\n");
    
    /* Test 1: Basic eval object creation with init */
    syswrite("=== Test 1: Create eval object with init() ===");
    code = "init() { syswrite(\"  [LIFECYCLE] init() called on eval object\"); }";
    eval_obj = compile_string(code);
    if (eval_obj) {
        syswrite("  [PASS] Eval object created successfully");
        syswrite(sprintf("  [INFO] Eval object: %O", eval_obj));
    } else {
        syswrite("  [FAIL] Failed to create eval object");
    }
    
    /* Test 2: Call a function on the eval object */
    syswrite("\n=== Test 2: Call function on eval object ===");
    code = "init() { syswrite(\"  [LIFECYCLE] init() called\"); }\n";
    code += "add(int a, int b) { return a + b; }";
    eval_obj = compile_string(code);
    if (eval_obj) {
        result = eval_obj.add(5, 3);
        if (result == 8) {
            syswrite(sprintf("  [PASS] Successfully called add(5, 3) = %d", result));
        } else {
            syswrite(sprintf("  [FAIL] add(5, 3) returned %d instead of 8", result));
        }
    } else {
        syswrite("  [FAIL] Failed to create eval object");
    }
    
    /* Test 3: Test destruct() callback */
    syswrite("\n=== Test 3: Destruct eval object ===");
    code = "init() { syswrite(\"  [LIFECYCLE] init() called\"); }\n";
    code += "destruct() { syswrite(\"  [LIFECYCLE] destruct() called - cleaning up\"); }\n";
    code += "get_value() { return 42; }";
    eval_obj = compile_string(code);
    if (eval_obj) {
        result = eval_obj.get_value();
        syswrite(sprintf("  [INFO] get_value() returned: %d", result));
        syswrite("  [INFO] Calling destruct on eval object...");
        destruct(eval_obj);
        syswrite("  [PASS] destruct() called (watch for destruct callback above)");
    } else {
        syswrite("  [FAIL] Failed to create eval object");
    }
    
    /* Test 4: Multiple function eval object */
    syswrite("\n=== Test 4: Multiple functions in eval object ===");
    code = "square(int x) { return x * x; }\n";
    code += "cube(int x) { return x * x * x; }";
    eval_obj = compile_string(code);
    if (eval_obj) {
        sq = eval_obj.square(4);
        cb = eval_obj.cube(3);
        if (sq == 16 && cb == 27) {
            syswrite(sprintf("  [PASS] square(4)=%d, cube(3)=%d", sq, cb));
        } else {
            syswrite(sprintf("  [FAIL] Got square=%d, cube=%d", sq, cb));
        }
        destruct(eval_obj);
    } else {
        syswrite("  [FAIL] Failed to create eval object");
    }
    
    /* Test 5: Eval object with global variables */
    syswrite("\n=== Test 5: Eval object with globals ===");
    code = "int counter;\n";
    code += "init() { counter = 0; syswrite(\"  [LIFECYCLE] init() - counter set to 0\"); }\n";
    code += "increment() { counter++; return counter; }\n";
    code += "get_counter() { return counter; }";
    eval_obj = compile_string(code);
    if (eval_obj) {
        val1 = eval_obj.increment();
        val2 = eval_obj.increment();
        val3 = eval_obj.get_counter();
        if (val1 == 1 && val2 == 2 && val3 == 2) {
            syswrite(sprintf("  [PASS] Counter incremented correctly: %d -> %d (final: %d)", val1, val2, val3));
        } else {
            syswrite(sprintf("  [FAIL] Counter values: %d, %d, %d", val1, val2, val3));
        }
        destruct(eval_obj);
    } else {
        syswrite("  [FAIL] Failed to create eval object");
    }
    
    syswrite("\n===============================================");
    syswrite("LIFECYCLE TEST SUITE COMPLETE");
    syswrite("===============================================\n");
}
