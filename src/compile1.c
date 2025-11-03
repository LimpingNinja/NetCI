/* compile.c */

/* This file contains the functions to compile CI-C into an easily
   interpreted form */

#include "config.h"
#include "object.h"
#include "instr.h"
#include "constrct.h"
#include "token.h"
#include "globals.h"
#include "file.h"

/* The precedence array - keeps track of operator precedences */

struct
{
  int comp_prec;             /* precedence used in comparison */
  int res_prec;              /* result precedence passed on */
} prec_array[NUM_OPERS]={ 1,1,
                          3,2,3,2,3,2,3,2,3,2,3,2,3,2,3,2,3,2,3,2,3,2,
                          5,4,
                          6,6,
                          7,7,
                          8,8,
                          9,9,
                          10,10,
                          11,11,11,11,
                          12,12,12,12,12,12,12,12,
                          13,13,13,13,
                          14,14,14,14,
                          15,15,15,15,15,15,
                          17,16,17,16,17,16,17,16,17,16,17,16,17,16 };

/* scall_array contains the names of system calls */

char *scall_array[NUM_SCALLS]={ "add_verb","add_xverb","call_other",
  "alarm","remove_alarm","caller_object","clone_object","destruct",
  "contents","next_object","location","next_child","parent","next_proto",
  "move_object","this_object","this_player","set_interactive","interactive",
  "set_priv","priv","in_editor","connected","get_devconn","send_device",
  "reconnect_device","disconnect_device","random","time","mktime",
  "typeof","command","compile_object","edit","cat","ls","rm","cp","mv",
  "mkdir","rmdir","hide","unhide","chown","syslog","sscanf","sprintf",
  "midstr","strlen","leftstr","rightstr","subst","instr","otoa","itoa",
  "atoi","atoo","upcase","downcase","is_legal","otoi","itoo","chmod",
  "fread","fwrite","remove_verb","ferase","chr","asc","sysctl","prototype",
  "iterate","next_who","get_devidle","get_conntime","connect_device",
  "flush_device","attach","this_component","detach","table_get","table_set",
  "table_delete","fstat","fowner","get_hostname","get_address",
  "set_localverbs","localverbs","next_verb","get_devport","get_devnet",
  "redirect_input","get_input_func","get_master","is_master","input_to",
  "sizeof","implode","explode","member_array","sort_array","reverse",
  "unique_array"
};

/* The functions themselves */

void set_c_err_msg(char *msg) {
  if (!c_err_msg)
    c_err_msg=msg;
}

void resize(fn_t *fn, unsigned long newsize)
{
  struct var *tmp;
  unsigned long x;

  if (newsize) {
    tmp=(struct var *) MALLOC(newsize*sizeof(struct var));
    for (x=0;x<(fn->num_code);x++)
      tmp[x]=fn->code[x];
    FREE(fn->code);
    fn->code=tmp;
  }
}

#define make_new(fn)                                                        \
  if ((fn->num_code)) {                                                     \
    if ((fn->num_code+1)>(fn->num_alloc))                                   \
      resize(fn,(fn->num_alloc)+=FN_BLK);                                   \
    (fn->num_code)++;                                                       \
  } else {                                                                  \
    fn->code=(struct var *) MALLOC(FN_BLK*sizeof(struct var));              \
    (fn->num_code)++;                                                       \
    fn->num_alloc=FN_BLK;                                                   \
  }

#define DO_FUNC1(A,B,C,D)                                                   \
  void A(fn_t *curr_fn, B val)                                              \
  {                                                                         \
    unsigned long x;                                                        \
                                                                            \
    x=curr_fn->num_code;                                                    \
    make_new(curr_fn)                                                       \
    curr_fn->code[x].type=C;                                                \
    curr_fn->code[x].value.D=val;                                           \
  }

#define DO_FUNC2(A,B)                                                       \
  void A(fn_t *curr_fn)                                                     \
  {                                                                         \
    make_new(curr_fn)                                                       \
    curr_fn->code[curr_fn->num_code-1].type=B;                              \
  }

