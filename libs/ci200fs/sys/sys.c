/* sys.c */

#include <flags.h>
#include <types.h>
#include <strings.h>

string doingquery,whoquote;
int ncf,registration,find_toggle,guest_toggle,combat_toggle,names;
string gender_array[8][5];
string sub_array[8];

sys_combat_enabled() { return combat_toggle; }

sys_set_combat_enabled(i) {
  if (!priv(caller_object())) return;
  combat_toggle=i;
}

sys_guests_disabled() { return guest_toggle; }

sys_set_guests_disabled(i) {
  if (!priv(caller_object())) return;
  guest_toggle=i;
}

sys_set_names_disabled(i) {
  if (!priv(caller_object())) return;
  names=i;
}

sys_names_disabled() { return names; }

sys_find_disabled() { return find_toggle; }

sys_set_find_disabled(arg) {
  if (!priv(caller_object())) return;
  find_toggle=arg;
}

is_legal_name(s) {
  int count,len;
  string c;

  count=1;
  s=downcase(s);
  len=strlen(s);
  if (len>14) return 0;
  if (!len) return 0;
  while (count<=len) {
    c=midstr(s,count,1);
    if (c=="\n" || c=="\r" || c=="\t" || c=="\b" || c=="\f" || c=="\v" ||
        c==" " || c=="/" || c=="\\" || c=="#" || c=="*" || c=="?" || c=="\a" ||
        c=="~" || c=="%")
      return 0;
    count++;
  }
  if (s=="root" || s=="void") return 0;
  if (leftstr(s,5)=="guest") return 0;
  return 1;
}

add_new_player(name,o) {
  if (!priv(caller_object())) return;
  table_set(downcase(name),itoa(otoi(o)));
}

check_player(name) {
  return table_get(name);
}

rename_player(name,o) {
  if (caller_object()==o.get_owner() || priv(caller_object())) {
    table_delete(downcase(o.get_name()));
    table_set(downcase(name),itoa(otoi(o)));
  }
}

remove_player(name) {
  if (!priv(caller_object())) return;
  table_delete(downcase(name));
}

remove_guest() {
  if (leftstr(otoa(caller_object()),12)!="/obj/player#") return;
  if (!caller_object().is_guest()) return;
  table_delete(downcase(caller_object().get_name()));
  caller_object().recycle();
}

make_sys_flags(o) {
  string s;

  if (connected(o)) s+=" connected";
  if (in_editor(o)) s+=" in_editor";
  if (interactive(o)) s+=" interactive";
  if (localverbs(o)) s+=" localverbs";
  if (priv(o)) s+=" priv";
  if (prototype(o)) s+=" prototype";
  return s;
}

make_flags(f) {
  string s;

  if (f & FLAG_BUILDER) { s+=" builder"; f&=~FLAG_BUILDER; }
  if (f & FLAG_DARK) { s+=" dark"; f&=~FLAG_DARK; }
  if (f & FLAG_JUMP_OK) { s+=" jump_ok"; f&=~FLAG_JUMP_OK; }
  if (f & FLAG_LINK_OK) { s+=" link_ok"; f&=~FLAG_LINK_OK; }
  if (f & FLAG_PROGRAMMER) { s+=" programmer"; f&=~FLAG_PROGRAMMER; }
  if (f & FLAG_STICKY) { s+=" sticky"; f&=~FLAG_STICKY; }
  if (f & FLAG_HAVEN) { s+=" haven";f&=~FLAG_HAVEN; }
  if (f) s+=" "+itoa(f);
  return s;
}

