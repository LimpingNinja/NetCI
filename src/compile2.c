/* compile.c */

/* This file contains the functions to compile CI-C into an easily
   interpreted form */

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

  c_err_msg=NULL;
  buf=(char *) MALLOC(strlen(filename)+3);
  strcpy(buf,filename);
  strcat(buf,".c");
  if (!(file_info.curr_file=open_file(buf,FREAD_MODE,NULL))) {
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
  file_info.depth=0;
  glob_sym.num=0;
  glob_sym.varlist=NULL;
  file_info.glob_sym=&glob_sym;
  file_info.phys_line=0;
  file_info.defs=NULL;
  line_num=top_level_parse(&file_info);
  close_file(file_info.curr_file);
  file_info.curr_code->gst=glob_sym.varlist;
  file_info.curr_code->num_globals=glob_sym.num;
  free_file_stack(&file_info);
  free_define(&file_info);
  if (line_num) {
    free_code(file_info.curr_code);
    return line_num;
  } else {
    *result=file_info.curr_code;
    return 0;
  }
}
