/* test_array_math.c - Interactive test suite for array arithmetic operations */
#include <sys.h>

create() {
  start_menu();
}

start_menu() {
  this_player().listen("=== Array Arithmetic Test Suite ===\n");
  show_menu();
}

show_menu() {
  this_player().listen("\nArray Arithmetic Tests:\n");
  this_player().listen("1. Test array concatenation (+)\n");
  this_player().listen("2. Test array concatenation assignment (+=)\n");
  this_player().listen("3. Test array subtraction (-)\n");
  this_player().listen("4. Test array subtraction assignment (-=)\n");
  this_player().listen("5. Test complex operations\n");
  this_player().listen("6. Run all tests\n");
  this_player().listen("0. Exit\n");
  this_player().listen("\nEnter choice: ");
  input_to(this_object(), "handle_menu");
}

handle_menu(string choice) {
  if (choice == "1") {
    test_concatenation();
  } else if (choice == "2") {
    test_concat_assignment();
  } else if (choice == "3") {
    test_subtraction();
  } else if (choice == "4") {
    test_subtract_assignment();
  } else if (choice == "5") {
    test_complex();
  } else if (choice == "6") {
    run_all_tests();
  } else if (choice == "0") {
    this_player().listen("Exiting array arithmetic test suite.\n");
    return;
  } else {
    this_player().listen("Invalid choice. Try again.\n");
  }
  show_menu();
}

test_concatenation() {
  int *arr1, *arr2, *result;
  int size;
  
  this_player().listen("\n--- Test: Array Concatenation (+) ---\n");
  
  arr1 = ({ 1, 2, 3 });
  arr2 = ({ 4, 5, 6 });
  result = arr1 + arr2;
  
  size = sizeof(result);
  this_player().listen("arr1 = ({ 1, 2, 3 })\n");
  this_player().listen("arr2 = ({ 4, 5, 6 })\n");
  this_player().listen("result = arr1 + arr2\n");
  this_player().listen("Result size: " + itoa(size) + "\n");
  
  if (size == 6) {
    this_player().listen("PASS: Result has correct size\n");
  } else {
    this_player().listen("FAIL: Expected size 6, got " + itoa(size) + "\n");
  }
  
  this_player().listen("Elements: ");
  this_player().listen(itoa(result[0]) + " ");
  this_player().listen(itoa(result[1]) + " ");
  this_player().listen(itoa(result[2]) + " ");
  this_player().listen(itoa(result[3]) + " ");
  this_player().listen(itoa(result[4]) + " ");
  this_player().listen(itoa(result[5]) + "\n");
  
  if (result[0] == 1 && result[1] == 2 && result[2] == 3 &&
      result[3] == 4 && result[4] == 5 && result[5] == 6) {
    this_player().listen("PASS: All elements correct\n");
  } else {
    this_player().listen("FAIL: Elements incorrect\n");
  }
  
  /* Test with strings */
  string *words1, *words2, *words_result;
  words1 = ({ "hello", "world" });
  words2 = ({ "foo", "bar" });
  words_result = words1 + words2;
  
  this_player().listen("\nString array test:\n");
  this_player().listen("words1 = ({ \"hello\", \"world\" })\n");
  this_player().listen("words2 = ({ \"foo\", \"bar\" })\n");
  this_player().listen("Result size: " + itoa(sizeof(words_result)) + "\n");
  
  if (sizeof(words_result) == 4) {
    this_player().listen("PASS: String array concatenation size correct\n");
  } else {
    this_player().listen("FAIL: String array concatenation size incorrect\n");
  }
}

