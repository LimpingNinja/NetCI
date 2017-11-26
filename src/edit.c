/* edit.c */

#include "config.h"
#include "object.h"
#include "intrface.h"
#include "constrct.h"
#include "globals.h"
#include "file.h"

struct edit_s *find_buf(struct object *obj) {
  struct edit_s *curr;

  curr=edit_list;
  while (curr) {
    if (curr->obj==obj) return curr;
    curr=curr->next;
  }
  return NULL;
}

void delete_line(struct edit_s *buf, unsigned long line) {
  struct edit_buf *curr;
  unsigned int num_bufs,index,count;
  char **old_buffer;

  count=buf->num_lines-line;
  --(buf->num_lines);
  num_bufs=(line-1)/NUM_ELINES;
  index=(line-1)%NUM_ELINES;
  curr=buf->buf;
  while (num_bufs--) curr=curr->next;
  if (curr->buf[index]) FREE(curr->buf[index]);
  while (count--) {
    old_buffer=&curr->buf[index];
    if (index==NUM_ELINES-1) {
      curr->buf[index]=curr->next->buf[0];
      index=0;
      curr=curr->next;
    } else
      curr->buf[index]=curr->buf[++index];
    (*old_buffer)=curr->buf[index];
  }
}

void insert_line(struct edit_s *buf, unsigned long line, char *new_line) {
  struct edit_buf *curr;
  unsigned long index,count,num_bufs;
  char *tmp,*tmp2;

  count=buf->num_lines-line;
  ++(buf->num_lines);
  curr=buf->buf;
  if (!curr) {
    buf->buf=MALLOC(sizeof(struct edit_buf));
    buf->buf->next=NULL;
    curr=buf->buf;
  }
  index=(line)%NUM_ELINES;
  num_bufs=(line)/NUM_ELINES;
  while (num_bufs--) {
    if (!(curr->next)) {
      curr->next=MALLOC(sizeof(struct edit_buf));
      curr->next->next=NULL;
    }
    curr=curr->next;
  }
  tmp=curr->buf[index];
  curr->buf[index]=copy_string(new_line);
  while (count--) {
    if (index==NUM_ELINES-1) {
      if (!(curr->next)) {
        curr->next=MALLOC(sizeof(struct edit_buf));
        curr->next->next=NULL;
      }
      tmp2=curr->next->buf[0];
      curr->next->buf[0]=tmp;
      tmp=tmp2;
      index=0;
      curr=curr->next;
    } else {
      tmp2=curr->buf[++index];
      curr->buf[index]=tmp;
      tmp=tmp2;
    }
  }
}

void empty_buf(struct edit_s *buf) {
  struct edit_buf *curr,*next;
  unsigned long count;

  count=buf->num_lines;
  curr=buf->buf;
  if (buf->path) {
    FREE(buf->path);
    buf->path=NULL;
  }
  while (count) delete_line(buf,count--);
  curr=buf->buf;
  while (curr) {
    next=curr->next;
    FREE(curr);
    curr=next;
  }
  buf->buf=NULL;
  buf->num_lines=0;
  buf->is_changed=0;
  buf->curr_line=0;
}

void read_into_buf(struct edit_s *buf, char *filename) {
  FILE *f;
  unsigned long count,index;
  struct edit_buf *curr;
  char sbuf[MAX_STR_LEN];
  int len;

  if (!(f=open_file(filename,FREAD_MODE,buf->obj))) {
    send_device(buf->obj,"couldn't read ");
    send_device(buf->obj,filename);
    send_device(buf->obj,"\n");
    return;
  }
  count=0;
  empty_buf(buf);
  buf->buf=MALLOC(sizeof(struct edit_buf));
  curr=buf->buf;
  curr->next=NULL;
  index=0;
  while (fgets(sbuf,MAX_STR_LEN,f)) {
    count++;
    len=strlen(sbuf);
    if (len)
      if (sbuf[len-1]=='\n')
        sbuf[len-1]='\0';
    curr->buf[index]=copy_string(sbuf);
    index++;
    if (index==NUM_ELINES) {
      index=0;
      curr->next=MALLOC(sizeof(struct edit_buf));
      curr->next->next=NULL;
      curr=curr->next;
    }
  }
  buf->num_lines=count;
  buf->path=copy_string(filename);
  buf->curr_line=0;
  buf->is_changed=0;
  buf->inserting=0;
  close_file(f);
  send_device(buf->obj,"read ");
  send_device(buf->obj,filename);
  send_device(buf->obj,"\n");
}    

