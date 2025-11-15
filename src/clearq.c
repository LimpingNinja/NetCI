/* clearq.c */

#include "config.h"
#include "object.h"
#include "dbhandle.h"
#include "constrct.h"
#include "interp.h"
#include "cache.h"
#include "globals.h"
#include "edit.h"
#include "intrface.h"
#include "file.h"
#include "protos.h"

/* functions for clearing the queues */

void handle_destruct() {
  struct destq *curr_dest;
  struct object *prev_obj,*curr_obj,*next_obj;
  struct proto *prev_proto,*curr_proto;
  signed long num_globals,loop;
  struct ref_list *curr_ref;
  struct attach_list *curr_attach,*prev_attach;
  struct verb *next_verb,*curr_verb;
  struct cmdq *curr_cmd,*prev_cmd,*tmp_cmd;
  struct alarmq *curr_alarm,*prev_alarm;
  char logbuf[256];

  while (dest_list) {
    curr_dest=dest_list;
    dest_list=dest_list->next;
    
    /* Log lifecycle event before actual destruction */
    sprintf(logbuf, "lifecycle: Destructing object %s#%ld",
            curr_dest->obj->parent ? curr_dest->obj->parent->pathname : "NULL",
            (long)curr_dest->obj->refno);
    logger(LOG_INFO, logbuf);
    curr_cmd=cmd_head;
    prev_cmd=NULL;
    if (curr_dest->obj->devnum!=(-1))
      immediate_disconnect(curr_dest->obj->devnum);
    while (curr_cmd) {
      if (curr_cmd->obj==curr_dest->obj) {
        if (prev_cmd)
          prev_cmd->next=curr_cmd->next;
        if (cmd_head==curr_cmd)
          cmd_head=curr_cmd->next;
        if (cmd_tail==curr_cmd)
          cmd_tail=prev_cmd;
        tmp_cmd=curr_cmd;
        curr_cmd=curr_cmd->next;
        FREE(tmp_cmd);
      } else {
        prev_cmd=curr_cmd;
        curr_cmd=curr_cmd->next;
      }
    }
    curr_alarm=alarm_list;
    prev_alarm=NULL;
    while (curr_alarm) {
      if (curr_alarm->obj==curr_dest->obj) {
        struct alarmq *tmp_alarm = curr_alarm->next;  /* Save next BEFORE freeing */
        if (prev_alarm)
          prev_alarm->next=curr_alarm->next;
        else
          alarm_list=curr_alarm->next;
        FREE(curr_alarm->funcname);
        FREE(curr_alarm);
        curr_alarm=tmp_alarm;  /* Use saved pointer, not freed memory */
      } else {
        prev_alarm=curr_alarm;
        curr_alarm=curr_alarm->next;
      }
    }
    num_globals=curr_dest->obj->parent->funcs->num_globals;
    if (curr_dest->obj->flags & PROTOTYPE) {
      curr_obj=curr_dest->obj->next_child;
      while (curr_obj) {
        queue_for_destruct(curr_obj);
        curr_obj=curr_obj->next_child;
      }
      handle_destruct();
      prev_proto=ref_to_obj(0)->parent;
      curr_proto=prev_proto->next_proto;
      while (curr_proto) {
        if (curr_proto==curr_dest->obj->parent) {
          prev_proto->next_proto=curr_proto->next_proto;
          break;
        }
        prev_proto=curr_proto;
        curr_proto=curr_proto->next_proto;
      }
    } else {
      prev_obj=curr_dest->obj->parent->proto_obj;
      curr_obj=prev_obj->next_child;
      while (curr_obj) {
        if (curr_obj==curr_dest->obj) {
          prev_obj->next_child=curr_dest->obj->next_child;
          break;
        }
        prev_obj=curr_obj;
        curr_obj=curr_obj->next_child;
      }
    }
    load_data(curr_dest->obj);
    loop=0;
    while (loop<num_globals) {
      clear_global_var(curr_dest->obj,loop);
      loop++;
    }
    curr_ref=curr_dest->obj->refd_by;
    while (curr_ref) {
      load_data(curr_ref->ref_obj);
      curr_ref->ref_obj->obj_state=DIRTY;
      curr_ref->ref_obj->globals[curr_ref->ref_num].type=INTEGER;
      curr_ref->ref_obj->globals[curr_ref->ref_num].value.integer=0;
      curr_ref=curr_ref->next;
    }
    unload_object(curr_dest->obj);
    if (curr_dest->obj->flags & PROTOTYPE) {
      /* Free inheritance chain */
      struct inherit_list *curr_inherit, *next_inherit;
      curr_inherit = curr_dest->obj->parent->inherits;
      while (curr_inherit) {
        next_inherit = curr_inherit->next;
        if (curr_inherit->inherit_path) FREE(curr_inherit->inherit_path);
        FREE(curr_inherit);
        curr_inherit = next_inherit;
      }
      free_code(curr_dest->obj->parent->funcs);
      FREE(curr_dest->obj->parent);
    }
    curr_attach=curr_dest->obj->attachees;
    while (curr_attach) {
      curr_attach->attachee->attacher=NULL;
      prev_attach=curr_attach;
      curr_attach=curr_attach->next;
      FREE(prev_attach);
    }
    if (curr_dest->obj->attacher) {
      prev_attach=NULL;
      curr_attach=curr_dest->obj->attacher->attachees;
      while (curr_attach) {
        if (curr_attach->attachee==curr_dest->obj) break;
        prev_attach=curr_attach;
        curr_attach=curr_attach->next;
      }
      if (curr_attach) {
        if (prev_attach) prev_attach->next=curr_attach->next;
        else curr_dest->obj->attacher->attachees=curr_attach->next;
        FREE(curr_attach);
      }
    }
    curr_dest->obj->attacher=NULL;
    curr_dest->obj->attachees=NULL;
    curr_obj=curr_dest->obj->contents;
    while (curr_obj) {
      curr_obj->location=curr_dest->obj->location;
      next_obj=curr_obj->next_object;
      if (curr_obj->location) {
        curr_obj->next_object=curr_obj->location->contents;
        curr_obj->location->contents=curr_obj;
      } else
        curr_obj->next_object=NULL;
      curr_obj=next_obj;
    }
    curr_dest->obj->contents=NULL;
    if (curr_dest->obj->location) {
      prev_obj=NULL;
      curr_obj=curr_dest->obj->location->contents;
      while (curr_obj) {
        if (curr_obj==curr_dest->obj) {
          if (prev_obj)
            prev_obj->next_object=curr_obj->next_object;
          else
            curr_dest->obj->location->contents=curr_obj->next_object;
          break;
        }
        prev_obj=curr_obj;
        curr_obj=curr_obj->next_object;
      }
    }
    curr_dest->obj->location=NULL;
    if (curr_dest->obj->flags & IN_EDITOR)
      remove_from_edit(curr_dest->obj);
    if (curr_dest->obj->flags & CONNECTED)
      disconnect_device(curr_dest->obj);
    curr_verb=curr_dest->obj->verb_list;
    while (curr_verb) {
      FREE(curr_verb->verb_name);
      FREE(curr_verb->function);
      next_verb=curr_verb->next;
      FREE(curr_verb);
      curr_verb=next_verb;
    }
    if (curr_dest->obj->input_func) {
      FREE(curr_dest->obj->input_func);
    }
    curr_dest->obj->devnum=-1;
    curr_dest->obj->input_func=NULL;
    curr_dest->obj->input_func_obj=NULL;
    curr_dest->obj->flags=GARBAGE;
    curr_dest->obj->parent=NULL;
    curr_dest->obj->next_child=NULL;
    curr_dest->obj->location=NULL;
    curr_dest->obj->contents=NULL;
    curr_dest->obj->next_object=free_obj_list;
    curr_dest->obj->globals=NULL;
    curr_dest->obj->refd_by=NULL;
    curr_dest->obj->verb_list=NULL;
    free_obj_list=curr_dest->obj;
    FREE(curr_dest);
  }
}

