/* dbhandle.c */

#include "config.h"
#include "object.h"
#include "globals.h"
#include "constrct.h"
#include "intrface.h"
#include "dbhandle.h"
#include "file.h"
#include "interp.h"
#include "protos.h"

/* Forward declarations */
int call_function_on_object(struct object *obj, char *func_name, 
                            struct var_stack **rts, int num_args);
int call_function_on_object_with_int(struct object *obj, char *func_name, 
                                      int arg_value);

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
  char logbuf[256];

  new=dest_list;
  while (new) {
    if (new->obj==obj) return;
    new=new->next;
  }
  
  /* Log lifecycle event */
  sprintf(logbuf, "lifecycle: Object %s#%ld queued for destruction",
          obj->parent ? obj->parent->pathname : "NULL",
          (long)obj->refno);
  logger(LOG_INFO, logbuf);
  
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
  obj->last_access_time=now_time;  /* Initialize to current time */
  obj->heart_beat_interval=0;      /* Heartbeat disabled by default */
  obj->last_heart_beat=0;
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

/* Helper to read source line - similar to interp.c's read_source_line */
static char *read_compile_source_line(char *filepath, unsigned int line_num) {
  FILE *f;
  char *line_buf;
  char *full_path;
  unsigned int current_line;
  int c, pos;
  
  /* Build full filesystem path */
  full_path = MALLOC(strlen(fs_path) + strlen(filepath) + 3);
  strcpy(full_path, fs_path);
  strcat(full_path, filepath);
  strcat(full_path, ".c");
  
  f = fopen(full_path, "r");
  FREE(full_path);
  
  if (!f) return NULL;
  
  line_buf = MALLOC(512);
  current_line = 1;
  pos = 0;
  
  while ((c = fgetc(f)) != EOF) {
    if (current_line == line_num) {
      if (c == '\n' || c == '\r') {
        line_buf[pos] = '\0';
        fclose(f);
        return line_buf;
      } else if (pos < 511) {
        line_buf[pos++] = c;
      }
    } else if (c == '\n') {
      current_line++;
      if (current_line > line_num) break;
    }
  }
  
  if (current_line == line_num && pos > 0) {
    line_buf[pos] = '\0';
    fclose(f);
    return line_buf;
  }
  
  FREE(line_buf);
  fclose(f);
  return NULL;
}