make_num(o) {
  int type,owner,f;
  string name,num;
  object rcv;

  name=o.get_name();
  f=o.get_flags();
  if (o.get_owner()!=caller_object() && !priv(caller_object()) &&
      !(f & (FLAG_LINK_OK | FLAG_JUMP_OK)) && o.get_owner()!=this_player())
    if (this_player()) {
      if (o.get_owner()!=this_player() && !priv(this_player()))
        return name;
    } else
      return name;
  type=o.get_type();
  num="(#"+itoa(otoi(o));
  if (type==TYPE_PLAYER) num+="p";
  else if (type==TYPE_OBJECT) num+="o";
  else if (type==TYPE_EXIT) num+="e";
  else if (type==TYPE_ROOM) num+="r";
  else num="("+otoa(o);
  if (f) {
    if (f & FLAG_BUILDER) num+="B";
    if (f & FLAG_DARK) num+="D";
    if (f & FLAG_JUMP_OK) num+="J";
    if (f & FLAG_LINK_OK) num+="L";
    if (f & FLAG_PROGRAMMER) num+="P";
    if (f & FLAG_STICKY) num+="S";
    if (f & FLAG_HAVEN) num+="H";
  }
  return name+num+")";
}

make_type(type) {
  if (type==TYPE_EXIT) return "EXIT";
  if (type==TYPE_ROOM) return "ROOM";
  if (type==TYPE_OBJECT) return "OBJECT";
  if (type==TYPE_PLAYER) return "PLAYER";
  return "UNKNOWN";
}

gender_subs(actor,message) {
  int gender,loop,pos,done;
  string name,c;

  gender=actor.get_gender();
  if (gender<0 || gender>5) gender=0;
  name=actor.get_name();
  if (!gender) {
    loop=0;
    while (loop<8) gender_array[loop++][0]=name;
    gender_array[2][0]=(
    gender_array[3][0]=(
    gender_array[6][0]=(
    gender_array[7][0]+="'s")));
  }
  loop=0;
  pos=1;
  while (pos=instr(message,pos,"%")) {
    c=midstr(message,pos+1,1);
    done=0;
    loop=0;
    while (loop<8 && !done) {
      if (sub_array[loop]==c) {
        done=1;
        message=subst(message,pos,2,gender_array[loop][gender]);
        pos+=strlen(gender_array[loop][gender]);
      }
      loop++;
    }
    if (!done)
      if (c=="n" || c=="N") {
        message=subst(message,pos,2,name);
        pos+=strlen(name);
      } else if (c=="%") {
        message=subst(message,pos,2,"%");
        pos++;
      } else pos+=2;
  }
  return message;
}

present(name,loc) {
  object curr;
  int count,pos;

  if (!loc) return 0;
  pos=instr(name,1,".");
  if (pos) {
    count=leftstr(name,pos-1);
    count=atoi(count);
  }
  if (count)
    name=rightstr(name,strlen(name)-pos);
  else
    count=1;
  if (!name) return 0;
  curr=contents(loc);
  while (curr) {
    if (curr.id(name))
      if (!(--count))
        return curr;
    curr=next_object(curr);
  }
  return 0;
}

tell_room(room,actor,actee,type,message) {
  if (!room) return;
  room.hear(actor,actee,type,message);
  iterate(contents(room),0,0,"hear",actor,actee,type,message);
}

tell_room_except(room,actor,actee,type,message) {
  if (!room) return;
  if (room!=actor) room.hear(actor,actee,type,message);
  iterate(contents(room),actor,0,"hear",actor,actee,type,message);
}

tell_room_except2(room,actor,actee,type,message) {
  if (!room) return;
  if (actor!=room && actee!=room) room.hear(actor,actee,type,message);
  iterate(contents(room),actor,actee,"hear",actor,actee,type,message);
}

broadcast(msg) {
  object curr;

  while (curr=next_who(curr)) {
    curr.listen(msg);
  }
}

sys_get_newchar_flags() { return ncf; }
sys_get_registration() { return registration; }
sys_get_doingquery() { return doingquery; }
sys_get_whoquote() { return whoquote; }

sys_set_newchar_flags(arg) {
  if (!priv(caller_object())) return;
  ncf=arg & (FLAG_BUILDER | FLAG_PROGRAMMER);
}

sys_set_registration(arg) {
  if (!priv(caller_object())) return;
  registration=arg;
}

sys_set_doingquery(arg) {
  if (!priv(caller_object())) return;
  doingquery=arg;
}

sys_set_whoquote(arg) {
  if (!priv(caller_object())) return;
  whoquote=arg;
}

static make_space(arg,len) {
  int count;

  count=len-strlen(arg);
  if (count<0) count=0;
  while (count--) arg+=" ";
  return arg;
}

