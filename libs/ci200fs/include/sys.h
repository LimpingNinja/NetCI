/* sys.h */

#define STRING_T 1
#define OBJECT_T 2
#define INT_T 0

#define SYSCTL_SAVE 0
#define SYSCTL_SHUTDOWN 1
#define SYSCTL_PANIC 2
#define SYSCTL_LIST_ALARMS 3
#define SYSCTL_LIST_CMDS 4
#define SYSCTL_LIST_DEST 5
#define SYSCTL_NOHARDLMT 6
#define SYSCTL_NOSOFTLMT 7
#define SYSCTL_VERSION 8

#define READ_OK 1
#define WRITE_OK 2
#define DIRECTORY 4

#define NET_CONSOLE 1
#define NET_TCP 2
#define NET_IPX 3
#define NET_NETBIOS 4

#define NULL 0

#define TRUE 1
#define FALSE 0

#define HUH_STRING "Huh? (type \"help\" for help)\n"
#define NOTHING_SPECIAL_STRING "You see nothing special.\n"

int SysObjNum;

#define SYSOBJ (SysObjNum?itoo(SysObjNum):                            \
                          itoo(SysObjNum=otoi(atoo("/sys/sys"))))

#define sys_set_combat_enabled(X) call_other(SYSOBJ, \
                                             "sys_set_combat_enabled",(X))
#define sys_combat_enabled(X) call_other(SYSOBJ,"sys_combat_enabled")
#define sys_set_find_disabled(X) call_other(SYSOBJ,"sys_set_find_disabled",(X))
#define sys_find_disabled() call_other(SYSOBJ,"sys_find_disabled")
#define is_legal_name(X) call_other(SYSOBJ,"is_legal_name",(X))
#define broadcast(X) call_other(SYSOBJ,"broadcast",(X))
#define sys_get_newchar_flags() call_other(SYSOBJ,"sys_get_newchar_flags")
#define sys_set_newchar_flags(X) call_other(SYSOBJ,"sys_set_newchar_flags",(X))
#define sys_get_registration() call_other(SYSOBJ,"sys_get_registration")
#define sys_set_registration(X) call_other(SYSOBJ,"sys_set_registration",(X))
#define sys_get_doingquery() call_other(SYSOBJ,"sys_get_doingquery")
#define sys_set_doingquery(X) call_other(SYSOBJ,"sys_set_doingquery",(X))
#define sys_get_whoquote() call_other(SYSOBJ,"sys_get_whoquote")
#define sys_set_whoquote(X) call_other(SYSOBJ,"sys_set_whoquote",(X))

#define make_flags(X) call_other(SYSOBJ,"make_flags",(X))
#define make_sys_flags(X) call_other(SYSOBJ,"make_sys_flags",(X))
#define make_num(X) call_other(SYSOBJ,"make_num",(X))
#define make_type(X) call_other(SYSOBJ,"make_type",(X))
#define present(X,Y) call_other(SYSOBJ,"present",(X),(Y))
#define write(X,Y) ((X).listen(Y))
#define who_list() call_other(SYSOBJ,"who_list")
#define wiz_who_list() call_other(SYSOBJ,"wiz_who_list")
#define add_new_player(X,Y) call_other(SYSOBJ,"add_new_player",(X),(Y))
#define remove_player(X) call_other(SYSOBJ,"remove_player",(X))
#define find_player(X) call_other(SYSOBJ,"find_player",(X))
#define remove_guest() call_other(SYSOBJ,"remove_guest")
#define sys_guests_disabled() call_other(SYSOBJ,"sys_guests_disabled")
#define sys_set_guests_disabled(X) call_other(SYSOBJ,      \
                                              "sys_set_guests_disabled",(X))
#define sys_names_disabled(X) call_other(SYSOBJ,"sys_names_disabled")
#define sys_set_names_disabled(X) call_other(SYSOBJ,       \
					     "sys_set_names_disabled",(X))
#define rename_player(X,Y) call_other(SYSOBJ,"rename_player",(X),(Y))


