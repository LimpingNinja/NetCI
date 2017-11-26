/* preprocess.c */

#include "config.h"
#include "object.h"
#include "file.h"
#include "token.h"
#include "constrct.h"
#include "instr.h"

extern char expand_buf[EBUFSIZ+1];
extern char tmpbuf[EBUFSIZ+1];
extern char name_buf[MAX_TOK_LEN+1];
extern char string_buf[MAX_STR_LEN+1];

#define isstart(c) (isalpha(c) || ((c)=='_'))
#define iscname(c) (isstart(c) || isdigit(c))
#define getch() *((file_info->expanded)++)
#define ungetch() --(file_info->expanded)

char *make_include_name(char *name)
{
  static char nbuf[MAX_STR_LEN];
  int c,n;

  c=0;
  n=strlen(name);
  if (n<3) return "";
  if ((*name=='<') && (name[n-1]=='>')) {
    strcpy(nbuf,INCLUDE_PATH);
    c=strlen(INCLUDE_PATH);
    nbuf[c++]='/';
  } else
    if (!((*name=='\"') && (name[n-1]=='\"')))
      return "";
  if (c+n-1>MAX_STR_LEN) return "";
  strncpy(nbuf+c,name+1,n-2);
  nbuf[c+n-2]='\0';
  return nbuf;
}

char *find_parm(struct define *def, char *name)
{
  struct parm *curr;

  curr=def->params;
  while (curr) {
    if (!strcmp(curr->name,name))
      return curr->exp;
    curr=curr->next;
  }
  return NULL;
}

void add_expand_buf(char *s1, char *s2)
{
  int counter;

  counter=0;
  if (s1==expand_buf)
    counter=strlen(s1);
  else
    while ((*s1) && (counter<EBUFSIZ))
      expand_buf[counter++]=*(s1++);
  while ((*s2) && (counter<EBUFSIZ))
    expand_buf[counter++]=*(s2++);
  expand_buf[counter]='\0';
}

void free_parm_exp(struct define *def)
{
  struct parm *curr;

  curr=def->params;
  while (curr) {
    if (curr->exp) {
      FREE(curr->exp);
      curr->exp=NULL;
    }
    curr=curr->next;
  }
}