test_concat_assignment() {
  int *arr;
  int size;
  
  this_player().listen("\n--- Test: Array Concatenation Assignment (+=) ---\n");
  
  arr = ({ 1, 2, 3 });
  this_player().listen("Initial: arr = ({ 1, 2, 3 })\n");
  this_player().listen("Size: " + itoa(sizeof(arr)) + "\n");
  
  arr += ({ 4, 5, 6 });
  this_player().listen("After arr += ({ 4, 5, 6 }):\n");
  
  size = sizeof(arr);
  this_player().listen("Size: " + itoa(size) + "\n");
  
  if (size == 6) {
    this_player().listen("PASS: Size correct after +=\n");
  } else {
    this_player().listen("FAIL: Expected size 6, got " + itoa(size) + "\n");
  }
  
  this_player().listen("Elements: ");
  this_player().listen(itoa(arr[0]) + " ");
  this_player().listen(itoa(arr[1]) + " ");
  this_player().listen(itoa(arr[2]) + " ");
  this_player().listen(itoa(arr[3]) + " ");
  this_player().listen(itoa(arr[4]) + " ");
  this_player().listen(itoa(arr[5]) + "\n");
  
  if (arr[0] == 1 && arr[1] == 2 && arr[2] == 3 &&
      arr[3] == 4 && arr[4] == 5 && arr[5] == 6) {
    this_player().listen("PASS: All elements correct\n");
  } else {
    this_player().listen("FAIL: Elements incorrect\n");
  }
  
  /* Test multiple concatenations */
  arr += ({ 7, 8 });
  this_player().listen("\nAfter arr += ({ 7, 8 }):\n");
  this_player().listen("Size: " + itoa(sizeof(arr)) + "\n");
  
  if (sizeof(arr) == 8) {
    this_player().listen("PASS: Multiple += operations work\n");
  } else {
    this_player().listen("FAIL: Multiple += failed\n");
  }
}

test_subtraction() {
  int *arr1, *arr2, *result;
  int size;
  
  this_player().listen("\n--- Test: Array Subtraction (-) ---\n");
  
  arr1 = ({ 1, 2, 3, 4, 5 });
  arr2 = ({ 2, 4 });
  result = arr1 - arr2;
  
  this_player().listen("arr1 = ({ 1, 2, 3, 4, 5 })\n");
  this_player().listen("arr2 = ({ 2, 4 })\n");
  this_player().listen("result = arr1 - arr2\n");
  
  size = sizeof(result);
  this_player().listen("Result size: " + itoa(size) + "\n");
  
  if (size == 3) {
    this_player().listen("PASS: Result has correct size\n");
  } else {
    this_player().listen("FAIL: Expected size 3, got " + itoa(size) + "\n");
  }
  
  this_player().listen("Elements: ");
  this_player().listen(itoa(result[0]) + " ");
  this_player().listen(itoa(result[1]) + " ");
  this_player().listen(itoa(result[2]) + "\n");
  
  if (result[0] == 1 && result[1] == 3 && result[2] == 5) {
    this_player().listen("PASS: Correct elements remain (1, 3, 5)\n");
  } else {
    this_player().listen("FAIL: Elements incorrect\n");
  }
  
  /* Test removing duplicates */
  arr1 = ({ 1, 2, 2, 3, 2, 4 });
  arr2 = ({ 2 });
  result = arr1 - arr2;
  
  this_player().listen("\nDuplicate removal test:\n");
  this_player().listen("arr1 = ({ 1, 2, 2, 3, 2, 4 })\n");
  this_player().listen("arr2 = ({ 2 })\n");
  this_player().listen("result = arr1 - arr2\n");
  this_player().listen("Result size: " + itoa(sizeof(result)) + "\n");
  
  if (sizeof(result) == 3) {
    this_player().listen("PASS: All occurrences of 2 removed\n");
  } else {
    this_player().listen("FAIL: Should have removed all 2's\n");
  }
}

