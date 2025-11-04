/* sys_arrays.c - Array system functions */

#include "config.h"
#include "object.h"
#include "protos.h"
#include "instr.h"
#include "constrct.h"
#include "file.h"

/* ========================================================================
 * HEAP ARRAY FUNCTIONS (Phase 2.5)
 * ======================================================================== */

/* Create a new heap-allocated array
 * size: Initial number of elements
 * max_size: Maximum allowed size (or UNLIMITED_ARRAY_SIZE)
 * Returns: Pointer to new array, or NULL on failure
 */
struct heap_array* allocate_array(unsigned int size, unsigned int max_size) {
  struct heap_array *arr;
  unsigned int i;
  char logbuf[256];
  
  sprintf(logbuf, "allocate_array: size=%u, max_size=%u", size, max_size);
  logger(LOG_DEBUG, logbuf);
  
  /* Allocate array structure */
  arr = (struct heap_array *) MALLOC(sizeof(struct heap_array));
  if (!arr) {
    logger(LOG_ERROR, "allocate_array: failed to allocate array structure");
    return NULL;
  }
  
  /* Initialize fields */
  arr->size = size;
  arr->capacity = size;  /* Initial capacity equals initial size */
  arr->max_size = max_size;
  arr->refcount = 1;  /* Start with refcount 1 */
  
  /* Allocate elements */
  if (size > 0) {
    arr->elements = (struct var *) MALLOC(sizeof(struct var) * size);
    if (!arr->elements) {
      logger(LOG_ERROR, "allocate_array: failed to allocate elements");
      FREE(arr);
      return NULL;
    }
    
    /* Initialize all elements to INTEGER 0 */
    for (i = 0; i < size; i++) {
      arr->elements[i].type = INTEGER;
      arr->elements[i].value.integer = 0;
    }
  } else {
    arr->elements = NULL;
  }
  
  sprintf(logbuf, "allocate_array: created array %p with %u elements", 
          (void*)arr, size);
  logger(LOG_DEBUG, logbuf);
  
  return arr;
}

/* Increment array reference count */
void array_addref(struct heap_array *arr) {
  char logbuf[256];
  
  if (!arr) return;
  
  arr->refcount++;
  sprintf(logbuf, "array_addref: array %p refcount now %u", 
          (void*)arr, arr->refcount);
  logger(LOG_DEBUG, logbuf);
}

/* Decrement array reference count and free if zero */
void array_release(struct heap_array *arr) {
  unsigned int i;
  char logbuf[256];
  
  if (!arr) return;
  
  sprintf(logbuf, "array_release: array %p refcount %u", 
          (void*)arr, arr->refcount);
  logger(LOG_DEBUG, logbuf);
  
  if (arr->refcount == 0) {
    logger(LOG_ERROR, "array_release: refcount already zero!");
    return;
  }
  
  arr->refcount--;
  
  if (arr->refcount == 0) {
    sprintf(logbuf, "array_release: freeing array %p with %u elements", 
            (void*)arr, arr->size);
    logger(LOG_DEBUG, logbuf);
    
    /* Free all elements */
    if (arr->elements) {
      for (i = 0; i < arr->size; i++) {
        clear_var(&arr->elements[i]);
      }
      FREE(arr->elements);
    }
    
    /* Free array structure */
    FREE(arr);
  }
}

/* Resize heap array to new size
 * Uses geometric growth (1.5x) for efficiency
 * Returns: 0 on success, 1 on failure
 */
int resize_heap_array(struct heap_array *arr, unsigned int new_size) {
  struct var *new_elements;
  unsigned int i, new_capacity;
  char logbuf[256];
  
  if (!arr) return 1;
  
  sprintf(logbuf, "resize_heap_array: array %p from size=%u to size=%u (capacity=%u)", 
          (void*)arr, arr->size, new_size, arr->capacity);
  logger(LOG_DEBUG, logbuf);
  
  /* Check against max_size */
  if (arr->max_size != UNLIMITED_ARRAY_SIZE && new_size > arr->max_size) {
    logger(LOG_ERROR, "resize_heap_array: exceeds max_size");
    return 1;
  }
  
  /* If new size fits in current capacity, just update size */
  if (new_size <= arr->capacity) {
    /* Initialize any new elements to INTEGER 0 */
    for (i = arr->size; i < new_size; i++) {
      arr->elements[i].type = INTEGER;
      arr->elements[i].value.integer = 0;
    }
    arr->size = new_size;
    sprintf(logbuf, "resize_heap_array: fits in capacity, new size=%u", new_size);
    logger(LOG_DEBUG, logbuf);
    return 0;
  }
  
  /* Need to grow capacity - use 1.5x growth strategy */
  new_capacity = arr->capacity + (arr->capacity / 2);  /* 1.5x */
  if (new_capacity < new_size) {
    new_capacity = new_size;  /* Ensure we have enough */
  }
  
  /* Cap at max_size if not unlimited */
  if (arr->max_size != UNLIMITED_ARRAY_SIZE && new_capacity > arr->max_size) {
    new_capacity = arr->max_size;
  }
  
  sprintf(logbuf, "resize_heap_array: growing capacity from %u to %u", 
          arr->capacity, new_capacity);
  logger(LOG_DEBUG, logbuf);
  
  /* Allocate new elements array */
  new_elements = (struct var *) MALLOC(sizeof(struct var) * new_capacity);
  if (!new_elements) {
    logger(LOG_ERROR, "resize_heap_array: allocation failed");
    return 1;
  }
  
  /* Copy existing elements */
  for (i = 0; i < arr->size; i++) {
    new_elements[i] = arr->elements[i];
  }
  
  /* Initialize new elements to INTEGER 0 */
  for (i = arr->size; i < new_capacity; i++) {
    new_elements[i].type = INTEGER;
    new_elements[i].value.integer = 0;
  }
  
  /* Free old elements and update */
  if (arr->elements) {
    FREE(arr->elements);
  }
  arr->elements = new_elements;
  arr->capacity = new_capacity;
  arr->size = new_size;
  
  sprintf(logbuf, "resize_heap_array: success, size=%u, capacity=%u", 
          arr->size, arr->capacity);
  logger(LOG_DEBUG, logbuf);
  
  return 0;
}


