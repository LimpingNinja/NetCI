/* construct.c */

#include "config.h"
#include "object.h"
#include "instr.h"
#include "constrct.h"
#include "protos.h"
#include "globals.h"
#include "cache.h"
#include "file.h"
#include "cache.h"
#include "interp.h"  /* For struct call_frame definition */

/**
 * Validates filename for virtual filesystem
 * Modern restrictions (removed legacy 8.3 DOS format)
 * 
 * Allowed:
 * - Length: 1-255 characters (POSIX standard)
 * - Characters: alphanumeric, underscore, hyphen, dot
 * - Case: both upper and lowercase allowed
 * - Multiple dots allowed (e.g., "file.tar.gz")
 * 
 * Disallowed:
 * - Leading dot (hidden files)
 * - Trailing dot
 * - Path separators: / \
 * - Wildcards: * ?
 * - Special chars: : # ~ (filesystem unsafe)
 * - Control characters and whitespace
 */
int is_legal(char *name) {
  int x, len;

  len = strlen(name);
  
  /* Check length: must be 1-255 characters */
  if (len < 1 || len > 255) return 0;
  
  /* Cannot start with dot (hidden files) */
  if (name[0] == '.') return 0;
  
  /* Cannot end with dot */
  if (name[len - 1] == '.') return 0;
  
  /* Check each character */
  for (x = 0; x < len; x++) {
    char c = name[x];
    
    /* Allow alphanumeric (both cases) */
    if (isalnum(c)) continue;
    
    /* Allow underscore, hyphen, dot */
    if (c == '_' || c == '-' || c == '.') continue;
    
    /* Reject everything else including:
     * - Path separators: / \
     * - Wildcards: * ?
     * - Special: : # ~
     * - Whitespace and control characters
     */
    return 0;
  }
  
  return 1;
}

void free_gst(struct var_tab *gst) {
  struct var_tab *curr_var,*next_var;
  struct array_size *curr_array,*next_array;

  curr_var=gst;
  while (curr_var) {
    next_var=curr_var->next;
    curr_array=curr_var->array;
    while (curr_array) {
      next_array=curr_array->next;
      FREE(curr_array);
      curr_array=next_array;
    }
    FREE(curr_var->name);
    FREE(curr_var);
    curr_var=next_var;
  }
}

void free_code(struct code *the_code) {
  struct fns *next,*curr;
  unsigned int x;

  if (!the_code) return;
  next=the_code->func_list;
  while (next) {
    curr=next;
    next=curr->next;
    x=0;
    while (x<curr->num_instr) {
      clear_var(&(curr->code[x]));
      x++;
    }
    if (curr->code)
      FREE(curr->code);
    FREE(curr->funcname);
    FREE(curr);
  }
  free_gst(the_code->gst);
  if (the_code->ancestor_map)
    FREE(the_code->ancestor_map);
  if (the_code->gst_map)
    FREE(the_code->gst_map);
  FREE(the_code);
}

char *copy_string(char *s) {
  return strcpy((char *) MALLOC(strlen(s)+1),s);
}

void push(struct var *data, struct var_stack **rts) {
  struct var_stack *tmp;

  tmp=MALLOC(sizeof(struct var_stack));
  tmp->data.type=data->type;
  switch (data->type) {
    case STRING:
    case FUNC_NAME:
    case EXTERN_FUNC:
      tmp->data.value.string=copy_string(data->value.string);
      break;
    case INTEGER:
      tmp->data.value.integer=data->value.integer;
      break;
    case OBJECT:
      tmp->data.value.objptr=data->value.objptr;
      break;
    case ASM_INSTR:
      tmp->data.value.instruction=data->value.instruction;
      break;
    case GLOBAL_L_VALUE:
    case LOCAL_L_VALUE:
      tmp->data.value.l_value.ref=data->value.l_value.ref;
      tmp->data.value.l_value.size=data->value.l_value.size;
      break;
    case FUNC_CALL:
      tmp->data.value.func_call=data->value.func_call;
      break;
    default:
      tmp->data.value.num=data->value.num;
      break;
  }
  tmp->next=*rts;
  *rts=tmp;
}

