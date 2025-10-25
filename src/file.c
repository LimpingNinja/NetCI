
/* file.c */

/* file permissions etc. are maintained in data structures & code here */

#include <sys/types.h>
#include <sys/stat.h>

#ifdef USE_WINDOWS
#include <direct.h>
#else
/* POSIX and LINUX should use UNISTD.H */
#include <unistd.h>
#endif /* USE_WINDOWS */

#include "config.h"
#include "object.h"
#include "file.h"
#include "globals.h"
#include "interp.h"
#include "constrct.h"
#ifdef USE_WINDOWS
#include "intrface.h"
#include "winmain.h"
#endif /* USE_WINDOWS */

int SYSTEM_mkdir(char *filename) {
#ifdef USE_WINDOWS
  if (mkdir(filename))
#else /* USE_WINDOWS */
  if (mkdir(filename,493))
#endif /* USE_WINDOWS */
    return 1;
  else
    return 0;
}

int SYSTEM_rmdir(char *filename) {
  if (rmdir(filename))
    return 1;
  else
    return 0;
}

char *make_path(struct file_entry *entry) {
  char *buf,*buf2;
  struct file_entry *curr;
  unsigned int len;

  len=strlen(fs_path);
  curr=entry;
  while (curr!=root_dir) {
    len=len+strlen(curr->filename)+1;
    curr=curr->parent;
  }
  len++;
  buf=MALLOC(len);
  buf2=MALLOC(len);
  *buf='\0';
  *buf2='\0';
  curr=entry;
  while (curr!=root_dir) {
    strcpy(buf2,buf);
#ifdef USE_WINDOWS
    strcpy(buf,"\\");
#else /* USE_WINDOWS */
    strcpy(buf,"/");
#endif /* USE_WINDOWS */
    strcat(buf,curr->filename);
    strcat(buf,buf2);
    curr=curr->parent;
  }
  strcpy(buf2,buf);
  strcpy(buf,fs_path);
  strcat(buf,buf2);
  FREE(buf2);
  return buf;
}

struct file_entry *find_entry(char *filename) {
  struct file_entry *curr;
  char *currname,*currptr;

  if (*filename!='/')
    return NULL;
  if (filename[1]=='\0')
    return root_dir;
  curr=root_dir;
  currname=MALLOC(strlen(filename)+1);
  while (*filename=='/') {
    currptr=currname;
    filename++;
    while (*filename!='/' && *filename!='\0')
      *(currptr++)=*(filename++);
    *currptr='\0';
    curr=curr->contents;
    while (curr) {
      if (!strcmp(curr->filename,currname))
        break;
      curr=curr->next_file;
    }
    if (!curr) {
      FREE(currname);
      return NULL;
    }
  }
  FREE(currname);
  return curr;
}

int can_read(struct file_entry *fe, struct object *uid) {
  if (!uid) return 1;
  if (!fe) return 1;
  if (uid->flags & PRIV) return 1;
  if (fe==root_dir) return 1;
  while (fe!=root_dir) {
    if (uid->refno!=fe->owner && !(fe->flags & READ_OK))
      return 0;
    fe=fe->parent;
  }
  return 1;
}

struct file_entry *split_dir(char *filename, char **f) {
  char *buf1,*buf2;
  struct file_entry *fe;
  int count,maxcount,x;

  if (*filename!='/') return NULL;
  count=0;
  maxcount=0;
  while (filename[count]) {
    if (filename[count]=='/') maxcount=count;
    count++;
  }
  buf1=MALLOC(maxcount+2);
  buf2=MALLOC(count-maxcount);
  x=0;
  while (x<maxcount) {
    buf1[x]=filename[x];
    x++;
  }
  buf1[x]='\0';
  while (x<count) {
    x++;
    buf2[x-maxcount-1]=filename[x];
  }
  if (!maxcount) {
    buf1[0]='/';
    buf1[1]='\0';
  }
  fe=find_entry(buf1);
  FREE(buf1);
  if (!fe) {
    FREE(buf2);
    return NULL;
  }
  if (f)
    *f=buf2;
  else
    FREE(buf2);
  return fe;
}

