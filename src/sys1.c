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

int s_add_verb(struct object *caller, struct object *obj, struct object
               *player, struct var_stack **rts) {
  struct var tmp,tmp2;
  struct verb *new_verb;
  struct fns *tmp_fns;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=2) {
    return 1;
  }
  if (pop(&tmp,rts,obj)) {
    return 1;
  }
  if (tmp.type!=STRING) {
    clear_var(&tmp);
    return 1;
  }
  if (pop(&tmp2,rts,obj)) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp2.type==INTEGER && tmp2.value.integer==0) {
    tmp2.type=STRING;
    tmp2.value.string=copy_string("");
  }
  if (tmp2.type!=STRING) {
    clear_var(&tmp);
    clear_var(&tmp2);
    return 1;
  }
  if ((strlen(tmp2.value.string)+1)>MAX_STR_LEN) {
    clear_var(&tmp);
    clear_var(&tmp2);
    tmp.type=INTEGER;
    tmp.value.integer=1;
    push(&tmp,rts);
    return 0;
  }
  tmp_fns=find_function(tmp.value.string,obj,NULL);
  if (!tmp_fns) {
    clear_var(&tmp);
    clear_var(&tmp2);
    tmp.type=INTEGER;
    tmp.value.num=1;
    push(&tmp,rts);
    return 0;
  }
  remove_verb(obj,tmp2.value.string);
  new_verb=(struct verb *) MALLOC(sizeof(struct verb));
  new_verb->verb_name=tmp2.value.string;
  new_verb->is_xverb=0;
  new_verb->function=tmp.value.string;
  new_verb->next=obj->verb_list;
  obj->verb_list=new_verb;
  tmp.type=INTEGER;
  tmp.value.num=0;
  push(&tmp,rts);
  return 0;
}

int s_add_xverb(struct object *caller, struct object *obj, struct object
               *player, struct var_stack **rts) {
  struct var tmp,tmp2;
  struct verb *new_verb;
  struct fns *tmp_fns;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=2) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=STRING) {
    clear_var(&tmp);
    return 1;
  }
  if (pop(&tmp2,rts,obj)) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp2.type==INTEGER && tmp2.value.integer==0) {
    tmp2.type=STRING;
    tmp2.value.string=copy_string("");
  }
  if (tmp2.type!=STRING) {
    clear_var(&tmp);
    clear_var(&tmp2);
    return 1;
  }
  if ((strlen(tmp2.value.string)+1)>MAX_STR_LEN) {
    clear_var(&tmp);
    clear_var(&tmp2);
    tmp.type=INTEGER;
    tmp.value.integer=1;
    push(&tmp,rts);
    return 0;
   }
  tmp_fns=find_function(tmp.value.string,obj,NULL);
  if (!tmp_fns) {
    clear_var(&tmp);
    clear_var(&tmp2);
    tmp.type=INTEGER;
    tmp.value.num=1;
    push(&tmp,rts);
    return 0;
  }
  remove_verb(obj,tmp2.value.string);
  new_verb=(struct verb *) MALLOC(sizeof(struct verb));
  new_verb->verb_name=tmp2.value.string;
  new_verb->is_xverb=1;
  new_verb->function=tmp.value.string;
  new_verb->next=obj->verb_list;
  obj->verb_list=new_verb;
  tmp.type=INTEGER;
  tmp.value.num=0;
  push(&tmp,rts);
  return 0;
}