#define DO_FUNC3(A,B)                                                       \
  void A(fn_t *curr_fn, unsigned int ref, unsigned int size)                \
  {                                                                         \
    unsigned long x;                                                        \
                                                                            \
    x=curr_fn->num_code;                                                    \
    make_new(curr_fn)                                                       \
    curr_fn->code[x].type=B;                                                \
    curr_fn->code[x].value.l_value.ref=ref;                                 \
    curr_fn->code[x].value.l_value.size=size;                               \
  }

DO_FUNC1(add_code_branch,    unsigned long,    BRANCH,         num)
DO_FUNC1(add_code_new_line,  unsigned int,     NEW_LINE,       num)
DO_FUNC1(add_code_instr,     unsigned char,    ASM_INSTR,      instruction)
DO_FUNC1(add_code_jump,      unsigned long,    JUMP,           num)
DO_FUNC1(add_code_integer,   signed long,      INTEGER,        integer)
DO_FUNC1(add_code_func_call, struct fns *,     FUNC_CALL,      func_call)
DO_FUNC1(add_code_func_name, char *,           FUNC_NAME,      string)
DO_FUNC1(add_code_num_args,  unsigned int,     NUM_ARGS,       num)

DO_FUNC2(add_code_return,    RETURN)
DO_FUNC2(add_code_gr,        GLOBAL_REF)
DO_FUNC2(add_code_lr,        LOCAL_REF)

DO_FUNC3(add_code_glv,       GLOBAL_L_VALUE)
DO_FUNC3(add_code_llv,       LOCAL_L_VALUE)

void add_code_string(fn_t *curr_fn, char *val)
{
  unsigned long x;

  x=curr_fn->num_code;
  make_new(curr_fn);
  if (*val) {
    curr_fn->code[x].type=STRING;
    curr_fn->code[x].value.string=copy_string(val);
  } else {
    curr_fn->code[x].type=INTEGER;
    curr_fn->code[x].value.integer=0;
  }
}

void free_sym_t(sym_tab_t *sym)
{
  struct var_tab *curr_var, *next_var;
  struct array_size *curr_array, *next_array;

  curr_var=sym->varlist;
  while (curr_var) {
    next_var=curr_var->next;
    curr_array=curr_var->array;
    while (curr_array) {
      next_array=curr_array->next;
      FREE(curr_array);
      curr_array=next_array;
    }
    FREE(curr_var->name);
    FREE(curr_var);
    curr_var=next_var;
  }
}

void free_file_stack(filptr *file_info)
{
  struct file_stack *curr, *next;

  curr=file_info->previous;
  while (curr) {
    close_file(curr->file_ptr);
    next=curr->previous;
    FREE(curr);
    curr=next;
  }
}

void free_define(filptr *file_info)
{
  struct define *curr, *next;
  struct parm *currparm, *nextparm;

  curr=file_info->defs;
  while (curr) {
    next=curr->next;
    currparm=curr->params;
    while (currparm) {
      nextparm=currparm->next;
      FREE(currparm->name);
      if (currparm->exp)
        FREE(currparm->exp);
      FREE(currparm);
      currparm=nextparm;
    }
    FREE(curr->name);
    FREE(curr->definition);
    FREE(curr);
    curr=next;
  }
}

unsigned int calc_size(struct array_size *array)
{
  if (array==NULL)
    return 1;
  else
    return (array->size)*calc_size(array->next);
}

unsigned char find_syscall(char *name)
{
  int x;
  for (x=0;x<NUM_SCALLS;x++)
    if (!strcmp(name,scall_array[x]))
      return x+NUM_OPERS;
  if (!strcmp(name,"new")) return S_CLONE_OBJECT;
  return 0;
}

struct fns *find_func(struct fns *position, char *name)
{
  while (position) {
    if (!strcmp(position->funcname,name))
      return position;
    position=position->next;
  }
  return NULL;
}

