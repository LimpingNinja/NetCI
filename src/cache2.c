/* cache2.c */
#include "config.h"
#include "object.h"
#include "globals.h"
#include "constrct.h"
#include "dbhandle.h"
#include "interp.h"
#include "file.h"
#include "clearq.h"
#include "table.h"
#include "cache.h"
#include "protos.h"
#include "compile.h"

/* Forward declaration */
int compile_auto_object();

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

/* ========================================================================
 * DATABASE REMOVED - November 12, 2025
 * 
 * TinyMUD-style database save/load has been removed.
 * See database-investigation.md for rationale.
 * 
 * Replacement: save_object()/restore_object() to be implemented.
 * Future: SQLite with JSON1 plugin for structured persistence.
 * ======================================================================== */

/* save_db() and init_db() removed - see database-investigation.md */

/* Boot system - compiles /boot.c and initializes the MUD
 * Always does fresh boot, no database loading
 */
int boot_system() {
  struct code *the_code;
  unsigned int result;
  struct object *obj;
  struct proto *proto_obj;
  long loop;
  struct fns *init_func;
  struct var_stack *rts;
  struct var tmp;

  /* Add boot.c to filesystem */
  db_add_entry("/boot.c",0,0);
  
  /* Parse and compile boot object */
  result=parse_code("/boot",NULL,&the_code);
  if (result==((unsigned int) -1)) return 1;
  if (result) {
    compile_error(NULL,"/boot",result);
    return 1;
  }
  
  /* Create boot object prototype */
  obj=newobj();
  proto_obj=MALLOC(sizeof(struct proto));
  proto_obj->pathname=copy_string("/boot");
  proto_obj->funcs=the_code;
  proto_obj->proto_obj=obj;
  proto_obj->inherits=the_code->inherits;
  proto_obj->next_proto=NULL;
  obj->refno=0;
  obj->devnum=-1;
  obj->input_func=NULL;
  obj->input_func_obj=NULL;
  obj->flags=PROTOTYPE | PRIV;
  obj->parent=proto_obj;
  obj->next_child=NULL;
  obj->location=NULL;
  obj->contents=NULL;
  obj->next_object=NULL;
  
  /* Initialize global variables */
  if (the_code->num_globals) {
    obj->globals=MALLOC(sizeof(struct var)*(the_code->num_globals));
    loop=0;
    while (loop<the_code->num_globals) {
      struct var_tab *var_info = the_code->gst;
      struct heap_array *arr;
      int is_array = 0;
      unsigned int array_size = 0;
      unsigned int max_size = 0;
      
      /* Find this variable in the global symbol table */
      while (var_info) {
        if (var_info->base == loop && var_info->array) {
          is_array = 1;
          /* Calculate total array size (product of all dimensions) */
          struct array_size *dim = var_info->array;
          array_size = 1;
          while (dim) {
            if (dim->size == 255) {
              /* Unlimited size - start with 0 elements, will grow on access */
              array_size = 0;
              break;
            }
            array_size *= dim->size;
            dim = dim->next;
          }
          /* Determine max_size (255 means unlimited) */
          max_size = (var_info->array->size == 255) ? UNLIMITED_ARRAY_SIZE : array_size;
          break;
        }
        var_info = var_info->next;
      }
      
      if (is_array) {
        /* Allocate heap array */
        arr = allocate_array(array_size, max_size);
        obj->globals[loop].type = ARRAY;
        obj->globals[loop].value.array_ptr = arr;
      } else {
        /* Regular variable - initialize to 0 */
        obj->globals[loop].type=INTEGER;
        obj->globals[loop].value.integer=0;
      }
      loop++;
    }
  } else {
    obj->globals=NULL;
  }
  
  obj->refd_by=NULL;
  obj->verb_list=NULL;
  add_loaded(obj);
  obj->obj_state=DIRTY;

  /* Compile auto object BEFORE calling boot.init()
   * This ensures auto is available when boot compiles other objects
   */
  if (compile_auto_object()) {
    /* Auto compilation failed - abort startup */
    return 1;
  }
  
  /* Attach auto to boot object */
  if (auto_proto) {
    attach_auto_to(ref_to_obj(0));
  }

#ifdef CYCLE_HARD_MAX
  hard_cycles=0;
#endif
#ifdef CYCLE_SOFT_MAX
  soft_cycles=0;
#endif

  /* Call boot.init() to initialize the MUD */
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

/* Compile and initialize the auto object
 * Returns 0 on success, non-zero on failure
 * Sets global auto_proto on success
 */
int compile_auto_object() {
  struct code *the_code;
  unsigned int result;
  struct object *obj;
  struct proto *proto_obj;
  long loop;
  struct fns *init_func;
  struct var_stack *rts;
  struct var tmp;
  char *buf;
  
  /* Skip if no auto object configured */
  if (!auto_object_path || !auto_object_path[0]) {
    auto_proto = NULL;
    return 0;
  }
  
  /* Log that we're compiling auto */
  buf = MALLOC(strlen(auto_object_path) + 50);
  sprintf(buf, " system: compiling auto object %s", auto_object_path);
  logger(LOG_INFO, buf);
  FREE(buf);
  
  /* Add file entry for auto object */
  buf = MALLOC(strlen(auto_object_path) + 3);
  sprintf(buf, "%s.c", auto_object_path);
  db_add_entry(buf, 0, 0);
  FREE(buf);
  
  /* Parse and compile the auto object */
  result = parse_code(auto_object_path, NULL, &the_code);
  if (result == ((unsigned int) -1)) {
    buf = MALLOC(strlen(auto_object_path) + 50);
    sprintf(buf, " system: auto object %s not found", auto_object_path);
    logger(LOG_ERROR, buf);
    FREE(buf);
    return 1;
  }
  if (result) {
    compile_error(NULL, auto_object_path, result);
    buf = MALLOC(strlen(auto_object_path) + 50);
    sprintf(buf, " system: auto object %s failed to compile", auto_object_path);
    logger(LOG_ERROR, buf);
    FREE(buf);
    return 1;
  }
  
  /* Create the auto object prototype */
  obj = newobj();
  proto_obj = MALLOC(sizeof(struct proto));
  proto_obj->pathname = copy_string(auto_object_path);
  proto_obj->funcs = the_code;
  proto_obj->proto_obj = obj;
  proto_obj->inherits = the_code->inherits;  /* Copy inherits from code */
  proto_obj->next_proto = ref_to_obj(0)->parent->next_proto;
  ref_to_obj(0)->parent->next_proto = proto_obj;
  
  obj->devnum = -1;
  obj->input_func = NULL;
  obj->input_func_obj = NULL;
  obj->flags = PROTOTYPE;  /* Note: NOT PRIV - auto is not privileged */
  obj->parent = proto_obj;
  obj->next_child = NULL;
  obj->location = NULL;
  obj->contents = NULL;
  obj->next_object = NULL;
  obj->attachees = NULL;
  obj->attacher = NULL;
  
  /* Initialize globals */
  if (the_code->num_globals) {
    obj->globals = MALLOC(sizeof(struct var) * (the_code->num_globals));
    loop = 0;
    while (loop < the_code->num_globals) {
      struct var_tab *var_info = the_code->gst;
      struct heap_array *arr;
      int is_array = 0;
      unsigned int array_size = 0;
      unsigned int max_size = 0;
      
      /* Find this variable in the global symbol table */
      while (var_info) {
        if (var_info->base == loop && var_info->array) {
          is_array = 1;
          /* Calculate total array size (product of all dimensions) */
          struct array_size *dim = var_info->array;
          array_size = 1;
          while (dim) {
            if (dim->size == 255) {
              /* Unlimited size - start with 0 elements, will grow on access */
              array_size = 0;
              break;
            }
            array_size *= dim->size;
            dim = dim->next;
          }
          /* Determine max_size (255 means unlimited) */
          max_size = (var_info->array->size == 255) ? UNLIMITED_ARRAY_SIZE : array_size;
          break;
        }
        var_info = var_info->next;
      }
      
      if (is_array) {
        /* Allocate heap array */
        arr = allocate_array(array_size, max_size);
        obj->globals[loop].type = ARRAY;
        obj->globals[loop].value.array_ptr = arr;
      } else {
        /* Regular variable - initialize to 0 */
        obj->globals[loop].type = INTEGER;
        obj->globals[loop].value.integer = 0;
      }
      loop++;
    }
  } else {
    obj->globals = NULL;
  }
  
  obj->refd_by = NULL;
  obj->verb_list = NULL;
  add_loaded(obj);
  obj->obj_state = DIRTY;
  
  /* Call init() if it exists */
  init_func = find_function("init", obj, &obj);
  if (init_func) {
    rts = NULL;
    tmp.type = NUM_ARGS;
    tmp.value.num = 0;
    push(&tmp, &rts);
    interp(NULL, obj, NULL, &rts, init_func);
    free_stack(&rts);
  }
  
  /* Set global auto_proto */
  auto_proto = obj;
  
  buf = MALLOC(strlen(auto_object_path) + 50);
  sprintf(buf, " system: auto object %s compiled successfully", auto_object_path);
  logger(LOG_INFO, buf);
  FREE(buf);
  
  return 0;
}

/* ========================================================================
 * GLOBAL PROTO CACHE
 * ======================================================================== */

/* Global proto cache for compile-time inheritance
 * Each file is compiled once and cached here
 * Key is the canonical absolute path
 */
static struct proto_cache_entry {
    char *pathname;              /* Canonical path */
    struct proto *proto;         /* Cached proto */
    struct proto_cache_entry *next;
} *proto_cache_head = NULL;

/* Find a cached proto by canonical pathname
 * Returns NULL if not found
 */
struct proto *find_cached_proto(char *pathname) {
    struct proto_cache_entry *curr = proto_cache_head;
    
    while (curr) {
        if (strcmp(curr->pathname, pathname) == 0) {
            return curr->proto;
        }
        curr = curr->next;
    }
    
    return NULL;
}

/* Add a proto to the global cache
 * pathname should be canonical (normalized)
 */
void cache_proto(char *pathname, struct proto *proto) {
    struct proto_cache_entry *entry;
    
    /* Check if already cached (shouldn't happen, but be safe) */
    if (find_cached_proto(pathname)) {
        char logbuf[512];
        sprintf(logbuf, "cache_proto: warning - '%s' already cached", pathname);
        logger(LOG_WARNING, logbuf);
        return;
    }
    
    entry = MALLOC(sizeof(struct proto_cache_entry));
    entry->pathname = copy_string(pathname);
    entry->proto = proto;
    entry->next = proto_cache_head;
    proto_cache_head = entry;
}

/* Clear the proto cache (for development/testing)
 * Note: Does not free the protos themselves as they may still be in use
 */
void clear_proto_cache() {
    struct proto_cache_entry *curr = proto_cache_head;
    
    while (curr) {
        struct proto_cache_entry *next = curr->next;
        FREE(curr->pathname);
        /* Don't free proto - it might still be referenced */
        FREE(curr);
        curr = next;
    }
    
    proto_cache_head = NULL;
}
