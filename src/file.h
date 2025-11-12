/* file.h */

/* the file opening, etc, operations */

/* Error codes for file operations */
#define FILE_ERR_SUCCESS 0
#define FILE_ERR_GENERAL 1
#define FILE_ERR_NOT_FOUND -1
#define FILE_ERR_PERMISSION_DENIED -2
#define FILE_ERR_PATH_INVALID -3

/* Security functions */
int validate_path(char *path);
int check_master_permission(char *path, char *operation, struct object *uid, int is_write);

/* Discovery functions */
struct file_entry *discover_file(char *path, struct object *uid);
int discover_directory(char *path, struct object *uid);
int discover_recursive(char *path, struct object *uid);

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

/* Logging levels - higher number = more verbose */
#define LOG_STDOUT  -1 /* Special: writes to syswrite.txt with object-based formatting */
#define LOG_ERROR   0  /* Only errors */
#define LOG_WARNING 1  /* Errors + warnings */
#define LOG_INFO    2  /* Errors + warnings + info (default) */
#define LOG_DEBUG   3  /* Everything including debug */

/* Legacy compatibility */
#define LOG LOG_INFO

void logger(int level, char *msg);
