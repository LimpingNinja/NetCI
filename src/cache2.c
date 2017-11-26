/* cache2.c */
#include "config.h"
#include "object.h"
#include "globals.h"
#include "constrct.h"
#include "dbhandle.h"
#include "interp.h"
#include "file.h"
#include "compile.h"
#include "clearq.h"
#include "table.h"
#include "cache.h"

#ifdef USE_WINDOWS
#include <winsock.h>
#include "winmain.h"
#endif /* USE_WINDOWS */

struct obj_link {
  struct object *obj;
  struct obj_link *prev_link;
  struct obj_link *next_link;
  struct obj_link *prev_queue;
  struct obj_link *next_queue;
};

extern struct obj_link *hash_list[CACHE_HASH];
extern struct obj_link *cache_head;
extern struct obj_link *cache_tail;

extern signed long loaded_obj_count;

/* if access_load_file is 0, then all loads from the db will be from
   the save_name file. if it is non-zero, loads will be from the
   load_name file. */

extern int access_load_file;

struct object *db_ref_to_obj(signed long refno);
void pass_data(FILE *infile);
void writeinstr(FILE *outfile, struct var *v);
char *readstring(FILE *infile);
void readinstr(FILE *infile, struct var *v);
void writedata(FILE *outfile, struct object *obj);
void readdata(FILE *infile, struct object *obj);
void freedata(struct object *obj);
void write_filesystem(FILE *outfile, struct file_entry *parent_dir,
                      char *parent_dir_name);
signed long db_obj_to_ref(struct object *obj);
void writeattachees(FILE *outfile, struct attach_list *curr_attach);
void writeverb(FILE *outfile, struct verb *curr_verb);

int manual_move(char *oldname, char *newname) {
  FILE *oldf,*newf;
  int c;

  oldf=fopen(oldname,FREAD_MODE);
  if (!oldf) return 1;
  newf=fopen(newname,FWRITE_MODE);
  if (!newf) {
    fclose(oldf);
    return 1;
  }
  c=fgetc(oldf);
  while (c!=(-1)) {
    fputc(c,newf);
    c=fgetc(oldf);
  }
  fclose(oldf);
  fclose(newf);
  remove(oldname);
  return 0;
}

