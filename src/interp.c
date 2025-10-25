/* interp.c */

#include "config.h"
#include "object.h"
#include "constrct.h"
#include "interp.h"
#include "instr.h"
#include "protos.h"
#include "globals.h"
#include "cache.h"
#include "file.h"
#include "intrface.h"

int (*oper_array[NUM_OPERS+NUM_SCALLS])(struct object *caller, struct object
                                        *obj, struct object *player, struct
                                        var_stack **rts)={
  comma_oper,eq_oper,pleq_oper,mieq_oper,mueq_oper,dieq_oper,moeq_oper,
  aneq_oper,exeq_oper,oreq_oper,lseq_oper,rseq_oper,cond_oper,or_oper,
  and_oper,bitor_oper,exor_oper,bitand_oper,condeq_oper,noteq_oper,less_oper,
  lesseq_oper,great_oper,greateq_oper,ls_oper,rs_oper,add_oper,min_oper,
  mul_oper,div_oper,mod_oper,not_oper,bitnot_oper,postadd_oper,preadd_oper,
  postmin_oper,premin_oper,umin_oper,s_add_verb,s_add_xverb,s_call_other,
  s_alarm,s_remove_alarm,s_caller_object,s_clone_object,s_destruct,
  s_contents,s_next_object,s_location,s_next_child,s_parent,s_next_proto,
  s_move_object,s_this_object,s_this_player,s_set_interactive,
  s_interactive,s_set_priv,s_priv,s_in_editor,s_connected,s_get_devconn,
  s_send_device,s_reconnect_device,s_disconnect_device,s_random,s_time,
  s_mktime,s_typeof,s_command,s_compile_object,s_edit,s_cat,s_ls,s_rm,s_cp,
  s_mv,s_mkdir,s_rmdir,s_hide,s_unhide,s_chown,s_syslog,s_sscanf,s_sprintf,
  s_midstr,s_strlen,s_leftstr,s_rightstr,s_subst,s_instr,s_otoa,s_itoa,
  s_atoi,s_atoo,s_upcase,s_downcase,s_is_legal,s_otoi,s_itoo,s_chmod,
  s_fread,s_fwrite,s_remove_verb,s_ferase,s_chr,s_asc,s_sysctl,
  s_prototype,s_iterate,s_next_who,s_get_devidle,s_get_conntime,
  s_connect_device,s_flush_device,s_attach,s_this_component,s_detach,
  s_table_get,s_table_set,s_table_delete,s_fstat,s_fowner,s_get_hostname,
  s_get_address,s_set_localverbs,s_localverbs,s_next_verb,s_get_devport,
  s_get_devnet,s_redirect_input,s_get_input_func,s_get_master,s_is_master };

void interp_error(char *msg, struct object *player, struct object *obj,
                  struct fns *func, unsigned long line) {
  char *buf;

  buf=MALLOC(strlen(obj->parent->pathname)+strlen(msg)+(2*ITOA_BUFSIZ)+20);
  sprintf(buf," interp: %s#%ld line #%ld: %s",obj->parent->pathname,
          (long) obj->refno,(long) line,msg);
  logger(LOG_ERROR, buf);
  if (player) {
    send_device(player,buf+1);
    send_device(player,"\n");
  }
  FREE(buf);
}

void clear_locals() {
  int loop;

  if (!num_locals) return;
  loop=0;
  while (loop<num_locals) {
    clear_var(&(locals[loop]));
    loop++;
  }
  FREE(locals);
}

struct fns *find_fns(char *name, struct object *obj) {
  struct fns *next;

  next=obj->parent->funcs->func_list;
  while (next) {
    if (!strcmp(next->funcname,name))
      return next;
    next=next->next;
  }
  return NULL;
}

struct fns *find_function(char *name, struct object *obj,
                          struct object **real_obj) {
  struct fns *tmpfns;
  struct attach_list *curr_attach;

  tmpfns=find_fns(name,obj);
  if (tmpfns) {
    if (real_obj) (*real_obj)=obj;
    return tmpfns;
  }
  curr_attach=obj->attachees;
  while (curr_attach) {
    if ((tmpfns=find_function(name,curr_attach->attachee,real_obj)))
      return tmpfns;
    curr_attach=curr_attach->next;
  }
  return NULL;
}

