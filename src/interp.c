/* interp.c */

#include "config.h"
#include "object.h"
#include "constrct.h"
#include "interp.h"
#include "instr.h"
#include "protos.h"
#include "globals.h"
#include "cache.h"
#include "file.h"
#include "intrface.h"

int (*oper_array[NUM_OPERS+NUM_SCALLS])(struct object *caller, struct object
                                        *obj, struct object *player, struct
                                        var_stack **rts)={
  comma_oper,eq_oper,pleq_oper,mieq_oper,mueq_oper,dieq_oper,moeq_oper,
  aneq_oper,exeq_oper,oreq_oper,lseq_oper,rseq_oper,cond_oper,or_oper,
  and_oper,bitor_oper,exor_oper,bitand_oper,condeq_oper,noteq_oper,less_oper,
  lesseq_oper,great_oper,greateq_oper,ls_oper,rs_oper,add_oper,min_oper,
  mul_oper,div_oper,mod_oper,not_oper,bitnot_oper,postadd_oper,preadd_oper,
  postmin_oper,premin_oper,umin_oper,s_add_verb,s_add_xverb,s_call_other,
  s_alarm,s_remove_alarm,s_caller_object,s_clone_object,s_destruct,
  s_contents,s_next_object,s_location,s_next_child,s_parent,s_next_proto,
  s_move_object,s_this_object,s_this_player,s_set_interactive,
  s_interactive,s_set_priv,s_priv,s_in_editor,s_connected,s_get_devconn,
  s_send_device,s_reconnect_device,s_disconnect_device,s_random,s_time,
  s_mktime,s_typeof,s_command,s_compile_object,s_edit,s_cat,s_ls,s_rm,s_cp,
  s_mv,s_mkdir,s_rmdir,s_hide,s_unhide,s_chown,s_syslog,s_sscanf,s_sprintf,
  s_midstr,s_strlen,s_leftstr,s_rightstr,s_subst,s_instr,s_otoa,s_itoa,
  s_atoi,s_atoo,s_upcase,s_downcase,s_is_legal,s_otoi,s_itoo,s_chmod,
  s_fread,s_fwrite,s_remove_verb,s_ferase,s_chr,s_asc,s_sysctl,
  s_prototype,s_iterate,s_next_who,s_get_devidle,s_get_conntime,
  s_connect_device,s_flush_device,s_attach,s_this_component,s_detach,
  s_table_get,s_table_set,s_table_delete,s_fstat,s_fowner,s_get_hostname,
  s_get_address,s_set_localverbs,s_localverbs,s_next_verb,s_get_devport,
  s_get_devnet,s_redirect_input,s_get_input_func,s_get_master,s_is_master,
  s_input_to,s_sizeof,s_implode,s_explode,s_member_array,s_sort_array,
  s_reverse,s_unique_array,s_array_literal };

void interp_error(char *msg, struct object *player, struct object *obj,
                  struct fns *func, unsigned long line) {
  char *buf;

  buf=MALLOC(strlen(obj->parent->pathname)+strlen(msg)+(2*ITOA_BUFSIZ)+20);
  sprintf(buf," interp: %s#%ld line #%ld: %s",obj->parent->pathname,
          (long) obj->refno,(long) line,msg);
  logger(LOG_ERROR, buf);
  if (player) {
    send_device(player,buf+1);
    send_device(player,"\n");
  }
  FREE(buf);
}

/* Helper function to read a specific line from a source file
 * Returns allocated string with the line content, or NULL if file can't be read.
 * Caller must FREE the returned string.
 */
static char *read_source_line(char *filepath, unsigned long line_num) {
  FILE *f;
  char *line_buf;
  char *full_path;
  char *log_buf;
  unsigned long current_line;
  int c, pos;
  
  /* Build full filesystem path: fs_path + virtual_path + .c extension */
  full_path = MALLOC(strlen(fs_path) + strlen(filepath) + 3);  /* +3 for ".c" and null */
  strcpy(full_path, fs_path);
  strcat(full_path, filepath);
  strcat(full_path, ".c");  /* Add .c extension */
  
  /* Try to open the source file */
  f = fopen(full_path, "r");
  
  if (!f) {
    /* Log the failure with the attempted path */
    log_buf = MALLOC(strlen(full_path) + 100);
    sprintf(log_buf, " traceback: failed to read source file: %s", full_path);
    logger(LOG_WARNING, log_buf);
    FREE(log_buf);
    FREE(full_path);
    return NULL;  /* File doesn't exist or can't be read */
  }
  
  FREE(full_path);
  
  /* Allocate buffer for line (max 512 chars) */
  line_buf = MALLOC(512);
  current_line = 1;
  pos = 0;
  
  /* Read file line by line until we reach the target line */
  while ((c = fgetc(f)) != EOF) {
    if (current_line == line_num) {
      /* We're on the target line - collect characters */
      if (c == '\n' || c == '\r') {
        /* End of line reached */
        line_buf[pos] = '\0';
        fclose(f);
        return line_buf;
      } else if (pos < 511) {
        /* Add character to buffer (leave room for null terminator) */
        line_buf[pos++] = c;
      }
    } else if (c == '\n') {
      /* Move to next line */
      current_line++;
      if (current_line > line_num) {
        /* We've passed the target line without finding it */
        break;
      }
    }
  }
  
  /* If we're on the target line but hit EOF, null-terminate and return */
  if (current_line == line_num && pos > 0) {
    line_buf[pos] = '\0';
    fclose(f);
    return line_buf;
  }
  
  /* Line not found or file too short */
  FREE(line_buf);
  fclose(f);
  return NULL;
}

