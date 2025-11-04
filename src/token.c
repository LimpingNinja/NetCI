/* token.c - tokenizes a file for the compiler */

#include "config.h"
#include "object.h"
#include "file.h"
#include "token.h"
#include "constrct.h"
#include "instr.h"

char expand_buf[EBUFSIZ+1];
char tmpbuf[EBUFSIZ+1];
char name_buf[MAX_TOK_LEN+1];
char string_buf[MAX_STR_LEN+1];

#define isstart(c) (isalpha(c) || ((c)=='_'))
#define iscname(c) (isstart(c) || isdigit(c))
#define getch() *((file_info->expanded)++)
#define ungetch() --(file_info->expanded)

struct define *find_define(filptr *file_info, char *name)
{
  struct define *curr;

  curr=file_info->defs;
  while (curr) {
    if (!strcmp(curr->name,name))
      return curr;
    curr=curr->next;
  }
  return NULL;
}

unsigned char find_keyword(char *name)
{
  if (!strcmp(name,"if"))
    return IF_TOK;
  if ((!strcmp(name,"int")) || (!strcmp(name,"string")) ||
      (!strcmp(name,"object")) || (!strcmp(name,"var")))
    return VAR_DCL_TOK;
  if (!strcmp(name,"mapping"))
    return MAPPING_TOK;
  if (!strcmp(name,"static"))
    return STATIC_TOK;
  if (!strcmp(name,"else"))
    return ELSE_TOK;
  if (!strcmp(name,"while"))
    return WHILE_TOK;
  if (!strcmp(name,"for"))
    return FOR_TOK;
  if (!strcmp(name,"do"))
    return DO_TOK;
  if (!strcmp(name,"return"))
    return RETURN_TOK;
  return 0;
}

