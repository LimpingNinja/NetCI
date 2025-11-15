/* globals.h */

/* just some variable declarations is all */

/* MSSP variable storage (C-friendly) */
struct mssp_var {
    char *name;
    char *value;
};

extern struct mssp_var *mssp_vars;  /* Array of MSSP variables */
extern int mssp_var_count;          /* Number of variables */

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
extern long boot_time;
extern struct obj_blk *obj_list;
extern char *load_name;
extern char *save_name;
extern char *panic_name;
extern char *auto_object_path;
extern char *save_path;
extern char *save_type;
extern struct object *auto_proto;

/* Periodic timing configuration */
extern int pulses_per_second;  /* game loop tick rate (Hz) */
extern long time_cleanup;      /* seconds between clean_up() calls */
extern long time_reset;        /* seconds between reset() calls */
extern long time_heartbeat;    /* milliseconds for heartbeat interval */
extern long last_reset_time;   /* timestamp of last reset() cycle */
extern long last_cleanup_time; /* timestamp of last clean_up() cycle */
extern struct object *free_obj_list;
extern signed long objects_allocd;
extern signed long db_top;

/* Call stack tracking for error tracebacks */
extern struct call_frame *call_stack;
extern int call_stack_depth;
extern int max_call_stack_depth;
extern int max_trace_depth;
extern int trace_format;