void pushnocopy(struct var *data, struct var_stack **rts) {
  struct var_stack *tmp;

  tmp=MALLOC(sizeof(struct var_stack));
  tmp->data.type=data->type;
  switch (data->type) {
    case STRING:
    case FUNC_NAME:
    case EXTERN_FUNC:
      tmp->data.value.string=data->value.string;
      break;
    case INTEGER:
      tmp->data.value.integer=data->value.integer;
      break;
    case OBJECT:
      tmp->data.value.objptr=data->value.objptr;
      break;
    case ASM_INSTR:
      tmp->data.value.instruction=data->value.instruction;
      break;
    case GLOBAL_L_VALUE:
    case LOCAL_L_VALUE:
      tmp->data.value.l_value.ref=data->value.l_value.ref;
      tmp->data.value.l_value.size=data->value.l_value.size;
      break;
    case FUNC_CALL:
      tmp->data.value.func_call=data->value.func_call;
      break;
    default:
      tmp->data.value.num=data->value.num;
      break;
  }
  tmp->next=*rts;
  *rts=tmp;
}

int resolve_var(struct var *data, struct object *obj) {
  struct var *element_ptr;
  char logbuf[256];
  extern struct call_frame *call_stack;  /* For definer context */
  
  sprintf(logbuf, "resolve_var: type=%d", data->type);
  logger(LOG_DEBUG, logbuf);
  
  if (data->type==GLOBAL_L_VALUE) {
    sprintf(logbuf, "resolve_var: GLOBAL_L_VALUE ref=%lu, size=%u", 
            data->value.l_value.ref, data->value.l_value.size);
    logger(LOG_DEBUG, logbuf);
    
    /* Check if this is a heap array element (pointer) or regular global */
    if (data->value.l_value.ref>=obj->parent->funcs->num_globals) {
      /* This is a pointer to a heap array element */
      element_ptr = (struct var *)data->value.l_value.ref;
      data->type = element_ptr->type;
      switch (data->type) {
        case INTEGER:
          data->value.integer = element_ptr->value.integer;
          break;
        case STRING:
          data->value.string = copy_string(element_ptr->value.string);
          break;
        case OBJECT:
          data->value.objptr = element_ptr->value.objptr;
          break;
        case ARRAY:
          data->value.array_ptr = element_ptr->value.array_ptr;
          array_addref(data->value.array_ptr);  /* Increment refcount */
          break;
        case MAPPING:
          data->value.mapping_ptr = element_ptr->value.mapping_ptr;
          mapping_addref(data->value.mapping_ptr);  /* Increment refcount */
          break;
        default:
          return 1;
      }
    } else {
      /* Regular global variable - compute absolute index via GST mapping */
      int ok = 0;
      unsigned int effective_index = global_index_for(obj, call_stack ? call_stack->func : NULL,
                                                     (unsigned int)data->value.l_value.ref, &ok);
      sprintf(logbuf, "resolve_var: GLOBAL ref=%lu => effective_index=%u (ok=%d)", 
              data->value.l_value.ref, effective_index, ok);
      logger(LOG_DEBUG, logbuf);
      if (!ok) return 1;
      data->type=obj->globals[effective_index].type;
      switch (data->type) {
        case INTEGER:
          data->value.integer=obj->globals[effective_index].value.integer;
          break;
        case STRING:
          data->value.string=copy_string(obj->globals[effective_index].
                                         value.string);
          break;
        case OBJECT:
          data->value.objptr=obj->globals[effective_index].value.objptr;
          break;
        case ARRAY:
          data->value.array_ptr=obj->globals[effective_index].value.array_ptr;
          array_addref(data->value.array_ptr);  /* Increment refcount */
          break;
        case MAPPING:
          data->value.mapping_ptr=obj->globals[effective_index].value.mapping_ptr;
          mapping_addref(data->value.mapping_ptr);  /* Increment refcount */
          break;
        default:
          return 1;
          break;
      }
    }
  } else
    if (data->type==LOCAL_L_VALUE) {
      /* Check if this is a heap array element (pointer) or regular local */
      if (data->value.l_value.ref>=num_locals) {
        /* This is a pointer to a heap array element */
        element_ptr = (struct var *)data->value.l_value.ref;
        data->type = element_ptr->type;
        switch (data->type) {
          case INTEGER:
            data->value.integer = element_ptr->value.integer;
            break;
          case STRING:
            data->value.string = copy_string(element_ptr->value.string);
            break;
          case OBJECT:
            data->value.objptr = element_ptr->value.objptr;
            break;
          case ARRAY:
            data->value.array_ptr = element_ptr->value.array_ptr;
            array_addref(data->value.array_ptr);  /* Increment refcount */
            break;
          case MAPPING:
            data->value.mapping_ptr = element_ptr->value.mapping_ptr;
            mapping_addref(data->value.mapping_ptr);  /* Increment refcount */
            break;
          default:
            return 1;
        }
      } else {
        /* Regular local variable */
        data->type=locals[data->value.l_value.ref].type;
        switch (data->type) {
          case INTEGER:
            data->value.integer=locals[data->value.l_value.ref].value.integer;
            break;
          case STRING:
            data->value.string=copy_string(locals[data->value.l_value.ref].
                                           value.string);
            break;
          case OBJECT:
            data->value.objptr=locals[data->value.l_value.ref].value.objptr;
            break;
          case ARRAY:
            data->value.array_ptr=locals[data->value.l_value.ref].value.array_ptr;
            array_addref(data->value.array_ptr);  /* Increment refcount */
            break;
          case MAPPING:
            data->value.mapping_ptr=locals[data->value.l_value.ref].value.mapping_ptr;
            mapping_addref(data->value.mapping_ptr);  /* Increment refcount */
            break;
          default:
            return 1;
            break;
        }
      }
    }
  return 0;
}

