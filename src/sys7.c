#include "config.h"
#include "object.h"
#include "instr.h"
#include "interp.h"
#include "intrface.h"
#include "globals.h"
#include "cache.h"
#include "protos.h"
#include "constrct.h"
#include "table.h"

#define FNS_HASH_SIZE 16

struct fns_hash_item {
  struct proto *parent;
  struct fns *func;
};

struct fns_hash_item fns_hash_table[FNS_HASH_SIZE];

void clear_fns_hash() {
  int loop;

  loop=0;
  while (loop<FNS_HASH_SIZE)
    fns_hash_table[loop++].parent=NULL;
}

struct fns *hash_find_fns(char *name, struct object *obj,
                          struct object **real_obj) {
  int index;
  struct fns *tmpfns;

  index=obj->parent->proto_obj->refno%FNS_HASH_SIZE;
  if (fns_hash_table[index].parent==obj->parent) {
    (*real_obj)=obj;
    return fns_hash_table[index].func;
  }
  tmpfns=find_function(name,obj,real_obj);
  if (obj==(*real_obj)) {
    fns_hash_table[index].parent=obj->parent;
    return fns_hash_table[index].func=tmpfns;
  }
  return tmpfns;
}

int s_iterate(struct object *caller, struct object *obj, struct object
              *player, struct var_stack **rts) {
  struct var *arglist,*old_locals;
  struct fns *tmp_fns;
  struct var_stack *arg_stack;
  long num_args,loop;
  struct var tmp;
  char *name;
  struct object *curr_obj,*avoid1,*avoid2,*tmpobj;
  unsigned int old_num_locals;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num<4) return 1;
  num_args=tmp.value.num-4;
  if (num_args)
    arglist=MALLOC(sizeof(struct var)*num_args);
  else
    arglist=NULL;
  loop=0;
  while (loop<num_args) {
    if (pop(&tmp,rts,obj)) {
      while (loop>0)
        clear_var(&(arglist[--loop]));
      if (arglist) FREE(arglist);
      return 1;
    }
    arglist[loop].type=tmp.type;
    switch (tmp.type) {
      case INTEGER:
        arglist[loop].value.integer=tmp.value.integer;
        break;
      case STRING:
        arglist[loop].value.string=tmp.value.string;
        break;
      case OBJECT:
        arglist[loop].value.objptr=tmp.value.objptr;
        break;
      default:
        clear_var(&tmp);
        arglist[loop].type=INTEGER;
        arglist[loop].value.integer=0;
    }
    tmp.type=INTEGER;
    tmp.value.integer=0;
    loop++;
  }
  if (pop(&tmp,rts,obj)) {
    loop=0;
    while (loop<num_args) clear_var(&(arglist[loop++]));
    if (arglist) FREE(arglist);
    return 1;
  }
  if (tmp.type!=STRING) {
    clear_var(&tmp);
    loop=0;
    while (loop<num_args) clear_var(&(arglist[loop++]));
    if (arglist) FREE(arglist);
    return 1;
  }
  name=tmp.value.string;
  if (pop(&tmp,rts,obj)) {
    FREE(name);
    loop=0;
    while (loop<num_args) clear_var(&(arglist[loop++]));
    if (arglist) FREE(arglist);
    return 1;
  }
  if (tmp.type==INTEGER && tmp.value.integer==0) {
    tmp.type=OBJECT;
    tmp.value.objptr=NULL;
  }
  if (tmp.type!=OBJECT) {
    clear_var(&tmp);
    FREE(name);
    loop=0;
    while (loop<num_args) clear_var(&(arglist[loop++]));
    if (arglist) FREE(arglist);
    return 1;
  }
  avoid1=tmp.value.objptr;
  if (pop(&tmp,rts,obj)) {
    FREE(name);
    loop=0;
    while (loop<num_args) clear_var(&(arglist[loop++]));
    if (arglist) FREE(arglist);
    return 1;
  }
  if (tmp.type==INTEGER && tmp.value.integer==0) {
    tmp.type=OBJECT;
    tmp.value.objptr=NULL;
  }
  if (tmp.type!=OBJECT) {
    clear_var(&tmp);
    FREE(name);
    loop=0;
    while (loop<num_args) clear_var(&(arglist[loop++]));
    if (arglist) FREE(arglist);
    return 1;
  }
  avoid2=tmp.value.objptr;
  if (pop(&tmp,rts,obj)) {
    FREE(name);
    loop=0;
    while (loop<num_args) clear_var(&(arglist[loop++]));
    if (arglist) FREE(arglist);
    return 1;
  }
  if (tmp.type==INTEGER && tmp.value.integer==0) {
    tmp.type=OBJECT;
    tmp.value.objptr=NULL;
  }
  if (tmp.type!=OBJECT) {
    clear_var(&tmp);
    FREE(name);
    loop=0;
    while (loop<num_args) clear_var(&(arglist[loop++]));
    if (arglist) FREE(arglist);
    return 1;
  }
  curr_obj=tmp.value.objptr;
  old_locals=locals;
  old_num_locals=num_locals;
  clear_fns_hash();
  while (curr_obj) {
    if (curr_obj!=avoid1 && curr_obj!=avoid2) {
      tmp_fns=hash_find_fns(name,curr_obj,&tmpobj);
      if (tmp_fns) if (tmp_fns->is_static) tmp_fns=NULL;
      if (tmp_fns) {
        arg_stack=NULL;
        loop=num_args;
        while (loop-->0)
          push(&(arglist[loop]),&arg_stack);    
        tmp.type=NUM_ARGS;
        tmp.value.num=num_args;
        push(&tmp,&arg_stack);
        interp(obj,tmpobj,player,&arg_stack,tmp_fns);
        free_stack(&arg_stack);
      }
    }
    curr_obj=curr_obj->next_object;
  }
  locals=old_locals;
  num_locals=old_num_locals;
  loop=0;
  while (loop<num_args) clear_var(&(arglist[loop++]));
  if (arglist) FREE(arglist);
  tmp.type=INTEGER;
  tmp.value.integer=0;
  push(&tmp,rts);
  return 0;
}

