/* boot.c */

#define SAVE_DELAY 3600
#define STARTING_PACKAGE "core"
#define PKGLIST_FILE "/etc/pkg/packages.lst"
#define TMP_FILE "/etc/pkg/packages.tmp"

string pkg_to_build;
object report_to;

pkg_exists(pkg) {
  int filepos,line;

  while (line=fread(PKGLIST_FILE,filepos)) {
    if (rightstr(line,1)=="\n") line=leftstr(line,strlen(line)-1);
    if (rightstr(line,1)=="\r") line=leftstr(line,strlen(line)-1);
    if (line==pkg) return 1;
  }
  return 0;
}

static add_pkg_to_list(pkg,version) {
  int filepos,line,added_pkg;

  while (strlen(pkg)<8) pkg+=" ";
  if (fstat(PKGLIST_FILE)!=-1) while (line=fread(PKGLIST_FILE,filepos)) {
    if (rightstr(line,1)=="\n") line=leftstr(line,strlen(line)-1);
    if (rightstr(line,1)=="\r") line=leftstr(line,strlen(line)-1);
    if (leftstr(line,8)==pkg) {
      added_pkg=1;
      line=pkg+" "+version;
    } else if (leftstr(line,8)>pkg && !added_pkg) {
      fwrite(TMP_FILE,pkg+" "+version+"\n");
      added_pkg=1;
    }
    fwrite(TMP_FILE,line+"\n");
  }
  if (!added_pkg) fwrite(TMP_FILE,pkg+" "+version+"\n");
  cp(TMP_FILE,PKGLIST_FILE);
  rm(TMP_FILE);
  chmod(PKGLIST_FILE,1);
}

add_pkg(pkg) {
  if (!priv(caller_object())) return 0;
  pkg_to_build=pkg;
  report_to=this_player();
  alarm(0,"alarm_add_pkg");
  return 1;
}

static alarm_add_pkg() {
  string filename,cmd,cmdname,args,arg1,arg2,pkg,version_str;
  int filepos,pos,linecount,flags,version_count;

  pkg=pkg_to_build;
  filename="/etc/pkg/"+pkg+".pkg";
  if (fstat(filename)==-1) {
    unhide(filename,itoo(0),1);
    if (fread(filename,filepos)==(-1)) {
      hide(filename);
      if (report_to)
        report_to.listen("Package \""+pkg+"\" does not exist.\n");
      return;
    }
  }
  filepos=0;
  linecount=0;
  version_count=0;
  version_str="<unknown>";
  while (cmd=fread(filename,filepos)) {
    linecount++;
    while (leftstr(cmd,1)==" ") cmd=rightstr(cmd,strlen(cmd)-1);
    while (rightstr(cmd,1)=="\n") cmd=leftstr(cmd,strlen(cmd)-1);
    while (rightstr(cmd,1)=="\r") cmd=leftstr(cmd,strlen(cmd)-1);
    while (rightstr(cmd,1)==" ") cmd=leftstr(cmd,strlen(cmd)-1);
    if (cmd)
      if (leftstr(cmd,1)!="#") {
        pos=instr(cmd,1," ");
        if (pos) {
          cmdname=leftstr(cmd,pos-1);
          args=rightstr(cmd,strlen(cmd)-pos);
        } else {
          cmdname=cmd;
          args=0;
        }
        while (leftstr(args,1)==" ") args=rightstr(args,strlen(args)-1);
        if (cmdname=="version") {
          if (version_count++) {
            if (report_to)
              report_to.listen("Package \""+pkg+"\" build failed at line #"+
                               itoa(linecount)+"\n");
            return;
          }
          version_str=args;
        } else if (cmdname=="file") {
          pos=instr(args,1," ");
          if (pos) {
            arg1=leftstr(args,pos-1);
            arg2=rightstr(args,strlen(args)-pos);
            while (leftstr(arg2,1)==" ") arg2=rightstr(arg2,strlen(arg2)-1);
          } else {
            arg1=0;
            arg2=args;
          }
          if (arg1=="r") flags=1;
          else if (arg1=="w") flags=2;
          else if (arg1=="rw" || arg1=="wr") flags=3;
          else if (!arg1) flags=0;
          else {
            if (report_to)
              report_to.listen("Package \""+pkg+"\" build failed at line #"+
                               itoa(linecount)+"\n");
            return;
          }
          unhide(arg2,itoo(0),flags);
        } else if (cmdname=="compile") {
          if (args=="/boot") {
            if (atoo("/sys/bcompile"))
              atoo("/sys/bcompile").bcompile();
          } else compile_object(args);
        } else if (cmdname=="sync") {
          if (args=="/boot") {
            if (atoo("/sys/bcompile"))
              atoo("/sys/bcompile").bsync();
          } else if (atoo(args))
            atoo(args).sync();
        } else if (cmdname=="directory") {
          pos=instr(args,1," ");
          if (pos) {
            arg1=leftstr(args,pos-1);
            arg2=rightstr(args,strlen(args)-pos);
            while (leftstr(arg2,1)==" ") arg2=rightstr(arg2,strlen(arg2)-1);
          } else {
            arg1=0;
            arg2=args;
          }
          if (arg1=="r") flags=5;
          else if (arg1=="w") flags=6;
          else if (arg1=="rw" || arg1=="wr") flags=7;
          else if (!arg1) flags=4;
          else {
            if (report_to)
              report_to.listen("Package \""+pkg+"\" build failed at line #"+
                               itoa(linecount)+"\n");
            return;
          }
          unhide(arg2,itoo(0),flags);
        } else {
          if (report_to)
            report_to.listen("Package \""+pkg+"\" build failed at line #"+
                             itoa(linecount)+"\n");
          return;
        }
      }
  }
  add_pkg_to_list(pkg,version_str);
  if (report_to) report_to.listen("Package built\n");
  return;
}

