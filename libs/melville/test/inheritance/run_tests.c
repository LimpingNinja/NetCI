/* Test runner for inheritance tests
 * Compile and run this to execute all tests
 */

run_all_tests() {
    object test_obj;
    int result;
    string str_result;
    
    syswrite("\n========================================");
    syswrite("INHERITANCE TEST SUITE");
    syswrite("========================================\n");
    
    /* Test 1: Basic inheritance */
    syswrite("Test 1: Basic Inheritance (child.c)");
    syswrite("  Testing that child can access parent's variables and methods...");
    test_obj = new("/test/inheritance/child");
    if (test_obj) {
        result = call_other(test_obj, "test_parent_methods");
        if (result == 1) {
            syswrite("  ✓ PASS: Child can access and modify parent's variables");
        } else {
            syswrite("  ✗ FAIL: Child cannot properly access parent's variables");
        }
        
        str_result = call_other(test_obj, "test_parent_call");
        if (str_result == "SUCCESS") {
            syswrite("  ✓ PASS: ::parent_method() calls work correctly");
        } else {
            syswrite("  ✗ FAIL: ::parent_method() calls don't work");
        }
        
        destruct(test_obj);
    } else {
        syswrite("  ✗ FAIL: Could not compile child.c");
    }
    
    /* Test 2: Diamond inheritance - shared storage */
    syswrite("Test 2: Diamond Inheritance (diamond.c)");
    syswrite("  Testing that base variable has single shared storage...");
    test_obj = new("/test/inheritance/diamond");
    if (test_obj) {
        result = call_other(test_obj, "test_shared_storage");
        if (result == 1) {
            syswrite("  ✓ PASS: Base variable is shared across both inheritance paths");
        } else {
            syswrite("  ✗ FAIL: Base variable has separate storage (duplicate!)");
        }
        
        result = call_other(test_obj, "test_all_variables");
        if (result == 1) {
            syswrite("  ✓ PASS: All variables accessible (base, left, right, diamond)");
        } else {
            syswrite("  ✗ FAIL: Cannot access all variables");
        }
        
        destruct(test_obj);
    } else {
        syswrite("  ✗ FAIL: Could not compile diamond.c");
    }
    
    /* Test 3: Late inherit error */
    syswrite("Test 3: Late Inherit Error (late_inherit_error.c)");
    syswrite("  Testing that inherit after code produces compile error...");
    test_obj = new("/test/inheritance/late_inherit_error");
    if (test_obj) {
        syswrite("  ✗ FAIL: late_inherit_error.c compiled (should have failed!)");
        destruct(test_obj);
    } else {
        syswrite("  ✓ PASS: late_inherit_error.c correctly failed to compile");
    }
    
    /* Test 4: Variable name conflict */
    syswrite("Test 4: Variable Name Conflict (conflict_error.c)");
    syswrite("  Testing that duplicate var names in different parents error...");
    test_obj = new("/test/inheritance/conflict_error");
    if (test_obj) {
        syswrite("  ✗ FAIL: conflict_error.c compiled (should have failed!)");
        destruct(test_obj);
    } else {
        syswrite("  ✓ PASS: conflict_error.c correctly failed to compile");
    }
    
    /* Test 5: Shadowing error */
    syswrite("Test 5: Variable Shadowing Error (shadowing_error.c)");
    syswrite("  Testing that child cannot redeclare parent's variable...");
    test_obj = new("/test/inheritance/shadowing_error");
    if (test_obj) {
        syswrite("  ✗ FAIL: shadowing_error.c compiled (should have failed!)");
        destruct(test_obj);
    } else {
        syswrite("  ✓ PASS: shadowing_error.c correctly failed to compile");
    }
    
    syswrite("========================================");
    syswrite("TEST SUITE COMPLETE");
    syswrite("========================================\n");
}

init() {
    run_all_tests();
}
