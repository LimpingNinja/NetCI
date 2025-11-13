
/* interface.h */

#ifdef USE_WINDOWS
#include <winsock.h>
#include <wsipx.h>
#include <wsnetbs.h>
#else /* USE_WINDOWS */
typedef int SOCKET;
#endif /* USE_WINDOWS */

#define NOSINGLE 1
#define NOMULTI 2
#define PORTINUSE 3
#define NOSOCKET 4
#define NOPROT 5

/* Telnet protocol constants */
#define TELNET_IAC   255  /* Interpret As Command */
#define TELNET_DONT  254  /* Don't use option */
#define TELNET_DO    253  /* Use option */
#define TELNET_WONT  252  /* Won't use option */
#define TELNET_WILL  251  /* Will use option */
#define TELNET_SB    250  /* Subnegotiation begin */
#define TELNET_GA    249  /* Go ahead */
#define TELNET_SE    240  /* Subnegotiation end */

/* Telnet options */
#define TELOPT_ECHO       1   /* Echo */
#define TELOPT_SGA        3   /* Suppress Go Ahead */
#define TELOPT_TTYPE     24   /* Terminal Type */
#define TELOPT_NAWS      31   /* Negotiate About Window Size */
#define TELOPT_MSSP      70   /* MUD Server Status Protocol */
#define TELOPT_MCCP2     86   /* MUD Client Compression Protocol v2 */
#define TELOPT_GMCP     201   /* Generic MUD Communication Protocol */

/* Telnet parser states */
#define TELNET_STATE_DATA 0   /* Normal data */
#define TELNET_STATE_IAC  1   /* Received IAC */
#define TELNET_STATE_WILL 2   /* Received IAC WILL */
#define TELNET_STATE_WONT 3   /* Received IAC WONT */
#define TELNET_STATE_DO   4   /* Received IAC DO */
#define TELNET_STATE_DONT 5   /* Received IAC DONT */
#define TELNET_STATE_SB   6   /* In subnegotiation */
#define TELNET_STATE_SB_IAC 7 /* IAC in subnegotiation */

#ifdef CMPLNG_INTRFCE
struct connlist_s {
  SOCKET fd;
  union {
    struct sockaddr_in tcp_addr;
#ifdef USE_LINUX_IPX
# ifndef IPX_TYPE
#  define IPX_TYPE 1
# endif
    struct sockaddr_ipx ipx_addr;
#endif /* USE_LINUX_IPX */
#ifdef USE_WINDOWS
    struct sockaddr_ipx ipx_addr;
    struct sockaddr_nb netbios_addr;
#endif /* USE_WINDOWS */
  } address;
  int net_type;
  char inbuf[MAX_STR_LEN];
  int inbuf_count;
  int outbuf_count;
  char *outbuf;
  struct object *obj;
  long conn_time;
  long last_input_time;
  
  /* Telnet protocol state */
  int telnet_state;         /* Current parser state */
  unsigned char telnet_opt; /* Current option being negotiated */
  unsigned char sb_opt;     /* Subnegotiation option */
  unsigned char sb_buf[256]; /* Subnegotiation buffer */
  int sb_len;               /* Subnegotiation buffer length */
  
  /* Telnet option flags */
  unsigned char opt_echo:1;    /* ECHO negotiated */
  unsigned char opt_sga:1;     /* SGA negotiated */
  unsigned char opt_mssp:1;    /* MSSP sent */
  unsigned char opt_naws:1;    /* NAWS negotiated */
  unsigned char opt_ttype:1;   /* TTYPE negotiated */
  unsigned short win_width;    /* Window width from NAWS */
  unsigned short win_height;   /* Window height from NAWS */
  char term_type[64];          /* Terminal type from TTYPE (e.g. "xterm-256color") */
};
#endif /* CMPLNG_INTRFCE */

struct net_parms {
  int protocol;
  int tcp_port;
  unsigned long ipx_net;
  unsigned char ipx_node[6];
  int ipx_socket;
  char netbios_node[15];
  int netbios_port;
};

int get_devport(struct object *obj);
int get_devnet(struct object *obj);
char *host_to_addr(char *host, int net_type);
char *addr_to_host(char *addr, int net_type);
int convert_to_6byte(char *s, unsigned char *addr);
char *convert_from_6byte(unsigned char *addr);
int detach_session();
void immediate_disconnect(int devnum);
int init_interface(struct net_parms *port, int do_single);
void shutdown_interface();
void handle_input();
char *get_devconn(struct object *obj);
void send_device(struct object *obj, char *msg);
void send_prompt(struct object *obj, char *prompt);
int reconnect_device(struct object *src, struct object *dest);
void disconnect_device(struct object *obj);
struct object *next_who(struct object *obj);
void flush_device(struct object *obj);
int connect_device(struct object *obj, char *address, int port, int net_type);
long get_devidle(struct object *obj);
long get_conntime(struct object *obj);
