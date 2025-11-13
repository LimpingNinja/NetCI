/* sys_mappings.c - Core mapping operations for NetCI LPC
 *
 * IMPORTANT: Mappings are NOT arrays!
 * 
 * Key Differences:
 * - Arrays: Contiguous memory (struct heap_array), integer indices only
 * - Mappings: Hash table (struct heap_mapping), arbitrary key types
 * 
 * Mappings support:
 * - String keys: "name", "age", etc.
 * - Integer keys: 0, 1, 42, etc.
 * - Object keys: object pointers
 * 
 * Implementation:
 * - Hash table with chaining for collision resolution
 * - Reference counting for automatic memory management
 * - Dynamic growth when load factor exceeds threshold
 * - Identified by is_mapping flag in var_tab, NOT by array field
 * 
 * Status: Phase 1 Complete (Nov 3, 2025)
 * - Basic subscript operations working
 * - Assignment and reading tested
 * - Multiple keys supported
 */

#include "config.h"
#include "object.h"
#include "constrct.h"
#include "instr.h"
#include "protos.h"
#include "globals.h"
#include "file.h"
#include <string.h>

/* Hash function for strings (djb2 algorithm) */
unsigned int hash_string(const char *str) {
  unsigned int hash = 5381;
  int c;
  
  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  
  return hash;
}

/* Hash function for integers */
unsigned int hash_integer(int value) {
  /* Simple integer hash - multiply by prime and mix bits */
  unsigned int hash = (unsigned int)value;
  hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
  hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
  hash = (hash >> 16) ^ hash;
  return hash;
}

/* Hash function for object pointers */
unsigned int hash_object(struct object *obj) {
  /* Hash the pointer address */
  unsigned long addr = (unsigned long)obj;
  unsigned int hash = (unsigned int)(addr ^ (addr >> 32));
  hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
  hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
  hash = (hash >> 16) ^ hash;
  return hash;
}

/* Hash function dispatcher based on var type */
unsigned int hash_var(struct var *key) {
  switch (key->type) {
    case STRING:
      return hash_string(key->value.string);
    case INTEGER:
      return hash_integer(key->value.integer);
    case OBJECT:
      return hash_object(key->value.objptr);
    default:
      /* For other types, hash the type itself */
      return hash_integer(key->type);
  }
}

/* Compare two vars for equality (for key matching) */
int var_equals(struct var *v1, struct var *v2) {
  if (v1->type != v2->type)
    return 0;
  
  switch (v1->type) {
    case INTEGER:
      return v1->value.integer == v2->value.integer;
    case STRING:
      return strcmp(v1->value.string, v2->value.string) == 0;
    case OBJECT:
      return v1->value.objptr == v2->value.objptr;
    default:
      /* For other types, only equal if same type */
      return 1;
  }
}

/* Copy a var (for storing keys/values) */
void copy_var_to(struct var *dest, struct var *src) {
  dest->type = src->type;
  
  switch (src->type) {
    case STRING:
      dest->value.string = copy_string(src->value.string);
      break;
    case ARRAY:
      dest->value.array_ptr = src->value.array_ptr;
      array_addref(src->value.array_ptr);
      break;
    case MAPPING:
      dest->value.mapping_ptr = src->value.mapping_ptr;
      mapping_addref(src->value.mapping_ptr);
      break;
    default:
      /* For simple types, just copy the value */
      dest->value = src->value;
      break;
  }
}

/* Allocate a new mapping with given initial capacity */
struct heap_mapping* allocate_mapping(unsigned int initial_capacity) {
  struct heap_mapping *map;
  unsigned int i;
  
  if (initial_capacity == 0)
    initial_capacity = DEFAULT_MAPPING_CAPACITY;
  
  map = (struct heap_mapping *) MALLOC(sizeof(struct heap_mapping));
  if (!map)
    return NULL;
  