int save_db(char *filename) {
  long count,index,place,loop;
  struct obj_blk *curr_block;
  struct cmdq *curr_cmd;
  struct alarmq *curr_alarm;
  struct object *curr_obj;
  struct file_entry *curr_file;
  FILE *outfile,*infile,*cachefile;
  struct proto *curr_proto;
  struct fns *curr_fns;
  struct var_tab *curr_var;
  struct array_size *curr_array;

#ifdef USE_WINDOWS
  AddSavingToTitle();
#endif /* USE_WINDOWS */
  cachefile=fopen(transact_log_name,FREAD_MODE);
  outfile=fopen(tmpdb_name,FWRITE_MODE);
  if (!outfile) {
#ifdef USE_WINDOWS
    RemoveSavingFromTitle();
#endif /* USE_WINDOWS */
    if (cachefile) fclose(cachefile);
    log_sysmsg("  cache: save_db() couldn't write to temporary database");
    FATAL_ERROR();
    return 1;
  }
  if (access_load_file==1)
    infile=fopen(load_name,FREAD_MODE);
  else
    if (access_load_file==0)
      infile=fopen(save_name,FREAD_MODE);
    else
      infile=NULL;
  if (access_load_file!=(-1) && !infile) {
#ifdef USE_WINDOWS
    RemoveSavingFromTitle();
#endif /* USE_WINDOWS */
    log_sysmsg("  cache: save_db() couldn't read from database");
    FATAL_ERROR();
    fclose(outfile);
    return 1;
  }
  count=0;
  index=0;
  curr_block=obj_list;
  curr_file=root_dir;
  fprintf(outfile,"%s%ld\n%d\n%ld\n",DB_IDENSTR,(long) db_top,
          (int) root_dir->flags,(long) root_dir->owner);
  write_filesystem(outfile,root_dir,"");
  fprintf(outfile,".END\n");
  write_table(outfile);
  fprintf(outfile,".END\n");
  while (count<db_top) {
    curr_obj=&(curr_block->block[index]);
    if (curr_obj->input_func) fprintf(outfile,"*%ld\n%s",
                                      (long) strlen(curr_obj->input_func),
                                      curr_obj->input_func);
    fprintf(outfile,"%ld\n",(long) curr_obj->flags);
    fprintf(outfile,"%ld\n",(long) db_obj_to_ref(curr_obj->next_child));
    fprintf(outfile,"%ld\n",(long) db_obj_to_ref(curr_obj->location));
    fprintf(outfile,"%ld\n",(long) db_obj_to_ref(curr_obj->contents));
    fprintf(outfile,"%ld\n",(long) db_obj_to_ref(curr_obj->next_object));
    fprintf(outfile,"%ld\n",(long) db_obj_to_ref(curr_obj->attacher));
    if (curr_obj->obj_state==IN_DB) {
      place=ftell(outfile);
      fseek(infile,curr_obj->file_offset,SEEK_SET);
      readdata(infile,curr_obj);
      writedata(outfile,curr_obj);
      freedata(curr_obj);
      curr_obj->file_offset=place;
    } else
      if (curr_obj->obj_state==IN_CACHE) {
        if (!cachefile) {
#ifdef USE_WINDOWS
          RemoveSavingFromTitle();
#endif /* USE_WINDOWS */
          log_sysmsg("  cache: save_db() couldn't read from cache");
          FATAL_ERROR();
          return 1;
	}
        place=ftell(outfile);
        fseek(cachefile,curr_obj->file_offset,SEEK_SET);
        readdata(cachefile,curr_obj);
        writedata(outfile,curr_obj);
        freedata(curr_obj);
        curr_obj->file_offset=place;
        curr_obj->obj_state=IN_DB;
      } else {
        place=ftell(outfile);
        writedata(outfile,curr_obj);
        curr_obj->file_offset=place;
        curr_obj->obj_state=FROM_DB;
      }
    writeattachees(outfile,curr_obj->attachees);
    fprintf(outfile,".END\n");
    writeverb(outfile,curr_obj->verb_list);
    fprintf(outfile,".END\n");
    count++;
    index++;
    if (index==OBJ_ALLOC_BLKSIZ) {
      index=0;
      curr_block=curr_block->next;
    }
  }
  if (cachefile) fclose(cachefile);
  remove(transact_log_name);
  cache_top=0;
  curr_proto=ref_to_obj(0)->parent;
  while (curr_proto) {
    fprintf(outfile,"%s\n%ld\n%ld\n",curr_proto->pathname,
            (long) curr_proto->proto_obj->refno,
            (long) curr_proto->funcs->num_globals);
    curr_var=curr_proto->funcs->gst;
    while (curr_var) {
      fprintf(outfile,"*%s\n%d\n",curr_var->name,(int) curr_var->base);
      curr_array=curr_var->array;
      while (curr_array) {
        fprintf(outfile,"%d\n",(int) curr_array->size);
        curr_array=curr_array->next;
      }
      fprintf(outfile,"\n");
      curr_var=curr_var->next;
    }
    curr_fns=curr_proto->funcs->func_list;
    while (curr_fns) {
      fprintf(outfile,"%d\n%ld\n%ld\n%ld\n%ld\n%s",
              (int) curr_fns->is_static,
              (long) curr_fns->num_args,
              (long) curr_fns->num_locals,
              (long) curr_fns->num_instr,
              (long) strlen(curr_fns->funcname),
              curr_fns->funcname);
      loop=0;
      while (loop<curr_fns->num_instr) {
        writeinstr(outfile,&(curr_fns->code[loop]));
        loop++;
      }
      curr_fns=curr_fns->next;
    }
    fprintf(outfile,".END\n");
    curr_proto=curr_proto->next_proto;
  }
  fprintf(outfile,".END\n");
  curr_cmd=cmd_head;
  while (curr_cmd) {
    fprintf(outfile,"%ld\n%ld\n%s",(long) curr_cmd->obj->refno,
            (long) strlen(curr_cmd->cmd),
            curr_cmd->cmd);
    curr_cmd=curr_cmd->next;
  }
  fprintf(outfile,".END\n");
  curr_alarm=alarm_list;
  while (curr_alarm) {
    fprintf(outfile,"%ld\n%ld\n%ld\n%s",(long) curr_alarm->obj->refno,
            (long) curr_alarm->delay,
            (long) strlen(curr_alarm->funcname),
            curr_alarm->funcname);
    curr_alarm=curr_alarm->next;
  }
  fprintf(outfile,".END\n");
  fprintf(outfile,"db.END\n");
  fclose(outfile);
  if (infile) fclose(infile);
  if (filename)
    if (strcmp(save_name,filename)) {
      FREE(save_name);
      save_name=copy_string(filename);
    }
  remove(save_name);
  if (rename(tmpdb_name,save_name))
    if (manual_move(tmpdb_name,save_name)) {
#ifdef USE_WINDOWS
      RemoveSavingFromTitle();
#endif /* USE_WINDOWS */
      log_sysmsg(" cache: save_db() couldn't move temporary database to "
                 "save database");
      FATAL_ERROR();
      return 1;
    }
  access_load_file=0;
#ifdef USE_WINDOWS
  RemoveSavingFromTitle();
#endif /* USE_WINDOWS */
  return 0;
}

