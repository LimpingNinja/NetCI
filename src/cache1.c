/* cache1.c */

#include "config.h"
#include "object.h"
#include "globals.h"
#include "constrct.h"
#include "dbhandle.h"
#include "interp.h"
#include "file.h"
#include "table.h"

struct obj_link {
  struct object *obj;
  struct obj_link *prev_link;
  struct obj_link *next_link;
  struct obj_link *prev_queue;
  struct obj_link *next_queue;
};

struct obj_link *hash_list[CACHE_HASH];
struct obj_link *cache_head;
struct obj_link *cache_tail;

signed long loaded_obj_count;

/* if access_load_file is 0, then all loads from the db will be from
   the save_name file. if it is non-zero, loads will be from the
   load_name file. */

int access_load_file;

signed long db_obj_to_ref(struct object *obj) {
  if (!obj) return -1;
  return obj->refno;
}

struct obj_link *find_cache(struct object *obj) {
  struct obj_link *curr_link;

  curr_link=hash_list[obj->refno%CACHE_HASH];
  while (curr_link) {
    if (curr_link->obj==obj)
      return curr_link;
    curr_link=curr_link->next_link;
  }
  return NULL;
}

void write_filesystem(FILE *outfile, struct file_entry *parent_dir,
                      char *parent_dir_name) {
  struct file_entry *curr_entry;
  char *buf;

  curr_entry=parent_dir->contents;
  while (curr_entry) {
    fprintf(outfile,"%s/%s\n%ld\n%ld\n",parent_dir_name,
            curr_entry->filename,
            (long) curr_entry->flags,
            (long) curr_entry->owner);
    if (curr_entry->contents) {
      buf=MALLOC(strlen(parent_dir_name)+strlen(curr_entry->filename)+2);
      sprintf(buf,"%s/%s",parent_dir_name,curr_entry->filename);
      write_filesystem(outfile,curr_entry,buf);
      FREE(buf);
    }
    curr_entry=curr_entry->next_file;
  }
}

void freedata(struct object *obj) {
  long loop;
  struct ref_list *curr,*next;

  loop=0;
  if (obj->globals) {
    while (loop<obj->parent->funcs->num_globals) {
      if (obj->globals[loop].type==STRING)
        FREE(obj->globals[loop].value.string);
      loop++;
    }
    FREE(obj->globals);
    obj->globals=NULL;
  }
  curr=obj->refd_by;
  while (curr) {
    next=curr->next;
    FREE(curr);
    curr=next;
  }
  obj->refd_by=NULL;
  obj->flags&=~(RESIDENT);
}

struct object *db_ref_to_obj(signed long refno) {
  struct obj_blk *curr;
  signed long count,index;

  if (refno<0) return NULL;
  count=refno/OBJ_ALLOC_BLKSIZ;
  index=refno%OBJ_ALLOC_BLKSIZ;
  curr=obj_list;
  while (count--)
    curr=curr->next;
  return &(curr->block[index]);
}

void init_globals(char *loadpath, char *savepath, char *panicpath) {
  signed long loop;

  cache_top=0;
  loop=0;
  while (loop<CACHE_HASH) {
    hash_list[loop]=NULL;
    loop++;
  }
  db_top=0;
  use_soft_cycles=1;
  use_hard_cycles=1;
  edit_list=NULL;
  cache_head=NULL;
  cache_tail=NULL;
  verbs_changed=0;
  doing_ls=0;
  loaded_obj_count=0;
  init_table();
  root_dir=MALLOC(sizeof(struct file_entry));
  root_dir->filename=copy_string("");
  root_dir->flags=READ_OK | DIRECTORY;
  root_dir->owner=0;
  root_dir->contents=NULL;
  root_dir->parent=NULL;
  root_dir->prev_file=NULL;
  root_dir->next_file=NULL;
  num_locals=0;
  locals=NULL;
  c_err_msg=NULL;
  cmd_head=NULL;
  cmd_tail=NULL;
  dest_list=NULL;
  alarm_list=NULL;
  now_time=time2int(time(NULL));
  obj_list=NULL;
  if (loadpath)
    load_name=copy_string(loadpath);
  else
    load_name=copy_string(DEFAULT_LOAD);
  if (savepath)
    save_name=copy_string(savepath);
  else
    save_name=copy_string(DEFAULT_SAVE);
  if (panicpath)
    panic_name=copy_string(panicpath);
  else
    panic_name=copy_string(DEFAULT_PANIC);
  free_obj_list=NULL;
  objects_allocd=0;
  db_top=0;
}

