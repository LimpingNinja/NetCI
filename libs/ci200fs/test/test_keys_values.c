/* test_keys_values.c - Simple test for keys() and values() reconstruction */
#include <sys.h>

create() {
  mapping m, m2;
  int *k, *v;
  int i;
  
  this_player().listen("=== Simple Keys/Values Test ===\n\n");
  
  /* Create a simple mapping with 3 elements */
  m["a"] = 1;
  m["b"] = 2;
  m["c"] = 3;
  
  this_player().listen("Original mapping:\n");
  this_player().listen("  m[\"a\"] = " + itoa(m["a"]) + "\n");
  this_player().listen("  m[\"b\"] = " + itoa(m["b"]) + "\n");
  this_player().listen("  m[\"c\"] = " + itoa(m["c"]) + "\n");
  this_player().listen("  sizeof(m) = " + itoa(sizeof(m)) + "\n\n");
  
  /* Get keys and values */
  k = keys(m);
  v = values(m);
  
  this_player().listen("Keys array: sizeof = " + itoa(sizeof(k)) + "\n");
  i = 0;
  while (i < sizeof(k)) {
    this_player().listen("  k[" + itoa(i) + "] = '" + k[i] + "'\n");
    i = i + 1;
  }
  
  this_player().listen("\nValues array: sizeof = " + itoa(sizeof(v)) + "\n");
  i = 0;
  while (i < sizeof(v)) {
    this_player().listen("  v[" + itoa(i) + "] = " + itoa(v[i]) + "\n");
    i = i + 1;
  }
  
  /* Try to reconstruct */
  this_player().listen("\nReconstructing m2 from keys and values:\n");
  i = 0;
  while (i < sizeof(k)) {
    this_player().listen("  Setting m2['" + k[i] + "'] = " + itoa(v[i]) + "\n");
    m2[k[i]] = v[i];
    i = i + 1;
  }
  
  this_player().listen("\nReconstructed mapping:\n");
  this_player().listen("  sizeof(m2) = " + itoa(sizeof(m2)) + "\n");
  this_player().listen("  member(m2, \"a\") = " + itoa(member(m2, "a")) + "\n");
  this_player().listen("  member(m2, \"b\") = " + itoa(member(m2, "b")) + "\n");
  this_player().listen("  member(m2, \"c\") = " + itoa(member(m2, "c")) + "\n");
  
  if (member(m2, "a")) {
    this_player().listen("  m2[\"a\"] = " + itoa(m2["a"]) + "\n");
  }
  if (member(m2, "b")) {
    this_player().listen("  m2[\"b\"] = " + itoa(m2["b"]) + "\n");
  }
  if (member(m2, "c")) {
    this_player().listen("  m2[\"c\"] = " + itoa(m2["c"]) + "\n");
  }
  
  this_player().listen("\n=== Test Complete ===\n");
}
