/* sys6a.c */

#include "config.h"
#include "object.h"
#include "protos.h"
#include "instr.h"
#include "constrct.h"

/* s_sscanf() moved to sfun_strings.c */
/* s_sprintf() moved to sfun_sprintf.c */

int s_midstr(struct object *caller, struct object *obj, struct object *player,
             struct var_stack **rts) {
  struct var tmp1,tmp2,tmp3;
  char *buf;
  long len,loop;

  if (pop(&tmp3,rts,obj)) return 1;
  if (tmp3.type!=NUM_ARGS) {
    clear_var(&tmp3);
    return 1;
  }
  if (tmp3.value.num!=3) return 1;
  if (pop(&tmp3,rts,obj)) return 1;
  if (tmp3.type!=INTEGER) {
    clear_var(&tmp3);
    return 1;
  }
  if (pop(&tmp2,rts,obj)) return 1;
  if (tmp2.type!=INTEGER) {
    clear_var(&tmp2);
    return 1;
  }
  if (pop(&tmp1,rts,obj)) return 1;
  if (tmp1.type==INTEGER && tmp1.value.integer==0) {
    tmp1.type=STRING;
    *(tmp1.value.string=MALLOC(1))='\0';
  }
  if (tmp1.type!=STRING) {
    clear_var(&tmp1);
    return 1;
  }
  len=strlen(tmp1.value.string);
  if (tmp2.value.integer<1 || tmp3.value.integer<1 || tmp2.value.integer>len) {
    clear_var(&tmp1);
    tmp1.type=INTEGER;
    tmp1.value.integer=0;
    push(&tmp1,rts);
    return 0;
  }
  if (tmp3.value.integer+tmp2.value.integer-1>len)
    tmp3.value.integer=len-tmp2.value.integer+1;
  loop=0;
  buf=MALLOC(tmp3.value.integer+1);
  while (loop<tmp3.value.integer) {
    buf[loop]=tmp1.value.string[tmp2.value.integer-1+loop];
    loop++;
  }
  buf[loop]='\0';
  clear_var(&tmp1);
  tmp1.type=STRING;
  tmp1.value.string=buf;
  if (*(tmp1.value.string)=='\0') {
    clear_var(&tmp1);
    tmp1.type=INTEGER;
    tmp1.value.integer=0;
  }
  pushnocopy(&tmp1,rts);
  return 0;
}

int s_strlen(struct object *caller, struct object *obj, struct object *player,
             struct var_stack **rts) {
  struct var tmp;
  long retval;

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
  retval=strlen(tmp.value.string);
  clear_var(&tmp);
  tmp.type=INTEGER;
  tmp.value.integer=retval;
  push(&tmp,rts);
  return 0;
}

int s_leftstr(struct object *caller, struct object *obj, struct object *player,
              struct var_stack **rts) {
  struct var tmp1,tmp2;

  if (pop(&tmp2,rts,obj)) return 1;
  if (tmp2.type!=NUM_ARGS) {
    clear_var(&tmp2);
    return 1;
  }
  if (tmp2.value.num!=2) return 1;
  if (pop(&tmp2,rts,obj)) return 1;
  if (tmp2.type!=INTEGER) {
    clear_var(&tmp2);
    return 1;
  }
  if (pop(&tmp1,rts,obj)) return 1;
  if (tmp1.type==INTEGER && tmp1.value.integer==0) {
    push(&tmp1,rts);
    return 0;
  }
  if (tmp1.type!=STRING) {
    clear_var(&tmp1);
    return 1;
  }
  if (tmp2.value.integer<1) {
    clear_var(&tmp1);
    tmp1.type=INTEGER;
    tmp1.value.integer=0;
    push(&tmp1,rts);
    return 0;
  }
  if (tmp2.value.integer<strlen(tmp1.value.string))
    tmp1.value.string[tmp2.value.integer]='\0';
  pushnocopy(&tmp1,rts);
  return 0;
}

