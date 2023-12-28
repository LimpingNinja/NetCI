/* player.c */

#include <secure.h>
#include <sys.h>
#include <types.h>
#include <flags.h>
#include <hear.h>
#include <pkg.h>
#include <verb.h>
#include <gender.h>
#include <ban.h>
#include <strings.h>

#define PASTE_BUF_LEN 8000

string path;
int doing_ls;
int wrap;
string password;
string doing_string;
object possessing;
object sharobj;
string ls_dir;
int dir_count;
object last_location;
int is_a_guest;
int use_crlf;
object paste_rcv;
string paste_msg;
int paste_header_sent;
string conn_addr;

#define resolve(X) ((X)=resolve_path(X))

#define split_equal(X,Y,Z)                                     \
  do {                                                         \
    if (instr((X),1,"=")) {                                    \
      (Y)=leftstr((X),instr((X),1,"=")-1);                     \
      (Z)=rightstr((X),strlen(X)-instr((X),1,"="));            \
    } else {                                                   \
      (Y)=(X);                                                 \
      (Z)=0;                                                   \
    }                                                          \
    while (leftstr((Y),1)==" ") (Y)=rightstr((Y),strlen(Y)-1); \
    while (rightstr((Y),1)==" ") (Y)=leftstr((Y),strlen(Y)-1); \
    while (leftstr((Z),1)==" ") (Z)=rightstr((Z),strlen(Z)-1); \
    while (rightstr((Z),1)==" ") (Z)=leftstr((Z),strlen(Z)-1); \
  } while (0)

recycle() {
  if (!priv(caller_object())) return 0;
  if (prototype(this_object())) return 0;
  if (caller_object()==this_object()) return 0;
  sharobj.recycle();
  destruct(this_object());
  return 1;
}

static do_at_listban(arg) {
  if (!priv(this_player())) {
    this_player().listen("Permission Denied\n");
    return 1;
  }
  list_sites();
  return 1;
}

static do_at_updateban(arg) {
  if (!priv(this_player())) {
    this_player().listen("Permission Denied\n");
    return 1;
  }
  if (reload_sites()) {
    this_player().listen("Permission Denied\n");
    return 1;
  }
  this_player().listen("Site List Reloaded.\n");
  return 1;
}


static do_at_wrap(arg) {
  if (!arg) {
    if (wrap==0) {
      listen("Word wrap is turned off.\n");
    } else {
      listen("Word wrapping at "+itoa(wrap)+" spaces.\n");
    }
    return 1;
  }
  if (wrap == "off") {
    wrap=0;
    listen("Word wrap is turned off.\n");
    return 1;
  }
  wrap=atoi(arg);
  if (wrap==0) {
    listen("Word wrap is turned off.\n");
    return 1;
  }
  listen("Word wrapping at "+itoa(wrap)+" spaces.\n");
  return 1;
}
  

is_guest() { return is_a_guest; }

set_guest() {
  if (leftstr(otoa(caller_object()),11)!="/sys/login#") return;
  is_a_guest=1;
}

static do_to(arg) {
  int pos;
  object o;
  string msg;

  if (!get_flags() & FLAG_BUILDER) {
    listen("Permission denied.\n");
    return 1;
  }
  pos=instr(arg,1," ");
  o=find_object(leftstr(arg,pos-1));
  msg=rightstr(arg,strlen(arg)-pos);
  if (!o) {
    listen("You don't see that here.\n");
    return 1;
  }
  if (!location(this_object())) {
    listen("You don't see that here.\n");
    return 1;
  }
  if (location(o)!=location(this_object())) {
    listen("You don't see that here.\n");
    return 1;
  }
  if (o.get_type()!=TYPE_PLAYER) {
    listen("You can't stage to that.\n");
    return 1;
  }
  tell_room(location(this_object()),this_object(),o,HEAR_STAGE,msg);
  return 1;
}

static do_news(arg) {
  string topic;

  if (!arg) {
    if (cat("/etc/news/news"))
      listen("No News.\n");
    return 1;
  }
  if (cat("/etc/news/"+arg))
    listen("No news under the topic" + topic +".\n");
  return 1;
}

static do_finger(arg) {
  object obj;
  int pad;

  if (!arg) {
    listen("Usage: finger <name>\n");
    return 1;
  }

  obj=find_player(arg); 
  if (!obj) { 
    listen("That player does not exist.\n"); 
    return 1; 
  } 
  listen("("+pad("",76,"=")+")\n");
  listen("( Info on: "+pad(obj.get_name(),66," "));
  listen(")\n");
  listen("("+pad("",76,"-")+")\n");
  listen(" Real Name:      ");
  if (obj.get_rlname()) {
    listen(obj.get_rlname()+"\n");
  } else {
    listen("<not listed>\n");
  }
  listen(" Email Address:  ");
  if (obj.get_email()) {
    listen(obj.get_email()+"\n");
  } else {
    listen("<not set>\n");
  }
  listen(" URL:            ");
  if (obj.get_url()) {
    listen(obj.get_url()+"\n\n");
  } else {
    listen("<not set>\n\n");
  }
  if (connected(obj)) {
    listen(" Connected on:   "+mktime(get_conntime(obj)));
    listen("                 ("+ make_time(get_devidle(obj))+" idle time)\n\n");
    listen("("+center("Total Time Connected",76,"-")+")\n");
    listen("("+center(make_time(obj.get_total_conn()+(time()-get_conntime(obj))),76)+")\n");
  } else {

    listen(" Disconnected:   "+mktime(obj.get_disconnect_time()));
    listen("                 (on for "+make_time(obj.get_disconnect_time()-obj.get_connect_time())+")\n\n");
    listen("("+center("Total Time Connected",76,"-")+")\n");
    listen("("+center(make_time(obj.get_total_conn()),76)+")\n");
  }
  listen("("+pad("",76,"=")+")\n");
  return 1;
}

make_time(on_for) {
  string retval;
  int days,hours,minutes,seconds;

 retval="";
  if (on_for>86400) {
    days=on_for/86400;
    on_for=on_for-(days*86400);
  }
  if (on_for>3600) {
    hours=on_for/3600;
    on_for=on_for-(hours*3600);
  }
  if (on_for>60) {
    minutes=on_for/60;
    on_for=on_for-(minutes*60);
  }
  seconds=on_for;
  if (days==1) {
    retval="1 day ";
  } else if (days>1) {
    retval=itoa(days)+" days ";
  }
  if (hours==1) {
    retval=retval+"1 hour ";
  } else if (hours>1) {
    retval=retval+itoa(hours)+" hours ";
  }
  if (minutes==1) {
    retval=retval+"1 minute ";
  } else if (minutes>1) {
    retval=retval+itoa(minutes)+" minutes ";
  }
  if (seconds==1) {
    retval=retval+"1 second";
  } else if (seconds>1 || seconds == 0) {
    retval=retval+itoa(seconds)+" seconds";
  }
/*  while (strlen(retval)<6) retval=" "+retval; */
  return retval;
}

static hyphened(arg) {
  int left,right;

  right=(76-strlen(arg))/2;
  left=76-strlen(arg)-right;
  while ((left--)>0) arg="-"+arg;
  while ((right--)>0) arg+="-";
  return arg+"\n";
}

static paste_handler(arg) {
  object rcpt;

  if (paste_rcv) rcpt=paste_rcv;
  else rcpt=location(this_object());
  if (arg==".") {
    if (rcpt) {
      if (!paste_header_sent)
        if (paste_rcv)
          tell_player(rcpt,this_object(),NULL,HEAR_PASTE,center("Private pasting from "+get_name(),76,"-")+"\n");
/* HERE */
        else
          tell_room(rcpt,this_object(),NULL,HEAR_PASTE,center("Pasting from "+get_name(),76,"-")+"\n");
      if (paste_rcv)
        tell_player(rcpt,this_object(),NULL,HEAR_PASTE,paste_msg);
      else
        tell_room(rcpt,this_object(),NULL,HEAR_PASTE,paste_msg);
      if (paste_rcv)
        tell_player(rcpt,this_object(),NULL,HEAR_PASTE,center("End Pasting",76,"-")+"\n");
      else
        tell_room(rcpt,this_object(),NULL,HEAR_PASTE,center("End Pasting",76,"-")+"\n");
      listen("Pasted.\n");
      paste_msg=NULL;
      paste_rcv=NULL;
    } else {
      listen("Unable to paste output.\n");
    }
  } else {
    paste_msg=leftstr(paste_msg+arg,8000)+"\n";
    redirect_input("paste_handler");
  }
}

static do_at_paste(arg) {
  if (!(get_flags() & FLAG_BUILDER)) {
    listen("Permission denied.\n");
    return 1;
  }
  if (arg) {
    listen("@paste does not take arguments.\n");
    return 1;
  }
  if (!location(this_object())) {
    listen("You cannot @paste in void.\n");
    return 1;
  }
  paste_rcv=NULL;
  paste_msg=NULL;
  paste_header_sent=FALSE;
  redirect_input("paste_handler");
  listen("Accepting paste input. Enter '.' on a line by itself to end.\n");
  return 1;
}

