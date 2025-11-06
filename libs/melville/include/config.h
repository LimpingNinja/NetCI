/* config.h - Melville/NetCI Configuration */

/* System Paths */
#define BOOT_OB         "/boot"
#define AUTO_OB         "/sys/auto"
#define USER_OB         "/sys/user"
#define PLAYER_OB       "/sys/player"
#define PLAYER_PATH     "/sys/player"
#define LOGIN_OB        "/sys/login"

/* Inheritable Paths */
#define OBJECT_PATH     "/inherits/object"
#define CONTAINER_PATH  "/inherits/container"
#define ROOM_PATH       "/inherits/room"
#define LIVING_PATH     "/inherits/living"

/* Daemon Paths */
#define CMD_D           "/sys/daemons/cmd_d"
#define USER_D          "/sys/daemons/user_d"
#define LOG_D           "/sys/daemons/log_d"

/* Directory Paths */
#define CMD_DIR         "/cmds"
#define PLAYER_CMD_DIR  "/cmds/player"
#define BUILDER_CMD_DIR "/cmds/builder"
#define WIZARD_CMD_DIR  "/cmds/wizard"
#define ADMIN_CMD_DIR   "/cmds/admin"
#define WORLD_DIR       "/world"
#define START_DIR       "/world/start"
#define WIZ_DIR         "/world/wiz"
#define SYS_DIR         "/sys"
#define DATA_DIR        "/sys/data"
#define PLAYER_SAVE_DIR "/sys/data/players/"

/* Starting Locations */
#define START_ROOM      "/world/start/void"
#define VOID_ROOM       "/world/start/void"

/* Admin Configuration */
#define ADMIN_NAMES     ({ "admin", "root" })

/* System Configuration */
#define SAVE_INTERVAL   3600    /* Autosave every hour */
#define MAX_PLAYERS     100     /* Maximum concurrent players */
#define MUD_NAME        "Melville/NetCI"
#define MUD_VERSION     "0.1.0"

/* Security Configuration */
#define PROTECT_BOOT    1       /* Protect boot.c from modification */
#define PROTECT_SYS     1       /* Protect /sys from non-admin access */
