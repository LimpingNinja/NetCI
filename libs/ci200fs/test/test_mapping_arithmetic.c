/* test_mapping_arithmetic.c - Phase 5: Mapping Arithmetic Operations */
#include <sys.h>

create() {
  mapping m1, m2, m3;
  int test_result;
  
  this_player().listen("=== Mapping Arithmetic Tests ===\n");
  
  /* Test 1: Basic merge with + operator */
  this_player().listen("\n[Test 1] Basic merge (m1 + m2):\n");
  m1 = ([ "a": 1, "b": 2 ]);
  m2 = ([ "c": 3, "d": 4 ]);
  m3 = m1 + m2;
  this_player().listen("  m1 = ([ \"a\": 1, \"b\": 2 ])\n");
  this_player().listen("  m2 = ([ \"c\": 3, \"d\": 4 ])\n");
  this_player().listen("  m3 = m1 + m2\n");
  this_player().listen("  Result size: " + itoa(sizeof(m3)) + " (expected: 4)\n");
  this_player().listen("  m3[\"a\"] = " + itoa(m3["a"]) + " (expected: 1)\n");
  this_player().listen("  m3[\"b\"] = " + itoa(m3["b"]) + " (expected: 2)\n");
  this_player().listen("  m3[\"c\"] = " + itoa(m3["c"]) + " (expected: 3)\n");
  this_player().listen("  m3[\"d\"] = " + itoa(m3["d"]) + " (expected: 4)\n");
  
  /* Test 2: Merge with overlapping keys (m2 wins) */
  this_player().listen("\n[Test 2] Merge with overlapping keys:\n");
  m1 = ([ "a": 1, "b": 2, "c": 3 ]);
  m2 = ([ "b": 99, "c": 88, "d": 4 ]);
  m3 = m1 + m2;
  this_player().listen("  m1 = ([ \"a\": 1, \"b\": 2, \"c\": 3 ])\n");
  this_player().listen("  m2 = ([ \"b\": 99, \"c\": 88, \"d\": 4 ])\n");
  this_player().listen("  m3 = m1 + m2\n");
  this_player().listen("  Result size: " + itoa(sizeof(m3)) + " (expected: 4)\n");
  this_player().listen("  m3[\"a\"] = " + itoa(m3["a"]) + " (expected: 1)\n");
  this_player().listen("  m3[\"b\"] = " + itoa(m3["b"]) + " (expected: 99 - m2 wins!)\n");
  this_player().listen("  m3[\"c\"] = " + itoa(m3["c"]) + " (expected: 88 - m2 wins!)\n");
  this_player().listen("  m3[\"d\"] = " + itoa(m3["d"]) + " (expected: 4)\n");
  
  /* Test 3: Basic subtraction with - operator */
  this_player().listen("\n[Test 3] Basic subtraction (m1 - m2):\n");
  m1 = ([ "a": 1, "b": 2, "c": 3, "d": 4 ]);
  m2 = ([ "b": 0, "d": 0 ]);
  m3 = m1 - m2;
  this_player().listen("  m1 = ([ \"a\": 1, \"b\": 2, \"c\": 3, \"d\": 4 ])\n");
  this_player().listen("  m2 = ([ \"b\": 0, \"d\": 0 ])\n");
  this_player().listen("  m3 = m1 - m2\n");
  this_player().listen("  Result size: " + itoa(sizeof(m3)) + " (expected: 2)\n");
  this_player().listen("  m3[\"a\"] = " + itoa(m3["a"]) + " (expected: 1)\n");
  this_player().listen("  m3[\"c\"] = " + itoa(m3["c"]) + " (expected: 3)\n");
  test_result = member(m3, "b");
  this_player().listen("  member(m3, \"b\") = " + itoa(test_result) + " (expected: 0 - removed!)\n");
  test_result = member(m3, "d");
  this_player().listen("  member(m3, \"d\") = " + itoa(test_result) + " (expected: 0 - removed!)\n");
  
  /* Test 4: Subtract with no overlap */
  this_player().listen("\n[Test 4] Subtract with no overlap:\n");
  m1 = ([ "a": 1, "b": 2 ]);
  m2 = ([ "x": 99, "y": 88 ]);
  m3 = m1 - m2;
  this_player().listen("  m1 = ([ \"a\": 1, \"b\": 2 ])\n");
  this_player().listen("  m2 = ([ \"x\": 99, \"y\": 88 ])\n");
  this_player().listen("  m3 = m1 - m2\n");
  this_player().listen("  Result size: " + itoa(sizeof(m3)) + " (expected: 2 - nothing removed)\n");
  this_player().listen("  m3[\"a\"] = " + itoa(m3["a"]) + " (expected: 1)\n");
  this_player().listen("  m3[\"b\"] = " + itoa(m3["b"]) + " (expected: 2)\n");
  
  /* Test 5: += operator (merge assignment) */
  this_player().listen("\n[Test 5] Merge assignment (m1 += m2):\n");
  m1 = ([ "a": 1, "b": 2 ]);
  m2 = ([ "c": 3, "b": 99 ]);
  this_player().listen("  Before: m1 = ([ \"a\": 1, \"b\": 2 ])\n");
  this_player().listen("  m2 = ([ \"c\": 3, \"b\": 99 ])\n");
  m1 += m2;
  this_player().listen("  After m1 += m2:\n");
  this_player().listen("  m1 size: " + itoa(sizeof(m1)) + " (expected: 3)\n");
  this_player().listen("  m1[\"a\"] = " + itoa(m1["a"]) + " (expected: 1)\n");
  this_player().listen("  m1[\"b\"] = " + itoa(m1["b"]) + " (expected: 99)\n");
  this_player().listen("  m1[\"c\"] = " + itoa(m1["c"]) + " (expected: 3)\n");
  
  /* Test 6: -= operator (subtract assignment) */
  this_player().listen("\n[Test 6] Subtract assignment (m1 -= m2):\n");
  m1 = ([ "a": 1, "b": 2, "c": 3, "d": 4 ]);
  m2 = ([ "a": 0, "c": 0 ]);
  this_player().listen("  Before: m1 = ([ \"a\": 1, \"b\": 2, \"c\": 3, \"d\": 4 ])\n");
  this_player().listen("  m2 = ([ \"a\": 0, \"c\": 0 ])\n");
  m1 -= m2;
  this_player().listen("  After m1 -= m2:\n");
  this_player().listen("  m1 size: " + itoa(sizeof(m1)) + " (expected: 2)\n");
  this_player().listen("  m1[\"b\"] = " + itoa(m1["b"]) + " (expected: 2)\n");
  this_player().listen("  m1[\"d\"] = " + itoa(m1["d"]) + " (expected: 4)\n");
  test_result = member(m1, "a");
  this_player().listen("  member(m1, \"a\") = " + itoa(test_result) + " (expected: 0 - removed!)\n");
  test_result = member(m1, "c");
  this_player().listen("  member(m1, \"c\") = " + itoa(test_result) + " (expected: 0 - removed!)\n");
  
  /* Test 7: String values in merge */
  this_player().listen("\n[Test 7] String values in merge:\n");
  m1 = ([ "name": "Alice", "age": 25 ]);
  m2 = ([ "city": "NYC", "age": 30 ]);
  m3 = m1 + m2;
  this_player().listen("  m1 = ([ \"name\": \"Alice\", \"age\": 25 ])\n");
  this_player().listen("  m2 = ([ \"city\": \"NYC\", \"age\": 30 ])\n");
  this_player().listen("  m3 = m1 + m2\n");
  this_player().listen("  Result size: " + itoa(sizeof(m3)) + " (expected: 3)\n");
  this_player().listen("  m3[\"name\"] = " + m3["name"] + " (expected: Alice)\n");
  this_player().listen("  m3[\"city\"] = " + m3["city"] + " (expected: NYC)\n");
  this_player().listen("  m3[\"age\"] = " + itoa(m3["age"]) + " (expected: 30 - m2 wins!)\n");
  
  /* Test 8: Chain operations */
  this_player().listen("\n[Test 8] Chain operations:\n");
  m1 = ([ "a": 1 ]);
  m2 = ([ "b": 2 ]);
  m3 = ([ "c": 3 ]);
  m1 = m1 + m2 + m3;
  this_player().listen("  m1 = ([ \"a\": 1 ]) + ([ \"b\": 2 ]) + ([ \"c\": 3 ])\n");
  this_player().listen("  Result size: " + itoa(sizeof(m1)) + " (expected: 3)\n");
  this_player().listen("  m1[\"a\"] = " + itoa(m1["a"]) + " (expected: 1)\n");
  this_player().listen("  m1[\"b\"] = " + itoa(m1["b"]) + " (expected: 2)\n");
  this_player().listen("  m1[\"c\"] = " + itoa(m1["c"]) + " (expected: 3)\n");
  
  this_player().listen("\n=== All Mapping Arithmetic Tests Complete! ===\n");
}
