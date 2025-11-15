/* adapter/adapter_core.c - Save adapter management */

#include <stdio.h>
#include "../config.h"
#include "../autoconf.h"
#include "../object.h"
#include "../save_adapter.h"
#include "../file.h"

/* Current active adapter */
static save_adapter_t *current_adapter = NULL;

/* Get current save adapter */
save_adapter_t *get_save_adapter(void) {
    if (!current_adapter) {
        /* Default to file adapter */
        current_adapter = &file_adapter;
    }
    return current_adapter;
}

/* Set save adapter (for plugins) */
void set_save_adapter(save_adapter_t *adapter) {
    char buf[256];
    
    if (adapter) {
        current_adapter = adapter;
        sprintf(buf, "  save_adapter: switched to adapter: %s", adapter->name);
        logger(LOG_INFO, buf);
    }
}

/* Initialize save adapter system */
void init_save_adapter(void) {
    char buf[256];
    
    current_adapter = &file_adapter;
    sprintf(buf, "  save_adapter: initialized with file adapter");
    logger(LOG_INFO, buf);
}