struct var_tab *find_var(char *name, sym_tab_t *sym)
{
  struct var_tab *curr;

  curr=sym->varlist;
  while (curr) {
    if (!strcmp(curr->name,name))
      return curr;
    curr=curr->next;
  }
  return NULL;
}

unsigned int add_var(filptr *file_info, sym_tab_t *sym)
{
  token_t token;
  struct var_tab *curr_var;
  int done;
  struct array_size **rest;
  int is_pointer = 0;

  /* Check for pointer syntax: int *name */
  get_token(file_info,&token);
  if (token.type == MUL_OPER) {
    is_pointer = 1;
    get_token(file_info,&token);
  }
  
  if (token.type!=NAME_TOK) {
    set_c_err_msg("expected variable name declaration");
    return file_info->phys_line;
  }
  curr_var=(struct var_tab *) MALLOC(sizeof(struct var_tab));
  curr_var->name=copy_string(token.token_data.name);
  curr_var->base=sym->num;
  curr_var->array=NULL;
  curr_var->next=sym->varlist;
  sym->varlist=curr_var;
  rest=&(curr_var->array);
  done=0;
    while (!done) {
        get_token(file_info, &token);
        if (token.type != LARRAY_TOK) {
            done=1;
        }
    else {
      get_token(file_info,&token);
      if (token.type!=INTEGER_TOK && token.type!=RARRAY_TOK) {
        set_c_err_msg("array size must be integer constant");
        return file_info->phys_line;
      }
      if (token.type!=RARRAY_TOK) {
        *rest=(struct array_size *) MALLOC(sizeof(struct array_size));
        (*rest)->size=token.token_data.integer;
        (*rest)->next=NULL;
        rest=&((*rest)->next);
        get_token(file_info,&token);
        if (token.type!=RARRAY_TOK) {
          set_c_err_msg("expected ]");
          return file_info->phys_line;
        }
      }
      else {
        *rest=(struct array_size *) MALLOC(sizeof(struct array_size));
        (*rest)->size=255;
        (*rest)->next=NULL;
        rest=&((*rest)->next);
      }
    }
  }
  unget_token(file_info,&token);
  
  /* If pointer syntax was used without brackets, create default array */
  if (is_pointer && curr_var->array == NULL) {
    curr_var->array = (struct array_size *) MALLOC(sizeof(struct array_size));
    curr_var->array->size = 255;  /* Default size for unsized arrays */
    curr_var->array->next = NULL;
  }
  
  sym->num+=calc_size(curr_var->array);
  return 0;
}

unsigned int parse_base(filptr *file_info,fn_t *curr_fn, sym_tab_t
                        *loc_sym, struct array_size **rest)
{
  token_t token;

  if (*rest==NULL) {
    set_c_err_msg("array reference on non-array type");
    return file_info->phys_line;
  }
  (*rest)=(*rest)->next;
  if (parse_exp(file_info,curr_fn,loc_sym,0,0))
    return file_info->phys_line;
  add_code_integer(curr_fn,calc_size(*rest));
  add_code_instr(curr_fn,MUL_OPER);
  
  /* Check if there's another dimension coming */
  get_token(file_info,&token);
  if (token.type!=RARRAY_TOK) {
    set_c_err_msg("expected ]");
    return file_info->phys_line;
  }
  get_token(file_info,&token);
  if (token.type==LARRAY_TOK) {
    /* More dimensions - ADD now and recurse */
    add_code_instr(curr_fn,ADD_OPER);
    return parse_base(file_info,curr_fn,loc_sym,rest);
  }
  /* Last dimension - DON'T add, let runtime do it after bounds check */
  unget_token(file_info,&token);
  return 0;
}

