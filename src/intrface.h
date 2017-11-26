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

#ifdef CMPLNG_INTRFCE
struct connlist_s {
  SOCKET fd;
  union {
    struct sockaddr_in tcp_addr;
#ifdef USE_LINUX_IPX
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
void immediate_disconnect();
int init_interface(struct net_parms *port, int do_single);
void shutdown_interface();
void handle_input();
char *get_devconn(struct object *obj);
void send_device(struct object *obj, char *msg);
int reconnect_device(struct object *src, struct object *dest);
void disconnect_device(struct object *obj);
struct object *next_who(struct object *obj);
void flush_device(struct object *obj);
int connect_device(struct object *obj, char *address, int port, int net_type);
long get_devidle(struct object *obj);
long get_conntime(struct object *obj);