int s_call_other(struct object *caller, struct object *obj, struct object
                 *player, struct var_stack **rts) {
  struct var tmp1,tmp2;
  struct var_stack *arg_stack;
  struct fns *tmp_fns;
  struct var *old_locals;
  unsigned int old_num_locals;
  struct object *tmpobj;

  if (pop(&tmp1,rts,obj)) return 1;
  if (tmp1.type!=NUM_ARGS) {
    clear_var(&tmp1);
    return 1;
  }
  if (tmp1.value.num<2) return 1;
  tmp1.value.num-=2;
  push(&tmp1,rts);
  if (!(arg_stack=gen_stack(rts,obj))) return 1;
  if (pop(&tmp2,rts,obj)) {
    free_stack(&arg_stack);
    return 1;
  }
  if (pop(&tmp1,rts,obj)) {
    clear_var(&tmp2);
    free_stack(&arg_stack);
    return 1;
  }
  if (tmp1.type!=OBJECT || tmp2.type!=STRING) {
    clear_var(&tmp1);
    clear_var(&tmp2);
    free_stack(&arg_stack);
    return 1;
  }
  
  /* Check if object is destructed/garbage */
  if (!tmp1.value.objptr || (tmp1.value.objptr->flags & GARBAGE)) {
    clear_var(&tmp1);
    clear_var(&tmp2);
    free_stack(&arg_stack);
    tmp1.type=INTEGER;
    tmp1.value.integer=0;
    push(&tmp1,rts);
    return 0;
  }
  
  tmp_fns=find_function(tmp2.value.string,tmp1.value.objptr,&tmpobj);
  clear_var(&tmp2);
  
  if (!tmp_fns) {
    free_stack(&arg_stack);
    tmp1.type=INTEGER;
    tmp1.value.integer=0;
    push(&tmp1,rts);
    return 0;
  }
  
  /* Verify tmpobj is valid - find_function might return a function from a GARBAGE object */
  if (!tmpobj || (tmpobj->flags & GARBAGE) || !tmpobj->parent || !tmpobj->parent->funcs) {
    free_stack(&arg_stack);
    tmp1.type=INTEGER;
    tmp1.value.integer=0;
    push(&tmp1,rts);
    return 0;
  }
  
  if (tmp_fns->is_static) {
    free_stack(&arg_stack);
    tmp1.type=INTEGER;
    tmp1.value.integer=0;
    push(&tmp1,rts);
    return 0;
  }
  old_locals=locals;
  old_num_locals=num_locals;
  if (interp(obj,tmpobj,player,&arg_stack,tmp_fns)) {
    locals=old_locals;
    num_locals=old_num_locals;
    free_stack(&arg_stack);
    tmp1.type=INTEGER;
    tmp1.value.integer=0;
    push(&tmp1,rts);
    return 0;
  }
  
  locals=old_locals;
  num_locals=old_num_locals;
  if (pop(&tmp1,&arg_stack,tmpobj)) {
    tmp1.type=INTEGER;
    tmp1.value.integer=0;
  }
  free_stack(&arg_stack);
  push(&tmp1,rts);
  return 0;
}

int s_alarm(struct object *caller, struct object *obj, struct object
            *player, struct var_stack **rts) {
  struct var tmp1,tmp2;

  if (pop(&tmp1,rts,obj)) return 1;
  if (tmp1.type!=NUM_ARGS) {
    clear_var(&tmp1);
    return 1;
  }
  if (tmp1.value.num!=2) return 1;
  if (pop(&tmp2,rts,obj)) return 1;
  if (pop(&tmp1,rts,obj)) {
    clear_var(&tmp2);
    return 1;
  }
  if (tmp1.type!=INTEGER || tmp2.type!=STRING) {
    clear_var(&tmp1);
    clear_var(&tmp2);
    return 1;
  }
  if (!find_function(tmp2.value.string,obj,NULL) || tmp1.value.integer<0) {
    clear_var(&tmp2);
    tmp1.value.integer=1;
    push(&tmp1,rts);
    return 0;
  }
  queue_for_alarm(obj,tmp1.value.integer,tmp2.value.string);
  clear_var(&tmp2);
  tmp1.value.integer=0;
  push(&tmp1,rts);
  return 0;
}

int s_remove_alarm(struct object *caller, struct object *obj, struct object
                   *player, struct var_stack **rts) {
  struct var tmp;
  signed long tmpint;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num==0) {
    tmp.type=INTEGER;
    tmp.value.integer=remove_alarm(obj,NULL);
    push(&tmp,rts);
    return 0;
  } else
    if (tmp.value.num==1) {
      if (pop(&tmp,rts,obj)) return 1;
      if (tmp.type!=STRING) {
        clear_var(&tmp);
        return 1;
      }
      tmpint=remove_alarm(obj,tmp.value.string);
      clear_var(&tmp);
      tmp.type=INTEGER;
      tmp.value.integer=tmpint;
      push(&tmp,rts);
      return 0;
    }
  return 1;
}

int s_caller_object(struct object *caller, struct object *obj, struct object
                    *player, struct var_stack **rts) {
  struct var tmp;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=0) return 1;
  tmp.type=OBJECT;
  tmp.value.objptr=caller;
  if (!caller) {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  }
  push(&tmp,rts);
  return 0;
}