static do_at_pasteto(arg) {
  object o;

  if (!(get_flags() & FLAG_BUILDER)) {
    listen("Permission denied.\n");
    return 1;
  }
  o=find_player(arg);
  if (!o) {
    listen("No player by that name exists.\n");
    return 1;
  }
  paste_rcv=o;
  paste_msg=NULL;
  paste_header_sent=FALSE;
  redirect_input("paste_handler");
  listen("Accepting paste input. Enter '.' by itself on a line to end.\n");
  return 1;
}

static do_rlname(arg) {
  if (!arg) {
    if (get_rlname()) {
      listen("Your real name is set to "+get_rlname()+"\n");
    } else {
      listen("Your real name is unset. To set it, type: rlname <your rlname>\n");
    }
    return 1;
  }
  set_rlname(arg);
  listen("Your real name is set to "+get_rlname()+"\n");
  return 1;
}

static do_url(arg) {
  if (!arg) {
    if (get_url()) {
      listen("Your URL is set to "+get_url()+"\n");
    } else {
      listen("Your URL is unset. To set it, type: url <your url>\n");
    }
    return 1;
  }
  set_url(arg);
  listen("Your url is set to "+get_url()+"\n");
  return 1;
}

static do_email(arg) {
  if (!arg) {
    if (get_email()) {
      listen("Your email address is "+get_email()+"\n");
    } else {
      listen("Your email address is unset. To set it, type: email <email address>\n");
    }
    return 1;
  }
  set_email(arg);
  listen("Your email address is set to "+get_email()+"\n");
  return 1;
}

static do_gender(arg) {
  if (get_gender() && arg) {
    listen("Your gender is already set.\n");
    return 1;
  }
  if (arg=="male") set_gender(GENDER_MALE);
  else if (arg=="female") set_gender(GENDER_FEMALE);
  else if (arg) {
    listen("You can't set your gender to that.\n");
    return 1;
  }
  if (get_gender()==GENDER_NONE) listen("Your gender is not set.\n");
  else if (get_gender()==GENDER_MALE) listen("Your gender is set to male.\n");
  else if (get_gender()==GENDER_FEMALE) listen("Your gender is set to "+
                                               "female.\n");
  else if (get_gender()==GENDER_NEUTER) listen("Your gender is set to "+
                                               "neuter.\n");
  else if (get_gender()==GENDER_PLURAL) listen("Your gender is set to "+
                                               "plural.\n");
  else listen("Your gender is unknown.\n");
  return 1;
}

get_last_location() { return last_location; }

static find_any_object(name) {
  object result;

  if (name=="me") return this_player();
  if (name=="here") return location(this_player());
  if (name=="root") return atoo("/boot");
  if (leftstr(name,1)=="/" || leftstr(name,1)=="#")
    if (result=atoo(name)) return result;
  if (leftstr(name,1)=="*")
    if (result=find_player(rightstr(name,strlen(name)-1)))
      return result;
  if (result=present(name,this_player()))
    return result;
  if (result=present(name,location(this_player())))
    return result;
  return NULL;
}

static basename(path) {
  int pos;

  while (pos=instr(path,1,"/")) path=rightstr(path,strlen(path)-pos);
  if (path) return path;
  else return "/";
}

static print_dot_dirs() {
  string filename;

  filename=ls_dir;
  ls_dir=0;
  listen(itoa(fowner(filename))+" "+itoa(fstat(filename))+" .\n");
  filename+="/..";
  resolve(filename);
  listen(itoa(fowner(filename))+" "+itoa(fstat(filename))+" ..\n");
}

static remove_tilde(path) {
  int pos;
  string username;

  if (path=="~") return "/home/"+get_uid();
  pos=instr(path,1,"/");
  if (pos==2) {
    username=get_uid();
    path=rightstr(path,strlen(path)-pos);
  } else if (pos) {
    username=midstr(path,2,pos-2);
    path=rightstr(path,strlen(path)-pos);
  } else {
    username=rightstr(path,strlen(path)-1);
    path="";
  }
  if (path) return "/home/"+username+"/"+path;
  else return "/home/"+username;
}

static remove_dots(path) {
  int pos,lastpos,currpos;

  while (pos=instr(path,1,"/./")) path=leftstr(path,pos)+
                                       rightstr(path,strlen(path)-pos-2);
  if (leftstr(path,1)!="/") path="/"+path;
  if (rightstr(path,2)=="/.") path=leftstr(path,strlen(path)-2);
  if (!path) path="/";
  while (pos=instr(path,1,"/../")) {
    lastpos=0;
    currpos=instr(path,1,"/");
    while (currpos<pos) {
      lastpos=currpos;
      currpos=instr(path,lastpos+1,"/");
    }
    if (lastpos) path=leftstr(path,lastpos)+
                      rightstr(path,strlen(path)-pos-3);
    else path=rightstr(path,strlen(path)-pos-3);
    if (leftstr(path,1)!="/") path="/"+path;
  }
  if (leftstr(path,1)!="/") path="/"+path;
  if (rightstr(path,3)=="/..") {
    lastpos=0;
    currpos=instr(path,1,"/");
    while (currpos<(strlen(path)-2)) {
      lastpos=currpos;
      currpos=instr(path,lastpos+1,"/");
    }
    if (lastpos) path=leftstr(path,lastpos-1);
    else path="/";
  }
  if (!path) path="/";
  return path;
}

static resolve_path(upath) {
  if (!upath) return path;
  if (leftstr(upath,1)=="/") return remove_dots(upath);
  if (leftstr(upath,1)=="~") return remove_dots(remove_tilde(upath));
  if (path=="/") return remove_dots(path+upath);
  return remove_dots(path+"/"+upath);
}

static do_users(arg) {
  if (!(get_flags() & FLAG_PROGRAMMER)) {
    listen("Permission denied.\n");
    return 1;
  }
  wiz_who_list();
  return 1;
}

static do_crlf(arg) {
  if (arg=="on") use_crlf=TRUE;
  else if (arg=="off") use_crlf=FALSE;
  else if (arg) {
    listen("usage: crlf [on | off]\n");
    return 1;
  }
  if (use_crlf) listen("Newline to Carriage-Return/Newline translation is "+
                       "ON.\n");
  else listen("Newline to Carriage-Return/Newline translation is OFF.\n");
  return 1;
}

static do_pwd(arg) {
  if (!(get_flags() & FLAG_PROGRAMMER)) {
    listen("Permission denied.\n");
    return 1;
  }
  listen(path+"\n");
  return 1;
}

static do_cd(arg) {
  int tmp;

  if (!(get_flags() & FLAG_PROGRAMMER)) {
    listen("Permission denied.\n");
    return 1;
  }
  if (!arg) {
    listen(path+"\n");
    return 1;
  }
  resolve(arg);
  if ((tmp=fstat(arg))==(-1)) {
    listen("cd: directory "+arg+" does not exist\n");
    return 1;
  }
  if (!(tmp & DIRECTORY)) {
    listen("cd: "+arg+" is not a directory\n");
    return 1;
  }
  path=arg;
  listen(path+"\n");
  return 1;
}

static do_at_edit(arg) {
  if (!(get_flags() & FLAG_PROGRAMMER)) {
    listen("Permission denied.\n");
    return 1;
  }
  if (arg) resolve(arg);
  edit(arg);
  return 1;
}

static do_cat(arg) {
  if (!(get_flags() & FLAG_PROGRAMMER)) {
    listen("Permission denied.\n");
    return 1;
  }
  resolve(arg);
  if (cat(arg))
    listen("cat: couldn't open "+arg+"\n");
  return 1;
}

static do_ls(arg) {
  int perms;

  if (!(get_flags() & FLAG_PROGRAMMER)) {
    listen("Permission denied.\n");
    return 1;
  }
  resolve(arg);
  perms=fstat(arg);
  if (perms==(-1)) {
    listen("ls: couldn't find "+arg+"\n");
    return 1;
  }
  if (!(perms & DIRECTORY)) {
    doing_ls=1;
    listen(itoa(fowner(arg))+" "+itoa(perms)+" "+basename(arg)+"\n");
    doing_ls=0;
    return 1;
  }
  doing_ls=1;
  ls_dir=arg;
  dir_count=0;
  if (ls(arg)) {
    doing_ls=0;
    listen("ls: couldn't read "+arg+"\n");
    return 1;
  }
  if (!dir_count) print_dot_dirs();
  ls_dir=0;
  doing_ls=0;
  return 1;
}

static do_rm(arg) {
  if (!(get_flags() & FLAG_PROGRAMMER)) {
    listen("Permission denied.\n");
    return 1;
  }
  if (!arg) {
    listen("usage: rm filename\n");
    return 1;
  }
  resolve(arg);
  if (rm(arg))
    listen("rm: couldn't remove "+arg+"\n");
  else
    listen("Ok.\n");
  return 1;
}

static do_cp(arg) {
  string src,dest;
  int pos;

  if (!(get_flags() & FLAG_PROGRAMMER)) {
    listen("Permission denied.\n");
    return 1;
  }
  pos=instr(arg,1," ");
  src=leftstr(arg,pos-1);
  dest=rightstr(arg,strlen(arg)-pos);
  if ((!src) || (!dest)) {
    listen("usage: cp source destination\n");
    return 1;
  }
  resolve(src);
  resolve(dest);
  if (cp(src,dest))
    listen("cp: couldn't copy "+src+" to "+dest+"\n");
  else
    listen("Ok.\n");
  return 1;
}

