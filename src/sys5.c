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
#include "edit.h"

struct var *convert_cvt(long num_globals, signed long *cvt, struct
                        object *obj) {
  struct var *new_globals;
  long loop;
  struct ref_list *tmpref;

  if (num_globals)
    new_globals=MALLOC(num_globals*sizeof(struct var));
  else
    new_globals=NULL;
  loop=0;
  while (loop<num_globals) {
    new_globals[loop].type=INTEGER;
    new_globals[loop].value.integer=0;
    loop++;
  }
  loop=0;
  while (loop<obj->parent->funcs->num_globals) {
    if (cvt[loop]>=0) {
      new_globals[cvt[loop]].type=obj->globals[loop].type;
      switch (obj->globals[loop].type) {
        case INTEGER:
          new_globals[cvt[loop]].value.integer=obj->globals[loop].value.integer;
          break;
        case STRING:
          new_globals[cvt[loop]].value.string=obj->globals[loop].value.string;
          obj->globals[loop].type=INTEGER;
          obj->globals[loop].value.integer=0;
          break;
        case OBJECT:
          new_globals[cvt[loop]].value.objptr=obj->globals[loop].value.objptr;
          clear_global_var(obj,loop);
          break;
        default:
          clear_global_var(obj,loop);
           break;
      }
    } else
      clear_global_var(obj,loop);
    loop++;
  }
  loop=0;
  while (loop<num_globals) {
    if (new_globals[loop].type==OBJECT) {
      tmpref=MALLOC(sizeof(struct ref_list));
      tmpref->ref_obj=obj;
      tmpref->ref_num=loop;
      load_data(new_globals[loop].value.objptr);
      new_globals[loop].value.objptr->obj_state=DIRTY;
      tmpref->next=new_globals[loop].value.objptr->refd_by;
      new_globals[loop].value.objptr->refd_by=tmpref;
    }
    loop++;
  }
  return new_globals;
}

int gstcmp(struct code *c1,struct code *c2,signed long *cvt) {
  long loop;

  if (c1->num_globals!=c2->num_globals) return 1;
  loop=0;
  while (loop<c1->num_globals) {
    if (cvt[loop]!=loop) return 1;
    loop++;
  }
  return 0;
}

struct var_tab *find_var_gst(char *name, struct var_tab *curr) {
  while (curr)
    if (strcmp(name,curr->name))
      curr=curr->next;
    else
      return curr;
  return NULL;
}

void cvt_array_recurse(signed long *cvt, long old_base, long new_base,
                       struct array_size *old_array, struct array_size
                       *new_array) {
  long loop,size1,size2;
  struct array_size *curr1,*curr2;

  if ((!old_array->next) && (!new_array->next)) {
    loop=0;
    while (loop<old_array->size && loop<new_array->size) {
      cvt[old_base+loop]=new_base+loop;
      loop++;
    }
  } else if (old_array->next && new_array->next) {
    size1=1;
    size2=1;
    curr1=old_array->next;
    curr2=new_array->next;
    while (curr1) {
      size1*=curr1->size;
      curr1=curr1->next;
    }
    while (curr2) {
      size2*=curr2->size;
      curr2=curr2->next;
    }
    loop=0;
    while (loop<old_array->size && loop<new_array->size) {
      cvt_array_recurse(cvt,old_base+loop*size1,new_base+loop*size2,
                        old_array->next,new_array->next);
      loop++;
    }
  }
}

signed long *make_cvt(struct code *oc, struct code *nc) {
  signed long *cvt,loop;
  struct var_tab *curr_var,*new_var;

  if (!(oc->num_globals)) return NULL;
  cvt=MALLOC(sizeof(signed long)*(oc->num_globals));
  loop=0;
  while (loop<oc->num_globals) cvt[loop++]=(-1);
  if (!oc->gst) {
    loop=0;
    while (loop<nc->num_globals && loop<oc->num_globals) {
      cvt[loop]=loop;
      loop++;
    }
    return cvt;
  }
  curr_var=oc->gst;
  while (curr_var) {
    if ((new_var=find_var_gst(curr_var->name,nc->gst))) {
      if (curr_var->array || new_var->array)
        cvt_array_recurse(cvt,curr_var->base,new_var->base,
                          curr_var->array,new_var->array);
      else
        cvt[curr_var->base]=new_var->base;
    }
    curr_var=curr_var->next;
  }
  return cvt;
}