unsigned int parse_var(char *name,filptr *file_info,fn_t *curr_fn,sym_tab_t
                       *loc_sym)
{
  token_t token;
  int global;
  struct var_tab *the_var;
  struct array_size *rest;
  char logbuf[256];

  sprintf(logbuf, "parse_var: name='%s'", name);
  logger(LOG_DEBUG, logbuf);

  if ((the_var=find_var(name,loc_sym)))
    global=0;
  else
    if ((the_var=find_var(name,file_info->glob_sym)))
      global=1;
    else {
      set_c_err_msg("variable undefined");
      return file_info->phys_line;
    }
  get_token(file_info,&token);
  
  sprintf(logbuf, "parse_var: next token.type=%d (LARRAY_TOK=%d)", token.type, LARRAY_TOK);
  logger(LOG_DEBUG, logbuf);
  
  if (token.type!=LARRAY_TOK) {
    unget_token(file_info,&token);
    sprintf(logbuf, "parse_var: generating L_VALUE for '%s', global=%d", name, global);
    logger(LOG_DEBUG, logbuf);
    if (global)
      add_code_glv(curr_fn,the_var->base,calc_size(the_var->array));
    else
      add_code_llv(curr_fn,the_var->base,calc_size(the_var->array));
  } else {
    rest=the_var->array;
    add_code_integer(curr_fn,the_var->base);
    if (parse_base(file_info,curr_fn,loc_sym,&rest))
      return file_info->phys_line;
    /* Push size of FIRST dimension for bounds checking, not remaining */
    add_code_integer(curr_fn,the_var->array->size);
    if (global)
      add_code_gr(curr_fn);
    else
      add_code_lr(curr_fn);
  }
  return 0;
}

unsigned int parse_arglist(filptr *file_info, fn_t *curr_fn, sym_tab_t
                          *loc_sym)
{
  token_t token;
  int done,argcount;

  done=0;
  get_token(file_info,&token);
  if (token.type==RPAR_TOK)
    argcount=0;
  else
    argcount=1;
  unget_token(file_info,&token);
  while (!done) {
    if (parse_exp(file_info,curr_fn,loc_sym,1,0))
      return file_info->phys_line;
    get_token(file_info,&token);
    if (token.type==RPAR_TOK)
      done=1;
    else
      if (token.type!=COMMA_TOK) {
        set_c_err_msg("expected )");
        return file_info->phys_line;
      } else
        argcount++;
  }
  add_code_num_args(curr_fn,argcount);
  return 0;
}