static do_mv(arg) {
  string src,dest;
  int pos;

  if (!(get_flags() & FLAG_PROGRAMMER)) {
    listen("Permission denied.\n");
    return 1;
  }
  pos=instr(arg,1," ");
  src=leftstr(arg,pos-1);
  dest=rightstr(arg,strlen(arg)-pos);
  if ((!src) || (!dest)) {
    listen("usage: mv source destination\n");
    return 1;
  }
  resolve(src);
  resolve(dest);
  if (mv(src,dest))
    listen("mv: couldn't move "+src+" to "+dest+"\n");
  else
    listen("Ok.\n");
  return 1;
}

static do_mkdir(arg) {
  if (!(get_flags() & FLAG_PROGRAMMER)) {
    listen("Permission denied.\n");
    return 1;
  }
  if (!arg) {
    listen("usage: mkdir directory_name\n");
    return 1;
  }
  resolve(arg);
  if (mkdir(arg))
    listen("mkdir: couldn't make directory "+arg+"\n");
  else
    listen("Ok.\n");
  return 1;
}

static do_rmdir(arg) {
  if (!(get_flags() & FLAG_PROGRAMMER)) {
    listen("Permission denied.\n");
    return 1;
  }
  if (!arg) {
    listen("usage: rmdir directory_name\n");
    return 1;
  }
  resolve(arg);
  if (rmdir(arg))
    listen("rmdir: couldn't remove directory "+arg+"\n");
  else
    listen("Ok.\n");
  return 1;
}

static do_at_localverbs(arg) {
  if (!priv(this_object())) {
    listen("Permission denied.\n");
    return 1;
  }
  if (arg=="on") {
    set_localverbs(TRUE);
    listen("Now using only local verbs.\n");
  } else if (arg=="off") {
    set_localverbs(FALSE);
    listen("Now using all verbs.\n");
  } else if (!arg) {
    if (localverbs(this_object())) listen("Only using local verbs.\n");
    else listen("Using all verbs.\n");
  } else listen("usage: @localverbs [on | off]\n");
  return 1;
}

static do_at_hide(arg) {
  if (!priv(this_object())) {
    listen("Permission denied.\n");
    return 1;
  }
  if (!arg) {
    listen("usage: @hide filename\n");
    return 1;
  }
  resolve(arg);
  if (hide(arg))
    listen("@hide: couldn't hide "+arg+"\n");
  else
    listen("Ok.\n");
  return 1;
}

static do_at_unhidedir(arg) {
  if (!priv(this_object())) {
    listen("Permission denied.\n");
    return 1;
  }
  if (!arg) {
    listen("usage: @unhidedir directoryname\n");
    return 1;
  }
  resolve(arg);
  if (unhide(arg,this_object(),DIRECTORY))
    listen("@unhidedir: couldn't unhide "+arg+"\n");
  else
    listen("Ok.\n");
  return 1;
}

static do_at_unhide(arg) {
  if (!priv(this_object())) {
    listen("Permission denied.\n");
    return 1;
  }
  if (!arg) {
    listen("usage: @unhide filename\n");
    return 1;
  }
  resolve(arg);
  if (unhide(arg,this_object(),0))
    listen("@unhide: couldn't unhide "+arg+"\n");
  else
    listen("Ok.\n");
  return 1;
}

static do_chown(arg) {
  string file,owner;
  int pos;
  object o;

  if (!(get_flags() & FLAG_PROGRAMMER)) return 0;
  pos=instr(arg,1," ");
  file=rightstr(arg,strlen(arg)-pos);
  owner=leftstr(arg,pos-1);
  if ((!file) || (!owner)) {
    listen("usage: chown owner filename\n");
    return 1;
  }
  resolve(file);
  if (!(o=atoo(owner)))
    if (!(o=find_player(owner)))
      if (!(o=(owner=="root" ? atoo("/boot") : 0))) {
        listen("chown: couldn't find owner \""+owner+"\"\n");
        return 1;
      }
  if (chown(file,o))
    listen("chown: couldn't chown "+file+" to "+owner+"\n");
  else
    listen("Ok.\n");
  return 1;
}

static do_chmod(arg) {
  int pos;
  string modestr,file;
  int mode;

  if (!(get_flags() & FLAG_PROGRAMMER)) return 0;
  pos=instr(arg,1," ");
  modestr=leftstr(arg,pos-1);
  file=rightstr(arg,strlen(arg)-pos);
  if ((!modestr) || (!file)) {
    listen("usage: chown mode file\n");
    return 1;
  }
  mode=atoi(modestr);
  if (!mode && modestr!="0")
    if (modestr=="r")
      mode=READ_OK;
    else if (modestr=="w")
      mode=WRITE_OK;
    else if (modestr=="rw" || modestr=="wr")
      mode=WRITE_OK | READ_OK;
    else {
      listen("chmod: illegal mode \""+modestr+"\"\n");
      return 1;
    }
  resolve(file);
  if (chmod(file,mode))
    listen("chmod: couldn't change "+file+" to mode "+modestr+"\n");
  else
    listen("Ok.\n");
  return 1;
}

static do_at_call(arg) {
  int has_arg,pos;
  string o,func,tmp;
  var pass,x;
  object obj;

  if (!(get_flags() & FLAG_PROGRAMMER)) {
    listen("Permission denied.\n");
    return 1;
  }
  pos=instr(arg,1," ");
  o=leftstr(arg,pos-1);
  func=rightstr(arg,strlen(arg)-pos);
  pos=instr(func,1,"=");
  if (pos) {
    tmp=func;
    has_arg=1;
    func=leftstr(tmp,pos-1);
    x=rightstr(tmp,strlen(tmp)-pos);
  }
  if (!o || !func) {
    listen("usage: @call object func\n");
    listen("       @call object func=value\n");
    return 1;
  }
  if (!(obj=find_any_object(o))) {
    listen("@call: couldn't find object \""+o+"\"\n");
    return 1;
  }
  if (has_arg) {
    if (strlen(x)>1 && leftstr(x,1)=="\"" && rightstr(x,1)=="\"")
      pass=midstr(x,2,strlen(x)-2);
    else
      if (!(pass=find_object(x)))
        if (!(pass=atoi(x)))
          if (x!="0") {
            listen("@call: bad parameter \""+x+"\"\n");
            return 1;
          }
  }
  if (has_arg)
    x=call_other(obj,func,pass);
  else
    x=call_other(obj,func);
  listen("@call: "+otoa(obj)+" "+func);
  if (has_arg) {
    listen("(");
    if (typeof(pass)==STRING_T)
      listen("\""+pass+"\"");
    else if (typeof(pass)==OBJECT_T)
      listen(otoa(pass));
    else listen(itoa(pass));
    listen(")");
  } else
    listen("()");
  listen(" returned ");
  if (typeof(x)==STRING_T)
    listen("\""+x+"\"\n");
  else if (typeof(x)==OBJECT_T)
    listen(otoa(x)+"\n");
  else
    listen(itoa(x)+"\n");
  return 1;
}

static do_at_clone(arg) {
  object o;

  if (!(get_flags() & FLAG_PROGRAMMER)) {
    listen("Permission denied.\n");
    return 1;
  }
  resolve(arg);
  o=new(arg);
  if (o)
    listen("@clone: cloned "+otoa(o)+"\n");
  else
    listen("@clone: failed to clone \""+arg+"\"\n");
  return 1;
}

static do_at_gender(arg) {
  object o;
  string g;

  if (!(get_flags() & FLAG_BUILDER)) {
    listen("Permission denied.\n");
    return 1;
  }
  split_equal(arg,o,g);
  o=find_object(o);
  if (!o) {
    listen("Couldn't find that object.\n");
    return 1;
  }
  if (g=="male") g=GENDER_MALE;
  else if (g=="female") g=GENDER_FEMALE;
  else if (g=="neuter") g=GENDER_NEUTER;
  else if (g=="plural") g=GENDER_PLURAL;
  else if (g=="none") g=GENDER_NONE;
  else if (g=="spivak") g=GENDER_SPIVAK;
  else {
    listen("Illegal gender.\n");
    return 1;
  }
  if (o.set_gender(g)) listen("Gender set.\n");
  else listen("Permission denied.\n");
  return 1;
}

static do_at_move(arg) {
  object src,dest;
  string ssrc,sdest;

  if (!(get_flags() & FLAG_PROGRAMMER)) {
    listen("Permission denied.\n");
    return 1;
  }
  split_equal(arg,ssrc,sdest);
  src=find_object(ssrc);
  dest=find_object(sdest);
  if (!dest && sdest=="void") dest="void";
  if ((!src) || (!dest)) {
    listen("usage: @move item=destination\n");
    return 1;
  }
  if (dest=="void") dest=NULL;
  if (move_object(src,dest)) {
    listen("@move: failed\n");
    return 1;
  }
  listen("Ok.\n");
  return 1;
}

