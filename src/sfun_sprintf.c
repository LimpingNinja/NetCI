/* sfun_sprintf.c - String formatting function for NLPC
 *
 * Implementation of sprintf() efun with C-style and LPC extensions.
 * 
 * Phase 1 (MVP): Basic C-style formatting ✅ COMPLETE
 * - Format specifiers: %s %d %i %c %o %x %X %%
 * - Modifiers: width, precision, zero-pad, left-align, signs
 *
 * Phase 2 (LPC Extensions): ✅ COMPLETE
 * - %O: Pretty-print arrays/mappings using save_value_internal()
 * - %@: Array iteration (apply format to each element)
 * - %*: Dynamic width from argument
 * - %|: Center alignment
 *
 * Phase 3 (Advanced Features): ✅ COMPLETE
 * - %=: Column mode (word wrapping with padding)
 * - %#: Table mode (multi-column layout from newline-separated list)
 * - %$: Justify mode (even spacing between words)
 * - Default width: 80 chars (terminal negotiation for future)
 *
 * Usage: string sprintf(string format, ...)
 * Returns: formatted string
 */

#include "config.h"
#include "object.h"
#include "instr.h"
#include "interp.h"
#include "protos.h"
#include "globals.h"
#include "constrct.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Format spec flags */
#define FLAG_LEFT_ALIGN   0x01  /* - */
#define FLAG_ZERO_PAD     0x02  /* 0 */
#define FLAG_SHOW_SIGN    0x04  /* + */
#define FLAG_SPACE_SIGN   0x08  /* (space) */
#define FLAG_HASH         0x10  /* # (for table mode, phase 3) */
#define FLAG_ARRAY_ITER   0x20  /* @ (for phase 2) */
#define FLAG_CENTER       0x40  /* | (for phase 2) */
#define FLAG_COLUMN       0x80  /* = (for column/word-wrap mode, phase 3) */
#define FLAG_JUSTIFY      0x100 /* $ (for justify mode, phase 3) */

/* Format specifier structure */
struct format_spec {
  char type;          /* 's', 'd', 'x', etc. */
  int width;          /* minimum field width */
  int precision;      /* precision for strings/floats, -1 if not specified */
  int flags;          /* combination of FLAG_* above */
  char *custom_pad;   /* custom padding string (phase 2), NULL = use default */
};

/* Output buffer management */
struct output_buffer {
  char *data;
  size_t length;
  size_t capacity;
};

/* ========================================================================
 * BUFFER MANAGEMENT
 * ======================================================================== */

static struct output_buffer* buf_create(void) {
  struct output_buffer *buf = MALLOC(sizeof(struct output_buffer));
  buf->capacity = 256;
  buf->data = MALLOC(buf->capacity);
  buf->data[0] = '\0';
  buf->length = 0;
  return buf;
}

static void buf_append(struct output_buffer *buf, const char *str, size_t len) {
  if (buf->length + len + 1 > buf->capacity) {
    while (buf->length + len + 1 > buf->capacity) {
      buf->capacity *= 2;
    }
    buf->data = realloc(buf->data, buf->capacity);
  }
  memcpy(buf->data + buf->length, str, len);
  buf->length += len;
  buf->data[buf->length] = '\0';
}

static void buf_append_char(struct output_buffer *buf, char ch) {
  buf_append(buf, &ch, 1);
}

static char* buf_finalize(struct output_buffer *buf) {
  char *result = buf->data;
  FREE(buf);
  return result;
}

static void buf_destroy(struct output_buffer *buf) {
  FREE(buf->data);
  FREE(buf);
}

/* ========================================================================
 * FORMAT STRING PARSING
 * ======================================================================== */

/* Parse a single format specifier from format string
 * Returns: pointer to char after specifier, or NULL on error
 * Fills in *spec with parsed values
 */
