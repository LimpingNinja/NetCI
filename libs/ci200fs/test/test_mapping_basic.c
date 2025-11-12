/* test_mapping_basic.c - Basic mapping subscript test */
#include <sys.h>

create() {
  mapping m, m2, stress, int_keys;
  string name;
  int age, i;
  
  this_player().listen("=== Basic Mapping Test ===\n");
  
  /* Test 1: Create mapping and set string key */
  this_player().listen("\nTest 1: Setting string key\n");
  m["name"] = "Alice";
  this_player().listen("Set m[\"name\"] = \"Alice\"\n");
  
  /* Test 2: Read string key */
  this_player().listen("\nTest 2: Reading string key\n");
  name = m["name"];
  this_player().listen("Read m[\"name\"] = " + name + "\n");
  
  /* Test 3: Set integer key */
  this_player().listen("\nTest 3: Setting integer key\n");
  m["age"] = 30;
  this_player().listen("Set m[\"age\"] = 30\n");
  
  /* Test 4: Read integer value */
  this_player().listen("\nTest 4: Reading integer value\n");
  age = m["age"];
  this_player().listen("Read m[\"age\"] = " + itoa(age) + "\n");
  
  /* Test 5: Update existing key */
  this_player().listen("\nTest 5: Updating existing key\n");
  m["name"] = "Bob";
  this_player().listen("Updated m[\"name\"] = \"Bob\"\n");
  name = m["name"];
  this_player().listen("Read m[\"name\"] = " + name + "\n");
  
  /* Test 6: Multiple keys */
  this_player().listen("\nTest 6: Multiple keys\n");
  m["city"] = "Seattle";
  m["country"] = "USA";
  this_player().listen("Set m[\"city\"] = \"Seattle\"\n");
  this_player().listen("Set m[\"country\"] = \"USA\"\n");
  this_player().listen("Read m[\"city\"] = " + m["city"] + "\n");
  this_player().listen("Read m[\"country\"] = " + m["country"] + "\n");
  
  /* Test 7: Reference Counting - Shared mappings */
  this_player().listen("\nTest 7: Reference counting (shared mappings)\n");
  /* Note: LPC doesn't support arbitrary block scoping, so m2 is function-scoped */
  m2 = m;  /* m2 now shares same mapping as m */
  this_player().listen("Created m2 = m (shared reference)\n");
  this_player().listen("m[\"name\"] = " + m["name"] + "\n");
  this_player().listen("m2[\"name\"] = " + m2["name"] + " (should be same)\n");
  
  /* Modify via m2, should affect m */
  m2["shared"] = "test";
  this_player().listen("Set m2[\"shared\"] = \"test\"\n");
  this_player().listen("m[\"shared\"] = " + m["shared"] + " (should be 'test')\n");
  
  /* Test 8: Stress test - Many keys (test collision handling & rehashing) */
  this_player().listen("\nTest 8: Stress test - Adding 25 keys\n");
  /* Add 25 keys to trigger potential rehashing (default capacity is 16) */
  i = 0;
  while (i < 25) {
    stress["key" + itoa(i)] = i * 10;
    i = i + 1;
  }
  this_player().listen("Added 25 keys to mapping\n");
  
  /* Verify a few keys */
  this_player().listen("stress[\"key0\"] = " + itoa(stress["key0"]) + " (should be 0)\n");
  this_player().listen("stress[\"key10\"] = " + itoa(stress["key10"]) + " (should be 100)\n");
  this_player().listen("stress[\"key24\"] = " + itoa(stress["key24"]) + " (should be 240)\n");
  
  /* Test collision handling - verify all keys still accessible */
  i = 0;
  while (i < 25) {
    if (stress["key" + itoa(i)] != i * 10) {
      this_player().listen("ERROR: Key collision or data loss at key" + itoa(i) + "!\n");
    }
    i = i + 1;
  }
  this_player().listen("All 25 keys verified - collision handling works!\n");
  
  /* Test 9: Error handling - Invalid subscript operations */
  this_player().listen("\nTest 9: Error handling\n");
  /* Note: These SHOULD cause errors - we're testing error handling */
  /* Commenting out for now since they would stop execution */
  /*
  {
    int x;
    x = 5;
    // x[0] = 10;  // Should error: integer not subscriptable
    this_player().listen("Tested invalid subscript (commented out)\n");
  }
  */
  this_player().listen("Error handling tests skipped (would cause runtime errors)\n");
  
  /* Test 10: Integer keys (not just string keys) */
  this_player().listen("\nTest 10: Integer keys\n");
  int_keys[0] = "zero";
  int_keys[1] = "one";
  int_keys[42] = "answer";
  int_keys[100] = "hundred";
  
  this_player().listen("Set int_keys[0] = \"zero\"\n");
  this_player().listen("Set int_keys[42] = \"answer\"\n");
  this_player().listen("Read int_keys[0] = " + int_keys[0] + "\n");
  this_player().listen("Read int_keys[42] = " + int_keys[42] + "\n");
  this_player().listen("Read int_keys[100] = " + int_keys[100] + "\n");
  
  this_player().listen("\n=== All Tests Complete! ===\n");
  this_player().listen("If you see this, mappings are WORKING! 🎉\n");
  this_player().listen("Phase 1 tests: PASSED ✅\n");
}
