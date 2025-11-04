/* test_mapping_basic.c - Basic mapping subscript test */
#include <sys.h>

create() {
  mapping m;
  string name;
  int age;
  
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
  
  this_player().listen("\n=== All Tests Complete! ===\n");
  this_player().listen("If you see this, mappings are WORKING! 🎉\n");
}
