/* globals.h */

/* just some variable declarations is all */

extern int doing_ls;
extern int verbs_changed;
extern char *window_title;
extern long time_offset;
extern int noisy;
extern int log_level;
extern long transact_log_size;
extern long soft_cycles;
extern long hard_cycles;
extern int use_soft_cycles;
extern int use_hard_cycles;
extern char *fs_path;
extern char *syslog_name;
extern char *transact_log_name;
extern char *tmpdb_name;
extern signed long cache_top;
extern struct edit_s *edit_list;
extern struct file_entry *root_dir;
extern unsigned int num_locals;
extern struct var *locals;
extern char *c_err_msg;
extern struct cmdq *cmd_head;
extern struct cmdq *cmd_tail;
extern struct destq *dest_list;
extern struct alarmq *alarm_list;
extern long now_time;
extern struct obj_blk *obj_list;
extern char *load_name;
extern char *save_name;
extern char *panic_name;
extern char *auto_object_path;
extern struct object *auto_proto;
extern struct object *free_obj_list;
extern signed long objects_allocd;
extern signed long db_top;

/* Call stack tracking for error tracebacks */
extern struct call_frame *call_stack;
extern int call_stack_depth;
extern int max_call_stack_depth;
extern int max_trace_depth;
extern int trace_format;
