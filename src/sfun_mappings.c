/* sfun_mappings.c - Mapping system functions (efuns) exposed to softcode
 *
 * Phase 2: Essential mapping efuns for querying and manipulating mappings
 * - keys(mapping) - Get all keys as array
 * - values(mapping) - Get all values as array
 * - map_delete(mapping, key) - Delete a key
 * - member(mapping, key) - Check if key exists
 */

#include "config.h"
#include "object.h"
#include "protos.h"
#include "instr.h"
#include "constrct.h"
#include "file.h"

/* External reference to locals from interp.c */
extern struct var *locals;
extern unsigned int num_locals;

/* ============================================================================
 * MAPPING EFUNS (LPC-standard functions callable from softcode)
 * ============================================================================ */

/* keys() - Return array of all keys in mapping
 * Usage: mixed *keys(mapping m)
 * Returns: Array containing all keys
 */
int s_keys(struct object *caller, struct object *obj, struct object *player,
           struct var_stack **rts) {
  struct var tmp, map_var, result;
  struct heap_mapping *map;
  struct heap_array *keys_array;
  unsigned int i, key_count;
  
  /* Pop NUM_ARGS */
  if (pop(&tmp, rts, obj)) return 1;
  if (tmp.type != NUM_ARGS || tmp.value.num != 1) {
    clear_var(&tmp);
    return 1;
  }
  
  /* Pop mapping */
  if (pop(&map_var, rts, obj)) return 1;
  if (map_var.type != MAPPING) {
    clear_var(&map_var);
    return 1;
  }
  
  map = map_var.value.mapping_ptr;
  
  /* Allocate array for keys */
  keys_array = allocate_array(map->size, map->size);
  if (!keys_array) {
    clear_var(&map_var);
    return 1;
  }
  
  /* Collect all keys from hash table */
  key_count = 0;
  for (i = 0; i < map->capacity; i++) {
    struct mapping_entry *entry = map->buckets[i];
    while (entry) {
      if (key_count < map->size) {
        /* Copy key to array */
        copy_var(&keys_array->elements[key_count], &entry->key);
        key_count++;
      }
      entry = entry->next;
    }
  }
  
  /* Push result array */
  result.type = ARRAY;
  result.value.array_ptr = keys_array;
  push(&result, rts);
  
  clear_var(&map_var);
  return 0;
}

/* values() - Return array of all values in mapping
 * Usage: mixed *values(mapping m)
 * Returns: Array containing all values
 */
int s_values(struct object *caller, struct object *obj, struct object *player,
             struct var_stack **rts) {
  struct var tmp, map_var, result;
  struct heap_mapping *map;
  struct heap_array *values_array;
  unsigned int i, value_count;
  
  /* Pop NUM_ARGS */
  if (pop(&tmp, rts, obj)) return 1;
  if (tmp.type != NUM_ARGS || tmp.value.num != 1) {
    clear_var(&tmp);
    return 1;
  }
  
  /* Pop mapping */
  if (pop(&map_var, rts, obj)) return 1;
  if (map_var.type != MAPPING) {
    clear_var(&map_var);
    return 1;
  }
  
  map = map_var.value.mapping_ptr;
  
  /* Allocate array for values */
  values_array = allocate_array(map->size, map->size);
  if (!values_array) {
    clear_var(&map_var);
    return 1;
  }
  
  /* Collect all values from hash table */
  value_count = 0;
  for (i = 0; i < map->capacity; i++) {
    struct mapping_entry *entry = map->buckets[i];
    while (entry) {
      if (value_count < map->size) {
        /* Copy value to array */
        copy_var(&values_array->elements[value_count], &entry->value);
        value_count++;
      }
      entry = entry->next;
    }
  }
  
  /* Push result array */
  result.type = ARRAY;
  result.value.array_ptr = values_array;
  push(&result, rts);
  
  clear_var(&map_var);
  return 0;
}

/* map_delete() - Delete a key from mapping
 * Usage: void map_delete(mapping m, mixed key)
 * Returns: 0 (void)
 * Note: Modifies mapping in-place
 */