unsigned int parse_exp(filptr *file_info, fn_t *curr_fn, sym_tab_t *loc_sym,
                       int prec, int last_was_arg)
{
  token_t token;
  char name[MAX_TOK_LEN+1];
  unsigned char instr;
  struct fns *func;
  unsigned long marker1,marker2;
  char logbuf[256];

  get_token(file_info,&token);
  if (token.type==LPAR_TOK) {
    if (last_was_arg) {
      set_c_err_msg("expected arithmetic operation");
      return file_info->phys_line;
    }
    if (parse_exp(file_info,curr_fn,loc_sym,0,0))
      return file_info->phys_line;
    get_token(file_info,&token);
    if (token.type!=RPAR_TOK) {
      set_c_err_msg("expected )");
      return file_info->phys_line;
    }
    last_was_arg=1;
    get_token(file_info,&token);
  }
  if (token.type==INTEGER_TOK) {
    if (last_was_arg) {
      set_c_err_msg("expected arithmetic operation");
      return file_info->phys_line;
    }
    add_code_integer(curr_fn,token.token_data.integer);
    last_was_arg=1;
    get_token(file_info,&token);
  }
  if (token.type==STRING_TOK) {
    if (last_was_arg) {
      set_c_err_msg("expected arithmetic operation");
      return file_info->phys_line;
    }
    add_code_string(curr_fn,token.token_data.name);
    last_was_arg=1;
    get_token(file_info,&token);
  }
  if (token.type==NAME_TOK) {
    strcpy(name,token.token_data.name);
    if (last_was_arg) {
      set_c_err_msg("expected arithmetic operation");
      return file_info->phys_line;
    }
    get_token(file_info,&token);
    if (token.type==LPAR_TOK) {
      if (parse_arglist(file_info,curr_fn,loc_sym))
        return file_info->phys_line;
      if ((func=find_func(file_info->curr_code->func_list,name)))
        add_code_func_call(curr_fn,func);
      else
        if ((instr=find_syscall(name)))
          add_code_instr(curr_fn,instr);
        else
          add_code_func_name(curr_fn,copy_string(name));
    } else {
      unget_token(file_info,&token);
      if (parse_var(name,file_info,curr_fn,loc_sym))
        return file_info->phys_line;
    }
    last_was_arg=1;
    get_token(file_info,&token);
  }
  if (token.type==SECOND_TOK) {
    set_c_err_msg("operation \'::\' unsupported");
    return file_info->phys_line;
  }
  if (token.type==DOT_TOK || token.type==CALL_TOK) {
    if (!last_was_arg) {
      set_c_err_msg("malformed expression (invalid use of call operator)");
      return file_info->phys_line;
    }
    get_token(file_info,&token);
    if (token.type!=NAME_TOK) {
      set_c_err_msg("expected function name following call operator");
      return file_info->phys_line;
    }
    add_code_string(curr_fn,token.token_data.name);
    get_token(file_info,&token);
    if (token.type!=LPAR_TOK) {
      set_c_err_msg("expected argument list or functional call");
      return file_info->phys_line;
    }
    if (parse_arglist(file_info,curr_fn,loc_sym))
      return file_info->phys_line;

/* NOTE THIS: since I know that the last thing parse_arglist does is
   dump a NUM_ARGS on the code list, I can increment that NUM_ARGS by two */

    curr_fn->code[curr_fn->num_code-1].value.num+=2;
    add_code_instr(curr_fn,S_CALL_OTHER);
    get_token(file_info,&token);
  }
  if (token.type<NUM_OPERS) {
    if ((token.type==MIN_OPER) && (!last_was_arg))
      token.type=UMIN_OPER;
    if ((token.type==POSTADD_OPER) && (!last_was_arg))
      token.type=PREADD_OPER;
    if ((token.type==POSTMIN_OPER) && (!last_was_arg))
      token.type=PREMIN_OPER;
    instr=token.type;
    if (prec_array[instr].comp_prec<=prec) {
      unget_token(file_info,&token);
      return 0;
    }
    if ((instr!=PREADD_OPER) && (instr!=PREMIN_OPER) && (instr!=UMIN_OPER)
        && (instr!=BITNOT_OPER) && (instr!=NOT_OPER) && (!last_was_arg)) {
      set_c_err_msg("malformed expression");
      return file_info->phys_line;
    }
    if (instr==COND_OPER) {
      add_code_branch(curr_fn,0);
      marker1=curr_fn->num_code-1;
      if (parse_exp(file_info,curr_fn,loc_sym,0,0))
        return file_info->phys_line;
      add_code_jump(curr_fn,0);
      marker2=curr_fn->num_code-1;
      add_code_instr(curr_fn,instr);
      curr_fn->code[marker1].value.num=curr_fn->num_code-1;
      get_token(file_info,&token);
      if (token.type!=COLON_TOK) {
        set_c_err_msg("expected :");
        return file_info->phys_line;
      }
      if (parse_exp(file_info,curr_fn,loc_sym,prec_array[instr].res_prec,0))
        return file_info->phys_line;
      add_code_instr(curr_fn,instr);
      curr_fn->code[marker2].value.num=curr_fn->num_code-1;
    } else {
      if ((instr!=POSTADD_OPER) && (instr!=POSTMIN_OPER))
        if (parse_exp(file_info,curr_fn,loc_sym,prec_array[instr].res_prec,
                       0))
          return file_info->phys_line;
      add_code_instr(curr_fn,instr);
    }
    return parse_exp(file_info,curr_fn,loc_sym,prec,1);
  }
  unget_token(file_info,&token);
  return 0;
}