void handle_alarm() {
  struct alarmq *curr_alarm;
  struct fns *func;
  struct var tmp;
  struct var_stack *rts;
  struct object *obj;

  while (alarm_list) {
    if (alarm_list->delay>now_time) return;
    curr_alarm=alarm_list;
    alarm_list=alarm_list->next;
    func=find_function(curr_alarm->funcname,curr_alarm->obj,&obj);

#ifdef CYCLE_SOFT_MAX
    soft_cycles=0;
#endif /* CYCLE_SOFT_MAX */

    if (func) {
      rts=NULL;
      tmp.type=NUM_ARGS;
      tmp.value.num=0;
      push(&tmp,&rts);
      interp(NULL,obj,NULL,&rts,func);
      free_stack(&rts);
    }
    FREE(curr_alarm->funcname);
    FREE(curr_alarm);
    handle_destruct();
  }
}

struct verb *find_vname(struct object *obj, char *vname, int is_second_run) {
  struct verb *curr;

  if (is_second_run) {
    /* Null safety - check parent chain exists */
    if (obj->parent && obj->parent->proto_obj)
      curr=obj->parent->proto_obj->verb_list;
    else
      curr=NULL;
  } else {
    curr=obj->verb_list;
  }
  while (curr) {
    if (!strcmp(curr->verb_name,vname)) return curr;
    curr=curr->next;
  }
  return NULL;
}

