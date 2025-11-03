/* sfun_arrays.c - Array system functions (efuns) exposed to softcode */

#include "config.h"
#include "object.h"
#include "protos.h"
#include "instr.h"
#include "constrct.h"
#include "file.h"

/* External reference to locals from interp.c - will be passed as parameter */
extern struct var *locals;
extern unsigned int num_locals;

/* ============================================================================
 * ARRAY EFUNS (LPC-standard functions callable from softcode)
 * ============================================================================ */

/* implode() - Join array elements into string with separator
 * Usage: string implode(mixed *arr, string separator)
 */
int s_implode(struct object *caller, struct object *obj, struct object *player,
              struct var_stack **rts) {
  struct var tmp, arr_var, sep_var, result;
  struct heap_array *arr;
  char *result_str;
  unsigned int i, total_len;
  
  /* Pop NUM_ARGS */
  if (pop(&tmp, rts, obj)) return 1;
  if (tmp.type != NUM_ARGS || tmp.value.num != 2) {
    clear_var(&tmp);
    return 1;
  }
  
  /* Pop separator */
  if (pop(&sep_var, rts, obj)) return 1;
  if (sep_var.type != STRING) {
    clear_var(&sep_var);
    return 1;
  }
  
  /* Pop array */
  if (pop(&arr_var, rts, obj)) {
    clear_var(&sep_var);
    return 1;
  }
  if (arr_var.type != ARRAY) {
    clear_var(&arr_var);
    clear_var(&sep_var);
    return 1;
  }
  
  arr = arr_var.value.array_ptr;
  if (!arr) {
    clear_var(&arr_var);
    clear_var(&sep_var);
    return 1;
  }
  
  /* Calculate total length needed */
  total_len = 1; /* null terminator */
  for (i = 0; i < arr->size; i++) {
    if (arr->elements[i].type == STRING) {
      total_len += strlen(arr->elements[i].value.string);
    } else if (arr->elements[i].type == INTEGER) {
      total_len += 20; /* max int string length */
    }
    
    if (i < arr->size - 1) {
      total_len += strlen(sep_var.value.string);
    }
  }
  
  /* Allocate result string */
  result_str = (char *) MALLOC(total_len);
  result_str[0] = '\0';
  
  /* Build result */
  for (i = 0; i < arr->size; i++) {
    if (arr->elements[i].type == STRING) {
      strcat(result_str, arr->elements[i].value.string);
    } else if (arr->elements[i].type == INTEGER) {
      sprintf(result_str + strlen(result_str), "%ld", (long)arr->elements[i].value.integer);
    }
    
    if (i < arr->size - 1) {
      strcat(result_str, sep_var.value.string);
    }
  }
  
  clear_var(&arr_var);
  clear_var(&sep_var);
  
  result.type = STRING;
  result.value.string = result_str;
  push(&result, rts);
  return 0;
}

/* explode() - Split string into array by separator
 * Usage: mixed *explode(string str, string separator)
 * Returns: New heap array of strings
 */
int s_explode(struct object *caller, struct object *obj, struct object *player,
              struct var_stack **rts) {
  struct var tmp, str_var, sep_var, result;
  struct heap_array *arr;
  char *str, *sep, *str_copy, *start, *token;
  unsigned int count, i, sep_len, len;
  
  /* Pop NUM_ARGS */
  if (pop(&tmp, rts, obj)) return 1;
  if (tmp.type != NUM_ARGS || tmp.value.num != 2) {
    clear_var(&tmp);
    return 1;
  }
  
  /* Pop separator */
  if (pop(&sep_var, rts, obj)) return 1;
  if (sep_var.type != STRING) {
    clear_var(&sep_var);
    return 1;
  }
  sep = sep_var.value.string;
  sep_len = strlen(sep);
  
  /* Pop string */
  if (pop(&str_var, rts, obj)) {
    clear_var(&sep_var);
    return 1;
  }
  if (str_var.type != STRING) {
    clear_var(&str_var);
    clear_var(&sep_var);
    return 1;
  }
  str = str_var.value.string;
  
  /* Count occurrences to determine array size */
  str_copy = copy_string(str);
  count = 0;
  token = str_copy;
  while (*token) {
    if (strncmp(token, sep, sep_len) == 0) {
      count++;
      token += sep_len;
    } else {
      token++;
    }
  }
  count++; /* One more element than separators */
  FREE(str_copy);
  
  /* Allocate heap array */
  arr = allocate_array(count, count);
  if (!arr) {
    clear_var(&str_var);
    clear_var(&sep_var);
    return 1;
  }
  
  /* Split string and populate array using strtok-like approach */
  str_copy = copy_string(str);
  start = str_copy;
  i = 0;
  
  while (i < count) {
    /* Find next separator or end of string */
    char *sep_pos = strstr(start, sep);
    
    if (sep_pos) {
      /* Found separator - extract substring */
      len = sep_pos - start;
      char *element = (char *) MALLOC(len + 1);
      strncpy(element, start, len);
      element[len] = '\0';
      
      arr->elements[i].type = STRING;
      arr->elements[i].value.string = element;
      i++;
      
      /* Move past separator */
      start = sep_pos + sep_len;
    } else {
      /* Last element - rest of string */
      arr->elements[i].type = STRING;
      arr->elements[i].value.string = copy_string(start);
      i++;
      break;
    }
  }
  
  FREE(str_copy);
  
  clear_var(&str_var);
  clear_var(&sep_var);
  
  /* Return heap array */
  result.type = ARRAY;
  result.value.array_ptr = arr;
  push(&result, rts);
  return 0;
}