struct fns *find_extern_function(char *name, struct object *obj,
                                 struct object **real_obj) {
  struct fns *tmpfns;
  struct attach_list *curr_attach;

  curr_attach=obj->attachees;
  while (curr_attach) {
    if ((tmpfns=find_function(name,curr_attach->attachee,real_obj)))
      return tmpfns;
    curr_attach=curr_attach->next;
  }
  return NULL;
}

int interp(struct object *caller, struct object *obj, struct object *player,
           struct var_stack **arg_stack, struct fns *func) {
  struct var_stack *rts,*stack1;
  struct var tmp,tmp2;
  struct fns *temp_fns;
  unsigned long loop,line;
  unsigned int old_num_locals;
  struct var *old_locals;
  int retstatus;
  int old_use_soft_cycles,old_use_hard_cycles;
  struct object *tmpobj;

  if (caller) while (caller->attacher) caller=caller->attacher;
  old_use_soft_cycles=use_soft_cycles;
  old_use_hard_cycles=use_hard_cycles;
  load_data(obj);
  rts=NULL;
  stack1=NULL;
  line=0;
  num_locals=func->num_locals;
  loop=0;
  if (num_locals)
    locals=(struct var *) MALLOC(sizeof(struct var)*num_locals);
  else
    locals=NULL;
  while (loop<num_locals) {
    locals[loop].type=INTEGER;
    locals[loop].value.integer=0;
    loop++;
  }
  if (pop(&tmp,arg_stack,caller)) {
    interp_error("malformed argument stack",player,obj,func,0);
    free_stack(arg_stack);
    return 1;
  }
  if (tmp.type!=NUM_ARGS) {
    interp_error("malformed argument stack",player,obj,func,0);
    free_stack(arg_stack);
    return 1;
  }
  if (tmp.value.num>num_locals) {
    interp_error("too many arguments",player,obj,func,0);
    return 1;
  }
  loop=tmp.value.num;
  while (loop>0) {
    if (pop(&tmp,arg_stack,caller)) {
      interp_error("malformed argument stack",player,obj,func,0);
      free_stack(arg_stack);
      clear_locals();
      return 1;
    }
    --loop;
    copy_var(&(locals[loop]),&tmp);
    clear_var(&tmp);
  }
  loop=0;
  while (1) {

#ifdef CYCLE_HARD_MAX
    if (use_hard_cycles)
      if (hard_cycles++>CYCLE_HARD_MAX) {
        tmp.type=INTEGER;
        tmp.value.integer=0;
        interp_error("cycle hard maximum exceeded",player,obj,func,line);
        pushnocopy(&tmp,arg_stack);
        clear_locals();
        return 0;
      }
#endif /* CYCLE_HARD_MAX */

#ifdef CYCLE_SOFT_MAX
    if (use_soft_cycles)
      if (soft_cycles++>CYCLE_SOFT_MAX) {
        tmp.type=INTEGER;
        tmp.value.integer=0;
        interp_error("cycle soft maximum exceeded",player,obj,func,line);
        pushnocopy(&tmp,arg_stack);
        clear_locals();
        return 0;
      }
#endif /* CYCLE_SOFT_MAX */

    switch (func->code[loop].type) {
      case INTEGER:
      case STRING:
      case OBJECT:
      case GLOBAL_L_VALUE:
      case LOCAL_L_VALUE:
      case NUM_ARGS:
      case ARRAY_SIZE:
        push(&(func->code[loop]),&rts);
        loop++;
        break;
      case LOCAL_REF:
      case GLOBAL_REF:
        if (popint(&tmp,&rts,obj)) {
          interp_error("failed array reference",player,obj,func,line);
          free_stack(&rts);
          clear_locals();
          use_soft_cycles=old_use_soft_cycles;
          use_hard_cycles=old_use_hard_cycles;
          return 1;
	}
        tmp2.value.l_value.size=tmp.value.integer;
        if (popint(&tmp,&rts,obj)) {
          interp_error("failed array reference",player,obj,func,line);
          free_stack(&rts);
          clear_locals();
          use_soft_cycles=old_use_soft_cycles;
          use_hard_cycles=old_use_hard_cycles;
          return 1;
	}
        tmp2.value.l_value.ref=tmp.value.integer;
        if (func->code[loop].type==LOCAL_REF) {
          if (tmp2.value.l_value.ref>=num_locals) {
            interp_error("array reference out of bound",player,obj,func,line);
            free_stack(&rts);
            clear_locals();
            use_soft_cycles=old_use_soft_cycles;
            use_hard_cycles=old_use_hard_cycles;
            return 1;
	  }
	} else
          if (tmp2.value.l_value.ref>=obj->parent->funcs->num_globals) {
            interp_error("array reference out of bound",player,obj,func,line);
            free_stack(&rts);
            clear_locals();
            use_soft_cycles=old_use_soft_cycles;
            use_hard_cycles=old_use_hard_cycles;
            return 1;
	  }
        if (tmp2.value.l_value.size!=1) {
          interp_error("illegal array reference",player,obj,func,line);
          free_stack(&rts);
          clear_locals();
          use_soft_cycles=old_use_soft_cycles;
          use_hard_cycles=old_use_hard_cycles;
          return 1;
	}
        if (func->code[loop].type==LOCAL_REF)
          tmp2.type=LOCAL_L_VALUE;
        else
          tmp2.type=GLOBAL_L_VALUE;
        push(&tmp2,&rts);
        loop++;
        break;
      case ASM_INSTR:
        if (func->code[loop].value.instruction<NUM_OPERS) {
          retstatus=((*oper_array[func->code[loop].value.instruction])
                     (caller,obj,player,&rts));
          if (retstatus) {
            interp_error("arithmetic operation failed",player,obj,func,line);
            free_stack(&rts);
            clear_locals();
            use_soft_cycles=old_use_soft_cycles;
            use_hard_cycles=old_use_hard_cycles;
            return 1;
          }
        } else {
          old_locals=locals;
          old_num_locals=num_locals;
          if (func->code[loop].value.instruction==S_SSCANF ||
              func->code[loop].value.instruction==S_SPRINTF ||
              func->code[loop].value.instruction==S_FREAD)
            stack1=gen_stack_noresolve(&rts,obj);
          else
            stack1=gen_stack(&rts,obj);
          retstatus=((*oper_array[func->code[loop].value.instruction])
                     (caller,obj,player,&stack1));
          locals=old_locals;
          num_locals=old_num_locals;
          if (retstatus) {
            interp_error("system call failed",player,obj,func,line);
            free_stack(&rts);
            free_stack(&stack1);
            clear_locals();
            use_hard_cycles=old_use_hard_cycles;
            use_soft_cycles=old_use_soft_cycles;
            return 1;
          }
          if (pop(&tmp,&stack1,obj)) {
            interp_error("system call returned malformed stack",player,obj,
                         func,line);
            free_stack(&rts);
            free_stack(&stack1);
            clear_locals();
            use_hard_cycles=old_use_hard_cycles;
            use_soft_cycles=old_use_soft_cycles;
            return 1;
          }
          pushnocopy(&tmp,&rts);
          free_stack(&stack1);
        }
        loop++;
        break;
      case FUNC_CALL:
        stack1=gen_stack(&rts,obj);
        old_locals=locals;
        old_num_locals=num_locals;
        if (interp(obj,obj,player,&stack1,func->code[loop].value.func_call)) {
          locals=old_locals;
          num_locals=old_num_locals;
          free_stack(&stack1);
          free_stack(&rts);
          clear_locals();
          use_soft_cycles=old_use_soft_cycles;
          use_hard_cycles=old_use_hard_cycles;
          return 1;
        }
        locals=old_locals;
        num_locals=old_num_locals;
        if (pop(&tmp,&stack1,obj)) {
          interp_error("function returned malformed stack",player,obj,func,
                       line);
          free_stack(&rts);
          free_stack(&stack1);
          clear_locals();
          use_soft_cycles=old_use_soft_cycles;
          use_hard_cycles=old_use_hard_cycles;
          return 1;
        }
        pushnocopy(&tmp,&rts);
        free_stack(&stack1);
        loop++;
        break;
      case EXTERN_FUNC:
        temp_fns=find_extern_function(func->code[loop].value.string,obj,
                                      &tmpobj);
        if (!temp_fns) {
          interp_error("unknown function",player,obj,func,line);
          free_stack(&rts);
          clear_locals();
          use_soft_cycles=old_use_soft_cycles;
          use_hard_cycles=old_use_hard_cycles;
          return 1;
        }
        stack1=gen_stack(&rts,obj);
        old_locals=locals;
        old_num_locals=num_locals;
        if (interp(obj,tmpobj,player,&stack1,temp_fns)) {
          locals=old_locals;
          num_locals=old_num_locals;
          free_stack(&stack1);
          free_stack(&rts);
          clear_locals();
          use_soft_cycles=old_use_soft_cycles;
          use_hard_cycles=old_use_hard_cycles;
          return 1;
        }
        locals=old_locals;
        num_locals=old_num_locals;
        if (pop(&tmp,&stack1,obj)) {
          interp_error("function returned malformed stack",player,obj,func,
                       line);
          free_stack(&rts);
          free_stack(&stack1);
          clear_locals();
          use_soft_cycles=old_use_soft_cycles;
          use_hard_cycles=old_use_hard_cycles;
          return 1;
        }
        pushnocopy(&tmp,&rts);
        free_stack(&stack1);
        loop++;
        break;
      case FUNC_NAME:
        temp_fns=find_function(func->code[loop].value.string,obj,&tmpobj);
        if (!temp_fns) {
          interp_error("unknown function",player,obj,func,line);
          free_stack(&rts);
          clear_locals();
          use_soft_cycles=old_use_soft_cycles;
          use_hard_cycles=old_use_hard_cycles;
          return 1;
        }
        if (tmpobj==obj) {
          clear_var(&(func->code[loop]));
          func->code[loop].type=FUNC_CALL;
          func->code[loop].value.func_call=temp_fns;
        } else {
          func->code[loop].type=EXTERN_FUNC;
        }
        break;
      case JUMP:
        loop=func->code[loop].value.num;
        break;
      case BRANCH:
        if (pop(&tmp,&rts,obj)) {
          interp_error("failed branch instruction",player,obj,func,line);
          free_stack(&rts);
          clear_locals();
          use_soft_cycles=old_use_soft_cycles;
          use_hard_cycles=old_use_hard_cycles;
          return 1;
        }
        if (resolve_var(&tmp,obj)) {
          interp_error("failed variable resolution",player,obj,func,line);
          free_stack(&rts);
          clear_locals();
          use_soft_cycles=old_use_soft_cycles;
          use_hard_cycles=old_use_hard_cycles;
          return 1;
        }
        if (tmp.type==INTEGER && tmp.value.integer==0) {
          loop=func->code[loop].value.num;
        } else {
          clear_var(&tmp);
          loop++;
        }
        break;
      case NEW_LINE:
        free_stack(&rts);
        line=func->code[loop].value.num;
        loop++;
        break;
      case RETURN:
        if (pop(&tmp,&rts,obj)) {
          interp_error("stack malformed on return",player,obj,func,line);
          free_stack(&rts);
          clear_locals();
          use_hard_cycles=old_use_hard_cycles;
          use_soft_cycles=old_use_soft_cycles;
          return 1;
        }
        if (resolve_var(&tmp,obj)) {
          interp_error("stack malformed on return",player,obj,func,line);
          free_stack(&rts);
          clear_locals();
          use_hard_cycles=old_use_hard_cycles;
          use_soft_cycles=old_use_soft_cycles;
          return 1;
        }
        pushnocopy(&tmp,arg_stack);
        free_stack(&rts);
        clear_locals();
        use_soft_cycles=old_use_soft_cycles;
        use_hard_cycles=old_use_hard_cycles;
        return 0;
        break;
    }
  }
}