int s_map_delete(struct object *caller, struct object *obj, struct object *player,
                 struct var_stack **rts) {
  struct var tmp, map_var, key_var, result;
  struct heap_mapping *map;
  
  /* Pop NUM_ARGS */
  if (pop(&tmp, rts, obj)) return 1;
  if (tmp.type != NUM_ARGS || tmp.value.num != 2) {
    clear_var(&tmp);
    return 1;
  }
  
  /* Pop key */
  if (pop(&key_var, rts, obj)) return 1;
  
  /* Pop mapping */
  if (pop(&map_var, rts, obj)) {
    clear_var(&key_var);
    return 1;
  }
  if (map_var.type != MAPPING) {
    clear_var(&map_var);
    clear_var(&key_var);
    return 1;
  }
  
  map = map_var.value.mapping_ptr;
  
  /* Delete the key */
  mapping_delete(map, &key_var);
  
  /* Push 0 as return value (void function) */
  result.type = INTEGER;
  result.value.integer = 0;
  push(&result, rts);
  
  clear_var(&map_var);
  clear_var(&key_var);
  return 0;
}

/* member() - Check if key exists in mapping
 * Usage: int member(mapping m, mixed key)
 * Returns: 1 if key exists, 0 otherwise
 */
int s_member(struct object *caller, struct object *obj, struct object *player,
             struct var_stack **rts) {
  struct var tmp, map_var, key_var, result;
  struct heap_mapping *map;
  int exists;
  
  /* Pop NUM_ARGS */
  if (pop(&tmp, rts, obj)) return 1;
  if (tmp.type != NUM_ARGS || tmp.value.num != 2) {
    clear_var(&tmp);
    return 1;
  }
  
  /* Pop key */
  if (pop(&key_var, rts, obj)) return 1;
  
  /* Pop mapping */
  if (pop(&map_var, rts, obj)) {
    clear_var(&key_var);
    return 1;
  }
  if (map_var.type != MAPPING) {
    clear_var(&map_var);
    clear_var(&key_var);
    return 1;
  }
  
  map = map_var.value.mapping_ptr;
  
  /* Check if key exists */
  exists = mapping_exists(map, &key_var);
  
  /* Push result */
  result.type = INTEGER;
  result.value.integer = exists;
  push(&result, rts);
  
  clear_var(&map_var);
  clear_var(&key_var);
  return 0;
}

/* ============================================================================
 * MAPPING LITERALS (Phase 4)
 * ============================================================================ */

/* s_mapping_literal() - Create mapping from literal syntax
 * Stack layout: [key_n] [value_n] ... [key_1] [value_1] [pair_count]
 * Usage: ([ key1: value1, key2: value2, ... ])
 * Creates a new heap mapping and populates it with the key-value pairs on the stack
 */
int s_mapping_literal(struct object *caller, struct object *obj, struct object *player,
                      struct var_stack **rts) {
  struct var count_var, result, key_var, value_var;
  struct heap_mapping *map;
  unsigned int pair_count, i;
  
  /* Pop the pair count */
  if (pop(&count_var, rts, obj))
    return 1;
  if (count_var.type != INTEGER) {
    clear_var(&count_var);
    return 1;
  }
  
  pair_count = count_var.value.integer;
  
  /* Allocate the mapping */
  map = allocate_mapping(pair_count > 0 ? pair_count : DEFAULT_MAPPING_CAPACITY);
  if (!map) {
    /* Pop and discard all key-value pairs */
    for (i = 0; i < pair_count * 2; i++) {
      pop(&key_var, rts, obj);
      clear_var(&key_var);
    }
    return 1;
  }
  
  /* Pop and insert key-value pairs (in reverse order) */
  for (i = 0; i < pair_count; i++) {
    /* Pop value first (pushed last) */
    if (pop(&value_var, rts, obj)) {
      mapping_release(map);
      return 1;
    }
    
    /* Pop key */
    if (pop(&key_var, rts, obj)) {
      clear_var(&value_var);
      mapping_release(map);
      return 1;
    }
    
    /* Insert into mapping */
    if (mapping_set(map, &key_var, &value_var)) {
      clear_var(&key_var);
      clear_var(&value_var);
      mapping_release(map);
      return 1;
    }
    
    /* Clean up - mapping_set makes copies */
    clear_var(&key_var);
    clear_var(&value_var);
  }
  
  /* Push the mapping onto the stack */
  result.type = MAPPING;
  result.value.mapping_ptr = map;
  push(&result, rts);
  
  return 0;
}
