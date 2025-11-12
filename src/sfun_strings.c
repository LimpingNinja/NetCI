/* sfun_string.c - String manipulation functions for NLPC
 *
 * Contains implementations of string-related efuns:
 * - sscanf() - pattern matching and parsing
 *
 * sscanf() supports: %s (string), %d (integer), %x (hex), %*x (skip), %% (literal %)
 * Usage: int sscanf(string input, string format, ...)
 * Returns: number of successful assignments
 */

#include "config.h"
#include "object.h"
#include "instr.h"
#include "interp.h"
#include "protos.h"
#include "globals.h"
#include "constrct.h"
#include "file.h"
#include "token.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Format specifier types */
enum spec_type {
  LITERAL,      /* Literal text between specifiers */
  SPEC_STRING,  /* %s - match string */
  SPEC_INT,     /* %d - match integer */
  SPEC_HEX      /* %x - match hex, convert to int */
};

/* Format specifier structure */
struct format_spec {
  enum spec_type type;
  char *literal_text;  /* For LITERAL type only */
  int skip;            /* 1 if %*x (skip assignment) */
};

/* ========================================================================
 * HELPER FUNCTIONS - Format String Parsing
 * ======================================================================== */

/* Parse format string into array of format_spec structures
 * Returns array of specs and sets spec_count
 * Caller must free the array and any literal_text strings
 */
static struct format_spec* parse_format_string(const char *fmt, int *spec_count) {
  struct format_spec *specs = NULL;
  int capacity = 8;
  int count = 0;
  const char *p = fmt;
  
  specs = (struct format_spec*) MALLOC(capacity * sizeof(struct format_spec));
  
  while (*p) {
    /* Expand array if needed */
    if (count >= capacity) {
      capacity *= 2;
      specs = (struct format_spec*) realloc(specs, capacity * sizeof(struct format_spec));
    }
    
    if (*p == '%') {
      p++;
      
      /* Handle %% (literal %) */
      if (*p == '%') {
        specs[count].type = LITERAL;
        specs[count].literal_text = MALLOC(2);
        specs[count].literal_text[0] = '%';
        specs[count].literal_text[1] = '\0';
        specs[count].skip = 0;
        count++;
        p++;
        continue;
      }
      
      /* Check for skip flag %* */
      int skip = 0;
      if (*p == '*') {
        skip = 1;
        p++;
      }
      
      /* Parse specifier type */
      if (*p == 's') {
        specs[count].type = SPEC_STRING;
        specs[count].literal_text = NULL;
        specs[count].skip = skip;
        count++;
        p++;
      } else if (*p == 'd') {
        specs[count].type = SPEC_INT;
        specs[count].literal_text = NULL;
        specs[count].skip = skip;
        count++;
        p++;
      } else if (*p == 'x') {
        specs[count].type = SPEC_HEX;
        specs[count].literal_text = NULL;
        specs[count].skip = skip;
        count++;
        p++;
      } else {
        /* Invalid specifier - treat as literal */
        specs[count].type = LITERAL;
        specs[count].literal_text = MALLOC(3);
        specs[count].literal_text[0] = '%';
        specs[count].literal_text[1] = *p;
        specs[count].literal_text[2] = '\0';
        specs[count].skip = 0;
        count++;
        p++;
      }
    } else {
      /* Collect literal text until next % or end */
      const char *start = p;
      while (*p && *p != '%') p++;
      
      int len = p - start;
      if (len > 0) {
        specs[count].type = LITERAL;
        specs[count].literal_text = MALLOC(len + 1);
        memcpy(specs[count].literal_text, start, len);
        specs[count].literal_text[len] = '\0';
        specs[count].skip = 0;
        count++;
      }
    }
  }
  
  *spec_count = count;
  return specs;
}

/* Free format spec array and all allocated strings */
static void free_format_specs(struct format_spec *specs, int count) {
  int i;
  for (i = 0; i < count; i++) {
    if (specs[i].literal_text) {
      FREE(specs[i].literal_text);
    }
  }
  FREE(specs);
}

/* ========================================================================
 * HELPER FUNCTIONS - Pattern Matching
 * ======================================================================== */

/* Match literal text at current position
 * Returns 1 if matched, 0 if not
 * Advances pos if matched
 */
static int match_literal(const char *input, int *pos, const char *literal) {
  int len = strlen(literal);
  if (strncmp(input + *pos, literal, len) == 0) {
    *pos += len;
    return 1;
  }
  return 0;
}

