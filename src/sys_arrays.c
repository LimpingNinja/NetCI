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
