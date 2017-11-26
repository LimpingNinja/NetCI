#include <types.h>

object sharobj;

id(arg) {
  return (instr(downcase(get_name()),1,downcase(arg))>0);
}

recycle() {
  if (!priv(caller_object()) && get_owner()!=caller_object()) return 0;
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
  return;
}