void compile_error(struct object *player, char *path, unsigned int line) {
  char *buf;
  char *source_line;
  char *trimmed;
  unsigned int display_line = line;
  unsigned int context_start, context_end;
  unsigned int i;
  int is_error_line;

  if (!c_err_msg) c_err_msg="unknown error";
  
  /* Log the error header */
  buf=MALLOC(21+ITOA_BUFSIZ+strlen(path)+strlen(c_err_msg));
  sprintf(buf,"compile: %s.c line #%ld: %s",path,(long) line,c_err_msg);
  logger(LOG_ERROR, buf);
  
  if (player) {
    send_device(player,buf);
    send_device(player,"\n");
  }
  FREE(buf);
  
  /* Check if reported line is a comment or blank - if so, error is on next line */
  source_line = read_compile_source_line(path, line);
  if (source_line) {
    trimmed = source_line;
    while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
    
    if ((*trimmed == '/' && *(trimmed+1) == '/') || 
        *trimmed == '\0' || 
        (*trimmed == '/' && *(trimmed+1) == '*')) {
      display_line = line + 1;
    }
    FREE(source_line);
  }
  
  /* Show context: 2 lines before and 2 lines after the error line */
  context_start = (display_line > 2) ? display_line - 2 : 1;
  context_end = display_line + 2;
  
  for (i = context_start; i <= context_end; i++) {
    source_line = read_compile_source_line(path, i);
    if (!source_line) continue;  /* Skip if line doesn't exist */
    
    trimmed = source_line;
    while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
    
    /* Skip completely blank lines in context */
    if (*trimmed == '\0') {
      FREE(source_line);
      continue;
    }
    
    is_error_line = (i == display_line);
    
    /* Format: "      Line N: code" or " >>>> Line N: code" for error line */
    buf = MALLOC(strlen(trimmed) + ITOA_BUFSIZ + 30);
    if (is_error_line) {
      sprintf(buf, " >>>> Line %ld: %s", (long)i, trimmed);
    } else {
      sprintf(buf, "      Line %ld: %s", (long)i, trimmed);
    }
    
    logger(LOG_ERROR, buf);
    if (player) {
      send_device(player, buf);
      send_device(player, "\n");
    }
    
    FREE(buf);
    FREE(source_line);
  }
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

/* call_reset_on_all() - Call reset() apply on all non-exempt objects
 *
 * Iterates through all objects and calls their reset() function if they:
 * - Are not the boot object (/boot.c)
 * - Are not the auto object (from config)
 * - Are not prototypes (only clones get reset)
 * - Have been idle long enough (configurable)
 */
void call_reset_on_all() {
  struct obj_blk *curr_block;
  struct object *obj;
  struct object *boot_obj;
  signed long i;
  int count = 0;
  char logbuf[256];
  
  /* Get boot object for exemption check */
  boot_obj = find_proto("/boot");
  
  /* Walk all object blocks */
  curr_block = obj_list;
  while (curr_block) {
    for (i = 0; i < OBJ_ALLOC_BLKSIZ && ((curr_block->block[i].refno) < db_top); i++) {
      obj = &(curr_block->block[i]);
      
      /* Skip if garbage */
      if (obj->flags & GARBAGE) continue;
      
      /* Skip boot object */
      if (boot_obj && obj == boot_obj) continue;
      
      /* Skip auto object */
      if (auto_proto && obj == auto_proto) continue;
      
      /* Skip prototypes - only reset clones */
      if (obj->parent && obj == obj->parent->proto_obj) continue;
      
      /* Skip uninitialized objects (no proto/funcs yet) */
      if (!obj->parent || !obj->parent->funcs) continue;

      /* Call reset() if it exists */
      if (find_function("reset", obj, NULL)) {
        call_function_on_object(obj, "reset", NULL, 0);
        count++;
      }
    }
    curr_block = curr_block->next;
  }
  
  sprintf(logbuf, " system: reset() called on %d objects", count);
  logger(LOG_INFO, logbuf);
}

/* call_cleanup_on_all() - Call clean_up() apply on idle objects
 *
 * Iterates through all objects and calls their clean_up(int refs) function if:
 * - They meet the same exemptions as reset()
 * - They have been idle (not accessed) for sufficient time
 * - If clean_up() returns 1, the object is marked for destruction
 */
void call_cleanup_on_all() {
  struct obj_blk *curr_block;
  struct object *obj;
  struct object *boot_obj;
  struct ref_list *refs;
  signed long i;
  int count = 0;
  int destructed = 0;
  int ref_count;
  int result;
  char logbuf[256];
  long idle_threshold = 600; /* Only cleanup objects idle for 10+ minutes */
  /* Get boot object for exemption check */
  boot_obj = find_proto("/boot");
  
  /* Walk all object blocks */
  curr_block = obj_list;
  while (curr_block) {
    for (i = 0; i < OBJ_ALLOC_BLKSIZ && ((curr_block->block[i].refno) < db_top); i++) {
      obj = &(curr_block->block[i]);
      
      /* Skip if garbage */
      if (obj->flags & GARBAGE) continue;
      
      /* Skip boot object */
      if (boot_obj && obj == boot_obj) continue;
      
      /* Skip auto object */
      if (auto_proto && obj == auto_proto) continue;
      
      /* Skip prototypes */
      if (obj->parent && obj == obj->parent->proto_obj) continue;
      
      /* Skip uninitialized objects (no proto/funcs yet) */
      if (!obj->parent || !obj->parent->funcs) continue;

      /* Skip objects in another object's inventory - they inherit parent's lifecycle */
      if (obj->location) continue;
      
      /* Skip objects containing an INTERACTIVE (rooms with players) */
      struct object *contents = obj->contents;
      int has_interactive = 0;
      while (contents) {
        if (contents->flags & INTERACTIVE) {
          has_interactive = 1;
          break;
        }
        contents = contents->next_object;
      }
      if (has_interactive) continue;

      /* Skip recently accessed objects */
      long idle_time = now_time - obj->last_access_time;
      if (idle_time < idle_threshold) continue;
      
      /* Log which object passed the threshold */
      sprintf(logbuf, "  clean_up: obj#%ld passed threshold (idle: %ld sec, threshold: %ld sec, now: %ld, last_access: %ld)",
              (long)obj->refno, idle_time, idle_threshold, (long)now_time, (long)obj->last_access_time);
      logger(LOG_INFO, logbuf);
      
      /* Count references to this object */
      ref_count = 0;
      refs = obj->refd_by;
      while (refs) {
        ref_count++;
        refs = refs->next;
      }
      
      /* Call clean_up(refs) if it exists */
      if (find_function("clean_up", obj, NULL)) {
        sprintf(logbuf, "  clean_up: CALLING clean_up() on %s#%ld (now: %ld, last_access: %ld, idle: %ld sec, refs: %d)",
                obj->parent ? obj->parent->pathname : "NULL",
                (long)obj->refno,
                (long)now_time,
                (long)obj->last_access_time,
                idle_time,
                ref_count);
        logger(LOG_INFO, logbuf);
        
        result = call_function_on_object_with_int(obj, "clean_up", ref_count);
        count++;
        
        /* If clean_up returned 1, queue for destruction */
        if (result == 1) {
          queue_for_destruct(obj);
          destructed++;
        }
      }
    }
    curr_block = curr_block->next;
  }
  
  sprintf(logbuf, " system: clean_up() called on %d objects, %d marked for destruction", 
          count, destructed);
  logger(LOG_INFO, logbuf);
}

/* Call heart_beat() on all objects that have it enabled
 * This is called every pulse by the game loop
 */
void call_heart_beat_on_all() {
  struct obj_blk *curr_block;
  struct object *obj;
  signed long i;
  int count = 0;
  
  /* Walk all object blocks */
  curr_block = obj_list;
  while (curr_block) {
    for (i = 0; i < OBJ_ALLOC_BLKSIZ && ((curr_block->block[i].refno) < db_top); i++) {
      obj = &(curr_block->block[i]);
      
      /* Skip if garbage */
      if (obj->flags & GARBAGE) continue;
      
      /* Skip uninitialized objects (no proto/funcs yet) */
      if (!obj->parent || !obj->parent->funcs) continue;
      
      /* Skip if heart_beat not enabled */
      if (obj->heart_beat_interval <= 0) continue;
      
      /* Check if enough time has passed since last heart_beat */
      if (now_time - obj->last_heart_beat < obj->heart_beat_interval) continue;
      
      /* Update last heartbeat time */
      obj->last_heart_beat = now_time;
      
      /* Call heart_beat() if it exists */
      if (find_function("heart_beat", obj, NULL)) {
        call_function_on_object(obj, "heart_beat", NULL, 0);
        count++;
      }
    }
    curr_block = curr_block->next;
  }
}

/* Helper: Call a function with no arguments on an object
 * Returns 0 on success, 1 on error
 */
int call_function_on_object(struct object *obj, char *func_name, 
                            struct var_stack **rts, int num_args) {
  struct fns *func;
  struct object *real_obj;
  struct var tmp;
  struct var_stack *local_rts = NULL;
  struct var *old_locals;
  unsigned int old_num_locals;
  int ret;
  
  func = find_function(func_name, obj, &real_obj);
  if (!func || !real_obj) return 1;
  
  /* Push NUM_ARGS marker */
  tmp.type = NUM_ARGS;
  tmp.value.num = num_args;
  push(&tmp, &local_rts);
  
  /* Call the function */
  old_locals = locals;
  old_num_locals = num_locals;
  ret = interp(obj, real_obj, NULL, &local_rts, func);
  locals = old_locals;
  num_locals = old_num_locals;
  
  /* Clean up stack */
  free_stack(&local_rts);
  
  return ret;
}

/* Helper: Call clean_up(int refs) and return its result
 * Returns the integer result from clean_up(), or 0 if error
 */
int call_function_on_object_with_int(struct object *obj, char *func_name, 
                                      int arg_value) {
  struct fns *func;
  struct object *real_obj;
  struct var tmp, result;
  struct var_stack *local_rts = NULL;
  struct var *old_locals;
  unsigned int old_num_locals;
  int ret;
  int return_value = 0;
  
  func = find_function(func_name, obj, &real_obj);
  if (!func || !real_obj) return 0;
  
  /* Push the integer argument */
  tmp.type = INTEGER;
  tmp.value.integer = arg_value;
  push(&tmp, &local_rts);
  
  /* Push NUM_ARGS marker */
  tmp.type = NUM_ARGS;
  tmp.value.num = 1;
  push(&tmp, &local_rts);
  
  /* Call the function */
  old_locals = locals;
  old_num_locals = num_locals;
  ret = interp(obj, real_obj, NULL, &local_rts, func);
  locals = old_locals;
  num_locals = old_num_locals;
  
  /* Get return value from stack */
  if (!ret && local_rts) {
    if (!pop(&result, &local_rts, obj)) {
      if (result.type == INTEGER) {
        return_value = result.value.integer;
      }
      clear_var(&result);
    }
  }
  
  /* Clean up stack */
  free_stack(&local_rts);
  
  return return_value;
}
