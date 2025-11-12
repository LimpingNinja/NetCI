#include "config.h"
#include "object.h"
#include "constrct.h"
#include "instr.h"
#include "protos.h"
#include "interp.h"  /* For global_index_for and call_frame */
#include "operdef.h"
#include "globals.h"
#include "cache.h"
#include "file.h"

int comma_oper(struct object *caller, struct object *obj,
                struct object *player, struct var_stack **rts) {
  struct var tmp;

  if (pop(&tmp,rts,obj)) return 1;
  clear_var(&tmp);
  if (pop(&tmp,rts,obj)) return 1;
  pushnocopy(&tmp,rts);
  return 0;
}

int eq_oper(struct object *caller, struct object *obj,
             struct object *player, struct var_stack **rts) {
  struct var tmp1,tmp2;
  struct ref_list *tmpref;
  char logbuf[256];

  if (pop(&tmp2,rts,obj)) {
    logger(LOG_DEBUG, "eq_oper: failed to pop tmp2");
    return 1;
  }
  sprintf(logbuf, "eq_oper: popped tmp2, type=%d", tmp2.type);
  logger(LOG_DEBUG, logbuf);
  
  if (pop(&tmp1,rts,obj)) {
    logger(LOG_DEBUG, "eq_oper: failed to pop tmp1");
    clear_var(&tmp2);
    return 1;
  }
  sprintf(logbuf, "eq_oper: popped tmp1, type=%d", tmp1.type);
  logger(LOG_DEBUG, logbuf);
  
  if (tmp1.type!=GLOBAL_L_VALUE && tmp1.type!=LOCAL_L_VALUE) {
    sprintf(logbuf, "eq_oper: tmp1 is not an L_VALUE! type=%d", tmp1.type);
    logger(LOG_DEBUG, logbuf);
    clear_var(&tmp1);
    clear_var(&tmp2);
    return 1;
  }
  /* Sanity check: size should be at least 1 */
  if (tmp1.value.l_value.size < 1) {
    clear_var(&tmp2);
    return 1;
  }
  if (resolve_var(&tmp2,obj)) return 1;
  
  /* Increment refcount for arrays and mappings on assignment */
  if (tmp2.type == ARRAY) {
    array_addref(tmp2.value.array_ptr);
  } else if (tmp2.type == MAPPING) {
    mapping_addref(tmp2.value.mapping_ptr);
  }
  
  if (tmp1.type==GLOBAL_L_VALUE) {
    obj->obj_state=DIRTY;
    
    /* Check if this is a heap array element (pointer) or regular global */
    if (tmp1.value.l_value.ref >= obj->parent->funcs->num_globals) {
      /* Heap array element - direct pointer assignment */
      struct var *element_ptr = (struct var *)tmp1.value.l_value.ref;
      /* Clear old value before overwriting */
      if (element_ptr->type == STRING || element_ptr->type == FUNC_NAME || 
          element_ptr->type == EXTERN_FUNC) {
        FREE(element_ptr->value.string);
      } else if (element_ptr->type == ARRAY) {
        array_release(element_ptr->value.array_ptr);
      } else if (element_ptr->type == MAPPING) {
        mapping_release(element_ptr->value.mapping_ptr);
      }
      *element_ptr = tmp2;
    } else {
      /* Regular global variable - compute absolute index via GST mapping */
      extern struct call_frame *call_stack;  /* Defined in interp.c */
      int ok = 0;
      unsigned int effective_index = global_index_for(obj, call_stack ? call_stack->func : NULL,
                                                     (unsigned int)tmp1.value.l_value.ref, &ok);
      sprintf(logbuf, "eq_oper: GLOBAL ref=%lu => effective_index=%u (ok=%d)", 
              tmp1.value.l_value.ref, effective_index, ok);
      logger(LOG_DEBUG, logbuf);
      if (!ok) { clear_var(&tmp1); clear_var(&tmp2); return 1; }
      if (tmp2.type==OBJECT) {
        load_data(tmp2.value.objptr);
        tmp2.value.objptr->obj_state=DIRTY;
        tmpref=MALLOC(sizeof(struct ref_list));
        tmpref->ref_obj=obj;
        tmpref->ref_num=effective_index;
        tmpref->next=tmp2.value.objptr->refd_by;
        tmp2.value.objptr->refd_by=tmpref;
      }
      clear_global_var(obj,effective_index);
      obj->globals[effective_index]=tmp2;
    }
  } else {
    /* LOCAL_L_VALUE */
    /* Check if this is a heap array element (pointer) or regular local */
    if (tmp1.value.l_value.ref >= num_locals) {
      /* Heap array element - direct pointer assignment */
      struct var *element_ptr = (struct var *)tmp1.value.l_value.ref;
      /* Clear old value before overwriting */
      if (element_ptr->type == STRING || element_ptr->type == FUNC_NAME || 
          element_ptr->type == EXTERN_FUNC) {
        FREE(element_ptr->value.string);
      } else if (element_ptr->type == ARRAY) {
        array_release(element_ptr->value.array_ptr);
      } else if (element_ptr->type == MAPPING) {
        mapping_release(element_ptr->value.mapping_ptr);
      }
      *element_ptr = tmp2;
    } else {
      /* Regular local variable */
      clear_var(&(locals[tmp1.value.l_value.ref]));
      locals[tmp1.value.l_value.ref]=tmp2;
    }
  }
  push(&tmp2,rts);
  return 0;
}

