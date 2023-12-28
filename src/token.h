/* token.h */

/* contains the declarations for token structures, etc */

#define MAX_TOK_LEN 31
#define FN_BLK 256

/* Token Types */

#define NO_TOK            38
#define NAME_TOK          39
#define STRING_TOK        40
#define INTEGER_TOK       41
#define EOF_TOK           42    /* eof has been received */
#define VAR_DCL_TOK       43    /* int, string, object */
#define STATIC_TOK        44    /* static */
#define COMMA_TOK          0    /* , */
#define SEMI_TOK          45    /* ; */
#define LBRACK_TOK        46    /* { */
#define RBRACK_TOK        47    /* } */
#define LPAR_TOK          48    /* ( */
#define RPAR_TOK          49    /* ) */
#define IF_TOK            50    /* if */
#define ELSE_TOK          51    /* else */
#define WHILE_TOK         52    /* while */
#define FOR_TOK           53    /* for */
#define DO_TOK            54    /* do */
#define COLON_TOK         55    /* : */
#define LARRAY_TOK        56    /* [ */
#define RARRAY_TOK        57    /* ] */
#define SECOND_TOK        58    /* :: */
#define RETURN_TOK        59    /* return */
#define DOT_TOK           60    /* . */
#define CALL_TOK          61    /* -> */
#define LARRASGN_TOK      62    /* ({*/
#define RARRASGN_TOK      63    /* }) */

/* Data structure declarations */

typedef struct
{
  unsigned char type;
  union
  {
    signed long integer;
    char *name;
  } token_data;
} token_t;

struct file_stack
{
  FILE *file_ptr;
  struct file_stack *previous;
};

typedef struct
{
  unsigned int num;
  struct var_tab *varlist;
} sym_tab_t;

struct parm
{
  char *name;
  char *exp;
  struct parm *next;
};

struct define
{
  char *name;
  char *definition;
  int has_paren;
  struct parm *params;
  struct define *next;
};

typedef struct
{
  FILE *curr_file;
  struct file_stack *previous;        /* previously opened files */
  token_t put_back_token;             /* the put-back token */
  int is_put_back;
  char *expanded;                     /* expanded #defines, etc */
  unsigned int phys_line;             /* physical line */
  struct code *curr_code;             /* code */
  sym_tab_t *glob_sym;                /* global symbol table */
  struct define *defs;                /* #defines */
  int depth;
} filptr;

typedef struct
{
  struct var *code;
  unsigned long num_code;
  unsigned long num_alloc;
} fn_t;

/* Function prototypes */

struct fns *find_func(struct fns *curr_fn, char *name);
unsigned char find_syscall(char *name);
struct var_tab *find_var(char *name, sym_tab_t *sym);

void free_file_stack(filptr *file_info);
void free_sym_t(sym_tab_t *sym);
void free_define(filptr *file_info);

unsigned int top_level_parse(filptr *file_info);
unsigned int parse_block(filptr *file_info, fn_t *curr_fn, sym_tab_t *loc_sym);
unsigned int parse_exp(filptr *file_info, fn_t *curr_fn, sym_tab_t *loc_sym,
                       int prec, int last_was_arg);
unsigned int parse_line(filptr *file_info, fn_t *curr_fn, sym_tab_t *loc_sym);
unsigned int parse_arglist(filptr *file_info, fn_t *curr_fn, sym_tab_t
                           *loc_sym);
unsigned int parse_var(char *name,filptr *file_info, fn_t *curr_fn, sym_tab_t
                       *loc_sym);

unsigned int add_var(filptr *file_info, sym_tab_t *sym);

void get_token(filptr *file_info, token_t *token);
void unget_token(filptr *file_info, token_t *token);
int preprocess(filptr *file_info);
void expand_def(struct define *def, char *buf);
void expand(struct define *def, filptr *file_info);
void expand_exp(struct define *def, filptr *file_info);
void set_c_err_msg(char *msg);
