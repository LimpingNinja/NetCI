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

/* ========================================================================
 * OLD METADATA FUNCTIONS (Phase 2 - TO BE REMOVED)
 * ======================================================================== */

/* Helper function to calculate array size from var_tab
 * Based on GetVarSize() from winmain.c
 */
static unsigned int calc_array_size(unsigned int base_index, struct var_tab *gst) {
  struct var_tab *curr_var;
  struct array_size *curr_array;
  unsigned int size;
  
  curr_var = gst;
  while (curr_var) {
    size = 1;
    curr_array = curr_var->array;
    
    /* Multiply all dimensions */
    while (curr_array) {
      size *= curr_array->size;
      curr_array = curr_array->next;
    }
    
    /* Check if this variable contains our base index */
    if ((base_index >= curr_var->base) && (base_index < (curr_var->base + size))) {
      return size;
    }
    curr_var = curr_var->next;
  }
  
  /* Not found or scalar variable */
  return 1;
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

/* ========================================================================
 * Array Metadata Management Functions
 * ======================================================================== */

/* Find array metadata for a given base index */
struct array_metadata* find_array_metadata(struct object *obj, unsigned int base, 
                                           unsigned char is_global) {
  struct array_metadata *meta = obj->array_meta;
  
  while (meta) {
    if (meta->base == base && meta->is_global == is_global) {
      return meta;
    }
    meta = meta->next;
  }
  return NULL;
}

/* Register new array metadata when array is created */
struct array_metadata* register_array_metadata(struct object *obj, unsigned int base,
                                               unsigned int initial_size, 
                                               unsigned int max_size,
                                               unsigned char is_global) {
  struct array_metadata *meta;
  char logbuf[256];
  
  /* Check if metadata already exists */
  meta = find_array_metadata(obj, base, is_global);
  if (meta) {
    /* Update existing metadata */
    meta->current_size = initial_size;
    meta->max_size = max_size;
    meta->ref_count = 1;
    sprintf(logbuf, "register_array_metadata: updated base=%u, size=%u, max=%u, global=%d",
            base, initial_size, max_size, is_global);
    logger(LOG_DEBUG, logbuf);
    return meta;
  }
  
  /* Create new metadata */
  meta = (struct array_metadata *) MALLOC(sizeof(struct array_metadata));
  meta->base = base;
  meta->current_size = initial_size;
  meta->max_size = max_size;
  meta->ref_count = 1;
  meta->is_global = is_global;
  meta->next = obj->array_meta;
  obj->array_meta = meta;
  
  sprintf(logbuf, "register_array_metadata: created base=%u, size=%u, max=%u, global=%d",
          base, initial_size, max_size, is_global);
  logger(LOG_DEBUG, logbuf);
  
  return meta;
}

/* Increment refcount for array metadata */
void metadata_addref(struct array_metadata *meta) {
  if (meta) {
    meta->ref_count++;
  }
}

/* Decrement array reference count and free if zero */
void metadata_release(struct object *obj, struct array_metadata *meta) {
  struct array_metadata *curr, *prev;
  
  if (!meta) return;
  
  meta->ref_count--;
  if (meta->ref_count == 0) {
    /* Remove from linked list */
    prev = NULL;
    curr = obj->array_meta;
    while (curr) {
      if (curr == meta) {
        if (prev) {
          prev->next = curr->next;
        } else {
          obj->array_meta = curr->next;
        }
        FREE(meta);
        return;
      }
      prev = curr;
      curr = curr->next;
    }
  }
}

/* Free all array metadata for an object */
void free_all_array_metadata(struct object *obj) {
  struct array_metadata *curr, *next;
  
  curr = obj->array_meta;
  while (curr) {
    next = curr->next;
    FREE(curr);
    curr = next;
  }
  obj->array_meta = NULL;
}

/* Resize array to accommodate new size
 * For locals: locals_ptr must point to the locals array pointer (so we can realloc)
 * For globals: locals_ptr can be NULL
 * Returns 0 on success, 1 on failure (bounds exceeded)
 */
int resize_array(struct object *obj, unsigned int base, unsigned int new_size,
                 unsigned char is_global, struct var **locals_ptr, unsigned int *num_locals_ptr) {
  struct array_metadata *meta;
  struct var *new_array;
  unsigned int old_size, i;
  char logbuf[256];
  
  /* Find metadata */
  meta = find_array_metadata(obj, base, is_global);
  if (!meta) {
    sprintf(logbuf, "resize_array: no metadata for base=%u, global=%d", base, is_global);
    logger(LOG_ERROR, logbuf);
    return 1;
  }
  
  /* Check if resize is allowed */
  if (meta->max_size != UNLIMITED_ARRAY_SIZE && new_size > meta->max_size) {
    sprintf(logbuf, "resize_array: size %u exceeds max %u", new_size, meta->max_size);
    logger(LOG_ERROR, logbuf);
    return 1;  /* Bounds exceeded */
  }
  
  /* Already big enough? */
  if (new_size <= meta->current_size) {
    return 0;
  }
  
  sprintf(logbuf, "resize_array: growing from %u to %u (base=%u, global=%d)",
          meta->current_size, new_size, base, is_global);
  logger(LOG_DEBUG, logbuf);
  
  old_size = meta->current_size;
  
  if (is_global) {
    /* Resize globals array in place */
    /* Allocate new array */
    new_array = (struct var *) MALLOC(sizeof(struct var) * new_size);
    
    /* Copy old data */
    for (i = 0; i < old_size; i++) {
      new_array[i] = obj->globals[base + i];
    }
    
    /* Initialize new elements */
    for (i = old_size; i < new_size; i++) {
      new_array[i].type = INTEGER;
      new_array[i].value.integer = 0;
    }
    
    /* Free old elements */
    for (i = 0; i < old_size; i++) {
      clear_var(&obj->globals[base + i]);
    }
    
    /* Copy back */
    for (i = 0; i < new_size; i++) {
      obj->globals[base + i] = new_array[i];
    }
    
    FREE(new_array);
  } else {
    /* Resize locals array - need to reallocate entire locals */
    unsigned int old_num_locals, new_num_locals;
    
    if (!locals_ptr || !*locals_ptr || !num_locals_ptr) {
      logger(LOG_ERROR, "resize_array: locals_ptr or num_locals_ptr is NULL");
      return 1;
    }
    
    old_num_locals = *num_locals_ptr;
    
    /* Calculate new locals size if array extends beyond current locals */
    new_num_locals = old_num_locals;
    if (base + new_size > old_num_locals) {
      new_num_locals = base + new_size;
      sprintf(logbuf, "resize_array: expanding locals from %u to %u", old_num_locals, new_num_locals);
      logger(LOG_DEBUG, logbuf);
    }
    
    /* Allocate new locals array */
    new_array = (struct var *) MALLOC(sizeof(struct var) * new_num_locals);
    
    /* Copy existing locals */
    for (i = 0; i < old_num_locals; i++) {
      new_array[i] = (*locals_ptr)[i];
    }
    
    /* Initialize any new locals beyond old num_locals */
    for (i = old_num_locals; i < new_num_locals; i++) {
      new_array[i].type = INTEGER;
      new_array[i].value.integer = 0;
    }
    
    /* Initialize new array elements specifically */
    for (i = old_size; i < new_size; i++) {
      new_array[base + i].type = INTEGER;
      new_array[base + i].value.integer = 0;
    }
    
    /* Replace locals pointer and update count */
    FREE(*locals_ptr);
    *locals_ptr = new_array;
    *num_locals_ptr = new_num_locals;
  }
  
  /* Update metadata */
  meta->current_size = new_size;
  if (is_global) {
    obj->obj_state = DIRTY;
  }
  
  return 0;
}

/* Register or get metadata for an array on first access
 * For explicitly-sized arrays: max_size = declared size
 * For unlimited arrays (size=255): max_size = UNLIMITED
 */
struct array_metadata* ensure_array_metadata(struct object *obj, unsigned int base,
                                             unsigned int declared_size,
                                             unsigned char is_global) {
  struct array_metadata *meta;
  unsigned int max_size;
  char logbuf[256];
  
  /* Check if metadata already exists */
  meta = find_array_metadata(obj, base, is_global);
  if (meta) {
    return meta;  /* Already registered */
  }
  
  /* Determine max_size based on declared size */
  /* If size is 255 (default for int *arr or int arr[]), treat as unlimited */
  if (declared_size == 255) {
    max_size = UNLIMITED_ARRAY_SIZE;
  } else {
    max_size = declared_size;  /* Explicit size limit */
  }
  
  sprintf(logbuf, "ensure_array_metadata: registering base=%u, size=%u, max=%u, global=%d",
          base, declared_size, max_size, is_global);
  logger(LOG_DEBUG, logbuf);
  
  /* Register new metadata */
  return register_array_metadata(obj, base, declared_size, max_size, is_global);
}
