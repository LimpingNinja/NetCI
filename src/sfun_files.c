/* sfun_files.c - LPC-standard filesystem functions for NetCI
 *
 * This module provides standard LPC filesystem functions with proper naming
 * conventions compatible with MudOS, DGD, and LDMud drivers.
 *
 * Functions:
 *   read_file()  - Read lines from a file
 *   write_file() - Write string to file
 *   remove()     - Delete a file
 *   rename()     - Rename/move a file
 *   get_dir()    - Get directory listing as array
 *   file_size()  - Get file size in bytes
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "config.h"
#include "autoconf.h"
#include "stdinc.h"
#include "tune.h"
#include "ci.h"
#include "object.h"
#include "protos.h"
#include "instr.h"
#include "constrct.h"
#include "file.h"
#include "intrface.h"

/* read_file() - Read lines from a file
 * 
 * Syntax: string read_file(string path)
 *         string read_file(string path, int start_line)
 *         string read_file(string path, int start_line, int num_lines)
 * 
 * Returns: String containing the file contents, or 0 on error
 */
int s_read_file(struct object *caller, struct object *obj,
                struct object *player, struct var_stack **rts) {
    struct var tmp_num_args, tmp_path, tmp_start, tmp_lines;
    FILE *f;
    int num_args, start_line = 1, num_lines = 0;
    int current_line = 1;
    char line_buf[MAX_STR_LEN];
    char *result = NULL;
    int result_len = 0;
    int result_capacity = 0;
    
    /* Pop NUM_ARGS */
    if (pop(&tmp_num_args, rts, obj)) return 1;
    if (tmp_num_args.type != NUM_ARGS) {
        clear_var(&tmp_num_args);
        return 1;
    }
    num_args = tmp_num_args.value.num;
    
    /* Handle optional num_lines argument */
    if (num_args == 3) {
        if (pop(&tmp_lines, rts, obj)) return 1;
        if (tmp_lines.type != INTEGER) {
            clear_var(&tmp_lines);
            return 1;
        }
        num_lines = tmp_lines.value.integer;
    }
    
    /* Handle optional start_line argument */
    if (num_args >= 2) {
        if (pop(&tmp_start, rts, obj)) return 1;
        if (tmp_start.type != INTEGER) {
            clear_var(&tmp_start);
            return 1;
        }
        start_line = tmp_start.value.integer;
        if (start_line < 1) start_line = 1;
    }
    
    /* Get path argument */
    if (num_args < 1 || num_args > 3) return 1;
    if (pop(&tmp_path, rts, obj)) return 1;
    if (tmp_path.type != STRING) {
        clear_var(&tmp_path);
        goto error_return;
    }
    
    /* Open file */
    f = open_file(tmp_path.value.string, FREAD_MODE, obj);
    clear_var(&tmp_path);
    
    if (!f) {
        goto error_return;
    }
    
    /* Allocate initial result buffer */
    result_capacity = 1024;
    result = (char *)MALLOC(result_capacity);
    if (!result) {
        close_file(f);
        goto error_return;
    }
    result[0] = '\0';
    result_len = 0;
    
    /* Read file */
    while (fgets(line_buf, MAX_STR_LEN, f)) {
        if (current_line >= start_line) {
            /* Check if we've read enough lines */
            if (num_lines > 0 && current_line >= start_line + num_lines) {
                break;
            }
            
            /* Append line to result */
            int line_len = strlen(line_buf);
            while (result_len + line_len + 1 > result_capacity) {
                result_capacity *= 2;
                result = (char *)realloc(result, result_capacity);
                if (!result) {
                    close_file(f);
                    goto error_return;
                }
            }
            strcpy(result + result_len, line_buf);
            result_len += line_len;
        }
        current_line++;
    }
    
    close_file(f);
    
    /* Push result */
    tmp_path.type = STRING;
    tmp_path.value.string = result;
    push(&tmp_path, rts);
    return 0;
    
error_return:
    /* Return 0 on error */
    tmp_path.type = INTEGER;
    tmp_path.value.integer = 0;
    push(&tmp_path, rts);
    return 0;
}

/* write_file() - Write string to file (append mode)
 * 
 * Syntax: int write_file(string path, string content)
 * 
 * Returns: 1 on success, 0 on error
 */