/* member_array() - Find element in array, return index or -1
 * Usage: int member_array(mixed element, mixed *arr)
 */
int s_member_array(struct object *caller, struct object *obj, struct object *player,
                   struct var_stack **rts) {
  struct var tmp, elem_var, arr_var, result;
  struct heap_array *arr;
  unsigned int i;
  int found;
  
  /* Pop NUM_ARGS */
  if (pop(&tmp, rts, obj)) return 1;
  if (tmp.type != NUM_ARGS || tmp.value.num != 2) {
    clear_var(&tmp);
    return 1;
  }
  
  /* Pop array */
  if (pop(&arr_var, rts, obj)) return 1;
  if (arr_var.type != ARRAY) {
    clear_var(&arr_var);
    return 1;
  }
  
  arr = arr_var.value.array_ptr;
  if (!arr) {
    clear_var(&arr_var);
    return 1;
  }
  
  /* Pop element to search for */
  if (pop(&elem_var, rts, obj)) {
    clear_var(&arr_var);
    return 1;
  }
  
  /* Search for element */
  found = -1;
  for (i = 0; i < arr->size; i++) {
    /* Compare based on type */
    if (arr->elements[i].type == elem_var.type) {
      if (elem_var.type == INTEGER && arr->elements[i].value.integer == elem_var.value.integer) {
        found = i;
        break;
      } else if (elem_var.type == STRING && strcmp(arr->elements[i].value.string, elem_var.value.string) == 0) {
        found = i;
        break;
      } else if (elem_var.type == OBJECT && arr->elements[i].value.objptr == elem_var.value.objptr) {
        found = i;
        break;
      }
    }
  }
  
  clear_var(&elem_var);
  clear_var(&arr_var);
  
  result.type = INTEGER;
  result.value.integer = found;
  push(&result, rts);
  return 0;
}

/* Helper function for sort_array - compare two vars */
static int compare_vars(struct var *a, struct var *b) {
  if (a->type != b->type) return 0; /* Different types, no order */
  
  if (a->type == INTEGER) {
    return (a->value.integer < b->value.integer) ? -1 : 
           (a->value.integer > b->value.integer) ? 1 : 0;
  } else if (a->type == STRING) {
    return strcmp(a->value.string, b->value.string);
  }
  return 0;
}

/* sort_array() - Sort array in place (simple bubble sort for now)
 * Usage: mixed *sort_array(mixed *arr)
 * Note: Only supports simple numeric/string sorting, no custom function yet
 */
int s_sort_array(struct object *caller, struct object *obj, struct object *player,
                 struct var_stack **rts) {
  struct var tmp, arr_var, result;
  struct heap_array *arr;
  unsigned int i, j;
  int swapped;
  struct var temp_var;
  
  /* Pop NUM_ARGS */
  if (pop(&tmp, rts, obj)) return 1;
  if (tmp.type != NUM_ARGS || tmp.value.num != 1) {
    clear_var(&tmp);
    return 1;
  }
  
  /* Pop array */
  if (pop(&arr_var, rts, obj)) return 1;
  if (arr_var.type != ARRAY) {
    clear_var(&arr_var);
    return 1;
  }
  
  arr = arr_var.value.array_ptr;
  if (!arr || arr->size == 0) {
    push(&arr_var, rts);
    return 0;
  }
  
  /* Bubble sort */
  for (i = 0; i < arr->size - 1; i++) {
    swapped = 0;
    for (j = 0; j < arr->size - i - 1; j++) {
      if (compare_vars(&arr->elements[j], &arr->elements[j + 1]) > 0) {
        /* Swap */
        temp_var = arr->elements[j];
        arr->elements[j] = arr->elements[j + 1];
        arr->elements[j + 1] = temp_var;
        swapped = 1;
      }
    }
    if (!swapped) break; /* Already sorted */
  }
  
  /* Return the array */
  result = arr_var;
  push(&result, rts);
  return 0;
}