static do_at_fry(arg) {
  object o;

  if (!priv(this_object())) {
    listen("Permission denied.\n");
    return 1;
  }
  if (!arg) {
    listen("usage: @fry object\n");
    return 1;
  }
  o=find_any_object(arg);
  if (!o) {
    listen("@fry: couldn't find "+arg+"\n");
    return 1;
  }
  destruct(o);
  listen("Ok.\n");
  return 1;
}

static do_at_recon(arg) {
  object o;

  if (!priv(this_object())) {
    listen("Permission denied.\n");
    return 1;
  }
  o=find_object(arg);
  if (!o) {
    listen("@recon: couldn't find object \""+arg+"\"\n");
    return 1;
  }
  if (reconnect_device(o))
    listen("@recon: couldn't reconnect to object "+otoa(o)+"\n");
  disconnect();
  o.connect();
  return 1;
}

static do_at_compile(arg) {
  object result;

  if (!(get_flags() & FLAG_PROGRAMMER)) {
    listen("Permission denied.\n");
    return 1;
  }
  resolve(arg);
  if (arg=="/obj/player") {
    atoo("/sys/pcompile").pcompile();
    return 1;
  }
  result=compile_object(arg);
  if (result)
    listen("@compile: compiled "+otoa(result)+"\n");
  else
    listen("@compile: failed\n");
  return 1;
}

get_doing_string() {
  return doing_string;
}

get_uid() {
  return downcase(get_name());
}

check_password(arg) {
  return arg==password;
}

static do_quit(arg) {
  listen("Bye!\n");
  disconnect_device();
  disconnect();
  return 1;
}

force_disconnect() {
  if (!priv(caller_object())) return 0;
  disconnect_device();
  disconnect();
  return 1;
}

get_conn_addr() {
  return conn_addr;
}

connect() {
  object dest;

  if (!location(this_object())) {
    if (last_location) dest=last_location;
    else if (!(dest=get_link())) dest=atoo("/obj/room");
    move_object(this_object(),dest);
    tell_room_except(dest,this_object(),NULL,HEAR_ENTER,
                     NULL);
  }
  if (conn_addr) {
    this_object().listen("Last connected from ");
    if (get_hostname(conn_addr)) {
      this_object().listen(get_hostname(conn_addr));
    } else {
      this_object().listen(conn_addr);
    }
    this_object().listen(" at "+mktime(this_object().get_connect_time())+" (on for "+make_time(this_object().get_disconnect_time()-this_object().get_connect_time())+")\n");
  }
  conn_addr=get_devconn(this_object());
  last_location=0;
  doing_string=0;
  this_object().time_conn();
  alarm(0,"welcome");
}

disconnect() {
  this_object().sum_total_conn(time()-this_object().get_connect_time());
  redirect_input(NULL);
  paste_msg=0;
  paste_rcv=0;
  this_object().disconnect_time();
  tell_room_except(location(this_object()),this_object(),NULL,HEAR_DISCONNECT,
                   NULL);
  if (location(this_object())==atoo("/obj/room")) {
    last_location=location(this_object());
    tell_room_except(last_location,this_object(),NULL,HEAR_LEAVE,NULL);
    move_object(this_object(),NULL);
  }
  if (is_a_guest) remove_guest();
}

static welcome() {
  do_look();
  tell_room_except(location(this_object()),this_object(),NULL,HEAR_CONNECT,
                   NULL);
}

set_password(arg) {
  if (typeof(arg)!=STRING_T) return 0;
  if (caller_object()!=this_object() && !priv(caller_object())) return 0;
  password=arg;
  return 1;
}

#define FUNC_AT_SET_VAR(VAR_X,VAR_Y)                    \
  static VAR_X (arg) {                                  \
    string obj_name,new_string;                         \
    object obj;                                         \
                                                        \
    if (!(get_flags() & FLAG_BUILDER)) {                \
      listen("Permission denied.\n");                   \
      return 1;                                         \
    }                                                   \
    split_equal(arg,obj_name,new_string);               \
    obj=find_object(obj_name);                          \
    if (!obj) {                                         \
      listen("You don't see that here.\n");             \
      return 1;                                         \
    }                                                   \
    if (obj.VAR_Y(new_string)) {                        \
      listen("Set.\n");                                 \
      return 1;                                         \
    } else {                                            \
      listen("Permission denied.\n");                   \
      return 1;                                         \
    }                                                   \
  }

FUNC_AT_SET_VAR(do_at_name,set_name)
FUNC_AT_SET_VAR(do_at_desc,set_desc)
FUNC_AT_SET_VAR(do_at_succ,set_succ)
FUNC_AT_SET_VAR(do_at_osucc,set_osucc)
FUNC_AT_SET_VAR(do_at_fail,set_fail)
FUNC_AT_SET_VAR(do_at_ofail,set_ofail)
FUNC_AT_SET_VAR(do_at_drop,set_drop)
FUNC_AT_SET_VAR(do_at_odrop,set_odrop)

static do_describe(arg) {
  set_desc(arg);
  listen("Ok.\n");
  return 1;
}

static do_who(arg) {
  who_list();
  return 1;
}

static command_possessed(arg) {
  if (!(get_flags() & FLAG_PROGRAMMER)) return 0;
  if (!possessing) {
    listen("You aren't possessing anyone.\n");
    return 1;
  }
  possessing.force_object(arg);
  return 1;
}

possession_report(arg) {
  send_device(arg);
}

static do_at_possess(arg) {
  if (!(get_flags() & FLAG_PROGRAMMER)) {
    listen("Permission denied.\n");
    return 1;
  }
  if (!arg) {
    if (possessing) {
      listen("You are possessing "+make_num(possessing)+"\n");
      return 1;
    } else {
      listen("You are not possessing anyone.\n");
      return 1;
    }
  }
  if (arg=="off") {
    if (possessing) {
      possessing.dispossess();
      listen("You cease possessing "+make_num(possessing)+"\n");
      possessing=0;
      return 1;
    } else {
      listen("You aren't possessing anyone.\n");
      return 1;
    }
  }
  if (possessing) {
    possessing.dispossess();
    listen("You cease possessing "+make_num(possessing)+"\n");
    possessing=0;
  }
  possessing=find_object(arg);
  if (!possessing) {
    listen("You can't find that object.\n");
    return 1;
  }
  if (!possessing.possess()) {
    listen("You can't possess that.\n");
    possessing=0;
    return 1;
  }
  listen("You possess "+make_num(possessing)+"\n");
  return 1;
}

static do_whisper(arg) {
  string obj_name,message;
  object obj;

  if (!arg) return 0;
  split_equal(arg,obj_name,message);
  if (!obj_name) {
    listen("You must specify who to whisper to.\n");
    return 1;
  }
  if (!message) {
    listen("You must specify a message to whisper.\n");
    return 1;
  }
  obj=find_object(obj_name);
  if (!obj) {
    listen("You don't see that here.\n");
    return 1;
  }
  if (obj.get_type()!=TYPE_PLAYER) {
    listen("That isn't a player.\n");
    return 1;
  }
  if (location(obj)!=location(this_object())) {
    listen("You don't see that here.\n");
    return 1;
  }
  tell_player(obj,this_object(),obj,HEAR_WHISPER,message);
  listen("You whisper \""+message+"\" to "+obj.get_name()+"\n");
  return 1;
}

static do_at_find(arg) {
  object o;
  object curr;

  if (!(get_flags() & FLAG_BUILDER)) {
    listen("Permission denied.\n");
    return 1;
  }
  if (sys_find_disabled()) {
    listen("@find has been disabled.\n");
    return 1;
  }
  if (!arg) o=this_object();
  else o=find_player(arg);
  if (!o) {
    listen("That player does not exist.\n");
    return 1;
  }
  if (o!=this_object() && !(get_flags() & FLAG_PROGRAMMER)) {
    listen("Permission denied.\n");
    return 1;
  }
  curr=atoo("/obj/room");
  while (curr) {
    if (curr.get_owner()==o) listen(make_num(curr)+"\n");
    curr=next_child(curr);
  }
  curr=atoo("/obj/object");
  while (curr) {
    if (curr.get_owner()==o) listen(make_num(curr)+"\n");
    curr=next_child(curr);
  }
  listen("Done.\n");
  return 1;
}

static do_page(arg) {
  string obj_name,message;
  object obj;

  split_equal(arg,obj_name,message);
  if (!obj_name) {
    listen("You must specify who to page.\n");
    return 1;
  }
  obj=find_player(obj_name);
  if (!obj) {
    listen("That player does not exist.\n");
    return 1;
  }
  if (!(obj.get_flags() & FLAG_BUILDER) &&
      !(get_flags() & FLAG_BUILDER)) {
    listen("Permission denied.\n");
    return 1;
  }
  if (!connected(obj)) {
    listen("That player is not connected.\n");
    return 1;
  }
 if (obj.get_flags() & FLAG_HAVEN) {
   listen("That player does not wish to be disturbed.\n");
   return 1;
 }
  if (leftstr(message,1) == ":") {
    if (midstr(message,2,1) == "'") {
      message = this_object().get_name()+rightstr(message,strlen(message)-1);
    } else {
      message = this_object().get_name()+" "+rightstr(message,strlen(message)-1);
    }
  }
  tell_player(obj,this_object(),obj,HEAR_PAGE,message);
  if (message) {
    listen("You paged "+obj.get_name()+" with: "+message+"\n");
  } else {
    listen("You notifed "+obj.get_name()+" of your location.\n");
  }
  return 1;
}