void write_to_buf(struct edit_s *buf, char *filename) {
  FILE *f;
  unsigned long count,index;
  struct edit_buf *curr;

  if (!(f=open_file(filename,FWRITE_MODE,buf->obj))) {
    send_device(buf->obj,"couldn't write ");
    send_device(buf->obj,filename);
    send_device(buf->obj,"\n");
    return;
  }
  count=0;
  index=0;
  curr=buf->buf;
  while (count++<buf->num_lines) {
    fputs(curr->buf[index],f);
    fputc('\n',f);
    index++;
    if (index==NUM_ELINES) {
      index=0;
      curr=curr->next;
    }
  }
  buf->is_changed=0;
  close_file(f);
  send_device(buf->obj,"wrote ");
  send_device(buf->obj,filename);
  send_device(buf->obj,"\n");
}

void add_to_edit(struct object *obj, char *file) {
  struct edit_s *curr;

  if (obj->flags & IN_EDITOR) return;
  if (!(obj->flags & CONNECTED)) return;
  obj->flags|=IN_EDITOR;
  curr=MALLOC(sizeof(struct edit_s));
  curr->obj=obj;
  curr->num_lines=0;
  curr->buf=NULL;
  curr->path=NULL;
  curr->curr_line=0;
  curr->inserting=0;
  curr->is_changed=0;
  curr->next=edit_list;
  edit_list=curr;
  send_device(obj,"entering editor\n");
  if (file) read_into_buf(curr,file);
}

void remove_from_edit(struct object *obj) {
  struct edit_s *prev,*curr;

  if (!(obj->flags & IN_EDITOR)) return;
  obj->flags&=~IN_EDITOR;
  curr=edit_list;
  prev=NULL;
  while (curr) {
    if (curr->obj==obj) {
      if (prev)
        prev->next=curr->next;
      else
        edit_list=curr->next;
      break;
    }
    curr=curr->next;
  }
  if (!curr) return;
  empty_buf(curr);
  FREE(curr);
}

void split_arg(struct edit_s *buf, unsigned long *arg1, unsigned long *arg2,
               char *arg) {
  int count,len,has_hyphen;
  char *a1,*a2;
  
  count=0;
  a1=arg;
  a2="";
  has_hyphen=0;
  len=strlen(arg);
  while (count<len) {
    if (arg[count]=='-') {
      arg[count]='\0';
      has_hyphen=1;
      a2=&(arg[count+1]);
      break;
    }
    count++;
  }
  if (has_hyphen) {
    if (*a1)
      *arg1=atol(a1);
    else
      *arg1=1;
    if (*a2)
      *arg2=atol(a2);
    else
      *arg2=buf->num_lines;
  } else
    if (*arg) {
      *arg1=atol(arg);
      *arg2=*arg1;
    } else {
      *arg1=buf->curr_line;
      *arg2=*arg1;
    }
}