int s_write_file(struct object *caller, struct object *obj,
                 struct object *player, struct var_stack **rts) {
    struct var tmp_num_args, tmp_path, tmp_content;
    FILE *f;
    int retval;
    
    /* Pop NUM_ARGS */
    if (pop(&tmp_num_args, rts, obj)) return 1;
    if (tmp_num_args.type != NUM_ARGS) {
        clear_var(&tmp_num_args);
        return 1;
    }
    if (tmp_num_args.value.num != 2) return 1;
    
    /* Pop content */
    if (pop(&tmp_content, rts, obj)) return 1;
    if (tmp_content.type != STRING) {
        clear_var(&tmp_content);
        return 1;
    }
    
    /* Pop path */
    if (pop(&tmp_path, rts, obj)) return 1;
    if (tmp_path.type != STRING) {
        clear_var(&tmp_path);
        clear_var(&tmp_content);
        goto error_return;
    }
    
    /* Open file in append mode */
    f = open_file(tmp_path.value.string, FAPPEND_MODE, obj);
    clear_var(&tmp_path);
    
    if (!f) {
        clear_var(&tmp_content);
        goto error_return;
    }
    
    /* Write content */
    fputs(tmp_content.value.string, f);
    close_file(f);
    clear_var(&tmp_content);
    
    /* Return success */
    tmp_path.type = INTEGER;
    tmp_path.value.integer = 1;
    push(&tmp_path, rts);
    return 0;
    
error_return:
    /* Return failure */
    tmp_path.type = INTEGER;
    tmp_path.value.integer = 0;
    push(&tmp_path, rts);
    return 0;
}

/* remove() - Delete a file
 * 
 * Syntax: int remove(string path)
 * 
 * Returns: 0 on success, 1 on error
 */
int s_remove(struct object *caller, struct object *obj,
             struct object *player, struct var_stack **rts) {
    struct var tmp;
    int retval;
    
    /* Pop NUM_ARGS */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != NUM_ARGS) {
        clear_var(&tmp);
        return 1;
    }
    if (tmp.value.num != 1) return 1;
    
    /* Pop path */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != STRING) {
        clear_var(&tmp);
        return 1;
    }
    
    /* Remove file */
    retval = remove_file(tmp.value.string, obj);
    clear_var(&tmp);
    
    /* Push result */
    tmp.type = INTEGER;
    tmp.value.integer = retval;
    push(&tmp, rts);
    return 0;
}

/* rename() - Rename/move a file
 * 
 * Syntax: int rename(string old_path, string new_path)
 * 
 * Returns: 0 on success, 1 on error
 */
int s_rename(struct object *caller, struct object *obj,
             struct object *player, struct var_stack **rts) {
    struct var tmp_num_args, tmp_old, tmp_new;
    int retval;
    
    /* Pop NUM_ARGS */
    if (pop(&tmp_num_args, rts, obj)) return 1;
    if (tmp_num_args.type != NUM_ARGS) {
        clear_var(&tmp_num_args);
        return 1;
    }
    if (tmp_num_args.value.num != 2) return 1;
    
    /* Pop new path */
    if (pop(&tmp_new, rts, obj)) return 1;
    if (tmp_new.type != STRING) {
        clear_var(&tmp_new);
        return 1;
    }
    
    /* Pop old path */
    if (pop(&tmp_old, rts, obj)) {
        clear_var(&tmp_new);
        return 1;
    }
    if (tmp_old.type != STRING) {
        clear_var(&tmp_old);
        clear_var(&tmp_new);
        return 1;
    }
    
    /* Move/rename file */
    retval = move_file(tmp_old.value.string, tmp_new.value.string, obj);
    clear_var(&tmp_old);
    clear_var(&tmp_new);
    
    /* Push result */
    tmp_old.type = INTEGER;
    tmp_old.value.integer = retval;
    push(&tmp_old, rts);
    return 0;
}

/* get_dir() - Get directory listing as array
 * 
 * Syntax: string *get_dir(string path)
 * 
 * Returns: Array of filenames, or 0 on error
 */
