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
BI_INT_OPER(min_oper, - )
BI_INT_OPER(mul_oper, * )
CZBI_OPER(div_oper, / )
CZBI_OPER(mod_oper, % )
EQ_INT_OPER(mieq_oper, -= )
EQ_INT_OPER(mueq_oper, *= )
EQ_INT_OPER(aneq_oper, &= )
EQ_INT_OPER(exeq_oper, ^= )
EQ_INT_OPER(oreq_oper, |= )
EQ_INT_OPER(lseq_oper, <<= )
EQ_INT_OPER(rseq_oper, >>= )
CZEQ_OPER(dieq_oper, /= )
CZEQ_OPER(moeq_oper, %= )