static const char* parse_spec(const char *fmt, struct format_spec *spec) {
  const char *p = fmt;
  
  /* Initialize */
  memset(spec, 0, sizeof(struct format_spec));
  spec->precision = -1;  /* -1 means not specified */
  
  /* Must start with % */
  if (*p != '%') return NULL;
  p++;
  
  /* Parse flags */
  while (*p) {
    if (*p == '-') {
      spec->flags |= FLAG_LEFT_ALIGN;
      p++;
    } else if (*p == '0') {
      spec->flags |= FLAG_ZERO_PAD;
      p++;
    } else if (*p == '+') {
      spec->flags |= FLAG_SHOW_SIGN;
      p++;
    } else if (*p == ' ') {
      spec->flags |= FLAG_SPACE_SIGN;
      p++;
    } else if (*p == '#') {
      spec->flags |= FLAG_HASH;
      p++;
    } else if (*p == '@') {
      spec->flags |= FLAG_ARRAY_ITER;
      p++;
    } else if (*p == '|') {
      spec->flags |= FLAG_CENTER;
      p++;
    } else if (*p == '=') {
      spec->flags |= FLAG_COLUMN;
      p++;
    } else if (*p == '$') {
      spec->flags |= FLAG_JUSTIFY;
      p++;
    } else {
      break;
    }
  }
  
  /* Parse width */
  if (*p == '*') {
    /* Dynamic width from argument - Phase 2 feature */
    spec->width = -1;  /* Special marker */
    p++;
  } else if (isdigit(*p)) {
    spec->width = 0;
    while (isdigit(*p)) {
      spec->width = spec->width * 10 + (*p - '0');
      p++;
    }
  }
  
  /* Parse precision */
  if (*p == '.') {
    p++;
    if (*p == '*') {
      /* Dynamic precision - Phase 2 */
      spec->precision = -2;  /* Special marker */
      p++;
    } else if (isdigit(*p)) {
      spec->precision = 0;
      while (isdigit(*p)) {
        spec->precision = spec->precision * 10 + (*p - '0');
        p++;
      }
    } else {
      spec->precision = 0;  /* ".x" with no number means 0 */
    }
  }
  
  /* Parse type specifier */
  if (*p) {
    spec->type = *p;
    p++;
  } else {
    return NULL;  /* Missing type */
  }
  
  return p;
}

/* ========================================================================
 * TYPE FORMATTERS - PHASE 1
 * ======================================================================== */

/* Apply padding to a string according to format spec */
static void apply_padding(struct output_buffer *buf, const char *str, 
                         size_t str_len, struct format_spec *spec) {
  int width = spec->width;
  int pad_len = (width > str_len) ? (width - str_len) : 0;
  char pad_char = (spec->flags & FLAG_ZERO_PAD) ? '0' : ' ';
  int i;
  
  /* Center alignment (%|) */
  if ((spec->flags & FLAG_CENTER) && pad_len > 0) {
    int left_pad = pad_len / 2;
    int right_pad = pad_len - left_pad;
    
    /* Left padding */
    for (i = 0; i < left_pad; i++) {
      buf_append_char(buf, ' ');
    }
    
    /* The string itself */
    buf_append(buf, str, str_len);
    
    /* Right padding */
    for (i = 0; i < right_pad; i++) {
      buf_append_char(buf, ' ');
    }
    return;
  }
  
  /* Left padding (right justify) */
  if (!(spec->flags & FLAG_LEFT_ALIGN) && pad_len > 0) {
    for (i = 0; i < pad_len; i++) {
      buf_append_char(buf, pad_char);
    }
  }
  
  /* The string itself */
  buf_append(buf, str, str_len);
  
  /* Right padding (left justify) */
  if ((spec->flags & FLAG_LEFT_ALIGN) && pad_len > 0) {
    for (i = 0; i < pad_len; i++) {
      buf_append_char(buf, ' ');  /* Always space for left-align padding */
    }
  }
}

/* Format a string value */
static void format_string(struct output_buffer *buf, const char *str, 
                         struct format_spec *spec) {
  size_t len = strlen(str);
  
  /* Apply precision (truncation) */
  if (spec->precision >= 0 && spec->precision < len) {
    len = spec->precision;
  }
  
  apply_padding(buf, str, len, spec);
}