struct file_entry *make_entry(struct file_entry *dir, char *name,
                              struct object *uid) {
  struct file_entry *fe,*curr,*prev;
  int x;

  if (!is_legal(name)) return NULL;
  curr=dir->contents;
  prev=NULL;
  while (curr) {
    x=strcmp(name,curr->filename);
    if (!x) return curr;
    if (x<0) {
      fe=MALLOC(sizeof(struct file_entry));
      fe->filename=copy_string(name);
      fe->flags=0;
      if (uid)
        fe->owner=uid->refno;
      else
        fe->owner=0;
      fe->contents=NULL;
      fe->parent=dir;
      fe->prev_file=prev;
      fe->next_file=curr;
      curr->prev_file=fe;
      if (prev)
        prev->next_file=fe;
      else
        dir->contents=fe;
      break;
    }
    prev=curr;
    curr=curr->next_file;
  }
  if (!curr) {
    fe=MALLOC(sizeof(struct file_entry));
    fe->filename=name;
    fe->flags=0;
    if (uid)
      fe->owner=uid->refno;
    else
      fe->owner=0;
    fe->contents=NULL;
    fe->parent=dir;
    fe->prev_file=prev;
    fe->next_file=NULL;
    if (prev)
      prev->next_file=fe;
    else
      dir->contents=fe;
  }
  return fe;
}

int remove_entry(struct file_entry *fe) {
  if (fe==root_dir) return 1;
  if (fe->contents) return 1;
  if (fe->parent->contents==fe)
    fe->parent->contents=fe->next_file;
  if (fe->next_file)
    fe->next_file->prev_file=fe->prev_file;
  if (fe->prev_file)
    fe->prev_file->next_file=fe->next_file;
  FREE(fe->filename);
  FREE(fe);
  return 0;
}

int ls_dir(char *filename, struct object *uid, struct object *player) {
  struct file_entry *fe;
  struct fns *listen_func;
  char *buf;
  struct var_stack *rts;
  struct var tmp;
  struct object *rcv,*tmpobj;

  if (!uid) return 1;
  rcv=player;
  if (!player) rcv=uid;
  fe=find_entry(filename);
  if (!fe) return 1;
  if (!can_read(fe,uid)) return 1;
  if (!(fe->flags & DIRECTORY)) return 1;
  listen_func=find_function("listen",rcv,&tmpobj);
  if (!listen_func) return 0;
  fe=fe->contents;
  doing_ls=1;
  while (fe) {
    buf=MALLOC((2*ITOA_BUFSIZ)+4+strlen(fe->filename));
    sprintf(buf,"%ld %ld %s\n",(long) fe->owner,(long) fe->flags,fe->filename);
    rts=NULL;
    tmp.type=STRING;
    tmp.value.string=buf;
    push(&tmp,&rts);
    FREE(buf);
    tmp.type=NUM_ARGS;
    tmp.value.integer=1;
    push(&tmp,&rts);
    interp(uid,tmpobj,player,&rts,listen_func);
    free_stack(&rts);
    fe=fe->next_file;
  }
  doing_ls=0;
  return 0;
}

int stat_file(char *filename, struct object *uid) {
  struct file_entry *fe,*homefe;
  char *newbuf;

  fe=find_entry(filename);
  if (!fe) return -1;
  homefe=split_dir(filename,&newbuf);
  if (!homefe) return -1;
  FREE(newbuf);
  if (!can_read(homefe,uid)) return -1;
  return fe->flags;
}
int owner_file(char *filename, struct object *uid) {
  struct file_entry *fe,*homefe;
  char *newbuf;

  fe=find_entry(filename);
  if (!fe) return -1;
  homefe=split_dir(filename,&newbuf);
  if (!homefe) return -1;
  FREE(newbuf);
  if (!can_read(homefe,uid)) return -1;
  return fe->owner;
}

FILE *open_file(char *filename, char *mode, struct object *uid) {
  struct file_entry *fe,*homefe;
  char *buf,*newbuf;
  FILE *f;

  fe=find_entry(filename);
  if (!fe) {
    if (*mode=='r') return NULL;
    if (!uid) return NULL;
    homefe=split_dir(filename,&newbuf);
    if (!homefe) return NULL;
    if (!can_read(homefe,uid)) {
      FREE(newbuf);
      return NULL;
    }
    if (!(homefe->owner==uid->refno || (uid->flags & PRIV) ||
          (homefe->flags & WRITE_OK))) {
      FREE(newbuf);
      return NULL;
    }
    fe=make_entry(homefe,newbuf,uid);
    if (!fe) {
      FREE(newbuf);
      return NULL;
    }
  }
  if (fe->flags & DIRECTORY) return NULL;
  if (!can_read(fe->parent,uid)) return NULL;
  buf=make_path(fe);
  if (!uid && *mode=='r') {
    f=fopen(buf,mode);
    FREE(buf);
    return f;
  }
  if (!uid) {
    FREE(buf);
    return NULL;
  }
  if (uid->refno==fe->owner || (uid->flags & PRIV)) {
    f=fopen(buf,mode);
    FREE(buf);
    return f;
  }
  if (*mode=='r' && (fe->flags & READ_OK)) {
    f=fopen(buf,mode);
    FREE(buf);
    return f;
  }
  if ((*mode=='a' || *mode=='w') && (fe->flags & WRITE_OK) && !doing_ls) {
    f=fopen(buf,mode);
    FREE(buf);
    return f;
  }
  FREE(buf);
  return NULL;
}