static do_help(arg) {
  string section_s,topic;
  int section,pos,done;

  pos=instr(arg,1," ");
  if (!pos) {
    section=atoi(arg);
    if (!section) topic=arg;
  } else {
    section=atoi(leftstr(arg,pos-1));
    if (section) topic=rightstr(arg,strlen(arg)-pos);
    else topic=arg;
  }
  if (section) if (section<1 || section>9) {
    listen("Invalid section.\n");
    return 1;
  }
  if (strlen(topic)>8) topic=leftstr(topic,8)+"."+midstr(topic,9,3);
  if (section)
    if (topic) {
      if (cat("/help/help."+itoa(section)+"/"+topic)) {
        listen("No help available on that topic.\n");
        return 1;
      }
      return 1;
    } else
      cat("/help/contents."+itoa(section));
  else
    if (topic) {
      pos=0;
      while (pos<9 && !done) {
        pos++;
        if (!cat("/help/help."+itoa(pos)+"/"+topic)) done=1;
      }
      if (!done) listen("No help available on that topic.\n");
      return 1;
    } else
      cat("/help/contents.0");
  return 1;
}

static do_at_shout(arg) {
  if (!(get_flags() & FLAG_PROGRAMMER)) {
    listen("Permission denied.\n");
    return 1;
  }
  broadcast(get_name()+" shouts: "+arg+"\n");
  return 1;
}

static do_version(arg) {
  int version;

  version=sysctl(SYSCTL_VERSION);
  listen("NetCI version "+itoa(version/10000)+"."+itoa((version%10000)/100)+
         "."+itoa(version%100)+"\n");
  return 1;
}

static do_home(arg) {
  object curr,next,dest;

  if (!(get_flags() & FLAG_BUILDER)) {
    listen("Permission denied.\n");
    return 1;
  }
  if (!get_link()) set_link(atoo("/obj/room"));
  if (!get_link()) {
    listen("You aren't linked anywhere.\n");
    return 1;
  }
  curr=contents(this_object());
  while (curr) {
    next=next_object(curr);
    if (!curr.cannot_drop())
      if (dest=curr.get_link())
        move_object(curr,dest);
    curr=next;
  }
  if (location(this_object()))
    tell_room_except(location(this_object()),this_object(),NULL,HEAR_HOME,
                     NULL);
  move_object(this_object(),get_link());
  listen("There's no place like home...\n");
  listen("There's no place like home...\n");
  listen("There's no place like home...\n");
  do_look();
  tell_room_except(get_link(),this_object(),NULL,HEAR_ENTER,NULL);
  return 1;
}

static do_kill(arg) {
  object obj,curr,next,dest;
  string killmsg;

  if (!(get_flags() & FLAG_BUILDER)) {
    listen("Permission denied.\n");
    return 1;
  }
  obj=find_object(arg);
  if (!obj) {
    listen("You don't see that here.\n");
    return 1;
  }
  if (obj.get_type()!=TYPE_PLAYER) {
    listen("You can't kill that.\n");
    return 1;
  }
  if (location(this_object())!=location(obj)) {
    listen("You don't see that here.\n");
    return 1;
  }
  if (location(this_object()).get_flags() & FLAG_HAVEN) {
    listen("You cannot kill people in this room.\n");
    return 1;
  }
  if (!(dest=obj.get_link()))
    dest=atoo("/obj/room");
  if (location(this_object())) {
    tell_room_except2(location(this_object()),this_object(),obj,HEAR_KILLOTHER,
                      obj.get_odrop());
  }
  tell_player(obj,this_object(),obj,HEAR_KILLYOU,NULL);
  hear(this_object(),obj,HEAR_KILLER,obj.get_drop());
  if (location(this_object()))
    tell_room_except(location(this_object()),obj,NULL,HEAR_LEAVE,
                     NULL);
  if (dest==atoo("/obj/room") && !connected(obj))
    move_object(obj,NULL);
  else {
    move_object(obj,dest);
    tell_room_except(dest,obj,NULL,HEAR_ENTER,NULL);
  }
  curr=contents(obj);
  while (curr) {
    next=next_object(curr);
    if (dest=curr.get_link())
      if (!curr.cannot_drop())
        move_object(curr,dest);
    curr=next;
  }
  obj.do_look();
  return 1;
}

static do_at_doing(arg) {
  if (strlen(arg)>46)
    listen("Truncated.\n");
  else
    listen("Set.\n");
  doing_string=leftstr(arg,46);
  return 1;
}  

static do_at_exam(arg) {
  object o,curr;
  string s;

  if (!arg) o=location(this_object());
  else o=find_object(arg);
  if (!o) {
    listen("Couldn't find object.\n");
    return 1;
  }
/*  if (!(get_flags() & FLAG_BUILDER)) { */
/*     return 1; */
/*  } */
  if (!o.print_stats())
    if (priv(this_object())) {
      listen("Name: "+make_num(o)+"\n");
      listen("Type: *UNKNOWN*\n");
      s=otoa(o);
      s=leftstr(s,instr(s,1,"#")-1);
      listen("Source: "+s+".c\n");
      s=make_sys_flags(o);
      if (s) listen("System Flags:"+s+"\n");
      if (location(o))
        listen("Location: "+make_num(o)+"\n");
      else
        listen("Location: *VOID*\n");
      curr=contents(o);
      if (curr) {
        listen("Contents:\n");
        while (curr) {
          listen(make_num(curr)+"\n");
          curr=next_object(curr);
        }
      }
    } else {
      if (o.get_name()) {
	listen("Name: "+o.get_name()+".\n");
	listen("Owner: "+(o.get_owner()).get_name()+".\n");
      } else {
	listen("Unable to display any information about that object.\n");
      }
    }
  return 1;
}

static do_at_set(arg) {
  string obj_name,flag_name;
  object o;
  int f,newf,is_anti;

  if (!(get_flags() & FLAG_BUILDER) && !priv(this_object())) {
    listen("Permission denied.\n");
    return 1;
  }
  split_equal(arg,obj_name,flag_name);
  if (!obj_name) {
    listen("You must specify an object.\n");
    return 1;
  }
  if (leftstr(flag_name,1)=="!") {
    flag_name=rightstr(flag_name,strlen(flag_name)-1);
    is_anti=1;
  }
  if (!flag_name) {
    listen("You must specify a flag to set.\n");
    return 1;
  }
  o=find_object(obj_name);
  if (!o) {
    listen("Couldn't find object.\n");
    return 1;
  }
  f=o.get_flags();
  flag_name=downcase(flag_name);
  if (flag_name=="link_ok") newf=FLAG_LINK_OK;
  else if (flag_name=="dark") newf=FLAG_DARK;
  else if (flag_name=="sticky") newf=FLAG_STICKY;
  else if (flag_name=="builder") newf=FLAG_BUILDER;
  else if (flag_name=="jump_ok") newf=FLAG_JUMP_OK;
  else if (flag_name=="programmer") newf=FLAG_PROGRAMMER;
  else if (flag_name=="haven") newf=FLAG_HAVEN;
  else {
    listen("Flag unknown.\n");
    return 1;
  }
  if (!priv(this_object()) && (newf &
      (FLAG_BUILDER | FLAG_PROGRAMMER))) {
    listen("Permission denied.\n");
    return 1;
  }
  if (o.get_type()==TYPE_PLAYER && newf==FLAG_DARK &&
      !priv(this_object()) && !(get_flags() & FLAG_PROGRAMMER)) {
    listen("Permission denied.\n");
    return 1;
  }
  if (is_anti) f&=~newf;
  else f|=newf;
  if (o.set_flags(f)) {
    listen("Set.\n");
    return 1;
  } else {
    listen("Permission denied.\n");
    return 1;
  }
}

static do_at_force(arg) {
  string obj_name,cmd;
  object o;

  if (!(get_flags() & FLAG_BUILDER)) {
    listen("Permission denied.\n");
    return 1;
  }
  split_equal(arg,obj_name,cmd);
  if (!obj_name) {
    listen("You must specify a player to @force.\n");
    return 1;
  }
  o=find_object(obj_name);
  if (!o) {
    listen("Couldn't find object.\n");
    return 1;
  }
  if (o.force_object(cmd)) listen("Forced.\n");
  else listen("Permission denied.\n");
  return 1;
}

static do_at_boot(arg) {
  object o;

  if (!priv(this_object())) {
    listen("Permission denied.\n");
    return 1;
  }
  o=find_object(arg);
  if (!o) {
    listen("Couldn't find object.\n");
    return 1;
  }
  if (!connected(o)) {
    listen("That object isn't connected.\n");
    return 1;
  }
  if (o.force_disconnect())
    listen("Booted.\n");
  else
    listen("Failed.\n");
  return 1;
}

