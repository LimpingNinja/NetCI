/* living.c */

#include <sys.h>
#include <types.h>
#include <flags.h>
#include <hear.h>
#include <combat.h>
#include <gender.h>

/* I put this macro cause I found myself typing it a few times */
#define PERM_CHECK (!priv(caller_object()) && caller_object()!=get_owner())

int alive;
int show_exit_list;
object possessor;
int strength;
int intelligence;
int willpower;
int stamina;
int accuracy;
int agility;
int speed;
int piety;
int hp,max_hp;
int xp;
int gold;
int time_connected;
int time_disconnected;
int total_conn;
object profession;
object religion;
object weapon;
object armor[10];
object sharobj;
object attacher;
int gender;
object attacking;
object killer;
string email;
string url;
string rlname;

get_email() { return email; }
get_url() { return url; }
get_rlname() { return rlname; }

time_conn() {
  time_connected=time();
}

sum_total_conn(arg) {
  total_conn=total_conn+arg;
}

reset_total_conn() {
  total_conn=0;
}

get_total_conn() {
  return total_conn;
}

get_connect_time() {
  return time_connected;
}

disconnect_time() {
  time_disconnected=time();
}

get_disconnect_time() {
  return time_disconnected;
}

additional_stats(rcpt) {
  if (get_gender()==GENDER_NONE) write(rcpt,"Gender: NONE\n");
  else if (get_gender()==GENDER_MALE) write(rcpt,"Gender: MALE\n");
  else if (get_gender()==GENDER_FEMALE) write(rcpt,"Gender: FEMALE\n");
  else if (get_gender()==GENDER_NEUTER) write(rcpt,"Gender: NEUTER\n");
  else if (get_gender()==GENDER_PLURAL) write(rcpt,"Gender: PLURAL\n");
  else if (get_gender()==GENDER_SPIVAK) write(rcpt,"Gender: SPIVAK\n");
  else write(rcpt,"Gender: UNKNOWN\n");
  if (!sys_combat_enabled()) return;
  write(rcpt,"Strength: "+itoa(strength)+"\n");
  write(rcpt,"Intelligence: "+itoa(intelligence)+"\n");
  write(rcpt,"Willpower: "+itoa(willpower)+"\n");
  write(rcpt,"Stamina: "+itoa(stamina)+"\n");
  write(rcpt,"Accuracy: "+itoa(accuracy)+"\n");
  write(rcpt,"Speed: "+itoa(speed)+"\n");
  write(rcpt,"Agility: "+itoa(agility)+"\n");
  write(rcpt,"Piety: "+itoa(piety)+"\n");
  write(rcpt,"HP: "+itoa(hp)+"/"+itoa(max_hp)+"\n");
  write(rcpt,"XP: "+itoa(xp)+"\n");
  write(rcpt,"Gold: "+itoa(gold)+"\n");
  if (profession) profession.additional_stats(rcpt);
  else write(rcpt,"Profession: none\n");
  if (religion) religion.additional_stats(rcpt);
  else write(rcpt,"Religion: none\n");
  if (killer) write(rcpt,"Killed By: "+make_num(killer)+"\n");
  if (attacking) write(rcpt,"Attacking: "+make_num(attacking)+"\n");
}

dispossess() {
  if (!priv(caller_object()) && caller_object()!=possessor) return 0;
  possessor=0;
  return 1;
}

get_possessor() {
  if (caller_object()==this_object())
    return possessor;
  else
    return 0;
}

possess() {
  if (possessor) return 0;
  if (!priv(caller_object()) && caller_object()!=get_owner()) return 0;
  if (priv(this_object())) return 0;
  possessor=caller_object();
  return 1;
}

find_object(name) {
  object result;

  if (name=="me") return this_player();
  if (leftstr(name,1)=="/" || leftstr(name,1)=="#")
    if (result=atoo(name)) {
      if (result) if (!is_master(result)) return NULL;
      return result;
    }
  if (leftstr(name,1)=="*")
    if (result=find_player(rightstr(name,strlen(name)-1)))
      return result;
  if (result=present(name,this_player()))
    return result;
  if (result=present(name,location(this_player())))
    return result;
  if (name=="here") return location(this_player());
  if (name=="root") return atoo("/boot");
  return NULL;
}

