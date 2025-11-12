/* sys8.c */

#include "config.h"
#include "object.h"
#include "protos.h"
#include "instr.h"
#include "constrct.h"
#include "file.h"
#include "intrface.h"
#include "bcrypt.h"

int s_fstat(struct object *caller, struct object *obj,
            struct object *player, struct var_stack **rts) {
  struct var tmp;
  int retval;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=1) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=STRING) {
    clear_var(&tmp);
    return 1;
  }
  retval=stat_file(tmp.value.string,obj);
  clear_var(&tmp);
  tmp.type=INTEGER;
  tmp.value.integer=retval;
  push(&tmp,rts);
  return 0;
}

int s_fowner(struct object *caller, struct object *obj,
             struct object *player, struct var_stack **rts) {
  struct var tmp;
  int retval;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=1) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=STRING) {
    clear_var(&tmp);
    return 1;
  }
  retval=owner_file(tmp.value.string,obj);
  clear_var(&tmp);
  tmp.type=INTEGER;
  tmp.value.integer=retval;
  push(&tmp,rts);
  return 0;
}

int s_get_hostname(struct object *caller, struct object *obj,
                   struct object *player, struct var_stack **rts) {
  struct var tmp;
  char *retval;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=1) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=STRING) {
    clear_var(&tmp);
    return 1;
  }
  retval=addr_to_host(tmp.value.string,0);
  clear_var(&tmp);
  if (retval) {
    tmp.type=STRING;
    tmp.value.string=retval;
  } else {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  }
  push(&tmp,rts);
  return 0;
}

int s_get_address(struct object *caller, struct object *obj,
                  struct object *player, struct var_stack **rts) {
  struct var tmp;
  char *retval;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=1) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=STRING) {
    clear_var(&tmp);
    return 1;
  }
  retval=host_to_addr(tmp.value.string,0);
  clear_var(&tmp);
  if (retval) {
    tmp.type=STRING;
    tmp.value.string=retval;
  } else {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  }
  push(&tmp,rts);
  return 0;
}

int s_set_localverbs(struct object *caller, struct object *obj,
                     struct object *player, struct var_stack **rts) {
  struct var tmp;
  int bool;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=1) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  bool=1;
  if (tmp.type==INTEGER) if (tmp.value.integer==0) bool=0;
  clear_var(&tmp);
  if (bool) obj->flags|=LOCALVERBS;
  else obj->flags&=~LOCALVERBS;
  tmp.type=INTEGER;
  tmp.value.integer=0;
  push(&tmp,rts);
  return 0;

}

int s_localverbs(struct object *caller, struct object *obj,
                 struct object *player, struct var_stack **rts) {
  struct var tmp;

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
  if (tmp.value.objptr->flags & LOCALVERBS) {
    tmp.type=INTEGER;
    tmp.value.integer=1;
  } else {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  }
  push(&tmp,rts);
  return 0;
}

char *search_verb(struct object *obj, char *vname) {
  struct verb *curr_verb;

  curr_verb=obj->verb_list;
  if (vname)
    while (curr_verb) {
      if (!strcmp(curr_verb->verb_name,vname)) {
        curr_verb=curr_verb->next;
        break;
      }
      curr_verb=curr_verb->next;
    }
  if (curr_verb) return curr_verb->verb_name;
  else return NULL;
}

int s_next_verb(struct object *caller, struct object *obj,
                struct object *player, struct var_stack **rts) {
  struct var tmp1,tmp2;
  char *retval;

  if (pop(&tmp1,rts,obj)) return 1;
  if (tmp1.type!=NUM_ARGS) {
    clear_var(&tmp1);
    return 1;
  }
  if (tmp1.value.num!=2) return 1;
  if (pop(&tmp2,rts,obj)) return 1;
  if (tmp2.type!=STRING && !(tmp2.type==INTEGER && tmp2.value.integer==0)) {
    clear_var(&tmp2);
    return 1;
  }
  if (pop(&tmp1,rts,obj)) {
    clear_var(&tmp2);
    return 1;
  }
  if (tmp1.type!=OBJECT) {
    clear_var(&tmp2);
    return 1;
  }
  if (tmp2.type==INTEGER)
    retval=search_verb(tmp1.value.objptr,NULL);
  else
    retval=search_verb(tmp1.value.objptr,tmp2.value.string);
  clear_var(&tmp2);
  if (retval) {
    tmp1.type=STRING;
    tmp1.value.string=retval;
  } else {
    tmp1.type=INTEGER;
    tmp1.value.integer=0;
  }
  push(&tmp1,rts);
  return 0;
}

int s_get_devport(struct object *caller, struct object *obj,
                  struct object *player, struct var_stack **rts) {
  struct var tmp;
  int port;

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
  port=get_devport(tmp.value.objptr);
  tmp.type=INTEGER;
  tmp.value.integer=port;
  push(&tmp,rts);
  return 0;
}

int s_get_devnet(struct object *caller, struct object *obj,
                 struct object *player, struct var_stack **rts) {
  struct var tmp;
  int net;

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
  net=get_devnet(tmp.value.objptr);
  tmp.type=INTEGER;
  tmp.value.integer=net;
  push(&tmp,rts);
  return 0;
  return 1;
}

