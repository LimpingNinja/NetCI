/* token_parse.c - Token parser helpers for value deserialization
 *
 * Contains parser functions that build values directly (not bytecode):
 * - create_string_filptr() - Create mock filptr for string parsing
 * - parse_value() - Parse any value type
 * - parse_array_value() - Parse array literals
 * - parse_mapping_value() - Parse mapping literals
 */

#include "config.h"
#include "object.h"
#include "protos.h"
#include "token.h"
#include "instr.h"
#include "file.h"
#include "globals.h"
#include "constrct.h"
#include <string.h>
#include <stdio.h>

/* Forward declarations */
int parse_value(filptr *file_info, struct var *result);
int parse_array_value(filptr *file_info, struct var *result);
int parse_mapping_value(filptr *file_info, struct var *result);

/* ========================================================================
 * FILPTR MOCKING FOR STRING PARSING
 * ======================================================================== */

/* Create a mock filptr that reads from a string
 * Opens /dev/null as dummy file to prevent crash
 * Returns: filptr pointer (caller must call free_string_filptr)
 */
filptr *create_string_filptr(char *str) {
    filptr *fp;
    FILE *dummy;
    
    fp = MALLOC(sizeof(filptr));
    if (!fp) return NULL;
    
    /* Open /dev/null as a dummy file to prevent crash if tokenizer tries to read from it */
    dummy = fopen("/dev/null", "r");
    if (!dummy) {
        FREE(fp);
        return NULL;
    }
    
    /* Initialize fields */
    memset(fp, 0, sizeof(filptr));
    fp->curr_file = dummy;             /* Dummy file that returns EOF immediately */
    fp->expanded = str;                /* Point directly to LPC string (don't free!) */
    fp->phys_line = 1;
    
    return fp;
}

/* Free a mock filptr created by create_string_filptr */
void free_string_filptr(filptr *fp) {
    if (fp) {
        /* Close the dummy file */
        if (fp->curr_file) fclose(fp->curr_file);
        /* Don't free expanded - it's managed by LPC */
        FREE(fp);
    }
}

/* ========================================================================
 * VALUE PARSING (BUILD VALUES, NOT BYTECODE)
 * ======================================================================== */

/* Parse and BUILD a value (not generate code)
 * Handles: integers, strings, arrays, mappings
 * Returns: 0 on success, 1 on error
 */
int parse_value(filptr *file_info, struct var *result) {
    token_t token;
    int is_negative = 0;
    
    /* Safety check: if expanded is NULL or empty, fail */
    if (!file_info->expanded || !*(file_info->expanded)) {
        logger(LOG_ERROR, "parse_value: no data to parse");
        return 1;
    }
    
    get_token(file_info, &token);
    
    /* Handle negative numbers: check for minus operator followed by integer */
    if (token.type == MIN_OPER) {
        is_negative = 1;
        get_token(file_info, &token);
    }
    
    /* Integer literal */
    if (token.type == INTEGER_TOK) {
        result->type = INTEGER;
        result->value.integer = is_negative ? -token.token_data.integer : token.token_data.integer;
        return 0;
    }
    
    /* String literal */
    if (token.type == STRING_TOK) {
        result->type = STRING;
        result->value.string = copy_string(token.token_data.name);
        return 0;
    }
    
    /* Array literal: ({...}) */
    if (token.type == LARRASGN_TOK) {
        return parse_array_value(file_info, result);
    }
    
    /* Mapping literal: ([...]) */
    if (token.type == LMAPSGN_TOK) {
        return parse_mapping_value(file_info, result);
    }
    
    /* Handle ( followed by { or [ - the tokenizer gives us LPAR_TOK separately */
    if (token.type == LPAR_TOK) {
        token_t next_token;
        get_token(file_info, &next_token);
        
        if (next_token.type == LBRACK_TOK) {
            /* It's an array: ({ */
            return parse_array_value(file_info, result);
        } else if (next_token.type == LARRAY_TOK) {
            /* It's a mapping: ([ */
            return parse_mapping_value(file_info, result);
        } else {
            /* Not a literal we recognize */
            unget_token(file_info, &next_token);
            {
                char logbuf[256];
                sprintf(logbuf, "parse_value: unexpected token after LPAR: %d", next_token.type);
                logger(LOG_ERROR, logbuf);
            }
            return 1;
        }
    }
    
    /* EOF or unknown token */
    {
        char logbuf[256];
        sprintf(logbuf, "parse_value: unexpected token type %d", token.type);
        logger(LOG_ERROR, logbuf);
    }
    return 1;
}

/* Parse array literal and build heap_array directly
 * Expects to be called after LARRASGN_TOK or after LPAR_TOK + LBRACK_TOK
 * Returns: 0 on success, 1 on error
 */