/* s_sizeof() - Return the size of an array
 * Usage: int size = sizeof(array_var);
 * Note: Works with heap arrays (Phase 2.5)
 */
int s_sizeof(struct object *caller, struct object *obj, struct object *player,
             struct var_stack **rts) {
  struct var tmp, result;
  unsigned int array_size;
  struct heap_array *arr;
  
  /* Pop number of arguments */
  if (pop(&tmp, rts, obj)) return 1;
  
  if (tmp.type != NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num != 1) return 1;
  
  /* Pop the array/mapping variable */
  if (pop(&tmp, rts, obj)) return 1;
  
  /* Resolve L_VALUE to get actual value */
  if (tmp.type == LOCAL_L_VALUE || tmp.type == GLOBAL_L_VALUE) {
    if (resolve_var(&tmp, obj)) {
      clear_var(&tmp);
      return 1;
    }
  }
  
  /* Check if it's an array, mapping, or uninitialized (INTEGER 0) */
  if (tmp.type == INTEGER && tmp.value.integer == 0) {
    /* Uninitialized array/mapping - return 0 */
    array_size = 0;
  } else if (tmp.type == ARRAY) {
    /* Get array size from heap array */
    arr = tmp.value.array_ptr;
    if (!arr) {
      clear_var(&tmp);
      return 1;
    }
    array_size = arr->size;
  } else if (tmp.type == MAPPING) {
    /* Get mapping size (number of key-value pairs) */
    struct heap_mapping *map = tmp.value.mapping_ptr;
    if (!map) {
      clear_var(&tmp);
      return 1;
    }
    array_size = map->size;
  } else {
    /* Not an array or mapping */
    clear_var(&tmp);
    return 1;
  }
  
  /* Push result */
  result.type = INTEGER;
  result.value.integer = array_size;
  push(&result, rts);
  
  clear_var(&tmp);
  return 0;
}

/* s_array_literal() - Create array from literal syntax
 * Stack layout: [elem_n] ... [elem_1] [elem_0] [count]
 * Usage: ({ elem1, elem2, elem3 })
 * Creates a new heap array and populates it with the elements on the stack
 */
int s_array_literal(struct object *caller, struct object *obj, struct object *player,
                    struct var_stack **rts) {
  struct var count_var, result;
  struct heap_array *arr;
  unsigned int elem_count, i;
  struct var_stack *elem_stack;
  char logbuf[256];
  
  logger(LOG_DEBUG, "s_array_literal: starting");
  
  /* Pop the element count */
  pop(&count_var, rts, obj);
  if (count_var.type != INTEGER) {
    logger(LOG_ERROR, "s_array_literal: count is not an integer");
    clear_var(&count_var);
    return 1;
  }
  
  elem_count = count_var.value.integer;
  sprintf(logbuf, "s_array_literal: creating array with %u elements", elem_count);
  logger(LOG_DEBUG, logbuf);
  
  /* Allocate the array */
  arr = allocate_array(elem_count, UNLIMITED_ARRAY_SIZE);
  if (!arr) {
    logger(LOG_ERROR, "s_array_literal: failed to allocate array");
    return 1;
  }
  
  /* Pop elements in reverse order and store them */
  for (i = 0; i < elem_count; i++) {
    pop(&arr->elements[elem_count - 1 - i], rts, obj);
  }
  
  sprintf(logbuf, "s_array_literal: populated %u elements", elem_count);
  logger(LOG_DEBUG, logbuf);
  
  /* Push the array onto the stack */
  result.type = ARRAY;
  result.value.array_ptr = arr;
  push(&result, rts);
  
  logger(LOG_DEBUG, "s_array_literal: success");
  return 0;
}

/* array_concat() - Concatenate two arrays
 * Returns a new heap array containing all elements from arr1 followed by arr2
 * Caller is responsible for managing refcounts
 */
