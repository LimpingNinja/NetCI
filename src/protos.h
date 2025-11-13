#define OPER_PROTO(FUNCNAME_)                                           \
  extern int FUNCNAME_(struct object *caller, struct object *obj,       \
                       struct object *player, struct var_stack **rts);

OPER_PROTO(comma_oper)
OPER_PROTO(eq_oper)
OPER_PROTO(pleq_oper)
OPER_PROTO(mieq_oper)
OPER_PROTO(mueq_oper)
OPER_PROTO(dieq_oper)
OPER_PROTO(moeq_oper)
OPER_PROTO(aneq_oper)
OPER_PROTO(exeq_oper)
OPER_PROTO(oreq_oper)
OPER_PROTO(lseq_oper)
OPER_PROTO(rseq_oper)
OPER_PROTO(cond_oper)
OPER_PROTO(or_oper)
OPER_PROTO(and_oper)
OPER_PROTO(bitor_oper)
OPER_PROTO(exor_oper)
OPER_PROTO(bitand_oper)
OPER_PROTO(condeq_oper)
OPER_PROTO(noteq_oper)
OPER_PROTO(less_oper)
OPER_PROTO(lesseq_oper)
OPER_PROTO(great_oper)
OPER_PROTO(greateq_oper)
OPER_PROTO(ls_oper)
OPER_PROTO(rs_oper)
OPER_PROTO(add_oper)
OPER_PROTO(min_oper)
OPER_PROTO(mul_oper)
OPER_PROTO(div_oper)
OPER_PROTO(mod_oper)
OPER_PROTO(not_oper)
OPER_PROTO(bitnot_oper)
OPER_PROTO(postadd_oper)
OPER_PROTO(preadd_oper)
OPER_PROTO(postmin_oper)
OPER_PROTO(premin_oper)
OPER_PROTO(umin_oper)
OPER_PROTO(s_add_verb)
OPER_PROTO(s_add_xverb)
OPER_PROTO(s_call_other)
OPER_PROTO(s_alarm)
OPER_PROTO(s_remove_alarm)
OPER_PROTO(s_caller_object)
OPER_PROTO(s_clone_object)
OPER_PROTO(s_destruct)
OPER_PROTO(s_contents)
OPER_PROTO(s_next_object)
OPER_PROTO(s_location)
OPER_PROTO(s_next_child)
OPER_PROTO(s_parent)
OPER_PROTO(s_next_proto)
OPER_PROTO(s_move_object)
OPER_PROTO(s_this_object)
OPER_PROTO(s_this_player)
OPER_PROTO(s_set_interactive)
OPER_PROTO(s_interactive)
OPER_PROTO(s_set_priv)
OPER_PROTO(s_priv)
OPER_PROTO(s_in_editor)
OPER_PROTO(s_connected)
OPER_PROTO(s_get_devconn)
OPER_PROTO(s_send_device)
OPER_PROTO(s_send_prompt)
OPER_PROTO(s_reconnect_device)
OPER_PROTO(s_disconnect_device)
OPER_PROTO(s_random)
OPER_PROTO(s_time)
OPER_PROTO(s_mktime)
OPER_PROTO(s_typeof)
OPER_PROTO(s_command)
OPER_PROTO(s_compile_object)
OPER_PROTO(s_compile_string)
OPER_PROTO(s_crypt)
OPER_PROTO(s_read_file)
OPER_PROTO(s_write_file)
OPER_PROTO(s_remove)
OPER_PROTO(s_rename)
OPER_PROTO(s_get_dir)
OPER_PROTO(s_file_size)
OPER_PROTO(s_users)
OPER_PROTO(s_objects)
OPER_PROTO(s_children)
OPER_PROTO(s_all_inventory)
OPER_PROTO(s_query_terminal)
OPER_PROTO(s_get_mssp)
OPER_PROTO(s_set_mssp)
OPER_PROTO(s_edit)
OPER_PROTO(s_cat)
OPER_PROTO(s_ls)
OPER_PROTO(s_rm)
OPER_PROTO(s_cp)
OPER_PROTO(s_mv)
OPER_PROTO(s_mkdir)
OPER_PROTO(s_rmdir)
OPER_PROTO(s_hide)
OPER_PROTO(s_unhide)
OPER_PROTO(s_chown)
OPER_PROTO(s_syslog)
OPER_PROTO(s_syswrite)
OPER_PROTO(s_sscanf)
OPER_PROTO(s_sprintf)
OPER_PROTO(s_midstr)
OPER_PROTO(s_strlen)
OPER_PROTO(s_leftstr)
OPER_PROTO(s_rightstr)
OPER_PROTO(s_subst)
OPER_PROTO(s_instr)
OPER_PROTO(s_otoa)
OPER_PROTO(s_itoa)
OPER_PROTO(s_atoi)
OPER_PROTO(s_atoo)
OPER_PROTO(s_upcase)
OPER_PROTO(s_downcase)
OPER_PROTO(s_is_legal)
OPER_PROTO(s_otoi)
OPER_PROTO(s_itoo)
OPER_PROTO(s_chmod)
OPER_PROTO(s_fread)
OPER_PROTO(s_fwrite)
OPER_PROTO(s_remove_verb)
OPER_PROTO(s_ferase)
OPER_PROTO(s_chr)
OPER_PROTO(s_asc)
OPER_PROTO(s_sysctl)
OPER_PROTO(s_prototype)
OPER_PROTO(s_iterate)
OPER_PROTO(s_next_who)
OPER_PROTO(s_get_devidle)
OPER_PROTO(s_get_conntime)
OPER_PROTO(s_connect_device)
OPER_PROTO(s_flush_device)
OPER_PROTO(s_attach)
OPER_PROTO(s_this_component)
OPER_PROTO(s_detach)
OPER_PROTO(s_table_get)
OPER_PROTO(s_table_set)
OPER_PROTO(s_table_delete)
OPER_PROTO(s_fstat)
OPER_PROTO(s_fowner)
OPER_PROTO(s_get_hostname)
OPER_PROTO(s_get_address)
OPER_PROTO(s_set_localverbs)
OPER_PROTO(s_localverbs)
OPER_PROTO(s_next_verb)
OPER_PROTO(s_get_devport)
OPER_PROTO(s_get_devnet)
OPER_PROTO(s_redirect_input)
OPER_PROTO(s_get_input_func)
OPER_PROTO(s_get_master)
OPER_PROTO(s_is_master)
OPER_PROTO(s_input_to)
OPER_PROTO(s_sizeof)
OPER_PROTO(s_implode)
OPER_PROTO(s_explode)
OPER_PROTO(s_member_array)
OPER_PROTO(s_sort_array)
OPER_PROTO(s_reverse)
OPER_PROTO(s_unique_array)
OPER_PROTO(s_array_literal)

