/* common.c */

#include <sys.h>
#include <types.h>
#include <flags.h>

/* Common functions for visible objects */

object owner,lock,link,attacher;
string name,desc,fail,ofail,succ,osucc,drop,odrop;
int type,flags;

/* #define CANNOT_MODIFY (!priv(caller_object()) && owner!=caller_object()) */
/* New CANNOT_MODIFY from hurin on 9.27.95 */
#define CANNOT_MODIFY (!priv(caller_object()) && owner!=caller_object() && this_object()!=caller_object())

print_stats() {
  object curr;
  string s;

  if (CANNOT_MODIFY) return 0;
  write(caller_object(),"Name: "+make_num(this_object())+"\n");
  write(caller_object(),"Type: "+make_type(type)+"\n");
  s=otoa(this_object());
  s=leftstr(s,instr(s,1,"#")-1);
  write(caller_object(),"Source: "+s+".c\n");
  s=make_sys_flags(this_object());
  if (s) write(caller_object(),"System Flags:"+s+"\n");
  if (flags) write(caller_object(),"Flags:"+make_flags(flags)+"\n");
  if (owner) write(caller_object(),"Owner: "+make_num(owner)+"\n");
  else write(caller_object(),"Owner: *NONE*\n");
  if (lock) write(caller_object(),"Lock: "+lock.get_bool_exp()+"\n");
  else
    write(caller_object(),"Lock: *UNLOCKED*\n");
  if (link && link!="home") write(caller_object(),"Linked To: "+make_num(link)+"\n");
  else if (link=="home") write(caller_object(),"Linked To: **HOME**\n");
  else write(caller_object(),"Linked To: *UNLINKED*\n");
  if (location(this_object()))
    write(caller_object(),"Location: "+make_num(location(this_object()))+
          "\n");
  else {
    if (curr=this_object().get_last_location())
      write(caller_object(),"Location: *VOID* ("+make_num(curr)+")\n");
    else
      write(caller_object(),"Location: *VOID*\n");
  }
  if (desc) write(caller_object(),"Desc: "+desc+"\n");
  if (succ) write(caller_object(),"Succ: "+succ+"\n");
  if (osucc) write(caller_object(),"OSucc: "+osucc+"\n");
  if (fail) write(caller_object(),"Fail: "+fail+"\n");
  if (ofail) write(caller_object(),"OFail: "+ofail+"\n");
  if (drop) write(caller_object(),"Drop: "+drop+"\n");
  if (odrop) write(caller_object(),"ODrop: "+odrop+"\n");
  this_object().additional_stats(caller_object());
  if (contents(this_object())) {
    write(caller_object(),"Contents:\n");
    curr=contents(this_object());
    while (curr) {
      write(caller_object(),make_num(curr)+"\n");
      curr=next_object(curr);
    }
  }
  return 1;
}

set_owner(arg) {
  if ((CANNOT_MODIFY) && owner) return 0;
  if (arg && typeof(arg)!=OBJECT_T) return 0;
  if (arg) if (arg.get_type()!=TYPE_PLAYER) return 0;
  owner=arg;
  return 1;
}

get_owner(arg) {
  return owner;
}

set_link(arg) {
  if (CANNOT_MODIFY)
    if (type!=TYPE_EXIT || link)
      return 0;
  if (!arg && type==TYPE_PLAYER) return 0;
  if (arg && arg=="home") link="home";
  else {
    if (arg && typeof(arg)!=OBJECT_T) return 0;
    if (arg)
      if (!arg.can_link(caller_object()))
	return 0;
    link=arg;
  }
  if (!(owner==caller_object()) && type==TYPE_EXIT) owner=caller_object();
  return 1;
}

set_name(arg) {
  if (CANNOT_MODIFY) return 0;
  if (typeof(arg)!=STRING_T) return 0;
  if (type==TYPE_PLAYER && name)
    if (sys_names_disabled()) {
      return 0;
    } else {
      if (instr(arg,0," ")) arg=leftstr(arg,(instr(arg,0," ")-1));
      if (!find_player(arg)) rename_player(arg,this_object());
      else return 0;
    }
  name=arg;
  return 1;
}

