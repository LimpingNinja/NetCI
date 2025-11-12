/* compile.c */

/* This file contains the functions to compile CI-C into an easily
   interpreted form */

#include <time.h>
#include "config.h"
#include "object.h"
#include "instr.h"
#include "constrct.h"
#include "file.h"
#include "token.h"
#include "globals.h"

unsigned int parse_code(char *filename, struct object *caller_obj,
                        struct code **result)
{
  sym_tab_t glob_sym;
  unsigned int line_num;
  filptr file_info;
  char *buf;
  char logbuf[512];
  int is_boot;
  long start_time, end_time;

  c_err_msg=NULL;
  buf=(char *) MALLOC(strlen(filename)+3);
  strcpy(buf,filename);
  strcat(buf,".c");
  
  /* Check if compiling boot.c for logging */
  is_boot = (strcmp(filename, "/boot") == 0);
  
  if (is_boot) {
    sprintf(logbuf, "COMPILE: Starting compilation of boot.c");
    logger(LOG_INFO, logbuf);
    start_time = time(NULL);
  }
  
  if (!(file_info.curr_file=open_file(buf,FREAD_MODE,NULL))) {
    if (is_boot) {
      logger(LOG_ERROR, "COMPILE: boot.c FAILED - could not open file");
    }
    FREE(buf);
    return (unsigned int) -1;
  }
  FREE(buf);
  ungetc('\n',file_info.curr_file);
  file_info.previous=NULL;
  file_info.is_put_back=0;
  file_info.expanded=NULL;
  file_info.curr_code=(struct code *) MALLOC(sizeof(struct code));
  file_info.curr_code->num_refs=0;
  file_info.curr_code->num_globals=0;
  file_info.curr_code->func_list=NULL;
  file_info.curr_code->gst=NULL;
  file_info.curr_code->own_vars=NULL;  /* Initialize own_vars list */
  file_info.curr_code->inherits=NULL;
  /* Initialize ancestor map metadata */
  file_info.curr_code->ancestor_map=NULL;
  file_info.curr_code->ancestor_count=0;
  file_info.curr_code->ancestor_capacity=0;
  file_info.curr_code->self_var_offset=0;
  /* Initialize GST ref mapping */
  file_info.curr_code->gst_map=NULL;
  file_info.curr_code->gst_count=0;
  file_info.depth=0;
  file_info.layout_locked=0;  /* Initialize layout_locked flag */
  glob_sym.num=0;
  glob_sym.varlist=NULL;
  file_info.glob_sym=&glob_sym;
  file_info.phys_line=0;
  file_info.defs=NULL;
  line_num=top_level_parse(&file_info);
  close_file(file_info.curr_file);
  file_info.curr_code->gst=glob_sym.varlist;
  file_info.curr_code->num_globals=glob_sym.num;
  
  /* Build GST ref mapping: gst[base] -> {owner_proto, owner_local_index} */
  if (file_info.curr_code->num_globals) {
    struct var_tab *v = file_info.curr_code->gst;
    file_info.curr_code->gst_map = (struct gst_ref_entry *)MALLOC(sizeof(struct gst_ref_entry) * file_info.curr_code->num_globals);
    file_info.curr_code->gst_count = (unsigned short)file_info.curr_code->num_globals;
    /* Initialize to defaults */
    for (unsigned int i = 0; i < file_info.curr_code->num_globals; i++) {
      file_info.curr_code->gst_map[i].owner = NULL;
      file_info.curr_code->gst_map[i].local_index = 0;
    }
    /* Populate from gst linked list */
    while (v) {
      unsigned int idx = v->base;
      if (idx < file_info.curr_code->num_globals) {
        file_info.curr_code->gst_map[idx].owner = v->origin_prog; /* NULL => self */
        
        /* For own variables, look up correct local_index from own_vars */
        if (v->origin_prog == NULL) {
          /* Find this variable in own_vars to get correct local index */
          struct var_tab *own = file_info.curr_code->own_vars;
          int found = 0;
          while (own) {
            if (!strcmp(own->name, v->name)) {
              file_info.curr_code->gst_map[idx].local_index = own->owner_local_index;
              found = 1;
              break;
            }
            own = own->next;
          }
          if (!found) {
            /* Shouldn't happen, but default to 0 */
            file_info.curr_code->gst_map[idx].local_index = 0;
          }
        } else {
          /* Inherited variable - use the owner_local_index as-is */
          file_info.curr_code->gst_map[idx].local_index = v->owner_local_index;
        }
      }
      v = v->next;
    }
  }
  
  /* Log own_vars vs all_vars for debugging */
  /* COMMENTED OUT - Enable for inheritance debugging
  {
    char logbuf[512];
    sprintf(logbuf, "COMPILE COMPLETE: proto has %u total globals", file_info.curr_code->num_globals);
    logger(LOG_DEBUG, logbuf);
    
    sprintf(logbuf, "  OWN_VARS:");
    logger(LOG_DEBUG, logbuf);
    struct var_tab *own = file_info.curr_code->own_vars;
    int own_count = 0;
    while (own) {
      sprintf(logbuf, "    - %s (slot=%u)", own->name, own->base);
      logger(LOG_DEBUG, logbuf);
      own_count++;
      own = own->next;
    }
    sprintf(logbuf, "  Total own_vars: %d", own_count);
    logger(LOG_DEBUG, logbuf);
    
    sprintf(logbuf, "  ALL_VARS (gst):");
    logger(LOG_DEBUG, logbuf);
    struct var_tab *all = file_info.curr_code->gst;
    int all_count = 0;
    while (all) {
      sprintf(logbuf, "    - %s (slot=%u, origin=%s)", 
              all->name, all->base,
              all->origin_prog ? (all->origin_prog->pathname ? all->origin_prog->pathname : "unknown") : "current");
      logger(LOG_DEBUG, logbuf);
      all_count++;
      all = all->next;
    }
    sprintf(logbuf, "  Total all_vars: %d", all_count);
    logger(LOG_DEBUG, logbuf);
  }
  */
  
  free_file_stack(&file_info);
  free_define(&file_info);
  if (line_num) {
    if (is_boot) {
      sprintf(logbuf, "COMPILE: boot.c FAILED at line %u - %s", line_num, c_err_msg ? c_err_msg : "unknown error");
      logger(LOG_ERROR, logbuf);
    }
    free_code(file_info.curr_code);
    return line_num;
  } else {
    if (is_boot) {
      end_time = time(NULL);
      sprintf(logbuf, "COMPILE: boot.c compiled successfully (%ld seconds, %u globals, %p funcs)", 
              end_time - start_time, file_info.curr_code->num_globals, (void*)file_info.curr_code->func_list);
      logger(LOG_INFO, logbuf);
    }
    *result=file_info.curr_code;
    return 0;
  }
}