int s_compile_object(struct object *caller, struct object *obj, struct object
                     *player, struct var_stack **rts) {
  struct var tmp;
  struct code *newcode;
  unsigned int line;
  struct object *tmpobj,*proto_obj,*tmpobj2;
  unsigned int old_num_locals;
  struct var *old_locals;
  struct var *new_globals;
  struct proto *tmp_proto;
  unsigned long loop;
  struct var_stack *arg_stack;
  struct fns *tmpfns;
  signed long *cvt;

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
  tmpobj=find_proto(tmp.value.string);
  /* Diagnostic logging: check early-return guard conditions */
  {
    char *buf;
    const char *caller_path = caller && caller->parent ? caller->parent->pathname : "NULL";
    const char *self_path = obj && obj->parent ? obj->parent->pathname : "NULL";
    const char *target_path = tmp.value.string ? tmp.value.string : "(null)";
    buf = MALLOC(256 + strlen(caller_path) + strlen(self_path) + strlen(target_path));
    sprintf(buf,
            " compile_object: caller=%s self=%s target=%s proto_exists=%s",
            caller_path,
            self_path,
            target_path,
            (tmpobj ? "yes" : "no"));
    logger(LOG_DEBUG, buf);
    FREE(buf);
  }
  /* Guard: block only if caller is non-NULL and not auto_proto, OR if proto is up-to-date */
  if (((caller && caller != auto_proto)) || (obj->parent->proto_obj==tmpobj)) {
    {
      char *buf2;
      const char *target_path = tmp.value.string ? tmp.value.string : "(null)";
      const char *caller_path2 = caller && caller->parent ? caller->parent->pathname : "NULL";
      buf2 = MALLOC(128 + strlen(target_path));
      sprintf(buf2,
              " compile_object: EARLY RETURN (blocked). reason=%s caller=%s target=%s",
              (obj->parent->proto_obj==tmpobj) ? "proto-up-to-date" : "caller-not-allowed",
              caller_path2,
              target_path);
      logger(LOG_DEBUG, buf2);
      FREE(buf2);
    }
    clear_var(&tmp);
    tmp.type=INTEGER;
    tmp.value.integer=0;
    push(&tmp,rts);
    return 0;
  }
  line=parse_code(tmp.value.string,obj,&newcode);
  if (line==((unsigned int) -1)) {
    {
      char *buf3;
      const char *target_path = tmp.value.string ? tmp.value.string : "(null)";
      buf3 = MALLOC(128 + strlen(target_path));
      sprintf(buf3,
              " compile_object: PARSE FAILED (-1) target=%s",
              target_path);
      logger(LOG_DEBUG, buf3);
      FREE(buf3);
    }
    clear_var(&tmp);
    tmp.type=INTEGER;
    tmp.value.integer=0;
    push(&tmp,rts);
    return 0;
  }
  if (line) {
    {
      char *buf4;
      const char *target_path = tmp.value.string ? tmp.value.string : "(null)";
      buf4 = MALLOC(160 + strlen(target_path));
      sprintf(buf4,
              " compile_object: SYNTAX ERROR line=%u target=%s",
              line,
              target_path);
      logger(LOG_DEBUG, buf4);
      FREE(buf4);
    }
    compile_error(player,tmp.value.string,line);
    clear_var(&tmp);
    tmp.value.integer=0;
    tmp.type=INTEGER;
    push(&tmp,rts);
    return 0;
  }
  if (tmpobj) {
    {
      char *buf5;
      const char *target_path = tmp.value.string ? tmp.value.string : "(null)";
      buf5 = MALLOC(128 + strlen(target_path));
      sprintf(buf5,
              " compile_object: UPDATE EXISTING target=%s",
              target_path);
      logger(LOG_DEBUG, buf5);
      FREE(buf5);
    }
    proto_obj=tmpobj;
    cvt=make_cvt(tmpobj->parent->funcs,newcode);
    if (gstcmp(tmpobj->parent->funcs,newcode,cvt))
      while (tmpobj) {
        load_data(tmpobj);
        tmpobj->obj_state=DIRTY;
        new_globals=convert_cvt(newcode->num_globals,cvt,tmpobj);
        if (tmpobj->globals)
          FREE(tmpobj->globals);
        tmpobj->globals=new_globals;
        tmpobj=tmpobj->next_child;
      }
    free_code(proto_obj->parent->funcs);
    proto_obj->parent->funcs=newcode;
    if (cvt)
      FREE(cvt);
    clear_var(&tmp);
  } else {
    {
      char *buf6;
      const char *target_path = tmp.value.string ? tmp.value.string : "(null)";
      buf6 = MALLOC(128 + strlen(target_path));
      sprintf(buf6,
              " compile_object: CREATE NEW target=%s",
              target_path);
      logger(LOG_DEBUG, buf6);
      FREE(buf6);
    }
    proto_obj=newobj();
    tmp_proto=(struct proto *) MALLOC(sizeof(struct proto));
    tmp_proto->pathname=tmp.value.string;
    tmp_proto->funcs=newcode;
    tmp_proto->proto_obj=proto_obj;
    tmp_proto->next_proto=ref_to_obj(0)->parent->next_proto;
    ref_to_obj(0)->parent->next_proto=tmp_proto;
    proto_obj->flags|=PROTOTYPE;
    proto_obj->parent=tmp_proto;
    if (newcode->num_globals)
      proto_obj->globals=(struct var *) MALLOC (newcode->num_globals *
                                                sizeof(struct var));
    else
      proto_obj->globals=NULL;
    loop=0;
    while (loop<newcode->num_globals) {
      proto_obj->globals[loop].type=INTEGER;
      proto_obj->globals[loop].value.integer=0;
      loop++;
    }
    add_loaded(proto_obj);
    proto_obj->obj_state=DIRTY;
    old_locals=locals;
    old_num_locals=num_locals;
    tmpfns=find_function("init",proto_obj,&tmpobj2);
    arg_stack=NULL;
    if (tmpfns) {
      tmp.type=NUM_ARGS;
      tmp.value.num=0;
      push(&tmp,&arg_stack);
      interp(obj,tmpobj2,player,&arg_stack,tmpfns);
      free_stack(&arg_stack);
    }
    locals=old_locals;
    num_locals=old_num_locals;
  }
  
  /* Attach auto object to the newly compiled/updated prototype */
  attach_auto_to(proto_obj);
  
  tmp.type=OBJECT;
  tmp.value.objptr=proto_obj;
  push(&tmp,rts);
  return 0;
}