/* Format an integer in specified base */
static void format_integer(struct output_buffer *buf, long value, 
                          struct format_spec *spec, int base, int uppercase) {
  char temp[64];
  char *p = temp + sizeof(temp) - 1;
  int negative = 0;
  unsigned long abs_val;
  const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
  int temp_len;
  
  *p = '\0';
  
  /* Handle sign */
  if (value < 0 && base == 10) {
    negative = 1;
    abs_val = -value;
  } else {
    abs_val = (unsigned long)value;
  }
  
  /* Convert to string in reverse */
  if (abs_val == 0) {
    *(--p) = '0';
  } else {
    while (abs_val > 0) {
      *(--p) = digits[abs_val % base];
      abs_val /= base;
    }
  }
  
  temp_len = (temp + sizeof(temp) - 1) - p;
  
  /* Handle sign display */
  if (base == 10) {
    if (negative) {
      *(--p) = '-';
      temp_len++;
    } else if (spec->flags & FLAG_SHOW_SIGN) {
      *(--p) = '+';
      temp_len++;
    } else if (spec->flags & FLAG_SPACE_SIGN) {
      *(--p) = ' ';
      temp_len++;
    }
  }
  
  /* Special case: zero padding with sign needs sign first */
  if ((spec->flags & FLAG_ZERO_PAD) && 
      !(spec->flags & FLAG_LEFT_ALIGN) && 
      (negative || (spec->flags & (FLAG_SHOW_SIGN | FLAG_SPACE_SIGN)))) {
    /* Output sign first */
    if (negative) {
      buf_append_char(buf, '-');
    } else if (spec->flags & FLAG_SHOW_SIGN) {
      buf_append_char(buf, '+');
    } else if (spec->flags & FLAG_SPACE_SIGN) {
      buf_append_char(buf, ' ');
    }
    p++;  /* Skip the sign we already added */
    temp_len--;
    
    /* Now apply zero padding */
    int pad_len = spec->width - temp_len - 1;  /* -1 for sign already output */
    while (pad_len > 0) {
      buf_append_char(buf, '0');
      pad_len--;
    }
    buf_append(buf, p, temp_len);
  } else {
    /* Normal padding */
    apply_padding(buf, p, temp_len, spec);
  }
}

/* Format a character */
static void format_char(struct output_buffer *buf, int ch, 
                       struct format_spec *spec) {
  char str[2];
  str[0] = (char)ch;
  str[1] = '\0';
  apply_padding(buf, str, 1, spec);
}

/* ========================================================================
 * PHASE 3: ADVANCED FORMATTERS
 * ======================================================================== */

#define DEFAULT_TERMINAL_WIDTH 80

/* Format with word wrapping (column mode) - %= */
static void format_column(struct output_buffer *buf, const char *str,
                         struct format_spec *spec) {
  int width = spec->width > 0 ? spec->width : DEFAULT_TERMINAL_WIDTH;
  const char *p = str;
  const char *word_start;
  int line_len = 0;
  int word_len;
  int first_line = 1;
  
  while (*p) {
    /* Skip leading whitespace at start of line */
    while (*p == ' ' || *p == '\t') p++;
    if (!*p) break;
    
    /* Find end of word */
    word_start = p;
    while (*p && *p != ' ' && *p != '\t' && *p != '\n') p++;
    word_len = p - word_start;
    
    /* Check if word fits on current line */
    if (line_len > 0 && line_len + 1 + word_len > width) {
      /* Pad rest of line and start new one */
      while (line_len < width) {
        buf_append_char(buf, ' ');
        line_len++;
      }
      buf_append_char(buf, '\n');
      line_len = 0;
      first_line = 0;
    }
    
    /* Add space before word if not at start of line */
    if (line_len > 0) {
      buf_append_char(buf, ' ');
      line_len++;
    }
    
    /* Add the word */
    buf_append(buf, word_start, word_len);
    line_len += word_len;
  }
  
  /* Pad final line */
  while (line_len < width) {
    buf_append_char(buf, ' ');
    line_len++;
  }
}

/* Format as table (multi-column mode) - %# */
static void format_table(struct output_buffer *buf, const char *str,
                        struct format_spec *spec) {
  int width = spec->width > 0 ? spec->width : DEFAULT_TERMINAL_WIDTH;
  int num_cols = spec->precision > 0 ? spec->precision : 2;  /* default 2 columns */
  
  /* Split input on newlines */
  const char *lines[256];  /* Max 256 items */
  int line_count = 0;
  const char *p = str;
  const char *line_start = str;
  
  while (*p && line_count < 256) {
    if (*p == '\n') {
      lines[line_count++] = line_start;
      line_start = p + 1;
    }
    p++;
  }
  if (line_start < p && line_count < 256) {
    lines[line_count++] = line_start;
  }
  
  if (line_count == 0) return;
  
  /* Calculate column width */
  int col_width = width / num_cols;
  int num_rows = (line_count + num_cols - 1) / num_cols;
  
  /* Output table row by row */
  int i, col, item_idx;
  for (i = 0; i < num_rows; i++) {
    for (col = 0; col < num_cols; col++) {
      item_idx = i + col * num_rows;  /* Column-major order */
      
      if (item_idx < line_count) {
        /* Find length of this line */
        const char *line_end = lines[item_idx];
        while (*line_end && *line_end != '\n') line_end++;
        int len = line_end - lines[item_idx];
        
        /* Output the text */
        buf_append(buf, lines[item_idx], len < col_width ? len : col_width);
        
        /* Pad to column width */
        int pad = col_width - (len < col_width ? len : col_width);
        while (pad > 0) {
          buf_append_char(buf, ' ');
          pad--;
        }
      } else {
        /* Empty cell - just spaces */
        int pad = col_width;
        while (pad > 0) {
          buf_append_char(buf, ' ');
          pad--;
        }
      }
    }
    if (i < num_rows - 1) {
      buf_append_char(buf, '\n');
    }
  }
}

