/* construct.h */

/* some constructor functions */

void free_gst(struct var_tab *gst);
int is_legal(char *name);
void free_code(struct code *the_code);
char *copy_string(char *s);
void push(struct var *data, struct var_stack **rts);
void pushnocopy(struct var *data, struct var_stack **rts);
int pop(struct var *data, struct var_stack **rts, struct object *obj);
void free_stack(struct var_stack **rts);
int popint(struct var *data, struct var_stack **rts, struct object *obj);
void clear_var(struct var *data);
void clear_global_var(struct object *obj, unsigned int ref);
void copy_var(struct var *dest, struct var *src);
int resolve_var(struct var *data, struct object *obj);
struct var_stack *gen_stack(struct var_stack **rts, struct object *obj);
struct var_stack *gen_stack_noresolve(struct var_stack **rts,
                                      struct object *obj);
