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
#include "dbhandle.h"
#include "compile.h"
#include "cache.h"

/* Forward declarations */
int add_inherit(filptr *file_info, char *pathname);

/* Helper: Extract basename from path and remove .c extension for default alias */
static char *get_default_alias(char *pathname) {
    char *last_slash = strrchr(pathname, '/');
    char *basename = last_slash ? last_slash + 1 : pathname;
    
    /* Remove .c extension if present */
    char *dot = strrchr(basename, '.');
    if (dot && strcmp(dot, ".c") == 0) {
        char *alias = MALLOC(dot - basename + 1);
        strncpy(alias, basename, dot - basename);
        alias[dot - basename] = '\0';
        return alias;
    }
    
    return copy_string(basename);
}

/* Helper: Find a function by name in a proto's function list */
static struct fns *find_function_in_proto(char *func_name, struct proto *proto) {
    if (!proto || !proto->funcs) return NULL;
    
    struct fns *curr_func = proto->funcs->func_list;
    while (curr_func) {
        if (strcmp(curr_func->funcname, func_name) == 0) {
            return curr_func;
        }
        curr_func = curr_func->next;
    }
    
    return NULL;
}

/* Compute Method Resolution Order (MRO) for inheritance
 * Returns array of protos in linearized order: child first, then ancestors
 * This is a simplified depth-first traversal with duplicate removal
 */
static struct proto **compute_mro(struct code *current_code, int *mro_length) {
    struct proto **mro = MALLOC(sizeof(struct proto*) * 32);  /* Max depth 32 */
    int count = 0;
    
    /* Add current program first (if it has a proto) */
    if (current_code && current_code->inherits && current_code->inherits->entry) {
        /* For now, we don't have a back-pointer from code to proto
         * So we'll start with the inherits directly
         */
    }
    
    /* Traverse inherits in declaration order (depth-first) */
    struct inherit_list *curr = current_code ? current_code->inherits : NULL;
    while (curr && count < 32) {
        if (curr->entry && curr->entry->proto) {
            /* Check if already in MRO (for diamond inheritance) */
            int already_added = 0;
            for (int i = 0; i < count; i++) {
                if (mro[i] == curr->entry->proto) {
                    already_added = 1;
                    break;
                }
            }
            
            if (!already_added) {
                mro[count++] = curr->entry->proto;
                
                /* Recursively add that proto's parents */
                struct inherit_list *parent_inherits = curr->entry->proto->inherits;
                while (parent_inherits && count < 32) {
                    if (parent_inherits->entry && parent_inherits->entry->proto) {
                        /* Check for duplicates */
                        int dup = 0;
                        for (int i = 0; i < count; i++) {
                            if (mro[i] == parent_inherits->entry->proto) {
                                dup = 1;
                                break;
                            }
                        }
                        if (!dup) {
                            mro[count++] = parent_inherits->entry->proto;
                        }
                    }
                    parent_inherits = parent_inherits->next;
                }
            }
        }
        curr = curr->next;
    }
    
    *mro_length = count;
    return mro;
}

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
  "unique_array",NULL,"keys","values","map_delete","member",NULL,
  "save_value","restore_value","replace_string",NULL,NULL,"syswrite",
  "compile_string","crypt","read_file","write_file","remove","rename",
  "get_dir","file_size","users","objects","children","all_inventory"
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
DO_FUNC1(add_code_jump,        unsigned long,    JUMP,           num)
DO_FUNC1(add_code_integer,     signed long,      INTEGER,        integer)
DO_FUNC1(add_code_func_call,   struct fns *,     FUNC_CALL,      func_call)
DO_FUNC1(add_code_func_name,   char *,           FUNC_NAME,      string)
/* add_code_parent_call removed - now using indexed CALL_SUPER */
DO_FUNC1(add_code_num_args,    unsigned int,     NUM_ARGS,       num)

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
    if (scall_array[x] && !strcmp(name,scall_array[x]))
      return x+NUM_OPERS;
  
  /* LPC compatibility aliases - maps standard LPC names to NetCI names */
  /* NetCI Unix-style names (deprecated, use LPC standard names instead) */
  if (!strcmp(name,"fread")) return find_syscall("read_file");
  if (!strcmp(name,"fwrite")) return find_syscall("write_file");
  if (!strcmp(name,"rm")) return find_syscall("remove");
  if (!strcmp(name,"mv")) return find_syscall("rename");
  if (!strcmp(name,"ls")) return find_syscall("get_dir");
  
  /* Special aliases */
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