static do_at_recycle(arg) {
  object o;

  if (!(get_flags() & FLAG_BUILDER)) {
    listen("Permission denied.\n");
    return 1;
  }
  if (!arg) {
    listen("You must specify an object to recycle.\n");
    return 1;
  }
  o=find_object(arg);
  if (!o) {
    listen("Couldn't find object.\n");
    return 1;
  }
  if (o==get_link()) {
    listen("You can't recycle your home.\n");
    return 1;
  }
  if (o==this_object()) {
    listen("Self-recycling is strictly verboten.\n");
    return 1;
  }
  if (contents(o)) {
    listen("You can't recycle an object that contains things.\n");
    return 1;
  }
  if (leftstr(otoa(o),12)=="/obj/player#") {
    if (!priv(this_object())) {
      listen("Permission denied.\n");
      return 1;
    }
    table_delete(downcase(o.get_name()));
  }
  if (o.recycle())
    listen("Recycled.\n");
  else
    listen("Permission denied.\n");
  return 1;
}

static do_at_password(arg) {
  string old,new_pass;

  split_equal(arg,old,new_pass);
  if (!check_password(old)) {
    listen("Wrong password.\n");
    return 1;
  }
  if (!new_pass) {
    listen("New password cannot be null.\n");
    return 1;
  }
  set_password(new_pass);
  listen("Password changed.\n");
  return 1;
}

static do_at_tel(arg) {
  object obj,dest,orig_room;
  string obj_name,dest_name;

  if (!(get_flags() & FLAG_BUILDER)) {
    listen("Permission denied.\n");
    return 1;
  }
  split_equal(arg,obj_name,dest_name);
  if (!obj_name || !dest_name) {
    listen("You must specify an object and a destination.\n");
    return 1;
  }
  obj=find_object(obj_name);
  if (!obj) {
    listen("You don't see that here.\n");
    return 1;
  }
  dest=find_object(dest_name);
  if (!dest) {
    listen("You can't find that destination.\n");
    return 1;
  }
  if (!obj.can_teleport(this_object())) {
    listen("Permission denied.\n");
    return 1;
  }
  if (!dest.can_teleport_to(this_object(),obj)) {
    listen("Permission denied.\n");
    return 1;
  }
  if (location(this_object()))
    if (!location(this_object()).can_teleport_to(this_object(),obj)) {
      listen("Permission denied.\n");
      return 1;
    }
  if (obj.get_type()==TYPE_PLAYER) {
    clear_room();
    orig_room=location(obj);
    move_object(obj,dest);
    if (orig_room)
      tell_room_except(orig_room,obj,NULL,HEAR_LEAVE,NULL);
    tell_room_except(dest,obj,NULL,HEAR_ENTER,NULL);
    obj.do_look(NULL);
  } else
    move_object(obj,dest);
  listen("Teleported.\n");
  return 1;
}

static do_at_unlock(arg) {
  object obj;

  if (!(get_flags() & FLAG_BUILDER)) {
    listen("Permission denied.\n");
    return 1;
  }
  obj=find_object(arg);
  if (!obj) {
    listen("You don't see that here.\n");
    return 1;
  }
  if (!obj.set_lock(0)) {
    listen("Permission denied.\n");
    return 1;
  }
  listen("Unlocked.\n");
  return 1;
}

static do_at_lock(arg) {
  string obj_name,lock_string;
  object obj,new_bool;

  if (!(get_flags() & FLAG_BUILDER)) {
    listen("Permission denied.\n");
    return 1;
  }
  split_equal(arg,obj_name,lock_string);
  if (!obj_name) {
    listen("You must specify an object.\n");
    return 1;
  }
  obj=find_object(obj_name);
  if (!obj) {
    listen("You don't see that here.\n");
    return 1;
  }
  while (leftstr(lock_string,1)==" ")
    lock_string=rightstr(lock_string,strlen(lock_string)-1);
  if (lock_string) {
    new_bool=new("/sys/bool");
    if (new_bool.parse_bool_exp(lock_string,this_object())==(-1))
      return 1;
    new_bool=new_bool.eliminate_parentheses();
  } else
    new_bool=0;
  if (!obj.set_lock(new_bool)) {
    if (new_bool) new_bool.recycle_bool();
    listen("Permission denied.\n");
    return 1;
  }
  listen("Locked.\n");
  return 1;
}

static do_at_unlink(arg) {
  object obj;

  if (!(get_flags() & FLAG_BUILDER)) {
    listen("Permission denied.\n");
    return 1;
  }
  if (!arg) {
    listen("You must specify an object to unlink.\n");
    return 1;
  }
  obj=find_object(arg);
  if (!obj) {
    listen("You don't see that here.\n");
    return 1;
  }
  if (!obj.set_link(0)) {
    listen("Permission denied.\n");
    return 1;
  }
  listen("Unlinked.\n");
  return 1;
}

static do_at_link(arg) {
  string obj_name,dest_name;
  object obj,dest;

  if (!(get_flags() & FLAG_BUILDER)) {
    listen("Permission denied.\n");
    return 1;
  }
  split_equal(arg,obj_name,dest_name);
  if (!obj_name) {
    listen("You must specify an object.\n");
    return 1;
  }
  obj=find_object(obj_name);
  if (!obj) {
    listen("You don't see that here.\n");
    return 1;
  }
  if (dest_name) {
    if (dest_name!="home") {
      dest=find_object(dest_name);
      if (!dest) {
	listen("You can't find that destination.\n");
	return 1;
      }
    } else if (obj.get_type()!=TYPE_PLAYER) dest="home";
  }
  if (!obj.set_link(dest)) {
    listen("Permission denied.\n");
    return 1;
  }
  listen("Linked.\n");
  return 1;
}

static do_at_system(arg) {
  string func,value;
  int ncf,is_anti;

  if (!priv(this_object())) {
    listen("Permission denied.\n");
    return 1;
  }
  if (!arg) {
    if (sys_combat_enabled()) listen("combat=on\n");
    else listen("combat=off\n");
    listen("doingquery="+sys_get_doingquery()+"\n");
    if (sys_find_disabled()) listen("find=off\n");
    else listen("find=on\n");
    if (sys_names_disabled()) listen("Name Changing=off\n");
    else listen("Name Changing=on\n");
    if (sys_guests_disabled()) listen("guests=off\n");
    else listen("guests=on\n");
    ncf=sys_get_newchar_flags();
    if (ncf==FLAG_BUILDER) listen("newchar=builder\n");
    else if (ncf==FLAG_PROGRAMMER) listen("newchar=programmer\n");
    else if (ncf==(FLAG_BUILDER | FLAG_PROGRAMMER))
      listen("newchar=builder programmer\n");
    else listen("newchar=*NONE*\n");
    if (sys_get_registration()) listen("registration=on\n");
    else listen("registration=off\n");
    listen("whoquote="+sys_get_whoquote()+"\n");
    return 1;
  }
  split_equal(arg,func,value);
  func=downcase(func);
  if (func=="newchar") {
    value=downcase(value);
    if (leftstr(value,1)=="!") {
      is_anti=1;
      value=rightstr(value,strlen(value)-1);
    }
    ncf=sys_get_newchar_flags();
    if (value=="builder") {
      if (is_anti) ncf&=~FLAG_BUILDER;
      else ncf|=FLAG_BUILDER;
    } else if (value=="programmer") {
      if (is_anti) ncf&=!FLAG_PROGRAMMER;
      else ncf|=FLAG_PROGRAMMER;
    } else {
      listen("Invalid flag.\n");
      return 1;
    }
    sys_set_newchar_flags(ncf);
    listen("Set.\n");
    return 1;
  } else if (func=="registration") {
    if (value=="on") sys_set_registration(1);
    else if (value=="off") sys_set_registration(0);
    else {
      listen("Must be 'on' or 'off'.\n");
      return 1;
    }
    listen("Set.\n");
    return 1;
  } else if (func=="combat") {
    if (value=="on") sys_set_combat_enabled(1);
    else if (value=="off") sys_set_combat_enabled(0);
    else {
      listen("Must be 'on' or 'off'.\n");
      return 1;
    }
    listen("Set.\n");
    return 1;
  } else if (func=="find") {
    if (value=="on") sys_set_find_disabled(0);
    else if (value=="off") sys_set_find_disabled(1);
    else {
      listen("Must be 'on' or 'off'.\n");
      return 1;
    }
    listen("Set.\n");
    return 1;
  } else if (func=="guests") {
    if (value=="on") sys_set_guests_disabled(0);
    else if (value=="off") sys_set_guests_disabled(1);
    else {
      listen("Must be 'on' or 'off'.\n");
      return 1;
    }
    listen("Set.\n");
    return 1;
  } else if (func=="doingquery") {
    sys_set_doingquery(value);
    listen("Set.\n");
    return 1;
  } else if (func=="whoquote") {
    sys_set_whoquote(value);
    listen("Set.\n");
    return 1;
  } else if (func=="names") {
    if (value=="on") sys_set_names_disabled(0);
    else if (value=="off") sys_set_names_disabled(1);
    else {
      listen("Must be 'on' or 'off'.\n");
      return 1;
    }
    listen("Set.\n");
    return 1;
  } 
  listen("Invalid system variable.\n");
  return 1;
}