int find_verb(struct object *player, struct object *obj, char *vname, char
              *cmd, signed long disp) {
  struct verb *curr_verb;
  struct fns *func;
  struct var_stack *rts;
  struct var tmp;
  int is_match,is_second_run;
  char *cvn;
  struct object *tmpobj;

  if (!obj) return 0;
  if ((obj->flags & INTERACTIVE) && player!=obj) return 0;
  curr_verb=obj->verb_list;
  is_second_run=0;
  if (!curr_verb) {
    /* Try prototype's verb list if available */
    if (obj->parent && obj->parent->proto_obj) {
      curr_verb=obj->parent->proto_obj->verb_list;
      is_second_run=1;
    }
  }
  /* If still no verbs, return 0 (no match) - don't crash */
  if (!curr_verb) return 0;
  
  while (curr_verb) {
    is_match=0;
    cvn=copy_string(curr_verb->verb_name);
    if (curr_verb->is_xverb) {
      if ((!strncmp(curr_verb->verb_name,cmd,strlen(curr_verb->verb_name)))
          || (!(*(curr_verb->verb_name)))) {
        rts=NULL;
        func=find_function(curr_verb->function,obj,&tmpobj);
        if (func) {
          tmp.type=STRING;
          tmp.value.string=&(cmd[strlen(curr_verb->verb_name)]);
          if ((*(tmp.value.string))=='\0') {
            tmp.type=INTEGER;
            tmp.value.integer=0;
          }
          push(&tmp,&rts);
          tmp.type=NUM_ARGS;
          tmp.value.num=1;
          push(&tmp,&rts);
          if (!interp(NULL,tmpobj,player,&rts,func)) {
            if (!pop(&tmp,&rts,obj)) {
              if (tmp.type!=INTEGER || tmp.value.integer!=0)
                is_match=1;
              clear_var(&tmp);
            }
          }
          free_stack(&rts);
        }
      }
    } else {
      if (!strcmp(curr_verb->verb_name,vname)) {
        rts=NULL;
        func=find_function(curr_verb->function,obj,&tmpobj);
        if (func) {
          tmp.type=STRING;
          tmp.value.string=&(cmd[disp]);
          if ((*(tmp.value.string))=='\0') {
            tmp.type=INTEGER;
            tmp.value.integer=0;
          }
          push(&tmp,&rts);
          tmp.type=NUM_ARGS;
          tmp.value.num=1;
          push(&tmp,&rts);
          if (!interp(NULL,tmpobj,player,&rts,func)) {
            if (!pop(&tmp,&rts,obj)) {
              if (tmp.type!=INTEGER || tmp.value.integer!=0)
                is_match=1;
              clear_var(&tmp);
            }
          }
          free_stack(&rts);
          if (verbs_changed) is_match=1;
        }
      }
    }
    if (is_match) {
      FREE(cvn);
      /* Update last access time for object with the matched verb
       * Skip INTERACTIVE (players manage own idle) and PROTOTYPE (templates)
       * This marks rooms/items as "actively used" by player interaction
       */
      if (obj && !(obj->flags & (INTERACTIVE | PROTOTYPE))) {
        obj->last_access_time = now_time;
      }
      return 1;
    }
    curr_verb=find_vname(obj,cvn,is_second_run);
    FREE(cvn);
    if (curr_verb) curr_verb=curr_verb->next;
    if (!curr_verb)
      if (!(obj->flags & PROTOTYPE) && !is_second_run) {
        is_second_run=1;
        /* Null safety - check parent chain exists */
        if (obj->parent && obj->parent->proto_obj) {
          curr_verb=obj->parent->proto_obj->verb_list;
        }
      }
  }
  return 0;
}