unsigned int add_var(filptr *file_info, sym_tab_t *sym, int is_mapping)
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
  /* Check if variable already exists in all_vars (from ancestors) */
  struct var_tab *existing = sym->varlist;
  while (existing) {
    if (!strcmp(existing->name, token.token_data.name)) {
      if (existing->origin_prog != NULL) {
        /* Variable exists from an ancestor - shadowing error */
        char errbuf[512];
        sprintf(errbuf, "variable '%s' already defined in ancestor '%s'", 
                token.token_data.name,
                existing->origin_prog->pathname ? existing->origin_prog->pathname : "unknown");
        set_c_err_msg(errbuf);
        return file_info->phys_line;
      }
      /* Variable already defined in current program - duplicate error */
      char errbuf[256];
      sprintf(errbuf, "variable '%s' already defined", token.token_data.name);
      set_c_err_msg(errbuf);
      return file_info->phys_line;
    }
    existing = existing->next;
  }
  
  curr_var=(struct var_tab *) MALLOC(sizeof(struct var_tab));
  curr_var->name=copy_string(token.token_data.name);
  curr_var->base=sym->num;
  curr_var->array=NULL;
  curr_var->is_mapping=is_mapping;
  curr_var->origin_prog=NULL;  /* NULL = defined in current program */
  curr_var->owner_local_index=curr_var->base;
  curr_var->next=sym->varlist;
  sym->varlist=curr_var;
  
  /* Also add to own_vars list ONLY if this is a GLOBAL variable */
  if (sym == file_info->glob_sym) {
    struct var_tab *own_var = (struct var_tab *) MALLOC(sizeof(struct var_tab));
    own_var->name = copy_string(token.token_data.name);
    own_var->base = curr_var->base;
    own_var->array = NULL; /* Will be set below if needed */
    own_var->is_mapping = is_mapping;
    own_var->origin_prog = NULL;
    /* owner_local_index should be index within OWN variables, not flattened GST */
    /* Count how many own vars we have so far */
    unsigned short own_count = 0;
    struct var_tab *count_var = file_info->curr_code->own_vars;
    while (count_var) {
      own_count++;
      count_var = count_var->next;
    }
    own_var->owner_local_index = own_count; /* This will be the index in own_vars list */
    own_var->next = file_info->curr_code->own_vars;
    file_info->curr_code->own_vars = own_var;
  }
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
  if (is_pointer && curr_var->array == NULL && !is_mapping) {
    curr_var->array = (struct array_size *) MALLOC(sizeof(struct array_size));
    curr_var->array->size = 255;  /* Default size for unsized arrays */
    curr_var->array->next = NULL;
  }
  
  /* Copy array info to own_var (only if this was a global variable) */
  if (sym == file_info->glob_sym && curr_var->array) {
    struct var_tab *own_var = file_info->curr_code->own_vars;  /* Get the one we just added */
    struct array_size *src = curr_var->array;
    struct array_size **dst = &(own_var->array);
    while (src) {
      *dst = (struct array_size *) MALLOC(sizeof(struct array_size));
      (*dst)->size = src->size;
      (*dst)->next = NULL;
      dst = &((*dst)->next);
      src = src->next;
    }
  }
  
  /* IMPORTANT: Mappings don't use the array field at all!
   * - Mappings are identified by the is_mapping flag
   * - They use hash table storage (struct heap_mapping), not contiguous memory
   * - The array field remains NULL for mappings
   * - This is NOT a hack - mappings and arrays are fundamentally different data structures
   */
  
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
  // char logbuf[256];

  // sprintf(logbuf, "parse_var: name='%s'", name);
  // logger(LOG_DEBUG, logbuf);

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
  
  // sprintf(logbuf, "parse_var: next token.type=%d (LARRAY_TOK=%d)", token.type, LARRAY_TOK);
  // logger(LOG_DEBUG, logbuf);
  
  if (token.type!=LARRAY_TOK) {
    unget_token(file_info,&token);
    // sprintf(logbuf, "parse_var: generating L_VALUE for '%s', global=%d", name, global);
    // logger(LOG_DEBUG, logbuf);
    if (global)
      add_code_glv(curr_fn,the_var->base,calc_size(the_var->array));
    else
      add_code_llv(curr_fn,the_var->base,calc_size(the_var->array));
  } else {
    /* Subscript syntax - check if it's an array or mapping
     * NOTE: Mappings and arrays are DIFFERENT data structures:
     * - Arrays: contiguous memory, integer indices only
     * - Mappings: hash tables, arbitrary key types (string/int/object)
     */
    if (the_var->is_mapping) {
      /* MAPPING: Allow subscript, push 0 as declared_size to signal mapping */
      add_code_integer(curr_fn,the_var->base);
      if (parse_exp(file_info,curr_fn,loc_sym,0,0))
        return file_info->phys_line;
      get_token(file_info,&token);
      if (token.type!=RARRAY_TOK) {
        set_c_err_msg("expected ]");
        return file_info->phys_line;
      }
      add_code_integer(curr_fn,0);  /* 0 signals mapping to runtime */
      if (global)
        add_code_gr(curr_fn);
      else
        add_code_lr(curr_fn);
    } else {
      /* ARRAY: Use existing array logic */
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
  if (token.type==LARRASGN_TOK) {
    /* Array literal: ({ expr1, expr2, ... }) */
    unsigned int elem_count = 0;
    if (last_was_arg) {
      set_c_err_msg("expected arithmetic operation");
      return file_info->phys_line;
    }
    get_token(file_info,&token);
    /* Handle empty array: ({}) */
    if (token.type==RARRASGN_TOK) {
      add_code_integer(curr_fn,0);  /* Push element count */
      add_code_instr(curr_fn,S_ARRAY_LITERAL);
      last_was_arg=1;
      get_token(file_info,&token);
    } else {
      /* Parse array elements */
      unget_token(file_info,&token);
      do {
        /* Parse with precedence 1 to stop at commas (precedence 0) */
        if (parse_exp(file_info,curr_fn,loc_sym,1,0))
          return file_info->phys_line;
        elem_count++;
        get_token(file_info,&token);
      } while (token.type==COMMA_TOK);
      if (token.type!=RARRASGN_TOK) {
        set_c_err_msg("expected }) to close array literal");
        return file_info->phys_line;
      }
      add_code_integer(curr_fn,elem_count);  /* Push element count */
      add_code_instr(curr_fn,S_ARRAY_LITERAL);
      last_was_arg=1;
      get_token(file_info,&token);
    }
  }
  if (token.type==LMAPSGN_TOK) {
    /* Mapping literal: ([ key: value, key2: value2, ... ]) */
    unsigned int pair_count = 0;
    if (last_was_arg) {
      set_c_err_msg("expected arithmetic operation");
      return file_info->phys_line;
    }
    get_token(file_info,&token);
    /* Handle empty mapping: ([ ]) */
    if (token.type==RARRAY_TOK) {
      get_token(file_info,&token);
      if (token.type!=RPAR_TOK) {
        set_c_err_msg("expected ) after ] in empty mapping literal");
        return file_info->phys_line;
      }
      add_code_integer(curr_fn,0);  /* Push pair count */
      add_code_instr(curr_fn,S_MAPPING_LITERAL);
      last_was_arg=1;
      get_token(file_info,&token);
    } else {
      /* Parse key-value pairs - EXACTLY like array elements but with : separator */
      unget_token(file_info,&token);
      do {
        /* Parse key with precedence 1 to stop at commas (precedence 0) */
        if (parse_exp(file_info,curr_fn,loc_sym,1,0))
          return file_info->phys_line;
        get_token(file_info,&token);
        if (token.type!=COLON_TOK) {
          set_c_err_msg("expected : after mapping key");
          return file_info->phys_line;
        }
        /* Parse value with precedence 1 to stop at commas (precedence 0) */
        if (parse_exp(file_info,curr_fn,loc_sym,1,0))
          return file_info->phys_line;
        pair_count++;
        get_token(file_info,&token);
      } while (token.type==COMMA_TOK);
      if (token.type!=RARRAY_TOK) {
        set_c_err_msg("expected ] to close mapping literal");
        return file_info->phys_line;
      }
      get_token(file_info,&token);
      if (token.type!=RPAR_TOK) {
        set_c_err_msg("expected ) after ] in mapping literal");
        return file_info->phys_line;
      }
      add_code_integer(curr_fn,pair_count);  /* Push pair count */
      add_code_instr(curr_fn,S_MAPPING_LITERAL);
      last_was_arg=1;
      get_token(file_info,&token);
    }
  }
  if (token.type==NAME_TOK) {
    strcpy(name,token.token_data.name);
    if (last_was_arg) {
      set_c_err_msg("expected arithmetic operation");
      return file_info->phys_line;
    }
    get_token(file_info,&token);
    
    /* Check for name::function() syntax */
    if (token.type == SECOND_TOK) {
      /* Named parent call: name::function() */
      char *parent_name = copy_string(name);
      
      get_token(file_info, &token);
      if (token.type != NAME_TOK) {
        set_c_err_msg("expected function name after ::");
        return file_info->phys_line;
      }
      
      char *func_name = copy_string(token.token_data.name);
      
      get_token(file_info, &token);
      if (token.type != LPAR_TOK) {
        set_c_err_msg("expected ( after function name");
        return file_info->phys_line;
      }
      
      /* Parse arguments */
      if (parse_arglist(file_info, curr_fn, loc_sym))
        return file_info->phys_line;
      
      /* Task 4: Alias-based resolution for Alias::function() */
      /* Find the inherit entry by alias */
      struct inherit_list *target_inherit = NULL;
      int target_inherit_idx = -1;
      {
        struct inherit_list *inh = file_info->curr_code->inherits;
        int idx = 0;
        while (inh) {
          if (inh->entry && inh->entry->alias && 
              strcmp(inh->entry->alias, parent_name) == 0) {
            target_inherit = inh;
            target_inherit_idx = idx;
            break;
          }
          idx++;
          inh = inh->next;
        }
      }
      
      if (!target_inherit) {
        char errbuf[256];
        sprintf(errbuf, "no parent with alias '%s' found", parent_name);
        set_c_err_msg(errbuf);
        return file_info->phys_line;
      }
      
      /* Find the function in the target parent */
      struct fns *target_func = find_function_in_proto(func_name, target_inherit->entry->proto);
      
      if (!target_func) {
        char errbuf[256];
        sprintf(errbuf, "function '%s' not found in parent '%s'", func_name, parent_name);
        set_c_err_msg(errbuf);
        return file_info->phys_line;
      }
      
      /* In LPC, static functions ARE accessible via Alias::function() from children
       * Only truly private (not static) functions should be blocked
       * For now, allow all functions since we're using static for everything
       */
      
      /* Emit CALL_PARENT_NAMED with (inherit_idx, func_idx) */
      {
        unsigned long x = curr_fn->num_code;
        make_new(curr_fn);
        curr_fn->code[x].type = CALL_PARENT_NAMED;
        curr_fn->code[x].value.parent_call.inherit_idx = target_inherit_idx;
        curr_fn->code[x].value.parent_call.func_idx = target_func->func_index;
        
        // char logbuf[256];
        // sprintf(logbuf, "Debug>> Resolved %s::%s() to inherit[%d].func[%d]", 
        //         parent_name, func_name, target_inherit_idx, target_func->func_index);
        // logger(LOG_DEBUG, logbuf);
      }
      
      last_was_arg = 1;
      get_token(file_info, &token);
    } else if (token.type==LPAR_TOK) {
      if (parse_arglist(file_info,curr_fn,loc_sym))
        return file_info->phys_line;
      if ((func=find_func(file_info->curr_code->func_list,name)))
        add_code_func_call(curr_fn,func);
      else
        if ((instr=find_syscall(name)))
          add_code_instr(curr_fn,instr);
        else
          add_code_func_name(curr_fn,copy_string(name));
      last_was_arg=1;
      get_token(file_info,&token);
    } else {
      unget_token(file_info,&token);
      if (parse_var(name,file_info,curr_fn,loc_sym))
        return file_info->phys_line;
      last_was_arg=1;
      get_token(file_info,&token);
    }
  }
  if (token.type==SECOND_TOK) {
    /* ::function_name() - call next-up in MRO */
    get_token(file_info, &token);
    if (token.type != NAME_TOK) {
      set_c_err_msg("expected function name after ::");
      return file_info->phys_line;
    }
    
    char *func_name = copy_string(token.token_data.name);
    
    /* Debug: Check if we have inherits */
    {
        char logbuf[512];
        int inherit_count = 0;
        struct inherit_list *check = file_info->curr_code->inherits;
        while (check) {
            inherit_count++;
            check = check->next;
        }
        sprintf(logbuf, "Debug>> ::function() for '%s': current file has %d inherits", 
                func_name, inherit_count);
        logger(LOG_DEBUG, logbuf);
    }
    
    /* Compute MRO for current program */
    int mro_length;
    struct proto **mro = compute_mro(file_info->curr_code, &mro_length);
    
    {
        char logbuf[512];
        sprintf(logbuf, "Debug>> ::function() MRO for '%s': found %d protos in chain", 
                func_name, mro_length);
        logger(LOG_DEBUG, logbuf);
        for (int i = 0; i < mro_length; i++) {
            sprintf(logbuf, "Debug>>   MRO[%d]: %s", i, mro[i] ? mro[i]->pathname : "NULL");
            logger(LOG_DEBUG, logbuf);
        }
    }
    
    /* Search for next definition up the chain */
    struct fns *next_func = NULL;
    struct proto *next_proto = NULL;
    int next_inherit_idx = -1;
    
    for (int i = 0; i < mro_length; i++) {
        struct proto *candidate = mro[i];
        struct fns *candidate_func = find_function_in_proto(func_name, candidate);
        
        {
            char logbuf[512];
            sprintf(logbuf, "Debug>>   Checking MRO[%d] (%s) for '%s': %s", 
                    i, candidate ? candidate->pathname : "NULL", func_name,
                    candidate_func ? "FOUND" : "NOT FOUND");
            logger(LOG_DEBUG, logbuf);
            
            if (!candidate_func && candidate && candidate->funcs) {
                // List all functions in this proto
                char logbuf[256];
                struct fns *list_fn = candidate->funcs->func_list;
                int fn_count = 0;
                while (list_fn) {
                    sprintf(logbuf, "Debug>>     Available: %s (static=%d, vis=%d)", 
                            list_fn->funcname, list_fn->is_static, list_fn->visibility);
                    logger(LOG_DEBUG, logbuf);
                    fn_count++;
                    list_fn = list_fn->next;
                }
                sprintf(logbuf, "Debug>>     Total functions in proto: %d", fn_count);
                logger(LOG_DEBUG, logbuf);
            }
        }
        
        if (candidate_func) {
            /* In LPC, static functions ARE accessible via :: from children
             * Only truly private (not static) functions should be skipped
             * For now, allow all functions via :: since we're using static for everything
             */
            char logbuf[256];
            sprintf(logbuf, "Debug>>   Function '%s' found with visibility=%d, using it!", 
                    func_name, candidate_func->visibility);
            logger(LOG_DEBUG, logbuf);
            
            next_func = candidate_func;
            next_proto = candidate;
            
            /* Find inherit_idx for this proto */
            struct inherit_list *inh = file_info->curr_code->inherits;
            int idx = 0;
            while (inh) {
                if (inh->entry && inh->entry->proto == next_proto) {
                    next_inherit_idx = idx;
                    break;
                }
                idx++;
                inh = inh->next;
            }
            
            /* CRITICAL: Find the function's ACTUAL index in the parent's function table
             * We cannot use candidate_func->func_index because that might be from a different context.
             * We must walk the parent proto's function list to find the real index.
             */
            struct fns *parent_fn = next_proto->funcs->func_list;
            int parent_idx = 0;
            int found_idx = -1;
            while (parent_fn) {
                if (parent_fn == candidate_func) {
                    found_idx = parent_idx;
                    sprintf(logbuf, "Debug>>   Found '%s' at ACTUAL parent index %d (func_index field was %d)", 
                            func_name, parent_idx, candidate_func->func_index);
                    logger(LOG_DEBUG, logbuf);
                    break;
                }
                parent_idx++;
                parent_fn = parent_fn->next;
            }
            
            if (found_idx < 0) {
                sprintf(logbuf, "INTERNAL ERROR: Function '%s' found but not in parent's function list!", func_name);
                logger(LOG_ERROR, logbuf);
                set_c_err_msg("internal error in :: resolution");
                return file_info->phys_line;
            }
            
            /* Override func_index with the actual parent table index */
            candidate_func->func_index = found_idx;
            
            break;
        }
    }
    
    FREE(mro);
    
    if (!next_func) {
        char errbuf[256];
        sprintf(errbuf, "no super definition for '%s' in inheritance chain", func_name);
        set_c_err_msg(errbuf);
        return file_info->phys_line;
    }
    
    /* Expect argument list */
    get_token(file_info, &token);
    if (token.type != LPAR_TOK) {
      set_c_err_msg("expected ( after function name");
      return file_info->phys_line;
    }
    
    /* Parse arguments */
    if (parse_arglist(file_info, curr_fn, loc_sym))
      return file_info->phys_line;
    
    /* Emit CALL_SUPER with (inherit_idx, func_idx) */
    {
        unsigned long x = curr_fn->num_code;
        make_new(curr_fn);
        curr_fn->code[x].type = CALL_SUPER;
        curr_fn->code[x].value.parent_call.inherit_idx = next_inherit_idx;
        curr_fn->code[x].value.parent_call.func_idx = next_func->func_index;
        
        char logbuf[256];
        sprintf(logbuf, "Emitted CALL_SUPER for %s: inherit_idx=%d, func_idx=%d", 
                func_name, next_inherit_idx, next_func->func_index);
        logger(LOG_DEBUG, logbuf);
    }
    
    last_was_arg = 1;
    get_token(file_info, &token);
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
    case MAPPING_TOK:
      {
        int is_mapping_decl = (token.type == MAPPING_TOK);  /* Save for all vars in list */
        done=0;
        while (!done) {
          if (add_var(file_info,loc_sym,is_mapping_decl))
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

/* Helper for compute_unique_linearization - recursively add proto and ancestors */
static void add_unique_proto(struct proto *p, struct proto **order, int *num) {
    char logbuf[256];
    
    if (!p) return;
    
    /* Check if already added */
    for (int i = 0; i < *num; i++) {
        if (order[i] == p) {
            // sprintf(logbuf, "linearize: proto '%s' (ptr=%p) already in list at position %d, skipping", 
            //         p->pathname ? p->pathname : "NULL", (void*)p, i);
            // logger(LOG_DEBUG, logbuf);
            return;  /* Already in list */
        }
    }
    
    /* Add parent's ancestors first (depth-first, base-first order) */
    if (p->funcs && p->funcs->inherits) {
        struct inherit_list *inh = p->funcs->inherits;
        while (inh) {
            add_unique_proto(inh->parent_proto, order, num);
            inh = inh->next;
        }
    }
    
    /* Then add this program */
    // sprintf(logbuf, "linearize: adding proto '%s' at position %d", 
    //         p->pathname ? p->pathname : "NULL", *num);
    // logger(LOG_DEBUG, logbuf);
    order[(*num)++] = p;
}

/* Compute unique linearization of ancestor programs (base → child order)
 * Returns array of unique proto pointers, sets count
 * Caller must FREE the returned array
 */
struct proto **compute_unique_linearization(struct code *current_code, int *count) {
    struct proto **order = MALLOC(sizeof(struct proto*) * 32);  /* Max depth */
    int num = 0;
    char logbuf[256];
    
    // sprintf(logbuf, "linearize: starting linearization for current_code=%p", (void*)current_code);
    // logger(LOG_DEBUG, logbuf);
    
    /* Process all direct inherits */
    struct inherit_list *curr = current_code->inherits;
    int inherit_num = 0;
    while (curr) {
        // sprintf(logbuf, "linearize: processing inherit[%d] '%s' (proto=%p)", 
        //         inherit_num++, curr->inherit_path ? curr->inherit_path : "NULL", 
        //         (void*)curr->parent_proto);
        // logger(LOG_DEBUG, logbuf);
        add_unique_proto(curr->parent_proto, order, &num);
        curr = curr->next;
    }
    
    *count = num;
    // sprintf(logbuf, "linearize: completed with %d unique protos", num);
    // logger(LOG_DEBUG, logbuf);
    return order;
}

/* Build proper LPC variable layout from unique linearization
 * Called after all inherit statements are parsed
 * Returns 0 on success, 1 on failure
 */
int build_variable_layout(filptr *file_info) {
    int order_count;
    struct proto **order;
    char logbuf[512];
    
    sprintf(logbuf, "build_variable_layout: starting for file with %u existing globals", 
            file_info->curr_code->num_globals);
    logger(LOG_DEBUG, logbuf);
    
    /* Log child's own variables before merging parents */
    /* COMMENTED OUT - Enable for inheritance debugging
    if (file_info->glob_sym && file_info->glob_sym->varlist) {
        struct var_tab *v = file_info->glob_sym->varlist;
        int child_var_count = 0;
        while (v) {
            sprintf(logbuf, "build_variable_layout: child already has var '%s' at slot %u", 
                    v->name, v->base);
            logger(LOG_DEBUG, logbuf);
            child_var_count++;
            v = v->next;
        }
        sprintf(logbuf, "build_variable_layout: child has %d own variables before merge", 
                child_var_count);
        logger(LOG_DEBUG, logbuf);
    }
    */
    
    /* Compute unique linearization */
    order = compute_unique_linearization(file_info->curr_code, &order_count);
    
    /* Track which programs we've merged and their var_offset */
    struct {
        struct proto *prog;
        unsigned short var_offset;
    } merged[32];
    int merged_count = 0;
    
    /* Inherited-first layout: start inherited variables at slot 0 */
    unsigned short next_slot = 0;
    
    // sprintf(logbuf, "build_variable_layout: processing %d unique protos, starting at slot %u", 
    //         order_count, next_slot);
    // logger(LOG_DEBUG, logbuf);
    
    /* Process each unique program in order */
    for (int i = 0; i < order_count; i++) {
        struct proto *prog = order[i];
        
        // sprintf(logbuf, "build_variable_layout: examining proto[%d] = '%s'", 
        //         i, prog->pathname ? prog->pathname : "NULL");
        // logger(LOG_DEBUG, logbuf);
        
        /* Check if already merged */
        int already_merged = 0;
        unsigned short existing_offset = 0;
        for (int j = 0; j < merged_count; j++) {
            if (merged[j].prog == prog) {
                already_merged = 1;
                existing_offset = merged[j].var_offset;
                // sprintf(logbuf, "build_variable_layout: proto '%s' already merged at offset %u", 
                //         prog->pathname, existing_offset);
                // logger(LOG_DEBUG, logbuf);
                break;
            }
        }
        
        if (!already_merged) {
            /* Record this program's var_offset */
            merged[merged_count].prog = prog;
            merged[merged_count].var_offset = next_slot;
            
            // sprintf(logbuf, "build_variable_layout: merging proto '%s' at offset %u", 
            //         prog->pathname, next_slot);
            // logger(LOG_DEBUG, logbuf);
            
            /* Copy only THIS program's OWN defined variables (not inherited ones) */
            struct var_tab *var = prog->funcs ? prog->funcs->own_vars : NULL;
            int var_count = 0;
            
            // sprintf(logbuf, "build_variable_layout: using own_vars from proto '%s' (ptr=%p)", 
            //         prog->pathname, (void*)var);
            // logger(LOG_DEBUG, logbuf);
            
            while (var) {
                // sprintf(logbuf, "build_variable_layout: examining var '%s' from proto '%s'", 
                //         var->name, prog->pathname);
                // logger(LOG_DEBUG, logbuf);
                
                /* Check if variable name already exists in child */
                struct var_tab *existing = file_info->glob_sym ? 
                                          file_info->glob_sym->varlist : NULL;
                int name_exists = 0;
                struct proto *existing_origin = NULL;
                
                while (existing) {
                    if (!strcmp(existing->name, var->name)) {
                        name_exists = 1;
                        existing_origin = existing->origin_prog;
                        
                        /* Check if from same origin (OK) or different origin (ERROR) */
                        if (existing_origin && existing_origin != prog) {
                            char errbuf[512];
                            sprintf(errbuf, "variable '%s' defined in both '%s' and '%s'", 
                                    var->name, 
                                    existing_origin->pathname ? existing_origin->pathname : "unknown",
                                    prog->pathname ? prog->pathname : "unknown");
                            set_c_err_msg(errbuf);
                            // sprintf(logbuf, "build_variable_layout: ERROR - %s", errbuf);
                            // logger(LOG_ERROR, logbuf);
                            FREE(order);
                            return 1;  /* Compile error */
                        }
                        
                        // sprintf(logbuf, "build_variable_layout: var '%s' already exists from same origin '%s', skipping", 
                        //         var->name, prog->pathname ? prog->pathname : "unknown");
                        // logger(LOG_DEBUG, logbuf);
                        break;
                    }
                    existing = existing->next;
                }
                
                if (!name_exists) {
                    /* Add variable to child's table */
                    struct var_tab *new_var = MALLOC(sizeof(struct var_tab));
                    new_var->name = copy_string(var->name);
                    new_var->base = next_slot++;
                    new_var->origin_prog = prog;  /* Track which program defined this variable */
                    new_var->owner_local_index = var->owner_local_index; /* Copy owner's local index */
                    
                    /* Deep copy array size info if present */
                    if (var->array) {
                        struct array_size *parent_arr = var->array;
                        struct array_size *new_arr = NULL;
                        struct array_size **arr_tail = &new_arr;
                        
                        while (parent_arr) {
                            struct array_size *copy = MALLOC(sizeof(struct array_size));
                            copy->size = parent_arr->size;
                            copy->next = NULL;
                            *arr_tail = copy;
                            arr_tail = &copy->next;
                            parent_arr = parent_arr->next;
                        }
                        new_var->array = new_arr;
                    } else {
                        new_var->array = NULL;
                    }
                    
                    new_var->is_mapping = var->is_mapping;
                    new_var->next = file_info->glob_sym ? file_info->glob_sym->varlist : NULL;
                    
                    if (file_info->glob_sym) {
                        file_info->glob_sym->varlist = new_var;
                        file_info->glob_sym->num++;
                    } else {
                        file_info->glob_sym = MALLOC(sizeof(sym_tab_t));
                        file_info->glob_sym->num = 1;
                        file_info->glob_sym->varlist = new_var;
                    }
                    file_info->curr_code->num_globals++;
                    var_count++;
                    
                    // sprintf(logbuf, "build_variable_layout: added var '%s' at slot %u", 
                    //         new_var->name, new_var->base);
                    // logger(LOG_DEBUG, logbuf);
                }
                
                var = var->next;
            }
            
            // sprintf(logbuf, "build_variable_layout: merged %d variables from proto '%s'", 
            //         var_count, prog->pathname);
            // logger(LOG_DEBUG, logbuf);
            
            merged_count++;
        }
    }
    
    /* Attach var_offset to all inherit entries */
    sprintf(logbuf, "build_variable_layout: attaching var_offsets to inherit entries");
    logger(LOG_DEBUG, logbuf);
    
    struct inherit_list *inh = file_info->curr_code->inherits;
    while (inh) {
        for (int i = 0; i < merged_count; i++) {
            if (merged[i].prog == inh->parent_proto) {
                inh->entry->var_offset = merged[i].var_offset;
                sprintf(logbuf, "build_variable_layout: INHERIT ALIAS '%s' maps to prog '%s' with var_offset=%u", 
                        inh->entry->alias,
                        merged[i].prog->pathname ? merged[i].prog->pathname : "unknown",
                        merged[i].var_offset);
                logger(LOG_DEBUG, logbuf);
                break;
            }
        }
        inh = inh->next;
    }
    
    FREE(order);
    
    {
        struct code *child_code = file_info->curr_code;
        if (merged_count > child_code->ancestor_capacity) {
            unsigned short new_capacity = merged_count;
            struct ancestor_var_offset *new_map;
            new_map = MALLOC(sizeof(struct ancestor_var_offset) * new_capacity);
            if (!new_map) {
                set_c_err_msg("out of memory allocating ancestor map");
                return 1;
            }
            if (child_code->ancestor_map && child_code->ancestor_count) {
                for (unsigned short i = 0; i < child_code->ancestor_count && i < new_capacity; i++) {
                    new_map[i] = child_code->ancestor_map[i];
                }
            }
            if (child_code->ancestor_map) {
                FREE(child_code->ancestor_map);
            }
            child_code->ancestor_map = new_map;
            child_code->ancestor_capacity = new_capacity;
        }
        child_code->ancestor_count = merged_count;
        for (int i = 0; i < merged_count; i++) {
            child_code->ancestor_map[i].proto = merged[i].prog;
            child_code->ancestor_map[i].var_offset = merged[i].var_offset;
            sprintf(logbuf, "build_variable_layout: ancestor map '%s' -> offset %u",
                    merged[i].prog->pathname ? merged[i].prog->pathname : "unknown",
                    merged[i].var_offset);
            logger(LOG_DEBUG, logbuf);
        }
        /* Child-defined globals start after all inherited variables */
        child_code->self_var_offset = next_slot;
    }
    
    // sprintf(logbuf, "build_variable_layout: completed, final num_globals=%u", 
    //         file_info->curr_code->num_globals);
    // logger(LOG_DEBUG, logbuf);
    
    /* Log final variable table */
    /* COMMENTED OUT - Enable for inheritance debugging
    sprintf(logbuf, "build_variable_layout: FINAL VARIABLE TABLE:");
    logger(LOG_DEBUG, logbuf);
    struct var_tab *final_var = file_info->glob_sym ? file_info->glob_sym->varlist : NULL;
    int max_slot = -1;
    while (final_var) {
        sprintf(logbuf, "  slot=%u, name='%s', origin='%s'", 
                final_var->base, 
                final_var->name,
                final_var->origin_prog ? 
                    (final_var->origin_prog->pathname ? final_var->origin_prog->pathname : "unknown") : 
                    "current");
        logger(LOG_DEBUG, logbuf);
        if ((int)final_var->base > max_slot) {
            max_slot = (int)final_var->base;
        }
        final_var = final_var->next;
    }
    
    // Assert num_globals == max(slot_index)+1
    if (max_slot >= 0 && file_info->curr_code->num_globals != (unsigned int)(max_slot + 1)) {
        sprintf(logbuf, "build_variable_layout: WARNING - num_globals=%u but max_slot=%d (expected %d)", 
                file_info->curr_code->num_globals, max_slot, max_slot + 1);
        logger(LOG_ERROR, logbuf);
    }
    */
    
    return 0;
}

/* Process inherit statement - load parent proto and add to chain
 * Returns 0 on success, 1 on failure
 */
int add_inherit(filptr *file_info, char *pathname) {
    struct inherit_list *new_inherit;
    struct proto *parent_proto;
    struct code *parent_code;
    struct object *parent_obj;
    unsigned int result;
    char logbuf[256];
    
    sprintf(logbuf, "add_inherit: loading '%s'", pathname);
    logger(LOG_DEBUG, logbuf);
    
    /* Check if already inherited in current file */
    struct inherit_list *curr = file_info->curr_code->inherits;
    while (curr) {
        if (!strcmp(curr->inherit_path, pathname)) {
            sprintf(logbuf, "add_inherit: '%s' already inherited in current file", pathname);
            logger(LOG_WARNING, logbuf);
            return 0;  /* Not an error, just skip */
        }
        curr = curr->next;
    }
    
    /* Check global proto cache first */
    parent_proto = find_cached_proto(pathname);
    
    if (parent_proto) {
        sprintf(logbuf, "add_inherit: using cached proto for '%s'", pathname);
        logger(LOG_DEBUG, logbuf);
        
        /* Validate cached proto */
        if (!parent_proto->funcs) {
            sprintf(logbuf, "add_inherit: ERROR - cached proto for '%s' has NULL funcs", pathname);
            logger(LOG_ERROR, logbuf);
            set_c_err_msg("cached proto has no function table");
            return 1;
        }
        if (!parent_proto->pathname) {
            sprintf(logbuf, "add_inherit: ERROR - cached proto for '%s' has NULL pathname", pathname);
            logger(LOG_ERROR, logbuf);
            set_c_err_msg("cached proto has no pathname");
            return 1;
        }
    } else {
        /* Compile once and cache */
        sprintf(logbuf, "add_inherit: compiling and caching '%s'", pathname);
        logger(LOG_DEBUG, logbuf);
        
        result = parse_code(pathname, NULL, &parent_code);
        if (result) {
            char errbuf[512];
            sprintf(logbuf, "add_inherit: failed to compile '%s'", pathname);
            logger(LOG_ERROR, logbuf);
            sprintf(errbuf, "failed to compile inherited file '%s'", pathname);
            set_c_err_msg(errbuf);
            return 1;
        }
        
        /* Create proto for parent */
        parent_proto = MALLOC(sizeof(struct proto));
        parent_proto->pathname = copy_string(pathname);
        parent_proto->funcs = parent_code;
        parent_proto->proto_obj = NULL;  /* No instance yet */
        parent_proto->next_proto = NULL;
        parent_proto->inherits = parent_code->inherits;  /* Copy inherits from code */
        
        /* Set origin_proto for all functions in this proto */
        {
            struct fns *curr_fn = parent_code->func_list;
            int count = 0;
            while (curr_fn) {
                sprintf(logbuf, "add_inherit: Setting origin_proto for func '%s' to proto '%s' (%p)",
                        curr_fn->funcname ? curr_fn->funcname : "unknown",
                        parent_proto->pathname ? parent_proto->pathname : "unknown",
                        (void*)parent_proto);
                logger(LOG_DEBUG, logbuf);
                curr_fn->origin_proto = parent_proto;
                curr_fn = curr_fn->next;
                count++;
            }
            sprintf(logbuf, "add_inherit: Set origin_proto for %d functions in '%s'", 
                    count, parent_proto->pathname ? parent_proto->pathname : "unknown");
            logger(LOG_DEBUG, logbuf);
        }
        
        /* Add to proto list (link to boot object's proto chain) */
        struct object *boot_obj = ref_to_obj(0);
        if (boot_obj && boot_obj->parent) {
            parent_proto->next_proto = boot_obj->parent->next_proto;
            boot_obj->parent->next_proto = parent_proto;
        }
        
        /* Cache it globally */
        cache_proto(pathname, parent_proto);
    }
    
    /* Create inherit entry with alias */
    struct inherit_entry *entry = MALLOC(sizeof(struct inherit_entry));
    entry->alias = get_default_alias(pathname);  /* Extract basename as default alias */
    entry->canon_path = copy_string(pathname);   /* TODO: Add path canonicalization */
    entry->proto = parent_proto;
    entry->func_offset = 0;  /* Will be set later if needed */
    entry->var_offset = 0;   /* Will be set later if needed */
    
    {
        char logbuf[256];
        sprintf(logbuf, "add_inherit: created entry with alias '%s' for path '%s'", 
                entry->alias, pathname);
        logger(LOG_DEBUG, logbuf);
    }
    
    /* Add to current file's inheritance list */
    new_inherit = MALLOC(sizeof(struct inherit_list));
    new_inherit->entry = entry;
    new_inherit->parent_proto = parent_proto;  /* Keep for compatibility */
    new_inherit->inherit_path = copy_string(pathname);  /* Keep for compatibility */
    new_inherit->next = file_info->curr_code->inherits;
    file_info->curr_code->inherits = new_inherit;
    
    /* Variable copying is now deferred to build_variable_layout() 
     * which is called after all inherits are parsed */
    sprintf(logbuf, "add_inherit: successfully added '%s' (variable layout deferred)", 
            new_inherit->inherit_path);
    logger(LOG_DEBUG, logbuf);
    
    return 0;
}

unsigned int top_level_parse(filptr *file_info)
{
  token_t token;
  sym_tab_t loc_sym;
  int done, is_static;
  fn_t curr_func;
  struct fns *tmp_fns;
  char logbuf[256];
  
  /* Initialize layout_locked flag for this file */
  file_info->layout_locked = 0;

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
      case MAPPING_TOK:
        /* BARRIER: First non-inherit token - build variable layout now */
        if (!file_info->layout_locked) {
          if (file_info->curr_code->inherits) {
            sprintf(logbuf, "top_level_parse: BARRIER hit at VAR/MAPPING, building variable layout");
            logger(LOG_DEBUG, logbuf);
            
            if (build_variable_layout(file_info)) {
              return file_info->phys_line;
            }
            
            sprintf(logbuf, "top_level_parse: layout locked, inherit phase ended at slot count %u", 
                    file_info->curr_code->num_globals);
            logger(LOG_DEBUG, logbuf);
          }
          /* Lock layout regardless of whether we have inherits - no more inherits allowed after this point */
          file_info->layout_locked = 1;
        }
        
        {
          int is_mapping_decl = (token.type == MAPPING_TOK);  /* Save for all vars in list */
          while (!done) {
            if (add_var(file_info,file_info->glob_sym,is_mapping_decl))
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
        }
        break;
      case INHERIT_TOK:
        /* Check if inherit comes after layout was built (error) */
        if (file_info->layout_locked) {
          set_c_err_msg("inherit must appear before all variable and function declarations");
          return file_info->phys_line;
        }
        
        /* inherit "/path/to/file"; or inherit MACRO; */
        get_token(file_info, &token);
        
        /* If it's a NAME_TOK, it might be a macro - try to expand it */
        if (token.type == NAME_TOK) {
          struct define *def;
          
          /* Look for macro definition */
          def = file_info->defs;
          while (def) {
            if (!strcmp(def->name, token.token_data.name)) {
              /* Found macro - expand it to STRING_TOK */
              token.type = STRING_TOK;
              token.token_data.name = copy_string(def->definition);
              break;
            }
            def = def->next;
          }
        }
        
        if (token.type != STRING_TOK) {
          set_c_err_msg("expected string after inherit");
          return file_info->phys_line;
        }
        
        /* Copy the path string before it gets overwritten by subsequent tokens */
        char *inherit_path = copy_string(token.token_data.name);
        
        {
          char logbuf[256];
          sprintf(logbuf, "Parser: about to add_inherit('%s') at line %d", inherit_path, file_info->phys_line);
          logger(LOG_DEBUG, logbuf);
        }
        
        /* Add to inheritance list */
        if (add_inherit(file_info, inherit_path)) {
          char errbuf[512];
          sprintf(errbuf, "failed to load inherited file '%s'", inherit_path);
          set_c_err_msg(errbuf);
          return file_info->phys_line;
        }
        
        {
          char logbuf[256];
          sprintf(logbuf, "Parser: add_inherit returned, getting next token");
          logger(LOG_DEBUG, logbuf);
        }
        
        get_token(file_info, &token);
        
        {
          char logbuf[256];
          sprintf(logbuf, "Parser: after inherit, got token type %d (SEMI_TOK=%d)", token.type, SEMI_TOK);
          logger(LOG_DEBUG, logbuf);
          if (token.type == NAME_TOK && token.token_data.name) {
            sprintf(logbuf, "Parser: token is NAME_TOK with value '%s'", token.token_data.name);
            logger(LOG_DEBUG, logbuf);
          }
        }
        
        if (token.type != SEMI_TOK) {
          char errbuf[256];
          sprintf(errbuf, "expected ; after inherit, got token type %d (SEMI_TOK=%d)", token.type, SEMI_TOK);
          set_c_err_msg(errbuf);
          return file_info->phys_line;
        }
        break;
      case NAME_TOK:
        /* BARRIER: First function definition - build variable layout now */
        if (!file_info->layout_locked) {
          if (file_info->curr_code->inherits) {
            sprintf(logbuf, "top_level_parse: BARRIER hit at NAME_TOK (function), building variable layout");
            logger(LOG_DEBUG, logbuf);
            
            if (build_variable_layout(file_info)) {
              return file_info->phys_line;
            }
            
            sprintf(logbuf, "top_level_parse: layout locked, inherit phase ended at slot count %u", 
                    file_info->curr_code->num_globals);
            logger(LOG_DEBUG, logbuf);
          }
          /* Lock layout regardless of whether we have inherits - no more inherits allowed after this point */
          file_info->layout_locked = 1;
        }
        
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
        
        /* Assign function index - count existing functions */
        {
            int func_count = 0;
            struct fns *count_fn = file_info->curr_code->func_list;
            while (count_fn) {
                func_count++;
                count_fn = count_fn->next;
            }
            tmp_fns->func_index = func_count;
        }
        
        tmp_fns->visibility=is_static ? VISIBILITY_PRIVATE : VISIBILITY_PUBLIC;
        tmp_fns->origin_proto=NULL;  /* Will be set after proto is created */
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
            int is_mapping_param = (token.type==MAPPING_TOK);
            if (token.type!=VAR_DCL_TOK && token.type!=MAPPING_TOK)
              unget_token(file_info,&token);
            if (add_var(file_info,&loc_sym,is_mapping_param)) {
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