int s_next_who(struct object *caller, struct object *obj, struct object
               *player, struct var_stack **rts) {
  struct var tmp;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num>1) return 1;
  if (tmp.value.num) {
    if (pop(&tmp,rts,obj)) return 1;
    if (tmp.type==INTEGER && tmp.value.integer==0) {
      tmp.type=OBJECT;
      tmp.value.objptr=NULL;
    }
    if (tmp.type!=OBJECT) {
      clear_var(&tmp);
      return 1;
    }
  } else {
    tmp.type=OBJECT;
    tmp.value.objptr=NULL;
  }
  tmp.value.objptr=next_who(tmp.value.objptr);
  if (!tmp.value.objptr) {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  }
  push(&tmp,rts);
  return 0;
}

int s_get_devidle(struct object *caller, struct object *obj,
                  struct object *player, struct var_stack **rts) {
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
  tmp.type=INTEGER;
  tmp.value.integer=get_devidle(tmp.value.objptr);
  push(&tmp,rts);
  return 0;
}

int s_get_conntime(struct object *caller, struct object *obj,
                   struct object *player, struct var_stack **rts) {
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
  tmp.type=INTEGER;
  tmp.value.integer=get_conntime(tmp.value.objptr);
  push(&tmp,rts);
  return 0;
}

int s_connect_device(struct object *caller, struct object *obj,
                     struct object *player, struct var_stack **rts) {
  struct var tmp;
  int port,retval;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=2) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=INTEGER) {
    clear_var(&tmp);
    return 1;
  }
  port=tmp.value.integer;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=STRING) {
    clear_var(&tmp);
    return 1;
  }
  if (obj->flags & PRIV)
    retval=connect_device(obj,tmp.value.string,port,0);
  else
    retval=0;
  clear_var(&tmp);
  tmp.type=INTEGER;
  tmp.value.integer=retval;
  push(&tmp,rts);
  return 0;
}

int s_flush_device(struct object *caller, struct object *obj,
                   struct object *player, struct var_stack **rts) {
  struct var tmp;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num==0) {
    if (obj->flags & PRIV) {
      flush_device(NULL);
      tmp.type=INTEGER;
      tmp.value.integer=0;
      push(&tmp,rts);
      return 0;
    } else {
      tmp.type=INTEGER;
      tmp.value.integer=1;
      push(&tmp,rts);
      return 0;
    }
  }
  if (tmp.value.num!=1) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=OBJECT) {
    clear_var(&tmp);
    return 1;
  }
  if (obj->flags & PRIV) {
    flush_device(tmp.value.objptr);
    tmp.type=INTEGER;
    tmp.value.integer=0;
    push(&tmp,rts);
    return 0;
  }
  tmp.type=INTEGER;
  tmp.value.integer=1;
  push(&tmp,rts);
  return 0;
}

int is_attached_to(struct object *slave, struct object *master) {
  struct attach_list *curr_attach;

  if (slave==master) return 1;
  curr_attach=master->attachees;
  while (curr_attach) {
    if (is_attached_to(slave,curr_attach->attachee)) return 1;
    curr_attach=curr_attach->next;
  }
  return 0;
}

