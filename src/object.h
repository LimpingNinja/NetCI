/* object.h */

/* This header file contains the declarations for the object
   structures used in the compiler/interpreter */


/* Here are the types used in the struct var */

#define INTEGER 0                   /* integer */
#define STRING 1                    /* string */
#define OBJECT 2                    /* an object pointer string */
#define ASM_INSTR 3                 /* a code instruction to be interpreted */
#define GLOBAL_L_VALUE 4            /* a global variable index */
#define LOCAL_L_VALUE 5             /* a local variable index */
#define FUNC_CALL 6                 /* function call to known function */
#define NUM_ARGS 7                  /* in a function call, the number of args
                                       preceding this var on the stack */
#define ARRAY_SIZE 8                /* the number of preceding var types that
                                       are part of this argument */
#define JUMP 9                      /* Absolute branch */
#define BRANCH 10                   /* Conditional branch */
#define NEW_LINE 11                 /* Clear stack, set new physical line */
#define RETURN 12                   /* end of function call; return value on
                                       stack */
#define LOCAL_REF 13                /* an array reference; preceded by
                                       the size and the base */
#define GLOBAL_REF 14               /* an array reference; preceded by
                                       the size and the base */
#define FUNC_NAME 15                /* function name - should be converted
                                       to FUNC_CALL at run-time */

#define EXTERN_FUNC 16              /* call to external function */
#define ARRAY 17                    /* heap-allocated array pointer */
#define MAPPING 18                  /* heap-allocated mapping pointer */

/* Forward declarations */
struct heap_array;
struct heap_mapping;

/* The lval structure contains information about lvalues */

struct lval
{
  unsigned long ref;   /* Changed to unsigned long to hold pointers on 64-bit systems */
  unsigned int size;
};

/* The var structure contains information about variables and
   object code instructions that may be placed on the stack */

/* Parent call indices for ::function() and Alias::function() */
struct parent_call_indices
{
  unsigned short inherit_idx;  /* Index into program's inherit table */
  unsigned short func_idx;     /* Index into that program's function table */
};

struct var
{
  unsigned char type;
  union
  {
    signed long integer;             /* integer value for vars */
    char *string;                    /* a character string */
    struct object *objptr;           /* pointer to an object */
    unsigned char instruction;       /* an asm instruction */
    struct lval l_value;             /* l-value reference */
    struct fns *func_call;           /* pointer to a function */
    unsigned long num;               /* generic integer value */
    struct heap_array *array_ptr;   /* pointer to heap array */
    struct heap_mapping *mapping_ptr; /* pointer to heap mapping */
    struct parent_call_indices parent_call; /* For CALL_SUPER and CALL_PARENT_NAMED */
  } value;
};

/* Heap-allocated array structure */
#define UNLIMITED_ARRAY_SIZE 0xFFFFFFFF

struct heap_array {
  unsigned int size;        /* Current number of elements (logical size) */
  unsigned int capacity;    /* Allocated capacity (physical size) */
  unsigned int max_size;    /* Maximum size (or UNLIMITED_ARRAY_SIZE) */
  unsigned int refcount;    /* Reference count for garbage collection */
  struct var *elements;     /* Array of elements */
};

/* Heap-allocated mapping structure */
#define DEFAULT_MAPPING_CAPACITY 16
#define MAPPING_LOAD_FACTOR 0.75

struct mapping_entry {
  struct var key;           /* Key (can be string, int, or object) */
  struct var value;         /* Value (any type) */
  unsigned int hash;        /* Cached hash value */
  struct mapping_entry *next; /* For collision chaining */
};

struct heap_mapping {
  unsigned int size;        /* Current number of key-value pairs */
  unsigned int capacity;    /* Allocated capacity (hash table size) */
  unsigned int refcount;    /* Reference count for garbage collection */
  struct mapping_entry **buckets; /* Array of bucket chains */
};

/* the var_stack structure is used as a stack of vars */

struct var_stack
{
  struct var data;
  struct var_stack *next;
};

/* Visibility flags for functions */
#define VISIBILITY_PUBLIC    0
#define VISIBILITY_PROTECTED 1
#define VISIBILITY_PRIVATE   2

/* The fns structure contains information about functions */

struct fns
{
  unsigned char is_static;
  unsigned int num_args;
  unsigned int num_locals;
  struct var *code;           /* To be treated as an array */
  unsigned long num_instr;
  char *funcname;
  struct var_tab *lst;        /* Local symbol table (for array initialization) */
  struct fns *next;
  unsigned short func_index;  /* Position in function table (for indexed calls) */
  unsigned char visibility;   /* VISIBILITY_PUBLIC/PROTECTED/PRIVATE */
  struct proto *origin_proto; /* Which proto defined this function */
};

struct array_size {
  unsigned int size;
  struct array_size *next;
};

