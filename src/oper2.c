#include "config.h"
#include "object.h"
#include "constrct.h"
#include "instr.h"
#include "protos.h"
#include "operdef.h"
#include "globals.h"

int not_oper(struct object *caller, struct object *obj,
             struct object *player, struct var_stack **rts) {
  struct var tmp;
  int result;

  if (pop(&tmp,rts,obj)) return 1;
  if (resolve_var(&tmp,obj)) return 1;
  result=(tmp.type==INTEGER && tmp.value.integer==0);
  clear_var(&tmp);
  tmp.type=INTEGER;
  tmp.value.integer=result;
  push(&tmp,rts);
  return 0;
}

int bitnot_oper(struct object *caller, struct object *obj,
                struct object *player, struct var_stack **rts) {
  struct var tmp;

  if (popint(&tmp,rts,obj)) return 1;
  tmp.value.integer=~tmp.value.integer;
  push(&tmp,rts);
  return 0;
}

int postadd_oper(struct object *caller, struct object *obj,
                 struct object *player, struct var_stack **rts) {
  struct var tmp1,tmp2;

  if (pop(&tmp1,rts,obj)) return 1;
  if (tmp1.type!=GLOBAL_L_VALUE && tmp1.type!=LOCAL_L_VALUE) {
    clear_var(&tmp1);
    return 1;
  }
  tmp2=tmp1;
  if (resolve_var(&tmp2,obj)) return 1;
  if (tmp2.type!=INTEGER) {
    clear_var(&tmp2);
    return 1;
  }
  push(&tmp2,rts);
  if (tmp1.type==GLOBAL_L_VALUE) {
    ++(obj->globals[tmp1.value.l_value.ref].value.integer);
    obj->obj_state=DIRTY;
  } else
    ++(locals[tmp1.value.l_value.ref].value.integer);
  return 0;
}

int preadd_oper(struct object *caller, struct object *obj,
                struct object *player, struct var_stack **rts) {
  struct var tmp1,tmp2;

  if (pop(&tmp1,rts,obj)) return 1;
  if (tmp1.type!=GLOBAL_L_VALUE && tmp1.type!=LOCAL_L_VALUE) {
    clear_var(&tmp1);
    return 1;
  }
  tmp2=tmp1;
  if (resolve_var(&tmp2,obj)) return 1;
  if (tmp2.type!=INTEGER) {
    clear_var(&tmp2);
    return 1;
  }
  ++(tmp2.value.integer);
  push(&tmp2,rts);
  if (tmp1.type==GLOBAL_L_VALUE) {
    ++(obj->globals[tmp1.value.l_value.ref].value.integer);
    obj->obj_state=DIRTY;
  } else
    ++(locals[tmp1.value.l_value.ref].value.integer);
  return 0;
}

int postmin_oper(struct object *caller, struct object *obj,
                 struct object *player, struct var_stack **rts) {
  struct var tmp1,tmp2;

  if (pop(&tmp1,rts,obj)) return 1;
  if (tmp1.type!=GLOBAL_L_VALUE && tmp1.type!=LOCAL_L_VALUE) {
    clear_var(&tmp1);
    return 1;
  }
  tmp2=tmp1;
  if (resolve_var(&tmp2,obj)) return 1;
  if (tmp2.type!=INTEGER) {
    clear_var(&tmp2);
    return 1;
  }
  push(&tmp2,rts);
  if (tmp1.type==GLOBAL_L_VALUE) {
    --(obj->globals[tmp1.value.l_value.ref].value.integer);
    obj->obj_state=DIRTY;
  } else
    --(locals[tmp1.value.l_value.ref].value.integer);
  return 0;
}

int premin_oper(struct object *caller, struct object *obj,
                struct object *player, struct var_stack **rts) {
  struct var tmp1,tmp2;

  if (pop(&tmp1,rts,obj)) return 1;
  if (tmp1.type!=GLOBAL_L_VALUE && tmp1.type!=LOCAL_L_VALUE) {
    clear_var(&tmp1);
    return 1;
  }
  tmp2=tmp1;
  if (resolve_var(&tmp2,obj)) return 1;
  if (tmp2.type!=INTEGER) {
    clear_var(&tmp2);
    return 1;
  }
  --(tmp2.value.integer);
  push(&tmp2,rts);
  if (tmp1.type==GLOBAL_L_VALUE) {
    --(obj->globals[tmp1.value.l_value.ref].value.integer);
    obj->obj_state=DIRTY;
  } else
    --(locals[tmp1.value.l_value.ref].value.integer);
  return 0;
}

int umin_oper(struct object *caller, struct object *obj,
              struct object *player, struct var_stack **rts) {
  struct var tmp;

  if (popint(&tmp,rts,obj)) return 1;
  tmp.value.integer=-tmp.value.integer;
  push(&tmp,rts);
  return 0;
}

BI_INT_OPER(bitor_oper, | )
BI_INT_OPER(exor_oper, ^ )
BI_INT_OPER(bitand_oper, & )
CND_OPER(less_oper, < )
CND_OPER(lesseq_oper, <= )
CND_OPER(great_oper, > )
CND_OPER(greateq_oper, >= )
BI_INT_OPER(ls_oper, << )
BI_INT_OPER(rs_oper, >> )

