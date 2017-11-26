/* ci.h */

/* this header file contains certain system configuration information.
   users shouldn't be changing this. */

#define CI_VERSION ((int) 10905)  /* driver version # */

#define MULTI 0
#define SINGLE 1

#define CI_PROTOCOL_CONSOLE 1
#define CI_PROTOCOL_TCP 2
#define CI_PROTOCOL_IPX 3
#define CI_PROTOCOL_NETBIOS 4

#define DB_IDENSTR "CI db format v2.0\n" /* string stuck at beginning of
                                           db file */

/* the flags on objects */

#define GARBAGE     1
#define INTERACTIVE 2
#define CONNECTED   4
#define PROTOTYPE   8
#define IN_EDITOR   16
#define PRIV        32
#define RESIDENT    64
#define LOCALVERBS  128

/* the flags on files */

#define READ_OK     1
#define WRITE_OK    2
#define DIRECTORY   4

#define MALLOC(SIZE_) malloc(SIZE_)
#define FREE(POINTER_) free(POINTER_)

#ifdef DEBUG
#define FATAL_ERROR() abort()
#else /* DEBUG */
#define FATAL_ERROR() exit(1)
#endif /* DEBUG */

#define digit_value(X) ((X)-'0')
#define int2time(X) ((time_t) ((X)+time_offset))
#define time2int(X) (((long) (X))-time_offset)

#ifdef USE_WINDOWS
#define FREAD_MODE "rb"
#define FWRITE_MODE "wb"
#define FAPPEND_MODE "ab"
#else /* USE_WINDOWS */
#define FREAD_MODE "r"
#define FWRITE_MODE "w"
#define FAPPEND_MODE "a"
#endif /* USE_WINDOWS */

/* this ridiculous #definage is because SunOS4.1.3_U1 doesn't have strtoul
   in it's C library; since that's only used by the IPX address reading
   stuff, if we're not on a system that uses IPX, we can just sub in strtol
   instead */
#ifdef USE_LINUX_IPX
#define STRTOUL strtoul
#else /* USE_LINUX_IPX */
#ifdef USE_WINDOWS
#define STRTOUL strtoul
#else /* USE_WINDOWS */
#define STRTOUL strtol
#endif /* USE_WINDOWS */
#endif /* USE_LINUX_IPX */

/* for cool systems with sizable fd_set's */
#ifdef CMPLNG_INTRFCE
#ifdef USE_POSIX
#define FD_SETSIZE (MAX_CONNS+1)
#endif /* USE_POSIX */
#ifdef USE_WINDOWS
#define FD_SETSIZE (MAX_CONNS+1)
#endif /* USE_WINDOWS */
#endif /* CMPLNG_INTRFCE */
