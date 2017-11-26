/* sys6b.c */

#include "config.h"
#include "object.h"
#include "protos.h"
#include "instr.h"
#include "constrct.h"
#include "dbhandle.h"

int s_atoo(struct object *caller, struct object *obj, struct object *player,
           struct var_stack **rts) {
  struct var tmp;
  struct object *result;
  char *pathbuf,*numbuf;
  long breakpoint;
  signed long expected_refno;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=1) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type==INTEGER && tmp.value.integer==0) {
    tmp.type=STRING;
    *(tmp.value.string=MALLOC(1))='\0';
  }
  if (tmp.type!=STRING) {
    clear_var(&tmp);
    return 1;
  }
  breakpoint=0;
  pathbuf=tmp.value.string;
  numbuf="";
  while (tmp.value.string[breakpoint]) {
    if (tmp.value.string[breakpoint]=='#') {
      numbuf=&(tmp.value.string[breakpoint+1]);
      tmp.value.string[breakpoint]='\0';
      break;
    }
    breakpoint++;
  }
  if (*numbuf=='\0') {
    result=find_proto(pathbuf);
  } else {
    expected_refno=atol(numbuf);
    result=ref_to_obj(expected_refno);
    if (result)
      if (*pathbuf)
        if (strcmp(pathbuf,result->parent->pathname))
          result=NULL;
  }
  clear_var(&tmp);
  if (result) {
    tmp.type=OBJECT;
    tmp.value.objptr=result;
  } else {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  }
  push(&tmp,rts);
  return 0;
}

int s_upcase(struct object *caller, struct object *obj, struct object *player,
             struct var_stack **rts) {
  struct var tmp;
  long loop,len;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=1) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type==INTEGER && tmp.value.integer==0) {
    push(&tmp,rts);
    return 0;
  }
  if (tmp.type!=STRING) {
    clear_var(&tmp);
    return 1;
  }
  loop=0;
  len=strlen(tmp.value.string);
  while (loop<len) {
    if (islower(tmp.value.string[loop]))
      tmp.value.string[loop]=toupper(tmp.value.string[loop]);
    loop++;
  }
  pushnocopy(&tmp,rts);
  return 0;
}

int s_downcase(struct object *caller, struct object *obj, struct object
               *player, struct var_stack **rts) {
  struct var tmp;
  long loop,len;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=1) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type==INTEGER && tmp.value.integer==0) {
    push(&tmp,rts);
    return 0;
  }
  if (tmp.type!=STRING) {
    clear_var(&tmp);
    return 1;
  }
  loop=0;
  len=strlen(tmp.value.string);
  while (loop<len) {
    if (isupper(tmp.value.string[loop]))
      tmp.value.string[loop]=tolower(tmp.value.string[loop]);
    loop++;
  }
  pushnocopy(&tmp,rts);
  return 0;
}

int s_is_legal(struct object *caller, struct object *obj,
               struct object *player, struct var_stack **rts) {
  struct var tmp;
  long result;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=1) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type==INTEGER && tmp.value.integer==0) {
    push(&tmp,rts);
    return 0;
  }
  if (tmp.type!=STRING) {
    clear_var(&tmp);
    return 1;
  }
  result=is_legal(tmp.value.string);
  clear_var(&tmp);
  tmp.type=INTEGER;
  tmp.value.integer=result;
  push(&tmp,rts);
  return 0;
}

int s_otoi(struct object *caller, struct object *obj, struct object *player,
           struct var_stack **rts) {
  struct var tmp;
  signed long result;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=1) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=OBJECT) {
    clear_var(&tmp);
    return 1;
  }
  result=tmp.value.objptr->refno;
  tmp.type=INTEGER;
  tmp.value.integer=result;
  push(&tmp,rts);
  return 0;
}

int s_itoo(struct object *caller, struct object *obj, struct object *player,
           struct var_stack **rts) {
  struct var tmp;
  struct object *result;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=1) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=INTEGER) {
    clear_var(&tmp);
    return 1;
  }
  result=ref_to_obj(tmp.value.integer);
  if (result) {
    tmp.type=OBJECT;
    tmp.value.objptr=result;
  } else {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  }
  push(&tmp,rts);
  return 0;
}

int s_chr(struct object *caller, struct object *obj, struct object *player,
          struct var_stack **rts) {
  struct var tmp;
  char *buf;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=1) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=INTEGER) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.integer<0 || tmp.value.integer>SCHAR_MAX) {
    tmp.value.integer=0;
    push(&tmp,rts);
    return 0;
  }
  if (!isgraph(((unsigned char) tmp.value.integer)) &&
      !isspace(((unsigned char) tmp.value.integer))) {
    tmp.value.integer=0;
    push(&tmp,rts);
    return 0;
  }
  buf=MALLOC(2);
  buf[0]=(char) tmp.value.integer;
  buf[1]='\0';
  tmp.type=STRING;
  tmp.value.string=buf;
  pushnocopy(&tmp,rts);
  return 0;
}

int s_asc(struct object *caller, struct object *obj, struct object *player,
          struct var_stack **rts) {
  struct var tmp;
  long result;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=1) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type==INTEGER && tmp.value.integer==0) {
    push(&tmp,rts);
    return 0;
  }
  if (tmp.type!=STRING) {
    clear_var(&tmp);
    return 1;
  }
  result=*(tmp.value.string);
  clear_var(&tmp);
  tmp.type=INTEGER;
  tmp.value.integer=result;
  push(&tmp,rts);
  return 0;
}