test_subtract_assignment() {
  int *arr;
  int size;
  
  this_player().listen("\n--- Test: Array Subtraction Assignment (-=) ---\n");
  
  arr = ({ 1, 2, 3, 4, 5 });
  this_player().listen("Initial: arr = ({ 1, 2, 3, 4, 5 })\n");
  this_player().listen("Size: " + itoa(sizeof(arr)) + "\n");
  
  arr -= ({ 2, 4 });
  this_player().listen("After arr -= ({ 2, 4 }):\n");
  
  size = sizeof(arr);
  this_player().listen("Size: " + itoa(size) + "\n");
  
  if (size == 3) {
    this_player().listen("PASS: Size correct after -=\n");
  } else {
    this_player().listen("FAIL: Expected size 3, got " + itoa(size) + "\n");
  }
  
  this_player().listen("Elements: ");
  this_player().listen(itoa(arr[0]) + " ");
  this_player().listen(itoa(arr[1]) + " ");
  this_player().listen(itoa(arr[2]) + "\n");
  
  if (arr[0] == 1 && arr[1] == 3 && arr[2] == 5) {
    this_player().listen("PASS: Correct elements remain\n");
  } else {
    this_player().listen("FAIL: Elements incorrect\n");
  }
  
  /* Test multiple subtractions */
  arr -= ({ 3 });
  this_player().listen("\nAfter arr -= ({ 3 }):\n");
  this_player().listen("Size: " + itoa(sizeof(arr)) + "\n");
  
  if (sizeof(arr) == 2 && arr[0] == 1 && arr[1] == 5) {
    this_player().listen("PASS: Multiple -= operations work\n");
  } else {
    this_player().listen("FAIL: Multiple -= failed\n");
  }
}

test_complex() {
  int *arr, *temp;
  
  this_player().listen("\n--- Test: Complex Operations ---\n");
  
  /* Chained operations */
  arr = ({ 1, 2, 3 }) + ({ 4, 5 }) + ({ 6 });
  this_player().listen("Chained concatenation:\n");
  this_player().listen("arr = ({ 1, 2, 3 }) + ({ 4, 5 }) + ({ 6 })\n");
  this_player().listen("Size: " + itoa(sizeof(arr)) + "\n");
  
  if (sizeof(arr) == 6) {
    this_player().listen("PASS: Chained concatenation works\n");
  } else {
    this_player().listen("FAIL: Chained concatenation failed\n");
  }
  
  /* Concatenate then subtract */
  arr = ({ 1, 2, 3 }) + ({ 4, 5, 6 });
  arr = arr - ({ 2, 5 });
  this_player().listen("\nConcatenate then subtract:\n");
  this_player().listen("arr = ({ 1, 2, 3 }) + ({ 4, 5, 6 })\n");
  this_player().listen("arr = arr - ({ 2, 5 })\n");
  this_player().listen("Size: " + itoa(sizeof(arr)) + "\n");
  
  if (sizeof(arr) == 4) {
    this_player().listen("PASS: Combined operations work\n");
  } else {
    this_player().listen("FAIL: Combined operations failed\n");
  }
  
  /* Empty array operations */
  arr = ({});
  arr += ({ 1, 2, 3 });
  this_player().listen("\nEmpty array concatenation:\n");
  this_player().listen("arr = ({})\n");
  this_player().listen("arr += ({ 1, 2, 3 })\n");
  this_player().listen("Size: " + itoa(sizeof(arr)) + "\n");
  
  if (sizeof(arr) == 3) {
    this_player().listen("PASS: Empty array += works\n");
  } else {
    this_player().listen("FAIL: Empty array += failed\n");
  }
  
  /* Subtract everything */
  arr = ({ 1, 2, 3 });
  arr -= ({ 1, 2, 3 });
  this_player().listen("\nSubtract all elements:\n");
  this_player().listen("arr = ({ 1, 2, 3 })\n");
  this_player().listen("arr -= ({ 1, 2, 3 })\n");
  this_player().listen("Size: " + itoa(sizeof(arr)) + "\n");
  
  if (sizeof(arr) == 0) {
    this_player().listen("PASS: Subtracting all elements results in empty array\n");
  } else {
    this_player().listen("FAIL: Should be empty array\n");
  }
}

run_all_tests() {
  this_player().listen("\n=== Running All Tests ===\n");
  test_concatenation();
  test_concat_assignment();
  test_subtraction();
  test_subtract_assignment();
  test_complex();
  this_player().listen("\n=== All Tests Complete ===\n");
}