int popint(struct var *data, struct var_stack **rts, struct object *obj) {
  struct var tmp;
  struct var_stack *ptr;

  ptr=*rts;
  if (ptr==NULL) return 1;
  *rts=ptr->next;
  data->type=ptr->data.type;
  switch (ptr->data.type) {
    case INTEGER:
      data->value.integer=ptr->data.value.integer;
      break;
    case STRING:
    case FUNC_NAME:
    case EXTERN_FUNC:
      data->value.string=ptr->data.value.string;
      break;
    case OBJECT:
      data->value.objptr=ptr->data.value.objptr;
      break;
    case ASM_INSTR:
      data->value.instruction=ptr->data.value.instruction;
      break;
    case GLOBAL_L_VALUE:
    case LOCAL_L_VALUE:
      data->value.l_value.ref=ptr->data.value.l_value.ref;
      data->value.l_value.size=ptr->data.value.l_value.size;
      break;
    case FUNC_CALL:
      data->value.func_call=ptr->data.value.func_call;
      break;
    case LOCAL_REF:
    case GLOBAL_REF:
      {
        unsigned int array_base, array_size, index;
        /* Pop size, index, base (new 3-value format) */
        if (popint(&tmp,rts,obj)) {
          FREE(ptr);
          return 1;
        }
        array_size = tmp.value.integer;
        if (popint(&tmp,rts,obj)) {
          FREE(ptr);
          return 1;
        }
        index = tmp.value.integer;
        if (popint(&tmp,rts,obj)) {
          FREE(ptr);
          return 1;
        }
        array_base = tmp.value.integer;
        /* Calculate final reference */
        data->value.l_value.ref = array_base + index;
        data->value.l_value.size = 1;
        if (data->type==LOCAL_REF) {
          if (data->value.l_value.ref>=num_locals) {
            FREE(ptr);
            return 1;
          }
          data->type=LOCAL_L_VALUE;
        } else {
          if (data->value.l_value.ref>=obj->parent->funcs->num_globals) {
            FREE(ptr);
            return 1;
          }
          data->type=GLOBAL_L_VALUE;
        }
      }
      break;
    default:
      data->value.num=ptr->data.value.num;
      break;
  }
  /* Allow arrays (size>1) to be popped for array operations like sizeof(), join(), etc. */
  if (resolve_var(data,obj)) {
    clear_var(data);
    FREE(ptr);
    return 1;
  }
  if (data->type!=INTEGER) {
    clear_var(data);
    FREE(ptr);
    return 1;
  }
  FREE(ptr);
  return 0;
}