void pass_data(FILE *infile) {
  char c;
  char buf[ITOA_BUFSIZ+1];
  int done;
  long count;

  done=0;
  while (!done) {
    c=fgetc(infile);
    switch (c) {
      case 'I':
      case 'O':
        fgets(buf,ITOA_BUFSIZ+1,infile);
        break;
      case 'S':
        fgets(buf,ITOA_BUFSIZ+1,infile);
        count=atol(buf);
        while (count--) fgetc(infile);
        break;
      case '.':
        done=1;
        break;
      case EOF:
        return;
        break;
    }
  }
  fgets(buf,ITOA_BUFSIZ+1,infile);
  c=fgetc(infile);
  while (c!='.' && c!=EOF) c=fgetc(infile);
  fgets(buf,ITOA_BUFSIZ+1,infile);
}

void writeinstr(FILE *outfile,struct var *v) {
  if (v->type==FUNC_CALL || v->type==EXTERN_FUNC)
    fprintf(outfile,"%d\n",(int) FUNC_NAME);
  else
    fprintf(outfile,"%d\n",(int) v->type);
  switch (v->type) {
    case INTEGER:
      fprintf(outfile,"%ld\n",(long) v->value.integer);
      break;
    case STRING:
    case FUNC_NAME:
    case EXTERN_FUNC:
      fprintf(outfile,"%ld\n%s",(long) strlen(v->value.string),
              v->value.string);
      break;
    case OBJECT:
      fprintf(outfile,"%ld\n",(long) v->value.objptr->refno);
      break;
    case ASM_INSTR:
      fprintf(outfile,"%d\n",(int) v->value.instruction);
      break;
    case GLOBAL_L_VALUE:
    case LOCAL_L_VALUE:
      fprintf(outfile,"%d\n%d\n",(int) v->value.l_value.ref,
              (int) v->value.l_value.size);
      break;
    case FUNC_CALL:
      fprintf(outfile,"%ld\n%s",(long) strlen(v->value.func_call->funcname),
              v->value.func_call->funcname);
      break;
    case NUM_ARGS:
    case ARRAY_SIZE:
    case JUMP:
    case BRANCH:
    case NEW_LINE:
      fprintf(outfile,"%ld\n",(long) v->value.num);
      break;
    default:
      break;
  }
}

char *readstring(FILE *infile) {
  static char buf[ITOA_BUFSIZ+1];
  static char *string;
  static long len,loop;

  fgets(buf,ITOA_BUFSIZ+1,infile);
  len=atol(buf);
  loop=0;
  string=MALLOC(len+1);
  while (loop<len) {
    string[loop]=fgetc(infile);
    loop++;
  }
  string[loop]='\0';
  return string;
}

void readinstr(FILE *infile,struct var *v) {
  static char type;
  static char buf[ITOA_BUFSIZ+1];

  fgets(buf,ITOA_BUFSIZ+1,infile);
  type=atoi(buf);
  v->type=type;
  switch (type) {
    case INTEGER:
      fgets(buf,ITOA_BUFSIZ+1,infile);
      v->value.integer=atol(buf);
      break;
    case STRING:
    case FUNC_NAME:
      v->value.string=readstring(infile);
      break;
    case OBJECT:
      fgets(buf,ITOA_BUFSIZ+1,infile);
      v->value.objptr=db_ref_to_obj(atol(buf));
      break;
    case ASM_INSTR:
      fgets(buf,ITOA_BUFSIZ+1,infile);
      v->value.instruction=atoi(buf);
      break;
    case GLOBAL_L_VALUE:
    case LOCAL_L_VALUE:
      fgets(buf,ITOA_BUFSIZ+1,infile);
      v->value.l_value.ref=atoi(buf);
      fgets(buf,ITOA_BUFSIZ+1,infile);
      v->value.l_value.size=atoi(buf);
      break;
    case NUM_ARGS:
    case ARRAY_SIZE:
    case JUMP:
    case BRANCH:
    case NEW_LINE:
      fgets(buf,ITOA_BUFSIZ+1,infile);
      v->value.num=atol(buf);
      break;
    default:
      break;
  }
}

