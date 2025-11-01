/* cache.h */

/* the cache functions. what else? */

struct object *db_ref_to_obj(signed long refno);
void unload_object(struct object *obj);
void load_data(struct object *obj);
void unload_data();
void add_loaded(struct object *obj);
int init_db();
int create_db();
int save_db(char *filename);
void init_globals(char *loadpath, char *savepath, char *panicpath);