int pop(struct var *data, struct var_stack **rts, struct object *obj) {
  struct var tmp;
  struct var_stack *ptr;

  ptr=*rts;
  if (ptr==NULL)
    return 1;
  *rts=ptr->next;
  data->type=ptr->data.type;
  switch (ptr->data.type) {
    case INTEGER:
      data->value.integer=ptr->data.value.integer;
      break;
    case STRING:
    case FUNC_NAME:
    case EXTERN_FUNC:
      data->value.string=ptr->data.value.string;
      break;
    case OBJECT:
      data->value.objptr=ptr->data.value.objptr;
      break;
    case ASM_INSTR:
      data->value.instruction=ptr->data.value.instruction;
      break;
    case GLOBAL_L_VALUE:
    case LOCAL_L_VALUE:
      data->value.l_value.ref=ptr->data.value.l_value.ref;
      data->value.l_value.size=ptr->data.value.l_value.size;
      break;
    case FUNC_CALL:
      data->value.func_call=ptr->data.value.func_call;
      break;
    case LOCAL_REF:
    case GLOBAL_REF:
      {
        unsigned int array_base, array_size, index;
        /* Pop size, index, base (new 3-value format) */
        if (popint(&tmp,rts,obj)) {
          FREE(ptr);
          return 1;
        }
        array_size = tmp.value.integer;
        if (popint(&tmp,rts,obj)) {
          FREE(ptr);
          return 1;
        }
        index = tmp.value.integer;
        if (popint(&tmp,rts,obj)) {
          FREE(ptr);
          return 1;
        }
        array_base = tmp.value.integer;
        /* Calculate final reference */
        data->value.l_value.ref = array_base + index;
        data->value.l_value.size = 1;
        if (data->type==LOCAL_REF) {
          if (data->value.l_value.ref>=num_locals) {
            FREE(ptr);
            return 1;
          }
          data->type=LOCAL_L_VALUE;
        } else {
          if (data->value.l_value.ref>=obj->parent->funcs->num_globals) {
            FREE(ptr);
            return 1;
          }
          data->type=GLOBAL_L_VALUE;
        }
      }
      break;
    default:
      data->value.num=ptr->data.value.num;
      break;
  }
  /* Note: Removed size==1 validation to allow arrays to be passed as values
   * This is needed for array operations like sizeof(), join(), etc.
   * The old check prevented arrays from being used as first-class values.
   */
  FREE(ptr);
  return 0;
}

void free_stack(struct var_stack **rts) {
  struct var_stack *next;

  next=*rts;
  while (next) {
    next=(*rts)->next;
    if ((*rts)->data.type==STRING || (*rts)->data.type==FUNC_NAME ||
        (*rts)->data.type==EXTERN_FUNC)
      FREE((*rts)->data.value.string);
    FREE(*rts);
    *rts=next;
  }
}

void clear_var(struct var *data) {
  if (data->type==STRING || data->type==FUNC_NAME || data->type==EXTERN_FUNC)
    FREE(data->value.string);
  else if (data->type==ARRAY)
    array_release(data->value.array_ptr);  /* Release heap array */
  else if (data->type==MAPPING)
    mapping_release(data->value.mapping_ptr);  /* Release heap mapping */
  data->type=INTEGER;
  data->value.integer=0;
}

