/* file.h */

/* the file opening, etc, operations */

int stat_file(char *filename,struct object *uid);
int owner_file(char *filename,struct object *uid);
int ls_dir(char *filename, struct object *uid, struct object *player);
FILE *open_file(char *filename, char *mode, struct object *uid);
#define close_file(_FILE) fclose(_FILE)
int remove_file(char *filename, struct object *uid);
int copy_file(char *src,char *dest,struct object *uid);
int move_file(char *src, char *dest, struct object *uid);
int make_dir(char *filename, struct object *uid);
int remove_dir(char *filename, struct object *uid);
int hide(char *filename);
int unhide(char *filename, struct object *uid, int flags);
int db_add_entry(char *filename, signed long uid, int flags);
int chown_file(char *filename, struct object *uid, struct object *new_owner);
int chmod_file(char *filename, struct object *uid, int flags);
void log_sysmsg(char *msg);