static do_at_create(arg) {
  object o;

  if (!(get_flags() & FLAG_BUILDER)) {
    listen("Permission denied.\n");
    return 1;
  }
  if (!arg) {
    listen("You must specify a name for the object.\n");
    return 1;
  }
  o=new("/obj/object");
  if (!o) {
    listen("Unable to clone /obj/object.\n");
    listen("Contact the game administrator.\n");
    return 1;
  }
  o.set_owner(this_object());
  o.set_name(arg);
  if (location(this_object())) {
    if (!o.set_link(location(this_object())))
      if (!o.set_link(get_link())) {
        listen("Cannot set link for object.\n");
        o.recycle();
        return 1;
      }
  } else
    if (!o.set_link(get_link())) {
      listen("Cannot set link for object.\n");
      o.recycle();
      return 1;
    }
  move_object(o,this_object());
  listen("Object #"+itoa(otoi(o))+" created.\n");
  return 1;
}

static do_at_open(arg) {
  object o,dest;
  string exit_name,dest_name;

  if (!(get_flags() & FLAG_BUILDER)) {
    listen("Permission denied.\n");
    return 1;
  }
  split_equal(arg,exit_name,dest_name);
  if (!exit_name) {
    listen("You must specify an exit name.\n");
    return 1;
  }
  if (!location(this_object())) {
    listen("Can't create exit from void.\n");
    return 1;
  }
  if (location(this_object()).get_type()!=TYPE_ROOM) {
    listen("Can only create exits in rooms.\n");
    return 1;
  }
  if (!priv(this_object()) && location(this_object()).get_owner()!=
      this_object()) {
    listen("Permission denied.\n");
    return 1;
  }
  if (dest_name) {
    if (dest_name!="home") {
      dest=find_object(dest_name);
      if (!dest) {
	listen("Couldn't find destination.\n");
	return 1;
      }
      if (!dest.can_link(this_object())) {
	listen("Permission denied.\n");
	return 1;
      }
    } else dest="home";
  }
  o=new("/obj/exit");
  if (!o) {
    listen("Couldn't clone /obj/exit.\n");
    listen("Contact a game administrator.\n");
    return 1;
  }
  o.set_owner(this_object());
  o.set_name(exit_name);
  move_object(o,location(this_object()));
  listen("Exit #"+itoa(otoi(o))+" created.\n");
  if (dest) {
    o.set_link(dest);
    listen("Exit #"+itoa(otoi(o))+" linked.\n");
  }
  return 1;
}

static do_at_dig(arg) {
  object o;
  string name,desc;

  if (!(get_flags() & FLAG_BUILDER)) {
    listen("Permission denied.\n");
    return 1;
  }
  split_equal(arg,name,desc);
  if (!name) {
    listen("You must specify a room name.\n");
    return 1;
  }
  o=new("/obj/room");
  if (!o) {
    listen("Unable to clone /obj/room.\n");
    listen("Contact a game administrator.\n");
    return 1;
  }
  o.set_owner(this_object());
  o.set_name(arg);
  o.set_desc(desc);
  listen("Room #"+itoa(otoi(o))+" created.\n");
  return 1;
}

static do_at_chown(arg) {
  object o,new_owner;
  string obj_name,owner_name;

  if (!(get_flags() & FLAG_BUILDER)) {
    listen("Permission denied.\n");
    return 1;
  }
  split_equal(arg,obj_name,owner_name);
  if (!obj_name) {
    listen("You must specify an object name.\n");
    return 1;
  }
  if (!owner_name) {
    listen("You must specify a new owner.\n");
    return 1;
  }
  o=find_object(obj_name);
  if (!o) {
    listen("Can't find that object.\n");
    return 1;
  }
  if (o==this_object()) {
    listen("You can't @chown yourself.\n");
    return 1;
  }
  new_owner=find_object(owner_name);
  if (!new_owner) {
    listen("Can't find new owner.\n");
    return 1;
  }
  if (o.set_owner(new_owner)) {
    listen("Owner changed.\n");
    return 1;
  } else {
    listen("Permission denied.\n");
    return 1;
  }
}

static do_at_newpassword(arg) {
  string pname,new_passwd;
  object o;

  if (!priv(this_object())) {
    listen("Permission denied.\n");
    return 1;
  }
  split_equal(arg,pname,new_passwd);
  if (!pname) {
    listen("You must specify a player name.\n");
    return 1;
  }
  if (!new_passwd) {
    listen("You must specify a new password.\n");
    return 1;
  }
  o=find_object(pname);
  if (!o) {
    listen("Cannot find that player.\n");
    return 1;
  }
  if (o.get_type()!=TYPE_PLAYER) {
    listen("That object is not a player.\n");
    return 1;
  }
  if (o.set_password(new_passwd)) {
    listen("Password changed.\n");
    return 1;
  } else {
    listen("Unable to change password.\n");
    return 1;
  }
}

static do_at_save(arg) {
  object curr;

  if (!priv(this_object())) {
    listen("Permission denied.\n");
    return 1;
  }
  while (curr=next_who(curr)) write(curr,"\n*** Saving Database ***\n\n");
  flush_device();
  if (sysctl(SYSCTL_SAVE)) {
    listen("Save failed.\n");
    return 1;
  } else {
    listen("Saved.\n");
    return 1;
  }
}

static do_at_shutdown(arg) {
  object curr;

  if (!priv(this_object())) {
    listen("Permission denied.\n");
    return 1;
  }
  while (curr=next_who(curr)) write(curr,"\n*** Shutting Down ***\n\n");
  flush_device();
  if (sysctl(SYSCTL_SHUTDOWN)) {
    listen("Shutdown failed.\n");
    return 1;
  } else {
    listen("Shutting down.\n");
    return 1;
  }
}

static do_at_panic(arg) {
  if (!priv(this_object())) {
    listen("Permission denied.\n");
    return 1;
  }
  if (sysctl(SYSCTL_PANIC)) {
    listen("Panic failed.\n");
    return 1;
  } else {
    listen("Panicking.\n");
    return 1;
  }
}

static do_at_priv(arg) {
  object o;

  if (!priv(this_object())) {
    listen("Permission denied.\n");
    return 1;
  }
  if (!arg) {
    listen("Couldn't find object.\n");
    return 1;
  }
  o=find_object(arg);
  if (!o) {
    listen("Couldn't find object.\n");
    return 1;
  }
  if (priv(o)) {
    listen("Object is already privileged.\n");
    return 1;
  }
  set_priv(o,TRUE);
  listen("Object is now priveleged.\n");
  return 1;
}

static do_at_unpriv(arg) {
  object o;

  if (!priv(this_object())) {
    listen("Permission denied.\n");
    return 1;
  }
  if (!arg) {
    listen("Couldn't find object.\n");
    return 1;
  }
  o=find_object(arg);
  if (!o) {
    listen("Couldn't find object.\n");
    return 1;
  }
  if (!priv(o)) {
    listen("That object isn't privileged.\n");
    return 1;
  }
  if (o==this_object()) {
    listen("Can't remove privileges from yourself.\n");
    return 1;
  }
  if (o==itoo(0)) {
    listen("Can't remove privileges from boot object.\n");
    return 1;
  }
  set_priv(o,FALSE);
  listen("Object is no longer privileged.\n");
  return 1;
}

static do_at_sync(arg) {
  object o;

  if (!priv(this_object())) {
    listen("Permission denied.\n");
    return 1;
  }
  if (!arg) {
    listen("You must specify an object to sync.\n");
    return 1;
  }
  o=find_any_object(arg);
  if (!o) {
    listen("Couldn't find specified object.\n");
    return 1;
  }
  if (!prototype(o)) {
    listen("Object isn't a prototype.\n");
    return 1;
  }
  o.sync();
  listen("Sync'd.\n");
  return 1;
}

static do_at_package(arg) {
  if (!(get_flags() & FLAG_PROGRAMMER)) {
    listen("Permission denied.\n");
    return 1;
  }
  if (arg) {
    if (!priv(this_object())) {
      listen("Permission denied.\n");
      return 1;
    }
    listen("Building package \""+arg+"\"...\n");
    flush_device(this_object());
    add_pkg(arg);
    return 1;
  }
  listen("Package  Version\n");
  listen("-------- -------\n");
  if (cat("/etc/pkg/packages.lst")) listen("<no packages installed>\n");
  return 1;
}

static do_at_pcreate(arg) {
  string name,passwd;
  object o,dest;

  if (!priv(this_object())) {
    listen("Permission denied.\n");
    return 1;
  }
  split_equal(arg,name,passwd);
  if (!name) {
    listen("You must specify a player name.\n");
    return 1;
  }
  if (!passwd) {
    listen("You must specify a player password.\n");
    return 1;
  }
  if (!is_legal_name(name)) {
    listen("Illegal name.\n");
    return 1;
  }
  if (find_player(name)) {
    listen("That player already exists.\n");
    return 1;
  }
  if (!is_legal_name(name)) {
    listen("Illegal player name.\n");
    return 1;
  }
  dest=atoo("/obj/room");
  if (!dest) {
    listen("No room to link player to.\n");
    return 1;
  }
  o=new("/obj/player");
  if (!o) {
    listen("Unable to clone /obj/player.\n");
    listen("This error is absolutely ridiculous.\n");
    return 1;
  }
  o.set_secure();
  o.set_name(name);
  o.set_password(passwd);
  o.set_link(dest);
  o.set_flags(sys_get_newchar_flags());
  o.set_owner(o);
  move_object(o,dest);
  tell_room_except(dest,this_object(),NULL,HEAR_ENTER,NULL);
  o.set_dest(dest);
  add_new_player(name,o);
  listen("Player #"+itoa(otoi(o))+" created.\n");
  return 1;
}