int s_rightstr(struct object *caller, struct object *obj, struct object
               *player, struct var_stack **rts) {
  struct var tmp1,tmp2;
  long len,loop;
  char *buf;

  if (pop(&tmp2,rts,obj)) return 1;
  if (tmp2.type!=NUM_ARGS) {
    clear_var(&tmp2);
    return 1;
  }
  if (tmp2.value.num!=2) return 1;
  if (pop(&tmp2,rts,obj)) return 1;
  if (tmp2.type!=INTEGER) {
    clear_var(&tmp2);
    return 1;
  }
  if (pop(&tmp1,rts,obj)) return 1;
  if (tmp1.type==INTEGER && tmp1.value.integer==0) {
    push(&tmp1,rts);
    return 0;
  }
  if (tmp1.type!=STRING) {
    clear_var(&tmp1);
    return 1;
  }
  if (tmp2.value.integer<1) {
    clear_var(&tmp1);
    tmp1.type=INTEGER;
    tmp1.value.integer=0;
    push(&tmp1,rts);
    return 0;
  }
  len=strlen(tmp1.value.string);
  if (tmp2.value.integer>len)
    tmp2.value.integer=len;
  buf=MALLOC(len+1);
  loop=len-tmp2.value.integer;
  while (loop<len) {
    buf[loop-len+tmp2.value.integer]=tmp1.value.string[loop];
    loop++;
  }
  buf[loop-len+tmp2.value.integer]='\0';
  clear_var(&tmp1);
  tmp1.type=STRING;
  tmp1.value.string=buf;
  pushnocopy(&tmp1,rts);
  return 0;
}

int s_subst(struct object *caller, struct object *obj, struct object *player,
            struct var_stack **rts) {
  struct var tmp1,tmp2,tmp3,tmp4;
  char *buf,*first_half,*second_half;
  long len,loop;

  if (pop(&tmp4,rts,obj)) return 1;
  if (tmp4.type!=NUM_ARGS) {
    clear_var(&tmp4);
    return 1;
  }
  if (tmp4.value.num!=4) return 1;
  if (pop(&tmp4,rts,obj)) return 1;
  if (tmp4.type==INTEGER && tmp4.value.integer==0) {
    tmp4.type=STRING;
    *(tmp4.value.string=MALLOC(1))='\0';
  }
  if (tmp4.type!=STRING) {
    clear_var(&tmp4);
    return 1;
  }
  if (pop(&tmp3,rts,obj)) {
    clear_var(&tmp4);
    return 1;
  }
  if (tmp3.type!=INTEGER) {
    clear_var(&tmp4);
    clear_var(&tmp3);
    return 1;
  }
  if (pop(&tmp2,rts,obj)) {
    clear_var(&tmp4);
    return 1;
  }
  if (tmp2.type!=INTEGER) {
    clear_var(&tmp4);
    clear_var(&tmp2);
    return 1;
  }
  if (pop(&tmp1,rts,obj)) {
    clear_var(&tmp4);
    return 1;
  }
  if (tmp1.type==INTEGER && tmp1.value.integer==0) {
    tmp1.type=STRING;
    *(tmp1.value.string=MALLOC(1))='\0';
  }
  if (tmp1.type!=STRING) {
    clear_var(&tmp4);
    clear_var(&tmp1);
    return 1;
  }
  len=strlen(tmp1.value.string);
  if (tmp2.value.integer>len+1) tmp2.value.integer=len+1;
  if (tmp2.value.integer<1) tmp2.value.integer=1;
  if (tmp3.value.integer<0) tmp3.value.integer=0;
  if (tmp3.value.integer+tmp2.value.integer-1>len)
    tmp3.value.integer=len-tmp2.value.integer+1;
  --(tmp2.value.integer);
  loop=0;
  first_half=MALLOC(tmp2.value.integer+1);
  while (loop<tmp2.value.integer) {
    first_half[loop]=tmp1.value.string[loop];
    loop++;
  }
  first_half[loop]='\0';
  loop=0;
  second_half=MALLOC(len-tmp2.value.integer-tmp3.value.integer+1);
  while (loop<len-tmp2.value.integer-tmp3.value.integer+1) {
    second_half[loop]=tmp1.value.string[loop+tmp2.value.integer+
                                        tmp3.value.integer];
    loop++;
  }
  buf=MALLOC(strlen(first_half)+strlen(tmp4.value.string)+strlen(second_half)
             +1);
  strcpy(buf,first_half);
  strcat(buf,tmp4.value.string);
  strcat(buf,second_half);
  clear_var(&tmp1);
  clear_var(&tmp4);
  FREE(first_half);
  FREE(second_half);
  if (*buf) {
    tmp1.type=STRING;
    tmp1.value.string=buf;
  } else {
    tmp1.type=INTEGER;
    tmp1.value.integer=0;
    FREE(buf);
  }
  pushnocopy(&tmp1,rts);
  return 0;
}

