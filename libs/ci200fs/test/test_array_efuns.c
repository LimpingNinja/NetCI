/* test_array_efuns.c - Interactive test suite for array efuns */
#include <sys.h>

/* Global arrays for test results */
string *exploded_result;
int *sorted_nums;
int *unique_result;

create() {
  show_menu();
}

show_menu() {
  this_player().listen("\n=== Array Efuns Test Suite ===\n");
  this_player().listen("1. Test implode()\n");
  this_player().listen("2. Test explode()\n");
  this_player().listen("3. Test member_array()\n");
  this_player().listen("4. Test sort_array()\n");
  this_player().listen("5. Test reverse()\n");
  this_player().listen("6. Test unique_array()\n");
  this_player().listen("7. Run all tests\n");
  this_player().listen("0. Exit\n");
  this_player().listen("\nSelect test: ");
  input_to(this_object(), "handle_choice");
}

handle_choice(string choice) {
  int num;
  
  num = atoi(choice);
  
  if (num == 1) {
    test_implode();
  } else if (num == 2) {
    test_explode();
  } else if (num == 3) {
    test_member_array();
  } else if (num == 4) {
    test_sort_array();
  } else if (num == 5) {
    test_reverse();
  } else if (num == 6) {
    test_unique_array();
  } else if (num == 7) {
    run_all_tests();
  } else if (num == 0) {
    this_player().listen("Exiting test suite.\n");
    return;
  } else {
    this_player().listen("Invalid choice.\n");
  }
  
  show_menu();
}

test_implode() {
  string str_arr[3];
  string result_str;
  
  this_player().listen("\n--- Test: implode() ---\n");
  str_arr[0] = "apple";
  str_arr[1] = "banana";
  str_arr[2] = "cherry";
  result_str = implode(str_arr, ", ");
  this_player().listen("implode([apple, banana, cherry], \", \") = " + result_str + "\n");
  this_player().listen("Expected: apple, banana, cherry\n");
  if (result_str == "apple, banana, cherry") {
    this_player().listen("PASS\n");
  } else {
    this_player().listen("FAIL\n");
  }
}

test_explode() {
  this_player().listen("\n--- Test: explode() ---\n");
  exploded_result = explode("one,two,three", ",");
  this_player().listen("explode(\"one,two,three\", \",\")\n");
  this_player().listen("sizeof(result) = " + itoa(sizeof(exploded_result)) + " (expected 3)\n");
  this_player().listen("result[0] = " + exploded_result[0] + " (expected 'one')\n");
  this_player().listen("result[1] = " + exploded_result[1] + " (expected 'two')\n");
  this_player().listen("result[2] = " + exploded_result[2] + " (expected 'three')\n");
  if (sizeof(exploded_result) == 3 && exploded_result[0] == "one" && 
      exploded_result[1] == "two" && exploded_result[2] == "three") {
    this_player().listen("PASS\n");
  } else {
    this_player().listen("FAIL\n");
  }
}

test_member_array() {
  int test_arr[5];
  int idx;
  
  this_player().listen("\n--- Test: member_array() ---\n");
  test_arr[0] = 10;
  test_arr[1] = 20;
  test_arr[2] = 30;
  test_arr[3] = 40;
  test_arr[4] = 50;
  
  idx = member_array(30, test_arr);
  this_player().listen("member_array(30, [10,20,30,40,50]) = " + itoa(idx) + " (expected 2)\n");
  
  idx = member_array(99, test_arr);
  this_player().listen("member_array(99, [10,20,30,40,50]) = " + itoa(idx) + " (expected -1)\n");
  
  if (member_array(30, test_arr) == 2 && member_array(99, test_arr) == -1) {
    this_player().listen("PASS\n");
  } else {
    this_player().listen("FAIL\n");
  }
}

test_sort_array() {
  int test_arr[5];
  
  this_player().listen("\n--- Test: sort_array() ---\n");
  test_arr[0] = 50;
  test_arr[1] = 10;
  test_arr[2] = 40;
  test_arr[3] = 20;
  test_arr[4] = 30;
  
  this_player().listen("Before sort: " + implode(test_arr, ",") + "\n");
  sorted_nums = sort_array(test_arr);
  this_player().listen("After sort:  " + implode(sorted_nums, ",") + "\n");
  this_player().listen("Expected:    10,20,30,40,50\n");
  
  if (sorted_nums[0] == 10 && sorted_nums[1] == 20 && sorted_nums[2] == 30 &&
      sorted_nums[3] == 40 && sorted_nums[4] == 50) {
    this_player().listen("PASS\n");
  } else {
    this_player().listen("FAIL\n");
  }
}

test_reverse() {
  int test_arr[5];
  
  this_player().listen("\n--- Test: reverse() ---\n");
  test_arr[0] = 1;
  test_arr[1] = 2;
  test_arr[2] = 3;
  test_arr[3] = 4;
  test_arr[4] = 5;
  
  this_player().listen("Before reverse: " + implode(test_arr, ",") + "\n");
  test_arr = reverse(test_arr);
  this_player().listen("After reverse:  " + implode(test_arr, ",") + "\n");
  this_player().listen("Expected:       5,4,3,2,1\n");
  
  if (test_arr[0] == 5 && test_arr[1] == 4 && test_arr[2] == 3 &&
      test_arr[3] == 2 && test_arr[4] == 1) {
    this_player().listen("PASS\n");
  } else {
    this_player().listen("FAIL\n");
  }
}

test_unique_array() {
  int test_arr[5];
  
  this_player().listen("\n--- Test: unique_array() ---\n");
  test_arr[0] = 1;
  test_arr[1] = 2;
  test_arr[2] = 2;
  test_arr[3] = 3;
  test_arr[4] = 1;
  
  this_player().listen("Before unique: " + implode(test_arr, ",") + "\n");
  unique_result = unique_array(test_arr);
  this_player().listen("After unique:  " + implode(unique_result, ",") + "\n");
  this_player().listen("sizeof(result) = " + itoa(sizeof(unique_result)) + " (expected 3)\n");
  this_player().listen("Expected:      1,2,3\n");
  
  if (sizeof(unique_result) == 3 && unique_result[0] == 1 && 
      unique_result[1] == 2 && unique_result[2] == 3) {
    this_player().listen("PASS\n");
  } else {
    this_player().listen("FAIL\n");
  }
}

run_all_tests() {
  this_player().listen("\n=== Running All Array Efuns Tests ===\n");
  test_implode();
  test_explode();
  test_member_array();
  test_sort_array();
  test_reverse();
  test_unique_array();
  this_player().listen("\n=== All Tests Complete ===\n");
}