int remove_file(char *filename, struct object *uid) {
  struct file_entry *fe;
  char *buf;

  if (doing_ls) return 1;
  if (!uid) return 1;
  fe=find_entry(filename);
  if (!fe) return 1;
  if (fe->flags & DIRECTORY) return 1;
  if (!can_read(fe->parent,uid)) return 1;
  if (fe->parent->owner!=uid->refno && !(fe->parent->flags & WRITE_OK)
      && !(uid && (uid->flags & PRIV)))
    return 1;
  buf=make_path(fe);
  if (remove(buf)) {
    FREE(buf);
    return 1;
  }
  remove_entry(fe);
  FREE(buf);
  return 0;
}

int copy_file(char *src, char *dest, struct object *uid) {
  FILE *srcf,*destf;
  int c;

  if (doing_ls) return 1;
  if (!uid) return 1;
  srcf=open_file(src,FREAD_MODE,uid);
  if (!srcf) return 1;
  destf=open_file(dest,FWRITE_MODE,uid);
  if (!destf) {
    close_file(srcf);
    return 1;
  }
  c=fgetc(srcf);
  while (c!=EOF) {
    fputc(c,destf);
    c=fgetc(srcf);
  }
  close_file(srcf);
  close_file(destf);
  return 0;
}

int move_file(char *src, char *dest, struct object *uid) {
  struct file_entry *homefe,*srcf,*destf;
  char *buf;
  FILE *f1,*f2;
  int c;

  if (doing_ls) return 1;
  if (!uid) return 1;
  if (!strcmp(src,dest)) return 1;
  srcf=find_entry(src);
  if (!srcf) return 1;
  if (srcf->flags & DIRECTORY) return 1;
  destf=find_entry(dest);
  if (!destf) {
    homefe=split_dir(dest,&buf);
    if (!homefe) return 1;
    if (!can_read(homefe,uid)) {
      FREE(buf);
      return 1;
    }
    if (!(homefe->owner==uid->refno || (uid->flags & PRIV) ||
          (homefe->flags & WRITE_OK))) {
      FREE(buf);
      return 1;
    }
    destf=make_entry(homefe,buf,uid);
    if (!destf) {
      FREE(buf);
      return 1;
    }
  }
  if (destf->flags & DIRECTORY) return 1;
  if (!can_read(srcf,uid)) return 1;
  if (!(srcf->parent->owner==uid->refno || (uid->flags & PRIV) ||
        (srcf->parent->flags & WRITE_OK)))
    return 1;
  if (!can_read(destf->parent,uid)) return 1;
  if (!(destf->owner==uid->refno || (uid->flags & PRIV) ||
        (destf->flags & WRITE_OK)))
    return 1;
  f1=open_file(src,FREAD_MODE,uid);
  if (!f1) return 1;
  f2=open_file(dest,FWRITE_MODE,uid);
  if (!f2) {
    close_file(f1);
    return 1;
  }
  c=fgetc(f1);
  while (c!=EOF) {
    fputc(c,f2);
    c=fgetc(f1);
  }
  close_file(f1);
  close_file(f2);
  buf=make_path(srcf);
  if (remove(buf)) {
    FREE(buf);
    return 1;
  }
  FREE(buf);
  remove_entry(srcf);
  return 0;
}

int make_dir(char *filename, struct object *uid) {
  struct file_entry *fe,*homefe;
  char *buf;

  if (doing_ls) return 1;
  if (!uid) return 1;
  fe=find_entry(filename);
  if (fe) return 1;
  homefe=split_dir(filename,&buf);
  if (!homefe) return 1;
  if (!(homefe->owner==uid->refno || (uid->flags & PRIV) ||
        (homefe->flags & WRITE_OK))) {
    FREE(buf);
    return 1;
  }
  if (!(fe=make_entry(homefe,buf,uid))) {
    FREE(buf);
    return 1;
  }
  fe->flags=DIRECTORY;
  buf=make_path(fe);
  if (SYSTEM_mkdir(buf)) {
    FREE(buf);
    remove_entry(fe);
    return 1;
  }
  FREE(buf);
  return 0;
}

