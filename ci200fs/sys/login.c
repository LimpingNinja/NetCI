/* login.c */

#include <sys.h>
#include <secure.h>
#include <hear.h>
#include <ban.h>

#define REG_FILE "/etc/register"
#define MOTD_FILE "/etc/motd"
#define GUEST_FILE "/etc/guest"
#define NOGUEST_FILE "/etc/noguest"
#define SITEREG_FILE "/etc/sitereg"
#define SITEBAN_FILE "/etc/siteban"

static init() {
  check_secure(SECURE_BOOTOBJ,NULL);
  set_interactive(TRUE);
  if (!prototype(this_object())) return;
  add_xverb("","do_huh");
  add_verb("connect","do_connect");
  add_verb("conn","do_connect");
  add_verb("c","do_connect");
  add_verb("create","do_create");
  add_verb("help","do_help");
  add_verb("quit","do_quit");
  add_verb("QUIT","do_quit");
  add_verb("WHO","do_who");
}

disconnect() {
  destruct(this_object());
}

static do_who() {
  who_list();
  return 1;
}

connect() {
  if (connected(this_object()))
    alarm(0,"welcome");
}

listen(arg) {
  send_device(arg);
}

static welcome() {
  cat("/etc/login");
}

static do_huh(arg) {
  if (arg)
    listen(HUH_STRING);
  return 1;
}

static do_help(arg) {
  listen("\nLogin Help:\n");
  listen("-----------\n");
  listen("create <name> <password>\n");
  listen("connect <name> <password>\n");
  listen("quit\n");
  listen("\n");
  return 1;
}

static do_create(arg) {
  string name,password,valid_site;
  int pos;
  object new_player;

  valid_site=site_status(get_devconn(this_object()));
  if (valid_site=="f") {
    cat(SITEBAN_FILE);
    disconnect_device();
    distruct(this_object());
    return 1;

  }
  if (sys_get_registration()) {
    if (cat(REG_FILE))
      listen("This MUD is registration only. Sorry.\n");
    return 1;
  }
  if (valid_site=="r") {
    cat(SITEREG_FILE);
    disconnect_device();
    distruct(this_object());
    return 1;
  }
  pos=instr(arg,1," ");
  name=leftstr(arg,pos-1);
  password=rightstr(arg,strlen(arg)-pos);
  if ((!name) || (!password)) return 0;
  if (!is_legal_name(name)) {
    listen("Illegal name.\n");
    return 1;
  }
  if (find_player(name)) {
    listen("That player already exists.\n");
    return 1;
  }
  new_player=new("/obj/player");
  new_player.set_secure();
  new_player.set_name(name);
  new_player.set_flags(sys_get_newchar_flags());
  new_player.set_password(password);
  new_player.set_link(atoo("/obj/room"));
  new_player.set_owner(new_player);
  move_object(new_player,atoo("/obj/room"));
  add_new_player(name,new_player);
  tell_room_except(atoo("/obj/room"),new_player,NULL,HEAR_ENTER,NULL);
  cat(MOTD_FILE);
  if (reconnect_device(new_player))
    send_device("Reconnect to "+otoa(new_player)+" failed.\n");
  else
    new_player.connect();
  self_destruct();
  return 1;
}

static self_destruct() {
  alarm(1,"do_self_destruct");
}

static do_self_destruct() {
  destruct(this_object());
}

static do_connect(arg) {
  string name,password,valid_site;
  int pos;
  object player;

  valid_site=site_status(get_devconn(this_object()));
  if (valid_site=="f") {
    cat(SITEBAN_FILE);
    disconnect_device();
    distruct(this_object());
    return 1;
  }
  if (downcase(arg)=="guest") {
    create_guest();
    return 1;
  }
  pos=instr(arg,1," ");
  name=leftstr(arg,pos-1);
  password=rightstr(arg,strlen(arg)-pos);
  if (!name) return 0;
  if (!(player=find_player(name))) {
    listen("That player does not exist.\n");
    return 1;
  }
  if (!player.check_password(password)) {
    listen("Wrong password.\n");
    return 1;
  }
  if (player.is_guest()) {
    listen("To connect to a guest character, use 'connect guest'\n");
    return 1;
  }
  if (connected(player)) player.force_disconnect();
  cat(MOTD_FILE);
  if (reconnect_device(player))
    send_device("Reconnect to "+otoa(player)+" failed.\n");
  else
    player.connect();
  self_destruct();
  return 1;
}

static create_guest() {
  int loop;
  object new_player;

  if (sys_guests_disabled()) {
    cat(NOGUEST_FILE);
    return;
  }
  loop=1;
  while (table_get("guest"+itoa(loop))) loop++;
  new_player=new("/obj/player");
  new_player.set_secure();
  new_player.set_name("Guest"+itoa(loop));
  new_player.set_password("guest");
  new_player.set_link(atoo("/obj/room"));
  new_player.set_owner(new_player);
  new_player.set_guest();
  move_object(new_player,atoo("/obj/room"));
  add_new_player("Guest"+itoa(loop),new_player);
  tell_room_except(atoo("/obj/room"),new_player,NULL,HEAR_ENTER,NULL);
  cat(MOTD_FILE);
  cat(GUEST_FILE);
  reconnect_device(new_player);
  new_player.connect();
  self_destruct();
}

static do_quit(arg) {
  disconnect_device();
  destruct(this_object());
  return 1;
}
