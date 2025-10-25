/* globals.c */

/* just some variable declarations is all */

#include "config.h"
#include "object.h"
#include "file.h"

int doing_ls;
int verbs_changed;
char *window_title;
long time_offset;
int noisy;
int log_level = LOG;  /* Default: show ERROR, WARNING, LOG but not DEBUG */
long transact_log_size;
long soft_cycles;
long hard_cycles;
int use_soft_cycles;
int use_hard_cycles;
char *fs_path;
char *syslog_name;
char *transact_log_name;
char *tmpdb_name;
signed long cache_top;
struct edit_s *edit_list;
struct file_entry *root_dir;
unsigned int num_locals;
struct var *locals;
char *c_err_msg;
struct cmdq *cmd_head;
struct cmdq *cmd_tail;
struct destq *dest_list;
struct alarmq *alarm_list;
long now_time;
struct obj_blk *obj_list;
char *load_name;
char *save_name;
char *panic_name;
struct object *free_obj_list;
signed long objects_allocd;
signed long db_top;