int s_get_dir(struct object *caller, struct object *obj,
              struct object *player, struct var_stack **rts) {
    struct var tmp;
    struct file_entry *fe, *entry;
    struct heap_array *arr;
    int count = 0;
    int i = 0;
    
    /* Pop NUM_ARGS */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != NUM_ARGS) {
        clear_var(&tmp);
        return 1;
    }
    if (tmp.value.num != 1) return 1;
    
    /* Pop path */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != STRING) {
        clear_var(&tmp);
        goto error_return;
    }
    
    /* Validate path */
    if (!validate_path(tmp.value.string)) {
        clear_var(&tmp);
        goto error_return;
    }
    
    /* Use ls_dir logic - discover and validate */
    discover_directory(tmp.value.string, obj);
    
    /* Check master permission */
    if (!check_master_permission(tmp.value.string, "get_dir", obj, 0)) {
        clear_var(&tmp);
        goto error_return;
    }
    
    /* Find directory entry using internal file.c function */
    extern struct file_entry *find_entry(char *filename);
    extern int can_read(struct file_entry *fe, struct object *uid);
    
    fe = find_entry(tmp.value.string);
    clear_var(&tmp);
    
    if (!fe) goto error_return;
    if (!can_read(fe, obj)) goto error_return;
    if (!(fe->flags & DIRECTORY)) goto error_return;
    
    /* Count entries */
    entry = fe->contents;
    while (entry) {
        count++;
        entry = entry->next_file;
    }
    
    /* Allocate array with unlimited size */
    arr = allocate_array(count, UNLIMITED_ARRAY_SIZE);
    if (!arr) goto error_return;
    
    /* Fill array with filenames */
    entry = fe->contents;
    i = 0;
    while (entry && i < count) {
        arr->elements[i].type = STRING;
        arr->elements[i].value.string = (char *)MALLOC(strlen(entry->filename) + 1);
        if (!arr->elements[i].value.string) {
            /* Cleanup on allocation failure */
            while (i > 0) {
                i--;
                FREE(arr->elements[i].value.string);
            }
            array_release(arr);
            goto error_return;
        }
        strcpy(arr->elements[i].value.string, entry->filename);
        i++;
        entry = entry->next_file;
    }
    
    /* Push array */
    tmp.type = ARRAY;
    tmp.value.array_ptr = arr;
    push(&tmp, rts);
    return 0;
    
error_return:
    /* Return 0 on error */
    tmp.type = INTEGER;
    tmp.value.integer = 0;
    push(&tmp, rts);
    return 0;
}

/* file_size() - Get file size in bytes
 * 
 * Syntax: int file_size(string path)
 * 
 * Returns: File size in bytes, or -1 on error
 */
int s_file_size(struct object *caller, struct object *obj,
                struct object *player, struct var_stack **rts) {
    struct var tmp;
    struct stat st;
    char *full_path;
    extern char *fs_path;
    int result;
    
    /* Pop NUM_ARGS */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != NUM_ARGS) {
        clear_var(&tmp);
        return 1;
    }
    if (tmp.value.num != 1) return 1;
    
    /* Pop path */
    if (pop(&tmp, rts, obj)) return 1;
    if (tmp.type != STRING) {
        clear_var(&tmp);
        goto error_return;
    }
    
    /* Validate path */
    if (!validate_path(tmp.value.string)) {
        clear_var(&tmp);
        goto error_return;
    }
    
    /* Check master permission */
    if (!check_master_permission(tmp.value.string, "file_size", obj, 0)) {
        clear_var(&tmp);
        goto error_return;
    }
    
    /* Build full filesystem path */
    full_path = (char *)MALLOC(strlen(fs_path) + strlen(tmp.value.string) + 1);
    if (!full_path) {
        clear_var(&tmp);
        goto error_return;
    }
    sprintf(full_path, "%s%s", fs_path, tmp.value.string);
    
    /* Get file stats */
    result = stat(full_path, &st);
    FREE(full_path);
    clear_var(&tmp);
    
    if (result != 0) {
        goto error_return;
    }
    
    /* Return file size */
    tmp.type = INTEGER;
    tmp.value.integer = (int)st.st_size;
    push(&tmp, rts);
    return 0;
    
error_return:
    /* Return -1 on error */
    tmp.type = INTEGER;
    tmp.value.integer = -1;
    push(&tmp, rts);
    return 0;
}