void get_exp_token(filptr *file_info, token_t *token)
{
  char c;
  signed long val;
  int counter;
  char *str;
  struct define *tmp;

  val=0;
  counter=0;
  c=getch();
  while (c && isspace(c)) c=getch();
  if (isstart(c)) {
    while ((counter<MAX_TOK_LEN) && iscname(c)) {
      name_buf[counter++]=c;
      c=getch();
    }
    if (iscname(c)) {
      token->type=NO_TOK;
      return;
    }
    name_buf[counter]='\0';
    ungetch();
    if ((token->type=find_keyword(name_buf)))
      return;
    if ((tmp=find_define(file_info,name_buf)))
      if (file_info->depth<(MAX_DEPTH-1)) {
        expand_exp(tmp,file_info);
        (file_info->depth)++;
        get_exp_token(file_info,token);
        return;
      } else {
        set_c_err_msg("recursive #define");
        token->type=NO_TOK;
        return;
      }
    token->type=NAME_TOK;
    token->token_data.name=name_buf;
    return;
  }
  if (isdigit(c)) {
    while (isdigit(c)) {
      val=(val*10)+digit_value(c);
      c=getch();
    }
    ungetch();
    token->type=INTEGER_TOK;
    token->token_data.integer=val;
    return;
  }
  if (c=='\"') {
    str=string_buf;
    c=getch();
    while (c && (c!='\"') && ((counter++)<MAX_STR_LEN)) {
      if (c=='\\') {
        c=getch();
        if (c=='n')
          c='\n';
        if (c=='t')
          c='\t';
        if (c=='r')
          c='\r';
        if (c=='a')
          c='\a';
        if (c=='b')
          c='\b';
        if (c=='f')
          c='\f';
        if (c=='v')
          c='\v';
      }
      *(str++)=c;
      if (c)
        c=getch();
    }
    if (c!='\"') {
      token->type=NO_TOK;
      return;
    }
    *(str)='\0';
    token->type=STRING_TOK;
    token->token_data.name=string_buf;
    return;
  }
  if (c=='{') {
    token->type=LBRACK_TOK;
    return;
  }
  if (c=='}') {
    c=fgetc(file_info->curr_file);
    if (c==')') {
      token->type=RARRASGN_TOK;  /* }) for array literal end */
      return;
    }
    ungetc(c,file_info->curr_file);
    token->type=RBRACK_TOK;
    return;
  }
  if (c==',') {
    token->type=COMMA_TOK;
    return;
  }
  if (c==';') {
    token->type=SEMI_TOK;
    return;
  }
  if (c=='(') {
    c=fgetc(file_info->curr_file);
    if (c=='{') {
      token->type=LARRASGN_TOK;  /* ({ for array literal start */
      return;
    }
    if (c=='[') {
      token->type=LMAPSGN_TOK;  /* ([ for mapping literal start */
      return;
    }
    ungetc(c,file_info->curr_file);
    token->type=LPAR_TOK;
    return;
  }
  if (c==')') {
    token->type=RPAR_TOK;
    return;
  }
  if (c=='[') {
    token->type=LARRAY_TOK;
    return;
  }
  if (c=='?') {
    token->type=COND_OPER;
    return;
  }
  if (c==']') {
    token->type=RARRAY_TOK;
    return;
  }
  if (c=='.') {
    token->type=DOT_TOK;
    return;
  }
  if (c==':') {
    token->type=COLON_TOK;
    c=getch();
    if (c==':')
      token->type=SECOND_TOK;
    else
      ungetch();
    return;
  }
  if (c=='=') {
    token->type=EQ_OPER;
    c=getch();
    if (c=='=')
      token->type=CONDEQ_OPER;
    else
      ungetch();
    return;
  }
  if (c=='+') {
    token->type=ADD_OPER;
    c=getch();
    if (c=='+')
      token->type=POSTADD_OPER;
    else if (c=='=')
      token->type=PLEQ_OPER;
    else
      ungetch();
    return;
  }
  if (c=='-') {
    token->type=MIN_OPER;
    c=getch();
    if (c=='-')
      token->type=POSTMIN_OPER;
    else if (c=='=')
      token->type=MIEQ_OPER;
    else if (c=='>')
      token->type=CALL_TOK;
    else
      ungetch();
    return;
  }
  if (c=='*') {
    token->type=MUL_OPER;
    c=getch();
    if (c=='=')
      token->type=MUEQ_OPER;
    else
      ungetch();
    return;
  }
  if (c=='/') {
    token->type=DIV_OPER;
    c=getch();
    if (c=='=')
      token->type=DIEQ_OPER;
    else
      ungetch();
    return;
  }
  if (c=='%') {
    token->type=MOD_OPER;
    c=getch();
    if (c=='=')
      token->type=MOEQ_OPER;
    else
      ungetch();
    return;
  }
  if (c=='&') {
    token->type=BITAND_OPER;
    c=getch();
    if (c=='=')
      token->type=ANEQ_OPER;
    else if (c=='&')
      token->type=AND_OPER;
    else
      ungetch();
    return;
  }
  if (c=='^') {
    token->type=EXOR_OPER;
    c=getch();
    if (c=='=')
      token->type=EXEQ_OPER;
    else
      ungetch();
    return;
  }
  if (c=='|') {
    token->type=BITOR_OPER;
    c=getch();
    if (c=='=')
      token->type=OREQ_OPER;
    else if (c=='|')
      token->type=OR_OPER;
    else
      ungetch();
    return;
  }
  if (c=='!') {
    token->type=NOT_OPER;
    c=getch();
    if (c=='=')
      token->type=NOTEQ_OPER;
    else
      ungetch();
    return;
  }
  if (c=='~') {
    token->type=BITNOT_OPER;
    return;
  }
  if (c=='<') {
    token->type=LESS_OPER;
    c=getch();
    if (c=='=')
      token->type=LESSEQ_OPER;
    else if (c=='<') {
      token->type=LS_OPER;
      c=getch();
      if (c=='=')
        token->type=LSEQ_OPER;
      else
        ungetch();
    } else
      ungetch();
    return;
  }
  if (c=='>') {
    token->type=GREAT_OPER;
    c=getch();
    if (c=='=')
      token->type=GREATEQ_OPER;
    else if (c=='>') {
      token->type=RS_OPER;
      c=getch();
      if (c=='=')
        token->type=RSEQ_OPER;
      else
        ungetch();
    } else
      ungetch();
    return;
  }
  token->type=NO_TOK;
  return;
}

void unget_token(filptr *file_info, token_t *token)
{
  file_info->put_back_token=*token;
  file_info->is_put_back=1;
}

void tokenize_name(filptr *file_info, token_t *token)
{
  int c,counter;
  struct define *tmp;

  counter=0;
  c=fgetc(file_info->curr_file);
  while ((counter<MAX_TOK_LEN) && (c!=EOF) && iscname(c)) {
    name_buf[counter++]=c;
    c=fgetc(file_info->curr_file);
  }
  if ((c==EOF) || iscname(c)) {
    token->type=NO_TOK;
    return;
  }
  name_buf[counter]='\0';
  ungetc(c,file_info->curr_file);
  if ((token->type=find_keyword(name_buf)))
    return;
  if ((tmp=find_define(file_info,name_buf))) {
    expand(tmp,file_info);
    return;
  }
  token->type=NAME_TOK;
  token->token_data.name=name_buf;
} 