int s_clone_object(struct object *caller, struct object *obj, struct object
                   *player, struct var_stack **rts) {
  struct var tmp,tmp2,tmp3;
  struct object *tmpobj,*tmpobj2;
  struct code *newcode;
  unsigned int line;
  struct var_stack *arg_stack;
  struct fns *tmpfns;
  struct var *old_locals;
  struct proto *tmp_proto;
  unsigned int old_num_locals;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=1) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=STRING && tmp.type!=OBJECT) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.type==STRING) {
    if (!(tmpobj=find_proto(tmp.value.string))) {
      line=parse_code(tmp.value.string,obj,&newcode);
      if (line==((unsigned int) -1)) {
        clear_var(&tmp);
        tmp.type=INTEGER;
        tmp.value.integer=0;
        push(&tmp,rts);
        return 0;
      }
      if (line) {
        compile_error(player,tmp.value.string,line);
        clear_var(&tmp);
        tmp.type=INTEGER;
        tmp.value.integer=0;
        push(&tmp,rts);
        return 0;
      }
      tmpobj=newobj();
      tmp_proto=(struct proto *) MALLOC(sizeof(struct proto));
      tmpobj->flags|=PROTOTYPE;
      tmpobj->parent=tmp_proto;
      tmpobj->parent->funcs=newcode;
      tmpobj->parent->inherits=newcode->inherits;  /* Copy inherits from code */
      tmpobj->parent->next_proto=ref_to_obj(0)->parent->next_proto;
      ref_to_obj(0)->parent->next_proto=tmpobj->parent;
      tmpobj->parent->pathname=tmp.value.string;
      tmpobj->parent->proto_obj=tmpobj;
      if (newcode->num_globals) {
        tmpobj->globals=(struct var *) MALLOC(sizeof(struct var)*newcode->
                                              num_globals);
        line=0;
        while (line<newcode->num_globals) {
          tmpobj->globals[line].type=INTEGER;
          tmpobj->globals[line].value.integer=0;
          line++;
        }
      }
      add_loaded(tmpobj);
      tmpobj->obj_state=DIRTY;
      tmpfns=find_function("init",tmpobj,&tmpobj2);
      if (tmpfns) {
        tmp2.type=NUM_ARGS;
        tmp2.value.num=0;
        arg_stack=NULL;
        push(&tmp2,&arg_stack);
        old_locals=locals;
        old_num_locals=num_locals;
        interp(obj,tmpobj2,player,&arg_stack,tmpfns);
        locals=old_locals;
        num_locals=old_num_locals;
        free_stack(&arg_stack);
      }
    } else
      clear_var(&tmp);
    tmp.type=OBJECT;
    tmp.value.objptr=tmpobj;
  }
  tmpobj=newobj();
  tmpobj->parent=tmp.value.objptr->parent;
  tmpobj->next_child=tmpobj->parent->proto_obj->next_child;
  tmpobj->parent->proto_obj->next_child=tmpobj;
  if (tmpobj->parent->funcs->num_globals) {
    tmpobj->globals=(struct var *) MALLOC(sizeof(struct var)*tmpobj->parent->
                                          funcs->num_globals);
    line=0;
    while (line<tmpobj->parent->funcs->num_globals) {
     tmpobj->globals[line].type=INTEGER;
     tmpobj->globals[line].value.integer=0;
     line++;
    }
  }
  add_loaded(tmpobj);
  tmpobj->obj_state=DIRTY;
  tmpfns=find_function("init",tmpobj,&tmpobj2);
  if (tmpfns) {
    arg_stack=NULL;
    old_locals=locals;
    old_num_locals=num_locals;
    tmp3.type=NUM_ARGS;
    tmp3.value.num=0;
    push(&tmp3,&arg_stack);
    interp(obj,tmpobj2,player,&arg_stack,tmpfns);
    locals=old_locals;
    num_locals=old_num_locals;
    free_stack(&arg_stack);
  }
  
  /* Attach auto object to the newly cloned object */
  attach_auto_to(tmpobj);
  
  tmp.value.objptr=tmpobj;
  push(&tmp,rts);
  return 0;
}

int s_destruct(struct object *caller, struct object *obj, struct object
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
  if (tmp.value.objptr!=obj && (!(obj->flags & PRIV))) {
    tmp.type=INTEGER;
    tmp.value.integer=1;
    push(&tmp,rts);
    return 0;
  }
  if (tmp.value.objptr==ref_to_obj(0)) {
    tmp.type=INTEGER;
    tmp.value.integer=1;
    push(&tmp,rts);
    return 0;
  }
  queue_for_destruct(tmp.value.objptr);
  tmp.type=INTEGER;
  tmp.value.integer=0;
  push(&tmp,rts);
  return 0;
}