/* Match string specifier (%s)
 * Matches until next literal text or end of string
 * Returns allocated string (caller must FREE)
 */
static char* match_string_spec(const char *input, int *pos, const char *next_literal) {
  int start = *pos;
  int end = start;
  
  if (next_literal) {
    /* Match until we find the next literal */
    const char *found = strstr(input + start, next_literal);
    if (found) {
      end = found - input;
    } else {
      /* Next literal not found, match to end */
      end = strlen(input);
    }
  } else {
    /* No next literal, match to end of string */
    end = strlen(input);
  }
  
  /* Trim trailing whitespace if there's a next literal */
  if (next_literal && end > start) {
    while (end > start && isspace((unsigned char)input[end - 1])) {
      end--;
    }
  }
  
  int len = end - start;
  char *result = MALLOC(len + 1);
  memcpy(result, input + start, len);
  result[len] = '\0';
  
  *pos = end;
  return result;
}

/* Match integer specifier (%d)
 * Returns 1 if matched, 0 if not
 * Sets result if matched
 */
static int match_int_spec(const char *input, int *pos, long *result) {
  const char *p = input + *pos;
  char *endptr;
  long val;
  
  /* Skip leading whitespace */
  while (isspace((unsigned char)*p)) p++;
  
  /* Parse integer */
  val = strtol(p, &endptr, 10);
  
  /* Check if we parsed anything */
  if (endptr == p) {
    return 0;  /* No digits found */
  }
  
  *result = val;
  *pos = endptr - input;
  return 1;
}

/* Match hex specifier (%x)
 * Returns 1 if matched, 0 if not
 * Sets result if matched (converted to base 10)
 */
static int match_hex_spec(const char *input, int *pos, long *result) {
  const char *p = input + *pos;
  char *endptr;
  long val;
  
  /* Skip leading whitespace */
  while (isspace((unsigned char)*p)) p++;
  
  /* Skip optional 0x prefix */
  if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
    p += 2;
  }
  
  /* Parse hex */
  val = strtol(p, &endptr, 16);
  
  /* Check if we parsed anything */
  if (endptr == p) {
    return 0;  /* No hex digits found */
  }
  
  *result = val;
  *pos = endptr - input;
  return 1;
}

/* ========================================================================
 * HELPER FUNCTIONS - L-Value Assignment
 * ======================================================================== */

/* Assign a value to an l-value reference
 * Handles both LOCAL_L_VALUE and GLOBAL_L_VALUE
 * Note: l_value.ref can be either an index or a direct pointer to a var
 */
static void assign_to_lvalue(struct var *lvalue, struct var *value, struct object *obj) {
  struct var *target_var;
  
  if (lvalue->type == GLOBAL_L_VALUE) {
    obj->obj_state = DIRTY;
    
    /* Check if this is a heap array element (pointer) or regular global */
    if (lvalue->value.l_value.ref >= obj->parent->funcs->num_globals) {
      /* Heap array element - direct pointer */
      target_var = (struct var *)lvalue->value.l_value.ref;
      clear_var(target_var);
      *target_var = *value;
    } else {
      /* Regular global variable - map via GST */
      extern struct call_frame *call_stack;
      int ok = 0;
      unsigned int eff = global_index_for(obj, call_stack ? call_stack->func : NULL,
                                          (unsigned int)lvalue->value.l_value.ref, &ok);
      if (!ok) return;  /* Fail silently for sscanf semantics */
      clear_global_var(obj, eff);
      obj->globals[eff] = *value;
    }
  } else if (lvalue->type == LOCAL_L_VALUE) {
    /* Check if this is a heap array element (pointer) or regular local */
    if (lvalue->value.l_value.ref >= num_locals) {
      /* Heap array element - direct pointer */
      target_var = (struct var *)lvalue->value.l_value.ref;
      clear_var(target_var);
      *target_var = *value;
    } else {
      /* Regular local variable */
      clear_var(&locals[lvalue->value.l_value.ref]);
      locals[lvalue->value.l_value.ref] = *value;
    }
  }
}

/* ========================================================================
 * MAIN SSCANF IMPLEMENTATION
 * ======================================================================== */

/* s_sscanf - System call implementation
 * Stack layout (top to bottom):
 *   NUM_ARGS (total count)
 *   L_VALUE_n (variable references)
 *   ...
 *   L_VALUE_1
 *   STRING (format string)
 *   STRING (input string)
 */