void tokenize_int(filptr *file_info, token_t *token)
{
  int c;
  signed long val;

  val=0;
  c=fgetc(file_info->curr_file);
  while ((c!=EOF) && isdigit(c)) {
    val=(val*10)+digit_value(c);
    c=fgetc(file_info->curr_file);
  }
  ungetc(c,file_info->curr_file);
  token->type=INTEGER_TOK;
  token->token_data.integer=val;
} 

void tokenize_string(filptr *file_info, token_t *token)
{
  char *str;
  int counter;
  int c;

  counter=0;
  str=string_buf;
  c=fgetc(file_info->curr_file);
  while ((c!=EOF) && (c!='\"') && (c!='\n') && ((counter++)<MAX_STR_LEN)) {
    if (c=='\\') {
      c=fgetc(file_info->curr_file);
      if (c=='n')
        c='\n';
      if (c=='t')
        c='\t';
      if (c=='r')
        c='\r';
      if (c=='a')
        c='\a';
      if (c=='b')
        c='\b';
      if (c=='f')
        c='\f';
      if (c=='v')
        c='\v';
    }
    *(str++)=c;
    c=fgetc(file_info->curr_file);
  }
  if (c!='\"') {
    token->type=NO_TOK;
    return;
  }
  *(str)='\0';
  token->type=STRING_TOK;
  token->token_data.name=string_buf;
}