  map->buckets = (struct mapping_entry **) MALLOC(initial_capacity * sizeof(struct mapping_entry *));
  if (!map->buckets) {
    FREE(map);
    return NULL;
  }
  
  /* Initialize all buckets to NULL */
  for (i = 0; i < initial_capacity; i++)
    map->buckets[i] = NULL;
  
  map->size = 0;
  map->capacity = initial_capacity;
  map->refcount = 1;
  
  return map;
}

/* Increment reference count */
void mapping_addref(struct heap_mapping *map) {
  if (map)
    map->refcount++;
}

/* Free a mapping entry (including key and value) */
void free_mapping_entry(struct mapping_entry *entry) {
  if (!entry)
    return;
  
  /* Clear key and value */
  clear_var(&entry->key);
  clear_var(&entry->value);
  
  FREE(entry);
}

/* Decrement reference count and free if zero */
void mapping_release(struct heap_mapping *map) {
  unsigned int i;
  struct mapping_entry *entry, *next;
  
  if (!map)
    return;
  
  map->refcount--;
  
  if (map->refcount == 0) {
    /* Free all entries */
    for (i = 0; i < map->capacity; i++) {
      entry = map->buckets[i];
      while (entry) {
        next = entry->next;
        free_mapping_entry(entry);
        entry = next;
      }
    }
    
    /* Free buckets array and mapping structure */
    FREE(map->buckets);
    FREE(map);
  }
}

/* Rehash the mapping to a larger capacity */
void mapping_rehash(struct heap_mapping *map) {
  unsigned int old_capacity, new_capacity, i, bucket;
  struct mapping_entry **old_buckets, *entry, *next;
  
  old_capacity = map->capacity;
  new_capacity = old_capacity * 2;
  old_buckets = map->buckets;
  
  /* Allocate new buckets */
  map->buckets = (struct mapping_entry **) MALLOC(new_capacity * sizeof(struct mapping_entry *));
  if (!map->buckets) {
    /* Rehash failed - keep old buckets */
    map->buckets = old_buckets;
    return;
  }
  
  /* Initialize new buckets */
  for (i = 0; i < new_capacity; i++)
    map->buckets[i] = NULL;
  
  map->capacity = new_capacity;
  
  /* Rehash all entries */
  for (i = 0; i < old_capacity; i++) {
    entry = old_buckets[i];
    while (entry) {
      next = entry->next;
      
      /* Reinsert into new buckets */
      bucket = entry->hash % new_capacity;
      entry->next = map->buckets[bucket];
      map->buckets[bucket] = entry;
      
      entry = next;
    }
  }
  
  /* Free old buckets array */
  FREE(old_buckets);
}

/* Get pointer to value for key, creating entry if needed
 * Returns pointer to value slot in mapping (for L_VALUE semantics)
 * Returns NULL on error
 */
struct var* mapping_get_or_create(struct heap_mapping *map, struct var *key) {
  unsigned int hash, bucket;
  struct mapping_entry *entry;
  
  if (!map || !key)
    return NULL;
  
  hash = hash_var(key);
  bucket = hash % map->capacity;
  
  /* Search for existing entry */
  entry = map->buckets[bucket];
  while (entry) {
    if (entry->hash == hash && var_equals(&entry->key, key)) {
      /* Found it! */
      return &entry->value;
    }
    entry = entry->next;
  }
  
  /* Not found - create new entry */
  entry = (struct mapping_entry *) MALLOC(sizeof(struct mapping_entry));
  if (!entry)
    return NULL;
  
  /* Initialize entry */
  copy_var_to(&entry->key, key);
  entry->hash = hash;
  entry->value.type = INTEGER;
  entry->value.value.integer = 0;  /* Default value */
  
  /* Add to chain */
  entry->next = map->buckets[bucket];
  map->buckets[bucket] = entry;
  map->size++;
  