struct var_tab {
  char *name;
  unsigned int base;
  struct array_size *array;  /* Used ONLY for arrays, NULL for mappings */
  int is_mapping;  /* 1 if declared with 'mapping' keyword, 0 otherwise */
                   /* NOTE: Mappings are NOT arrays! They use hash tables, not the array field */
  struct proto *origin_prog;  /* Which program defined this variable (NULL = current program) */
  unsigned short owner_local_index; /* Index within the owner program's own var set (for GST mapping) */
  struct var_tab *next;
};

/* The code structure contains information about code, including
   num_refs, which keeps track of how many objects currently
   reference the code */

struct code
{
  unsigned long num_refs;
  unsigned int num_globals;
  struct fns *func_list;
  struct var_tab *gst;              /* all_vars: flattened view for this program's codegen (ancestors + own) */
  struct var_tab *own_vars;         /* own_vars: only variables declared in THIS source file */
  struct inherit_list *inherits;    /* Inheritance chain */
  /* Flattened ancestor offsets shared by all instances using this code */
  struct ancestor_var_offset *ancestor_map;
  unsigned short ancestor_count;
  unsigned short ancestor_capacity;
  unsigned short self_var_offset;
  /* Per-program GST ref mapping: maps definer's gst[ref] -> {owner_proto, owner_local_index} */
  struct gst_ref_entry *gst_map;
  unsigned short gst_count;
};

/* the verb struct is a simple linked list of verbs */

struct verb
{
  char *verb_name;
  unsigned char is_xverb;
  char *function;
  struct verb *next;
};

/* the ref_list structure is a linked list of global variables
   referencing the object */

struct ref_list
{
  struct object *ref_obj;
  unsigned int ref_num;
  struct ref_list *next;
};

/* Heap array constants */
#define UNLIMITED_ARRAY_SIZE 0xFFFFFFFF

/* the attach_list structure is a linked list of objects which
   are attached to this object */

struct attach_list
{
  struct object *attachee;
  struct attach_list *next;
};

/* The object structure contains information about objects */

struct object
{
  signed long refno;            /* object's index in array - unique */
  signed int devnum;            /* input device connected */
  unsigned int flags;           /* flags on the object */
  struct proto *parent;
  struct object *next_child;
  struct object *location;
  struct object *contents;
  struct object *next_object;
  struct object *attacher;
  struct attach_list *attachees;
  struct var *globals;
  struct ref_list *refd_by;
  struct verb *verb_list;       /* PROTO: activated verbs */
  char obj_state;
  signed long file_offset;
  char *input_func;
  struct object *input_func_obj;  /* target object for input_to() */
  long last_access_time;          /* timestamp of last access (for idle tracking) */
  int heart_beat_interval;        /* heartbeat interval in seconds (0 = disabled) */
  long last_heart_beat;           /* timestamp of last heartbeat */
};

/* legal object states */

#define DIRTY 0
#define IN_DB 3
#define IN_CACHE 7
#define FROM_DB 2
#define FROM_CACHE 6

struct proto
{
  char *pathname;
  struct code *funcs;
  struct object *proto_obj;
  struct proto *next_proto;
  struct inherit_list *inherits;  /* Inheritance chain */
};

/* Inherit entry - detailed information about a single inherited parent */
struct inherit_entry
{
  char *alias;                      /* Explicit alias (e.g., "Container") or default basename */
  char *canon_path;                 /* Canonical absolute path (normalized) */
  struct proto *proto;              /* Compiled/cached program */
  unsigned short func_offset;       /* Optional: offset in flat function table */
  unsigned short var_offset;        /* Optional: offset in flat variable table */
};

/* Inheritance chain - compile-time determined parent classes */
struct inherit_list
{
  struct inherit_entry *entry;      /* Detailed inherit information */
  char *inherit_path;               /* Original path for debugging (kept for compatibility) */
  struct proto *parent_proto;       /* Direct proto pointer (kept for compatibility) */
  struct inherit_list *next;        /* Linked list of parents */
};

/* Flattened ancestor map entry for quick var_offset lookup at runtime */
struct ancestor_var_offset
{
  struct proto *proto;              /* Ancestor program */
  unsigned short var_offset;        /* Absolute offset within object's globals */
};

/* GST ref mapping entry for runtime resolution */
struct gst_ref_entry
{
  struct proto *owner;              /* Owner program (NULL = self/current) */
  unsigned short local_index;       /* Index within owner's own variable set */
};

struct file_entry {
  char *filename;
  int flags;
  signed long owner;
  struct file_entry *contents;
  struct file_entry *parent;
  struct file_entry *prev_file;
  struct file_entry *next_file;
};

struct cmdq {
  char *cmd;
  struct object *obj;
  struct cmdq *next;
};

struct destq {
  struct object *obj;
  struct destq *next;
};

struct alarmq {
  struct object *obj;
  char *funcname;
  long delay;
  struct alarmq *next;
};

struct obj_blk {
  struct object *block;
  struct obj_blk *next;
};

struct edit_buf {
  char *buf[NUM_ELINES];
  struct edit_buf *next;
};

struct edit_s {
  struct object *obj;
  unsigned long num_lines;
  struct edit_buf *buf;
  char *path;
  unsigned long curr_line;
  int inserting;
  int is_changed;
  struct edit_s *next;
};
