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
#include "cache.h"
#include "file.h"

int s_random(struct object *caller, struct object *obj, struct object
             *player, struct var_stack **rts) {
  struct var tmp;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=1) return 1;
  if (popint(&tmp,rts,obj)) return 1;
  if (tmp.value.integer<=0) {
    tmp.value.integer=0;
    push(&tmp,rts);
    return 0;
  }
  tmp.value.integer=(rand() % tmp.value.integer);
  push(&tmp,rts);
  return 0;
}

int s_time(struct object *caller, struct object *obj, struct object
           *player, struct var_stack **rts) {
  struct var tmp;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=0) return 1;
  tmp.type=INTEGER;
  tmp.value.integer=now_time;
  push(&tmp,rts);
  return 0;
}

int s_mktime(struct object *caller, struct object *obj, struct object
             *player, struct var_stack **rts) {
  struct var tmp;
  time_t int_time;
  char *buf;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=1) return 1;
  if (popint(&tmp,rts,obj)) return 1;
  int_time=int2time(tmp.value.integer);
  buf=ctime(&int_time);
  if (!buf) {
    tmp.type=INTEGER;
    tmp.value.integer=0;
    push(&tmp,rts);
    return 0;
  }
  tmp.type=STRING;
  tmp.value.string=buf;
  push(&tmp,rts);
  return 0;
}

int s_typeof(struct object *caller, struct object *obj, struct object
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
  result=tmp.type;
  clear_var(&tmp);
  tmp.type=INTEGER;
  tmp.value.integer=result;
  push(&tmp,rts);
  return 0;
}

int s_command(struct object *caller, struct object *obj, struct object
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
    tmp.type=STRING;
    tmp.value.string=copy_string("");
  }
  if (tmp.type!=STRING) {
    clear_var(&tmp);
    return 1;
  }
  while (obj->attacher) obj=obj->attacher;
  queue_command(obj,tmp.value.string);
  clear_var(&tmp);
  tmp.type=INTEGER;
  tmp.value.integer=0;
  push (&tmp,rts);
  return 0;
}