int s_sscanf(struct object *caller, struct object *obj, 
             struct object *player, struct var_stack **rts) {
  struct var tmp, num_args_var;
  struct var input_var, format_var;
  struct var *lvalues = NULL;
  struct var result;
  int num_args, num_lvalues;
  int i;
  
  /* Pop NUM_ARGS */
  if (pop(&num_args_var, rts, obj)) return 1;
  if (num_args_var.type != NUM_ARGS) {
    clear_var(&num_args_var);
    return 1;
  }
  num_args = num_args_var.value.num;
  /* Need at least 2 args (input, format) */
  if (num_args < 2) {
    result.type = INTEGER;
    result.value.integer = 0;
    push(&result, rts);
    return 0;
  }
  
  /* Calculate number of l-values */
  num_lvalues = num_args - 2;
  
  /* Pop l-values into array (in reverse order) */
  if (num_lvalues > 0) {
    lvalues = MALLOC(num_lvalues * sizeof(struct var));
    for (i = num_lvalues - 1; i >= 0; i--) {
      if (pop(&lvalues[i], rts, obj)) {
        FREE(lvalues);
        return 1;
      }
      /* Verify it's an l-value */
      if (lvalues[i].type != LOCAL_L_VALUE && lvalues[i].type != GLOBAL_L_VALUE) {
        FREE(lvalues);
        return 1;
      }
    }
  }

  /* Pop format string */
  if (pop(&format_var, rts, obj)) {
    if (lvalues) FREE(lvalues);
    return 1;
  }
  /* Resolve refs for format (not an assignment target) */
  if (format_var.type==LOCAL_L_VALUE || format_var.type==GLOBAL_L_VALUE ||
      format_var.type==LOCAL_REF || format_var.type==GLOBAL_REF) {
    if (resolve_var(&format_var, obj)) {
      if (lvalues) FREE(lvalues);
      return 1;
    }
  }
  /* Treat INTEGER 0 as empty string for compatibility */
  if (format_var.type==INTEGER && format_var.value.integer==0) {
    format_var.type=STRING;
    *(format_var.value.string=MALLOC(1))='\0';
  }
  if (format_var.type != STRING) {
    clear_var(&format_var);
    if (lvalues) FREE(lvalues);
    return 1;
  }

  /* Pop input string */
  if (pop(&input_var, rts, obj)) {
    /* If input is missing or NULL on stack, treat as empty string */
    input_var.type = STRING;
    *(input_var.value.string = MALLOC(1)) = '\0';
  }
  /* Resolve refs for input (not an assignment target) */
  if (input_var.type==LOCAL_L_VALUE || input_var.type==GLOBAL_L_VALUE ||
      input_var.type==LOCAL_REF || input_var.type==GLOBAL_REF) {
    if (resolve_var(&input_var, obj)) {
      clear_var(&format_var);
      if (lvalues) FREE(lvalues);
      return 1;
    }
  }
  /* Treat INTEGER 0 as empty string for compatibility */
  if (input_var.type==INTEGER && input_var.value.integer==0) {
    input_var.type=STRING;
    *(input_var.value.string=MALLOC(1))='\0';
  }
  if (input_var.type != STRING) {
    clear_var(&input_var);
    clear_var(&format_var);
    if (lvalues) FREE(lvalues);
    return 1;
  }

  /* Parse format string */
  int spec_count;
  struct format_spec *specs = parse_format_string(format_var.value.string, &spec_count);
  
  /* Perform matching */
  int input_pos = 0;
  int lvalue_index = 0;
  int matched_count = 0;
  
  for (i = 0; i < spec_count; i++) {
    struct format_spec *spec = &specs[i];
    
    if (spec->type == LITERAL) {
      /* Match literal text */
      if (!match_literal(input_var.value.string, &input_pos, spec->literal_text)) {
        break;  /* Literal didn't match, stop parsing */
      }
    } else if (spec->type == SPEC_STRING) {
      /* Find next literal for boundary */
      const char *next_literal = NULL;
      if (i + 1 < spec_count && specs[i + 1].type == LITERAL) {
        next_literal = specs[i + 1].literal_text;
      }
      
      /* Match string */
      char *matched_str = match_string_spec(input_var.value.string, &input_pos, next_literal);
      
      if (!spec->skip) {
        /* Assign to l-value */
        if (lvalue_index < num_lvalues) {
          struct var str_val;
          str_val.type = STRING;
          str_val.value.string = matched_str;
          assign_to_lvalue(&lvalues[lvalue_index], &str_val, obj);
          lvalue_index++;
          matched_count++;
          /* Don't free matched_str - ownership transferred to variable */
        } else {
          /* No l-value to assign to, free the string */
          FREE(matched_str);
        }
      } else {
        /* Skip flag set, free the string */
        FREE(matched_str);
      }
    } else if (spec->type == SPEC_INT) {
      /* Match integer */
      long int_val;
      if (match_int_spec(input_var.value.string, &input_pos, &int_val)) {
        if (!spec->skip) {
          /* Assign to l-value */
          if (lvalue_index < num_lvalues) {
            struct var int_var;
            int_var.type = INTEGER;
            int_var.value.integer = int_val;
            assign_to_lvalue(&lvalues[lvalue_index], &int_var, obj);
            lvalue_index++;
            matched_count++;
          }
        }
      } else {
        break;  /* Integer didn't match, stop parsing */
      }
    } else if (spec->type == SPEC_HEX) {
      /* Match hex */
      long hex_val;
      if (match_hex_spec(input_var.value.string, &input_pos, &hex_val)) {
        if (!spec->skip) {
          /* Assign to l-value */
          if (lvalue_index < num_lvalues) {
            struct var hex_var;
            hex_var.type = INTEGER;
            hex_var.value.integer = hex_val;
            assign_to_lvalue(&lvalues[lvalue_index], &hex_var, obj);
            lvalue_index++;
            matched_count++;
          }
        }
      } else {
        break;  /* Hex didn't match, stop parsing */
      }
    }
  }
  
  /* Cleanup */
  free_format_specs(specs, spec_count);
  clear_var(&input_var);
  clear_var(&format_var);
  if (lvalues) FREE(lvalues);
  
  /* Push result (number of matches) */
  result.type = INTEGER;
  result.value.integer = matched_count;
  push(&result, rts);
  
  return 0;
}

