#include "config.h"
#include "object.h"
#include "instr.h"
#include "constrct.h"
#include "compile.h"
#include "interp.h"
#include "protos.h"
#include "intrface.h"
#include "dbhandle.h"
#include "globals.h"

int s_get_devconn(struct object *caller, struct object *obj, struct object
                *player, struct var_stack **rts) {
  struct var tmp;
  char *buf;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=1) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=OBJECT) {
    clear_var(&tmp);
    return 1;
  }
  buf=get_devconn(tmp.value.objptr);
  if (!buf) {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  } else {
    tmp.type=STRING;
    tmp.value.string=buf;
  }
  push(&tmp,rts);
  return 0;
}

int s_send_device(struct object *caller, struct object *obj, struct object
                  *player, struct var_stack **rts) {
  struct var tmp;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=1) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type==INTEGER && tmp.value.integer==0) {
    push(&tmp,rts);
    return 0;
  }
  if (tmp.type!=STRING) {
    clear_var(&tmp);
    return 1;
  }
  send_device(obj,tmp.value.string);
  clear_var(&tmp);
  tmp.type=INTEGER;
  tmp.value.integer=0;
  push(&tmp,rts);
  return 0;
}

int s_reconnect_device(struct object *caller, struct object *obj, struct object
                       *player, struct var_stack **rts) {
  struct var tmp;
  int result;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=1) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=OBJECT) {
    clear_var(&tmp);
    return 1;
  }
  if (!(obj->flags & PRIV)) {
    tmp.type=INTEGER;
    tmp.value.integer=1;
    push(&tmp,rts);
    return 0;
  }
  result=reconnect_device(obj,tmp.value.objptr);
  tmp.type=INTEGER;
  tmp.value.integer=result;
  push(&tmp,rts);
  return 0;
}

int s_disconnect_device(struct object *caller, struct object *obj,
                        struct object *player, struct var_stack **rts) {
  struct var tmp;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=0) return 1;
  disconnect_device(obj);
  tmp.type=INTEGER;
  tmp.value.integer=0;
  push(&tmp,rts);
  return 0;
}