int s_sysctl(struct object *caller, struct object *obj, struct object *player,
             struct var_stack **rts) {
  struct var tmp;
  struct object *rcv,*tmpobj;
  unsigned int old_num_locals;
  struct var *old_locals;
  struct fns *tmp_fns;
  struct var_stack *arg_stack;
  char *buf;
  int num_args;
  struct alarmq *curr_alarmq;
  struct destq *curr_destq;
  struct cmdq *curr_cmdq;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num<1) {
    clear_var(&tmp);
    return 1;
  }
  num_args=tmp.value.num-1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=INTEGER) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.integer<0 || tmp.value.integer>11 ||
      ((!(obj->flags & PRIV)) && tmp.value.integer!=8)) {
    tmp.value.integer=1;
    push(&tmp,rts);
    return 0;
  }
  if (player)
    rcv=player;
  else
    rcv=obj;
  switch (tmp.value.integer) {
    case 0:
      if (num_args) return 1;
      if (player) {
        buf=MALLOC(strlen(player->parent->pathname)+strlen(obj->parent->
                   pathname)+(2*ITOA_BUFSIZ)+46);
        sprintf(buf," sysctl: user %s#%ld object %s#%ld attempting "
                "save",player->parent->pathname,
                (long) player->refno,obj->parent->pathname,
                (long) obj->refno);
      } else {
        buf=MALLOC(strlen(obj->parent->pathname)+ITOA_BUFSIZ+39);
        sprintf(buf," sysctl: object %s#%ld attempting save",
                obj->parent->pathname,(long) obj->refno);
      }
      logger(LOG_INFO, buf);
      FREE(buf);
      tmp.value.integer=save_db(save_name);
      push(&tmp,rts);
      if (!tmp.value.integer)
        logger(LOG_INFO, " sysctl: save complete");
      else
        logger(LOG_ERROR, " sysctl: save failed");
      return 0;
      break;
    case 1:
      if (num_args) return 1;
      if (player) {
        buf=MALLOC(strlen(player->parent->pathname)+strlen(obj->parent->
                   pathname)+(2*ITOA_BUFSIZ)+46);
        sprintf(buf," sysctl: user %s#%ld object %s#%ld attempting "
                "shutdown",player->parent->pathname,
                (long) player->refno,obj->parent->pathname,
                (long) obj->refno);
      } else {
        buf=MALLOC(strlen(obj->parent->pathname)+ITOA_BUFSIZ+39);
        sprintf(buf," sysctl: object %s#%ld attempting shutdown",
                obj->parent->pathname,(long) obj->refno);
      }
      logger(LOG_INFO, buf);
      FREE(buf);
      tmp.value.integer=save_db(save_name);
      if (tmp.value.integer) {
        logger(LOG_ERROR, " sysctl: shutdown failed");
        push(&tmp,rts);
        return 0;
      }
      shutdown_interface();
      logger(LOG_INFO, " sysctl: shutdown complete");
      exit(0);
      break;
    case 2:
      if (num_args) return 1;
      if (player) {
        buf=MALLOC(strlen(player->parent->pathname)+strlen(obj->parent->
                   pathname)+(2*ITOA_BUFSIZ)+46);
        sprintf(buf," sysctl: user %s#%ld object %s#%ld attempting "
                "panic",player->parent->pathname,
                (long) player->refno,obj->parent->pathname,
                (long) obj->refno);
      } else {
        buf=MALLOC(strlen(obj->parent->pathname)+ITOA_BUFSIZ+39);
        sprintf(buf," sysctl: object %s#%ld attempting panic",
                obj->parent->pathname,(long) obj->refno);
      }
      logger(LOG_INFO, buf);
      FREE(buf);
      tmp.value.integer=save_db(panic_name);
      if (tmp.value.integer) {
        push(&tmp,rts);
        logger(LOG_ERROR, " sysctl: panic failed");
        return 0;
      }
      shutdown_interface();
      logger(LOG_INFO, " sysctl: panic complete");
      exit(-1);
      break;
    case 3:
      if (num_args) return 1;
      tmp_fns=find_function("listen",rcv,&tmpobj);
      if (!tmp_fns) {
        tmp.value.integer=0;
        push(&tmp,rts);
        return 0;
      }
      curr_alarmq=alarm_list;
      old_locals=locals;
      old_num_locals=num_locals;
      while (curr_alarmq) {
        buf=MALLOC(strlen(curr_alarmq->funcname)+2*ITOA_BUFSIZ+4);
        sprintf(buf,"%ld %s %ld\n",(long) curr_alarmq->obj->refno,
                curr_alarmq->funcname,(long) curr_alarmq->delay);
        tmp.type=STRING;
        tmp.value.string=buf;
        arg_stack=NULL;
        pushnocopy(&tmp,&arg_stack);
        interp(obj,tmpobj,player,&arg_stack,tmp_fns);
        free_stack(&arg_stack);
        curr_alarmq=curr_alarmq->next;
      }
      locals=old_locals;
      num_locals=old_num_locals;
      tmp.type=INTEGER;
      tmp.value.integer=0;
      push(&tmp,rts);
      return 0;
      break;
    case 4:
      if (num_args) return 1;
      tmp_fns=find_function("listen",rcv,&tmpobj);
      if (!tmp_fns) {
        tmp.value.integer=0;
        push(&tmp,rts);
        return 0;
      }
      curr_cmdq=cmd_head;
      old_locals=locals;
      old_num_locals=num_locals;
      while (curr_cmdq) {
        buf=MALLOC(strlen(curr_cmdq->cmd)+ITOA_BUFSIZ+3);
        sprintf(buf,"%ld %s\n",(long) curr_cmdq->obj->refno,curr_cmdq->cmd);
        tmp.type=STRING;
        tmp.value.string=buf;
        arg_stack=NULL;
        pushnocopy(&tmp,&arg_stack);
        interp(obj,tmpobj,player,&arg_stack,tmp_fns);
        free_stack(&arg_stack);
        curr_cmdq=curr_cmdq->next;
      }
      locals=old_locals;
      num_locals=old_num_locals;
      tmp.type=INTEGER;
      tmp.value.integer=0;
      push(&tmp,rts);
      return 0;
      break;
    case 5:
      if (num_args) return 1;
      tmp_fns=find_function("listen",rcv,&tmpobj);
      if (!tmp_fns) {
        tmp.value.integer=0;
        push(&tmp,rts);
        return 0;
      }
      old_locals=locals;
      old_num_locals=num_locals;
      curr_destq=dest_list;
      while (curr_destq) {
        buf=MALLOC(ITOA_BUFSIZ+2);
        sprintf(buf,"%ld\n",(long) curr_destq->obj->refno);
        tmp.type=STRING;
        tmp.value.string=buf;
        arg_stack=NULL;
        pushnocopy(&tmp,&arg_stack);
        interp(obj,tmpobj,player,&arg_stack,tmp_fns);
        free_stack(&arg_stack);
        curr_destq=curr_destq->next;
      }
      locals=old_locals;
      num_locals=old_num_locals;
      tmp.type=INTEGER;
      tmp.value.integer=0;
      push(&tmp,rts);
      return 0;
      break;
    case 6:
      use_hard_cycles=0;
      tmp.type=INTEGER;
      tmp.value.integer=0;
      push(&tmp,rts);
      return 0;
      break;
    case 7:
      use_soft_cycles=0;
      tmp.type=INTEGER;
      tmp.value.integer=0;
      push(&tmp,rts);
      return 0;
      break;
    case 8:
      tmp.type=INTEGER;
      tmp.value.integer=CI_VERSION;
      push(&tmp,rts);
      return 0;
      break;
    case 9:
      /* Get/set max call stack depth */
      if (num_args==0) {
        /* Get current value */
        tmp.type=INTEGER;
        tmp.value.integer=max_call_stack_depth;
        push(&tmp,rts);
        return 0;
      } else if (num_args==1) {
        /* Set new value */
        if (pop(&tmp,rts,obj)) return 1;
        if (tmp.type!=INTEGER) {
          clear_var(&tmp);
          return 1;
        }
        if (tmp.value.integer<10 || tmp.value.integer>1000) {
          /* Enforce reasonable limits: 10-1000 */
          tmp.value.integer=1;
          push(&tmp,rts);
          return 0;
        }
        max_call_stack_depth=tmp.value.integer;
        tmp.value.integer=0;
        push(&tmp,rts);
        return 0;
      }
      return 1;
      break;
    case 10:
      /* Get/set max trace depth */
      if (num_args==0) {
        /* Get current value */
        tmp.type=INTEGER;
        tmp.value.integer=max_trace_depth;
        push(&tmp,rts);
        return 0;
      } else if (num_args==1) {
        /* Set new value */
        if (pop(&tmp,rts,obj)) return 1;
        if (tmp.type!=INTEGER) {
          clear_var(&tmp);
          return 1;
        }
        if (tmp.value.integer<5 || tmp.value.integer>100) {
          /* Enforce reasonable limits: 5-100 */
          tmp.value.integer=1;
          push(&tmp,rts);
          return 0;
        }
        max_trace_depth=tmp.value.integer;
        tmp.value.integer=0;
        push(&tmp,rts);
        return 0;
      }
      return 1;
      break;
    case 11:
      /* Get/set trace format (0=detailed, 1=compact) */
      if (num_args==0) {
        /* Get current value */
        tmp.type=INTEGER;
        tmp.value.integer=trace_format;
        push(&tmp,rts);
        return 0;
      } else if (num_args==1) {
        /* Set new value */
        if (pop(&tmp,rts,obj)) return 1;
        if (tmp.type!=INTEGER) {
          clear_var(&tmp);
          return 1;
        }
        if (tmp.value.integer!=0 && tmp.value.integer!=1) {
          /* Only 0 or 1 allowed */
          tmp.value.integer=1;
          push(&tmp,rts);
          return 0;
        }
        trace_format=tmp.value.integer;
        tmp.value.integer=0;
        push(&tmp,rts);
        return 0;
      }
      return 1;
      break;
    default:
      return 1;
      break;
  }
}