int preprocess(filptr *file_info)
{
  int counter,c;
  struct define *currdef,*prevdef;
  struct parm *currparm,*nextparm;
  FILE *tmp;
  struct file_stack *tmp2;

  counter=0;
  c=fgetc(file_info->curr_file);
  while ((c!=EOF) && (!isspace(c)) && (counter<MAX_TOK_LEN)) {
    name_buf[counter++]=c;
    c=fgetc(file_info->curr_file);
  }
  if ((c!=EOF) && (!(isspace(c))))
    return 1;
  name_buf[counter]='\0';
  if (!strcmp(name_buf,"undef")) {
    while ((c!=EOF) && isspace(c)) c=fgetc(file_info->curr_file);
    counter=0;
    while ((c!=EOF) && (!isspace(c)) && (counter<MAX_TOK_LEN)) {
      name_buf[counter++]=c;
      c=fgetc(file_info->curr_file);
    }
    name_buf[counter]='\0';
    ungetc(c,file_info->curr_file);
    currdef=file_info->defs;
    prevdef=NULL;
    while (currdef)
      if (!strcmp(currdef->name,name_buf)) {
        if (prevdef)
          prevdef->next=currdef->next;
        else
          file_info->defs=currdef->next;
        FREE(currdef->name);
        FREE(currdef->definition);
        currparm=currdef->params;
        while (currparm) {
          nextparm=currparm->next;
          FREE(currparm->name);
          if (currparm->exp)
            FREE(currparm->exp);
          FREE(currparm);
          currparm=nextparm;
        }
        FREE(currdef);
      } else
        currdef=currdef->next;
    return 0;
  } else if (!strcmp(name_buf,"include")) {
    counter=0;
    while ((c!=EOF) && isspace(c)) c=fgetc(file_info->curr_file);
    counter=0;
    while ((c!=EOF) && (!isspace(c)) && (counter<MAX_STR_LEN)) {
      string_buf[counter++]=c;
      c=fgetc(file_info->curr_file);
    }
    string_buf[counter]='\0';
    ungetc(c,file_info->curr_file);
    if (!(tmp=open_file(make_include_name(string_buf),FREAD_MODE,NULL))) {
      set_c_err_msg("couldn't open include file");
      return 1;
    }
    tmp2=(struct file_stack *) MALLOC(sizeof(struct file_stack));
    tmp2->file_ptr=file_info->curr_file;
    tmp2->previous=file_info->previous;
    file_info->previous=tmp2;
    file_info->curr_file=tmp;
    ungetc('\n',tmp);
    return 0;
  } else if (!strcmp(name_buf,"define")) {
    counter=0;
    while ((c!=EOF) && isspace(c)) c=fgetc(file_info->curr_file);
    counter=0;
    if (!isstart(c)) return 1;
    while ((c!=EOF) && iscname(c) && (counter<MAX_TOK_LEN)) {
      name_buf[counter++]=c;
      c=fgetc(file_info->curr_file);
    }
    name_buf[counter]='\0';
    currdef=(struct define *) MALLOC(sizeof(struct define));
    currdef->name=copy_string(name_buf);
    currdef->has_paren=0;
    currdef->params=NULL;
    currparm=NULL;
    if (c=='(') {
      currdef->has_paren=1;
      c=fgetc(file_info->curr_file);
      while (c!=EOF) {
        while ((c!=EOF) && (c!='\n') && isspace(c))
          c=fgetc(file_info->curr_file);
        counter=0;
        if (c=='\n') {
          currdef->params=currparm;
          currdef->next=file_info->defs;
          file_info->defs=currdef;
          currdef->definition=copy_string("");
          set_c_err_msg("malformed #define");
          return 1;
        }
        if (c==')') {
          c=fgetc(file_info->curr_file);
          break;
        }
        while ((c!=EOF) && (iscname(c)) && (counter<MAX_TOK_LEN)) {
          name_buf[counter++]=c;
          c=fgetc(file_info->curr_file);
        }
        name_buf[counter]='\0';
        nextparm=(struct parm *) MALLOC(sizeof(struct parm));
        nextparm->next=currparm;
        nextparm->name=copy_string(name_buf);
        nextparm->exp=NULL;
        currparm=nextparm;
        while ((c!=EOF) && (c!='\n') && (isspace(c)))
          c=fgetc(file_info->curr_file);
        if ((c!=',') && (c!=')')) {
          currdef->params=currparm;
          currdef->next=file_info->defs;
          currdef->definition=copy_string("");
          file_info->defs=currdef;
          set_c_err_msg("malformed #define");
          return 1;
        }
        if (c==',') c=fgetc(file_info->curr_file);
      }
      c=fgetc(file_info->curr_file);
    } else
      ungetc(c,file_info->curr_file);
    while (currparm) {
      nextparm=currparm->next;
      currparm->next=currdef->params;
      currdef->params=currparm;
      currparm=nextparm;
    }
    currdef->next=file_info->defs;
    file_info->defs=currdef;
    while ((c!=EOF) && (c!='\n') && (isspace(c))) c=fgetc(file_info->curr_file);
    counter=0;
    while ((c!=EOF) && (c!='\n') && (counter<EBUFSIZ)) {
      if (c=='\\') {
        c=fgetc(file_info->curr_file);
        if (c=='\n') {
          if (!(file_info->previous))
            ++(file_info->phys_line);
          c=' ';
        } else {
          ungetc(c,file_info->curr_file);
          c='\\';
        }
      }
      if (c=='/') {
        c=fgetc(file_info->curr_file);
        if (c=='*') {
          while (c!=EOF) {
            c=fgetc(file_info->curr_file);
            if (c=='*') {
              c=fgetc(file_info->curr_file);
              if (c=='/') {
                c=' ';
                break;
              } else {
                ungetc(c,file_info->curr_file);
              }
            } else if (c=='\n')
              if (!(file_info->previous))
                ++(file_info->phys_line);
          }
        } else {
          ungetc(c,file_info->curr_file);
          c='/';
        }
      }
      if ((c!=EOF) && (c!='\n') && (isspace(c))) {
        expand_buf[counter++]=' ';
        while ((c!=EOF) && (c!='\n') && (isspace(c)))
          c=fgetc(file_info->curr_file);
      } else if ((c!=EOF) && (c!='\n')) {
        expand_buf[counter++]=c;
        c=fgetc(file_info->curr_file);
      }
    }
    ungetc(c,file_info->curr_file);
    expand_buf[counter]='\0';
    currdef->definition=copy_string(expand_buf);
    return 0;
  } else
    return 1;
}

