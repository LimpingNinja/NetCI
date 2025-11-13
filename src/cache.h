/* cache.h */

/* the cache functions. what else? */

struct object *db_ref_to_obj(signed long refno);
void unload_object(struct object *obj);
void load_data(struct object *obj);
void unload_data();
void add_loaded(struct object *obj);

/* Database removed - November 12, 2025 */
int boot_system();  /* Replaces create_db/init_db - always fresh boot */
void init_globals(char *loadpath, char *savepath, char *panicpath);

/* Proto cache functions */
struct proto *find_cached_proto(char *pathname);
void cache_proto(char *pathname, struct proto *proto);
void clear_proto_cache();