/* Enhanced error reporting with full call stack trace
 * 
 * ERROR REPORTING FORMAT (Detailed):
 * This function generates LPMud-style error tracebacks showing the complete
 * call chain from the error point back to the root caller.
 * 
 * Output format:
 *   interp: /path/to/file.c#123 line #45: error message
 *   Backtrace (most recent call first):
 *     [0] /path/to/file.c#123:45 in function_name()
 *     [1] /path/to/caller.c#456:78 in caller_function()
 *     ... (trace truncated at N frames)
 * 
 * The traceback walks the call_stack linked list backward from the current
 * frame to the root, displaying up to max_trace_depth frames.
 * 
 * Frame numbering: [0] is the most recent (where error occurred)
 * Each frame shows: file path, object ID, line number, function name
 */
void interp_error_with_trace(char *msg, struct object *player, struct object *obj,
                              struct fns *func, unsigned long line) {
  char *buf;
  struct call_frame *frame;
  int depth;
  
  /* Report the error header with file, object, line, and message */
  buf = MALLOC(strlen(obj->parent->pathname) + strlen(msg) + (2 * ITOA_BUFSIZ) + 100);
  sprintf(buf, " interp: %s#%ld line #%ld: %s", 
          obj->parent->pathname, (long)obj->refno, (long)line, msg);
  logger(LOG_ERROR, buf);
  if (player) {
    send_device(player, buf + 1);  /* Skip leading space for player output */
    send_device(player, "\n");
  }
  FREE(buf);
  
  /* BACKTRACE GENERATION
   * Walk the call stack linked list to build the traceback.
   * Start at call_stack (current frame) and follow prev pointers.
   * Stop at NULL or when max_trace_depth is reached.
   */
  if (call_stack) {
    buf = MALLOC(256);
    sprintf(buf, "Backtrace (most recent call first):");
    logger(LOG_ERROR, buf);
    if (player) {
      send_device(player, buf);
      send_device(player, "\n");
    }
    FREE(buf);
    
    frame = call_stack;
    depth = 0;
    
    /* Walk backward through call frames */
    while (frame && depth < max_trace_depth) {
      char *func_name = frame->func ? frame->func->funcname : "<unknown>";
      char *pathname = frame->obj->parent->pathname;
      char *source_line;
      
      /* Format: [depth] file#obj:line in function() */
      buf = MALLOC(strlen(pathname) + strlen(func_name) + (2 * ITOA_BUFSIZ) + 100);
      sprintf(buf, "  [%d] %s#%ld:%ld in %s()",
              depth, pathname, (long)frame->obj->refno, (long)frame->line, func_name);
      logger(LOG_ERROR, buf);
      if (player) {
        send_device(player, buf);
        send_device(player, "\n");
      }
      FREE(buf);
      
      /* Try to read and display the source line */
      source_line = read_source_line(pathname, frame->line);
      if (source_line) {
        /* Trim leading whitespace for display */
        char *trimmed = source_line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        
        buf = MALLOC(strlen(trimmed) + ITOA_BUFSIZ + 20);
        sprintf(buf, "      Line %ld: %s", (long)frame->line, trimmed);
        logger(LOG_ERROR, buf);
        if (player) {
          send_device(player, buf);
          send_device(player, "\n");
        }
        FREE(buf);
        FREE(source_line);
      }
      
      frame = frame->prev;
      depth++;
    }
    
    /* Indicate if trace was truncated */
    if (frame) {
      buf = MALLOC(64);
      sprintf(buf, "  ... (trace truncated at %d frames)", max_trace_depth);
      logger(LOG_ERROR, buf);
      if (player) {
        send_device(player, buf);
        send_device(player, "\n");
      }
      FREE(buf);
    }
  }
}

/* Compact error reporting format - single line per frame
 * 
 * ERROR REPORTING FORMAT (Compact):
 * Alternative format with reduced verbosity for production environments.
 * 
 * Output format:
 *   Error: error message at /path/to/file.c#123:45
 *     /path/to/file.c#123:45 in function_name()
 *     /path/to/caller.c#456:78 in caller_function()
 * 
 * Differences from detailed format:
 * - No frame numbering
 * - No "Backtrace:" header
 * - Single-line error header
 * - More concise output
 * 
 * Useful for production logs where verbosity should be minimized.
 */