  /* Check if rehash needed */
  if ((float)map->size / (float)map->capacity > MAPPING_LOAD_FACTOR) {
    mapping_rehash(map);
  }
  
  return &entry->value;
}

/* Get value for key (read-only access)
 * Returns 1 if key exists and sets result, 0 if key not found
 */
int mapping_get(struct heap_mapping *map, struct var *key, struct var *result) {
  unsigned int hash, bucket;
  struct mapping_entry *entry;
  
  if (!map || !key || !result)
    return 0;
  
  hash = hash_var(key);
  bucket = hash % map->capacity;
  
  /* Search for entry */
  entry = map->buckets[bucket];
  while (entry) {
    if (entry->hash == hash && var_equals(&entry->key, key)) {
      /* Found it - copy value to result */
      *result = entry->value;
      return 1;
    }
    entry = entry->next;
  }
  
  /* Not found */
  return 0;
}

/* Set value for key
 * Returns 0 on success, 1 on error
 */
int mapping_set(struct heap_mapping *map, struct var *key, struct var *value) {
  struct var *value_ptr;
  
  value_ptr = mapping_get_or_create(map, key);
  if (!value_ptr)
    return 1;
  
  /* Clear old value and set new one */
  clear_var(value_ptr);
  copy_var_to(value_ptr, value);
  
  return 0;
}

/* Delete a key from mapping
 * Returns 0 on success, 1 if key not found
 */
int mapping_delete(struct heap_mapping *map, struct var *key) {
  unsigned int hash, bucket;
  struct mapping_entry *entry, *prev;
  
  if (!map || !key)
    return 1;
  
  hash = hash_var(key);
  bucket = hash % map->capacity;
  
  /* Search for entry */
  prev = NULL;
  entry = map->buckets[bucket];
  while (entry) {
    if (entry->hash == hash && var_equals(&entry->key, key)) {
      /* Found it - remove from chain */
      if (prev)
        prev->next = entry->next;
      else
        map->buckets[bucket] = entry->next;
      
      free_mapping_entry(entry);
      map->size--;
      return 0;
    }
    prev = entry;
    entry = entry->next;
  }
  
  /* Not found */
  return 1;
}

/* Check if key exists in mapping
 * Returns 1 if exists, 0 if not
 */
int mapping_exists(struct heap_mapping *map, struct var *key) {
  unsigned int hash, bucket;
  struct mapping_entry *entry;
  
  if (!map || !key)
    return 0;
  
  hash = hash_var(key);
  bucket = hash % map->capacity;
  
  /* Search for entry */
  entry = map->buckets[bucket];
  while (entry) {
    if (entry->hash == hash && var_equals(&entry->key, key))
      return 1;
    entry = entry->next;
  }
  
  return 0;
}

/* Merge two mappings (m1 + m2)
 * Creates a new mapping with all keys from both
 * If key exists in both, m2's value wins
 * Returns new mapping or NULL on error
 */
struct heap_mapping* mapping_merge(struct heap_mapping *m1, struct heap_mapping *m2) {
  struct heap_mapping *result;
  struct mapping_entry *entry;
  unsigned int i;
  char logbuf[256];
  
  sprintf(logbuf, "mapping_merge: m1=%p (size=%u), m2=%p (size=%u)", 
          (void*)m1, m1 ? m1->size : 0, (void*)m2, m2 ? m2->size : 0);
  logger(LOG_INFO, logbuf);
  
  if (!m1 || !m2) {
    logger(LOG_ERROR, "mapping_merge: NULL mapping pointer");
    return NULL;
  }
  
  /* Allocate result mapping with capacity for both */
  result = allocate_mapping(m1->size + m2->size);
  if (!result) {
    logger(LOG_ERROR, "mapping_merge: failed to allocate result mapping");
    return NULL;
  }
  
  sprintf(logbuf, "mapping_merge: allocated result=%p, copying from m1", (void*)result);
  logger(LOG_INFO, logbuf);
  
