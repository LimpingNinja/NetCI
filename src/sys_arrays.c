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
  char logbuf[256];
  
  sprintf(logbuf, "s_sizeof: called");
  logger(LOG_DEBUG, logbuf);
  
  /* Pop number of arguments */
  if (pop(&tmp, rts, obj)) {
    logger(LOG_ERROR, "s_sizeof: failed to pop NUM_ARGS");
    return 1;
  }
  
  if (tmp.type != NUM_ARGS) {
    logger(LOG_ERROR, "s_sizeof: not NUM_ARGS");
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num != 1) {
    logger(LOG_ERROR, "s_sizeof: wrong arg count");
    return 1;
  }
  
  /* Pop the array variable */
  if (pop(&tmp, rts, obj)) {
    logger(LOG_ERROR, "s_sizeof: failed to pop argument");
    return 1;
  }
  
  /* Resolve L_VALUE to get actual value */
  if (tmp.type == LOCAL_L_VALUE || tmp.type == GLOBAL_L_VALUE) {
    if (resolve_var(&tmp, obj)) {
      logger(LOG_ERROR, "s_sizeof: failed to resolve variable");
      clear_var(&tmp);
      return 1;
    }
  }
  
  /* Check if it's an array or uninitialized (INTEGER 0) */
  if (tmp.type == INTEGER && tmp.value.integer == 0) {
    /* Uninitialized array - return 0 */
    sprintf(logbuf, "s_sizeof: uninitialized array, returning 0");
    logger(LOG_DEBUG, logbuf);
    array_size = 0;
  } else if (tmp.type == ARRAY) {
    /* Get array size from heap array */
    arr = tmp.value.array_ptr;
    if (!arr) {
      logger(LOG_ERROR, "s_sizeof: NULL array pointer");
      clear_var(&tmp);
      return 1;
    }
    array_size = arr->size;
  } else {
    sprintf(logbuf, "s_sizeof: not an array (type=%d)", tmp.type);
    logger(LOG_ERROR, logbuf);
    clear_var(&tmp);
    return 1;
  }
  
  sprintf(logbuf, "s_sizeof: array_size=%u", array_size);
  logger(LOG_DEBUG, logbuf);
  
  /* Push result */
  result.type = INTEGER;
  result.value.integer = array_size;
  push(&result, rts);
  
  logger(LOG_DEBUG, "s_sizeof: success");
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