int s_edit(struct object *caller, struct object *obj, struct object *player,
           struct var_stack **rts) {
  struct var tmp;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=1) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=STRING && (tmp.type!=INTEGER || tmp.value.integer!=0)) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.type==INTEGER)
    add_to_edit(obj,NULL);
  else
    add_to_edit(obj,tmp.value.string);
  clear_var(&tmp);
  tmp.type=INTEGER;
  tmp.value.integer=0;
  push(&tmp,rts);
  return 0;
}

int s_cat(struct object *caller, struct object *obj, struct object *player,
          struct var_stack **rts) {
  struct fns *listen_func;
  struct object *rcv,*tmpobj;
  struct var tmp;
  FILE *infile;
  static char buf[MAX_STR_LEN];
  struct var_stack *rts2;

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
  if (!obj) {
    clear_var(&tmp);
    tmp.type=INTEGER;
    tmp.value.integer=1;
    push(&tmp,rts);
    return 0;
  }
  rcv=player;
  if (!player) rcv=obj;
  infile=open_file(tmp.value.string,FREAD_MODE,obj);
  clear_var(&tmp);
  tmp.type=INTEGER;
  if (!infile) {
    tmp.value.integer=1;
    push(&tmp,rts);
    return 0;
  }
  tmp.value.integer=0;
  push(&tmp,rts);
  listen_func=find_function("listen",rcv,&tmpobj);
  if (!listen_func) {
    close_file(infile);
    return 0;
  }
  while (fgets(buf,MAX_STR_LEN,infile)) {
    rts2=NULL;
    tmp.type=STRING;
    tmp.value.string=buf;
    push(&tmp,&rts2);
    tmp.type=NUM_ARGS;
    tmp.value.num=1;
    push(&tmp,&rts2);
    interp(obj,tmpobj,player,&rts2,listen_func);
    free_stack(&rts2);
  }
  close_file(infile);
  return 0;
}