EQ_INT_OPER(intaddeq, += )

int pleq_oper(struct object *caller, struct object *obj,
              struct object *player, struct var_stack **rts) {
  struct var tmp1,tmp2;
  char *tmpstr;

  if (pop(&tmp2,rts,obj)) return 1;
  if (pop(&tmp1,rts,obj)) {
    clear_var(&tmp2);
    return 1;
  }
  if (tmp1.type!=GLOBAL_L_VALUE && tmp1.type!=LOCAL_L_VALUE) {
    clear_var(&tmp2);
    clear_var(&tmp1);
    return 1;
  }
  /* Sanity check: size should be at least 1 */
  if (tmp1.value.l_value.size < 1) {
    clear_var(&tmp2);
    return 1;
  }
  if (resolve_var(&tmp2,obj)) {
    clear_var(&tmp1);
    clear_var(&tmp2);
    return 1;
  }
  if (tmp1.type==GLOBAL_L_VALUE) {
    /* Compute effective global index for all accesses */
    extern struct call_frame *call_stack;
    int ok_eff = 0;
    unsigned int eff_idx = global_index_for(obj, call_stack ? call_stack->func : NULL,
                                           (unsigned int)tmp1.value.l_value.ref, &ok_eff);
    if (!ok_eff) { clear_var(&tmp1); clear_var(&tmp2); return 1; }
    if (tmp2.type==INTEGER && obj->globals[eff_idx].type==
        INTEGER) {
      push(&tmp1,rts);
      push(&tmp2,rts);
      return intaddeq(caller,obj,player,rts);
    }
    /* Array concatenation assignment: array += array */
    if (tmp2.type==ARRAY && obj->globals[eff_idx].type==ARRAY) {
      struct heap_array *result;
      
      result = array_concat(obj->globals[eff_idx].value.array_ptr, tmp2.value.array_ptr);
      if (!result) {
        clear_var(&tmp1);
        clear_var(&tmp2);
        return 1;
      }
      
      /* Release old array, assign new one */
      array_release(obj->globals[eff_idx].value.array_ptr);
      obj->globals[eff_idx].value.array_ptr = result;
      obj->obj_state=DIRTY;
      
      clear_var(&tmp1);
      clear_var(&tmp2);
      return 0;
    }
    /* Mapping merge assignment: mapping += mapping */
    if (tmp2.type==MAPPING && obj->globals[eff_idx].type==MAPPING) {
      struct heap_mapping *result;
      
      result = mapping_merge(obj->globals[eff_idx].value.mapping_ptr, tmp2.value.mapping_ptr);
      if (!result) {
        clear_var(&tmp1);
        clear_var(&tmp2);
        return 1;
      }
      
      /* Release old mapping, assign new one */
      mapping_release(obj->globals[eff_idx].value.mapping_ptr);
      obj->globals[eff_idx].value.mapping_ptr = result;
      obj->obj_state=DIRTY;
      
      clear_var(&tmp1);
      clear_var(&tmp2);
      return 0;
    }
    if (tmp2.type==INTEGER && tmp2.value.integer==0) {
      tmp2.type=STRING;
      tmp2.value.string=copy_string("");
    }
    if (tmp2.type==STRING) {
      if (obj->globals[eff_idx].type==INTEGER && obj->globals
          [eff_idx].value.integer==0) {
        obj->globals[eff_idx].type=STRING;
        obj->globals[eff_idx].value.string=copy_string("");
      }
    }
    if (tmp2.type!=STRING || obj->globals[eff_idx].type!=
        STRING) {
      clear_var(&tmp1);
      clear_var(&tmp2);
      return 1;
    }
    tmpstr=MALLOC(strlen(obj->globals[eff_idx].value.string)+
                  strlen(tmp2.value.string)+1);
    strcat(strcpy(tmpstr,obj->globals[eff_idx].value.string),
           tmp2.value.string);
    clear_global_var(obj,eff_idx);
    obj->globals[eff_idx].type=STRING;
    obj->globals[eff_idx].value.string=tmpstr;
    obj->obj_state=DIRTY;
  } else {
    if (tmp2.type==INTEGER && locals[tmp1.value.l_value.ref].type==INTEGER) {
      push(&tmp1,rts);
      push(&tmp2,rts);
      return intaddeq(caller,obj,player,rts);
    }
    /* Array concatenation assignment: array += array */
    if (tmp2.type==ARRAY && locals[tmp1.value.l_value.ref].type==ARRAY) {
      struct heap_array *result;
      
      result = array_concat(locals[tmp1.value.l_value.ref].value.array_ptr, tmp2.value.array_ptr);
      if (!result) {
        clear_var(&tmp1);
        clear_var(&tmp2);
        return 1;
      }
      
      /* Release old array, assign new one */
      array_release(locals[tmp1.value.l_value.ref].value.array_ptr);
      locals[tmp1.value.l_value.ref].value.array_ptr = result;
      
      clear_var(&tmp1);
      clear_var(&tmp2);
      return 0;
    }
    /* Mapping merge assignment: mapping += mapping */
    if (tmp2.type==MAPPING && locals[tmp1.value.l_value.ref].type==MAPPING) {
      struct heap_mapping *result;
      
      result = mapping_merge(locals[tmp1.value.l_value.ref].value.mapping_ptr, tmp2.value.mapping_ptr);
      if (!result) {
        clear_var(&tmp1);
        clear_var(&tmp2);
        return 1;
      }
      
      /* Release old mapping, assign new one */
      mapping_release(locals[tmp1.value.l_value.ref].value.mapping_ptr);
      locals[tmp1.value.l_value.ref].value.mapping_ptr = result;
      
      clear_var(&tmp1);
      clear_var(&tmp2);
      return 0;
    }
    if (tmp2.type==INTEGER && tmp2.value.integer==0) {
      tmp2.type=STRING;
      tmp2.value.string=copy_string("");
    }
    if (tmp2.type==STRING) {
      if (locals[tmp1.value.l_value.ref].type==INTEGER && locals[tmp1.value.
          l_value.ref].value.integer==0) {
        locals[tmp1.value.l_value.ref].type=STRING;
        locals[tmp1.value.l_value.ref].value.string=copy_string("");
      }
    }
    if (tmp2.type!=STRING || locals[tmp1.value.l_value.ref].type!=
        STRING) {
      clear_var(&tmp1);
      clear_var(&tmp2);
      return 1;
    }
    tmpstr=MALLOC(strlen(locals[tmp1.value.l_value.ref].value.string)+
                  strlen(tmp2.value.string)+1);
    strcat(strcpy(tmpstr,locals[tmp1.value.l_value.ref].value.string),
           tmp2.value.string);
    clear_var(&(locals[tmp1.value.l_value.ref]));
    locals[tmp1.value.l_value.ref].type=STRING;
    locals[tmp1.value.l_value.ref].value.string=tmpstr;
  }
  clear_var(&tmp1);
  clear_var(&tmp2);
  tmp1.type=STRING;
  tmp1.value.string=tmpstr;
  push(&tmp1,rts);
  return 0;
}