int s_attach(struct object *caller, struct object *obj,
             struct object *player, struct var_stack **rts) {
  struct var tmp,tmp2;
  struct object *tmpobj;
  struct fns *func;
  struct var_stack *rts2;
  struct var *old_locals;
  unsigned int old_num_locals;
  struct attach_list *attachptr;

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
  if (tmp.value.objptr->attacher || tmp.value.objptr==obj ||
      is_attached_to(obj,tmp.value.objptr)) {
    tmp.type=INTEGER;
    tmp.value.integer=1;
    push(&tmp,rts);
    return 0;
  }
  func=find_function("allow_attach",tmp.value.objptr,&tmpobj);
  if (func) {
    tmp2.type=NUM_ARGS;
    tmp2.value.num=0;
    rts2=NULL;
    push(&tmp2,&rts2);
    old_locals=locals;
    old_num_locals=num_locals;
    if (interp(obj,tmpobj,player,&rts2,func)) {
      locals=old_locals;
      num_locals=old_num_locals;
      free_stack(&rts2);
      tmp.type=INTEGER;
      tmp.value.integer=1;
      push(&tmp,rts);
      return 0;
    }
    locals=old_locals;
    num_locals=old_num_locals;
    if (pop(&tmp2,&rts2,tmpobj)) {
      tmp.type=INTEGER;
      tmp.value.integer=1;
      push(&tmp,rts);
      return 0;
    }
    free_stack(&rts2);
    if (tmp2.type!=INTEGER || tmp2.value.integer==0) {
      tmp.type=INTEGER;
      tmp.value.integer=1;
      clear_var(&tmp2);
      push(&tmp,rts);
      return 0;
    }
    attachptr=MALLOC(sizeof(struct attach_list));
    attachptr->attachee=tmp.value.objptr;
    attachptr->next=obj->attachees;
    obj->attachees=attachptr;
    tmp.value.objptr->attacher=obj;
    tmp.type=INTEGER;
    tmp.value.integer=0;
    push(&tmp,rts);
    return 0;
  }
  tmp.type=INTEGER;
  tmp.value.integer=1;
  push(&tmp,rts);
  return 0;
}

int s_this_component(struct object *caller, struct object *obj,
                     struct object *player, struct var_stack **rts) {
  struct var tmp;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=0) return 1;
  if (obj) {
    tmp.type=OBJECT;
    tmp.value.objptr=obj;
  } else {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  }
  push(&tmp,rts);
  return 0;
}

int s_detach(struct object *caller, struct object *obj,
             struct object *player, struct var_stack **rts) {
  struct var tmp;
  struct attach_list *curr_attach,*prev_attach;

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
  if (tmp.value.objptr->attacher!=obj) {
    tmp.type=INTEGER;
    tmp.value.integer=1;
    push(&tmp,rts);
    return 0;
  }
  tmp.value.objptr->attacher=NULL;
  prev_attach=NULL;
  curr_attach=obj->attachees;
  while (curr_attach) {
    if (curr_attach->attachee==tmp.value.objptr) break;
    prev_attach=curr_attach;
    curr_attach=curr_attach->next;
  }
  if (curr_attach) {
    if (prev_attach) prev_attach->next=curr_attach->next;
    else obj->attachees=curr_attach->next;
    FREE(curr_attach);
  }
  tmp.type=INTEGER;
  tmp.value.integer=0;
  push(&tmp,rts);
  return 0;
}

int s_table_get(struct object *caller, struct object *obj,
                struct object *player, struct var_stack **rts) {
  struct var tmp;
  char *datum;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=1) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=STRING) {
    clear_var(&tmp);
    return 1;
  }
  datum=get_from_table(tmp.value.string);
  clear_var(&tmp);
  if (datum) {
    tmp.type=STRING;
    tmp.value.string=copy_string(datum);
  } else {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  }
  pushnocopy(&tmp,rts);
  return 0;
}

int s_table_set(struct object *caller, struct object *obj,
                struct object *player, struct var_stack **rts) {
  struct var tmp1,tmp2;
  char *key,*datum;

  if (pop(&tmp1,rts,obj)) return 1;
  if (tmp1.type!=NUM_ARGS) {
    clear_var(&tmp1);
    return 1;
  }
  if (tmp1.value.num!=2) return 1;
  if (pop(&tmp1,rts,obj)) return 1;
  if (tmp1.type!=STRING && !(tmp1.type==INTEGER && tmp1.value.integer==0)) {
    clear_var(&tmp1);
    return 1;
  }
  if (tmp1.type==STRING) datum=tmp1.value.string;
  else datum=NULL;
  if (pop(&tmp2,rts,obj)) return 1;
  if (tmp2.type!=STRING) {
    clear_var(&tmp1);
    clear_var(&tmp2);
    return 1;
  }
  if (!(obj->flags & PRIV)) {
    clear_var(&tmp1);
    clear_var(&tmp2);
    tmp1.type=INTEGER;
    tmp1.value.integer=1;
    push(&tmp1,rts);
    return 0;
  }
  key=tmp2.value.string;
  add_to_table(key,datum);
  clear_var(&tmp1);
  clear_var(&tmp2);
  tmp1.type=INTEGER;
  tmp1.value.integer=0;
  push(&tmp1,rts);
  return 0;
}

int s_table_delete(struct object *caller, struct object *obj,
                   struct object *player, struct var_stack **rts) {
  struct var tmp;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=1) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=STRING) {
    clear_var(&tmp);
    return 1;
  }
  if (!(obj->flags & PRIV)) {
    clear_var(&tmp);
    tmp.type=INTEGER;
    tmp.value.integer=1;
    push(&tmp,rts);
    return 0;
  }
  delete_from_table(tmp.value.string);
  clear_var(&tmp);
  tmp.type=INTEGER;
  tmp.value.integer=0;
  push(&tmp,rts);
  return 0;
}
