#define BI_INT_OPER(FUNCNAME_,OPERATOR_)                         \
  int FUNCNAME_(struct object *caller,struct object *obj,        \
                struct object *player,struct var_stack **rts) {  \
    struct var tmp1,tmp2;                                        \
                                                                 \
    if (popint(&tmp2,rts,obj)) return 1;                         \
    if (popint(&tmp1,rts,obj)) return 1;                         \
    tmp1.value.integer=((tmp1.value.integer) OPERATOR_           \
                        (tmp2.value.integer));                   \
    push(&tmp1,rts);                                             \
    return 0;                                                    \
  }

#define CND_OPER(FUNCNAME_,OPERATOR_)                           \
  int FUNCNAME_(struct object *caller, struct object *obj,       \
                struct object *player, struct var_stack **rts) { \
    struct var tmp1,tmp2;                                        \
    int result;                                                  \
                                                                 \
    if (pop(&tmp2,rts,obj)) return 1;                            \
    if (pop(&tmp1,rts,obj)) {                                    \
      clear_var(&tmp2);                                          \
      return 1;                                                  \
    }                                                            \
    if (resolve_var(&tmp1,obj)) {                                \
      clear_var(&tmp1);                                          \
      clear_var(&tmp2);                                          \
      return 1;                                                  \
    }                                                            \
    if (resolve_var(&tmp2,obj)) {                                \
      clear_var(&tmp1);                                          \
      clear_var(&tmp2);                                          \
      return 1;                                                  \
    }                                                            \
    if (tmp1.type==STRING &&                                     \
        (tmp2.type==INTEGER && tmp2.value.integer==0)) {         \
      tmp2.type=STRING;                                          \
      tmp2.value.string=copy_string("");                         \
    } else if (tmp2.type==STRING &&                              \
               (tmp1.type==INTEGER && tmp1.value.integer==0)) {  \
      tmp1.type=STRING;                                          \
      tmp1.value.string=copy_string("");                         \
    }                                                            \
    if (tmp1.type==STRING && tmp2.type==STRING) {                \
      result=(strcmp(tmp1.value.string,tmp2.value.string)        \
              OPERATOR_ 0);                                      \
      clear_var(&tmp1);                                          \
      clear_var(&tmp2);                                          \
      tmp1.type=INTEGER;                                         \
      tmp1.value.integer=result;                                 \
      push(&tmp1,rts);                                           \
      return 0;                                                  \
    }                                                            \
    if (tmp1.type!=INTEGER || tmp2.type!=INTEGER) {              \
      clear_var(&tmp1);                                          \
      clear_var(&tmp2);                                          \
      return 1;                                                  \
    }                                                            \
    result=(tmp1.value.integer OPERATOR_ tmp2.value.integer);    \
    clear_var(&tmp1);                                            \
    clear_var(&tmp2);                                            \
    tmp1.type=INTEGER;                                           \
    tmp1.value.integer=result;                                   \
    push(&tmp1,rts);                                             \
    return 0;                                                    \
  }


#define CZBI_OPER(FUNCNAME_,OPERATOR_)                           \
  int FUNCNAME_(struct object *caller,struct object *obj,        \
                struct object *player,struct var_stack **rts) {  \
    struct var tmp1,tmp2;                                        \
                                                                 \
    if (popint(&tmp2,rts,obj)) return 1;                         \
    if (popint(&tmp1,rts,obj)) return 1;                         \
    if (tmp2.value.integer==0) return 1;                         \
    tmp1.value.integer=((tmp1.value.integer) OPERATOR_           \
                        (tmp2.value.integer));                   \
    push(&tmp1,rts);                                             \
    return 0;                                                    \
  }

#define EQ_INT_OPER(FUNCNAME_,OPERATOR_)                         \
  int FUNCNAME_(struct object *caller,struct object *obj,        \
                struct object *player,struct var_stack **rts) {  \
    struct var tmp1,tmp2;                                        \
                                                                 \
    if (popint(&tmp2,rts,obj)) return 1;                         \
    if (pop(&tmp1,rts,obj)) return 1;                            \
    if (tmp1.type!=GLOBAL_L_VALUE && tmp1.type!=LOCAL_L_VALUE) { \
      clear_var(&tmp1);                                          \
      return 1;                                                  \
    }                                                            \
    if (tmp1.value.l_value.size!=1) return 1;                    \
    if (tmp1.type==GLOBAL_L_VALUE) {                             \
      if (obj->globals[tmp1.value.l_value.ref].type!=INTEGER)    \
        return 1;                                                \
      (obj->globals[tmp1.value.l_value.ref].value.integer)       \
      OPERATOR_ (tmp2.value.integer);                            \
      push(&(obj->globals[tmp1.value.l_value.ref]),rts);         \
      obj->obj_state=DIRTY;                                      \
    } else {                                                     \
      if (locals[tmp1.value.l_value.ref].type!=INTEGER)          \
        return 1;                                                \
      (locals[tmp1.value.l_value.ref].value.integer)             \
      OPERATOR_ (tmp2.value.integer);                            \
      push(&(locals[tmp1.value.l_value.ref]),rts);               \
    }                                                            \
    return 0;                                                    \
  }

#define CZEQ_OPER(FUNCNAME_,OPERATOR_)                           \
  int FUNCNAME_(struct object *caller,struct object *obj,        \
                struct object *player,struct var_stack **rts) {  \
    struct var tmp1,tmp2;                                        \
                                                                 \
    if (popint(&tmp2,rts,obj)) return 1;                         \
    if (pop(&tmp1,rts,obj)) return 1;                            \
    if (tmp1.type!=GLOBAL_L_VALUE && tmp1.type!=LOCAL_L_VALUE) { \
      clear_var(&tmp1);                                          \
      return 1;                                                  \
    }                                                            \
    if (tmp1.value.l_value.size!=1) return 1;                    \
    if (tmp1.type==GLOBAL_L_VALUE) {                             \
      if (obj->globals[tmp1.value.l_value.ref].type!=INTEGER)    \
        return 1;                                                \
      if (tmp2.value.integer==0) return 1;                       \
      (obj->globals[tmp1.value.l_value.ref].value.integer)       \
      OPERATOR_ (tmp2.value.integer);                            \
      push(&(obj->globals[tmp1.value.l_value.ref]),rts);         \
      obj->obj_state=DIRTY;                                      \
    } else {                                                     \
      if (locals[tmp1.value.l_value.ref].type!=INTEGER)          \
        return 1;                                                \
      if (tmp2.value.integer==0) return 1;                       \
      (locals[tmp1.value.l_value.ref].value.integer)             \
      OPERATOR_ (tmp2.value.integer);                            \
      push(&(locals[tmp1.value.l_value.ref]),rts);               \
    }                                                            \
    return 0;                                                    \
  }