static clear_room() {
  object tmp,otmp;
  int anyone;
 
  if (location(this_object()).get_link()) {
    anyone=0;
    tmp=contents(location(this_object()));
    while (tmp) {
      if (tmp.get_type()==TYPE_PLAYER && tmp!=this_object())
        anyone=1;
      tmp=next_object(tmp);
    }
    if (!anyone) {
      tmp=contents(location(this_object()));
      otmp=next_object(tmp);
      while (tmp) {
        if (tmp.get_type()==TYPE_OBJECT)
          if (location(this_object()).get_link()=="home")
            move_object(tmp,tmp.get_link());
          else
            move_object(tmp,location(this_object()).get_link());
        tmp=otmp;
        if (otmp)
          otmp=next_object(otmp);
      }
    }
  }
}



static do_huh(arg) {
  object curr,dest;
  string s;

  if (location(this_object())) {
    curr=contents(location(this_object()));
    while (curr) {
      if (curr.get_type()==TYPE_EXIT)
        if (curr.id(arg)) {
          if (curr.can_traverse(this_object()) &&
              (dest=curr.get_link())) {
	    if (dest=="home") dest=this_object().get_link();
	    clear_room();
            s=curr.get_succ();
            if (s) write(this_object(),s+"\n");
            move_object(this_object(),dest);
            tell_room_except(location(curr),this_object(),NULL,HEAR_LEAVE,
                             curr.get_osucc());
            do_look();
            s=curr.get_drop();
            if (s) write(this_object(),s+"\n");
            tell_room_except(dest,this_object(),NULL,HEAR_ENTER,
                             curr.get_odrop());
          } else {
            s=curr.get_fail();
            if (s) write(this_object(),s+"\n");
            else write(this_object(),"You can't go that way.\n");
            tell_room_except(location(curr),this_object(),curr,HEAR_FAIL,
                             curr.get_ofail());
          }
          return 1;
        }
      curr=next_object(curr);
    }
  }
  if (arg) write(this_object(),HUH_STRING);
  return 1;
}

static do_say(arg) {
  tell_room_except(location(this_object()),this_object(),NULL,HEAR_SAY,arg);
  write(this_object(),"You say \""+arg+"\"\n");
  return 1;
}

static do_date(arg) {
  write(this_object(),mktime(time()));
  return 1;
}

static do_pose(arg) {
  if (leftstr(arg,1)=="'") {
    do_nospc_pose(arg);
    return 1;
  }
  tell_room_except(location(this_object()),this_object(),NULL,HEAR_POSE,arg);
  write(this_object(),get_name()+" "+arg+"\n");
  return 1;
}

static do_nospc_pose(arg) {
  tell_room_except(location(this_object()),this_object(),NULL,HEAR_NOSPACE,arg);
  write(this_object(),get_name()+arg+"\n");
  return 1;
}

static do_exits(arg) {
  object curr;
  string s;
  int exit_shown,pos;

  if (arg) {
    arg=downcase(arg);
    if (arg=="on") {
      show_exit_list=1;
      write(this_object(),"Will display exit lists.\n");
    } else if (arg=="off") {
      show_exit_list=0;
      write(this_object(),"Will not display exit lists.\n");
    } else {
      write(this_object(),"Specify 'on' or 'off'.\n");
    }
    return 1;
  }
  if (!location(this_object())) {
    write(this_object(),"There are no obvious exits.\n");
    return 1;
  }
  curr=contents(location(this_object()));
  while (curr) {
    if (curr.get_type()==TYPE_EXIT)
      if (!(curr.get_flags() & FLAG_DARK)) {
        if (!exit_shown) {
          write(this_object(),"Obvious exits: ");
          exit_shown=1;
        } else
          write(this_object(),", ");
        s=curr.get_name();
        pos=instr(s,1,";");
        if (pos) s=leftstr(s,pos-1);
        write(this_object(),s);
      }
    curr=next_object(curr);
  }
  if (!exit_shown)
    write(this_object(),"There are no obvious exits.\n");
  else
    write(this_object(),"\n");
  return 1;
}