unsigned int parse_block(filptr *file_info, fn_t *curr_fn, sym_tab_t *loc_sym)
{
  int done;
  token_t token;

  done=0;
  while (!done) {
    get_token(file_info,&token);
    if (token.type==RBRACK_TOK)
      done=1;
    else {
      unget_token(file_info,&token);
      if (parse_line(file_info,curr_fn,loc_sym))
        return file_info->phys_line;
    }
  }
  return 0;
}

unsigned int parse_line(filptr *file_info, fn_t *curr_fn, sym_tab_t *loc_sym)
{
  token_t token;
  unsigned long marker1,marker2;
  int done;

  get_token(file_info,&token);
  switch (token.type) {
    case VAR_DCL_TOK:
      done=0;
      while (!done) {
        if (add_var(file_info,loc_sym))
          return file_info->phys_line;
        get_token(file_info,&token);
        if (token.type==SEMI_TOK)
          done=1;
        else
          if (token.type!=COMMA_TOK) {
            set_c_err_msg("expected ;");
            return file_info->phys_line;
          }
      }
      break;
    case IF_TOK:
      get_token(file_info,&token);
      if (token.type!=LPAR_TOK) {
        set_c_err_msg("expected (");
        return file_info->phys_line;
      }
      if (parse_exp(file_info,curr_fn,loc_sym,0,0))
        return file_info->phys_line;
      get_token(file_info,&token);
      if (token.type!=RPAR_TOK) {
        set_c_err_msg("expected )");
        return file_info->phys_line;
      }
      add_code_branch(curr_fn,0);
      marker1=curr_fn->num_code-1;
      get_token(file_info,&token);
      if (token.type==LBRACK_TOK) {
        if (parse_block(file_info,curr_fn,loc_sym))
          return file_info->phys_line;
      } else {
        unget_token(file_info,&token);
        if (parse_line(file_info,curr_fn,loc_sym))
          return file_info->phys_line;
      }
      get_token(file_info,&token);
      if (token.type==ELSE_TOK) {
        add_code_jump(curr_fn,0);
        marker2=curr_fn->num_code-1;
        get_token(file_info,&token);
        add_code_new_line(curr_fn,file_info->phys_line);
        curr_fn->code[marker1].value.num=curr_fn->num_code-1;
        if (token.type==LBRACK_TOK) {
          if (parse_block(file_info,curr_fn,loc_sym))
            return file_info->phys_line;
        } else {
          unget_token(file_info,&token);
          if (parse_line(file_info,curr_fn,loc_sym))
            return file_info->phys_line;
        }
        marker1=marker2;
      } else
        unget_token(file_info,&token);
      get_token(file_info,&token);
      unget_token(file_info,&token);
      add_code_new_line(curr_fn,file_info->phys_line);
      curr_fn->code[marker1].value.num=curr_fn->num_code-1;
      break;
    case WHILE_TOK:
      get_token(file_info,&token);
      if (token.type!=LPAR_TOK) {
        set_c_err_msg("exepcted (");
        return file_info->phys_line;
      }
      add_code_new_line(curr_fn,file_info->phys_line);
      marker1=curr_fn->num_code-1;
      if (parse_exp(file_info,curr_fn,loc_sym,0,0))
        return file_info->phys_line;
      get_token(file_info,&token);
      if (token.type!=RPAR_TOK) {
        set_c_err_msg("expected )");
        return file_info->phys_line;
      }
      add_code_branch(curr_fn,0);
      marker2=curr_fn->num_code-1;
      get_token(file_info,&token);
      if (token.type==LBRACK_TOK) {
        if (parse_block(file_info,curr_fn,loc_sym))
          return file_info->phys_line;
      } else {
        unget_token(file_info,&token);
        if (parse_line(file_info,curr_fn,loc_sym))
          return file_info->phys_line;
      }
      add_code_jump(curr_fn,marker1);
      get_token(file_info,&token);
      unget_token(file_info,&token);
      add_code_new_line(curr_fn,file_info->phys_line);
      curr_fn->code[marker2].value.num=curr_fn->num_code-1;
      break;
    case FOR_TOK:
      get_token(file_info,&token);
      if (token.type!=LPAR_TOK) {
        set_c_err_msg("expected (");
        return file_info->phys_line;
      }
      if (parse_exp(file_info,curr_fn,loc_sym,0,0))
        return file_info->phys_line;
      get_token(file_info,&token);
      if (token.type!=SEMI_TOK) {
        set_c_err_msg("expected ;");
        return file_info->phys_line;
      }
      add_code_new_line(curr_fn,file_info->phys_line);
      marker1=curr_fn->num_code-1;
      add_code_integer(curr_fn,1);
      if (parse_exp(file_info,curr_fn,loc_sym,0,0))
        return file_info->phys_line;
      get_token(file_info,&token);
      if (token.type!=SEMI_TOK) {
        set_c_err_msg("expected ;");
        return file_info->phys_line;
      }
      add_code_branch(curr_fn,0);
      marker2=curr_fn->num_code-1;
      add_code_jump(curr_fn,0);
      add_code_new_line(curr_fn,file_info->phys_line);
      if (parse_exp(file_info,curr_fn,loc_sym,0,0))
        return file_info->phys_line;
      get_token(file_info,&token);
      if (token.type!=RPAR_TOK) {
        set_c_err_msg("expected (");
        return file_info->phys_line;
      }
      add_code_jump(curr_fn,marker1);
      add_code_new_line(curr_fn,file_info->phys_line);
      curr_fn->code[marker2+1].value.num=curr_fn->num_code-1;
      get_token(file_info,&token);
      if (token.type==LBRACK_TOK) {
        if (parse_block(file_info,curr_fn,loc_sym))
          return file_info->phys_line;
      } else {
        unget_token(file_info,&token);
        if (parse_line(file_info,curr_fn,loc_sym))
          return file_info->phys_line;
      }
      add_code_jump(curr_fn,marker2+2);
      add_code_new_line(curr_fn,file_info->phys_line);
      curr_fn->code[marker2].value.num=curr_fn->num_code-1;
      break;
    case DO_TOK:
      add_code_new_line(curr_fn,file_info->phys_line);
      marker1=curr_fn->num_code-1;
      get_token(file_info,&token);
      if (token.type==LBRACK_TOK) {
        if (parse_block(file_info,curr_fn,loc_sym))
          return file_info->phys_line;
      } else {
        unget_token(file_info,&token);
        if (parse_line(file_info,curr_fn,loc_sym))
          return file_info->phys_line;
      }
      get_token(file_info,&token);
      if (token.type!=WHILE_TOK) {
        set_c_err_msg("expected \'while\'");
        return file_info->phys_line;
      }
      get_token(file_info,&token);
      if (token.type!=LPAR_TOK) {
        set_c_err_msg("expected (");
        return file_info->phys_line;
      }
      if (parse_exp(file_info,curr_fn,loc_sym,0,0))
        return file_info->phys_line;
      get_token(file_info,&token);
      if (token.type!=RPAR_TOK) {
        set_c_err_msg("expected )");
        return file_info->phys_line;
      }
      get_token(file_info,&token);
      if (token.type!=SEMI_TOK) {
        set_c_err_msg("expected ;");
        return file_info->phys_line;
      }
      add_code_branch(curr_fn,0);
      marker2=curr_fn->num_code-1;
      add_code_jump(curr_fn,marker1);
      add_code_new_line(curr_fn,file_info->phys_line);
      curr_fn->code[marker2].value.num=curr_fn->num_code-1;
      break;
    case RETURN_TOK:
      get_token(file_info,&token);
      if (token.type==SEMI_TOK)
        add_code_integer(curr_fn,0);
      else {
        unget_token(file_info,&token);
        if (parse_exp(file_info,curr_fn,loc_sym,0,0))
          return file_info->phys_line;
        get_token(file_info,&token);
        if (token.type!=SEMI_TOK) {
          set_c_err_msg("expected ;");
          return file_info->phys_line;
        }
      }
      add_code_return(curr_fn);
      break;
    default:
      add_code_new_line(curr_fn,file_info->phys_line);
      unget_token(file_info,&token);
      if (parse_exp(file_info,curr_fn,loc_sym,0,0))
        return file_info->phys_line;
      get_token(file_info,&token);
      if (token.type!=SEMI_TOK) {
        set_c_err_msg("expected ;");
        return file_info->phys_line;
      }
  }
  return 0;
}