/* Custom min_oper to support array subtraction */
int min_oper(struct object *caller, struct object *obj,
             struct object *player, struct var_stack **rts) {
  struct var tmp1, tmp2;
  
  if (pop(&tmp2, rts, obj)) return 1;
  if (pop(&tmp1, rts, obj)) {
    clear_var(&tmp2);
    return 1;
  }
  if (resolve_var(&tmp1, obj)) {
    clear_var(&tmp1);
    clear_var(&tmp2);
    return 1;
  }
  if (resolve_var(&tmp2, obj)) {
    clear_var(&tmp1);
    clear_var(&tmp2);
    return 1;
  }
  
  /* Integer subtraction */
  if (tmp1.type == INTEGER && tmp2.type == INTEGER) {
    tmp1.value.integer -= tmp2.value.integer;
    push(&tmp1, rts);
    return 0;
  }
  
  /* Array subtraction: array - array */
  if (tmp1.type == ARRAY && tmp2.type == ARRAY) {
    struct heap_array *result;
    struct var result_var;
    
    result = array_subtract(tmp1.value.array_ptr, tmp2.value.array_ptr);
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
  
  /* Mapping subtraction: mapping - mapping */
  if (tmp1.type == MAPPING && tmp2.type == MAPPING) {
    struct heap_mapping *result;
    struct var result_var;
    
    result = mapping_subtract(tmp1.value.mapping_ptr, tmp2.value.mapping_ptr);
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
  
  clear_var(&tmp1);
  clear_var(&tmp2);
  return 1;
}

BI_INT_OPER(mul_oper, * )
CZBI_OPER(div_oper, / )
CZBI_OPER(mod_oper, % )

/* Custom mieq_oper to support array -= array */
int mieq_oper(struct object *caller, struct object *obj,
              struct object *player, struct var_stack **rts) {
  struct var tmp1, tmp2;
  
  if (pop(&tmp2, rts, obj)) return 1;
  if (pop(&tmp1, rts, obj)) {
    clear_var(&tmp2);
    return 1;
  }
  if (tmp1.type != GLOBAL_L_VALUE && tmp1.type != LOCAL_L_VALUE) {
    clear_var(&tmp1);
    clear_var(&tmp2);
    return 1;
  }
  if (resolve_var(&tmp2, obj)) {
    clear_var(&tmp1);
    clear_var(&tmp2);
    return 1;
  }
  
  if (tmp1.type == GLOBAL_L_VALUE) {
    /* Integer subtraction assignment */
    if (tmp2.type == INTEGER && obj->globals[tmp1.value.l_value.ref].type == INTEGER) {
      obj->globals[tmp1.value.l_value.ref].value.integer -= tmp2.value.integer;
      obj->obj_state = DIRTY;
      clear_var(&tmp1);
      clear_var(&tmp2);
      return 0;
    }
    /* Array subtraction assignment: array -= array */
    if (tmp2.type == ARRAY && obj->globals[tmp1.value.l_value.ref].type == ARRAY) {
      struct heap_array *result;
      
      result = array_subtract(obj->globals[tmp1.value.l_value.ref].value.array_ptr, tmp2.value.array_ptr);
      if (!result) {
        clear_var(&tmp1);
        clear_var(&tmp2);
        return 1;
      }
      
      /* Release old array, assign new one */
      array_release(obj->globals[tmp1.value.l_value.ref].value.array_ptr);
      obj->globals[tmp1.value.l_value.ref].value.array_ptr = result;
      obj->obj_state = DIRTY;
      
      clear_var(&tmp1);
      clear_var(&tmp2);
      return 0;
    }
    /* Mapping subtraction assignment: mapping -= mapping */
    if (tmp2.type == MAPPING && obj->globals[tmp1.value.l_value.ref].type == MAPPING) {
      struct heap_mapping *result;
      
      result = mapping_subtract(obj->globals[tmp1.value.l_value.ref].value.mapping_ptr, tmp2.value.mapping_ptr);
      if (!result) {
        clear_var(&tmp1);
        clear_var(&tmp2);
        return 1;
      }
      
      /* Release old mapping, assign new one */
      mapping_release(obj->globals[tmp1.value.l_value.ref].value.mapping_ptr);
      obj->globals[tmp1.value.l_value.ref].value.mapping_ptr = result;
      obj->obj_state = DIRTY;
      
      clear_var(&tmp1);
      clear_var(&tmp2);
      return 0;
    }
  } else {
    /* Integer subtraction assignment */
    if (tmp2.type == INTEGER && locals[tmp1.value.l_value.ref].type == INTEGER) {
      locals[tmp1.value.l_value.ref].value.integer -= tmp2.value.integer;
      clear_var(&tmp1);
      clear_var(&tmp2);
      return 0;
    }
    /* Array subtraction assignment: array -= array */
    if (tmp2.type == ARRAY && locals[tmp1.value.l_value.ref].type == ARRAY) {
      struct heap_array *result;
      
      result = array_subtract(locals[tmp1.value.l_value.ref].value.array_ptr, tmp2.value.array_ptr);
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
    /* Mapping subtraction assignment: mapping -= mapping */
    if (tmp2.type == MAPPING && locals[tmp1.value.l_value.ref].type == MAPPING) {
      struct heap_mapping *result;
      
      result = mapping_subtract(locals[tmp1.value.l_value.ref].value.mapping_ptr, tmp2.value.mapping_ptr);
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
  }
  
  clear_var(&tmp1);
  clear_var(&tmp2);
  return 1;
}

EQ_INT_OPER(mueq_oper, *= )
EQ_INT_OPER(aneq_oper, &= )
EQ_INT_OPER(exeq_oper, ^= )
EQ_INT_OPER(oreq_oper, |= )
EQ_INT_OPER(lseq_oper, <<= )
EQ_INT_OPER(rseq_oper, >>= )
CZEQ_OPER(dieq_oper, /= )
CZEQ_OPER(moeq_oper, %= )