int s_contents(struct object *caller, struct object *obj, struct object
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
  if (tmp.value.objptr->contents) {
    tmp.value.objptr=tmp.value.objptr->contents;
  } else {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  }
  push(&tmp,rts);
  return 0;
}

int s_next_object(struct object *caller, struct object *obj, struct object
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
  if (tmp.value.objptr->next_object) {
    tmp.value.objptr=tmp.value.objptr->next_object;
  } else {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  }
  push(&tmp,rts);
  return 0;
}

int s_location(struct object *caller, struct object *obj, struct object
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
  if (tmp.value.objptr->location) {
    tmp.value.objptr=tmp.value.objptr->location;
  } else {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  }
  push(&tmp,rts);
  return 0;
}

int s_next_child(struct object *caller, struct object *obj, struct object
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
  if (tmp.value.objptr->next_child) {
    tmp.value.objptr=tmp.value.objptr->next_child;
  } else {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  }
  push(&tmp,rts);
  return 0;
}

int s_parent(struct object *caller, struct object *obj, struct object
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
  if (tmp.value.objptr->parent) {
    tmp.value.objptr=tmp.value.objptr->parent->proto_obj;
  } else {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  }
  push(&tmp,rts);
  return 0;
}

int s_next_proto(struct object *caller, struct object *obj, struct object
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
  if (tmp.value.objptr->parent->next_proto) {
    tmp.value.objptr=tmp.value.objptr->parent->next_proto->proto_obj;
  } else {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  }
  push(&tmp,rts);
  return 0;
}

int s_move_object(struct object *caller, struct object *obj, struct object
                  *player, struct var_stack **rts) {
  struct var tmp;
  struct object *item,*dest,*prev,*curr;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=2) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=OBJECT && (tmp.type!=INTEGER || tmp.value.integer!=0)) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.type==OBJECT)
    dest=tmp.value.objptr;
  else
    dest=NULL;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=OBJECT && (tmp.type!=INTEGER || tmp.value.integer!=0)) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.type==OBJECT)
    item=tmp.value.objptr;
  else {
    tmp.type=INTEGER;
    tmp.value.integer=1;
    push(&tmp,rts);
    return 0;
  }
  if (dest) {
    curr=dest;
    while (curr) {
      if (curr==item) {
        tmp.type=INTEGER;
        tmp.value.integer=1;
        push(&tmp,rts);
        return 0;
      }
      curr=curr->location;
    }
  }
  if (item->location) {
    curr=item->location->contents;
    if (curr==item)
      item->location->contents=item->next_object;
    else
      while (curr) {
        prev=curr;
        curr=curr->next_object;
        if (curr==item) {
          prev->next_object=curr->next_object;
          break;
        }
      }
  }
  item->next_object=NULL;
  item->location=dest;
  if (dest) {
    item->next_object=dest->contents;
    dest->contents=item;
  }
  
  /* Update access time for moved object and destination
   * Skip INTERACTIVE (players manage own idle) and PROTOTYPE (templates)
   */
  if (!(item->flags & (INTERACTIVE | PROTOTYPE))) {
    item->last_access_time = now_time;
  }
  if (dest && !(dest->flags & (INTERACTIVE | PROTOTYPE))) {
    dest->last_access_time = now_time;
  }
  
  tmp.type=INTEGER;
  tmp.value.integer=0;
  push(&tmp,rts);
  return 0;
}

int s_this_object(struct object *caller, struct object *obj, struct object
                    *player, struct var_stack **rts) {
  struct var tmp;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=0) return 1;
  tmp.type=OBJECT;
  tmp.value.objptr=obj;
  if (!obj) {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  } else
    while (tmp.value.objptr->attacher)
      tmp.value.objptr=tmp.value.objptr->attacher;
  push(&tmp,rts);
  return 0;
}

int s_this_player(struct object *caller, struct object *obj, struct object
                    *player, struct var_stack **rts) {
  struct var tmp;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=0) return 1;
  tmp.type=OBJECT;
  tmp.value.objptr=player;
  if (!player) {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  }
  push(&tmp,rts);
  return 0;
}

int s_remove_verb(struct object *caller, struct object *obj, struct object
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
    *(tmp.value.string=MALLOC(1))='\0';
  }
  if (tmp.type!=STRING) {
    clear_var(&tmp);
    return 1;
  }
  remove_verb(obj,tmp.value.string);
  clear_var(&tmp);
  tmp.type=INTEGER;
  tmp.value.integer=0;
  push(&tmp,rts);
  return 0;
}