/* Format with justified spacing - %$ */
static void format_justify(struct output_buffer *buf, const char *str,
                          struct format_spec *spec) {
  int width = spec->width > 0 ? spec->width : DEFAULT_TERMINAL_WIDTH;
  
  /* Count words and total text length */
  const char *words[128];  /* Max 128 words */
  int word_lens[128];
  int word_count = 0;
  int text_len = 0;
  const char *p = str;
  
  while (*p && word_count < 128) {
    /* Skip whitespace */
    while (*p == ' ' || *p == '\t') p++;
    if (!*p) break;
    
    /* Record word */
    words[word_count] = p;
    while (*p && *p != ' ' && *p != '\t') p++;
    word_lens[word_count] = p - words[word_count];
    text_len += word_lens[word_count];
    word_count++;
  }
  
  if (word_count == 0) return;
  if (word_count == 1) {
    /* Single word - just left align */
    buf_append(buf, words[0], word_lens[0]);
    int pad = width - word_lens[0];
    while (pad > 0) {
      buf_append_char(buf, ' ');
      pad--;
    }
    return;
  }
  
  /* Calculate spacing */
  int total_spaces = width - text_len;
  int gaps = word_count - 1;
  int space_per_gap = total_spaces / gaps;
  int extra_spaces = total_spaces % gaps;
  
  /* Output justified text */
  int i, j;
  for (i = 0; i < word_count; i++) {
    buf_append(buf, words[i], word_lens[i]);
    
    if (i < word_count - 1) {
      /* Add spaces between words */
      int spaces = space_per_gap;
      if (i < extra_spaces) spaces++;  /* Distribute extra spaces to first gaps */
      
      for (j = 0; j < spaces; j++) {
        buf_append_char(buf, ' ');
      }
    }
  }
}

/* ========================================================================
 * MAIN SPRINTF IMPLEMENTATION
 * ======================================================================== */