void interp_error_compact(char *msg, struct object *player, struct object *obj,
                          struct fns *func, unsigned long line) {
  char *buf;
  struct call_frame *frame;
  int depth;
  
  /* Single-line error header */
  buf = MALLOC(strlen(obj->parent->pathname) + strlen(msg) + (2 * ITOA_BUFSIZ) + 50);
  sprintf(buf, " Error: %s at %s#%ld:%ld", 
          msg, obj->parent->pathname, (long)obj->refno, (long)line);
  logger(LOG_ERROR, buf);
  if (player) {
    send_device(player, buf + 1);
    send_device(player, "\n");
  }
  FREE(buf);
  
  /* Compact backtrace - one line per frame */
  frame = call_stack;
  depth = 0;
  
  while (frame && depth < max_trace_depth) {
    char *func_name = frame->func ? frame->func->funcname : "?";
    char *pathname = frame->obj->parent->pathname;
    char *source_line;
    
    buf = MALLOC(strlen(pathname) + strlen(func_name) + (2 * ITOA_BUFSIZ) + 50);
    sprintf(buf, "  %s#%ld:%ld in %s()",
            pathname, (long)frame->obj->refno, (long)frame->line, func_name);
    logger(LOG_ERROR, buf);
    if (player) {
      send_device(player, buf);
      send_device(player, "\n");
    }
    FREE(buf);
    
    /* Try to read and display the source line */
    source_line = read_source_line(pathname, frame->line);
    if (source_line) {
      /* Trim leading whitespace for display */
      char *trimmed = source_line;
      while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
      
      buf = MALLOC(strlen(trimmed) + ITOA_BUFSIZ + 20);
      sprintf(buf, "      Line %ld: %s", (long)frame->line, trimmed);
      logger(LOG_ERROR, buf);
      if (player) {
        send_device(player, buf);
        send_device(player, "\n");
      }
      FREE(buf);
      FREE(source_line);
    }
    
    frame = frame->prev;
    depth++;
  }
}

void clear_locals() {
  int loop;

  if (!num_locals) return;
  loop=0;
  while (loop<num_locals) {
    clear_var(&(locals[loop]));
    loop++;
  }
  FREE(locals);
}

struct fns *find_fns(char *name, struct object *obj) {
  struct fns *next;

  next=obj->parent->funcs->func_list;
  while (next) {
    if (!strcmp(next->funcname,name))
      return next;
    next=next->next;
  }
  return NULL;
}

struct fns *find_function(char *name, struct object *obj,
                          struct object **real_obj) {
  struct fns *tmpfns;
  struct attach_list *curr_attach;

  tmpfns=find_fns(name,obj);
  if (tmpfns) {
    if (real_obj) (*real_obj)=obj;
    return tmpfns;
  }
  curr_attach=obj->attachees;
  while (curr_attach) {
    if ((tmpfns=find_function(name,curr_attach->attachee,real_obj)))
      return tmpfns;
    curr_attach=curr_attach->next;
  }
  return NULL;
}

struct fns *find_extern_function(char *name, struct object *obj,
                                 struct object **real_obj) {
  struct fns *tmpfns;
  struct attach_list *curr_attach;

  curr_attach=obj->attachees;
  while (curr_attach) {
    if ((tmpfns=find_function(name,curr_attach->attachee,real_obj)))
      return tmpfns;
    curr_attach=curr_attach->next;
  }
  return NULL;
}

