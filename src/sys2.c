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

int s_set_interactive(struct object *caller, struct object *obj, struct object
                      *player, struct var_stack **rts) {
  struct var tmp;
  int bool;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=1) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  bool=1;
  if (tmp.type==INTEGER) if (tmp.value.integer==0) bool=0;
  clear_var(&tmp);
  if (bool) obj->flags|=INTERACTIVE;
  else obj->flags&=~INTERACTIVE;
  tmp.type=INTEGER;
  tmp.value.integer=0;
  push(&tmp,rts);
  return 0;
}

int s_interactive(struct object *caller, struct object *obj, struct object
                  *player, struct var_stack **rts) {
  struct var tmp;

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
  if (tmp.value.objptr->flags & INTERACTIVE) {
    tmp.type=INTEGER;
    tmp.value.integer=1;
  } else {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  }
  push(&tmp,rts);
  return 0;
}

int s_set_priv(struct object *caller, struct object *obj, struct object
               *player, struct var_stack **rts) {
  struct var tmp;
  int bool;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=2) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  bool=1;
  if (tmp.type==INTEGER) if (tmp.value.integer==0) bool=0;
  clear_var(&tmp);
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=OBJECT) {
    clear_var(&tmp);
    return 1;
  }
  if ((!(obj->flags & PRIV)) || (tmp.value.objptr->refno==0)) {
    tmp.type=INTEGER;
    tmp.value.integer=1;
    push(&tmp,rts);
    return 0;
  }
  if (bool) tmp.value.objptr->flags|=PRIV;
  else tmp.value.objptr->flags&=~PRIV;
  tmp.type=INTEGER;
  tmp.value.integer=0;
  push(&tmp,rts);
  return 0;
}

int s_priv(struct object *caller, struct object *obj, struct object
           *player, struct var_stack **rts) {
  struct var tmp;

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
  if (tmp.value.objptr->flags & PRIV) {
    tmp.type=INTEGER;
    tmp.value.integer=1;
  } else {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  }
  push(&tmp,rts);
  return 0;
}

int s_in_editor(struct object *caller, struct object *obj, struct object
                  *player, struct var_stack **rts) {
  struct var tmp;

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
  if (tmp.value.objptr->flags & IN_EDITOR) {
    tmp.type=INTEGER;
    tmp.value.integer=1;
  } else {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  }
  push(&tmp,rts);
  return 0;
}

int s_connected(struct object *caller, struct object *obj, struct object
                *player, struct var_stack **rts) {
  struct var tmp;

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
  if (tmp.value.objptr->flags & CONNECTED) {
    tmp.type=INTEGER;
    tmp.value.integer=1;
  } else {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  }
  push(&tmp,rts);
  return 0;
}

int s_prototype(struct object *caller, struct object *obj, struct object
                *player, struct var_stack **rts) {
  struct var tmp;

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
  if (tmp.value.objptr->flags & PROTOTYPE) {
    tmp.type=INTEGER;
    tmp.value.integer=1;
  } else {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  }
  push(&tmp,rts);
  return 0;
}

/* set_heart_beat(interval)
 * Enable or disable heartbeat on this_object()
 * interval: heartbeat interval in seconds (0 = disable, >0 = enable)
 */
int s_set_heart_beat(struct object *caller, struct object *obj, struct object
                     *player, struct var_stack **rts) {
  struct var tmp;
  int interval;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=1) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  
  if (tmp.type!=INTEGER) {
    clear_var(&tmp);
    return 1;
  }
  
  interval = tmp.value.integer;
  clear_var(&tmp);
  
  /* Set or clear heartbeat interval */
  if (interval <= 0) {
    obj->heart_beat_interval = 0;
    obj->last_heart_beat = 0;
  } else {
    obj->heart_beat_interval = interval;
    obj->last_heart_beat = now_time;  /* Start immediately */
  }
  
  tmp.type=INTEGER;
  tmp.value.integer=0;
  push(&tmp,rts);
  return 0;
}