void get_token(filptr *file_info, token_t *token)
{
  int c,done;
  struct file_stack *tmp;

  if (file_info->is_put_back) {
    *token=file_info->put_back_token;
    file_info->is_put_back=0;
    return;
  }
  while (1) {
    if (file_info->expanded) {
      while (isspace(*(file_info->expanded)))
        (file_info->expanded)++;
      if (*(file_info->expanded)) {
        get_exp_token(file_info,token);
        return;
      } else {
        file_info->expanded=NULL;
        file_info->depth=0;
      }
    }
    c=fgetc(file_info->curr_file);
    if (c==EOF) {
      if (file_info->previous) {
        close_file(file_info->curr_file);
        file_info->curr_file=file_info->previous->file_ptr;
        tmp=file_info->previous;
        file_info->previous=tmp->previous;
        FREE(tmp);
        continue;
      }
      token->type=EOF_TOK;
      return;
    }
    if (c=='\n') {
      if (!(file_info->previous))
        ++(file_info->phys_line);
      c=fgetc(file_info->curr_file);
      if (c=='#') {
        if (preprocess(file_info)) {
          token->type=NO_TOK;
          return;
        }
      } else
        ungetc(c,file_info->curr_file);
      continue;
    }
    if (isspace(c))
      continue;
    if (isstart(c)) {
      ungetc(c,file_info->curr_file);
      tokenize_name(file_info,token);
      if (file_info->expanded)
        continue;
      return;
    }
    if (isdigit(c)) {
      ungetc(c,file_info->curr_file);
      tokenize_int(file_info,token);
      return;
    }
    if (c=='\"') {
      tokenize_string(file_info,token);
      return;
    }
    if (c=='/') {
      c=fgetc(file_info->curr_file);
      if (c=='*') {
        c=fgetc(file_info->curr_file);
        done=0;
        while ((c!=EOF) && (!done)) {
          if (c=='\n')
            if (!(file_info->previous))
              (file_info->phys_line)++;
          if (c=='*') {
            c=fgetc(file_info->curr_file);
            if (c=='/')
              done=1;
          } else
            c=fgetc(file_info->curr_file);
        }
        if (c!='/') {
          token->type=NO_TOK;
          return;
        }
        continue;
      }
      ungetc(c,file_info->curr_file);
      c='/';
    }
    if (c=='{') {
      token->type=LBRACK_TOK;
      return;
    }
    if (c=='}') {
      c=fgetc(file_info->curr_file);
      if (c==')') {
        token->type=RARRASGN_TOK;  /* }) for array literal end */
        return;
      }
      ungetc(c,file_info->curr_file);
      token->type=RBRACK_TOK;
      return;
    }
    if (c==',') {
      token->type=COMMA_TOK;
      return;
    }
    if (c==';') {
      token->type=SEMI_TOK;
      return;
    }
    if (c=='(') {
      c=fgetc(file_info->curr_file);
      if (c=='{') {
        token->type=LARRASGN_TOK;  /* ({ for array literal start */
        return;
      }
      if (c=='[') {
        token->type=LMAPSGN_TOK;  /* ([ for mapping literal start */
        return;
      }
      ungetc(c,file_info->curr_file);
      token->type=LPAR_TOK;
      return;
    }
    if (c==')') {
      token->type=RPAR_TOK;
      return;
    }
    if (c=='[') {
      token->type=LARRAY_TOK;
      return;
    }
    if (c=='?') {
      token->type=COND_OPER;
      return;
    }
    if (c==']') {
      token->type=RARRAY_TOK;
      return;
    }
    if (c=='.') {
      token->type=DOT_TOK;
      return;
    }
    if (c==':') {
      token->type=COLON_TOK;
      c=fgetc(file_info->curr_file);
      if (c==':')
        token->type=SECOND_TOK;
      else
        ungetc(c,file_info->curr_file);
      return;
    }
    if (c=='=') {
      token->type=EQ_OPER;
      c=fgetc(file_info->curr_file);
      if (c=='=')
        token->type=CONDEQ_OPER;
      else
        ungetc(c,file_info->curr_file);
      return;
    }
    if (c=='+') {
      token->type=ADD_OPER;
      c=fgetc(file_info->curr_file);
      if (c=='+')
        token->type=POSTADD_OPER;
      else if (c=='=')
        token->type=PLEQ_OPER;
      else
        ungetc(c,file_info->curr_file);
      return;
    }
    if (c=='-') {
      token->type=MIN_OPER;
      c=fgetc(file_info->curr_file);
      if (c=='-')
        token->type=POSTMIN_OPER;
      else if (c=='=')
        token->type=MIEQ_OPER;
      else if (c=='>')
      token->type=CALL_TOK;
      else
        ungetc(c,file_info->curr_file);
      return;
    }
    if (c=='*') {
      token->type=MUL_OPER;
      c=fgetc(file_info->curr_file);
      if (c=='=')
        token->type=MUEQ_OPER;
      else
        ungetc(c,file_info->curr_file);
      return;
    }
    if (c=='/') {
      token->type=DIV_OPER;
      c=fgetc(file_info->curr_file);
      if (c=='=')
        token->type=DIEQ_OPER;
      else
        ungetc(c,file_info->curr_file);
      return;
    }
    if (c=='%') {
      token->type=MOD_OPER;
      c=fgetc(file_info->curr_file);
      if (c=='=')
        token->type=MOEQ_OPER;
      else
        ungetc(c,file_info->curr_file);
      return;
    }
    if (c=='&') {
      token->type=BITAND_OPER;
      c=fgetc(file_info->curr_file);
      if (c=='=')
        token->type=ANEQ_OPER;
      else if (c=='&')
        token->type=AND_OPER;
      else
        ungetc(c,file_info->curr_file);
      return;
    }
    if (c=='^') {
      token->type=EXOR_OPER;
      c=fgetc(file_info->curr_file);
      if (c=='=')
        token->type=EXEQ_OPER;
      else
        ungetc(c,file_info->curr_file);
      return;
    }
    if (c=='|') {
      token->type=BITOR_OPER;
      c=fgetc(file_info->curr_file);
      if (c=='=')
        token->type=OREQ_OPER;
      else if (c=='|')
        token->type=OR_OPER;
      else
        ungetc(c,file_info->curr_file);
      return;
    }
    if (c=='!') {
      token->type=NOT_OPER;
      c=fgetc(file_info->curr_file);
      if (c=='=')
        token->type=NOTEQ_OPER;
      else
        ungetc(c,file_info->curr_file);
      return;
    }
    if (c=='~') {
      token->type=BITNOT_OPER;
      return;
    }
    if (c=='<') {
      token->type=LESS_OPER;
      c=fgetc(file_info->curr_file);
      if (c=='=')
        token->type=LESSEQ_OPER;
      else if (c=='<') {
        token->type=LS_OPER;
        c=fgetc(file_info->curr_file);
        if (c=='=')
          token->type=LSEQ_OPER;
        else
          ungetc(c,file_info->curr_file);
      } else
        ungetc(c,file_info->curr_file);
      return;
    }
    if (c=='>') {
      token->type=GREAT_OPER;
      c=fgetc(file_info->curr_file);
      if (c=='=')
        token->type=GREATEQ_OPER;
      else if (c=='>') {
        token->type=RS_OPER;
        c=fgetc(file_info->curr_file);
        if (c=='=')
          token->type=RSEQ_OPER;
        else
          ungetc(c,file_info->curr_file);
      } else
        ungetc(c,file_info->curr_file);
      return;
    }
    token->type=NO_TOK;
    return;
  }
}