int interp(struct object *caller, struct object *obj, struct object *player,
           struct var_stack **arg_stack, struct fns *func) {
  struct var_stack *rts,*stack1;
  struct var tmp,tmp2;
  struct fns *temp_fns;
  unsigned long loop,line;
  unsigned int old_num_locals;
  struct var *old_locals;
  int retstatus;
  int old_use_soft_cycles,old_use_hard_cycles;
  struct object *tmpobj;
  
  /* CALL FRAME LIFECYCLE - PUSH
   * Stack-allocate a call frame for this function execution.
   * The frame is automatically cleaned up when the function returns.
   * This provides zero-overhead call tracking for error reporting.
   */
  struct call_frame frame;

  if (caller) while (caller->attacher) caller=caller->attacher;
  
  /* Initialize call frame with execution context:
   * - obj: the object whose code is executing
   * - func: the function being executed
   * - line: current line number (starts at 0, updated by NEW_LINE instructions)
   * - prev: pointer to caller's frame (forms linked list for traceback)
   */
  frame.obj = obj;
  frame.func = func;
  frame.line = 0;
  frame.prev = call_stack;
  
  /* Push frame onto global call stack by updating the head pointer.
   * The stack grows downward in memory (newer frames point to older ones).
   */
  call_stack = &frame;
  call_stack_depth++;
  
  /* STACK OVERFLOW PROTECTION
   * Check depth immediately after push to prevent infinite recursion.
   * If limit exceeded, pop the frame and return gracefully with error.
   * This prevents stack exhaustion and system crashes.
   */
  if (call_stack_depth > max_call_stack_depth) {
    call_stack = frame.prev;  /* Pop frame before reporting error */
    call_stack_depth--;
    interp_error_with_trace("call stack overflow - recursion too deep", player, obj, func, 0);
    tmp.type = INTEGER;
    tmp.value.integer = 0;
    pushnocopy(&tmp, arg_stack);
    return 0;
  }
  
  old_use_soft_cycles=use_soft_cycles;
  old_use_hard_cycles=use_hard_cycles;
  load_data(obj);
  rts=NULL;
  stack1=NULL;
  line=0;
  num_locals=func->num_locals;
  loop=0;
  if (num_locals)
    locals=(struct var *) MALLOC(sizeof(struct var)*num_locals);
  else
    locals=NULL;
  
  /* Initialize locals - allocate arrays for array variables */
  while (loop<num_locals) {
    struct var_tab *var_info = func->lst;
    struct heap_array *arr;
    int is_array = 0;
    unsigned int array_size = 0;
    unsigned int max_size = 0;
    char logbuf[256];
    
    sprintf(logbuf, "init_locals: checking local %d, lst=%p", loop, (void*)func->lst);
    logger(LOG_DEBUG, logbuf);
    
    /* Find this variable in the local symbol table */
    while (var_info) {
      sprintf(logbuf, "init_locals: var_info base=%u, loop=%d, array=%p, is_mapping=%d", 
              var_info->base, loop, (void*)var_info->array, var_info->is_mapping);
      logger(LOG_DEBUG, logbuf);
      
      /* Skip mappings - they don't get pre-allocated */
      if (var_info->base == loop && var_info->is_mapping) {
        sprintf(logbuf, "init_locals: local %d is a mapping, skipping pre-allocation", loop);
        logger(LOG_DEBUG, logbuf);
        break;
      }
      
      if (var_info->base == loop && var_info->array) {
        is_array = 1;
        /* Calculate total array size (product of all dimensions) */
        struct array_size *dim = var_info->array;
        array_size = 1;
        while (dim) {
          if (dim->size == 255) {
            /* Unlimited size - start with 0 elements, will grow on access */
            array_size = 0;
            break;
          }
          array_size *= dim->size;
          dim = dim->next;
        }
        /* Determine max_size (255 means unlimited) */
        max_size = (var_info->array->size == 255) ? UNLIMITED_ARRAY_SIZE : array_size;
        
        sprintf(logbuf, "init_locals: found array at local %d, size=%u, max=%u", 
                loop, array_size, max_size);
        logger(LOG_DEBUG, logbuf);
        break;
      }
      var_info = var_info->next;
    }
    
    if (is_array) {
      /* Allocate heap array */
      arr = allocate_array(array_size, max_size);
      if (arr) {
        locals[loop].type = ARRAY;
        locals[loop].value.array_ptr = arr;
        sprintf(logbuf, "init_locals: allocated array for local %d", loop);
        logger(LOG_DEBUG, logbuf);
      } else {
        /* Allocation failed - initialize as INTEGER 0 */
        locals[loop].type = INTEGER;
        locals[loop].value.integer = 0;
        logger(LOG_ERROR, "init_locals: array allocation failed!");
      }
    } else {
      /* Regular variable - initialize to INTEGER 0 */
      locals[loop].type = INTEGER;
      locals[loop].value.integer = 0;
    }
    loop++;
  }
  if (pop(&tmp,arg_stack,caller)) {
    interp_error_with_trace("malformed argument stack",player,obj,func,0);
    free_stack(arg_stack);
    call_stack = frame.prev;  /* Pop frame on error exit */
    call_stack_depth--;
    return 1;
  }
  if (tmp.type!=NUM_ARGS) {
    interp_error_with_trace("malformed argument stack",player,obj,func,0);
    free_stack(arg_stack);
    call_stack = frame.prev;  /* Pop frame on error exit */
    call_stack_depth--;
    return 1;
  }
  if (tmp.value.num>num_locals) {
    interp_error_with_trace("too many arguments",player,obj,func,0);
    call_stack = frame.prev;  /* Pop frame on error exit */
    call_stack_depth--;
    return 1;
  }
  loop=tmp.value.num;
  while (loop>0) {
    if (pop(&tmp,arg_stack,caller)) {
      interp_error_with_trace("malformed argument stack",player,obj,func,0);
      free_stack(arg_stack);
      clear_locals();
      call_stack = frame.prev;  /* Pop frame on error exit */
      call_stack_depth--;
      return 1;
    }
    --loop;
    copy_var(&(locals[loop]),&tmp);
    clear_var(&tmp);
  }
  loop=0;
  while (1) {

#ifdef CYCLE_HARD_MAX
    if (use_hard_cycles)
      if (hard_cycles++>CYCLE_HARD_MAX) {
        tmp.type=INTEGER;
        tmp.value.integer=0;
        interp_error_with_trace("cycle hard maximum exceeded",player,obj,func,line);
        pushnocopy(&tmp,arg_stack);
        clear_locals();
        call_stack = frame.prev;  /* Pop frame on exit */
        call_stack_depth--;
        return 0;
      }
#endif /* CYCLE_HARD_MAX */

#ifdef CYCLE_SOFT_MAX
    if (use_soft_cycles)
      if (soft_cycles++>CYCLE_SOFT_MAX) {
        tmp.type=INTEGER;
        tmp.value.integer=0;
        interp_error_with_trace("cycle soft maximum exceeded",player,obj,func,line);
        pushnocopy(&tmp,arg_stack);
        clear_locals();
        call_stack = frame.prev;  /* Pop frame on exit */
        call_stack_depth--;
        return 0;
      }
#endif /* CYCLE_SOFT_MAX */

    switch (func->code[loop].type) {
      case INTEGER:
      case STRING:
      case OBJECT:
      case GLOBAL_L_VALUE:
      case LOCAL_L_VALUE:
      case NUM_ARGS:
      case ARRAY_SIZE:
        push(&(func->code[loop]),&rts);
        loop++;
        break;
      case LOCAL_REF:
      case GLOBAL_REF:
        {
          unsigned int var_index, declared_size;
          unsigned char is_global;
          struct var *var_slot;
          struct var key_var;  /* Changed: key can be any type */
          struct var tmp2;
          char logbuf[256];
          
          /* Stack has: [base] [key] [size] */
          
          /* Pop declared size */
          if (popint(&tmp,&rts,obj)) {
            interp_error_with_trace("failed subscript reference",player,obj,func,line);
            free_stack(&rts);
            clear_locals();
            use_soft_cycles=old_use_soft_cycles;
            use_hard_cycles=old_use_hard_cycles;
            call_stack = frame.prev;
            call_stack_depth--;
            return 1;
          }
          declared_size = tmp.value.integer;
          
          /* Pop key (was array_index) - can be any type! */
          if (pop(&tmp,&rts,obj)) {  /* Changed from popint to pop */
            interp_error_with_trace("failed subscript reference",player,obj,func,line);
            free_stack(&rts);
            clear_locals();
            use_soft_cycles=old_use_soft_cycles;
            use_hard_cycles=old_use_hard_cycles;
            call_stack = frame.prev;
            call_stack_depth--;
            return 1;
          }
          key_var = tmp;  /* Save the key */
          
          /* Pop variable index (where array pointer is stored) */
          if (popint(&tmp,&rts,obj)) {
            interp_error_with_trace("failed array reference",player,obj,func,line);
            free_stack(&rts);
            clear_locals();
            use_soft_cycles=old_use_soft_cycles;
            use_hard_cycles=old_use_hard_cycles;
            call_stack = frame.prev;
            call_stack_depth--;
            return 1;
          }
          var_index = tmp.value.integer;
          
          is_global = (func->code[loop].type == GLOBAL_REF);
          
          /* Get variable slot */
          if (is_global) {
            if (var_index >= obj->parent->funcs->num_globals) {
              interp_error_with_trace("global variable index out of bounds",player,obj,func,line);
              free_stack(&rts);
              clear_locals();
              use_soft_cycles=old_use_soft_cycles;
              use_hard_cycles=old_use_hard_cycles;
              call_stack = frame.prev;
              call_stack_depth--;
              return 1;
            }
            var_slot = &obj->globals[var_index];
          } else {
            if (var_index >= num_locals) {
              interp_error_with_trace("local variable index out of bounds",player,obj,func,line);
              free_stack(&rts);
              clear_locals();
              use_soft_cycles=old_use_soft_cycles;
              use_hard_cycles=old_use_hard_cycles;
              call_stack = frame.prev;
              call_stack_depth--;
              return 1;
            }
            var_slot = &locals[var_index];
          }
          
          /* Auto-create array or mapping on first access
           * IMPORTANT: Arrays and mappings are DIFFERENT data structures:
           * - Arrays: struct heap_array with contiguous memory
           * - Mappings: struct heap_mapping with hash table
           * We check the symbol table's is_mapping flag to determine which to create
           */
          if (var_slot->type == INTEGER && var_slot->value.integer == 0) {
            /* Check symbol table to determine if this is a mapping */
            struct var_tab *var_info = is_global ? obj->parent->funcs->gst : func->lst;
            int is_mapping_var = 0;
            
            while (var_info) {
              if (var_info->base == var_index) {
                is_mapping_var = var_info->is_mapping;
                break;
              }
              var_info = var_info->next;
            }
            
            if (!is_mapping_var && declared_size > 0) {
              /* Create array */
              struct heap_array *arr;
              unsigned int max_size = (declared_size == 255) ? UNLIMITED_ARRAY_SIZE : declared_size;
              sprintf(logbuf, "Creating heap array: var_index=%u, declared_size=%u, max_size=%u, global=%d",
                      var_index, declared_size, max_size, is_global);
              logger(LOG_DEBUG, logbuf);
              
              arr = allocate_array(declared_size, max_size);
              if (!arr) {
                interp_error_with_trace("failed to allocate array",player,obj,func,line);
                free_stack(&rts);
                clear_locals();
                use_soft_cycles=old_use_soft_cycles;
                use_hard_cycles=old_use_hard_cycles;
                call_stack = frame.prev;
                call_stack_depth--;
                return 1;
              }
              
              var_slot->type = ARRAY;
              var_slot->value.array_ptr = arr;
              if (is_global) obj->obj_state = DIRTY;
            } else {
              /* Create mapping */
              struct heap_mapping *map;
              sprintf(logbuf, "Creating heap mapping: var_index=%u, global=%d", var_index, is_global);
              logger(LOG_DEBUG, logbuf);
              
              map = allocate_mapping(DEFAULT_MAPPING_CAPACITY);
              if (!map) {
                interp_error_with_trace("failed to allocate mapping",player,obj,func,line);
                free_stack(&rts);
                clear_locals();
                use_soft_cycles=old_use_soft_cycles;
                use_hard_cycles=old_use_hard_cycles;
                call_stack = frame.prev;
                call_stack_depth--;
                return 1;
              }
              
              var_slot->type = MAPPING;
              var_slot->value.mapping_ptr = map;
              if (is_global) obj->obj_state = DIRTY;
            }
          }
          
          /* Type dispatch: ARRAY or MAPPING */
          if (var_slot->type == ARRAY) {
            /* ARRAY HANDLING */
            struct heap_array *arr;
            unsigned int array_index;
            
            /* Key must be integer for arrays */
            if (key_var.type != INTEGER) {
              interp_error_with_trace("array index must be integer",player,obj,func,line);
              clear_var(&key_var);
              free_stack(&rts);
              clear_locals();
              use_soft_cycles=old_use_soft_cycles;
              use_hard_cycles=old_use_hard_cycles;
              call_stack = frame.prev;
              call_stack_depth--;
              return 1;
            }
            array_index = key_var.value.integer;
            
            arr = var_slot->value.array_ptr;
            
            /* Bounds check and resize if needed */
            if (array_index >= arr->size) {
              if (arr->max_size == UNLIMITED_ARRAY_SIZE || array_index < arr->max_size) {
                sprintf(logbuf, "Resizing array from %u to %u elements", arr->size, array_index + 1);
                logger(LOG_DEBUG, logbuf);
                
                if (resize_heap_array(arr, array_index + 1)) {
                  interp_error_with_trace("array resize failed",player,obj,func,line);
                  clear_var(&key_var);
                  free_stack(&rts);
                  clear_locals();
                  use_soft_cycles=old_use_soft_cycles;
                  use_hard_cycles=old_use_hard_cycles;
                  call_stack = frame.prev;
                  call_stack_depth--;
                  return 1;
                }
                if (is_global) obj->obj_state = DIRTY;
              } else {
                interp_error_with_trace("array index out of bounds",player,obj,func,line);
                clear_var(&key_var);
                free_stack(&rts);
                clear_locals();
                use_soft_cycles=old_use_soft_cycles;
                use_hard_cycles=old_use_hard_cycles;
                call_stack = frame.prev;
                call_stack_depth--;
                return 1;
              }
            }
            
            /* Push L_VALUE reference to array element */
            tmp2.type = (is_global) ? GLOBAL_L_VALUE : LOCAL_L_VALUE;
            tmp2.value.l_value.ref = (unsigned long)&arr->elements[array_index];
            tmp2.value.l_value.size = 1;
            
            clear_var(&key_var);
            push(&tmp2,&rts);
            
          } else if (var_slot->type == MAPPING) {
            /* MAPPING HANDLING */
            struct heap_mapping *map;
            struct var *value_ptr;
            
            map = var_slot->value.mapping_ptr;
            
            /* Get or create entry in mapping */
            value_ptr = mapping_get_or_create(map, &key_var);
            if (!value_ptr) {
              interp_error_with_trace("mapping access failed",player,obj,func,line);
              clear_var(&key_var);
              free_stack(&rts);
              clear_locals();
              use_soft_cycles=old_use_soft_cycles;
              use_hard_cycles=old_use_hard_cycles;
              call_stack = frame.prev;
              call_stack_depth--;
              return 1;
            }
            
            /* Mark as dirty if global */
            if (is_global) obj->obj_state = DIRTY;
            
            /* Push L_VALUE reference to mapping value */
            tmp2.type = (is_global) ? GLOBAL_L_VALUE : LOCAL_L_VALUE;
            tmp2.value.l_value.ref = (unsigned long)value_ptr;
            tmp2.value.l_value.size = 1;
            
            clear_var(&key_var);
            push(&tmp2,&rts);
            
          } else {
            interp_error_with_trace("not an array or mapping",player,obj,func,line);
            clear_var(&key_var);
            free_stack(&rts);
            clear_locals();
            use_soft_cycles=old_use_soft_cycles;
            use_hard_cycles=old_use_hard_cycles;
            call_stack = frame.prev;
            call_stack_depth--;
            return 1;
          }
          loop++;
        }
        break;
      case ASM_INSTR:
        if (func->code[loop].value.instruction<NUM_OPERS) {
          retstatus=((*oper_array[func->code[loop].value.instruction])
                     (caller,obj,player,&rts));
          if (retstatus) {
            interp_error_with_trace("arithmetic operation failed",player,obj,func,line);
            free_stack(&rts);
            clear_locals();
            use_soft_cycles=old_use_soft_cycles;
            use_hard_cycles=old_use_hard_cycles;
            call_stack = frame.prev;  /* Pop frame on error exit */
            call_stack_depth--;
            return 1;
          }
        } else {
          char logbuf[256];
          sprintf(logbuf, "interp: syscall #%d, rts=%p, *rts=%p", 
                  func->code[loop].value.instruction, (void*)&rts, (void*)rts);
          logger(LOG_DEBUG, logbuf);
          
          old_locals=locals;
          old_num_locals=num_locals;
          if (func->code[loop].value.instruction==S_SSCANF ||
              func->code[loop].value.instruction==S_SPRINTF ||
              func->code[loop].value.instruction==S_FREAD ||
              func->code[loop].value.instruction==S_SIZEOF)
            stack1=gen_stack_noresolve(&rts,obj);
          else
            stack1=gen_stack(&rts,obj);
          
          sprintf(logbuf, "interp: after gen_stack, stack1=%p", (void*)stack1);
          logger(LOG_DEBUG, logbuf);
          
          retstatus=((*oper_array[func->code[loop].value.instruction])
                     (caller,obj,player,&stack1));
          locals=old_locals;
          num_locals=old_num_locals;
          if (retstatus) {
            char *errbuf;
            char logbuf[300];
            
            errbuf=MALLOC(200);
            
            /* Build detailed error message with return code */
            sprintf(errbuf,"system call failed (instruction #%d, return code %d)",
                    (int) func->code[loop].value.instruction, retstatus);
            
            /* Log additional context */
            logger(LOG_ERROR, errbuf);
            
            if (func && func->funcname) {
              sprintf(logbuf, "  in function: %s, line: %lu", func->funcname, line);
              logger(LOG_ERROR, logbuf);
            }
            
            interp_error_with_trace(errbuf,player,obj,func,line);
            FREE(errbuf);
            free_stack(&rts);
            free_stack(&stack1);
            clear_locals();
            use_hard_cycles=old_use_hard_cycles;
            use_soft_cycles=old_use_soft_cycles;
            call_stack = frame.prev;  /* Pop frame on error exit */
            call_stack_depth--;
            return 1;
          }
          if (pop(&tmp,&stack1,obj)) {
            interp_error_with_trace("system call returned malformed stack",player,obj,
                         func,line);
            free_stack(&rts);
            free_stack(&stack1);
            clear_locals();
            use_hard_cycles=old_use_hard_cycles;
            use_soft_cycles=old_use_soft_cycles;
            call_stack = frame.prev;  /* Pop frame on error exit */
            call_stack_depth--;
            return 1;
          }
          pushnocopy(&tmp,&rts);
          free_stack(&stack1);
        }
        loop++;
        break;
      case FUNC_CALL:
        stack1=gen_stack(&rts,obj);
        old_locals=locals;
        old_num_locals=num_locals;
        if (interp(obj,obj,player,&stack1,func->code[loop].value.func_call)) {
          locals=old_locals;
          num_locals=old_num_locals;
          free_stack(&stack1);
          free_stack(&rts);
          clear_locals();
          use_soft_cycles=old_use_soft_cycles;
          use_hard_cycles=old_use_hard_cycles;
          call_stack = frame.prev;  /* Pop frame on error exit */
          call_stack_depth--;
          return 1;
        }
        locals=old_locals;
        num_locals=old_num_locals;
        if (pop(&tmp,&stack1,obj)) {
          interp_error_with_trace("function returned malformed stack",player,obj,func,
                       line);
          free_stack(&rts);
          free_stack(&stack1);
          clear_locals();
          use_soft_cycles=old_use_soft_cycles;
          use_hard_cycles=old_use_hard_cycles;
          call_stack = frame.prev;  /* Pop frame on error exit */
          call_stack_depth--;
          return 1;
        }
        pushnocopy(&tmp,&rts);
        free_stack(&stack1);
        loop++;
        break;
      case EXTERN_FUNC:
        temp_fns=find_extern_function(func->code[loop].value.string,obj,
                                      &tmpobj);
        if (!temp_fns) {
          interp_error_with_trace("unknown function",player,obj,func,line);
          free_stack(&rts);
          clear_locals();
          use_soft_cycles=old_use_soft_cycles;
          use_hard_cycles=old_use_hard_cycles;
          call_stack = frame.prev;  /* Pop frame on error exit */
          call_stack_depth--;
          return 1;
        }
        stack1=gen_stack(&rts,obj);
        old_locals=locals;
        old_num_locals=num_locals;
        if (interp(obj,tmpobj,player,&stack1,temp_fns)) {
          locals=old_locals;
          num_locals=old_num_locals;
          free_stack(&stack1);
          free_stack(&rts);
          clear_locals();
          use_soft_cycles=old_use_soft_cycles;
          use_hard_cycles=old_use_hard_cycles;
          call_stack = frame.prev;  /* Pop frame on error exit */
          call_stack_depth--;
          return 1;
        }
        locals=old_locals;
        num_locals=old_num_locals;
        if (pop(&tmp,&stack1,obj)) {
          interp_error_with_trace("function returned malformed stack",player,obj,func,
                       line);
          free_stack(&rts);
          free_stack(&stack1);
          clear_locals();
          use_soft_cycles=old_use_soft_cycles;
          use_hard_cycles=old_use_hard_cycles;
          call_stack = frame.prev;  /* Pop frame on error exit */
          call_stack_depth--;
          return 1;
        }
        pushnocopy(&tmp,&rts);
        free_stack(&stack1);
        loop++;
        break;
      case FUNC_NAME:
        temp_fns=find_function(func->code[loop].value.string,obj,&tmpobj);
        if (!temp_fns) {
          interp_error_with_trace("unknown function",player,obj,func,line);
          free_stack(&rts);
          clear_locals();
          use_soft_cycles=old_use_soft_cycles;
          use_hard_cycles=old_use_hard_cycles;
          call_stack = frame.prev;  /* Pop frame on error exit */
          call_stack_depth--;
          return 1;
        }
        if (tmpobj==obj) {
          clear_var(&(func->code[loop]));
          func->code[loop].type=FUNC_CALL;
          func->code[loop].value.func_call=temp_fns;
        } else {
          func->code[loop].type=EXTERN_FUNC;
        }
        break;
      case JUMP:
        loop=func->code[loop].value.num;
        break;
      case BRANCH:
        if (pop(&tmp,&rts,obj)) {
          interp_error_with_trace("failed branch instruction",player,obj,func,line);
          free_stack(&rts);
          clear_locals();
          use_soft_cycles=old_use_soft_cycles;
          use_hard_cycles=old_use_hard_cycles;
          call_stack = frame.prev;  /* Pop frame on error exit */
          call_stack_depth--;
          return 1;
        }
        if (resolve_var(&tmp,obj)) {
          interp_error_with_trace("failed variable resolution",player,obj,func,line);
          free_stack(&rts);
          clear_locals();
          use_soft_cycles=old_use_soft_cycles;
          use_hard_cycles=old_use_hard_cycles;
          call_stack = frame.prev;  /* Pop frame on error exit */
          call_stack_depth--;
          return 1;
        }
        if (tmp.type==INTEGER && tmp.value.integer==0) {
          loop=func->code[loop].value.num;
        } else {
          clear_var(&tmp);
          loop++;
        }
        break;
      case NEW_LINE:
        /* LINE NUMBER TRACKING
         * NEW_LINE instructions mark transitions to new source lines.
         * Update both the local 'line' variable (for backward compatibility)
         * and the current frame's line field (for traceback generation).
         * This provides accurate line numbers in error reports with minimal overhead.
         */
        free_stack(&rts);
        line=func->code[loop].value.num;
        frame.line = line;  /* Update current frame's line number for traceback */
        loop++;
        break;
      case RETURN:
        /* CALL FRAME LIFECYCLE - POP (Normal and Error Paths)
         * All function exits must pop the call frame to maintain stack integrity.
         * This happens on both normal returns and error paths.
         * The frame is popped by restoring call_stack to point to the previous frame.
         */
        if (pop(&tmp,&rts,obj)) {
          interp_error_with_trace("stack malformed on return",player,obj,func,line);
          free_stack(&rts);
          clear_locals();
          use_hard_cycles=old_use_hard_cycles;
          use_soft_cycles=old_use_soft_cycles;
          call_stack = frame.prev;  /* Pop frame on error exit */
          call_stack_depth--;
          return 1;
        }
        if (resolve_var(&tmp,obj)) {
          interp_error_with_trace("stack malformed on return",player,obj,func,line);
          free_stack(&rts);
          clear_locals();
          use_hard_cycles=old_use_hard_cycles;
          use_soft_cycles=old_use_soft_cycles;
          call_stack = frame.prev;  /* Pop frame on error exit */
          call_stack_depth--;
          return 1;
        }
        /* Normal return path - clean up and pop frame */
        pushnocopy(&tmp,arg_stack);
        free_stack(&rts);
        clear_locals();
        use_soft_cycles=old_use_soft_cycles;
        use_hard_cycles=old_use_hard_cycles;
        call_stack = frame.prev;  /* Pop frame on normal exit */
        call_stack_depth--;
        return 0;
        break;
    }
  }
}