int cond_oper(struct object *caller, struct object *obj,
              struct object *player, struct var_stack **rts) {
  return 0;
}

int or_oper(struct object *caller, struct object *obj,
            struct object *player, struct var_stack **rts) {
  struct var tmp1,tmp2;
  int t1,t2;

  if (pop(&tmp2,rts,obj)) return 1;
  if (pop(&tmp1,rts,obj)) {
    clear_var(&tmp2);
    return 1;
  }
  if (resolve_var(&tmp1,obj)) {
    clear_var(&tmp1);
    clear_var(&tmp2);
    return 1;
  }
  if (resolve_var(&tmp2,obj)) {
    clear_var(&tmp1);
    clear_var(&tmp2);
    return 1;
  }
  t1=!(tmp1.type==INTEGER && tmp1.value.integer==0);
  t2=!(tmp2.type==INTEGER && tmp2.value.integer==0);
  clear_var(&tmp1);
  clear_var(&tmp2);
  tmp1.type=INTEGER;
  tmp1.value.integer=(t1 || t2);
  push(&tmp1,rts);
  return 0;
}

int and_oper(struct object *caller, struct object *obj,
             struct object *player, struct var_stack **rts) {
  struct var tmp1,tmp2;
  int t1,t2;

  if (pop(&tmp2,rts,obj)) return 1;
  if (pop(&tmp1,rts,obj)) {
    clear_var(&tmp2);
    return 1;
  }
  if (resolve_var(&tmp1,obj)) {
    clear_var(&tmp1);
    clear_var(&tmp2);
    return 1;
  }
  if (resolve_var(&tmp2,obj)) {
    clear_var(&tmp1);
    clear_var(&tmp2);
    return 1;
  }
  t1=!(tmp1.type==INTEGER && tmp1.value.integer==0);
  t2=!(tmp2.type==INTEGER && tmp2.value.integer==0);
  clear_var(&tmp1);
  clear_var(&tmp2);
  tmp1.type=INTEGER;
  tmp1.value.integer=(t1 && t2);
  push (&tmp1,rts);
  return 0;
}