force_object(cmd) {
  if (priv(caller_object()) || caller_object()==get_owner()) {
    command(cmd);
    return 1;
  }
  return 0;
}

static do_get(arg) {
  object obj;
  string s;

  obj=present(arg,location(this_object()));
  if (!obj) {
    write(this_object(),"You don't see the \""+arg+"\" here.\n");
    return 1;
  }
  if (obj.can_get(this_object())) {
    s=obj.get_succ();
    if (s)
      write(this_object(),s+"\n");
    else
      write(this_object(),"Gotten.\n");
    tell_room_except(location(this_object()),this_object(),obj,HEAR_GET,
                     obj.get_osucc());
    move_object(obj,this_object());
    return 1;
  } else {
    s=obj.get_fail();
    if (s)
      write(this_object(),s+"\n");
    else
      write(this_object(),"You can't get that.\n");
    tell_room_except(location(this_object()),this_object(),obj,HEAR_FAIL,
                     obj.get_ofail());
    return 1;
  }
}

static do_drop(arg) {
  object obj;
  string s;

  obj=present(arg,this_object());
  if (!obj) {
    write(this_object(),"You don't have that.\n");
    return 1;
  }
  if (obj.cannot_drop()) {
    write(this_object(),"You can't drop that.\n");
    return 1;
  }
  s=obj.get_drop();
  if (s)
    write(this_object(),s+"\n");
  else
    write(this_object(),"Dropped.\n");
  tell_room_except(location(this_object()),this_object(),obj,HEAR_DROP,
                   obj.get_odrop());
  if (obj.get_flags() & FLAG_STICKY)
    if (obj.get_link())
      move_object(obj,obj.get_link());
    else
      move_object(obj,location(this_object()));
  else if (location(this_object()).get_link())
    if (location(this_object()).get_flags() & FLAG_STICKY)
      move_object(obj,location(this_object()));
    else
      if (location(this_object()).get_link()=="home")
        move_object(obj,obj.get_link());
      else
        move_object(obj,location(this_object()).get_link());
  else
    if (location(this_object()))
      move_object(obj,location(this_object()));
    else
      move_object(obj,obj.get_link());
  return 1;
}


do_look(arg) {
  object obj,curr;
  int contents_printed;
  string desc;

  if (arg)
    obj=find_object(arg);
  else {
    obj=location(this_object());
    if (!obj) {
      write(this_object(),"You are in the void. Contact a wizard.\n");
      return 1;
    }
  }
  if (!obj) {
    write(this_object(),"You don't see that here.\n");
    return 1;
  }
  if (location(obj)!=location(this_object()) && location(obj)!=this_object() &&
      obj!=location(this_object())) {
    write(this_object(),"You don't see that here.\n");
    return 1;
  }
  if (obj.get_type()==TYPE_ROOM)
    write(this_object(),make_num(obj)+"\n");
  desc=obj.get_desc();
  if (desc)
    write(this_object(),desc+"\n");
  else 
    if (obj.get_type()!=TYPE_ROOM)
      write(this_object(),NOTHING_SPECIAL_STRING);
  if (obj.get_type()==TYPE_ROOM) {
    if (obj.test_lock(this_object())) {
      if (desc=obj.get_succ()) write(this_object(),desc+"\n");
      tell_room_except(obj,this_object(),NULL,HEAR_LOOK,obj.get_osucc());
    } else {
      if (desc=obj.get_fail()) write(this_object(),desc+"\n");
      tell_room_except(obj,this_object(),obj,HEAR_FAIL,obj.get_ofail());
    }
    if (show_exit_list) do_exits(NULL);
  }
  curr=contents(obj);
  if ((obj.get_flags() & FLAG_DARK) && obj.get_owner()
      !=this_object() && !priv(this_object())) curr=0;
  while (curr) {
    if (curr!=this_object())
      if (curr.get_type()!=TYPE_EXIT && !((curr.get_flags() & FLAG_DARK) &&
                                          curr.get_owner()!=this_object() &&
                                          !priv(this_object()))) {
        if (!contents_printed) {
          write(this_object(),"Contents:\n");
          contents_printed=1;
        }
        write(this_object(),make_num(curr)+"\n");
      }
    curr=next_object(curr);
  }
  return 1;
}