/* ========================================================================
 * SAVE_VALUE / RESTORE_VALUE EFUNS
 * ======================================================================== */

/* EFUN: save_value(mixed value) -> string
 * Serializes any LPC value to a string representation
 * Returns: String representation or 0 on error
 */
int s_save_value(struct object *caller, struct object *obj,
                 struct object *player, struct var_stack **rts) {
    struct var tmp;
    char *result_str;
    
    /* Pop NUM_ARGS */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != NUM_ARGS || tmp.value.num != 1) {
        clear_var(&tmp);
        return 1;
    }
    
    /* Pop value to save */
    if (pop(&tmp, rts, obj)) return 1;
    if (resolve_var(&tmp, obj)) return 1;
    
    /* Serialize */
    result_str = save_value_internal(&tmp, 0);
    clear_var(&tmp);
    
    if (!result_str) {
        /* Error - push 0 */
        tmp.type = INTEGER;
        tmp.value.integer = 0;
        push(&tmp, rts);
        return 0;
    }
    
    /* Push result string */
    tmp.type = STRING;
    tmp.value.string = result_str;
    push(&tmp, rts);
    
    return 0;
}

/* EFUN: restore_value(string str) -> mixed
 * Deserializes a string back into an LPC value
 * Returns: Deserialized value or 0 on error
 */
int s_restore_value(struct object *caller, struct object *obj,
                    struct object *player, struct var_stack **rts) {
    struct var tmp, result;
    filptr *fp;
    char *saved_str;
    
    /* Pop NUM_ARGS */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != NUM_ARGS || tmp.value.num != 1) {
        clear_var(&tmp);
        return 1;
    }
    
    /* Pop string argument */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != STRING) {
        clear_var(&tmp);
        return 1;
    }
    saved_str = tmp.value.string;
    
    /* Create mock filptr pointing to string */
    fp = create_string_filptr(saved_str);
    if (!fp) {
        clear_var(&tmp);
        /* Return 0 on error */
        result.type = INTEGER;
        result.value.integer = 0;
        push(&result, rts);
        return 0;
    }
    
    /* Parse the value using existing tokenizer! */
    if (parse_value(fp, &result)) {
        free_string_filptr(fp);
        clear_var(&tmp);
        
        /* Return 0 on parse error */
        result.type = INTEGER;
        result.value.integer = 0;
        push(&result, rts);
        return 0;
    }
    
    /* Clean up */
    free_string_filptr(fp);
    clear_var(&tmp);
    
    /* Push result */
    push(&result, rts);
    return 0;
}