/* Serialization efuns */
OPER_PROTO(s_save_value)
OPER_PROTO(s_restore_value)

/* String manipulation efuns */
OPER_PROTO(s_replace_string)

/* String helper functions (sys_strings.c) */
char *escape_string(char *str);
char *save_value_internal(struct var *value, int depth);

/* Token parser helpers (token_parse.c) - see token.h for prototypes */

/* Mapping efuns (Phase 2) */
OPER_PROTO(s_keys)
OPER_PROTO(s_values)
OPER_PROTO(s_map_delete)
OPER_PROTO(s_member)

/* Mapping literals (Phase 4) */
OPER_PROTO(s_mapping_literal)

/* Heap array functions (Phase 2.5) */
struct heap_array* allocate_array(unsigned int size, unsigned int max_size);
void array_addref(struct heap_array *arr);
void array_release(struct heap_array *arr);
int resize_heap_array(struct heap_array *arr, unsigned int new_size);

/* Array arithmetic helper functions (Phase 5) */
struct heap_array* array_concat(struct heap_array *arr1, struct heap_array *arr2);
struct heap_array* array_subtract(struct heap_array *arr1, struct heap_array *arr2);

/* Heap mapping functions (Mapping Redux Phase 1) */
struct heap_mapping* allocate_mapping(unsigned int initial_capacity);
void mapping_addref(struct heap_mapping *map);
void mapping_release(struct heap_mapping *map);
void mapping_rehash(struct heap_mapping *map);

/* Mapping operations */
unsigned int hash_string(const char *str);
unsigned int hash_integer(int value);
unsigned int hash_object(struct object *obj);
unsigned int hash_var(struct var *key);
int var_equals(struct var *v1, struct var *v2);
void copy_var_to(struct var *dest, struct var *src);

struct var* mapping_get_or_create(struct heap_mapping *map, struct var *key);
int mapping_get(struct heap_mapping *map, struct var *key, struct var *result);
int mapping_set(struct heap_mapping *map, struct var *key, struct var *value);
int mapping_delete(struct heap_mapping *map, struct var *key);
int mapping_exists(struct heap_mapping *map, struct var *key);
struct heap_mapping* mapping_merge(struct heap_mapping *m1, struct heap_mapping *m2);
struct heap_mapping* mapping_subtract(struct heap_mapping *m1, struct heap_mapping *m2);
struct heap_array* mapping_keys_array(struct heap_mapping *map);
struct heap_array* mapping_values_array(struct heap_mapping *map);

/* Auto-object attachment functions */
void attach_auto_to(struct object *target);
void attach_auto_to_all();