int parse_array_value(filptr *file_info, struct var *result) {
    token_t token;
    struct heap_array *arr;
    struct var elem;
    int elem_count = 0;
    struct var *temp_elements;
    int capacity = 16;
    int i;
    
    /* Allocate temporary storage for elements */
    temp_elements = MALLOC(sizeof(struct var) * capacity);
    if (!temp_elements) return 1;
    
    get_token(file_info, &token);
    
    /* Empty array: ({}) */
    if (token.type == RARRASGN_TOK) {
        arr = allocate_array(0, UNLIMITED_ARRAY_SIZE);
        if (!arr) {
            FREE(temp_elements);
            return 1;
        }
        result->type = ARRAY;
        result->value.array_ptr = arr;
        FREE(temp_elements);
        return 0;
    }
    
    /* Check for EOF */
    if (token.type == EOF_TOK) {
        FREE(temp_elements);
        logger(LOG_ERROR, "parse_array_value: unexpected EOF");
        return 1;
    }
    
    /* Parse elements */
    unget_token(file_info, &token);
    do {
        /* Grow temp storage if needed */
        if (elem_count >= capacity) {
            capacity *= 2;
            temp_elements = (struct var *) realloc(temp_elements, sizeof(struct var) * capacity);
            if (!temp_elements) return 1;
        }
        
        /* Parse element recursively */
        if (parse_value(file_info, &elem)) {
            /* Clean up on error */
            for (i = 0; i < elem_count; i++) {
                clear_var(&temp_elements[i]);
            }
            FREE(temp_elements);
            return 1;
        }
        
        temp_elements[elem_count++] = elem;
        
        get_token(file_info, &token);
    } while (token.type == COMMA_TOK);
    
    /* Check for }) - either as RARRASGN_TOK or RBRACK_TOK followed by RPAR_TOK */
    if (token.type == RARRASGN_TOK) {
        /* Got }) as single token - we're done */
    } else if (token.type == RBRACK_TOK) {
        /* Got } - need to consume ) */
        get_token(file_info, &token);
        if (token.type != RPAR_TOK) {
            /* Clean up on error */
            for (i = 0; i < elem_count; i++) {
                clear_var(&temp_elements[i]);
            }
            FREE(temp_elements);
            char logbuf[256];
            sprintf(logbuf, "parse_array_value: expected ) after }, got token %d", token.type);
            logger(LOG_ERROR, logbuf);
            return 1;
        }
    } else {
        /* Clean up on error */
        for (i = 0; i < elem_count; i++) {
            clear_var(&temp_elements[i]);
        }
        FREE(temp_elements);
        char logbuf[256];
        sprintf(logbuf, "parse_array_value: expected }), got token %d", token.type);
        logger(LOG_ERROR, logbuf);
        return 1;
    }
    
    /* Create heap array and copy elements */
    arr = allocate_array(elem_count, UNLIMITED_ARRAY_SIZE);
    if (!arr) {
        for (i = 0; i < elem_count; i++) {
            clear_var(&temp_elements[i]);
        }
        FREE(temp_elements);
        return 1;
    }
    
    for (i = 0; i < elem_count; i++) {
        arr->elements[i] = temp_elements[i];
    }
    
    FREE(temp_elements);
    
    result->type = ARRAY;
    result->value.array_ptr = arr;
    return 0;
}

/* Parse mapping literal and build heap_mapping directly
 * Expects to be called after LMAPSGN_TOK or after LPAR_TOK + LARRAY_TOK
 * Returns: 0 on success, 1 on error
 */
int parse_mapping_value(filptr *file_info, struct var *result) {
    token_t token;
    struct heap_mapping *map;
    struct var key, value;
    
    /* Create empty mapping */
    map = allocate_mapping(DEFAULT_MAPPING_CAPACITY);
    if (!map) return 1;
    
    get_token(file_info, &token);
    
    /* Empty mapping: ([]) */
    if (token.type == RARRAY_TOK) {
        get_token(file_info, &token);  /* Consume ) */
        if (token.type != RPAR_TOK) {
            mapping_release(map);
            char logbuf[256];
            sprintf(logbuf, "parse_mapping_value: expected ) after ], got token %d", token.type);
            logger(LOG_ERROR, logbuf);
            return 1;
        }
        result->type = MAPPING;
        result->value.mapping_ptr = map;
        return 0;
    }
    
    /* Parse key:value pairs */
    unget_token(file_info, &token);
    do {
        /* Parse key */
        if (parse_value(file_info, &key)) {
            mapping_release(map);
            return 1;
        }
        
        /* Expect colon */
        get_token(file_info, &token);
        if (token.type != COLON_TOK) {
            clear_var(&key);
            mapping_release(map);
            char logbuf[256];
            sprintf(logbuf, "parse_mapping_value: expected :, got token %d", token.type);
            logger(LOG_ERROR, logbuf);
            return 1;
        }
        
        /* Parse value */
        if (parse_value(file_info, &value)) {
            clear_var(&key);
            mapping_release(map);
            return 1;
        }
        
        /* Insert into mapping */
        mapping_set(map, &key, &value);
        
        /* Clean up key/value (mapping made copies) */
        clear_var(&key);
        clear_var(&value);
        
        get_token(file_info, &token);
    } while (token.type == COMMA_TOK);
    
    if (token.type != RARRAY_TOK) {
        char logbuf[256];
        mapping_release(map);
        sprintf(logbuf, "parse_mapping_value: expected ], got token %d", token.type);
        logger(LOG_ERROR, logbuf);
        return 1;
    }
    
    get_token(file_info, &token);  /* Consume ) */
    if (token.type != RPAR_TOK) {
        char logbuf[256];
        mapping_release(map);
        sprintf(logbuf, "parse_mapping_value: expected ) after ], got token %d", token.type);
        logger(LOG_ERROR, logbuf);
        return 1;
    }
    
    result->type = MAPPING;
    result->value.mapping_ptr = map;
    return 0;
}