int s_instr(struct object *caller, struct object *obj, struct object *player,
            struct var_stack **rts) {
  struct var tmp1,tmp2,tmp3;
  long loop,len1,len2,upper_limit;


  if (pop(&tmp3,rts,obj)) return 1;
  if (tmp3.type!=NUM_ARGS) {
    clear_var(&tmp3);
    return 1;
  }
  if (tmp3.value.num!=3) return 1;
  if (pop(&tmp3,rts,obj)) return 1;
  if (tmp3.type==INTEGER && tmp3.value.integer==0) {
    tmp3.type=STRING;
    *(tmp3.value.string=MALLOC(1))='\0';
  }
  if (tmp3.type!=STRING) {
    clear_var(&tmp3);
    return 1;
  }
  if (pop(&tmp2,rts,obj)) {
    clear_var(&tmp3);
    return 1;
  }
  if (tmp2.type!=INTEGER) {
    clear_var(&tmp3);
    clear_var(&tmp2);
    return 1;
  }
  if (pop(&tmp1,rts,obj)) {
    clear_var(&tmp3);
    return 1;
  }
  if (tmp1.type==INTEGER && tmp1.value.integer==0) {
    tmp1.type=STRING;
    *(tmp1.value.string=MALLOC(1))='\0';
  }
  if (tmp1.type!=STRING) {
    clear_var(&tmp3);
    clear_var(&tmp1);
    return 1;
  }
  if (tmp2.value.integer<1) tmp2.value.integer=1;
  if (*(tmp1.value.string)=='\0') {
    clear_var(&tmp1);
    clear_var(&tmp3);
    tmp1.type=INTEGER;
    tmp1.value.integer=0;
    push(&tmp1,rts);
    return 0;
  }
  len1=strlen(tmp1.value.string);
  len2=strlen(tmp3.value.string);
  if (tmp2.value.integer>len1) {
    clear_var(&tmp1);
    clear_var(&tmp3);
    tmp1.type=INTEGER;
    tmp1.value.integer=0;
    push(&tmp1,rts);
    return 0;
  }
  loop=tmp2.value.integer-1;
  upper_limit=len1-len2+1;
  while (loop<upper_limit) {
    if (!strncmp(&(tmp1.value.string[loop]),tmp3.value.string,(int) len2)) {
      clear_var(&tmp1);
      clear_var(&tmp3);
      tmp1.type=INTEGER;
      tmp1.value.integer=loop+1;
      push(&tmp1,rts);
      return 0;
    }
    loop++;
  }
  clear_var(&tmp1);
  clear_var(&tmp3);
  tmp1.type=INTEGER;
  tmp1.value.integer=0;
  push(&tmp1,rts);
  return 0;
}

int s_otoa(struct object *caller, struct object *obj, struct object *player,
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
  if (tmp.type==INTEGER && tmp.value.integer==0) {
    push(&tmp,rts);
    return 0;
  }
  if (tmp.type!=OBJECT) {
    clear_var(&tmp);
    return 1;
  }
  buf=MALLOC(strlen(tmp.value.objptr->parent->pathname)+ITOA_BUFSIZ+2);
  sprintf(buf,"%s#%ld",tmp.value.objptr->parent->pathname,
          (long) tmp.value.objptr->refno);
  clear_var(&tmp);
  tmp.type=STRING;
  tmp.value.string=buf;
  pushnocopy(&tmp,rts);
  return 0;
}

int s_itoa(struct object *caller, struct object *obj, struct object *player,
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
  buf=MALLOC(ITOA_BUFSIZ+1);
  sprintf(buf,"%ld",(long) tmp.value.integer);
  clear_var(&tmp);
  tmp.type=STRING;
  tmp.value.string=buf;
  pushnocopy(&tmp,rts);
  return 0;
}

int s_atoi(struct object *caller, struct object *obj, struct object *player,
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
  if (tmp.type==INTEGER && tmp.value.integer==0) {
    tmp.type=STRING;
    *(tmp.value.string=MALLOC(1))='\0';
  }
  if (tmp.type!=STRING) {
    clear_var(&tmp);
    return 1;
  }
  result=atol(tmp.value.string);
  clear_var(&tmp);
  tmp.type=INTEGER;
  tmp.value.integer=result;
  push(&tmp,rts);
  return 0;
}
