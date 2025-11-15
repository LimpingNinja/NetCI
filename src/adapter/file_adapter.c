/* adapter/file_adapter.c - File-based storage adapter for save_object */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "../config.h"
#include "../autoconf.h"
#include "../object.h"
#include "../save_adapter.h"
#include "../table.h"
#include "../file.h"
#include "../token.h"
#include "../protos.h"

extern char *fs_path;
extern char *save_path;

/* Forward declarations for functions used by adapter */
extern char *save_value_internal(struct var *value, int indent);
extern void clear_var(struct var *v);
extern void mapping_addref(struct heap_mapping *map);

/* Build full save path from key
 * Returns allocated string: fs_path/save_path + key + ".o"
 */
static char *build_save_path(char *key) {
    char *result;
    char *base_dir;
    char *key_part;
    int len, save_path_len;
    
    /* Default save_path if not configured */
    if (!save_path || !*save_path) {
        save_path = "data/save/";
    }
    
    /* Skip leading slash in key if save_path ends with slash */
    save_path_len = strlen(save_path);
    key_part = key;
    if (save_path[save_path_len - 1] == '/' && key[0] == '/') {
        key_part = key + 1;  /* Skip the leading slash in key */
    }
    
    /* Build full path: fs_path + "/" + save_path + key_part + ".o" */
    len = strlen(fs_path) + strlen(save_path) + strlen(key_part) + 6;
    result = MALLOC(len);
    
    /* Construct path */
    sprintf(result, "%s/%s%s.o", fs_path, save_path, key_part);
    
    return result;
}

/* Create parent directories for a file path */
static int ensure_directory_exists(char *filepath) {
    char *path, *p;
    struct stat st;
    char logbuf[512];
    
    /* Copy path so we can modify it */
    path = MALLOC(strlen(filepath) + 1);
    strcpy(path, filepath);
    
    sprintf(logbuf, "  file_adapter: ensure_directory_exists for: %s", filepath);
    logger(LOG_DEBUG, logbuf);
    
    /* Walk path and create directories */
    for (p = path + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            
            /* Check if directory exists */
            if (stat(path, &st) != 0) {
                /* Directory doesn't exist, try to create it */
                sprintf(logbuf, "  file_adapter: creating directory: %s", path);
                logger(LOG_INFO, logbuf);
                
                if (mkdir(path, 0755) != 0 && errno != EEXIST) {
                    sprintf(logbuf, "  file_adapter: mkdir failed for %s (errno=%d: %s)", 
                            path, errno, strerror(errno));
                    logger(LOG_ERROR, logbuf);
                    FREE(path);
                    return -1;
                }
            }
            
            *p = '/';
        }
    }
    
    FREE(path);
    return 0;
}

/* File adapter: save mapping to file
 * Format: mapping literal using save_value() serialization
 */
static int file_save_map(char *key, struct heap_mapping *data, struct object *caller) {
    char *path;
    char *serialized;
    FILE *fp;
    int result = 0;
    char logbuf[512];
    
    if (!key || !data) return -1;
    
    /* Build file path */
    path = build_save_path(key);
    
    sprintf(logbuf, "  file_adapter: attempting to save to: %s", path);
    logger(LOG_DEBUG, logbuf);
    
    /* Ensure parent directory exists */
    if (ensure_directory_exists(path) != 0) {
        sprintf(logbuf, "  file_adapter: failed to create directory for: %s (errno=%d)", path, errno);
        logger(LOG_ERROR, logbuf);
        FREE(path);
        return -1;
    }
    
    /* Serialize mapping to string using save_value_internal() */
    {
        struct var map_var;
        map_var.type = MAPPING;
        map_var.value.mapping_ptr = data;
        serialized = save_value_internal(&map_var, 0);
        /* Don't clear map_var since we don't own the mapping */
    }
    
    if (!serialized) {
        logger(LOG_ERROR, "  file_adapter: serialization failed");
        FREE(path);
        return -1;
    }
    
    /* Write to file */
    fp = fopen(path, "w");
    if (!fp) {
        logger(LOG_ERROR, "  file_adapter: failed to open file for writing");
        FREE(serialized);
        FREE(path);
        return -1;
    }
    
    /* Write serialized mapping */
    if (fprintf(fp, "%s\n", serialized) < 0) {
        logger(LOG_ERROR, "  file_adapter: write failed");
        result = -1;
    }
    
    fclose(fp);
    FREE(serialized);
    FREE(path);
    
    return result;
}

/* File adapter: restore mapping from file */
static struct heap_mapping *file_restore_map(char *key, struct object *caller) {
    char *path;
    char *buffer = NULL;
    FILE *fp;
    long file_size;
    struct heap_mapping *result = NULL;
    
    if (!key) return NULL;
    
    /* Build file path */
    path = build_save_path(key);
    
    /* Open file */
    fp = fopen(path, "r");
    if (!fp) {
        /* File not found is not an error, just return NULL */
        FREE(path);
        return NULL;
    }
    
    /* Get file size */
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (file_size <= 0 || file_size > 10485760) { /* 10MB max */
        logger(LOG_ERROR, "  file_adapter: invalid file size");
        fclose(fp);
        FREE(path);
        return NULL;
    }
    
    /* Read entire file */
    buffer = MALLOC(file_size + 1);
    if (fread(buffer, 1, file_size, fp) != file_size) {
        logger(LOG_ERROR, "  file_adapter: read failed");
        FREE(buffer);
        fclose(fp);
        FREE(path);
        return NULL;
    }
    buffer[file_size] = '\0';
    
    fclose(fp);
    
    /* Deserialize using parse_value() */
    {
        struct var parsed_value;
        filptr *fp_parse;
        
        /* Create mock filptr from string */
        fp_parse = create_string_filptr(buffer);
        if (!fp_parse) {
            logger(LOG_ERROR, "  file_adapter: failed to create string filptr");
            FREE(buffer);
            FREE(path);
            return NULL;
        }
        
        /* Parse the value */
        if (parse_value(fp_parse, &parsed_value)) {
            logger(LOG_ERROR, "  file_adapter: deserialization failed");
            free_string_filptr(fp_parse);
            FREE(buffer);
            FREE(path);
            return NULL;
        }
        
        free_string_filptr(fp_parse);
        
        /* Check if it's a mapping */
        if (parsed_value.type != MAPPING) {
            logger(LOG_ERROR, "  file_adapter: restored value is not a mapping");
            clear_var(&parsed_value);
            FREE(buffer);
            FREE(path);
            return NULL;
        }
        
        result = parsed_value.value.mapping_ptr;
        /* Increment refcount since we're returning it */
        mapping_addref(result);
        clear_var(&parsed_value); /* This will decrement refcount */
    }
    
    FREE(buffer);
    FREE(path);
    
    return result;
}

/* File adapter definition */
save_adapter_t file_adapter = {
    "file",
    file_save_map,
    file_restore_map
};
