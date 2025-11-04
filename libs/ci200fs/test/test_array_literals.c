/* test_array_literals.c - Interactive test suite for array literals */
#include <sys.h>

create() {
  start_menu();
}

start_menu() {
  this_player().listen("=== Array Literals Test Suite ===\n");
  show_menu();
}

show_menu() {
  this_player().listen("\nArray Literal Tests:\n");
  this_player().listen("1. Test empty array ({})\n");
  this_player().listen("2. Test integer array\n");
  this_player().listen("3. Test string array\n");
  this_player().listen("4. Test mixed type array\n");
  this_player().listen("5. Test nested arrays\n");
  this_player().listen("6. Test arrays with expressions\n");
  this_player().listen("0. Exit\n");
  this_player().listen("\nEnter choice: ");
  input_to(this_object(), "handle_menu");
}

handle_menu(string choice) {
  if (choice == "1") {
    test_empty_array();
  } else if (choice == "2") {
    test_integer_array();
  } else if (choice == "3") {
    test_string_array();
  } else if (choice == "4") {
    test_mixed_array();
  } else if (choice == "5") {
    test_nested_array();
  } else if (choice == "6") {
    test_expression_array();
  } else if (choice == "0") {
    this_player().listen("Exiting array literals test suite.\n");
    return;
  } else {
    this_player().listen("Invalid choice. Try again.\n");
  }
  show_menu();
}

test_empty_array() {
  int *arr;
  int size;
  
  this_player().listen("\n--- Test: Empty Array ---\n");
  arr = ({});
  size = sizeof(arr);
  this_player().listen("Created empty array: ({})\n");
  this_player().listen("Size: " + itoa(size) + "\n");
  
  if (size == 0) {
    this_player().listen("PASS: Empty array has size 0\n");
  } else {
    this_player().listen("FAIL: Empty array should have size 0, got " + itoa(size) + "\n");
  }
}

test_integer_array() {
  int *arr;
  int size;
  
  this_player().listen("\n--- Test: Integer Array ---\n");
  arr = ({ 1, 2, 3, 4, 5 });
  size = sizeof(arr);
  this_player().listen("Created array: ({ 1, 2, 3, 4, 5 })\n");
  this_player().listen("Size: " + itoa(size) + "\n");
  
  if (size == 5) {
    this_player().listen("PASS: Array has correct size\n");
  } else {
    this_player().listen("FAIL: Array should have size 5, got " + itoa(size) + "\n");
  }
  
  this_player().listen("Elements: ");
  this_player().listen("arr[0]=" + itoa(arr[0]) + " ");
  this_player().listen("arr[1]=" + itoa(arr[1]) + " ");
  this_player().listen("arr[2]=" + itoa(arr[2]) + " ");
  this_player().listen("arr[3]=" + itoa(arr[3]) + " ");
  this_player().listen("arr[4]=" + itoa(arr[4]) + "\n");
  
  if (arr[0] == 1 && arr[1] == 2 && arr[2] == 3 && arr[3] == 4 && arr[4] == 5) {
    this_player().listen("PASS: All elements have correct values\n");
  } else {
    this_player().listen("FAIL: Elements have incorrect values\n");
  }
}

test_string_array() {
  string *arr;
  int size;
  
  this_player().listen("\n--- Test: String Array ---\n");
  arr = ({ "hello", "world", "test" });
  size = sizeof(arr);
  this_player().listen("Created array: ({ \"hello\", \"world\", \"test\" })\n");
  this_player().listen("Size: " + itoa(size) + "\n");
  
  if (size == 3) {
    this_player().listen("PASS: Array has correct size\n");
  } else {
    this_player().listen("FAIL: Array should have size 3, got " + itoa(size) + "\n");
  }
  
  this_player().listen("Elements: ");
  this_player().listen("arr[0]=\"" + arr[0] + "\" ");
  this_player().listen("arr[1]=\"" + arr[1] + "\" ");
  this_player().listen("arr[2]=\"" + arr[2] + "\"\n");
  
  if (arr[0] == "hello" && arr[1] == "world" && arr[2] == "test") {
    this_player().listen("PASS: All elements have correct values\n");
  } else {
    this_player().listen("FAIL: Elements have incorrect values\n");
  }
}

test_mixed_array() {
  int *arr;
  int size;
  
  this_player().listen("\n--- Test: Mixed Type Array ---\n");
  arr = ({ 42, "hello", 100 });
  size = sizeof(arr);
  this_player().listen("Created array: ({ 42, \"hello\", 100 })\n");
  this_player().listen("Size: " + itoa(size) + "\n");
  
  if (size == 3) {
    this_player().listen("PASS: Array has correct size\n");
  } else {
    this_player().listen("FAIL: Array should have size 3, got " + itoa(size) + "\n");
  }
  
  this_player().listen("Element types: ");
  this_player().listen("arr[0]=" + itoa(typeof(arr[0])) + " ");
  this_player().listen("arr[1]=" + itoa(typeof(arr[1])) + " ");
  this_player().listen("arr[2]=" + itoa(typeof(arr[2])) + "\n");
}

test_nested_array() {
  int *arr;
  int *inner;
  int size;
  
  this_player().listen("\n--- Test: Nested Arrays ---\n");
  arr = ({ ({ 1, 2 }), ({ 3, 4 }), ({ 5, 6 }) });
  size = sizeof(arr);
  this_player().listen("Created array: ({ ({ 1, 2 }), ({ 3, 4 }), ({ 5, 6 }) })\n");
  this_player().listen("Size: " + itoa(size) + "\n");
  
  if (size == 3) {
    this_player().listen("PASS: Outer array has correct size\n");
  } else {
    this_player().listen("FAIL: Outer array should have size 3, got " + itoa(size) + "\n");
  }
  
  inner = arr[0];
  this_player().listen("First inner array size: " + itoa(sizeof(inner)) + "\n");
  if (sizeof(inner) == 2) {
    this_player().listen("PASS: Inner array has correct size\n");
  } else {
    this_player().listen("FAIL: Inner array should have size 2\n");
  }
}

test_expression_array() {
  int *arr;
  int x, y;
  int size;
  
  this_player().listen("\n--- Test: Arrays with Expressions ---\n");
  x = 10;
  y = 20;
  arr = ({ x + y, x * 2, y / 2 });
  size = sizeof(arr);
  this_player().listen("Created array: ({ x + y, x * 2, y / 2 }) where x=10, y=20\n");
  this_player().listen("Size: " + itoa(size) + "\n");
  
  if (size == 3) {
    this_player().listen("PASS: Array has correct size\n");
  } else {
    this_player().listen("FAIL: Array should have size 3, got " + itoa(size) + "\n");
  }
  
  this_player().listen("Elements: ");
  this_player().listen("arr[0]=" + itoa(arr[0]) + " (should be 30) ");
  this_player().listen("arr[1]=" + itoa(arr[1]) + " (should be 20) ");
  this_player().listen("arr[2]=" + itoa(arr[2]) + " (should be 10)\n");
  
  if (arr[0] == 30 && arr[1] == 20 && arr[2] == 10) {
    this_player().listen("PASS: All expressions evaluated correctly\n");
  } else {
    this_player().listen("FAIL: Expressions did not evaluate correctly\n");
  }
}
