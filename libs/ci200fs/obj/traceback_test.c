/* traceback_test.c - Test object for improved error traceback system */

#include <sys.h>

/* Test 1: Simple error at depth 1 */
test_simple_error() {
  int x;
  string s;
  
  /* Trigger type mismatch error */
  x = "not a number";  /* This will cause an error */
  return 1;
}

/* Test 2: Error in nested function call (depth 2) */
test_nested_error() {
  return nested_helper_1();
}

nested_helper_1() {
  return nested_helper_2();
}

nested_helper_2() {
  int arr[5];
  
  /* Trigger array bounds error */
  return arr[100];  /* Out of bounds access */
}

/* Test 3: Unknown function call */
test_unknown_function() {
  /* Call a function that doesn't exist */
  return this_function_does_not_exist();
}

/* Test 4: Deep recursion test */
test_deep_recursion(depth) {
  if (depth > 150) {
    /* This should trigger stack overflow */
    return test_deep_recursion(depth + 1);
  }
  return depth;
}

/* Test 5: Error in arithmetic operation */
test_arithmetic_error() {
  int x, y, z;
  string s;
  
  x = 10;
  s = "hello";
  
  /* Try to add integer and string - type mismatch */
  z = x + s;
  return z;
}

/* Test 6: Three-level nested call with error at bottom */
test_three_level_nesting() {
  return level_1();
}

level_1() {
  return level_2();
}

level_2() {
  return level_3();
}

level_3() {
  int x;
  
  /* Trigger error by calling unknown function */
  return undefined_function_call();
}

/* Test 7: Error in conditional branch */
test_branch_error() {
  int x;
  string s;
  
  s = "test";
  
  /* This will cause type error in condition */
  if (s + 5) {  /* Can't add string and int */
    return 1;
  }
  return 0;
}

/* Test 8: Multiple function calls before error */
test_call_chain() {
  chain_a();
  return 1;
}

chain_a() {
  chain_b();
  return 1;
}

chain_b() {
  chain_c();
  return 1;
}

chain_c() {
  /* Trigger error */
  return missing_function();
}

/* Main test runner - call this to run all tests */
run_all_tests() {
  this_player().listen("\n=== Traceback Test Suite ===\n\n");
  
  this_player().listen("Test 1: Simple error (depth 1)\n");
  this_player().listen("Run: test_simple_error()\n\n");
  
  this_player().listen("Test 2: Nested error (depth 2)\n");
  this_player().listen("Run: test_nested_error()\n\n");
  
  this_player().listen("Test 3: Unknown function\n");
  this_player().listen("Run: test_unknown_function()\n\n");
  
  this_player().listen("Test 4: Deep recursion (stack overflow)\n");
  this_player().listen("Run: test_deep_recursion(0)\n\n");
  
  this_player().listen("Test 5: Arithmetic type error\n");
  this_player().listen("Run: test_arithmetic_error()\n\n");
  
  this_player().listen("Test 6: Three-level nesting\n");
  this_player().listen("Run: test_three_level_nesting()\n\n");
  
  this_player().listen("Test 7: Branch condition error\n");
  this_player().listen("Run: test_branch_error()\n\n");
  
  this_player().listen("Test 8: Call chain error\n");
  this_player().listen("Run: test_call_chain()\n\n");
  
  this_player().listen("=== End of Test List ===\n");
  this_player().listen("Run each test individually to see tracebacks.\n");
  
  return 1;
}

/* Helper to show current sysctl settings */
show_settings() {
  int max_depth, trace_depth, format;
  
  max_depth = sysctl(9);
  trace_depth = sysctl(10);
  format = sysctl(11);
  
  this_player().listen("\n=== Current Traceback Settings ===\n");
  this_player().listen("Max call stack depth: " + itoa(max_depth) + "\n");
  this_player().listen("Max trace depth: " + itoa(trace_depth) + "\n");
  this_player().listen("Trace format: " + (format ? "compact" : "detailed") + "\n");
  this_player().listen("===================================\n\n");
  
  return 1;
}
