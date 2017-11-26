/* interp.h */

struct fns *find_fns(char *name, struct object *obj);
struct fns *find_function(char *name, struct object *obj,
                          struct object **real_obj);
void interp_error(char *msg, struct object *player, struct object *obj,
                  struct fns *func, unsigned long line);
int interp(struct object *caller, struct object *obj, struct object *player,
           struct var_stack **arg_stack, struct fns *func);
