/* dbhandle.c */

#include "config.h"
#include "object.h"
#include "globals.h"
#include "constrct.h"
#include "intrface.h"
#include "dbhandle.h"
#include "file.h"

void db_queue_for_alarm(struct object *obj, long delay, char *funcname) {
   struct alarmq *new,*curr,*prev;

   remove_alarm(obj,funcname);
   new=MALLOC(sizeof(struct alarmq));
   new->obj=obj;
   new->funcname=copy_string(funcname);
   new->delay=delay;
   curr=alarm_list;
   prev=NULL;
   while (curr) {
     if (new->delay<curr->delay) {
       if (prev)
         prev->next=new;
       else
         alarm_list=new;
       new->next=curr;
       return;
     }
     prev=curr;
     curr=curr->next;
   }
   if (prev)
     prev->next=new;
   else
     alarm_list=new;
  new->next=NULL;
}

void remove_verb(struct object *obj, char *verb_name) {
  struct verb *curr,*prev;

  curr=obj->verb_list;
  if (!curr) return;
  if (!strcmp(curr->verb_name,verb_name)) {
    FREE(curr->verb_name);
    FREE(curr->function);
    obj->verb_list=curr->next;
    FREE(curr);
    return;
  }
  prev=curr;
  curr=prev->next;
  while (curr) {
    if (!strcmp(curr->verb_name,verb_name)) {
      FREE(curr->verb_name);
      FREE(curr->function);
      prev->next=curr->next;
      FREE(curr);
      return;
    }
    prev=curr;
    curr=curr->next;
  }
}

void queue_command(struct object *player, char *cmd) {
  struct cmdq *new;

  if (!player) return;
  new=MALLOC(sizeof(struct cmdq));
  new->cmd=copy_string(cmd);
  new->obj=player;
  new->next=NULL;
  if (cmd_tail)
    cmd_tail->next=new;
  if (!cmd_head)
    cmd_head=new;
  cmd_tail=new;
}

void queue_for_destruct(struct object *obj) {
  struct destq *new;

  new=dest_list;
  while (new) {
    if (new->obj==obj) return;
    new=new->next;
  }
  new=MALLOC(sizeof(struct destq));
  new->obj=obj;
  new->next=dest_list;
  dest_list=new;
}

void queue_for_alarm(struct object *obj, long delay, char *funcname) {
   struct alarmq *new,*curr,*prev;

   if (delay<0) return;
   remove_alarm(obj,funcname);
   new=MALLOC(sizeof(struct alarmq));
   new->obj=obj;
   new->funcname=copy_string(funcname);
   new->delay=now_time+delay;
   curr=alarm_list;
   prev=NULL;
   while (curr) {
     if (new->delay<curr->delay) {
       if (prev)
         prev->next=new;
       else
         alarm_list=new;
       new->next=curr;
       return;
     }
     prev=curr;
     curr=curr->next;
   }
   if (prev)
     prev->next=new;
   else
     alarm_list=new;
  new->next=NULL;
}

long remove_alarm(struct object *obj, char *funcname) {
  struct alarmq *curr,*prev,*tmp;
  long result;

  curr=alarm_list;
  prev=NULL;
  if (!funcname) {
    while (curr) {
      if (obj==curr->obj) {
        if (prev)
          prev->next=curr->next;
        else
          alarm_list=curr->next;
        tmp=curr->next;
        FREE(curr->funcname);
        FREE(curr);
        curr=tmp;
      } else {
        prev=curr;
        curr=curr->next;
      }
    }
    return 0;
  }
  while (curr) {
    if (obj==curr->obj && (!strcmp(funcname,curr->funcname))) {
      if (prev)
        prev->next=curr->next;
      else
        alarm_list=curr->next;
      FREE(curr->funcname);
      result=curr->delay-now_time;
      FREE(curr);
      return result;
    }
    prev=curr;
    curr=curr->next;
  }
  return -1;
}

struct object *newobj() {
  struct obj_blk *newblock,*curr;
  struct object *obj;
  signed long count;

  if (free_obj_list) {
    obj=free_obj_list;
    free_obj_list=free_obj_list->next_object;
  } else {
    if (db_top==objects_allocd) {
      if (!obj_list) {
        obj_list=MALLOC(sizeof(struct obj_blk));
        obj_list->next=NULL;
        obj_list->block=MALLOC(sizeof(struct object)*OBJ_ALLOC_BLKSIZ);
      } else {
        curr=obj_list;
        while (curr->next) curr=curr->next;
        newblock=MALLOC(sizeof(struct obj_blk));
        curr->next=newblock;
        newblock->next=NULL;
        newblock->block=MALLOC(sizeof(struct object)*OBJ_ALLOC_BLKSIZ);
      }
      objects_allocd+=OBJ_ALLOC_BLKSIZ;
    }
    curr=obj_list;
    count=++db_top;
    while (count>OBJ_ALLOC_BLKSIZ) {
      curr=curr->next;
      count-=OBJ_ALLOC_BLKSIZ;
    }
    obj=&(curr->block[count-1]);
    obj->refno=db_top-1;
  }
  obj->devnum=-1;
  obj->input_func=NULL;
  obj->input_func_obj=NULL;
  obj->flags=0;
  obj->parent=NULL;
  obj->next_child=NULL;
  obj->location=NULL;
  obj->contents=NULL;
  obj->next_object=NULL;
  obj->globals=NULL;
  obj->refd_by=NULL;
  obj->verb_list=NULL;
  obj->attachees=NULL;
  obj->attacher=NULL;
  return obj;
}

struct object *find_proto(char *path) {
  struct object *obj;
  struct proto *curr;

  obj=ref_to_obj(0);
  curr=obj->parent;
  while (curr) {
    if (!strcmp(curr->pathname,path))
      return curr->proto_obj;
    curr=curr->next_proto;
  }
  return NULL;
}

void compile_error(struct object *player, char *path, unsigned int line) {
  char *buf;

  if (!c_err_msg) c_err_msg="unknown error";
  buf=MALLOC(21+ITOA_BUFSIZ+strlen(path)+strlen(c_err_msg));
  sprintf(buf,"compile: %s.c line #%ld: %s",path,(long) line,c_err_msg);
  logger(LOG_ERROR, buf);
  if (player) {
    send_device(player,buf);
    send_device(player,"\n");
  }
  FREE(buf);
}

struct object *ref_to_obj(signed long refno) {
  struct obj_blk *curr;
  struct object *obj;
  signed long count,index;

  if (refno>(db_top-1)) return NULL;
  if (refno<0) return NULL;
  count=refno/OBJ_ALLOC_BLKSIZ;
  index=refno%OBJ_ALLOC_BLKSIZ;
  curr=obj_list;
  while (count--)
    curr=curr->next;
  obj=&(curr->block[index]);
  if (obj->flags & GARBAGE) return NULL;
  return obj;
}
