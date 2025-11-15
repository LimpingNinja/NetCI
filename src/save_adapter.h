/* save_adapter.h */

#ifndef SAVE_ADAPTER_H
#define SAVE_ADAPTER_H

/* Forward declarations */
struct object;
struct heap_mapping;

/* Storage adapter interface for save_object/restore_object
 * 
 * Provides abstraction between object persistence and storage mechanism.
 * Default: file adapter (saves mappings as literals in filesystem)
 * Future: sqlite adapter, redis adapter, etc.
 */
typedef struct save_adapter {
    char *name;
    
    /* Save a mapping to storage with given key
     * key: storage key (usually object path)
     * data: mapping containing serialized object data
     * caller: object requesting the save (for permissions/logging)
     * Returns: 0 on success, non-zero on error
     */
    int (*save_map)(char *key, struct heap_mapping *data, struct object *caller);
    
    /* Restore a mapping from storage with given key
     * key: storage key to retrieve
     * caller: object requesting the restore (for permissions/logging)
     * Returns: mapping on success, NULL on error
     */
    struct heap_mapping *(*restore_map)(char *key, struct object *caller);
    
} save_adapter_t;

/* Get current save adapter (file by default) */
save_adapter_t *get_save_adapter(void);

/* Set save adapter (for plugins) */
void set_save_adapter(save_adapter_t *adapter);

/* Initialize save adapter system */
void init_save_adapter(void);

/* File adapter (default) */
extern save_adapter_t file_adapter;

#endif /* SAVE_ADAPTER_H */