/* reverse() - Reverse array in place
 * Usage: mixed *reverse(mixed *arr)
 */
int s_reverse(struct object *caller, struct object *obj, struct object *player,
              struct var_stack **rts) {
  struct var tmp, arr_var, result;
  struct heap_array *arr;
  unsigned int i, j;
  struct var temp_var;
  
  /* Pop NUM_ARGS */
  if (pop(&tmp, rts, obj)) return 1;
  if (tmp.type != NUM_ARGS || tmp.value.num != 1) {
    clear_var(&tmp);
    return 1;
  }
  
  /* Pop array */
  if (pop(&arr_var, rts, obj)) return 1;
  if (arr_var.type != ARRAY) {
    clear_var(&arr_var);
    return 1;
  }
  
  arr = arr_var.value.array_ptr;
  if (!arr || arr->size == 0) {
    push(&arr_var, rts);
    return 0;
  }
  
  /* Reverse in place by swapping elements */
  for (i = 0, j = arr->size - 1; i < j; i++, j--) {
    /* Swap */
    temp_var = arr->elements[i];
    arr->elements[i] = arr->elements[j];
    arr->elements[j] = temp_var;
  }
  
  /* Return the array */
  result = arr_var;
  push(&result, rts);
  return 0;
}

/* unique_array() - Remove duplicate elements from array
 * Usage: mixed *unique_array(mixed *arr)
 * Returns: New heap array with duplicates removed
 */
int s_unique_array(struct object *caller, struct object *obj, struct object *player,
                   struct var_stack **rts) {
  struct var tmp, arr_var, result;
  struct heap_array *arr, *new_arr;
  unsigned int i, j, unique_count;
  int is_duplicate;
  
  /* Pop NUM_ARGS */
  if (pop(&tmp, rts, obj)) return 1;
  if (tmp.type != NUM_ARGS || tmp.value.num != 1) {
    clear_var(&tmp);
    return 1;
  }
  
  /* Pop array */
  if (pop(&arr_var, rts, obj)) return 1;
  if (arr_var.type != ARRAY) {
    clear_var(&arr_var);
    return 1;
  }
  
  arr = arr_var.value.array_ptr;
  if (!arr) {
    clear_var(&arr_var);
    return 1;
  }
  
  /* Count unique elements */
  unique_count = 0;
  for (i = 0; i < arr->size; i++) {
    /* Check if this element appeared before */
    is_duplicate = 0;
    for (j = 0; j < i; j++) {
      if (arr->elements[i].type == arr->elements[j].type) {
        if ((arr->elements[i].type == INTEGER && arr->elements[i].value.integer == arr->elements[j].value.integer) ||
            (arr->elements[i].type == STRING && strcmp(arr->elements[i].value.string, arr->elements[j].value.string) == 0) ||
            (arr->elements[i].type == OBJECT && arr->elements[i].value.objptr == arr->elements[j].value.objptr)) {
          is_duplicate = 1;
          break;
        }
      }
    }
    
    if (!is_duplicate) {
      unique_count++;
    }
  }
  
  /* Allocate new heap array */
  new_arr = allocate_array(unique_count, unique_count);
  if (!new_arr) {
    clear_var(&arr_var);
    return 1;
  }
  
  /* Copy unique elements */
  unique_count = 0;
  for (i = 0; i < arr->size; i++) {
    /* Check if duplicate */
    is_duplicate = 0;
    for (j = 0; j < i; j++) {
      if (arr->elements[i].type == arr->elements[j].type) {
        if ((arr->elements[i].type == INTEGER && arr->elements[i].value.integer == arr->elements[j].value.integer) ||
            (arr->elements[i].type == STRING && strcmp(arr->elements[i].value.string, arr->elements[j].value.string) == 0) ||
            (arr->elements[i].type == OBJECT && arr->elements[i].value.objptr == arr->elements[j].value.objptr)) {
          is_duplicate = 1;
          break;
        }
      }
    }
    
    if (!is_duplicate) {
      /* Copy to new array */
      if (arr->elements[i].type == STRING) {
        new_arr->elements[unique_count].type = STRING;
        new_arr->elements[unique_count].value.string = copy_string(arr->elements[i].value.string);
      } else {
        new_arr->elements[unique_count] = arr->elements[i];
      }
      unique_count++;
    }
  }
  
  clear_var(&arr_var);
  
  /* Return new array */
  result.type = ARRAY;
  result.value.array_ptr = new_arr;
  push(&result, rts);
  return 0;
}
