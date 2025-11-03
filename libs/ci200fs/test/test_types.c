/* test_types.c - Interactive test suite for type system and arrays */
#include <sys.h>

create() {
  show_menu();
}

show_menu() {
  this_player().listen("\n=== Type System Test Suite ===\n");
  this_player().listen("1. Test typeof()\n");
  this_player().listen("2. Test sizeof()\n");
  this_player().listen("3. Test dynamic array growth\n");
  this_player().listen("4. Test array bounds checking\n");
  this_player().listen("5. Test pointer syntax\n");
  this_player().listen("6. Run all tests\n");
  this_player().listen("0. Exit\n");
  this_player().listen("\nSelect test: ");
  input_to(this_object(), "handle_choice");
}

handle_choice(string choice) {
  int num;
  
  num = atoi(choice);
  
  if (num == 1) {
    test_typeof();
  } else if (num == 2) {
    test_sizeof();
  } else if (num == 3) {
    test_dynamic_growth();
  } else if (num == 4) {
    test_bounds_checking();
  } else if (num == 5) {
    test_pointer_syntax();
  } else if (num == 6) {
    run_all_tests();
  } else if (num == 0) {
    this_player().listen("Exiting test suite.\n");
    return;
  } else {
    this_player().listen("Invalid choice.\n");
  }
  
  show_menu();
}

test_typeof() {
  int test_arr[5];
  int num;
  string str;
  
  num = 42;
  str = "hello";
  
  this_player().listen("\n--- Test: typeof() ---\n");
  this_player().listen("typeof(test_arr) = " + itoa(typeof(test_arr)) + " (expected 17 for ARRAY)\n");
  this_player().listen("typeof(42) = " + itoa(typeof(num)) + " (expected 0 for INTEGER)\n");
  this_player().listen("typeof(\"hello\") = " + itoa(typeof(str)) + " (expected 1 for STRING)\n");
  
  if (typeof(test_arr) == 17 && typeof(num) == 0 && typeof(str) == 1) {
    this_player().listen("PASS\n");
  } else {
    this_player().listen("FAIL\n");
  }
}

test_sizeof() {
  int arr[10];
  int *unlimited_arr;
  int size1, size2;
  
  this_player().listen("\n--- Test: sizeof() ---\n");
  
  size1 = sizeof(arr);
  this_player().listen("sizeof(int arr[10]) = " + itoa(size1) + " (expected 10)\n");
  
  size2 = sizeof(unlimited_arr);
  this_player().listen("sizeof(int *unlimited_arr) = " + itoa(size2) + " (expected 0 initially)\n");
  
  unlimited_arr[5] = 42;
  size2 = sizeof(unlimited_arr);
  this_player().listen("After unlimited_arr[5]=42, sizeof = " + itoa(size2) + " (expected 6)\n");
  
  if (size1 == 10 && size2 == 6) {
    this_player().listen("PASS\n");
  } else {
    this_player().listen("FAIL\n");
  }
}

test_dynamic_growth() {
  int *unlimited_arr;
  
  this_player().listen("\n--- Test: Dynamic Array Growth ---\n");
  this_player().listen("Initial sizeof(unlimited_arr) = " + itoa(sizeof(unlimited_arr)) + "\n");
  
  unlimited_arr[0] = 100;
  this_player().listen("After unlimited_arr[0]=100, sizeof = " + itoa(sizeof(unlimited_arr)) + "\n");
  
  unlimited_arr[10] = 200;
  this_player().listen("After unlimited_arr[10]=200, sizeof = " + itoa(sizeof(unlimited_arr)) + "\n");
  
  unlimited_arr[300] = 300;
  this_player().listen("After unlimited_arr[300]=300, sizeof = " + itoa(sizeof(unlimited_arr)) + "\n");
  
  this_player().listen("unlimited_arr[0] = " + itoa(unlimited_arr[0]) + "\n");
  this_player().listen("unlimited_arr[10] = " + itoa(unlimited_arr[10]) + "\n");
  this_player().listen("unlimited_arr[300] = " + itoa(unlimited_arr[300]) + "\n");
  
  if (sizeof(unlimited_arr) == 301 && unlimited_arr[0] == 100 && 
      unlimited_arr[10] == 200 && unlimited_arr[300] == 300) {
    this_player().listen("PASS: Unlimited array grows correctly\n");
  } else {
    this_player().listen("FAIL\n");
  }
}

test_bounds_checking() {
  int limited_arr[5];
  
  this_player().listen("\n--- Test: Array Bounds Checking ---\n");
  this_player().listen("sizeof(limited_arr[5]) = " + itoa(sizeof(limited_arr)) + "\n");
  
  limited_arr[0] = 10;
  limited_arr[4] = 50;
  
  this_player().listen("limited_arr[0] = " + itoa(limited_arr[0]) + "\n");
  this_player().listen("limited_arr[4] = " + itoa(limited_arr[4]) + "\n");
  this_player().listen("PASS: Valid indices work\n");
  
  this_player().listen("\nAttempting out-of-bounds access: limited_arr[10]...\n");
  this_player().listen("(This should trigger a bounds error)\n");
  
  limited_arr[10] = 999;
  this_player().listen("FAIL: Should not reach here!\n");
  /*
  this_player().listen("SKIPPED: Bounds violation test disabled (would crash)\n");
  */
}

test_pointer_syntax() {
  int *ptr_arr;
  int *sized_arr[5];
  int normal_arr[5];
  int size1, size2, size3;
  
  this_player().listen("\n--- Test: Pointer Syntax ---\n");
  
  size1 = sizeof(ptr_arr);
  this_player().listen("sizeof(int *ptr_arr) = " + itoa(size1) + "\n");
  
  size2 = sizeof(sized_arr);
  this_player().listen("sizeof(int *sized_arr[5]) = " + itoa(size2) + "\n");
  
  size3 = sizeof(normal_arr);
  this_player().listen("sizeof(int normal_arr[5]) = " + itoa(size3) + "\n");
  
  if (size2 == size3 && size3 == 5) {
    this_player().listen("PASS: Pointer syntax works like normal arrays\n");
  } else {
    this_player().listen("FAIL: Sizes don't match\n");
  }
  
  /* Test assignment and access */
  ptr_arr[0] = 42;
  sized_arr[0] = 100;
  normal_arr[0] = 200;
  
  this_player().listen("ptr_arr[0] = " + itoa(ptr_arr[0]) + "\n");
  this_player().listen("sized_arr[0] = " + itoa(sized_arr[0]) + "\n");
  this_player().listen("normal_arr[0] = " + itoa(normal_arr[0]) + "\n");
  
  /* Test multiplication doesn't break */
  int *test_arr;
  test_arr[0] = 5;
  test_arr[1] = 4;
  this_player().listen("test_arr[0] * test_arr[1] = " + itoa(test_arr[0] * test_arr[1]) + " (expected 20)\n");
  
  if (test_arr[0] * test_arr[1] == 20) {
    this_player().listen("PASS: Multiplication works correctly\n");
  } else {
    this_player().listen("FAIL\n");
  }
}

run_all_tests() {
  this_player().listen("\n=== Running All Type System Tests ===\n");
  test_typeof();
  test_sizeof();
  test_dynamic_growth();
  test_bounds_checking();
  test_pointer_syntax();
  this_player().listen("\n=== All Tests Complete ===\n");
}
