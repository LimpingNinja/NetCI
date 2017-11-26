#include <types.h>
#include <sys.h>
#include <hear.h>
#include <gender.h>
#include <flags.h>

object sharobj;

id(arg) {
  return (instr(downcase(get_name()),1,downcase(arg))>0);
}

recycle() {
  if (priv(caller_object()) && get_owner()!=caller_object()) return 0;
  if (prototype(this_object())) return 0;
  sharobj.recycle();
  destruct(this_object());
  return 1;
}

static init() {
  sharobj=new("/obj/share/common");
  attach(sharobj);
  set_type(TYPE_OBJECT);
  set_owner(caller_object());
  set_name("A hammock");
  add_verb("enter","enter_hammock");
  add_verb("leave","leave_hammock");
  add_verb("peek","do_peek");
  add_verb("yell","do_yell");
  return;
}

get_desc(arg) {
  if (location(this_player()) == location(this_object())) {
    return "You see a nice comfortable hammock.  It looks like you might be able to enter it.";
  } else {
    return "You are nestled into a nice warm hammock.  From here, you can hear people talking in the room you are in, but you aren't sure if they can hear you too.";
  }
}

do_yell(arg) {
  if (!arg) {
    this_player().listen("Yell what?\n");
    return 1;
  }
  this_player().listen("You shout, \""+arg+"\"\n");
  tell_room_except(this_object(),this_player(),NULL,HEAR_PASTE,this_player().get_name()+ " shouts, \""+arg+"\"\n");
  tell_room_except(location(this_object()),this_object(),NULL,HEAR_PASTE,"[From the hammock]: "+this_player().get_name()+" shouts, \""+arg+"\"\n");
  return 1;
}

static enter_hammock(arg) {
  if (!arg) {
    this_player().listen("Enter what?\n");
    return 1;
  } else if (arg == "hammock") { 
    this_player().listen("You climb into the hammock\n");
    tell_room_except(location(this_object()),this_player(),NULL,HEAR_LEAVE,"climbs into the hammock.");
    move_object(this_player(),this_object());
    tell_room_except(location(this_player()),this_player(),NULL,HEAR_ENTER,"joins you in the hammock.");
    this_player().do_look();
    return 1;
  } else {
    return 0;
  }
}

static do_peek(arg) {
  object curr,obj;
  int contents_printed;
  
  if (location(this_player()) == this_object()) {
    this_player().listen("You peek out of the hammock and see:\n");
    this_player().listen(">");
    this_player().listen(location(this_object()).get_desc()+"\n");
    obj = location(this_object());
    curr=contents(obj);
    if ((obj.get_flags() & FLAG_DARK) && obj.get_owner() != this_object()
	&& !priv(this_object())) curr=0;
    while (curr) {
      if (curr!=this_object())
	if (curr.get_type()!=TYPE_EXIT && !((curr.get_flags() & FLAG_DARK) &&
					    curr.get_owner()!=this_object() &&
					    !priv(this_object()))) {
	  if (!contents_printed) {
	    this_player().listen(">Contents:\n");
	    contents_printed=1;
	  }
	  this_player().listen(">"+curr.get_name()+"\n");
	}
      curr=next_object(curr);
    }
    return 1;
  }
}

static leave_hammock(arg) {
  this_player().listen("You climb out of the hammock\n");
  tell_room_except(location(this_player()),this_player(),NULL,HEAR_LEAVE,"climbs out of the hammock");
  move_object(this_player(),location(this_object()));
  tell_room_except(location(this_player()),this_player(),NULL,HEAR_ENTER,"climbs out of the hammock");
  this_player().do_look();
  return 1;
}


hear(actor,actee,type,message) {
  int ch;
  if (location(actor) == this_object()) {
      iterate(contents(this_object()),this_object(),this_player(),"hear",actor,actee,type,message); */
      if (random(10)== 4) {
	iterate(contents(location(this_object())),this_object(),this_player(),"hear",actor,actee,HEAR_PASTE,"You hear some rustling and mumbling from the hammock\n");
      }
  } else {
    iterate(contents(this_object()),this_object(),this_player(),"hear",actor,actee,type,message,"[From outside the hammock]: ");
  }
}