int s_ls(struct object *caller, struct object *obj, struct object *player,
         struct var_stack **rts) {
  int retval;
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
  retval=ls_dir(tmp.value.string,obj,player);
  clear_var(&tmp);
  tmp.type=INTEGER;
  tmp.value.integer=retval;
  push(&tmp,rts);
  return 0;
}

int s_rm(struct object *caller, struct object *obj, struct object *player,
         struct var_stack **rts) {
  struct var tmp;
  int retval;

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
  retval=remove_file(tmp.value.string,obj);
  clear_var(&tmp);
  tmp.type=INTEGER;
  tmp.value.integer=retval;
  push(&tmp,rts);
  return 0;
}

int s_cp(struct object *caller, struct object *obj, struct object *player,
         struct var_stack **rts) {
  int retval;
  struct var tmp1,tmp2;

  if (pop(&tmp2,rts,obj)) return 1;
  if (tmp2.type!=NUM_ARGS) {
    clear_var(&tmp2);
    return 1;
  }
  if (tmp2.value.num!=2) return 1;
  if (pop(&tmp2,rts,obj)) return 1;
  if (tmp2.type!=STRING) {
    clear_var(&tmp2);
    return 1;
  }
  if (pop(&tmp1,rts,obj)) {
    clear_var(&tmp2);
    return 1;
  }
  if (tmp1.type!=STRING) {
    clear_var(&tmp2);
    clear_var(&tmp1);
    return 1;
  }
  retval=copy_file(tmp1.value.string,tmp2.value.string,obj);
  clear_var(&tmp1);
  clear_var(&tmp2);
  tmp1.type=INTEGER;
  tmp1.value.integer=retval;
  push(&tmp1,rts);
  return 0;
}

int s_mv(struct object *caller, struct object *obj, struct object *player,
         struct var_stack **rts) {
  int retval;
  struct var tmp1,tmp2;

  if (pop(&tmp2,rts,obj)) return 1;
  if (tmp2.type!=NUM_ARGS) {
    clear_var(&tmp2);
    return 1;
  }
  if (tmp2.value.num!=2) return 1;
  if (pop(&tmp2,rts,obj)) return 1;
  if (tmp2.type!=STRING) {
    clear_var(&tmp2);
    return 1;
  }
  if (pop(&tmp1,rts,obj)) {
    clear_var(&tmp2);
    return 1;
  }
  if (tmp1.type!=STRING) {
    clear_var(&tmp2);
    clear_var(&tmp1);
    return 1;
  }
  retval=move_file(tmp1.value.string,tmp2.value.string,obj);
  clear_var(&tmp1);
  clear_var(&tmp2);
  tmp1.type=INTEGER;
  tmp1.value.integer=retval;
  push(&tmp1,rts);
  return 0;
}

int s_mkdir(struct object *caller, struct object *obj, struct object *player,
            struct var_stack **rts) {
  struct var tmp;
  int retval;

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
  retval=make_dir(tmp.value.string,obj);
  clear_var(&tmp);
  tmp.type=INTEGER;
  tmp.value.integer=retval;
  push(&tmp,rts);
  return 0;
}

int s_rmdir(struct object *caller, struct object *obj, struct object *player,
            struct var_stack **rts) {
  struct var tmp;
  int retval;

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
  retval=remove_dir(tmp.value.string,obj);
  clear_var(&tmp);
  tmp.type=INTEGER;
  tmp.value.integer=retval;
  push(&tmp,rts);
  return 0;
}

int s_hide(struct object *caller, struct object *obj, struct object *player,
           struct var_stack **rts) {
  int retval;
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
    tmp.value.integer=0;
    push(&tmp,rts);
    return 0;
  }
  retval=hide(tmp.value.string);
  clear_var(&tmp);
  tmp.type=INTEGER;
  tmp.value.integer=retval;
  push(&tmp,rts);
  return 0;
}

