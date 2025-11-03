/* test_driver.c - Interactive test suite for driver features */
#include <sys.h>

create() {
  show_menu();
}

show_menu() {
  this_player().listen("\n=== Driver Test Suite ===\n");
  this_player().listen("1. Test error tracebacks\n");
  this_player().listen("2. Test memory management (placeholder)\n");
  this_player().listen("3. Test call stack (placeholder)\n");
  this_player().listen("4. Run all tests\n");
  this_player().listen("0. Exit\n");
  this_player().listen("\nSelect test: ");
  input_to(this_object(), "handle_choice");
}

handle_choice(string choice) {
  int num;
  
  num = atoi(choice);
  
  if (num == 1) {
    test_tracebacks();
  } else if (num == 2) {
    test_memory();
  } else if (num == 3) {
    test_call_stack();
  } else if (num == 4) {
    run_all_tests();
  } else if (num == 0) {
    this_player().listen("Exiting test suite.\n");
    return;
  } else {
    this_player().listen("Invalid choice.\n");
  }
  
  show_menu();
}

test_tracebacks() {
  object traceback_obj;
  
  this_player().listen("\n--- Test: Error Tracebacks ---\n");
  this_player().listen("Launching interactive traceback test suite...\n\n");
  
  traceback_obj = find_object("/test/traceback_test");
  if (!traceback_obj) {
    this_player().listen("ERROR: Could not find /test/traceback_test\n");
    return;
  }
  
  call_other(traceback_obj, "start_menu");
}

test_memory() {
  this_player().listen("\n--- Test: Memory Management ---\n");
  this_player().listen("This test would check memory allocation/deallocation.\n");
  this_player().listen("PLACEHOLDER: Not implemented yet\n");
}

test_call_stack() {
  this_player().listen("\n--- Test: Call Stack ---\n");
  this_player().listen("This test would examine the call stack depth.\n");
  this_player().listen("PLACEHOLDER: Not implemented yet\n");
}

run_all_tests() {
  this_player().listen("\n=== Running All Driver Tests ===\n");
  test_tracebacks();
  test_memory();
  test_call_stack();
  this_player().listen("\n=== All Tests Complete ===\n");
}