static do_inv(arg) {
  object curr;
  int carrying_printed;

  curr=contents(this_object());
  while (curr) {
    if (priv(this_object()) || !(curr.get_flags() & FLAG_DARK)) {
      if (priv(this_object()) || curr.get_owner()==this_object()) {
        if (!carrying_printed) {
          write(this_object(),"You are carrying:\n");
          carrying_printed=1;
        }
        write(this_object(),make_num(curr)+"\n");
      } else {
        if (!carrying_printed) {
          write(this_object(),"You are carrying:\n");
          carrying_printed=1;
        }
        write(this_object(),curr.get_name()+"\n");
      }
    }
    curr=next_object(curr);
  }
  if (!carrying_printed)
    write(this_object(),"You are empty-handed.\n");
  return 1;
}

force(arg) {
  if (caller_object()!=get_owner() && !priv(caller_object())) return 0;
  if (priv(this_object())) return 0;
  command(arg);
  return 1;
}

allow_attach() {
  if (prototype(this_object())) return 0;
  if (attacher) return 0;
  attacher=caller_object();
  return 1;
}

recycle() {
  if (caller_object()!=attacher) return 0;
  if (prototype(this_object())) return 0;
  if (caller_object()==this_object()) return 0;
  sharobj.recycle();
  if (profession) profession.recycle();
  if (religion) religion.recycle();
  destruct(this_object());
  return 1;
}

set_email(arg) {
  if (PERM_CHECK) return 0;
  email=arg;
  return 1;
}

set_url(arg) {
  if (PERM_CHECK) return 0;
  url=arg;
  return 1;
}

set_rlname(arg) {
  if (PERM_CHECK) return 0;
  rlname=arg;
  return 1;
}

set_gender(arg) {
  if (PERM_CHECK) return 0;
  gender=arg;
  return 1;
}

get_gender() { return gender; }


static init() {
  sharobj=new("/obj/share/common");
  attach(sharobj);
  init_combat_attribs();
}

static init_combat_attribs() {
  alive=1;
  strength=BASE_STRENGTH;
  intelligence=BASE_INTELLIGENCE;
  willpower=BASE_WILLPOWER;
  stamina=BASE_STAMINA;
  accuracy=BASE_ACCURACY;
  agility=BASE_AGILITY;
  speed=BASE_SPEED;
  piety=BASE_PIETY;
  hp=BASE_MAX_HP;
  max_hp=BASE_MAX_HP;
  xp=0;
  gold=0;
}

get_defense() {
  int loop;
  int defense;

  while (loop<10) {
    if (armor[loop]) defense+=armor[loop].get_defense();
    loop++;
  }
  return defense+agility;
}

int healing;

heal() {
  if (!alive) {
    healing=0;
    return;
  }
  hp+=BASE_HEAL_AMOUNT;
  if (hp>=max_hp) {
    hp=max_hp;
    healing=0;
    return;
  }
  alarm(BASE_HEAL_DELAY,"heal");
  healing=1;
}

start_heal() {
  if (healing) return;
  alarm(BASE_HEAL_DELAY,"heal");
  healing=1;
}