void handle_command() {
  struct cmdq *curr;
  char *vname,*funcname;
  signed long begin_char,count,loop;
  struct object *curr_obj;
  int done;
  struct var_stack *rts;
  struct var tmp;
  struct fns *func;

  while (cmd_head) {
    curr=cmd_head;
    cmd_head=curr->next;
    if (!cmd_head) cmd_tail=NULL;

#ifdef CYCLE_SOFT_MAX
    soft_cycles=0;
#endif /* CYCLE_SOFT_MAX */

    if ((funcname=curr->obj->input_func)) {
      struct object *target_obj;
      
      curr->obj->input_func=NULL;
      target_obj=curr->obj->input_func_obj;
      curr->obj->input_func_obj=NULL;
      
      /* If input_to() was used, find function in target object */
      if (target_obj) {
        func=find_function(funcname,target_obj,&curr_obj);
      } else {
        /* Otherwise use redirect_input() behavior (function in self) */
        func=find_function(funcname,curr->obj,&curr_obj);
      }
      
      FREE(funcname);
      if (func) {
        if (curr->cmd) {
          if (*(curr->cmd)) {
            tmp.type=STRING;
            tmp.value.string=curr->cmd;
	  } else {
            FREE(curr->cmd);
            tmp.type=INTEGER;
            tmp.value.integer=0;
          }
        } else {
          tmp.type=INTEGER;
          tmp.value.integer=0;
        }
        rts=NULL;
        pushnocopy(&tmp,&rts);
        tmp.type=NUM_ARGS;
        tmp.value.num=1;
        push(&tmp,&rts);
        interp(NULL,curr_obj,curr->obj,&rts,func);
        free_stack(&rts);
      } else {
        FREE(curr->cmd);
      }
      FREE(curr);
      handle_destruct();
      continue;
    }
    done=0;
    begin_char=0;
    while (curr->cmd[begin_char]==' ') begin_char++;
    count=begin_char;
    while (curr->cmd[count]!='\0' && curr->cmd[count]!=' ') count++;
    vname=MALLOC(count-begin_char+1);
    loop=begin_char;
    while (loop<count) {
      vname[loop-begin_char]=curr->cmd[loop];
      loop++;
    }
    vname[loop]='\0';
    while (curr->cmd[count]==' ') count++;
    verbs_changed=0;
    if (!(curr->obj->flags & LOCALVERBS)) {
      if (find_verb(curr->obj,curr->obj->location,vname,curr->cmd,count))
        done=1;
      if (curr->obj->location) {
        curr_obj=curr->obj->location->contents;
        while (curr_obj && !done) {
          if (curr_obj!=curr->obj)
            if (find_verb(curr->obj,curr_obj,vname,curr->cmd,count)) done=1;
          curr_obj=curr_obj->next_object;
        }
      }
      curr_obj=curr->obj->contents;
      while (curr_obj && !done) {
        if (find_verb(curr->obj,curr_obj,vname,curr->cmd,count)) done=1;
        curr_obj=curr_obj->next_object;
      }
    }
    if (!done) find_verb(curr->obj,curr->obj,vname,curr->cmd,count);
    FREE(vname);
    FREE(curr->cmd);
    
    /* Send automatic prompt if interactive and no input_func override */
    if ((curr->obj->flags & INTERACTIVE) && !curr->obj->input_func) {
      send_prompt(curr->obj, "> ");
    }
    
    FREE(curr);
    handle_destruct();
  }
}