additional_stats(rcpt) {
  sharobj.additional_stats(rcpt);
  if (get_possessor() && (get_possessor()==rcpt || priv(rcpt)))
    write(rcpt,"Possessed By: "+make_num(get_possessor())+"\n");
  if (possessing)
    write(rcpt,"Possessing: "+make_num(possessing)+"\n");
}

id(arg) {
  return (leftstr(downcase(get_name()),strlen(arg))==downcase(arg));
}

get_wrap(arg) {
  return wrap;
}

listen(arg) {
  string owner,perms,filename,arg2,narg;
  int pos,cpos,lpos,spos,lbreak;

  if (doing_ls) {
    if (ls_dir) print_dot_dirs();
    dir_count++;
    pos=instr(arg,1," ");
    owner=atoi(leftstr(arg,pos-1));
    arg2=rightstr(arg,strlen(arg)-pos);
    pos=instr(arg2,1," ");
    perms=atoi(leftstr(arg2,pos-1));
    filename=rightstr(arg2,strlen(arg2)-pos);
    arg2=itoo(owner);
    if (!arg2) owner="NULL OWNER #"+itoa(owner);
    else {
      arg2=arg2.get_uid();
      if (arg2) owner=arg2;
      else owner=itoa(owner);
    }
    pos=strlen(owner);
    while (pos++<16) owner+=" ";
    if (perms & DIRECTORY) arg2="d";
    else arg2="-";
    if (perms & READ_OK) arg2+="r";
    else arg2+="-";
    if (perms & WRITE_OK) arg2+="w";
    else arg2+="-";
    if (get_possessor())
      get_possessor().possession_report("%"+arg2+" "+owner+" "+filename);
    if (use_crlf) filename=leftstr(filename,strlen(filename)-1)+"\r\n";
    send_device(arg2+" "+owner+" "+filename);
  } else {
    if (wrap>0) {
      spos=1;
      cpos=0;
      lbreak=1;
      while ((strlen(arg)-lbreak)>wrap) {
	lpos=instr(arg,spos," ");
	if (lpos!=0) {
	  while ((lpos-spos)>wrap) {
	    narg=narg+midstr(arg,spos,((wrap-cpos)-1))+"-\n";
	    spos=spos+(wrap-cpos)-1;
	    lbreak=spos;
	    cpos=0; 
	  }
	  if (((lpos-spos)+cpos)>=wrap) {
	    narg=narg+"\n";
	    narg=narg+midstr(arg,spos,(lpos-spos))+" ";
	    cpos=(lpos-spos)+1;
	    lbreak=spos;
	    spos=lpos+1;
	  } else {
	    narg=narg+midstr(arg,spos,(lpos-spos))+" ";
	    cpos=cpos+((lpos-spos)+1);
	    spos=lpos+1;
	  }
	} else {
	  while ((strlen(arg)-spos) > wrap) {
	    narg=narg+midstr(arg,spos,(wrap-cpos-1))+"-\n";
	    spos=spos+(wrap-cpos)-1;
	    lbreak=spos;
	    cpos=0;
	  }
	  if (((strlen(arg)-spos)+cpos)>wrap) {
	    narg=narg+"\n";
	    lbreak=strlen(arg);
	  } else {
	    lbreak=strlen(arg);
	  }
	}
      }
      narg=narg+rightstr(arg,(strlen(arg)-(spos-1)));
      arg=narg;
    }
    if (get_possessor())
      get_possessor().possession_report("%"+arg);
    if (use_crlf) {
      pos=1;
      while (pos=instr(arg,pos,"\n")) {
/* Added arg= so that it works -snooze */
        arg=subst(arg,pos,1,"\r\n");
        pos+=2;
      }
    }
    send_device(arg);
  }
}

hear(actor,actee,type,message,pre,post) {
    if (type==HEAR_SAY) {
      listen(pre+actor.get_name()+" says \""+message+"\" "+post+"\n");
    } else if (type==HEAR_POSE) {
      listen(pre+actor.get_name()+" "+message+" "+post+"\n");
    } else if (type==HEAR_CONNECT) {
      listen(pre+actor.get_name()+" has connected. "+post+"\n");
    } else if (type==HEAR_DISCONNECT) {
      listen(pre+actor.get_name()+" has disconnected. "+post+"\n");
    } else if (type==HEAR_ENTER) {
      if (message) listen(pre+actor.get_name()+" "+gender_subs(actor,message)+" "+post+"\n");
      listen(pre+actor.get_name()+" has arrived. "+post+"\n");
    } else if (type==HEAR_LEAVE) {
      if (message) listen(pre+actor.get_name()+" "+gender_subs(actor,message)+" "+post+"\n");
      listen(pre+actor.get_name()+" has left. "+post+"\n");
    } else if (type==HEAR_KILLOTHER) {
      listen(pre+actor.get_name()+" killed "+actee.get_name()+"! "+
	     gender_subs(actor,message)+" "+post+"\n");
    } else if (type==HEAR_KILLYOU) {
      listen(pre+actor.get_name()+" killed you! "+post+"\n");
    } else if (type==HEAR_KILLER) {
      if (message) listen(pre+message+" "+post+"\n");
      else listen(pre+"You killed "+actee.get_name()+"! "+post+"\n");
    } else if (type==HEAR_WHISPER) {
      listen(pre+actor.get_name()+" whispers \""+message+"\" "+post+"\n");
    } else if (type==HEAR_PAGE) {
      if (message) listen(actor.get_name()+" pages: "+message+"\n");
      else
	listen("You sense that "+actor.get_name()+" is looking for you in "+
	       (location(actor)?location(actor).get_name():"the void")+".\n");
    } else if (type==HEAR_HOME) {
      listen(pre+actor.get_name()+" goes home. "+post+"\n");
      listen(pre+actor.get_name()+" has left. "+post+"\n");
    } else if (type==HEAR_FAIL) {
      if (message) listen(pre+actor.get_name()+" "+gender_subs(actor,message)+" "+post+"\n");
    } else if (type==HEAR_GET) {
      if (message) listen(pre+actor.get_name()+" "+gender_subs(actor,message)+" "+post+"\n");
    } else if (type==HEAR_DROP) {
      if (message) listen(pre+actor.get_name()+" "+gender_subs(actor,message)+" "+post+"\n");
      else
	listen(pre+actor.get_name()+" dropped "+actee.get_name()+". "+post+"\n");
    } else if (type==HEAR_LOOK) {
      if (message) listen(actor.get_name()+" "+gender_subs(actor,message)+"\n");
    } else if (type==HEAR_STAGE) {
      if (actor==this_object()) listen(pre+"You [to "+actee.get_name()+"]: "+message+" "+post+"\n");
      else if (actee==this_object()) listen(pre+actor.get_name()+" [to you]: "+message+" "+post+"\n");
      else listen(pre+actor.get_name()+" [to "+actee.get_name()+"]: "+message+" "+post+"\n");
    } else if (type==HEAR_PASTE) {
      listen(message);
    } else if (type==HEAR_ATTACK) {
      if (actor==this_object()) listen(pre+"You start attacking "+actee.get_name()+". "+post+"\n");
      else if (actee=this_object()) listen(pre+actor.get_name()+" starts attacking you. "+post+"\n");
      else listen(pre+actor.get_name()+" starts attacking "+actee.get_name()+". "+post+"\n");
    } else if (type==HEAR_DIE) {
      if (actee==this_object()) listen(pre+"You have died. "+post+"\n");
      else if (actor==this_object()) listen(pre+"You killed "+actee.get_name()+". "+post+"\n");
      else listen(pre+actor.get_name()+" killed "+actee.get_name()+". "+post+"\n");
    } else if (type==HEAR_HIT) {
      
    } else if (type==HEAR_NOSPACE) {
      listen(pre+actor.get_name()+message+" "+post+"\n");
    } else {
      listen("Unknown message #"+itoa(type)+" \""+message+"\" from "+
	     ((typeof(actor)==OBJECT_T)?otoa(actor):"nobody")+" regarding "+
	     ((typeof(actee)==OBJECT_T)?otoa(actee):"nobody")+"\n");
    }
    }

sync() {
  if (!priv(caller_object())) return 0;
  if (!prototype(this_object())) return 1;
  read_verb_file("/obj/player.vrb");
  return 1;
  }

static init() {
  sharobj=new("/obj/share/living");
  attach(sharobj);
  set_type(TYPE_PLAYER);
  path="/";
  set_owner(this_object());
  set_interactive(TRUE);
  if (!prototype(this_object())) {
    check_secure(SECURE_PRIV,NULL);
    return;
  }
  sync();
}
