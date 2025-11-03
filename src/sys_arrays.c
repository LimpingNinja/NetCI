/* sys_arrays.c - Array system functions */

#include "config.h"
#include "object.h"
#include "protos.h"
#include "instr.h"
#include "constrct.h"
#include "file.h"

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
 * Note: Uses gen_stack_noresolve, so receives unresolved L_VALUES
 */
int s_sizeof(struct object *caller, struct object *obj, struct object *player,
             struct var_stack **rts) {
  struct var tmp, result;
  unsigned int array_size;
  char logbuf[256];
  
  sprintf(logbuf, "s_sizeof: called, rts=%p", (void*)rts);
  logger(LOG_DEBUG, logbuf);
  
  /* Pop number of arguments */
  if (pop(&tmp, rts, obj)) {
    logger(LOG_ERROR, "s_sizeof: failed to pop NUM_ARGS");
    return 1;
  }
  
  sprintf(logbuf, "s_sizeof: NUM_ARGS type=%d, value=%ld", tmp.type, tmp.value.num);
  logger(LOG_DEBUG, logbuf);
  
  if (tmp.type != NUM_ARGS) {
    logger(LOG_ERROR, "s_sizeof: not NUM_ARGS");
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num != 1) {
    logger(LOG_ERROR, "s_sizeof: wrong arg count");
    return 1;
  }
  
  /* Pop the array L_VALUE */
  if (pop(&tmp, rts, obj)) {
    logger(LOG_ERROR, "s_sizeof: failed to pop argument");
    return 1;
  }
  
  /* Check if it's an L_VALUE */
  if (tmp.type != LOCAL_L_VALUE && tmp.type != GLOBAL_L_VALUE) {
    logger(LOG_ERROR, "s_sizeof: not an L_VALUE");
    clear_var(&tmp);
    return 1;
  }
  
  /* Get array size from L_VALUE metadata */
  array_size = tmp.value.l_value.size;
  
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

/* Increment array reference count */
void array_addref(struct array_metadata *meta) {
  if (meta) {
    meta->ref_count++;
  }
}

/* Decrement array reference count and free if zero */
void array_release(struct object *obj, struct array_metadata *meta) {
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