int init_db() {
  FILE *infile;
  char *buf;
  char *string,*string2;
  signed long flags,uid,count,num_blocks,loop1,index;
  struct obj_blk *curr_block,*object_block;
  unsigned int num_globals;
  struct verb *verbptr;
  unsigned char is_xverb;
  struct proto *curr_proto;
  struct object *curr_obj;
  struct fns *curr_fns;
  char c;
  struct array_size *new_array,*curr_array;
  struct var_tab *new_var,*curr_var;
  struct attach_list *attachptr;
  int len;

  access_load_file=1;
  infile=fopen(load_name,FREAD_MODE);
  if (!infile) {
    log_sysmsg("  cache: init_db() couldn't read from database");
    return 1;
  }
  buf=MALLOC(MAX_STR_LEN);
  fgets(buf,MAX_STR_LEN,infile);
  if (strcmp(buf,DB_IDENSTR)) {
    FREE(buf);
    buf=MALLOC(strlen(load_name)+28);
    sprintf(buf," system: %s not a CI database",load_name);
    log_sysmsg(buf);
    FREE(buf);
    fclose(infile);
    return 1;
  }
  fgets(buf,MAX_STR_LEN,infile);
  db_top=atol(buf);
  if (db_top<=0) {
    FREE(buf);
    buf=MALLOC(strlen(load_name)+43);
    sprintf(buf," system: %s corrupt (while reading db_top)",load_name);
    log_sysmsg(buf);
    FREE(buf);
    fclose(infile);
    return 1;
  }
  num_blocks=db_top/OBJ_ALLOC_BLKSIZ+1;
  objects_allocd=num_blocks*OBJ_ALLOC_BLKSIZ;
  obj_list=NULL;
  free_obj_list=NULL;
  loop1=0;
  while (loop1<num_blocks) {
    object_block=MALLOC(sizeof(struct obj_blk));
    object_block->block=MALLOC(sizeof(struct object)*OBJ_ALLOC_BLKSIZ);
    object_block->next=obj_list;
    obj_list=object_block;
    loop1++;
  }
  count=0;
  fgets(buf,MAX_STR_LEN,infile);
  root_dir->flags=atoi(buf);
  fgets(buf,MAX_STR_LEN,infile);
  root_dir->owner=atol(buf);
  fgets(buf,MAX_STR_LEN,infile);
  while (strcmp(buf,".END\n")) {
    if (feof(infile)) {
      FREE(buf);
      buf=MALLOC(strlen(load_name)+47);
      sprintf(buf," system: %s corrupt (while reading filesystem)",load_name);
      log_sysmsg(buf);
      FREE(buf);
      fclose(infile);
      return 1;
    }
    string=copy_string(buf);
    string[strlen(string)-1]='\0';
    fgets(buf,MAX_STR_LEN,infile);
    flags=atoi(buf);
    fgets(buf,MAX_STR_LEN,infile);
    uid=atol(buf);
    db_add_entry(string,uid,flags);
    FREE(string);
    fgets(buf,MAX_STR_LEN,infile);
  }
  fgets(buf,MAX_STR_LEN,infile);
  while (strcmp(buf,".END\n")) {
    if (feof(infile)) {
      FREE(buf);
      buf=MALLOC(strlen(load_name)+42);
      sprintf(buf," system: %s corrupt (while reading table)",load_name);
      log_sysmsg(buf);
      FREE(buf);
      fclose(infile);
      return 1;
    }
    len=atoi(buf);
    count=0;
    string=MALLOC(len+1);
    while (count<len) string[count++]=fgetc(infile);
    string[count]='\0';
    fgets(buf,MAX_STR_LEN,infile);
    len=atoi(buf);
    count=0;
    string2=MALLOC(len+1);
    while (count<len) string2[count++]=fgetc(infile);
    string2[count]='\0';
    add_to_table(string,string2);
    FREE(string);
    FREE(string2);
    fgets(buf,MAX_STR_LEN,infile);
  }
  count=0;
  curr_block=obj_list;
  index=0;
  while (count<db_top) {
    if (feof(infile)) {
      FREE(buf);
      buf=MALLOC(strlen(load_name)+48+ITOA_BUFSIZ);
      sprintf(buf," system: %s corrupt (while reading object #%ld)",load_name,
              (long) count);
      log_sysmsg(buf);
      FREE(buf);
      fclose(infile);
      return 1;
    }
    curr_block->block[index].refno=count;
    curr_block->block[index].devnum=-1;
    c=fgetc(infile);
    if (c=='*') {
      curr_block->block[index].input_func=readstring(infile);
    } else {
      curr_block->block[index].input_func=NULL;
      if (c!=EOF) {
        ungetc(c,infile);
      }
    }
    fgets(buf,MAX_STR_LEN,infile);
    flags=atoi(buf);
    flags&=~(IN_EDITOR | RESIDENT);
    curr_block->block[index].flags=flags;
    fgets(buf,MAX_STR_LEN,infile);
    curr_block->block[index].next_child=db_ref_to_obj(atol(buf));
    fgets(buf,MAX_STR_LEN,infile);
    curr_block->block[index].location=db_ref_to_obj(atol(buf));
    fgets(buf,MAX_STR_LEN,infile);
    curr_block->block[index].contents=db_ref_to_obj(atol(buf));
    fgets(buf,MAX_STR_LEN,infile);
    curr_block->block[index].next_object=db_ref_to_obj(atol(buf));
    fgets(buf,MAX_STR_LEN,infile);
    curr_block->block[index].attacher=db_ref_to_obj(atol(buf));
    curr_block->block[index].file_offset=ftell(infile);
    if (flags & GARBAGE)
      curr_block->block[index].obj_state=DIRTY;
    else
      curr_block->block[index].obj_state=IN_DB;
    curr_block->block[index].globals=NULL;
    curr_block->block[index].refd_by=NULL;
    curr_block->block[index].verb_list=NULL;
    curr_block->block[index].attachees=NULL;
    if (flags & GARBAGE) {
      curr_block->block[index].next_object=free_obj_list;
      free_obj_list=&(curr_block->block[index]);
    }
    pass_data(infile);
    fgets(buf,MAX_STR_LEN,infile);
    while (strcmp(buf,".END\n") && !(feof(infile))) {
      attachptr=MALLOC(sizeof(struct attach_list));
      attachptr->attachee=db_ref_to_obj(atol(buf));
      attachptr->next=curr_block->block[index].attachees;
      curr_block->block[index].attachees=attachptr;
      fgets(buf,MAX_STR_LEN,infile);
    }
    fgets(buf,MAX_STR_LEN,infile);
    while (strcmp(buf,".END\n") && !(feof(infile))) {
      is_xverb=atoi(buf);
      string=readstring(infile);
      string2=readstring(infile);
      verbptr=MALLOC(sizeof(struct verb));
      verbptr->verb_name=string;
      verbptr->is_xverb=is_xverb;
      verbptr->function=string2;
      verbptr->next=curr_block->block[index].verb_list;
      curr_block->block[index].verb_list=verbptr;
      fgets(buf,MAX_STR_LEN,infile);
    }
    count++;
    index++;
    if (index==OBJ_ALLOC_BLKSIZ) {
      index=0;
      curr_block=curr_block->next;
    }
  }
  fgets(buf,MAX_STR_LEN,infile);
  while (strcmp(buf,".END\n")) {
    if (feof(infile)) {
      FREE(buf);
      buf=MALLOC(strlen(load_name)+47);
      sprintf(buf," system: %s corrupt (while reading prototypes)",load_name);
      log_sysmsg(buf);
      FREE(buf);
      fclose(infile);
      return 1;
    }
    curr_proto=MALLOC(sizeof(struct proto));
    buf[strlen(buf)-1]='\0';
    curr_proto->pathname=copy_string(buf);
    fgets(buf,MAX_STR_LEN,infile);
    curr_proto->proto_obj=db_ref_to_obj(atol(buf));
    curr_obj=curr_proto->proto_obj;
    while (curr_obj) {
      curr_obj->parent=curr_proto;
      curr_obj=curr_obj->next_child;
    }
    curr_obj=db_ref_to_obj(0);
    if (curr_obj->parent!=curr_proto) {
      curr_proto->next_proto=curr_obj->parent->next_proto;
      curr_obj->parent->next_proto=curr_proto;
    } else
      curr_proto->next_proto=NULL;
    curr_proto->funcs=MALLOC(sizeof(struct code));
    fgets(buf,MAX_STR_LEN,infile);
    num_globals=atoi(buf);
    curr_proto->funcs->num_globals=num_globals;
    curr_proto->funcs->func_list=NULL;
    curr_proto->funcs->gst=NULL;
    c=fgetc(infile);
    curr_var=NULL;
    while (c=='*' && !feof(infile)) {
      new_var=MALLOC(sizeof(struct var_tab));
      fgets(buf,MAX_STR_LEN,infile);
      buf[strlen(buf)-1]='\0';
      new_var->name=copy_string(buf);
      fgets(buf,MAX_STR_LEN,infile);
      new_var->base=atoi(buf);
      fgets(buf,MAX_STR_LEN,infile);
      new_var->array=NULL;
      new_var->next=NULL;
      curr_array=NULL;
      while (strcmp(buf,"\n") && !feof(infile)) {
        new_array=MALLOC(sizeof(struct array_size));
        new_array->size=atoi(buf);
        if (curr_array)
          curr_array->next=new_array;
        else
          new_var->array=new_array;
        new_array->next=NULL;
        curr_array=new_array;
        fgets(buf,MAX_STR_LEN,infile);
      }
      if (curr_var)
        curr_var->next=new_var;
      else
        curr_proto->funcs->gst=new_var;
      curr_var=new_var;
      c=fgetc(infile);
    }
    ungetc(c,infile);
    fgets(buf,MAX_STR_LEN,infile);
    while (strcmp(buf,".END\n")) {
      if (feof(infile)) {
        FREE(buf);
        buf=MALLOC(strlen(load_name)+46);
        sprintf(buf," system: %s corrupt (while reading functions)",load_name);
        log_sysmsg(buf);
        FREE(buf);
        fclose(infile);
        return 1;
      }
      curr_fns=MALLOC(sizeof(struct fns));
      curr_fns->is_static=atoi(buf);
      fgets(buf,MAX_STR_LEN,infile);
      curr_fns->num_args=atoi(buf);
      fgets(buf,MAX_STR_LEN,infile);
      curr_fns->num_locals=atoi(buf);
      fgets(buf,MAX_STR_LEN,infile);
      curr_fns->num_instr=atol(buf);
      curr_fns->funcname=readstring(infile);
      curr_fns->next=curr_proto->funcs->func_list;
      curr_proto->funcs->func_list=curr_fns;
      if (curr_fns->num_instr)
        curr_fns->code=MALLOC(sizeof(struct var)*(curr_fns->num_instr));
      else
        curr_fns->code=NULL;
      loop1=0;
      while (loop1<curr_fns->num_instr) {
        readinstr(infile,&(curr_fns->code[loop1]));
        loop1++;
      }
      fgets(buf,MAX_STR_LEN,infile);
    }
    fgets(buf,MAX_STR_LEN,infile);
  }
  fgets(buf,MAX_STR_LEN,infile);
  while (strcmp(buf,".END\n")) {
    if (feof(infile)) {
      FREE(buf);
      buf=MALLOC(strlen(load_name)+45);
      sprintf(buf," system: %s corrupt (while reading commands)",load_name);
      log_sysmsg(buf);
      FREE(buf);
      fclose(infile);
      return 1;
    }
    curr_obj=db_ref_to_obj(atol(buf));
    string=readstring(infile);
    queue_command(curr_obj,string);
    FREE(string);
    fgets(buf,MAX_STR_LEN,infile);
  }
  fgets(buf,MAX_STR_LEN,infile);
  while (strcmp(buf,".END\n")) {
    if (feof(infile)) {
      FREE(buf);
      buf=MALLOC(strlen(load_name)+43);
      sprintf(buf," system: %s corrupt (while reading alarms)",load_name);
      log_sysmsg(buf);
      FREE(buf);
      fclose(infile);
      return 1;
    }
    curr_obj=db_ref_to_obj(atol(buf));
    fgets(buf,MAX_STR_LEN,infile);
    count=atol(buf);
    string=readstring(infile);
    db_queue_for_alarm(curr_obj,count,string);
    FREE(string);
    fgets(buf,MAX_STR_LEN,infile);
  }
  fgets(buf,MAX_STR_LEN,infile);
  if (feof(infile) || strcmp(buf,"db.END\n")) {
    FREE(buf);
    buf=MALLOC(strlen(load_name)+45);
    sprintf(buf," system: %s corrupt (no db.END magic cookie)",load_name);
    log_sysmsg(buf);
    FREE(buf);
    fclose(infile);
    return 1;
  }
  FREE(buf);
  fclose(infile);
  return 0;
}

