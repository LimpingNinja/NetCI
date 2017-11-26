/* config.h */

/* this header file contains certain system configuration information */

#ifndef USE_WINDOWS
#include "autoconf.h"      /* automatically configured information */
#endif /* !USE_WINDOWS */
#include "tune.h"          /* tuning parameters, to hone system performance */
#include "ci.h"            /* various system #define's, don't change */

/******
 ****** nothing below needs to be changed. they are reasonable defaults,
 ****** and all parameters below can be set on the command line.
 ****** edit if you like, however.
 ******/

#define DEFAULT_INTERFACE MULTI /* the default interface type, can be
                                   MULTI or SINGLE */

#define DEFAULT_PROTOCOL CI_PROTOCOL_TCP /* default network protocol:
                                            can be CI_PROTOCOL_TCP,
                                            CI_PROTOCOL_IPX, or
                                            CI_PROTOCOL_NETBIOS */

#define TCP_PORT 5000            /* default tcp/ip port number */

#define IPX_NET 0x0              /* default IPX network number */
#define IPX_NODE "000000000001"  /* default IPX node number - 12 hex digits */
#define IPX_SOCKET 0x1388        /* default IPX socket number */

#define NETBIOS_NODE "NETCI"     /* default NetBIOS node name */
#define NETBIOS_PORT 5           /* default NetBIOS port, from 0 to 255 */

/* directory with the ci programs' source code (not the server source) */
#define FS_PATH "ci200fs"

#define INCLUDE_PATH "/include" /* appended to CI_PATH and prepended to
                                   included files, by the CI compiler */

/* file where system messages are recorded - probably best not to put it
   in FS_PATH */
#define SYSLOG_NAME "syslog.txt"

/* the cache file - best not to put it in FS_PATH */
#define TRANSACT_LOG_NAME "transact.log"

/* the maximum size the transaction log can reach before it forces an
   auto-save */
#define TRANSACT_LOG_SIZE 640000

#define TMPDB_NAME "_ci_temp.db"

/* default loaded ci database */
#define DEFAULT_LOAD "std.db"
/* default saved ci database */
#define DEFAULT_SAVE "std.db"
/* default panic database */
#define DEFAULT_PANIC "panic.db"

/* app. name / default window title */
#define NETCI_NAME "NetCI"

/* standard location for initialization information */
#define INIFILE "netci.ini"

#include "stdinc.h"