int condeq_oper(struct object *caller, struct object *obj,
                struct object *player, struct var_stack **rts) {
  struct var tmp1,tmp2;
  int result;

  if (pop(&tmp2,rts,obj)) return 1;
  if (pop(&tmp1,rts,obj)) {
    clear_var(&tmp2);
    return 1;
  }
  if (resolve_var(&tmp1,obj)) {
    clear_var(&tmp1);
    clear_var(&tmp2);
    return 1;
  }
  if (resolve_var(&tmp2,obj)) {
    clear_var(&tmp1);
    clear_var(&tmp2);
    return 1;
  }
  if (tmp1.type==STRING && tmp2.type==STRING) {
    result=(!strcmp(tmp1.value.string,tmp2.value.string));
    clear_var(&tmp1);
    clear_var(&tmp2);
    tmp1.type=INTEGER;
    tmp1.value.integer=result;
    push(&tmp1,rts);
    return 0;
  }
  result=(tmp1.type==tmp2.type && ((tmp1.type==INTEGER &&
          tmp1.value.integer==tmp2.value.integer) || (tmp1.type==OBJECT &&
          tmp1.value.objptr==tmp2.value.objptr)));
  clear_var(&tmp1);
  clear_var(&tmp2);
  tmp1.type=INTEGER;
  tmp1.value.integer=result;
  push(&tmp1,rts);
  return 0;
}

int noteq_oper(struct object *caller,struct object *obj,
               struct object *player, struct var_stack **rts) {
  struct var tmp;

  if (condeq_oper(caller,obj,player,rts)) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=INTEGER) {
    clear_var(&tmp);
    return 1;
  }
  tmp.value.integer=!(tmp.value.integer);
  push(&tmp,rts);
  return 0;
}

BI_INT_OPER(intadd, + )

int add_oper(struct object *caller, struct object *obj,
             struct object *player, struct var_stack **rts) {
  struct var tmp1,tmp2;
  char *tmpstr;

  if (pop(&tmp2,rts,obj)) return 1;
  if (pop(&tmp1,rts,obj)) {
    clear_var(&tmp2);
    return 1;
  }
  if (resolve_var(&tmp1,obj)) {
    clear_var(&tmp1);
    clear_var(&tmp2);
    return 1;
  }
  if (resolve_var(&tmp2,obj)) {
    clear_var(&tmp1);
    clear_var(&tmp2);
    return 1;
  }
  if (tmp1.type==INTEGER && tmp2.type==INTEGER) {
    push(&tmp1,rts);
    push(&tmp2,rts);
    return intadd(caller,obj,player,rts);
  }
  /* Array concatenation: array + array */
  if (tmp1.type==ARRAY && tmp2.type==ARRAY) {
    struct heap_array *result;
    struct var result_var;
    
    result = array_concat(tmp1.value.array_ptr, tmp2.value.array_ptr);
    if (!result) {
      clear_var(&tmp1);
      clear_var(&tmp2);
      return 1;
    }
    
    result_var.type = ARRAY;
    result_var.value.array_ptr = result;
    push(&result_var, rts);
    
    clear_var(&tmp1);
    clear_var(&tmp2);
    return 0;
  }
  /* Mapping merge: mapping + mapping */
  if (tmp1.type==MAPPING && tmp2.type==MAPPING) {
    struct heap_mapping *result;
    struct var result_var;
    
    result = mapping_merge(tmp1.value.mapping_ptr, tmp2.value.mapping_ptr);
    if (!result) {
      clear_var(&tmp1);
      clear_var(&tmp2);
      return 1;
    }
    
    result_var.type = MAPPING;
    result_var.value.mapping_ptr = result;
    push(&result_var, rts);
    
    clear_var(&tmp1);
    clear_var(&tmp2);
    return 0;
  }
  if (tmp1.type==INTEGER && tmp1.value.integer==0) {
    tmp1.type=STRING;
    tmp1.value.string=copy_string("");
  }
  if (tmp2.type==INTEGER && tmp2.value.integer==0) {
    tmp2.type=STRING;
    tmp2.value.string=copy_string("");
  }
  if (tmp2.type!=STRING || tmp1.type!=STRING) {
    clear_var(&tmp1);
    clear_var(&tmp2);
    return 1;
  }
  tmpstr=MALLOC(strlen(tmp1.value.string)+strlen(tmp2.value.string)+1);
  strcat(strcpy(tmpstr,tmp1.value.string),tmp2.value.string);
  clear_var(&tmp1);
  clear_var(&tmp2);
  tmp1.type=STRING;
  tmp1.value.string=tmpstr;
  pushnocopy(&tmp1,rts);
  return 0;
}