int create_db() {
  struct code *the_code;
  unsigned int result;
  struct object *obj;
  struct proto *proto_obj;
  long loop;
  struct fns *init_func;
  struct var_stack *rts;
  struct var tmp;

  access_load_file=-1;
  db_add_entry("/boot.c",0,0);
  result=parse_code("/boot",NULL,&the_code);
  if (result==((unsigned int) -1)) return 1;
  if (result) {
    compile_error(NULL,"/boot",result);
    return 1;
  }
  obj=newobj();
  proto_obj=MALLOC(sizeof(struct proto));
  proto_obj->pathname=copy_string("/boot");
  proto_obj->funcs=the_code;
  proto_obj->proto_obj=obj;
  proto_obj->next_proto=NULL;
  obj->refno=0;
  obj->devnum=-1;
  obj->input_func=NULL;
  obj->flags=PROTOTYPE | PRIV;
  obj->parent=proto_obj;
  obj->next_child=NULL;
  obj->location=NULL;
  obj->contents=NULL;
  obj->next_object=NULL;
  if (the_code->num_globals) {
    obj->globals=MALLOC(sizeof(struct var)*(the_code->num_globals));
    loop=0;
    while (loop<the_code->num_globals) {
      obj->globals[loop].type=INTEGER;
      obj->globals[loop].value.integer=0;
      loop++;
    }
  } else
    obj->globals=NULL;
  obj->refd_by=NULL;
  obj->verb_list=NULL;
  add_loaded(obj);
  obj->obj_state=DIRTY;

#ifdef CYCLE_HARD_MAX
  hard_cycles=0;
#endif /* CYCLE_HARD_MAX */

#ifdef CYCLE_SOFT_MAX
  soft_cycles=0;
#endif /* CYCLE_SOFT_MAX */

  init_func=find_function("init",obj,&obj);
  if (init_func) {
    rts=NULL;
    tmp.type=NUM_ARGS;
    tmp.value.num=0;
    push(&tmp,&rts);
    interp(NULL,obj,NULL,&rts,init_func);
    free_stack(&rts);
  }
  handle_destruct();
  return 0;
}