void writedata(FILE *outfile,struct object *obj) {
  long loop;
  struct ref_list *curr;

  loop=0;
  if (obj->flags & GARBAGE) {
    fprintf(outfile,".END\n.END\n");
    return;
  }
  while (loop<obj->parent->funcs->num_globals) {
    switch (obj->globals[loop].type) {
      case INTEGER:
        fprintf(outfile,"I%ld\n",(long) obj->globals[loop].value.integer);
        break;
      case OBJECT:
        fprintf(outfile,"O%ld\n",(long) obj->globals[loop].value.objptr->
                refno);
        break;
      case STRING:
        fprintf(outfile,"S%ld\n%s",(long) strlen(obj->globals[loop].value.
                string),obj->globals[loop].value.string);
        break;
      default:
        fprintf(outfile,"?");
        break;
    }
    loop++;
  }
  fprintf(outfile,".END\n");
  curr=obj->refd_by;
  while (curr) {
    fprintf(outfile,"%ld\n%ld\n",(long) curr->ref_obj->refno,
            (long) curr->ref_num);
    curr=curr->next;
  }
  fprintf(outfile,".END\n");
}

void readdata(FILE *infile, struct object *obj) {
  long loop;
  char c;
  char buf[ITOA_BUFSIZ+1];
  struct ref_list *curr;

  loop=0;
  if (obj->parent->funcs->num_globals)
    obj->globals=MALLOC(sizeof(struct var)*(obj->parent->funcs->num_globals));
  else
    obj->globals=NULL;
  while (loop<obj->parent->funcs->num_globals) {
    c=fgetc(infile);
    switch (c) {
      case 'I':
        obj->globals[loop].type=INTEGER;
        fgets(buf,ITOA_BUFSIZ+1,infile);
        obj->globals[loop].value.integer=atol(buf);
        break;
      case 'O':
        obj->globals[loop].type=OBJECT;
        fgets(buf,ITOA_BUFSIZ+1,infile);
        obj->globals[loop].value.objptr=db_ref_to_obj(atol(buf));
        break;
      case 'S':
        obj->globals[loop].type=STRING;
        obj->globals[loop].value.string=readstring(infile);
        break;
      default:
        obj->globals[loop].type=INTEGER;
        obj->globals[loop].value.integer=0;
        break;
    }
    loop++;
  }
  fgets(buf,ITOA_BUFSIZ+1,infile);
  if (feof(infile) || strcmp(buf,".END\n")) {
    log_sysmsg("  cache: readdata() encountered corrupt data");
    FATAL_ERROR();
    return;
  }
  fgets(buf,ITOA_BUFSIZ+1,infile);
  while (!feof(infile) && strcmp(buf,".END\n")) {
    curr=MALLOC(sizeof(struct ref_list));
    curr->ref_obj=db_ref_to_obj(atol(buf));
    fgets(buf,ITOA_BUFSIZ+1,infile);
    curr->ref_num=atoi(buf);
    curr->next=obj->refd_by;
    obj->refd_by=curr;
    fgets(buf,ITOA_BUFSIZ+1,infile);
  }
}

void unload_object(struct object *obj) {
  struct obj_link *this_link;

  this_link=find_cache(obj);
  if (this_link) {
    loaded_obj_count--;
    if (this_link->prev_link)
      this_link->prev_link->next_link=this_link->next_link;
    else
      hash_list[obj->refno%CACHE_HASH]=this_link->next_link;
    if (this_link->next_link)
      this_link->next_link->prev_link=this_link->prev_link;
    if (this_link->prev_queue)
      this_link->prev_queue->next_queue=this_link->next_queue;
    else
      cache_head=this_link->next_queue;
    if (this_link->next_queue)
      this_link->next_queue->prev_queue=this_link->prev_queue;
    else
      cache_tail=this_link->prev_queue;
    FREE(this_link);
  }
  if (obj->obj_state==FROM_DB || obj->obj_state==FROM_CACHE ||
      obj->obj_state==DIRTY)
    freedata(obj);
  obj->obj_state=DIRTY;
  obj->flags&=~(RESIDENT);
}

