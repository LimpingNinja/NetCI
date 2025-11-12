/* test_compile_string.c - Test the compile_string() efun */

run_tests() {
    int result;
    string code;
    
    syswrite("\n===============================================");
    syswrite("compile_string() Test Suite");
    syswrite("===============================================\n");
    
    /* Test 1: Simple function definition */
    syswrite("=== Test 1: Simple function compilation ===");
    code = "add(int a, int b) { return a + b; }";
    result = compile_string(code);
    if (result) {
        syswrite("  [PASS] Simple function compiled successfully");
    } else {
        syswrite("  [FAIL] Simple function compilation failed");
    }
    
    /* Test 2: Function with syswrite */
    syswrite("\n=== Test 2: Function with efun call ===");
    code = "greet() { syswrite(\"Hello from eval!\"); }";
    result = compile_string(code);
    if (result) {
        syswrite("  [PASS] Function with efun compiled successfully");
    } else {
        syswrite("  [FAIL] Function with efun compilation failed");
    }
    
    /* Test 3: Multiple functions */
    syswrite("\n=== Test 3: Multiple functions ===");
    code = "square(int x) { return x * x; }\ncube(int x) { return x * x * x; }";
    result = compile_string(code);
    if (result) {
        syswrite("  [PASS] Multiple functions compiled successfully");
    } else {
        syswrite("  [FAIL] Multiple functions compilation failed");
    }
    
    /* Test 4: Variable declarations */
    syswrite("\n=== Test 4: Global variables ===");
    code = "int test_var;\nset_test(int val) { test_var = val; }";
    result = compile_string(code);
    if (result) {
        syswrite("  [PASS] Variables and functions compiled successfully");
    } else {
        syswrite("  [FAIL] Variables and functions compilation failed");
    }
    
    /* Test 5: Syntax error handling */
    syswrite("\n=== Test 5: Syntax error handling ===");
    code = "broken( { return 0; }";  /* Missing parameter, broken syntax */
    result = compile_string(code);
    if (!result) {
        syswrite("  [PASS] Syntax error correctly detected and handled");
    } else {
        syswrite("  [FAIL] Syntax error should have failed compilation");
    }
    
    /* Test 6: Empty code */
    syswrite("\n=== Test 6: Empty code string ===");
    code = "";
    result = compile_string(code);
    if (result) {
        syswrite("  [PASS] Empty code handled gracefully");
    } else {
        syswrite("  [INFO] Empty code returned failure (expected behavior)");
    }
    
    /* Test 7: Array and mapping usage */
    syswrite("\n=== Test 7: Arrays and mappings ===");
    code = "test() { int a; mapping m; a = ({1,2,3}); m = ([]); return sizeof(a); }";
    result = compile_string(code);
    if (result) {
        syswrite("  [PASS] Arrays and mappings compiled successfully");
    } else {
        syswrite("  [FAIL] Arrays and mappings compilation failed");
    }
    
    /* Test 8: Control flow */
    syswrite("\n=== Test 8: Control flow structures ===");
    code = "factorial(int n) { if (n <= 1) return 1; return n * factorial(n-1); }";
    result = compile_string(code);
    if (result) {
        syswrite("  [PASS] Control flow compiled successfully");
    } else {
        syswrite("  [FAIL] Control flow compilation failed");
    }
    
    syswrite("\n===============================================");
    syswrite("TEST SUITE COMPLETE");
    syswrite("===============================================\n");
}