void clear_global_var(struct object *obj, unsigned int ref) {
  struct ref_list *next,*curr;

  if (ref>=obj->parent->funcs->num_globals)
    return;
  obj->obj_state=DIRTY;
  if (obj->globals[ref].type==STRING || obj->globals[ref].type==FUNC_NAME)
    FREE(obj->globals[ref].value.string);
  if (obj->globals[ref].type==OBJECT) {
    load_data(obj->globals[ref].value.objptr);
    obj->globals[ref].value.objptr->obj_state=DIRTY;
    next=obj->globals[ref].value.objptr->refd_by;
    if (next)
      if (next->ref_obj==obj && next->ref_num==ref) {
        obj->globals[ref].value.objptr->refd_by=next->next;
        FREE(next);
      } else
        while (next)
          if (next->ref_obj==obj && next->ref_num==ref) {
            curr->next=next->next;
            FREE(next);
            next=NULL;
          } else {
            curr=next;
            next=curr->next;
          }
  }
  obj->globals[ref].type=INTEGER;
  obj->globals[ref].value.integer=0;
}

void copy_var(struct var *dest, struct var *src) {
  dest->type=src->type;
  switch (src->type) {
    case INTEGER:
      dest->value.integer=src->value.integer;
      break;
    case STRING:
    case FUNC_NAME:
    case EXTERN_FUNC:
      dest->value.string=copy_string(src->value.string);
      break;
    case OBJECT:
      dest->value.objptr=src->value.objptr;
      break;
    case ARRAY:
      dest->value.array_ptr=src->value.array_ptr;
      if (dest->value.array_ptr)
        array_addref(dest->value.array_ptr);
      break;
    case MAPPING:
      dest->value.mapping_ptr=src->value.mapping_ptr;
      if (dest->value.mapping_ptr)
        mapping_addref(dest->value.mapping_ptr);
      break;
    case ASM_INSTR:
      dest->value.instruction=src->value.instruction;
      break;
    case GLOBAL_L_VALUE:
    case LOCAL_L_VALUE:
      dest->value.l_value.ref=src->value.l_value.ref;
      dest->value.l_value.size=src->value.l_value.size;
      break;
    default:
      dest->value.num=src->value.num;
      break;
  }
}

struct var_stack *gen_stack(struct var_stack **rts, struct object *obj) {
  struct var_stack *tmp,*stack1;
  unsigned long arg_count;
  char logbuf[256];

  stack1=*rts;
  tmp=*rts;
  if (!tmp) return NULL;
  
  sprintf(logbuf, "gen_stack: top of stack type=%d (NUM_ARGS=%d)", tmp->data.type, NUM_ARGS);
  logger(LOG_DEBUG, logbuf);
  
  arg_count=tmp->data.value.num;
  sprintf(logbuf, "gen_stack: arg_count=%lu", arg_count);
  logger(LOG_DEBUG, logbuf);
  
  while (arg_count && tmp->next) {
    tmp=tmp->next;
    arg_count--;
    sprintf(logbuf, "gen_stack: processing arg, remaining=%lu", arg_count);
    logger(LOG_DEBUG, logbuf);
    if (resolve_var(&(tmp->data),obj)) {
      logger(LOG_ERROR, "gen_stack: resolve_var failed");
      return NULL;
    }
  }
  
  sprintf(logbuf, "gen_stack: after loop, arg_count=%lu, tmp->next=%p", arg_count, (void*)tmp->next);
  logger(LOG_DEBUG, logbuf);
  
  if (arg_count) {
    sprintf(logbuf, "gen_stack: FAIL - arg_count still %lu", arg_count);
    logger(LOG_DEBUG, logbuf);
    return NULL;
  }
  *rts=tmp->next;
  tmp->next=NULL;
  return stack1;
}

struct var_stack *gen_stack_noresolve(struct var_stack **rts,
                                      struct object *obj) {
  struct var_stack *tmp,*stack1;
  unsigned long arg_count;

  stack1=*rts;
  tmp=*rts;
  if (!tmp) return NULL;
  arg_count=tmp->data.value.num;
  while (arg_count && tmp->next) {
    tmp=tmp->next;
    arg_count--;
  }
  if (arg_count) return NULL;
  *rts=tmp->next;
  tmp->next=NULL;
  return stack1;
}