void expand_def(struct define *def, char *buf)
{
  char *s,*s2;
  int counter,counter2;

  s=def->definition;
  counter=0;
  while ((*s) && (counter<EBUFSIZ)) {
    if (isstart(*s)) {
      counter2=0;
      while ((counter2<MAX_TOK_LEN) && (iscname(*s)))
        name_buf[counter2++]=*(s++);
      name_buf[counter2]='\0';
      if (!(s2=find_parm(def,name_buf)))
        s2=name_buf;
      while ((*s2) && (counter<EBUFSIZ))
        buf[counter++]=*(s2++);
    } else
      buf[counter++]=*(s++);
  }
  buf[counter]='\0';
}

void expand_exp(struct define *def, filptr *file_info)
{
  char c;
  int paren_counter,in_string,counter;
  struct parm *currparm;

  if (def->has_paren) {
    do {
      c=getch();
    } while (c && isspace(c));
    if (c!='(')
      return;
    paren_counter=0;
    in_string=0;
    currparm=def->params;
    do {
      do {
        c=getch();
      } while (c && isspace(c));
      if (c==')')
        break;
      if (!currparm) {
        free_parm_exp(def);
        return;
      }
      counter=0;
      while (c && (counter<EBUFSIZ)) {
        if (c=='\"')
          in_string=(!in_string);
        else if (c=='(' && (!in_string))
          paren_counter++;
        else if (c==')' && (!in_string))
          if (!paren_counter) {
            ungetch();
            break;
          } else
            paren_counter--;
        else if (c==',' && (!paren_counter) && (!in_string))
          break;
        else if (c=='\\' && in_string) {
          c=getch();
          if (counter<(EBUFSIZ-1))
            tmpbuf[counter++]='\\';
        }
        tmpbuf[counter++]=c;
        c=getch();
      }
      tmpbuf[counter]='\0';
      currparm->exp=copy_string(tmpbuf);
      currparm=currparm->next;
    }  while (c);
    if (!c) {
      free_parm_exp(def);
      return;
    }
    strcpy(tmpbuf,file_info->expanded);
    expand_def(def,expand_buf);
    add_expand_buf(expand_buf,tmpbuf);
    file_info->expanded=expand_buf;
    free_parm_exp(def);
  } else {
    strcpy(tmpbuf,file_info->expanded);
    add_expand_buf(def->definition,tmpbuf);
    file_info->expanded=expand_buf;
  }
}

void expand(struct define *def, filptr *file_info)
{
  int c,paren_counter,in_string,counter;
  struct parm *currparm;

  if (def->has_paren) {
    do {
      c=fgetc(file_info->curr_file);
      if (c=='\n')
        if (!(file_info->previous))
          ++(file_info->phys_line);
    } while ((c!=EOF) && isspace(c));
    if (c!='(')
      return;
    paren_counter=0;
    in_string=0;
    currparm=def->params;
    do {
      do {
        c=fgetc(file_info->curr_file);
        if (c=='\n')
          if (!(file_info->previous))
            ++(file_info->phys_line);
      } while ((c!=EOF) && isspace(c));
      if (c==')')
        break;
      if (!currparm) {
        free_parm_exp(def);
        return;
      }
      counter=0;
      while ((c!=EOF) && (counter<EBUFSIZ)) {
        if (c=='\n') {
          if (!(file_info->previous))
            ++(file_info->phys_line);
          c=' ';
        }
        if (c=='\"')
          in_string=(!in_string);
        else if (c=='(' && (!in_string))
          paren_counter++;
        else if (c==')' && (!in_string))
          if (!paren_counter) {
            ungetc(c,file_info->curr_file);
            break;
          } else
            paren_counter--;
        else if (c==',' && (!paren_counter) && (!in_string))
          break;
        else if (c=='\\' && in_string) {
          c=fgetc(file_info->curr_file);
          if (counter<(EBUFSIZ-1))
            tmpbuf[counter++]='\\';
        }
        tmpbuf[counter++]=c;
        c=fgetc(file_info->curr_file);
      }
      tmpbuf[counter]='\0';
      currparm->exp=copy_string(tmpbuf);
      currparm=currparm->next;
    }  while (c!=EOF);
    if (c==EOF) {
      free_parm_exp(def);
      return;
    }
    expand_def(def,expand_buf);
    file_info->expanded=expand_buf;
    free_parm_exp(def);
  } else {
    strcpy(expand_buf,def->definition);
    file_info->expanded=expand_buf;
  }
}