  /* Copy all entries from m1 */
  for (i = 0; i < m1->capacity; i++) {
    entry = m1->buckets[i];
    while (entry) {
      if (mapping_set(result, &entry->key, &entry->value)) {
        logger(LOG_ERROR, "mapping_merge: failed to set entry from m1");
        mapping_release(result);
        return NULL;
      }
      entry = entry->next;
    }
  }
  
  logger(LOG_INFO, "mapping_merge: copied m1, now copying m2");
  
  /* Copy all entries from m2 (overwrites duplicates from m1) */
  for (i = 0; i < m2->capacity; i++) {
    entry = m2->buckets[i];
    while (entry) {
      if (mapping_set(result, &entry->key, &entry->value)) {
        logger(LOG_ERROR, "mapping_merge: failed to set entry from m2");
        mapping_release(result);
        return NULL;
      }
      entry = entry->next;
    }
  }
  
  sprintf(logbuf, "mapping_merge: success, result size=%u", result->size);
  logger(LOG_INFO, logbuf);
  
  return result;
}

/* Subtract mappings (m1 - m2)
 * Creates a new mapping with keys from m1 that are NOT in m2
 * Returns new mapping or NULL on error
 */
struct heap_mapping* mapping_subtract(struct heap_mapping *m1, struct heap_mapping *m2) {
  struct heap_mapping *result;
  struct mapping_entry *entry;
  unsigned int i;
  
  if (!m1 || !m2)
    return NULL;
  
  /* Allocate result mapping */
  result = allocate_mapping(m1->size);
  if (!result)
    return NULL;
  
  /* Copy entries from m1 that don't exist in m2 */
  for (i = 0; i < m1->capacity; i++) {
    entry = m1->buckets[i];
    while (entry) {
      /* Only add if key doesn't exist in m2 */
      if (!mapping_exists(m2, &entry->key)) {
        if (mapping_set(result, &entry->key, &entry->value)) {
          mapping_release(result);
          return NULL;
        }
      }
      entry = entry->next;
    }
  }
  
  return result;
}

/* mapping_keys_array() - Extract all keys from mapping into an array
 * For use by C code that needs to iterate over mapping keys
 * Returns a new array with all keys (caller must release with array_release)
 */
struct heap_array* mapping_keys_array(struct heap_mapping *map) {
  struct heap_array *keys_array;
  struct mapping_entry *entry;
  unsigned int i, key_count;
  
  if (!map)
    return NULL;
  
  /* Allocate array for keys */
  keys_array = allocate_array(map->size, map->size);
  if (!keys_array)
    return NULL;
  
  /* Collect all keys from hash table */
  key_count = 0;
  for (i = 0; i < map->capacity; i++) {
    entry = map->buckets[i];
    while (entry) {
      if (key_count < map->size) {
        /* Copy key to array */
        copy_var(&keys_array->elements[key_count], &entry->key);
        key_count++;
      }
      entry = entry->next;
    }
  }
  
  return keys_array;
}

/* mapping_values_array() - Extract all values from mapping into an array
 * For use by C code that needs to iterate over mapping values
 * Returns a new array with all values (caller must release with array_release)
 */
struct heap_array* mapping_values_array(struct heap_mapping *map) {
  struct heap_array *values_array;
  struct mapping_entry *entry;
  unsigned int i, value_count;
  
  if (!map)
    return NULL;
  
  /* Allocate array for values */
  values_array = allocate_array(map->size, map->size);
  if (!values_array)
    return NULL;
  
  /* Collect all values from hash table */
  value_count = 0;
  for (i = 0; i < map->capacity; i++) {
    entry = map->buckets[i];
    while (entry) {
      if (value_count < map->size) {
        /* Copy value to array */
        copy_var(&values_array->elements[value_count], &entry->value);
        value_count++;
      }
      entry = entry->next;
    }
  }
  
  return values_array;
}