int remove_dir(char *filename, struct object *uid) {
  struct file_entry *fe;
  char *buf;

  if (doing_ls) return 1;
  if (!uid) return 1;
  fe=find_entry(filename);
  if (!(fe->flags & DIRECTORY)) return 1;
  if (fe->contents) return 1;
  if (fe==root_dir) return 1;
  if (!(fe->parent->owner==uid->refno || (uid->flags & PRIV) ||
        (fe->parent->flags & WRITE_OK)))
    return 1;
  buf=make_path(fe);
  if (SYSTEM_rmdir(buf)) {
    FREE(buf);
    return 1;
  }
  FREE(buf);
  remove_entry(fe);
  return 0;
}

int hide(char *filename) {
  struct file_entry *fe;

  if (doing_ls) return 1;
  fe=find_entry(filename);
  if (!fe) return 1;
  return remove_entry(fe);
}

int unhide(char *filename, struct object *uid, int flags) {
  struct file_entry *fe,*homefe;
  char *buf;

  if (doing_ls) return 1;
  fe=find_entry(filename);
  if (fe) return 1;
  homefe=split_dir(filename,&buf);
  if (!homefe) return 1;
  if (!(fe=make_entry(homefe,buf,uid))) {
    FREE(buf);
    return 1;
  }
  fe->flags=flags;
  return 0;
}

int db_add_entry(char *filename, signed long uid, int flags) {
  struct file_entry *fe,*homefe;
  char *buf;

  fe=find_entry(filename);
  if (fe) return 1;
  homefe=split_dir(filename,&buf);
  if (!homefe) return 1;
  if (!(fe=make_entry(homefe,buf,NULL))) {
    FREE(buf);
    return 1;
  }
  fe->flags=flags;
  fe->owner=uid;
  return 0;
}

int chown_file(char *filename, struct object *uid, struct object *new_owner) {
  struct file_entry *fe;

  fe=find_entry(filename);
  if (!uid) return 1;
  if (!new_owner) return 1;
  if (!fe) return 1;
  if (fe!=root_dir)
    if (!can_read(fe->parent,uid))
      return 1;
  if (!(fe->owner==uid->refno || (uid->flags & PRIV))) return 1;
  fe->owner=new_owner->refno;
  return 0;
}

int chmod_file(char *filename, struct object *uid, int flags) {
  struct file_entry *fe;

  fe=find_entry(filename);
  if (!fe) return 1;
  if (!uid) return 1;
  if (fe!=root_dir)
    if (!can_read(fe->parent,uid))
      return 1;
  if (!(fe->owner==uid->refno || (uid->flags & PRIV))) return 1;
  if (fe==root_dir && (!(flags & READ_OK))) return 1;
  flags&=(WRITE_OK | READ_OK);
  fe->flags&=DIRECTORY;
  fe->flags|=flags;
  return 0;
}

void logger(int level, char *msg) {
  char timebuf[20];
  char levelbuf[10];
  time_t now_time;
  FILE *logfile;
  struct tm *time_s;
  
  /* Filter based on log level */
  /* Higher log_level = more verbose output */
  /* ERROR(0) < WARNING(1) < LOG(2) < DEBUG(3) */
  /* If log_level=LOG(2), show ERROR, WARNING, LOG but not DEBUG */
  if (level > log_level)
    return;

  now_time=time(NULL);
  time_s=localtime(&now_time);
  sprintf(timebuf,"%02d-%02d %02d:%02d",(int) (time_s->tm_mon+1),
          (int) time_s->tm_mday,
          (int) time_s->tm_hour,
          (int) time_s->tm_min);ur)%12) ? ((time_s->tm_hour)%12):12),
          (int) time_s->tm_min,
          ((time_s->tm_hour<12) ? "AM":"PM"));
  
  /* Add level prefix */
  switch(level) {
    case LOG_ERROR:   strcpy(levelbuf, "[ERROR] "); break;
    case LOG_WARNING: strcpy(levelbuf, "[WARN]  "); break;
    case LOG:         strcpy(levelbuf, "[INFO]  "); break;
    case LOG_DEBUG:   strcpy(levelbuf, "[DEBUG] "); break;
    default:          levelbuf[0] = '\0'; break;
  }
  
#ifdef USE_WINDOWS
  AddText(timebuf);
  AddText(" ");
  AddText(levelbuf);
  AddText(msg);
  AddText("\n");
  logfile=fopen(syslog_name,"a");
  if (!logfile) {
    AddText(timebuf);
	AddText("  system: couldn't open system log ");
	AddText(syslog_name);
	AddText("\n");
    return;
  }
#else /* USE_WINDOWS */
  if (noisy)
    fprintf(stderr,"%s %s%s\n",timebuf,levelbuf,msg);
  logfile=fopen(syslog_name,"a");
  if (!logfile) return;
#endif /* USE_WINDOWS */
  fprintf(logfile,"%s %s%s\n",timebuf,levelbuf,msg);
  fclose(logfile);
}