int s_sprintf(struct object *caller, struct object *obj, 
              struct object *player, struct var_stack **rts) {
  struct var num_args_var, format_var, result;
  struct var *args = NULL;
  int num_args, num_format_args;
  int i, arg_index;
  const char *fmt, *p;
  struct output_buffer *buf;
  struct format_spec spec;
  
  /* Pop NUM_ARGS */
  if (pop(&num_args_var, rts, obj)) return 1;
  if (num_args_var.type != NUM_ARGS) {
    clear_var(&num_args_var);
    return 1;
  }
  num_args = num_args_var.value.num;
  
  /* Need at least 1 arg (format string) */
  if (num_args < 1) {
    result.type = INTEGER;
    result.value.integer = 0;
    push(&result, rts);
    return 0;
  }
  
  num_format_args = num_args - 1;  /* Args to format (excluding format string) */
  
  /* Pop all format arguments into array */
  if (num_format_args > 0) {
    args = MALLOC(num_format_args * sizeof(struct var));
    for (i = num_format_args - 1; i >= 0; i--) {
      if (pop(&args[i], rts, obj)) {
        FREE(args);
        return 1;
      }
      /* Resolve references */
      if (args[i].type == LOCAL_L_VALUE || args[i].type == GLOBAL_L_VALUE ||
          args[i].type == LOCAL_REF || args[i].type == GLOBAL_REF) {
        if (resolve_var(&args[i], obj)) {
          /* Clean up on error */
          int j;
          for (j = 0; j <= i; j++) {
            clear_var(&args[j]);
          }
          FREE(args);
          return 1;
        }
      }
    }
  }
  
  /* Pop format string */
  if (pop(&format_var, rts, obj)) {
    if (args) {
      for (i = 0; i < num_format_args; i++) {
        clear_var(&args[i]);
      }
      FREE(args);
    }
    return 1;
  }
  
  /* Resolve format string reference */
  if (format_var.type == LOCAL_L_VALUE || format_var.type == GLOBAL_L_VALUE ||
      format_var.type == LOCAL_REF || format_var.type == GLOBAL_REF) {
    if (resolve_var(&format_var, obj)) {
      if (args) {
        for (i = 0; i < num_format_args; i++) {
          clear_var(&args[i]);
        }
        FREE(args);
      }
      return 1;
    }
  }
  
  /* Handle INTEGER 0 as empty string */
  if (format_var.type == INTEGER && format_var.value.integer == 0) {
    format_var.type = STRING;
    *(format_var.value.string = MALLOC(1)) = '\0';
  }
  
  if (format_var.type != STRING) {
    clear_var(&format_var);
    if (args) {
      for (i = 0; i < num_format_args; i++) {
        clear_var(&args[i]);
      }
      FREE(args);
    }
    return 1;
  }
  
  /* Create output buffer */
  buf = buf_create();
  fmt = format_var.value.string;
  p = fmt;
  arg_index = 0;
  
  /* Process format string */
  while (*p) {
    if (*p == '%') {
      const char *spec_end = parse_spec(p, &spec);
      
      if (!spec_end) {
        /* Invalid format spec - output literal % */
        buf_append_char(buf, '%');
        p++;
        continue;
      }
      
      /* Handle %% (literal percent) */
      if (spec.type == '%') {
        buf_append_char(buf, '%');
        p = spec_end;
        continue;
      }
      
      /* Handle dynamic width (%*) */
      if (spec.width == -1) {
        if (arg_index < num_format_args && args[arg_index].type == INTEGER) {
          spec.width = (int)args[arg_index].value.integer;
          arg_index++;
        } else {
          spec.width = 0;
        }
      }
      
      /* Handle dynamic precision (%.*) */
      if (spec.precision == -2) {
        if (arg_index < num_format_args && args[arg_index].type == INTEGER) {
          spec.precision = (int)args[arg_index].value.integer;
          arg_index++;
        } else {
          spec.precision = 0;
        }
      }
      
      /* Check if we have enough arguments */
      if (arg_index >= num_format_args) {
        /* Not enough args - output placeholder */
        buf_append(buf, "<?>", 3);
        p = spec_end;
        continue;
      }
      
      /* Handle %@ array iteration */
      if (spec.flags & FLAG_ARRAY_ITER) {
        if (args[arg_index].type == ARRAY && args[arg_index].value.array_ptr) {
          struct heap_array *arr = args[arg_index].value.array_ptr;
          int i;
          struct format_spec elem_spec = spec;
          elem_spec.flags &= ~FLAG_ARRAY_ITER;  /* Remove @ flag for elements */
          
          for (i = 0; i < arr->size; i++) {
            /* Apply format to each element based on spec type */
            switch (spec.type) {
              case 's': {
                if (arr->elements[i].type == STRING) {
                  format_string(buf, arr->elements[i].value.string, &elem_spec);
                } else if (arr->elements[i].type == INTEGER && arr->elements[i].value.integer == 0) {
                  format_string(buf, "", &elem_spec);
                } else if (arr->elements[i].type == INTEGER) {
                  char temp[32];
                  sprintf(temp, "%ld", arr->elements[i].value.integer);
                  format_string(buf, temp, &elem_spec);
                }
                break;
              }
              case 'd':
              case 'i': {
                if (arr->elements[i].type == INTEGER) {
                  format_integer(buf, arr->elements[i].value.integer, &elem_spec, 10, 0);
                }
                break;
              }
              case 'o': {
                if (arr->elements[i].type == INTEGER) {
                  format_integer(buf, arr->elements[i].value.integer, &elem_spec, 8, 0);
                }
                break;
              }
              case 'x': {
                if (arr->elements[i].type == INTEGER) {
                  format_integer(buf, arr->elements[i].value.integer, &elem_spec, 16, 0);
                }
                break;
              }
              case 'X': {
                if (arr->elements[i].type == INTEGER) {
                  format_integer(buf, arr->elements[i].value.integer, &elem_spec, 16, 1);
                }
                break;
              }
              default:
                buf_append(buf, "<?>", 3);
                break;
            }
          }
        } else {
          buf_append(buf, "<not array>", 11);
        }
        arg_index++;
        p = spec_end;
        continue;
      }
      
      /* Format based on type */
      switch (spec.type) {
        case 's': {
          /* String */
          const char *str_val;
          char temp[32];
          
          /* Get string value */
          if (args[arg_index].type == INTEGER && args[arg_index].value.integer == 0) {
            str_val = "";
          } else if (args[arg_index].type == STRING) {
            str_val = args[arg_index].value.string;
          } else if (args[arg_index].type == INTEGER) {
            sprintf(temp, "%ld", args[arg_index].value.integer);
            str_val = temp;
          } else {
            str_val = "<?>";
          }
          
          /* Apply Phase 3 formatters if flags set */
          if (spec.flags & FLAG_COLUMN) {
            /* Column mode (word wrapping) */
            format_column(buf, str_val, &spec);
          } else if (spec.flags & FLAG_HASH) {
            /* Table mode (multi-column layout) */
            format_table(buf, str_val, &spec);
          } else if (spec.flags & FLAG_JUSTIFY) {
            /* Justify mode (even spacing) */
            format_justify(buf, str_val, &spec);
          } else {
            /* Normal string formatting */
            format_string(buf, str_val, &spec);
          }
          
          arg_index++;
          break;
        }
        
        case 'd':
        case 'i': {
          /* Decimal integer */
          long val = 0;
          if (args[arg_index].type == INTEGER) {
            val = args[arg_index].value.integer;
          }
          format_integer(buf, val, &spec, 10, 0);
          arg_index++;
          break;
        }
        
        case 'o': {
          /* Octal */
          long val = 0;
          if (args[arg_index].type == INTEGER) {
            val = args[arg_index].value.integer;
          }
          format_integer(buf, val, &spec, 8, 0);
          arg_index++;
          break;
        }
        
        case 'x': {
          /* Hex (lowercase) */
          long val = 0;
          if (args[arg_index].type == INTEGER) {
            val = args[arg_index].value.integer;
          }
          format_integer(buf, val, &spec, 16, 0);
          arg_index++;
          break;
        }
        
        case 'X': {
          /* Hex (uppercase) */
          long val = 0;
          if (args[arg_index].type == INTEGER) {
            val = args[arg_index].value.integer;
          }
          format_integer(buf, val, &spec, 16, 1);
          arg_index++;
          break;
        }
        
        case 'c': {
          /* Character */
          int ch = 0;
          if (args[arg_index].type == INTEGER) {
            ch = (int)args[arg_index].value.integer;
          }
          format_char(buf, ch, &spec);
          arg_index++;
          break;
        }
        
        case 'O': {
          /* LPC value pretty-print (arrays, mappings, objects, etc.) */
          if (args[arg_index].type == OBJECT) {
            /* Special formatting for objects */
            struct object *obj = args[arg_index].value.objptr;
            char obj_buf[512];
            
            if (!obj) {
              format_string(buf, "0", &spec);
            } else if (obj->flags & PROTOTYPE) {
              /* Prototype object: [pathname] */
              snprintf(obj_buf, sizeof(obj_buf), "[%s]", 
                       obj->parent ? obj->parent->pathname : "?");
              format_string(buf, obj_buf, &spec);
            } else {
              /* Instance object: pathname#refno */
              snprintf(obj_buf, sizeof(obj_buf), "%s#%ld",
                       obj->parent ? obj->parent->pathname : "?",
                       (long)obj->refno);
              format_string(buf, obj_buf, &spec);
            }
          } else {
            /* Other types: use save_value_internal */
            char *value_str = save_value_internal(&args[arg_index], 0);
            if (value_str) {
              format_string(buf, value_str, &spec);
              FREE(value_str);
            } else {
              format_string(buf, "<?>", &spec);
            }
          }
          arg_index++;
          break;
        }
        
        default:
          /* Unknown format specifier - output literal */
          buf_append_char(buf, '%');
          buf_append_char(buf, spec.type);
          arg_index++;
          break;
      }
      
      p = spec_end;
    } else {
      /* Literal character */
      buf_append_char(buf, *p);
      p++;
    }
  }
  
  /* Clean up arguments */
  if (args) {
    for (i = 0; i < num_format_args; i++) {
      clear_var(&args[i]);
    }
    FREE(args);
  }
  clear_var(&format_var);
  
  /* Finalize result */
  result.type = STRING;
  result.value.string = buf_finalize(buf);
  
  /* Handle empty string -> INTEGER 0 convention */
  if (result.value.string[0] == '\0') {
    FREE(result.value.string);
    result.type = INTEGER;
    result.value.integer = 0;
  }
  
  push(&result, rts);
  return 0;
}
