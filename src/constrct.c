/* construct.c */

#include "config.h"
#include "object.h"
#include "instr.h"
#include "constrct.h"
#include "globals.h"
#include "cache.h"

int is_legal(char *name) {
  int x,len,ncount,ecount,pcount;

  ncount=0;
  ecount=0;
  pcount=0;
  len=strlen(name);
  if (!len || len>14) return 0;
  if (*name=='.') return 0;
  x=0;
  while (x<len) {
    if ((!(isgraph(name[x]))) || name[x]=='/' || name[x]=='#' ||
        name[x]=='\\' || name[x]=='~' || name[x]=='*' || name[x]=='?' ||
        name[x]==':')
      return 0;
    if (isalpha(name[x]))
      if (isupper(name[x]))
        return 0;
    if (name[x]=='.') {
      pcount++;
      if (pcount>1) return 0;
    } else {
      if (pcount) ecount++;
      else ncount++;
      if (ecount>3 || ncount>8) return 0;
    }
    x++;
  }
  if (ncount<1) return 0;
  if (name[len-1]=='.') return 0;
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
  if (data->type==GLOBAL_L_VALUE) {
    if (data->value.l_value.size!=1)
      return 1;
    if (data->value.l_value.ref>=obj->parent->funcs->num_globals)
      return 1;
    data->type=obj->globals[data->value.l_value.ref].type;
    switch (data->type) {
      case INTEGER:
        data->value.integer=obj->globals[data->value.l_value.ref].value.integer;
        break;
      case STRING:
        data->value.string=copy_string(obj->globals[data->value.l_value.ref].
                                       value.string);
        break;
      case OBJECT:
        data->value.objptr=obj->globals[data->value.l_value.ref].value.objptr;
        break;
      default:
        return 1;
        break;
    }
  } else
    if (data->type==LOCAL_L_VALUE) {
      if (data->value.l_value.size!=1)
        return 1;
      if (data->value.l_value.ref>=num_locals)
        return 1;
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
        default:
          return 1;
          break;
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
      if (popint(&tmp,rts,obj)) {
        FREE(ptr);
        return 1;
      }
      data->value.l_value.size=tmp.value.integer;
      if (popint(&tmp,rts,obj)) {
        FREE(ptr);
        return 1;
      }
      data->value.l_value.ref=tmp.value.integer;
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
      break;
    default:
      data->value.num=ptr->data.value.num;
      break;
  }
  if (data->type==LOCAL_L_VALUE || data->type==GLOBAL_L_VALUE)
    if (data->value.l_value.size!=1) {
      FREE(ptr);
      return 1;
    }
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
      if (popint(&tmp,rts,obj)) {
        FREE(ptr);
        return 1;
      }
      data->value.l_value.size=tmp.value.integer;
      if (popint(&tmp,rts,obj)) {
        FREE(ptr);
        return 1;
      }
      data->value.l_value.ref=tmp.value.integer;
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
      break;
    default:
      data->value.num=ptr->data.value.num;
      break;
  }
  if (data->type==LOCAL_L_VALUE || data->type==GLOBAL_L_VALUE)
    if (data->value.l_value.size!=1) {
      FREE(ptr);
      return 1;
    }
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

  stack1=*rts;
  tmp=*rts;
  if (!tmp) return NULL;
  arg_count=tmp->data.value.num;
  while (arg_count && tmp->next) {
    tmp=tmp->next;
    arg_count--;
    if (resolve_var(&(tmp->data),obj))
      return NULL;
  }
  if (arg_count) return NULL;
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