int s_unhide(struct object *caller, struct object *obj, struct object *player,
             struct var_stack **rts) {
  int retval;
  struct var tmp1,tmp2,tmp3;

  if (pop(&tmp3,rts,obj)) return 1;
  if (tmp3.type!=NUM_ARGS) {
    clear_var(&tmp3);
    return 1;
  }
  if (tmp3.value.num!=3) return 1;
  if (pop(&tmp3,rts,obj)) return 1;
  if (tmp3.type!=INTEGER) {
    clear_var(&tmp3);
    return 1;
  }
  if (pop(&tmp2,rts,obj)) return 1;
  if (tmp2.type!=OBJECT) {
    clear_var(&tmp2);
    return 1;
  }
  if (pop(&tmp1,rts,obj)) return 1;
  if (tmp1.type!=STRING) {
    clear_var(&tmp1);
    return 1;
  }
  if (!(obj->flags & PRIV)) {
    clear_var(&tmp1);
    tmp1.type=INTEGER;
    tmp1.value.integer=1;
    push(&tmp1,rts);
    return 0;
  }
  retval=unhide(tmp1.value.string,tmp2.value.objptr,tmp3.value.integer);
  clear_var(&tmp1);
  tmp1.type=INTEGER;
  tmp1.value.integer=retval;
  push(&tmp1,rts);
  return 0;
}

int s_chown(struct object *caller, struct object *obj, struct object *player,
            struct var_stack **rts) {
  int retval;
  struct var tmp1,tmp2;

  if (pop(&tmp1,rts,obj)) return 1;
  if (tmp1.type!=NUM_ARGS) {
    clear_var(&tmp1);
    return 1;
  }
  if (tmp1.value.num!=2) return 1;
  if (pop(&tmp2,rts,obj)) return 1;
  if (tmp2.type!=OBJECT) {
    clear_var(&tmp2);
    return 1;
  }
  if (pop(&tmp1,rts,obj)) return 1;
  if (tmp1.type!=STRING) {
    clear_var(&tmp1);
    return 1;
  }
  retval=chown_file(tmp1.value.string,obj,tmp2.value.objptr);
  clear_var(&tmp1);
  tmp1.type=INTEGER;
  tmp1.value.integer=retval;
  push(&tmp1,rts);
  return 0;
}

int s_syslog(struct object *caller, struct object *obj, struct object *player,
             struct var_stack **rts) {
  struct var tmp;
  char *buf;

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
  buf=MALLOC(ITOA_BUFSIZ+strlen(obj->parent->pathname)+strlen(tmp.value.string)
             +12);
  sprintf(buf," syslog: %s#%ld %s",obj->parent->pathname,(long) obj->refno,
          tmp.value.string);
  clear_var(&tmp);
  logger(LOG_INFO, buf);
  FREE(buf);
  tmp.type=INTEGER;
  tmp.value.integer=0;
  push(&tmp,rts);
  return 0;
}

int s_chmod(struct object *caller, struct object *obj, struct object *player,
            struct var_stack **rts) {
  struct var tmp1,tmp2;
  int retval;

  if (pop(&tmp2,rts,obj)) return 1;
  if (tmp2.type!=NUM_ARGS) {
    clear_var(&tmp2);
    return 1;
  }
  if (tmp2.value.num!=2) return 1;
  if (pop(&tmp2,rts,obj)) return 1;
  if (tmp2.type!=INTEGER) {
    clear_var(&tmp2);
    return 1;
  }
  if (pop(&tmp1,rts,obj)) return 1;
  if (tmp1.type!=STRING) {
    clear_var(&tmp1);
    return 1;
  }
  retval=chmod_file(tmp1.value.string,obj,tmp2.value.integer);
  clear_var(&tmp1);
  tmp1.type=INTEGER;
  tmp1.value.integer=retval;
  push(&tmp1,rts);
  return 0;
}