get_version(pkg) {
  string filename,cmd,args,cmdname;
  int filepos,pos;

  filename="/etc/pkg/"+pkg+".pkg";
  filepos=0;
  while (cmd=fread(filename,filepos)) {
    while (leftstr(cmd,1)==" ") cmd=rightstr(cmd,strlen(cmd)-1);
    while (rightstr(cmd,1)=="\n") cmd=leftstr(cmd,strlen(cmd)-1);
    while (rightstr(cmd,1)=="\r") cmd=leftstr(cmd,strlen(cmd)-1);
    while (rightstr(cmd,1)==" ") cmd=leftstr(cmd,strlen(cmd)-1);
    if (cmd)
      if (leftstr(cmd,1)!="#") {
        pos=instr(cmd,1," ");
        if (pos) {
          cmdname=leftstr(cmd,pos-1);
          args=rightstr(cmd,strlen(cmd)-pos);
        } else {
          cmdname=cmd;
          args=0;
        }
        while (leftstr(args,1)==" ") args=rightstr(args,strlen(args)-1);
        if (cmdname=="version") {
          return args;
        }
      }
  }
  return 0;
}

/* Helper function to check if caller has a specific role */
static has_role(caller, role_name) {
  string name;
  
  if (!caller) return 0;
  name = caller.get_name();
  if (!name) return 0;
  
  return (downcase(name) == downcase(role_name));
}

/* Helper function to check if path starts with a prefix */
static path_starts_with(path, prefix) {
  int prefix_len;
  if (!path || !prefix) return 0;
  prefix_len = strlen(prefix);
  if (strlen(path) < prefix_len) return 0;
  return (leftstr(path, prefix_len) == prefix);
}

/* Helper function to check if path is in user's home directory */
static is_in_home_dir(path, username) {
  int next_slash;
  string path_user;
  if (!path || !username) return 0;
  if (!path_starts_with(path, "/home/")) return 0;
  next_slash = instr(path, 7, "/");
  if (!next_slash) return 0;
  path_user = midstr(path, 7, next_slash - 7);
  return (downcase(path_user) == downcase(username));
}

/* BASIC SECURITY (commented out - using role-based below)
valid_read(path, func, caller, owner, flags) {
  if (!caller) return 1;
  if (priv(caller)) return 1;
  if (flags == -1) return 1;
  if (owner == otoi(caller)) return 1;
  if (flags & 1) return 1;
  return 0;
}

valid_write(path, func, caller, owner, flags) {
  if (!caller) return 1;
  if (priv(caller)) return 1;
  if (flags == -1) {
    if (priv(caller)) return 1;
    return 0;
  }
  if (owner == otoi(caller)) return 1;
  if (flags & 2) return 1;
  return 0;
}
*/

/* ROLE-BASED SECURITY */
valid_read(path, func, caller, owner, flags) {
  int caller_flags;
  string caller_name;
  
  /* System access - always allow */
  if (!caller) return 1;
  
  /* Privileged objects - always allow */
  if (priv(caller)) return 1;
  
  /* Non-existent files - allow read attempts */
  if (flags == -1) return 1;
  
  /* Get caller info once */
  caller_flags = caller.get_flags();
  caller_name = caller.get_name();
  
  /* Wizard: Full read access except boot.c */
  if (has_role(caller, "Wizard")) {
    if (path == "/boot.c") return 0;
    return 1;
  }
  
  /* Programmer (flag 32): Read all except boot.c and /sys */
  if (caller_flags & 32) {
    if (path == "/boot.c") return 0;
    if (path == "/sys" || path_starts_with(path, "/sys/")) return 0;
    return 1;
  }
  
  /* Builder (flag 8): Read all except boot.c and /sys */
  if (caller_flags & 8) {
    if (path == "/boot.c") return 0;
    if (path == "/sys" || path_starts_with(path, "/sys/")) return 0;
    return 1;
  }
  
  /* Regular user: owner, home directory, or READ_OK flag */
  if (owner == otoi(caller)) return 1;
  if (caller_name && is_in_home_dir(path, caller_name)) return 1;
  if (flags & 1) return 1;
  
  return 0;
}

