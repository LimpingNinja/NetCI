/* dbhandle.h */

void db_queue_for_alarm(struct object *obj, long delay, char *funcname);
void remove_verb(struct object *obj, char *verb_name);
void queue_command(struct object *player, char *cmd);
void queue_for_destruct(struct object *obj);
void queue_for_alarm(struct object *obj, long delay, char *funcname);
long remove_alarm(struct object *obj, char *funcname);
struct object *newobj();
struct object *find_proto(char *path);
void compile_error(struct object *player, char *path, unsigned int line);
struct object *ref_to_obj(signed long refno);