static perform_attack() {
  int delay,xp_bonus,hit_chance,dam_bonus;

  if (!attacking) return;
  if (!location(this_object())) {
    attacking=0;
    return;
  }
  if (location(this_object())!=location(attacking)) {
    attacking=0;
    return;
  }
  if (!attacking.is_living()) {
    attacking=0;
    return;
  }
  xp_bonus=attacking.get_xp_bonus();
  attacking.attacked_by(this_object());
  hit_chance=BASE_HIT_PERCENTAGE+accuracy-attacking.get_defense();
  if (profession) hit_chance+=profession.get_hit_bonus();
  if (religion) hit_chance+=religion.get_hit_bonus();
  if (weapon) hit_chance+=weapon.get_hit_bonus();
  if (hit_chance>random(101)) {
    tell_room(location(this_object()),this_object(),attacking,HEAR_HIT,NULL);
    dam_bonus=(weapon?(weapon.get_dam_bonus(attacking)):BASE_DAM_BONUS);
    dam_bonus-=attacking.dam_reduce(this_object(),weapon,dam_bonus);
    if (dam_bonus<0) dam_bonus=0;
    if (attacking.damage(dam_bonus)) {
      tell_room(location(this_object()),this_object(),attacking,HEAR_DIE,
                NULL);
      write(this_object(),"You gain "+itoa(xp_bonus)+" experience points.\n");
      xp+=xp_bonus;
    } else {
      delay=BASE_ATTACK_DELAY-(speed/SPEED_MODIFIER);
      if (delay<1) delay=1;
      alarm(delay,"perform_attack");
    }
  } else {
    tell_room(location(this_object()),this_object(),attacking,HEAR_MISS,NULL);
    delay=BASE_ATTACK_DELAY-((speed-BASE_SPEED)/SPEED_MODIFIER);
    if (delay<1) delay=1;
    alarm(delay,"perform_attack");
  }
}

damage(i) {
  hp-=i;
  start_heal();
  if (hp<=0) {
    hp=0;
    attacking=0;
    alive=0;
    xp=0;
    killer=caller_object().get_responsible();
    write(this_object(),"You have died.\n");
    return 1;
  }
  return 0;
}

reincarnate() {
  if (alive) return 0;
  hp=1;
  start_heal();
  alive=1;
  write(this_object(),"You rejoin the living.\n");
  return 1;
}

is_living() { return alive; }

attacked_by(o) {
  int delay;

  if (attacking) return;
  attacking=o;
  delay=BASE_ATTACK_DELAY-((BASE_SPEED-speed)/SPEED_MODIFIER);
  if (delay<1) delay=1;
  alarm(delay,"perform_attack");
}

get_xp_bonus() {
  int bonus;

  bonus=xp/XP_BONUS_MODIFIER;
  if (profession) bonus+=profession.get_xp_bonus();
  if (religion) bonus+=religion.get_xp_bonus();
  return bonus;
}

attack(o) {
  int delay;

  if (attacking) {
    attacking=o;
  } else {
    attacking=o;
    delay=BASE_ATTACK_DELAY-((BASE_SPEED-speed)/SPEED_MODIFIER);
    if (delay<1) delay=1;
    alarm(delay,"perform_attack");
  }
}

get_attacking() { return attacking; }

get_max_hp() { return max_hp; }
set_max_hp(i) { max_hp=i+0; }
get_hp() { return hp; }
set_hp(i) { hp=i+0; }
add_hp(i) { if (i<0) damage(i); else { hp+=i; if (hp>max_hp) hp=max_hp; } }
get_xp() { return xp; }
add_xp(i) { xp+=i; }
get_gold() { return gold; }
set_gold(i) { gold=i+0; }
add_gold(i) { gold+=i; }
get_strength() { return strength; }
set_strength(i) { strength=i+0; }
get_intelligence() { return intelligence; }
set_intelligence(i) { intelligence=i+0; }
get_willpower() { return willpower; }
set_willpower(i) { willpower=i+0; }
get_stamina() { return stamina; }
set_stamina(i) { stamina=i+0; }
get_accuracy() { return accuracy; }
set_accuracy(i) { accuracy=i+0; }
get_speed() { return speed; }
set_speed(i) { speed=i+0; }
get_agility() { return agility; }
set_agility(i) { agility=i+0; }
get_piety() { return piety; }
set_piety(i) { piety=i+0; }
get_religion() { return religion; }
get_profession() { return profession; }

set_religion(o) {
  if (religion) return 0;
  if (!attach(o)) {
    religion=o;
    return 1;
  }
  return 0;
}

set_profession(o) {
  if (profession) return 0;
  if (!attach(o)) {
    profession=o;
    return 1;
  }
  return 0;
}
