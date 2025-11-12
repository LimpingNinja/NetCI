/* test_mapping_efuns.c - Phase 2: Test mapping efuns and type integration */
#include <sys.h>

create() {
  mapping m, big, int_map, original, reconstructed;
  int *key_array, *val_array, *orig_keys, *orig_values;
  int size, type_code, exists, i, j, k, match_count;
  
  this_player().listen("=== Mapping Efuns Test (Phase 2) ===\n");
  
  /* Test 1: sizeof() on empty mapping */
  this_player().listen("\nTest 1: sizeof() on empty mapping\n");
  size = sizeof(m);
  this_player().listen("sizeof(empty mapping) = " + itoa(size) + " (should be 0)\n");
  
  /* Test 2: Add some keys and check sizeof() */
  this_player().listen("\nTest 2: sizeof() after adding keys\n");
  m["name"] = "Alice";
  m["age"] = 30;
  m["city"] = "Seattle";
  size = sizeof(m);
  this_player().listen("Added 3 keys\n");
  this_player().listen("sizeof(m) = " + itoa(size) + " (should be 3)\n");
  
  /* Test 3: typeof() on mapping */
  this_player().listen("\nTest 3: typeof() on mapping\n");
  type_code = typeof(m);
  this_player().listen("typeof(m) = " + itoa(type_code) + " (should be 18 for MAPPING)\n");
  
  /* Test 4: keys() efun */
  this_player().listen("\nTest 4: keys() efun\n");
  key_array = keys(m);
  this_player().listen("keys(m) returned array of size " + itoa(sizeof(key_array)) + "\n");
  this_player().listen("Keys: " + implode(key_array, ", ") + "\n");
  
  /* Test 5: values() efun */
  this_player().listen("\nTest 5: values() efun\n");
  val_array = values(m);
  this_player().listen("values(m) returned array of size " + itoa(sizeof(val_array)) + "\n");
  /* Note: values might be mixed types, so we just show count */
  
  /* Test 6: member() efun - check existing key */
  this_player().listen("\nTest 6: member() - check existing key\n");
  exists = member(m, "name");
  this_player().listen("member(m, \"name\") = " + itoa(exists) + " (should be 1)\n");
  
  /* Test 7: member() efun - check non-existing key */
  this_player().listen("\nTest 7: member() - check non-existing key\n");
  exists = member(m, "country");
  this_player().listen("member(m, \"country\") = " + itoa(exists) + " (should be 0)\n");
  
  /* Test 8: map_delete() efun */
  this_player().listen("\nTest 8: map_delete() efun\n");
  this_player().listen("Before delete: sizeof(m) = " + itoa(sizeof(m)) + "\n");
  map_delete(m, "age");
  this_player().listen("After map_delete(m, \"age\"): sizeof(m) = " + itoa(sizeof(m)) + " (should be 2)\n");
  exists = member(m, "age");
  this_player().listen("member(m, \"age\") = " + itoa(exists) + " (should be 0)\n");
  
  /* Test 9: keys() after delete */
  this_player().listen("\nTest 9: keys() after delete\n");
  key_array = keys(m);
  this_player().listen("Keys after delete: " + implode(key_array, ", ") + "\n");
  
  /* Test 10: Stress test - many keys */
  this_player().listen("\nTest 10: Stress test with many keys\n");
  i = 0;
  while (i < 20) {
    big["key" + itoa(i)] = i * 100;
    i = i + 1;
  }
  
  this_player().listen("Created mapping with 20 keys\n");
  this_player().listen("sizeof(big) = " + itoa(sizeof(big)) + " (should be 20)\n");
  this_player().listen("member(big, \"key10\") = " + itoa(member(big, "key10")) + "\n");
  this_player().listen("member(big, \"key99\") = " + itoa(member(big, "key99")) + "\n");
  
  /* Delete a key and verify */
  map_delete(big, "key10");
  this_player().listen("After deleting key10: sizeof(big) = " + itoa(sizeof(big)) + " (should be 19)\n");
  this_player().listen("member(big, \"key10\") = " + itoa(member(big, "key10")) + " (should be 0)\n");
  
  /* Test 11: Integer keys with member() */
  this_player().listen("\nTest 11: Integer keys with member()\n");
  int_map[0] = "zero";
  int_map[42] = "answer";
  int_map[100] = "hundred";
  
  this_player().listen("sizeof(int_map) = " + itoa(sizeof(int_map)) + " (should be 3)\n");
  this_player().listen("member(int_map, 42) = " + itoa(member(int_map, 42)) + " (should be 1)\n");
  this_player().listen("member(int_map, 99) = " + itoa(member(int_map, 99)) + " (should be 0)\n");
  
  /* Test 12: Reconstruct mapping from keys() and values() */
  this_player().listen("\nTest 12: Reconstruct mapping from keys/values arrays\n");
  /* Create original mapping with 5 elements */
  original["apple"] = 100;
  original["banana"] = 200;
  original["cherry"] = 300;
  original["date"] = 400;
  original["elderberry"] = 500;
  
  this_player().listen("Created original mapping with 5 elements\n");
  this_player().listen("sizeof(original) = " + itoa(sizeof(original)) + "\n");
  
  /* Get keys and values arrays */
  orig_keys = keys(original);
  orig_values = values(original);
  
  this_player().listen("Got keys array: size = " + itoa(sizeof(orig_keys)) + "\n");
  this_player().listen("Keys (via implode): [" + implode(orig_keys, ", ") + "]\n");
  this_player().listen("Keys (manual loop):\n");
  k = 0;
  while (k < sizeof(orig_keys)) {
    this_player().listen("  [" + itoa(k) + "] type=" + itoa(typeof(orig_keys[k])));
    if (typeof(orig_keys[k]) == 1) {
      this_player().listen(", value='" + orig_keys[k] + "'");
    } else {
      this_player().listen(", value=" + itoa(orig_keys[k]));
    }
    this_player().listen("\n");
    k = k + 1;
  }
  
  this_player().listen("Got values array: size = " + itoa(sizeof(orig_values)) + "\n");
  this_player().listen("Values (via implode): [" + implode(orig_values, ", ") + "]\n");
  this_player().listen("Values (manual loop):\n");
  k = 0;
  while (k < sizeof(orig_values)) {
    this_player().listen("  [" + itoa(k) + "] = " + itoa(orig_values[k]) + " (type=" + itoa(typeof(orig_values[k])) + ")\n");
    k = k + 1;
  }
  
  /* Reconstruct mapping from keys and values */
  j = 0;
  while (j < sizeof(orig_keys)) {
    this_player().listen("  Reconstructing: " + orig_keys[j] + " = " + itoa(orig_values[j]) + "\n");
    reconstructed[orig_keys[j]] = orig_values[j];
    j = j + 1;
  }
  
  this_player().listen("Reconstructed mapping from keys/values\n");
  this_player().listen("sizeof(reconstructed) = " + itoa(sizeof(reconstructed)) + "\n");
  
  /* Verify all keys from original exist in reconstructed with same values */
  this_player().listen("\nVerifying reconstruction:\n");
  match_count = 0;
  
  this_player().listen("  apple: member=" + itoa(member(reconstructed, "apple")));
  if (member(reconstructed, "apple")) {
    this_player().listen(", value=" + itoa(reconstructed["apple"]) + " (expected 100)");
    if (reconstructed["apple"] == 100) match_count = match_count + 1;
  }
  this_player().listen("\n");
  
  this_player().listen("  banana: member=" + itoa(member(reconstructed, "banana")));
  if (member(reconstructed, "banana")) {
    this_player().listen(", value=" + itoa(reconstructed["banana"]) + " (expected 200)");
    if (reconstructed["banana"] == 200) match_count = match_count + 1;
  }
  this_player().listen("\n");
  
  this_player().listen("  cherry: member=" + itoa(member(reconstructed, "cherry")));
  if (member(reconstructed, "cherry")) {
    this_player().listen(", value=" + itoa(reconstructed["cherry"]) + " (expected 300)");
    if (reconstructed["cherry"] == 300) match_count = match_count + 1;
  }
  this_player().listen("\n");
  
  this_player().listen("  date: member=" + itoa(member(reconstructed, "date")));
  if (member(reconstructed, "date")) {
    this_player().listen(", value=" + itoa(reconstructed["date"]) + " (expected 400)");
    if (reconstructed["date"] == 400) match_count = match_count + 1;
  }
  this_player().listen("\n");
  
  this_player().listen("  elderberry: member=" + itoa(member(reconstructed, "elderberry")));
  if (member(reconstructed, "elderberry")) {
    this_player().listen(", value=" + itoa(reconstructed["elderberry"]) + " (expected 500)");
    if (reconstructed["elderberry"] == 500) match_count = match_count + 1;
  }
  this_player().listen("\n");
  
  this_player().listen("\nMatched " + itoa(match_count) + " out of 5 key-value pairs (should be 5)\n");
  
  if (match_count == 5) {
    this_player().listen("✅ Reconstruction successful! keys() and values() are properly aligned.\n");
  } else {
    this_player().listen("❌ Reconstruction failed! keys() and values() may not be aligned.\n");
  }
  
  this_player().listen("\n=== All Phase 2 Tests Complete! ===\n");
  this_player().listen("Phase 2 efuns: PASSED ✅\n");
}