int s_redirect_input(struct object *caller, struct object *obj,
                     struct object *player, struct var_stack **rts) {
  struct var tmp;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=1) return 1;
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=STRING && !(tmp.type==INTEGER && tmp.value.integer==0)) {
    clear_var(&tmp);
    return 1;
  }
  while (obj->attacher) obj=obj->attacher;
  if (obj->input_func) {
    FREE(obj->input_func);
    obj->input_func=NULL;
  }
  if (tmp.type==STRING) obj->input_func=tmp.value.string;
  tmp.type=INTEGER;
  tmp.value.integer=0;
  push(&tmp,rts);
  return 0;
}

int s_get_input_func(struct object *caller, struct object *obj,
                     struct object *player, struct var_stack **rts) {
  struct var tmp;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=0) return 1;
  while (obj->attacher) obj=obj->attacher;
  if (obj->input_func) {
    tmp.type=STRING;
    tmp.value.string=obj->input_func;
  } else {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  }
  push(&tmp,rts);
  return 0;
}

int s_get_master(struct object *caller, struct object *obj,
                 struct object *player, struct var_stack **rts) {
  struct var tmp;

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
  while (tmp.value.objptr->attacher)
    tmp.value.objptr=tmp.value.objptr->attacher;
  push(&tmp,rts);
  return 0;
}

int s_is_master(struct object *caller, struct object *obj,
                struct object *player, struct var_stack **rts) {
  struct var tmp;

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
  if (tmp.value.objptr->attacher) {
    tmp.type=INTEGER;
    tmp.value.integer=0;
  } else {
    tmp.type=INTEGER;
    tmp.value.integer=1;
  }
  push(&tmp,rts);
  return 0;
}

int s_input_to(struct object *caller, struct object *obj,
               struct object *player, struct var_stack **rts) {
  struct var tmp, tmp2;

  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp.value.num!=2) return 1;
  
  /* Pop function name */
  if (pop(&tmp,rts,obj)) return 1;
  if (tmp.type!=STRING && !(tmp.type==INTEGER && tmp.value.integer==0)) {
    clear_var(&tmp);
    return 1;
  }
  
  /* Pop target object */
  if (pop(&tmp2,rts,obj)) {
    clear_var(&tmp);
    return 1;
  }
  if (tmp2.type!=OBJECT) {
    clear_var(&tmp);
    clear_var(&tmp2);
    return 1;
  }
  
  /* Use player object (which has the device connection) instead of obj */
  if (!player) {
    clear_var(&tmp);
    clear_var(&tmp2);
    return 1;
  }
  
  /* Follow attacher chain to get the device object */
  while (player->attacher) player=player->attacher;
  
  /* Clear existing input handler */
  if (player->input_func) {
    FREE(player->input_func);
    player->input_func=NULL;
  }
  player->input_func_obj=NULL;
  
  /* Set new input handler */
  if (tmp.type==STRING) {
    player->input_func=tmp.value.string;
    player->input_func_obj=tmp2.value.objptr;
  }
  
  tmp.type=INTEGER;
  tmp.value.integer=0;
  push(&tmp,rts);
  return 0;
}

/* crypt() - Secure password hashing
 * 
 * Syntax: string crypt(string password)
 *         int crypt(string password, string hash)
 * 
 * With 1 argument: Generates a bcrypt hash of the password
 * With 2 arguments: Verifies password against hash (returns 1 if match, 0 if not)
 */
int s_crypt(struct object *caller, struct object *obj,
            struct object *player, struct var_stack **rts) {
  struct var tmp, tmp2;
  char *hash;
  int verify_mode = 0;
  
  /* Pop NUM_ARGS */
  if (pop(&tmp, rts, obj)) return 1;
  if (tmp.type != NUM_ARGS) {
    clear_var(&tmp);
    return 1;
  }
  
  /* Check if we're in verify mode (2 args) or hash mode (1 arg) */
  if (tmp.value.num == 2) {
    verify_mode = 1;
    
    /* Pop hash argument */
    if (pop(&tmp2, rts, obj)) return 1;
    if (tmp2.type != STRING) {
      clear_var(&tmp2);
      return 1;
    }
  } else if (tmp.value.num != 1) {
    return 1;
  }
  
  /* Pop password argument */
  if (pop(&tmp, rts, obj)) {
    if (verify_mode) clear_var(&tmp2);
    return 1;
  }
  
  if (tmp.type != STRING) {
    clear_var(&tmp);
    if (verify_mode) clear_var(&tmp2);
    return 1;
  }
  
  if (verify_mode) {
    /* Verify mode: crypt(password, hash) -> returns 1 or 0 */
    int result = bcrypt_verify(tmp.value.string, tmp2.value.string);
    clear_var(&tmp);
    clear_var(&tmp2);
    
    tmp.type = INTEGER;
    tmp.value.integer = result;
    push(&tmp, rts);
  } else {
    /* Hash mode: crypt(password) -> returns hash string */
    hash = bcrypt_hash(tmp.value.string);
    clear_var(&tmp);
    
    if (hash) {
      tmp.type = STRING;
      tmp.value.string = hash;
      push(&tmp, rts);
    } else {
      /* Error - return empty string */
      tmp.type = STRING;
      tmp.value.string = (char *)MALLOC(1);
      tmp.value.string[0] = '\0';
      push(&tmp, rts);
    }
  }
  
  return 0;
}