unsigned int top_level_parse(filptr *file_info)
{
  token_t token;
  sym_tab_t loc_sym;
  int done, is_static;
  fn_t curr_func;
  struct fns *tmp_fns;

  while (1) {
    is_static=0;
    done=0;
    get_token(file_info,&token);
    if (token.type==STATIC_TOK) {
      is_static=1;
      get_token(file_info,&token);
    }
    switch (token.type) {
      case EOF_TOK:
        return 0;
        break;
      case VAR_DCL_TOK:
        while (!done) {
          if (add_var(file_info,file_info->glob_sym))
            return file_info->phys_line;
          get_token(file_info,&token);
          if (token.type==SEMI_TOK)
            done=1;
          else
            if (token.type!=COMMA_TOK) {
              set_c_err_msg("expected ;");
              return file_info->phys_line;
            }
        }
        break;
      case NAME_TOK:
        curr_func.code=NULL;
        curr_func.num_code=0;
        curr_func.num_alloc=0;
        loc_sym.num=0;
        loc_sym.varlist=NULL;
        tmp_fns=(struct fns *) MALLOC(sizeof(struct fns));
        tmp_fns->is_static=is_static;
        tmp_fns->num_args=0;
        tmp_fns->num_instr=0;
        tmp_fns->num_locals=0;
        tmp_fns->code=NULL;
        tmp_fns->funcname=copy_string(token.token_data.name);
        tmp_fns->lst=NULL;  /* Initialize local symbol table */
        tmp_fns->next=file_info->curr_code->func_list;
        file_info->curr_code->func_list=tmp_fns;
        get_token(file_info,&token);
        if (token.type!=LPAR_TOK) {
          set_c_err_msg("expected (");
          return file_info->phys_line;
        }
        while (!done) {
          get_token(file_info,&token);
          if (token.type==RPAR_TOK)
            done=1;
          else {
            if (token.type!=VAR_DCL_TOK)
              unget_token(file_info,&token);
            if (add_var(file_info,&loc_sym)) {
              free_sym_t(&loc_sym);
              return file_info->phys_line;
            }
            get_token(file_info,&token);
            if (token.type==RPAR_TOK)
              unget_token(file_info,&token);
            else
              if (token.type!=COMMA_TOK) {
                set_c_err_msg("expected )");
                free_sym_t(&loc_sym);
                return file_info->phys_line;
              }
          }
        }
        tmp_fns->num_args=loc_sym.num;
        get_token(file_info,&token);
        if (token.type!=LBRACK_TOK) {
          set_c_err_msg("expected {");
          free_sym_t(&loc_sym);
          return file_info->phys_line;
        }
        add_code_new_line(&curr_func,file_info->phys_line);
        if (parse_block(file_info,&curr_func,&loc_sym)) {
          tmp_fns->code=curr_func.code;
          tmp_fns->num_instr=curr_func.num_code;
          free_sym_t(&loc_sym);
          return file_info->phys_line;
        }
        add_code_integer(&curr_func,0);
        add_code_return(&curr_func);
        resize(&curr_func,curr_func.num_code);
        tmp_fns->code=curr_func.code;
        tmp_fns->num_locals=loc_sym.num;
        tmp_fns->num_instr=curr_func.num_code;
        tmp_fns->lst=loc_sym.varlist;  /* Save local symbol table for array init */
        loc_sym.varlist=NULL;  /* Prevent free_sym_t from freeing it */
        free_sym_t(&loc_sym);
        break;
      default:
        return file_info->phys_line;
        break;
    }
  }
}