struct heap_array* array_concat(struct heap_array *arr1, struct heap_array *arr2) {
  struct heap_array *result;
  unsigned int i;
  char logbuf[256];
  
  sprintf(logbuf, "array_concat: arr1 size=%u, arr2 size=%u", arr1->size, arr2->size);
  logger(LOG_DEBUG, logbuf);
  
  /* Allocate new array with combined size */
  result = allocate_array(arr1->size + arr2->size, UNLIMITED_ARRAY_SIZE);
  if (!result) {
    logger(LOG_ERROR, "array_concat: failed to allocate result array");
    return NULL;
  }
  
  /* Copy elements from arr1 */
  for (i = 0; i < arr1->size; i++) {
    result->elements[i] = arr1->elements[i];
    /* Increment refcount for arrays and strings */
    if (result->elements[i].type == ARRAY && result->elements[i].value.array_ptr) {
      array_addref(result->elements[i].value.array_ptr);
    } else if (result->elements[i].type == STRING && result->elements[i].value.string) {
      result->elements[i].value.string = copy_string(result->elements[i].value.string);
    }
  }
  
  /* Copy elements from arr2 */
  for (i = 0; i < arr2->size; i++) {
    result->elements[arr1->size + i] = arr2->elements[i];
    /* Increment refcount for arrays and strings */
    if (result->elements[arr1->size + i].type == ARRAY && result->elements[arr1->size + i].value.array_ptr) {
      array_addref(result->elements[arr1->size + i].value.array_ptr);
    } else if (result->elements[arr1->size + i].type == STRING && result->elements[arr1->size + i].value.string) {
      result->elements[arr1->size + i].value.string = copy_string(result->elements[arr1->size + i].value.string);
    }
  }
  
  sprintf(logbuf, "array_concat: created array with %u elements", result->size);
  logger(LOG_DEBUG, logbuf);
  
  return result;
}

/* array_subtract() - Remove elements from arr1 that are in arr2
 * Returns a new heap array containing elements from arr1 not found in arr2
 * Removes ALL occurrences of matching elements
 */
struct heap_array* array_subtract(struct heap_array *arr1, struct heap_array *arr2) {
  struct heap_array *result;
  unsigned int i, j, result_count, found;
  struct var *temp_elements;
  char logbuf[256];
  
  sprintf(logbuf, "array_subtract: arr1 size=%u, arr2 size=%u", arr1->size, arr2->size);
  logger(LOG_DEBUG, logbuf);
  
  /* Allocate temporary array to hold result elements */
  temp_elements = (struct var *) MALLOC(sizeof(struct var) * arr1->size);
  if (!temp_elements) {
    logger(LOG_ERROR, "array_subtract: failed to allocate temp array");
    return NULL;
  }
  
  result_count = 0;
  
  /* For each element in arr1, check if it's in arr2 */
  for (i = 0; i < arr1->size; i++) {
    found = 0;
    
    /* Check if arr1[i] is in arr2 */
    for (j = 0; j < arr2->size; j++) {
      /* Simple equality check - integers and strings */
      if (arr1->elements[i].type == arr2->elements[j].type) {
        if (arr1->elements[i].type == INTEGER) {
          if (arr1->elements[i].value.integer == arr2->elements[j].value.integer) {
            found = 1;
            break;
          }
        } else if (arr1->elements[i].type == STRING) {
          if (arr1->elements[i].value.string && arr2->elements[j].value.string &&
              !strcmp(arr1->elements[i].value.string, arr2->elements[j].value.string)) {
            found = 1;
            break;
          }
        }
        /* For other types (objects, arrays), use pointer equality */
        else if (arr1->elements[i].value.objptr == arr2->elements[j].value.objptr) {
          found = 1;
          break;
        }
      }
    }
    
    /* If not found in arr2, add to result */
    if (!found) {
      temp_elements[result_count] = arr1->elements[i];
      /* Increment refcount for arrays and strings */
      if (temp_elements[result_count].type == ARRAY && temp_elements[result_count].value.array_ptr) {
        array_addref(temp_elements[result_count].value.array_ptr);
      } else if (temp_elements[result_count].type == STRING && temp_elements[result_count].value.string) {
        temp_elements[result_count].value.string = copy_string(temp_elements[result_count].value.string);
      }
      result_count++;
    }
  }
  
  /* Create result array with exact size needed */
  result = allocate_array(result_count, UNLIMITED_ARRAY_SIZE);
  if (!result) {
    logger(LOG_ERROR, "array_subtract: failed to allocate result array");
    FREE(temp_elements);
    return NULL;
  }
  
  /* Copy temp elements to result */
  for (i = 0; i < result_count; i++) {
    result->elements[i] = temp_elements[i];
  }
  
  FREE(temp_elements);
  
  sprintf(logbuf, "array_subtract: created array with %u elements (removed %u)", 
          result_count, arr1->size - result_count);
  logger(LOG_DEBUG, logbuf);
  
  return result;
}