int s_fread(struct object *caller, struct object *obj, struct object *player,
            struct var_stack **rts) {
  struct var tmp1,tmp2;
  FILE *f;
  static char buf[MAX_STR_LEN];
  long pos;

  if (pop(&tmp2,rts,obj)) return 1;
  if (tmp2.type!=NUM_ARGS) {
    clear_var(&tmp2);
    return 1;
  }
  if (tmp2.value.num!=2) return 1;
  if (pop(&tmp2,rts,obj)) return 1;
  if (tmp2.type!=INTEGER && tmp2.type!=GLOBAL_L_VALUE && tmp2.type!=
      LOCAL_L_VALUE) {
    clear_var(&tmp2);
    return 1;
  }
  if (tmp2.type==INTEGER)
    pos=tmp2.value.integer;
  else {
    if (tmp2.value.l_value.size!=1) return 1;
    if (tmp2.type==LOCAL_L_VALUE) {
      if (locals[tmp2.value.l_value.ref].type!=INTEGER)  return 1;
      pos=locals[tmp2.value.l_value.ref].value.integer;
    } else {
      if (obj->globals[tmp2.value.l_value.ref].type!=INTEGER) return 1;
      pos=obj->globals[tmp2.value.l_value.ref].value.integer;
    }
  }
  if (pop(&tmp1,rts,obj)) return 1;
  if (resolve_var(&tmp1,obj)) return 1;
  if (tmp1.type!=STRING) {
    clear_var(&tmp1);
    return 1;
  }
  f=open_file(tmp1.value.string,FREAD_MODE,obj);
  clear_var(&tmp1);
  if (!f) {
    tmp1.type=INTEGER;
    tmp1.value.integer=-1;
    push(&tmp1,rts);
    return 0;
  }
  if (fseek(f,pos,SEEK_SET)) {
    close_file(f);
    tmp1.type=INTEGER;
    tmp1.value.integer=-1;
    push(&tmp1,rts);
    return 0;
  }
  if (fgets(buf,MAX_STR_LEN,f)) {
    tmp1.type=STRING;
    tmp1.value.string=buf;
    push(&tmp1,rts);
  } else {
    tmp1.type=INTEGER;
    tmp1.value.integer=0;
    push(&tmp1,rts);
  }
  pos=ftell(f);
  close_file(f);
  if (tmp2.type!=INTEGER)
    if (tmp2.type==LOCAL_L_VALUE) {
      clear_var(&(locals[tmp2.value.l_value.ref]));
      locals[tmp2.value.l_value.ref].type=INTEGER;
      locals[tmp2.value.l_value.ref].value.integer=pos;
    } else {
      clear_global_var(obj,tmp2.value.l_value.ref);
      obj->globals[tmp2.value.l_value.ref].type=INTEGER;
      obj->globals[tmp2.value.l_value.ref].value.integer=pos;
    }
  return 0;
}

int s_fwrite(struct object *caller, struct object *obj, struct object *player,
             struct var_stack **rts) {
  struct var tmp1,tmp2;
  FILE *f;

  if (pop(&tmp2,rts,obj)) return 1;
  if (tmp2.type!=NUM_ARGS) {
    clear_var(&tmp2);
    return 1;
  }
  if (tmp2.value.num!=2) return 1;
  if (pop(&tmp2,rts,obj)) return 1;
  if (tmp2.type!=STRING) {
    clear_var(&tmp2);
    return 1;
  }
  if (pop(&tmp1,rts,obj)) return 1;
  if (tmp1.type!=STRING) {
    clear_var(&tmp2);
    clear_var(&tmp1);
    return 1;
  }
  f=open_file(tmp1.value.string,FAPPEND_MODE,obj);
  clear_var(&tmp1);
  if (!f) {
    clear_var(&tmp2);
    tmp1.type=INTEGER;
    tmp1.value.integer=1;
    push(&tmp1,rts);
    return 0;
  }
  fputs(tmp2.value.string,f);
  close_file(f);
  clear_var(&tmp2);
  tmp1.type=INTEGER;
  tmp1.value.integer=0;
  push(&tmp1,rts);
  return 0;
}

int s_ferase(struct object *caller, struct object *obj, struct object *player,
             struct var_stack **rts) {
  struct var tmp;
  FILE *f;

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
  f=open_file(tmp.value.string,FWRITE_MODE,obj);
  clear_var(&tmp);
  if (!f) {
    tmp.type=INTEGER;
    tmp.value.integer=1;
    push(&tmp,rts);
    return 0;
  }
  close_file(f);
  tmp.type=INTEGER;
  tmp.value.integer=0;
  push(&tmp,rts);
  return 0;
}