/* ========================================================================
 * REPLACE_STRING EFUN
 * ======================================================================== */

/* EFUN: replace_string(string str, string search, string replace) -> string
 * Replaces all occurrences of search string with replace string
 * Returns: New string with replacements, or original if search not found
 */
int s_replace_string(struct object *caller, struct object *obj,
                     struct object *player, struct var_stack **rts) {
    struct var tmp, str_var, search_var, replace_var, result;
    char *str, *search, *replace;
    char *result_str, *temp_str;
    char *pos, *last_pos;
    int search_len, replace_len, count;
    int result_len, segment_len;
    
    /* Pop NUM_ARGS */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != NUM_ARGS || tmp.value.num != 3) {
        clear_var(&tmp);
        return 1;
    }
    
    /* Pop replace string */
    if (pop(&replace_var, rts, obj)) return 1;
    if (replace_var.type == INTEGER && replace_var.value.integer == 0) {
        /* Handle 0 as empty string */
        replace = "";
        replace_len = 0;
    } else if (replace_var.type != STRING) {
        clear_var(&replace_var);
        return 1;
    } else {
        replace = replace_var.value.string;
        replace_len = strlen(replace);
    }
    
    /* Pop search string */
    if (pop(&search_var, rts, obj)) return 1;
    if (search_var.type == INTEGER && search_var.value.integer == 0) {
        /* Handle 0 as empty string */
        search = "";
        search_len = 0;
    } else if (search_var.type != STRING) {
        clear_var(&search_var);
        clear_var(&replace_var);
        return 1;
    } else {
        search = search_var.value.string;
        search_len = strlen(search);
    }
    
    /* Pop input string */
    if (pop(&str_var, rts, obj)) return 1;
    if (str_var.type == INTEGER && str_var.value.integer == 0) {
        /* Handle 0 as empty string - return empty string */
        clear_var(&search_var);
        clear_var(&replace_var);
        result.type = STRING;
        result.value.string = copy_string("");
        push(&result, rts);
        return 0;
    } else if (str_var.type != STRING) {
        clear_var(&str_var);
        clear_var(&search_var);
        clear_var(&replace_var);
        return 1;
    } else {
        str = str_var.value.string;
    }
    
    /* Handle edge cases */
    if (search_len == 0) {
        /* Empty search string - return original */
        result.type = STRING;
        result.value.string = copy_string(str);
        clear_var(&str_var);
        clear_var(&search_var);
        clear_var(&replace_var);
        push(&result, rts);
        return 0;
    }
    
    /* Count occurrences of search string */
    count = 0;
    pos = str;
    while ((pos = strstr(pos, search)) != NULL) {
        count++;
        pos += search_len;
    }
    
    /* If no matches, return original string */
    if (count == 0) {
        result.type = STRING;
        result.value.string = copy_string(str);
        clear_var(&str_var);
        clear_var(&search_var);
        clear_var(&replace_var);
        push(&result, rts);
        return 0;
    }
    
    /* Calculate result length */
    result_len = strlen(str) - (count * search_len) + (count * replace_len);
    
    /* Allocate result string */
    result_str = MALLOC(result_len + 1);
    if (!result_str) {
        clear_var(&str_var);
        clear_var(&search_var);
        clear_var(&replace_var);
        result.type = INTEGER;
        result.value.integer = 0;
        push(&result, rts);
        return 0;
    }
    
    /* Build result string with replacements */
    temp_str = result_str;
    last_pos = str;
    pos = str;
    
    while ((pos = strstr(pos, search)) != NULL) {
        /* Copy segment before match */
        segment_len = pos - last_pos;
        if (segment_len > 0) {
            memcpy(temp_str, last_pos, segment_len);
            temp_str += segment_len;
        }
        
        /* Copy replacement string */
        if (replace_len > 0) {
            memcpy(temp_str, replace, replace_len);
            temp_str += replace_len;
        }
        
        /* Move past the match */
        pos += search_len;
        last_pos = pos;
    }
    
    /* Copy remaining string after last match */
    segment_len = strlen(last_pos);
    if (segment_len > 0) {
        memcpy(temp_str, last_pos, segment_len);
        temp_str += segment_len;
    }
    
    /* Null terminate */
    *temp_str = '\0';
    
    /* Clean up and push result */
    clear_var(&str_var);
    clear_var(&search_var);
    clear_var(&replace_var);
    
    result.type = STRING;
    result.value.string = result_str;
    push(&result, rts);
    
    return 0;
}