void do_edit_command(struct object *obj, char *s) {
  char *cmd,*arg;
  long line1,line2;
  int loop,len,index,num_bufs,count;
  struct edit_s *buf;
  struct edit_buf *curr;
  char sbuf[ITOA_BUFSIZ+3];

  buf=find_buf(obj);
  if (!buf) return;
  if (buf->curr_line>buf->num_lines) buf->curr_line=buf->num_lines;
  if (buf->inserting) {
    if (!strcmp(s,".")) {
      buf->inserting=0;
      send_device(buf->obj,"exiting insertion mode\n");
  } else {
      insert_line(buf,buf->curr_line,s);
      ++(buf->curr_line);
    }
    return;
  }
  loop=0;
  len=strlen(s);
  cmd=s;
  arg="";
  while (loop<len) {
    if (s[loop]==' ') {
      s[loop]='\0';
      arg=&(s[loop+1]);
      break;
    }
    loop++;
  }
  if (!strcmp(cmd,"l") || !strcmp(cmd,"list")) {
    split_arg(buf,&line1,&line2,arg);
    if (!line1) return;
    if (line1<1) line1=1;
    if (line2>buf->num_lines) line2=buf->num_lines;
    count=line2-line1+1;
    if (count<1) return;
    index=(line1-1)%NUM_ELINES;
    num_bufs=(line1-1)/NUM_ELINES;
    curr=buf->buf;
    while (num_bufs--) curr=curr->next;
    while (count--) {
      sprintf(sbuf,"%ld: ",(long) line1++);
      send_device(buf->obj,sbuf);
      send_device(buf->obj,curr->buf[index]);
      send_device(buf->obj,"\n");
      index++;
      if (index==NUM_ELINES) {
        index=0;
        curr=curr->next;
      }
    }
    return;
  }
  if (!strcmp(cmd,"q") || !strcmp(cmd,"quit")) {
    if (buf->is_changed) {
      send_device(buf->obj,"file has been modified\n");
      return;
    }
    send_device(buf->obj,"exiting editor\n");
    remove_from_edit(buf->obj);
    return;
  }
  if (!strcmp(cmd,"i") || !strcmp(cmd,"insert")) {
    if (*arg) {
      line1=atol(arg);
      if (line1<0 || line1>buf->num_lines) {
        send_device(buf->obj,"line number out of range\n");
        return;
      }
      buf->curr_line=line1;
    }
    buf->is_changed=1;
    buf->inserting=1;
    send_device(buf->obj,"entering insertion mode\n");
    return;
  }
  if (!strcmp(cmd,"r") || !strcmp(cmd,"read")) {
    if (!arg) {
      send_device(buf->obj,"no filename specified\n");
      return;
    }
    empty_buf(buf);
    read_into_buf(buf,arg);
    return;
  }
  if (!strcmp(cmd,"w") || !strcmp(cmd,"write")) {
    if (!*arg && !buf->path) {
      send_device(buf->obj,"no filename specified\n");
      return;
    }
    if (*arg)
      buf->path=copy_string(arg);
    write_to_buf(buf,buf->path);
    return;
  }
  if (!strcmp(cmd,"x") || !strcmp(cmd,"exit")) {
    empty_buf(buf);
    send_device(buf->obj,"exiting editor\n");
    remove_from_edit(buf->obj);
    return;
  }
  if (!strcmp(cmd,"d") || !strcmp(cmd,"delete")) {
    split_arg(buf,&line1,&line2,arg);
    if (line1>buf->num_lines || line2>buf->num_lines || line1<=0) return;
    count=line2-line1+1;
    if (count<1) return;
    while (count--) delete_line(buf,line1);
    send_device(buf->obj,"deleted\n");
    return;
  }
  if (!strcmp(cmd,"?") || !strcmp(cmd,"h") || !strcmp(cmd,"help")) {
    send_device(buf->obj,"list <#>-<#>             lists file. line #'s "
                         "optional\n");
    send_device(buf->obj,"quit                     quit editor unless file "
                         "not saved\n");
    send_device(buf->obj,"insert <#>               insert line at directly "
                         "after specified #\n");
    send_device(buf->obj,"delete <#>-<#>           deletes lines. #'s "
                         "optional\n");
    send_device(buf->obj,"read filename            reads a file\n");
    send_device(buf->obj,"write <filename>         writes a file\n");
    send_device(buf->obj,"exit                     quit editor\n");
    send_device(buf->obj,"help                     this help screen\n");
    send_device(buf->obj,"status                   editor status\n");
    return;
  }
  if (!strcmp(cmd,"s") || !strcmp(cmd,"status")) {
    send_device(buf->obj,"file being edited: ");
    if (buf->path)
      send_device(buf->obj,buf->path);
    else
      send_device(buf->obj,"<unknown>");
    send_device(buf->obj,"\nnumber of lines: ");
    sprintf(sbuf,"%ld",(long) buf->num_lines);
    send_device(buf->obj,sbuf);
    send_device(buf->obj,"\ncurrent line: ");
    sprintf(sbuf,"%ld",(long) buf->curr_line);
    send_device(buf->obj,sbuf);
    if (buf->is_changed)
      send_device(buf->obj,"\nfile has been modified\n");
    else
      send_device(buf->obj,"\nfile has not been modified\n");
    return;
  }
  send_device(buf->obj,"syntax error. type 'help' for help\n");
}
