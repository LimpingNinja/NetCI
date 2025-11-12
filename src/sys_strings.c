/* sys_strings.c - Driver-level string manipulation helpers
 *
 * Contains low-level string utilities used by the driver:
 * - escape_string() - Escape special characters for serialization
 * - save_value_internal() - Recursive value serialization
 */

#include "config.h"
#include "object.h"
#include "protos.h"
#include "globals.h"
#include "constrct.h"
#include "file.h"
#include <string.h>
#include <stdio.h>

/* Maximum recursion depth to prevent stack overflow */
#define MAX_SAVE_DEPTH 50

/* ========================================================================
 * STRING ESCAPING
 * ======================================================================== */

/* Escape a string for serialization
 * Escapes: \ " \n \t \r
 * Returns: Newly allocated escaped string (caller must FREE)
 */
char *escape_string(char *str) {
    int len, i;
    char *result, *p;
    int escape_count = 0;
    
    if (!str) return copy_string("");
    
    len = strlen(str);
    
    /* Count characters that need escaping */
    for (i = 0; i < len; i++) {
        if (str[i] == '\\' || str[i] == '"' || str[i] == '\n' || 
            str[i] == '\t' || str[i] == '\r') {
            escape_count++;
        }
    }
    
    /* Allocate result string */
    result = MALLOC(len + escape_count + 1);
    if (!result) return NULL;
    
    /* Copy and escape */
    p = result;
    for (i = 0; i < len; i++) {
        if (str[i] == '\\') {
            *p++ = '\\';
            *p++ = '\\';
        } else if (str[i] == '"') {
            *p++ = '\\';
            *p++ = '"';
        } else if (str[i] == '\n') {
            *p++ = '\\';
            *p++ = 'n';
        } else if (str[i] == '\t') {
            *p++ = '\\';
            *p++ = 't';
        } else if (str[i] == '\r') {
            *p++ = '\\';
            *p++ = 'r';
        } else {
            *p++ = str[i];
        }
    }
    *p = '\0';
    
    return result;
}

/* ========================================================================
 * VALUE SERIALIZATION
 * ======================================================================== */

/* Serialize a value to string (recursive)
 * Handles: integers, strings, arrays, mappings, objects
 * Returns: Newly allocated string representation (caller must FREE)
 */
char *save_value_internal(struct var *value, int depth) {
    char *result, *temp, *escaped;
    struct heap_array *arr;
    struct heap_mapping *map;
    struct mapping_entry *entry;
    char numbuf[64];
    int i, first;
    char *key_str, *val_str;
    
    /* Check recursion depth */
    if (depth > MAX_SAVE_DEPTH) {
        logger(LOG_ERROR, "save_value: maximum recursion depth exceeded");
        return NULL;
    }
    
    switch (value->type) {
        case INTEGER:
            /* Simple integer */
            sprintf(numbuf, "%ld", value->value.integer);
            return copy_string(numbuf);
            
        case STRING:
            /* Escaped string with quotes */
            escaped = escape_string(value->value.string);
            if (!escaped) return NULL;
            
            result = MALLOC(strlen(escaped) + 3);  /* "string" */
            if (!result) {
                FREE(escaped);
                return NULL;
            }
            sprintf(result, "\"%s\"", escaped);
            FREE(escaped);
            return result;
            
        case ARRAY:
            /* Array literal: ({elem1,elem2,...}) */
            arr = value->value.array_ptr;
            if (!arr || arr->size == 0) {
                return copy_string("({})");
            }
            
            /* Build array string */
            result = copy_string("({");
            for (i = 0; i < arr->size; i++) {
                temp = save_value_internal(&arr->elements[i], depth + 1);
                if (!temp) {
                    FREE(result);
                    return NULL;
                }
                
                /* Append element */
                escaped = MALLOC(strlen(result) + strlen(temp) + 2);
                if (i > 0) {
                    sprintf(escaped, "%s,%s", result, temp);
                } else {
                    sprintf(escaped, "%s%s", result, temp);
                }
                FREE(result);
                FREE(temp);
                result = escaped;
            }
            
            /* Close array */
            temp = MALLOC(strlen(result) + 3);
            sprintf(temp, "%s})", result);
            FREE(result);
            return temp;
            
        case MAPPING:
            /* Mapping literal: ([key1:val1,key2:val2,...]) */
            map = value->value.mapping_ptr;
            if (!map || map->size == 0) {
                return copy_string("([])");
            }
            
            /* Build mapping string */
            result = copy_string("([");
            first = 1;
            
            for (i = 0; i < map->capacity; i++) {
                entry = map->buckets[i];
                while (entry) {
                    /* Serialize key */
                    key_str = save_value_internal(&entry->key, depth + 1);
                    if (!key_str) {
                        FREE(result);
                        return NULL;
                    }
                    
                    /* Serialize value */
                    val_str = save_value_internal(&entry->value, depth + 1);
                    if (!val_str) {
                        FREE(key_str);
                        FREE(result);
                        return NULL;
                    }
                    
                    /* Append key:value */
                    temp = MALLOC(strlen(result) + strlen(key_str) + strlen(val_str) + 4);
                    if (first) {
                        sprintf(temp, "%s%s:%s", result, key_str, val_str);
                        first = 0;
                    } else {
                        sprintf(temp, "%s,%s:%s", result, key_str, val_str);
                    }
                    FREE(result);
                    FREE(key_str);
                    FREE(val_str);
                    result = temp;
                    
                    entry = entry->next;
                }
            }
            
            /* Close mapping */
            temp = MALLOC(strlen(result) + 3);
            sprintf(temp, "%s])", result);
            FREE(result);
            return temp;
            
        case OBJECT:
            /* Save object as path string */
            if (value->value.objptr && value->value.objptr->parent && 
                value->value.objptr->parent->pathname) {
                escaped = escape_string(value->value.objptr->parent->pathname);
                result = MALLOC(strlen(escaped) + 3);
                sprintf(result, "\"%s\"", escaped);
                FREE(escaped);
                return result;
            }
            return copy_string("0");
            
        default:
            /* Unsupported type - return 0 */
            {
                char logbuf[256];
                sprintf(logbuf, "save_value: unsupported type %d", value->type);
                logger(LOG_WARNING, logbuf);
            }
            return copy_string("0");
    }
}