valid_write(path, func, caller, owner, flags) {
  int caller_flags, caller_id;
  string caller_name;
  
  /* System access - always allow */
  if (!caller) return 1;
  
  /* Privileged objects - always allow */
  if (priv(caller)) return 1;
  
  /* Get caller info once */
  caller_flags = caller.get_flags();
  caller_name = caller.get_name();
  caller_id = otoi(caller);
  
  /* Wizard: Full write access except boot.c */
  if (has_role(caller, "Wizard")) {
    if (path == "/boot.c") return 0;
    return 1;
  }
  
  /* Programmer (flag 32): Write all except /sys and boot.c */
  if (caller_flags & 32) {
    if (path == "/boot.c") return 0;
    if (path == "/sys" || path_starts_with(path, "/sys/")) return 0;
    /* Allow creating new files, owner files, or WRITE_OK files */
    if (flags == -1) return 1;
    if (owner == caller_id) return 1;
    if (flags & 2) return 1;
    return 0;
  }
  
  /* Builder (flag 8): Write only in /etc */
  if (caller_flags & 8) {
    if (path == "/boot.c") return 0;
    if (path == "/sys" || path_starts_with(path, "/sys/")) return 0;
    if (!path_starts_with(path, "/etc/")) return 0;
    /* Allow creating new files, owner files, or WRITE_OK files */
    if (flags == -1) return 1;
    if (owner == caller_id) return 1;
    if (flags & 2) return 1;
    return 0;
  }
  
  /* Regular user: Write only in home directory */
  if (caller_name && is_in_home_dir(path, caller_name)) {
    if (flags == -1) return 1;
    if (owner == caller_id) return 1;
    if (flags & 2) return 1;
    return 0;
  }
  
  return 0;
}

listen(arg) { send_device(arg); }

static save_db() {
  object curr;

  while (curr=next_who(curr))
    curr.listen("\n*** Autosaving Database ***\n");
  flush_device();
  alarm(SAVE_DELAY,"save_db");
  sysctl(0);
}

sync() {
  int seconds_left;

  if (!priv(caller_object())) return 0;
  seconds_left=remove_alarm("save_db");
  if (seconds_left<0) seconds_left=0;
  if (seconds_left<SAVE_DELAY) alarm(seconds_left,"save_db");
  else alarm(SAVE_DELAY,"save_db");
  return 1;
}

static init() {
  object me;

  me=this_object();
  if (prototype(this_object())) {
    alarm(SAVE_DELAY,"save_db");
    chmod("/boot.c",1);
    unhide("/etc",me,5);
    unhide("/etc/pkg",me,5);
    add_pkg(STARTING_PACKAGE);
    alarm(0,"finish_init");
  } else {
    destruct(this_object());
  }
}

static finish_init() {
  object wizobj;
  object room;

  set_priv(atoo("/sys/sys"),1);
  set_priv(atoo("/sys/ban"),1);
  atoo("/sys/ban").reload_sites();
  room=atoo("/obj/room");
  room.set_flags(19);
  wizobj=new("/obj/player");
  wizobj.set_secure();
  wizobj.set_name("Wizard");
  wizobj.set_owner(wizobj);
  wizobj.set_password("potrzebie");
  wizobj.set_flags(40);
  table_set("wizard",itoa(otoi(wizobj)));
  room.set_owner(wizobj);
  wizobj.set_link(room);
  move_object(wizobj,room);
  room.set_name("Limbo");
  room.set_desc("You are lost in a dense, gray fog.");
  set_priv(wizobj,1);
  chown("/home/wizard",wizobj);
}

get_uid() {
  return "root";
}

static connect() {
  object login_obj;

  login_obj=new("/sys/login");
  if (!login_obj) {
    send_device(otoa(this_object())+": unable to clone /sys/login\n");
    disconnect_device();
    return;
  }
  /* for secure.h */
  login_obj.set_secure();
  if (reconnect_device(login_obj)) {
    send_device(otoa(this_object())+": unable to reconnect to "+
                otoa(login_obj)+"\n");
    disconnect_device();
    destruct(login_obj);
    return;
  }
  set_priv(login_obj,1);
  login_obj.connect();
}