set_lock(arg) {
  if (CANNOT_MODIFY) return 0;
  if (arg && typeof(arg)!=OBJECT_T) return 0;
  if (lock)
    lock.recycle_bool();
  lock=arg;
  if (lock) lock.set_parent_object(this_object());
  return 1;
}

set_flags(arg) {
  if (owner!=caller_object() && !priv(caller_object())) return 0;
  if (typeof(arg)!=INT_T) return 0;
  flags=arg;
  return 1;
}

#define SET_STRING_FUNC(FUNC,VAR)                   \
  FUNC (arg) {                                      \
    if (CANNOT_MODIFY) return 0;                    \
    if (arg && typeof(arg)!=STRING_T) return 0;     \
    VAR=arg;                                        \
    return 1;                                       \
  }

SET_STRING_FUNC(set_desc,desc)
SET_STRING_FUNC(set_succ,succ)
SET_STRING_FUNC(set_osucc,osucc)
SET_STRING_FUNC(set_fail,fail)
SET_STRING_FUNC(set_ofail,ofail)
SET_STRING_FUNC(set_drop,drop)
SET_STRING_FUNC(set_odrop,odrop)

#define GET_FUNC(FUNC,VAR)                          \
  FUNC (arg) {                                      \
    return VAR;                                     \
  }

GET_FUNC(get_name,name)
GET_FUNC(get_desc,desc)
GET_FUNC(get_succ,succ)
GET_FUNC(get_osucc,osucc)
GET_FUNC(get_fail,fail)
GET_FUNC(get_ofail,ofail)
GET_FUNC(get_owner,owner)
GET_FUNC(get_type,type)
GET_FUNC(get_drop,drop)
GET_FUNC(get_odrop,odrop)
GET_FUNC(get_flags,flags)

get_link() {
/*  if (!link) link=atoo("/obj/room"); */
  return link;
}

recycle() {
  if (attacher!=caller_object()) return 0;
  if (prototype(this_object())) return 0;
  if (caller_object()==this_object()) return 0;
  if (lock)
    lock.recycle_bool();
  destruct(this_object());
  return 1;
}

test_lock(tester) {
  if (lock)
    return lock.eval(tester);
  else
    return 1;
}

can_get(getter) {
  if (type!=TYPE_OBJECT) return 0;
  if (lock)
    return lock.eval(getter);
  else
    return 1;
}

can_link(linker) {
  if (typeof(linker)!=OBJECT_T) return 0;
  if (type==TYPE_PLAYER) return 0;
  if (type==TYPE_OBJECT) return 0;
  if (type==TYPE_EXIT) return 0;
  if (type==TYPE_ROOM)
    if ((priv(linker) || linker==owner) || (flags & FLAG_LINK_OK)) return 1;
  return 0;
}

can_teleport(teleporter) {
  if (typeof(teleporter)!=OBJECT_T) return 0;
  if (type!=TYPE_OBJECT && type!=TYPE_PLAYER) return 0;
  if (location(this_object()))
    if (location(this_object()).get_owner()==teleporter) return 1;
  if (priv(teleporter) || teleporter==owner) return 1;
  return 0;
}

can_teleport_to(teleporter,teleported) {
  if (typeof(teleporter)!=OBJECT_T) return 0;
  if (typeof(teleported)!=OBJECT_T) return 0;
  if (type!=TYPE_ROOM &&
      !(teleported.get_type()==TYPE_OBJECT && type==TYPE_PLAYER))
    return 0;
  if ((priv(teleporter) || teleporter==owner) || (flags & FLAG_JUMP_OK))
    return 1;
  return 0;
}

cannot_drop() {
  return 0;
}

get_lock() { return lock; }

set_type(arg) {
  if (caller_object()==this_object()) {
    type=arg;
    return 1;
  } else return 0;
}

allow_attach() {
  if (prototype(this_object())) return 0;
  if (attacher) return 0;
  attacher=caller_object();
  return 1;
}
