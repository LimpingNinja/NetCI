/* traceback_test.c - Interactive test suite for improved error traceback system
 *
 * Usage:
 *   Interactive mode:  call /obj/traceback_test start_menu
 *   Single test:       call /obj/traceback_test run 1
 *   All tests:         call /obj/traceback_test run all
 *   Show settings:     call /obj/traceback_test run s
 *   Show menu:         call /obj/traceback_test run h
 *
 * Interactive mode uses input_to(object, function) to redirect input
 * from this_player() to this object's menu_handler function.
 */

#include <sys.h>

/* ========== Line-Level Error Tests ========== */

/* Test 1: Error in middle of function with multiple statements */
test_midfunction_error() {
  int x, y, z;
  string name;
  
  x = 10;
  y = 20;
  name = "test";
  
  /* Error occurs here on this specific line */
  z = x + name;  /* Type mismatch: int + string */
  
  /* This line never executes */
  return z;
}

/* Test 2: Error in complex expression */
test_complex_expression() {
  int a, b, c, result;
  string msg;
  
  a = 5;
  b = 10;
  msg = "value";
  
  /* Error in middle of calculation */
  result = (a * b) + (msg - 3) + (a / b);  /* String arithmetic */
  
  return result;
}

/* Test 3: Error in loop */
test_loop_error() {
  int i, sum;
  string bad_value;
  
  sum = 0;
  bad_value = "oops";
  
  i = 0;
  while (i < 5) {
    sum = sum + i;
    i = i + 1;
    
    /* Error on specific iteration */
    if (i == 3) {
      sum = sum + bad_value;  /* Type error in loop */
    }
  }
  
  return sum;
}

/* Test 4: Error after multiple successful operations */
test_sequential_operations() {
  int step1, step2, step3;
  string problem;
  
  step1 = 100;
  step2 = step1 * 2;
  step3 = step2 / 4;
  problem = "error";
  
  /* Several lines of valid code before error */
  step1 = step1 + 10;
  step2 = step2 - 5;
  step3 = step3 * 3;
  
  /* Error happens here */
  return step1 + step2 + step3 + problem;  /* Type mismatch */
}

/* Test 5: Nested call with mid-function error */
test_nested_midfunction() {
  int value;
  
  value = helper_with_error(42);
  return value;
}

helper_with_error(input) {
  int processed, temp;
  string bad;
  
  processed = input * 2;
  temp = processed + 10;
  bad = "fail";
  
  /* Error in middle of helper function */
  return temp / bad;  /* Type error */
}

/* ========== Call Chain Tests ========== */

/* Test 6: Deep call chain (5 levels) */
test_deep_chain() {
  return chain_level_1();
}

chain_level_1() {
  int x;
  x = 1;
  return chain_level_2(x);
}

chain_level_2(val) {
  int y;
  y = val + 1;
  return chain_level_3(y);
}

chain_level_3(val) {
  int z;
  z = val * 2;
  return chain_level_4(z);
}

chain_level_4(val) {
  string oops;
  oops = "error";
  /* Error at deepest level */
  return val + oops;  /* Type mismatch */
}

/* Test 7: Error in conditional branch */
test_conditional_error() {
  int choice, result;
  string bad;
  
  choice = 2;
  bad = "problem";
  
  if (choice == 1) {
    result = 100;
  } else if (choice == 2) {
    /* Error in this branch */
    result = 50 + bad;  /* Type error */
  } else {
    result = 0;
  }
  
  return result;
}

/* Test 8: Array operation error */
test_array_error() {
  int arr[5];
  int i, sum;
  
  arr[0] = 10;
  arr[1] = 20;
  arr[2] = 30;
  
  sum = arr[0] + arr[1];
  
  /* Array bounds error */
  return arr[100];  /* Out of bounds */
}

/* Test 9: Stack overflow (deep recursion) */
test_stack_overflow(depth) {
  int next;
  
  next = depth + 1;
  
  /* Recurse until stack overflow */
  if (depth < 200) {
    return test_stack_overflow(next);
  }
  
  return depth;
}

/* Test 10: Unknown function call */
test_unknown_call() {
  int x, y;
  
  x = 10;
  y = 20;
  
  /* Call non-existent function */
  return nonexistent_function(x, y);
}

/* ========== Menu System ========== */

menu() {
  this_player().listen("\n");
  this_player().listen("╔════════════════════════════════════════════════════════╗\n");
  this_player().listen("║     TRACEBACK TEST SUITE - Interactive Menu           ║\n");
  this_player().listen("╠════════════════════════════════════════════════════════╣\n");
  this_player().listen("║  Line-Level Error Tests:                              ║\n");
  this_player().listen("║    1. Mid-function error                              ║\n");
  this_player().listen("║    2. Complex expression error                        ║\n");
  this_player().listen("║    3. Error in loop                                   ║\n");
  this_player().listen("║    4. Sequential operations error                     ║\n");
  this_player().listen("║    5. Nested call with mid-function error             ║\n");
  this_player().listen("║                                                        ║\n");
  this_player().listen("║  Call Chain Tests:                                    ║\n");
  this_player().listen("║    6. Deep call chain (5 levels)                      ║\n");
  this_player().listen("║    7. Conditional branch error                        ║\n");
  this_player().listen("║    8. Array bounds error                              ║\n");
  this_player().listen("║    9. Stack overflow (recursion)                      ║\n");
  this_player().listen("║   10. Unknown function call                           ║\n");
  this_player().listen("║                                                        ║\n");
  this_player().listen("║  Utilities:                                           ║\n");
  this_player().listen("║    s. Show current settings                           ║\n");
  this_player().listen("║    a. Run all tests                                   ║\n");
  this_player().listen("║    q. Quit menu                                       ║\n");
  this_player().listen("║    h. Show this menu                                  ║\n");
  this_player().listen("╚════════════════════════════════════════════════════════╝\n");
  this_player().listen("\nEnter choice: ");
  
  return 1;
}