static make_dash(len) {
  int count;
  string s;

  while (count++<len) s+="-";
  return s;
}

make_time(on_for) {
  string retval;

  if (on_for>86400) retval=itoa(on_for/86400)+"d";
  else if (on_for>60) retval=itoa(on_for/3600)+":"+
                             (((on_for%3600)/60)>9 ? itoa((on_for%3600)/60) :
                                              "0"+itoa((on_for%3600)/60))+"m";
  else retval=itoa(on_for)+"s";
  while (strlen(retval)<6) retval=" "+retval;
  return retval;
}

who_list() {
  object curr;
  int count;
  string name;

  count=0;
  this_player().listen(pad("Name",17)+
                       pad("On For",7)+
                       pad("  Idle",7)+
                       doingquery+"\n");
  this_player().listen(pad("",16,"-")+" "+
                       pad("",6,"-")+" "+
                       pad("",6,"-")+" "+
                       pad("",strlen(doingquery),"-")+"\n");
  curr=next_who();
  while (curr) {
    name=curr.get_name();
    if (name) {
      count++;
      this_player().listen(pad(name,17)+
                           pad(make_time(time()-
                                                get_conntime(curr)),7)+
                           pad(make_time(get_devidle(curr)),7)+
                                      curr.get_doing_string()+"\n");
    }
    curr=next_who(curr);
  }
  if (count==1)
    this_player().listen("1 player connected. "+whoquote+"\n");
  else
    this_player().listen(itoa(count)+" players connected. "+whoquote+"\n");
}

wiz_who_list() {
  object curr;
  int count;

  count=0;
  curr=next_who();
  this_player().listen(pad("Address",16)+
                       pad("Name",32)+
                       "Location\n");
  this_player().listen(pad("-------",16)+
                       pad("----",32)+
                       "--------\n");
  while (curr) {
    count++;
    this_player().listen(pad(get_devconn(curr),16)+
                         pad(make_num(curr),32)+
                         (location(curr) ? 
                          make_num(location(curr)) : "void")+"\n");
    curr=next_who(curr);
  }
  if (count==1)
    this_player().listen("1 player connected.\n");
  else
    this_player().listen(itoa(count)+ " players connected.\n");
}

sync() {
  int loop1,loop2;

  if (!priv(caller_object())) return 0;
  gender_array[0][1]="he";
  gender_array[0][2]="she";
  gender_array[0][3]="it";
  gender_array[0][4]="they";
  gender_array[0][5]="e";
  gender_array[1][1]="him";
  gender_array[1][2]="her";
  gender_array[1][3]="it";
  gender_array[1][4]="them";
  gender_array[1][5]="em";
  gender_array[2][1]="his";
  gender_array[2][2]="hers";
  gender_array[2][3]="its";
  gender_array[2][4]="their";
  gender_array[2][5]="eir";
  gender_array[3][1]="his";
  gender_array[3][2]="hers";
  gender_array[3][3]="its";
  gender_array[3][4]="theirs";
  gender_array[3][5]="eirs";
  loop1=4;
  loop2=1;
  while (loop1<8) {
    while (loop2<5) {
      gender_array[loop1][loop2]=
        upcase(leftstr(gender_array[loop1-4][loop2],1))+
        rightstr(gender_array[loop1-4][loop2],
                 strlen(gender_array[loop1-4][loop2])-1);
      loop2++;
    }
    loop1++;
    loop2=1;
  }
  sub_array[0]="s"; sub_array[4]="S";
  sub_array[1]="o"; sub_array[5]="O";
  sub_array[2]="p"; sub_array[6]="P";
  sub_array[3]="a"; sub_array[7]="A";
  return 1;
}

static init() {
  doingquery="Doing";
  find_toggle=1;
  combat_toggle=1;
  if (!prototype(this_object()))
    destruct(this_object());
}

find_player(name) {
  object p,curr;

  if (name=="me") return (caller_object());
  p=table_get(downcase(name));
  if (p) return itoo(atoi(p));
  curr=next_who();
  while (curr) {
    if (curr.id(name))
      return curr;
    curr=next_who(curr);
  }
  return 0;
}
