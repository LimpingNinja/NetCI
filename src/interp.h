/* interp.h */

/* Call frame structure for stack trace tracking */
struct call_frame {
  struct object *obj;           /* Object being executed */
  struct fns *func;             /* Function being executed */
  unsigned long line;           /* Current line number */
  struct call_frame *prev;      /* Previous frame (caller) */
};

struct fns *find_fns(char *name, struct object *obj);
struct fns *find_function(char *name, struct object *obj,
                          struct object **real_obj);
void interp_error(char *msg, struct object *player, struct object *obj,
                  struct fns *func, unsigned long line);
void interp_error_with_trace(char *msg, struct object *player, struct object *obj,
                              struct fns *func, unsigned long line);
void interp_error_compact(char *msg, struct object *player, struct object *obj,
                          struct fns *func, unsigned long line);
int interp(struct object *caller, struct object *obj, struct object *player,
           struct var_stack **arg_stack, struct fns *func);