/* Interactive menu handler - called when user enters input */
menu_handler(input) {
  if (!input || input == "" || input == "q" || input == "quit") {
    this_player().listen("\nExiting test menu.\n\n");
    return 1;
  }
  
  /* Process the input */
  run(input);
  
  /* Show menu again and wait for next input */
  this_player().listen("\n");
  menu();
  input_to(this_object(), "menu_handler");  /* Continue accepting input */
  
  return 1;
}

/* Start interactive menu - entry point */
start_menu() {
  this_player().listen("\n╔════════════════════════════════════════════════════════╗\n");
  this_player().listen("║     Welcome to NetCI Traceback Test Suite             ║\n");
  this_player().listen("║     Interactive Mode                                   ║\n");
  this_player().listen("╚════════════════════════════════════════════════════════╝\n\n");
  
  menu();
  input_to(this_object(), "menu_handler");  /* Set up input handler */
  return 1;
}

run(choice) {
  if (!choice) {
    return menu();
  }
  
  if (choice == "h" || choice == "help") {
    return menu();
  }
  
  if (choice == "s" || choice == "settings") {
    return show_settings();
  }
  
  if (choice == "a" || choice == "all") {
    return run_all();
  }
  
  /* Run specific test */
  if (choice == "1" || choice == 1) {
    this_player().listen("\n>>> Running Test 1: Mid-function error\n");
    return test_midfunction_error();
  }
  
  if (choice == "2" || choice == 2) {
    this_player().listen("\n>>> Running Test 2: Complex expression error\n");
    return test_complex_expression();
  }
  
  if (choice == "3" || choice == 3) {
    this_player().listen("\n>>> Running Test 3: Error in loop\n");
    return test_loop_error();
  }
  
  if (choice == "4" || choice == 4) {
    this_player().listen("\n>>> Running Test 4: Sequential operations error\n");
    return test_sequential_operations();
  }
  
  if (choice == "5" || choice == 5) {
    this_player().listen("\n>>> Running Test 5: Nested call with mid-function error\n");
    return test_nested_midfunction();
  }
  
  if (choice == "6" || choice == 6) {
    this_player().listen("\n>>> Running Test 6: Deep call chain (5 levels)\n");
    return test_deep_chain();
  }
  
  if (choice == "7" || choice == 7) {
    this_player().listen("\n>>> Running Test 7: Conditional branch error\n");
    return test_conditional_error();
  }
  
  if (choice == "8" || choice == 8) {
    this_player().listen("\n>>> Running Test 8: Array bounds error\n");
    return test_array_error();
  }
  
  if (choice == "9" || choice == 9) {
    this_player().listen("\n>>> Running Test 9: Stack overflow\n");
    return test_stack_overflow(0);
  }
  
  if (choice == "10" || choice == 10) {
    this_player().listen("\n>>> Running Test 10: Unknown function call\n");
    return test_unknown_call();
  }
  
  this_player().listen("\nInvalid choice. Type 'call /obj/traceback_test run h' for menu.\n");
  return 0;
}

run_all() {
  int i;
  
  this_player().listen("\n╔════════════════════════════════════════════════════════╗\n");
  this_player().listen("║          Running All Tests (Expect Errors!)           ║\n");
  this_player().listen("╚════════════════════════════════════════════════════════╝\n\n");
  
  i = 1;
  while (i <= 10) {
    this_player().listen("─────────────────────────────────────────────────────────\n");
    run(i);
    i = i + 1;
  }
  
  this_player().listen("\n╔════════════════════════════════════════════════════════╗\n");
  this_player().listen("║              All Tests Complete                        ║\n");
  this_player().listen("╚════════════════════════════════════════════════════════╝\n\n");
  
  return 1;
}

show_settings() {
  int max_depth, trace_depth, format;
  
  max_depth = sysctl(9);
  trace_depth = sysctl(10);
  format = sysctl(11);
  
  this_player().listen("\n╔════════════════════════════════════════════════════════╗\n");
  this_player().listen("║         Current Traceback Settings                    ║\n");
  this_player().listen("╠════════════════════════════════════════════════════════╣\n");
  this_player().listen("║  Max call stack depth: " + itoa(max_depth));
  this_player().listen("                              ║\n");
  this_player().listen("║  Max trace depth:      " + itoa(trace_depth));
  this_player().listen("                              ║\n");
  this_player().listen("║  Trace format:         " + (format ? "compact" : "detailed"));
  this_player().listen("                       ║\n");
  this_player().listen("╚════════════════════════════════════════════════════════╝\n\n");
  
  return 1;
}