void add_loaded(struct object *obj) {
  signed long hash_value;
  struct obj_link *new_link;

  if (!obj) return;
  if ((new_link=find_cache(obj))) {
    if (new_link->prev_queue)
      new_link->prev_queue->next_queue=new_link->next_queue;
    if (new_link->next_queue)
      new_link->next_queue->prev_queue=new_link->prev_queue;
    if (new_link==cache_tail)
      cache_tail=new_link->prev_queue;
    if (new_link==cache_head)
      cache_head=new_link->next_queue;
  } else {
    hash_value=obj->refno%CACHE_HASH;
    new_link=MALLOC(sizeof(struct obj_link));
    new_link->obj=obj;
    new_link->prev_link=NULL;
    new_link->next_link=hash_list[hash_value];
    if (hash_list[hash_value])
      hash_list[hash_value]->prev_link=new_link;
    hash_list[hash_value]=new_link;
    loaded_obj_count++;
  }
  obj->flags|=RESIDENT;
  if (cache_head)
    cache_head->prev_queue=new_link;
  new_link->next_queue=cache_head;
  new_link->prev_queue=NULL;
  cache_head=new_link;
  if (!cache_tail)
    cache_tail=new_link;
}

void unload_data() {
  struct obj_link *curr_link;
  FILE *outfile;
  signed long place;

  while (loaded_obj_count>CACHE_SIZE) {
    curr_link=cache_tail;
    curr_link->prev_queue->next_queue=NULL;
    cache_tail=curr_link->prev_queue;
    if (curr_link->prev_link)
      curr_link->prev_link->next_link=curr_link->next_link;
    else
      hash_list[curr_link->obj->refno%CACHE_HASH]=curr_link->next_link;
    if (curr_link->next_link)
      curr_link->next_link->prev_link=curr_link->prev_link;
    if (curr_link->obj->flags & GARBAGE) {
      curr_link->obj->obj_state=DIRTY;
    } else
      if (curr_link->obj->obj_state==DIRTY) {
        outfile=fopen(transact_log_name,FAPPEND_MODE);
        if (!outfile) {
          log_sysmsg("  cache: unload_data() unable to write to cache");
          FATAL_ERROR();
          return;
	}
        place=ftell(outfile);
        writedata(outfile,curr_link->obj);
        cache_top=ftell(outfile);
        fclose(outfile);
        curr_link->obj->obj_state=IN_CACHE;
        curr_link->obj->file_offset=place;
      } else
        if (curr_link->obj->obj_state==FROM_DB)
          curr_link->obj->obj_state=IN_DB;
        else
          if (curr_link->obj->obj_state==FROM_CACHE)
            curr_link->obj->obj_state=IN_CACHE;
    freedata(curr_link->obj);
    FREE(curr_link);
    loaded_obj_count--;
  }
}

void load_data(struct object *obj) {
  FILE *infile;

  if (obj->flags & RESIDENT) {
    add_loaded(obj);
    return;
  }
  if (obj->flags & GARBAGE) return;
  if (obj->obj_state==IN_CACHE) {
    infile=fopen(transact_log_name,FREAD_MODE);
    if (!infile) {
      log_sysmsg("  cache: load_data() unable to read from cache");
      FATAL_ERROR();
      return;
    }
    fseek(infile,obj->file_offset,SEEK_SET);
    readdata(infile,obj);
    fclose(infile);
    obj->obj_state=FROM_CACHE;
  } else
    if (obj->obj_state==IN_DB) {
      infile=fopen((access_load_file ? load_name : save_name),FREAD_MODE);
      if (!infile) {
        log_sysmsg("  cache: load_data() unable to read from database");
        FATAL_ERROR();
        return;
      }
      fseek(infile,obj->file_offset,SEEK_SET);
      readdata(infile,obj);
      fclose(infile);
      obj->obj_state=FROM_DB;
    }
  add_loaded(obj);
}

void writeattachees(FILE *outfile, struct attach_list *curr_attach) {
  if (!curr_attach) return;
  writeattachees(outfile,curr_attach->next);
  fprintf(outfile,"%ld\n",(long) db_obj_to_ref(curr_attach->attachee));
}

void writeverb(FILE *outfile, struct verb *curr_verb) {
  if (!curr_verb) return;
  writeverb(outfile,curr_verb->next);
  fprintf(outfile,"%d\n%ld\n%s%ld\n%s",(int) curr_verb->is_xverb,
          (long) strlen(curr_verb->verb_name),
          curr_verb->verb_name,
          (long) strlen(curr_verb->function),
          curr_verb->function);
}
