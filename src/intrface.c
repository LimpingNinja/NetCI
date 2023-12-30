/* interface.c */

#define CMPLNG_INTRFCE

#include "config.h"

#ifndef USE_WINDOWS
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <signal.h>
#include <netdb.h>
#endif /* !USE_WINDOWS */

#ifdef USE_POSIX
#include <unistd.h>
#include <sys/resource.h>
#endif /* USE_POSIX */

#ifdef USE_LINUX
#include <unistd.h>
#ifdef USE_LINUX_IPX
#include <linux/ipx.h>
#endif /* USE_LINUX_IPX */
#endif /* USE_LINUX */

#include "object.h"
#include "globals.h"
#include "interp.h"
#include "dbhandle.h"
#include "constrct.h"
#include "intrface.h"
#include "clearq.h"
#include "file.h"
#include "cache.h"
#include "edit.h"

#ifdef USE_WINDOWS
#include "winmain.h"
#endif /* USE_WINDOWS */

struct connlist_s *connlist;
int num_conns,num_fds,net_protocol;
SOCKET sockfd;

#ifdef USE_POSIX
#ifdef RLIMIT_NOFILE
void set_num_fds() {
  struct rlimit max_fd;

  if (getrlimit(RLIMIT_NOFILE,&max_fd)==(-1)) {
    num_fds=sysconf(_SC_OPEN_MAX);
    return;
  }
  if (max_fd.rlim_cur>=(MIN_FREE_FILES+MAX_CONNS+7)) {
    num_fds=max_fd.rlim_cur;
    return;
  }
  max_fd.rlim_cur=MIN_FREE_FILES+MAX_CONNS+7;
  if ((max_fd.rlim_cur>=max_fd.rlim_max) && (max_fd.rlim_max!=RLIM_INFINITY))
    max_fd.rlim_cur=max_fd.rlim_max;
  setrlimit(RLIMIT_NOFILE,&max_fd);
  getrlimit(RLIMIT_NOFILE,&max_fd);
  num_fds=max_fd.rlim_cur;
}
#else /* RLIMIT_NOFILE */
void set_num_fds() {
  num_fds=sysconf(_SC_OPEN_MAX);
}
#endif /* RLIMIT_NOFILE */
#endif /* USE_POSIX */

#ifdef USE_BSD
void set_num_fds() {
  num_fds=getdtablesize();
}
#endif /* USE_BSD */

#ifdef USE_LINUX
void set_num_fds() {
  num_fds=getdtablesize();
}
#endif /* USE_LINUX */

#ifdef USE_WINDOWS
void set_num_fds() {
  num_fds=MIN_FREE_FILES+MAX_CONNS+7;
}
#endif /* USE_WINDOWS */

#ifdef USE_WINDOWS
int detach_session() {
  return 0;
}
#else
int detach_session() {
  int retval;

#ifdef SIGCLD
  signal(SIGCLD,SIG_IGN);
#endif /* SIGCLD */
#ifdef SIGCHLD
  signal(SIGCHLD,SIG_IGN);
#endif /* SIGCHLD */
  retval=fork();
  if (retval==(-1)) return (-1);
  if (retval) return 1;
#ifdef SIGHUP
  signal(SIGHUP,SIG_IGN);
#endif /* SIGHUP */
  fclose(stdin);
  fclose(stdout);
  fclose(stderr);
#ifdef USE_POSIX
  setpgid(0,0);
#endif /* USE_POSIX */
#ifdef USE_BSD
  setpgrp(0,0);
#endif /* USE_BSD */
#ifdef USE_LINUX
  setpgid(0,0);
  setsid();
#endif /* USE_LINUX */
  return 0;
}
#endif /* USE_WINDOWS */

/**
 * This function is designed to convert a hexadecimal string representation
 * of a MAC address into its corresponding 6-byte (48-bit) array format.
 *
 * @param s Pointer to the input string representing the MAC address in hexadecimal form.
 * @param addr Pointer to an array where the converted 6-byte MAC address will be stored.
 * @return int Returns 0 on successful conversion, or -1 if the input string is not a valid 
 *             12-character hexadecimal string.
 */
int convert_to_6byte(char *s, unsigned char *addr) {
  static unsigned char a[6];
  int len,c,count;

  len=strlen(s);
  if (len!=12) return (-1);
  count=0;
  while (count<6) {
    a[count]=0;
    if (!isxdigit(s[count*2])) return (-1);
    if (isdigit(s[count*2])) c=s[count*2]-'0';
    else if (isupper(s[count*2])) c=s[count*2]-'A'+10;
    else c=s[count*2]-'a'+10;
    a[count]=c<<4;
    if (!isxdigit(s[(count*2)+1])) return (-1);
    if (isdigit(s[(count*2)+1])) c=s[(count*2)+1]-'0';
    else if (isupper(s[(count*2)+1])) c=s[(count*2)+1]-'A'+10;
    else c=s[(count*2)+1]-'a'+10;
    a[count]|=c;
    count++;
  }
  memcpy(addr,a,6);
  return 0;
}

/**
 * Retrieves the address family based on the specified network type.
 * 
 * Supports TCP (IPv4), IPX (Linux/Windows), and NetBIOS (Windows only).
 * Returns the corresponding address family constant for each protocol.
 * 
 * @param net_type Network type (TCP, IPX, NetBIOS).
 * @return Address family constant or -1 if the network type is unsupported.
 */
int get_af(int net_type) {
  if (net_type==CI_PROTOCOL_TCP) return AF_INET;
#ifdef USE_LINUX_IPX
  if (net_type==CI_PROTOCOL_IPX) return AF_IPX;
#endif /* USE_LINUX_IPX */
#ifdef USE_WINDOWS
  if (net_type==CI_PROTOCOL_IPX) return AF_IPX;
  if (net_type==CI_PROTOCOL_NETBIOS) return AF_NETBIOS;
#endif /* USE_WINDOWS */
  return (-1);
}

/**
 * Converts an address and port to a sockaddr structure based on network type.
 * 
 * Supports TCP, IPX (Linux/Windows), and NetBIOS (Windows) protocols.
 * For TCP, converts IPv4 address and port.
 * For IPX, processes address string and port for IPX networks.
 * For NetBIOS (Windows only), sets up NetBIOS name and port.
 * 
 * @param address IP or IPX address as a string.
 * @param port Network port number.
 * @param host Pointer to the resulting sockaddr structure.
 * @param net_type Network type (TCP, IPX, NetBIOS).
 * @return Size of the sockaddr structure or -1 on error.
 */
int convert_to_sockaddr(char *address, int port, struct sockaddr **host,
                        int net_type) {
  static struct sockaddr_in tcp_addr;
#ifdef USE_LINUX_IPX
  static struct sockaddr_ipx ipx_addr;
#endif /* USE_LINUX_IPX */
#ifdef USE_WINDOWS
  static struct sockaddr_ipx ipx_addr;
  static struct sockaddr_nb netbios_addr;
#endif /* USE_WINDOWS */

  if (net_type==CI_PROTOCOL_TCP) {
    unsigned long octets;

    octets=inet_addr(address);
    if (((signed long) octets)==(-1)) return (-1);
    tcp_addr.sin_family=AF_INET;
    tcp_addr.sin_port=htons(port);
    tcp_addr.sin_addr.s_addr=octets;
    *host=(struct sockaddr *) &tcp_addr;
    return sizeof(struct sockaddr_in);
  }
#ifdef USE_LINUX_IPX
  if (net_type==CI_PROTOCOL_IPX) {
    char *dupaddr;
    int count,len;

    len=strlen(address);
    dupaddr=MALLOC(len+1);
    strcpy(dupaddr,address);
    count=0;
    while (dupaddr[count] && dupaddr[count]!='.') {
      if (!isxdigit(dupaddr[count])) return (-1);
      count++;
    }
    if ((len-count-1)!=12) {
      FREE(dupaddr);
      return (-1);
    }
    dupaddr[count]='\0';
    ipx_addr.sipx_family=AF_IPX;
    if (convert_to_6byte(dupaddr+count+1,
                         &(ipx_addr.sipx_node[0]))) {
      FREE(dupaddr);
      return (-1);
    }
    ipx_addr.sipx_network=htonl(STRTOUL(dupaddr,NULL,16));
    ipx_addr.sipx_port=htons(port);
    ipx_addr.sipx_type=IPX_TYPE;
    *host=(struct sockaddr *) &ipx_addr;
    FREE(dupaddr);
    return sizeof(struct sockaddr_ipx);
  }
#endif /* USE_LINUX_IPX */
#ifdef USE_WINDOWS
  if (net_type==CI_PROTOCOL_IPX) {
    char *dupaddr;
    int count,len;
	unsigned long ipx_net;

    len=strlen(address);
    dupaddr=MALLOC(len+1);
    strcpy(dupaddr,address);
    count=0;
    while (dupaddr[count] && dupaddr[count]!='.') {
      if (!isxdigit(dupaddr[count])) return (-1);
      count++;
    }
    if ((len-count-1)!=12) {
      FREE(dupaddr);
      return (-1);
    }
    dupaddr[count]='\0';
    ipx_addr.sa_family=AF_IPX;
    if (convert_to_6byte(dupaddr+count+1,
                         &(ipx_addr.sa_nodenum[0]))) {
      FREE(dupaddr);
      return (-1);
    }
	ipx_net=STRTOUL(dupaddr,NULL,16);
	ipx_addr.sa_netnum[0]=(unsigned char) (ipx_net & 0xFF);
	ipx_addr.sa_netnum[1]=(unsigned char) ((ipx_net >> 8) & 0xFF);
    ipx_addr.sa_netnum[2]=(unsigned char) ((ipx_net >> 16) & 0xFF);
	ipx_addr.sa_netnum[3]=(unsigned char) ((ipx_net >> 24) & 0xFF);
    ipx_addr.sa_socket=htons(port);
    *host=(struct sockaddr *) &ipx_addr;
    FREE(dupaddr);
    return sizeof(struct sockaddr_ipx);
  }
  if (net_type==CI_PROTOCOL_NETBIOS) {
    int loop;
    
    netbios_addr.snb_family=AF_NETBIOS;
	netbios_addr.snb_type=NETBIOS_UNIQUE_NAME | NETBIOS_REGISTERING;
    loop=0;
	while (loop<15 && address[loop]) {
	  netbios_addr.snb_name[loop]=address[loop];
	  loop++;
	}
	while (loop<15) address[loop++]=' ';
	netbios_addr.snb_name[15]=(unsigned char) port;
	return sizeof(struct sockaddr_nb);
  }
#endif /* USE_WINDOWS */
  return (-1);
}

/**
 * Converts a 6-byte MAC address array to a 12-character hexadecimal string.
 * The resulting string represents the MAC address in a readable format.
 * 
 * @param addr Input 6-byte MAC address array.
 * @return Pointer to a static buffer containing the 12-character hexadecimal string.
 */
char *convert_from_6byte(unsigned char *addr) {
  static char buf[13];
  int loop,c;

  loop=0;
  while (loop<6) {
    c=addr[loop] & 15;
    if (c<10) c+='0';
    else c+='A'-10;
    buf[loop*2]=c;
    c=(addr[loop]>>4) & 15;
    if (c<10) c+='0';
    else c+='A'-10;
    buf[(loop*2)+1]=c;
    loop++;
  }
  buf[12]='\0';
  return buf;
}

/**
 * Writes data to a socket with a non-blocking approach.
 * 
 * Attempts to write 'count' bytes from 'buf' to the socket 'fd'. Uses select()
 * for non-blocking I/O, checking for write-ability. Handles partial writes and
 * socket exceptions. This is mainly used to close out sockets outside of the
 * normal operations and only called by flush_device->flush_a_device and by the
 * immediate_disconnect function.
 * 
 * @param fd Socket file descriptor to write to.
 * @param count Number of bytes to write.
 * @param buf Buffer containing data to write.
 * @return Number of bytes successfully written, or partial count on error or block.
 */
int saniflush(SOCKET fd,int count,char *buf) {
  struct timeval timeout;
  fd_set writefds,exceptfds;
  int num_written,num_to_write,num_actually_wrote;

  num_written=0;
  while (num_written<count) {
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);
    FD_SET(fd,&writefds);
    FD_SET(fd,&exceptfds);
    timeout.tv_sec=0;
    timeout.tv_usec=0;
#ifdef USE_WINDOWS
    NT_select(num_fds,NULL,&writefds,&exceptfds,&timeout);
#else /* USE_WINDOWS */
    select(num_fds,NULL,&writefds,&exceptfds,&timeout);
#endif /* USE_WINDOWS */
    if (FD_ISSET(fd,&exceptfds)) return num_written;
    if (!(FD_ISSET(fd,&writefds))) return num_written;
    if ((count-num_written)<WRITE_BURST)
      num_to_write=count-num_written;
    else
      num_to_write=WRITE_BURST;
#ifdef USE_WINDOWS
    num_actually_wrote=send(fd,buf+num_written,num_to_write,0);
#else /* USE_WINDOWS */
    num_actually_wrote=write(fd,buf+num_written,num_to_write);
#endif /* USE_WINDOWS */
    if (num_actually_wrote<=0) return num_written;
    num_written+=num_actually_wrote;
  }
  return count;
}

/**
 * Resolves the given hostname to its corresponding IPv4 address using
 * gethostbyname(). Only supports TCP protocol. If the hostname cannot
 * be resolved or is not IPv4, returns NULL.
 * 
 * @param host Hostname to resolve.
 * @param net_type Network type (defaults to global net_protocol if 0).
 * @return Pointer to a static buffer containing the IPv4 address string, or NULL on failure.
 */
char *host_to_addr(char *host, int net_type) {
  struct hostent *h;

  if (!net_type) net_type=net_protocol;
  if (net_type!=CI_PROTOCOL_TCP) return NULL;
  h=gethostbyname(host);
  if (!h) return NULL;
  if (h->h_addrtype!=AF_INET) return NULL;
  if (!(h->h_addr_list[0])) return NULL;
  return inet_ntoa(*((struct in_addr *) h->h_addr_list[0]));
}

/**
 * Resolves the given IPv4 address to its hostname using gethostbyaddr().
 * Only supports TCP protocol. If the address cannot be resolved or is invalid,
 * returns NULL.
 * 
 * @param addr IPv4 address string to resolve.
 * @param net_type Network type (defaults to global net_protocol if 0).
 * @return Hostname corresponding to the IPv4 address, or NULL on failure.
 */
char *addr_to_host(char *addr, int net_type) {
  struct hostent *h;
  struct in_addr tcp_addr;

  if (!net_type) net_type=net_protocol;
  if (net_type!=CI_PROTOCOL_TCP) return NULL;
  if (((long) (tcp_addr.s_addr=inet_addr(addr)))==(-1)) return NULL;
  h=gethostbyaddr((char *) &tcp_addr,sizeof(tcp_addr),AF_INET);
  if (!h) return NULL;
  if (*(h->h_name))
    return (char *) h->h_name;
  else
    return NULL;
}

/**
 * Iterates through the connection list to find the next active object, starting
 * from a specified object. This function cycles through connected entities in
 * network applications. It returns the next object in the connection sequence
 * or NULL if no more objects are found.
 *
 * @param obj The reference object for starting the iteration, or NULL to start
 *            from the beginning.
 * @return The next connected object or NULL if there are no more.
 */
struct object *next_who(struct object *obj) {
  long loop;

  if (obj)
    loop=obj->devnum;
  else
    loop=(-1);
  while (++loop<num_conns)
    if (connlist[loop].fd!=(-1))
      return connlist[loop].obj;
  return NULL;
}

/**
 * Sends buffered output to a connected device. The function attempts to write
 * a specified amount of data from the output buffer to the device. It handles
 * partial writes by adjusting the buffer accordingly, retainin unwritten data
 *
 * @param devnum The device number identifying the target device in the
 *               connection list.
 */
void unbuf_output(int devnum) {
  int num_written;
  char *tmp;

#ifdef USE_WINDOWS
  num_written=send(connlist[devnum].fd,connlist[devnum].outbuf,
                   ((connlist[devnum].outbuf_count>WRITE_BURST) ?
				    WRITE_BURST:connlist[devnum].outbuf_count),0);
#else /* USE_WINDOWS */
  num_written=write(connlist[devnum].fd,connlist[devnum].outbuf,
                    ((connlist[devnum].outbuf_count>WRITE_BURST) ?
                     WRITE_BURST:connlist[devnum].outbuf_count));
#endif /* USE_WINDOWS */
  if (num_written<=0) return;
  if (num_written<connlist[devnum].outbuf_count) {
    tmp=MALLOC(connlist[devnum].outbuf_count-num_written+1);
    strcpy(tmp,&((connlist[devnum].outbuf)[num_written]));
    connlist[devnum].outbuf_count-=num_written;
    FREE(connlist[devnum].outbuf);
    connlist[devnum].outbuf=tmp;
  } else {
    connlist[devnum].outbuf_count=0;
    FREE(connlist[devnum].outbuf);
    connlist[devnum].outbuf=NULL;
  }
}

void set_now_time() {
  now_time=time2int(time(NULL));
}

/**
 * Terminates the connection for the specified device number. It covers flushing
 * the output buffer, closing the socket, and updating connection status. The
 * function also exits any active editing sessions for the device. This uses the
 * saniflush() function to finish/flush. This is mainly used throughout the net
 * interface for closing out faulty or expired connections, but also used in the
 * core when an object connected to a device is destroyed. disconnect_device() 
 * calls this and is the primary disconnect method exposed to NLPC.
 *
 * @param devnum Device number in the connection list. If -1, exits without action.
 */
void immediate_disconnect(int devnum) {
  if (devnum==(-1)) return;
  if (connlist[devnum].obj->flags & IN_EDITOR)
    remove_from_edit(connlist[devnum].obj);
  connlist[devnum].obj->flags&=~CONNECTED;
  if (connlist[devnum].outbuf_count>0)
    saniflush(connlist[devnum].fd,connlist[devnum].outbuf_count,
              connlist[devnum].outbuf);
#ifdef USE_WINDOWS
  SelectWSARemove(connlist[devnum].fd);
  closesocket(connlist[devnum].fd);
#else /* USE_WINDOWS */
  shutdown(connlist[devnum].fd,2);
  close(connlist[devnum].fd);

#endif /* USE_WINDOWS */
  connlist[devnum].fd=(-1);
  connlist[devnum].obj->devnum=(-1);
  connlist[devnum].outbuf_count=0;
  if (connlist[devnum].outbuf) FREE(connlist[devnum].outbuf);
  connlist[devnum].outbuf=NULL;
}

/**
 * Establishes a new connection based on the given socket and network type.
 * Accepts an incoming connection, assigns a device number, and initializes
 * various connection parameters. It handles different protocols like TCP, IPX,
 * and NETBIOS. The function also calls 'connect' for newly connected objects
 * and performs necessary cleanup for unsuccessful connections.
 *
 * @param sockfd Socket file descriptor.
 * @param net_type Network protocol type.
 */
void make_new_conn(SOCKET sockfd,int net_type) {
  int loop;
  int devnum;
  SOCKET new_fd;
  struct sockaddr_in tcp_addr;
#ifdef USE_LINUX_IPX
  struct sockaddr_ipx ipx_addr;
#endif /* USE_LINUX_IPX */
#ifdef USE_WINDOWS
  struct sockaddr_ipx ipx_addr;
  struct sockaddr_nb netbios_addr;
  unsigned long ioarg;
#endif /* USE_WINDOWS */
  struct object *boot_obj,*tmpobj;
  struct var_stack *rts;
  struct var tmp;
  struct fns *func;
  
  boot_obj=ref_to_obj(0);
  devnum=-1;
  if (!net_type) net_type=net_protocol;
  if (net_type==CI_PROTOCOL_TCP) {
    loop=sizeof(struct sockaddr_in);
    new_fd=accept(sockfd,(struct sockaddr *) &tcp_addr,&loop);
  }
#ifdef USE_LINUX_IPX
  else if (net_type==CI_PROTOCOL_IPX) {
    loop=sizeof(struct sockaddr_ipx);
    new_fd=accept(sockfd,(struct sockaddr *) &ipx_addr,&loop);
  }
#endif /* USE_LINUX_IPX */
#ifdef USE_WINDOWS
  else if (net_type==CI_PROTOCOL_IPX) {
    loop=sizeof(struct sockaddr_ipx);
	new_fd=accept(sockfd,(struct sockaddr *) &ipx_addr,&loop);
  } else if (net_type==CI_PROTOCOL_NETBIOS) {
    loop=sizeof(struct sockaddr_nb);
	new_fd=accept(sockfd,(struct sockaddr *) &netbios_addr,&loop);
  }
#endif /* USE_WINDOWS */
  else return;
#ifdef USE_WINDOWS
  if (new_fd==INVALID_SOCKET) return;
  ioarg=1;
  if (ioctlsocket(new_fd,FIONBIO,&ioarg)) {
    shutdown(new_fd,2);
	closesocket(new_fd);
	return;
  }
#else /* USE_WINDOWS */
  if (new_fd<0) return;
  if (fcntl(new_fd,F_SETFL,O_NDELAY)<0) {
    shutdown(new_fd,2);
    close(new_fd);
    return;
  }
#endif /* USE_WINDOWS */
  loop=0;
  while (loop<num_conns) {
    if (connlist[loop].fd==(-1)) {
      devnum=loop;
      break;
    }
    loop++;
  }   
  if (devnum==(-1) || (boot_obj->devnum!=(-1))) {
    shutdown(new_fd,2);
#ifdef USE_WINDOWS
    closesocket(new_fd);
#else /* USE_WINDOWS */
    close(new_fd);
#endif /* USE_WINDOWS */
    return;
  }
  connlist[devnum].fd=new_fd;
  connlist[devnum].inbuf_count=0;
  if (net_type==CI_PROTOCOL_TCP)
    connlist[devnum].address.tcp_addr=tcp_addr;
#ifdef USE_LINUX_IPX
  if (net_type==CI_PROTOCOL_IPX)
    connlist[devnum].address.ipx_addr=ipx_addr;
#endif /* USE_LINUX_IPX */
#ifdef USE_WINDOWS
  if (net_type==CI_PROTOCOL_IPX)
    connlist[devnum].address.ipx_addr=ipx_addr;
  if (net_type==CI_PROTOCOL_NETBIOS)
    connlist[devnum].address.netbios_addr=netbios_addr;
#endif /* USE_WINDOWS */
  connlist[devnum].net_type=net_type;
  connlist[devnum].obj=boot_obj;
  connlist[devnum].outbuf_count=0;
  connlist[devnum].outbuf=NULL;
  connlist[devnum].conn_time=now_time;
  connlist[devnum].last_input_time=now_time;
  boot_obj->devnum=devnum;
  boot_obj->flags|=CONNECTED;

#ifdef CYCLE_HARD_MAX
  hard_cycles=0;
#endif /* CYCLE_HARD_MAX */

#ifdef CYCLE_SOFT_MAX
  soft_cycles=0;
#endif /* CYCLE_SOFT_MAX */

#ifdef USE_WINDOWS
  SelectWSAPrepare(new_fd,SELMODE_DATA);
#endif /* USE_WINDOWS */

  func=find_function("connect",boot_obj,&tmpobj);
  if (func) {
    tmp.type=NUM_ARGS;
    tmp.value.num=0;
    rts=NULL;
    push(&tmp,&rts);
    interp(NULL,tmpobj,NULL,&rts,func);
    free_stack(&rts);
  }
  handle_destruct();
}

void buffer_input(int conn_num) {
  static char buf[MAX_STR_LEN];
  struct var_stack *rts;
  struct var tmp;
  struct fns *func;
  int retlen;
  struct object *obj,*tmpobj;
  int loop;

#ifdef USE_WINDOWS
  retlen=recv(connlist[conn_num].fd,buf,MAX_STR_LEN-2,0);
#else /* USE_WINDOWS */  
  retlen=read(connlist[conn_num].fd,buf,MAX_STR_LEN-2);
#endif /* USE_WINDOWS */
  if (retlen<=0) {
    obj=connlist[conn_num].obj;
    immediate_disconnect(conn_num);
    func=find_function("disconnect",obj,&tmpobj);

#ifdef CYCLE_HARD_MAX
    hard_cycles=0;
#endif /* CYCLE_HARD_MAX */

#ifdef CYCLE_SOFT_MAX
    soft_cycles=0;
#endif /* CYCLE_SOFT_MAX */

    if (func) {
      rts=NULL;
      tmp.type=NUM_ARGS;
      tmp.value.num=0;
      push(&tmp,&rts);
      interp(NULL,tmpobj,NULL,&rts,func);
      free_stack(&rts);
    }
    handle_destruct();
    return;
  }
  loop=0;
  while (loop<retlen) {
    if (buf[loop]=='\n') {
       connlist[conn_num].inbuf[connlist[conn_num].inbuf_count]='\0';
       if (connlist[conn_num].obj->flags & IN_EDITOR)
         do_edit_command(connlist[conn_num].obj,connlist[conn_num].inbuf);
       else
         queue_command(connlist[conn_num].obj,connlist[conn_num].inbuf);
       connlist[conn_num].inbuf_count=0;
    } else
      if (buf[loop]!='\r' && (isgraph(buf[loop]) || buf[loop]==' ')) {
        if (connlist[conn_num].inbuf_count<MAX_STR_LEN-2) {
          connlist[conn_num].inbuf[connlist[conn_num].inbuf_count]=buf[loop];
          ++(connlist[conn_num].inbuf_count);
	}
      }
    loop++;
  }
  connlist[conn_num].last_input_time=now_time;
}
  
/**
 * Handles input from all connected devices. This function continuously monitors
 * the input, output, and exception sets of file descriptors. It processes
 * incoming data, handles new connections, and manages disconnections. It also
 * deals with periodic tasks like database auto-saving and handling alarms.
 * The function operates in an infinite loop, handling input and output for each
 * connection and executing related functions based on the network activity.
 */
void handle_input() {
  fd_set input_set,output_set,exception_set;
  int loop;
  struct timeval delay_s;
  struct timeval *delay_p;
  struct var tmp;
  struct var_stack *rts;
  struct fns *func;
  struct object *obj,*tmpobj;

  set_now_time();
  while (1) {
    if (cache_top>transact_log_size) {
      log_sysmsg("  cache: auto-saving");
      if (save_db(NULL))
        log_sysmsg("  cache: auto-save failed");
      else
        log_sysmsg("  cache: auto-save completed");
    }
    FD_ZERO(&input_set);
    FD_ZERO(&output_set);
    FD_ZERO(&exception_set);
    loop=0;
    while (loop<num_conns) {
      if (connlist[loop].fd!=(-1)) {
        FD_SET(connlist[loop].fd,&input_set);
        FD_SET(connlist[loop].fd,&exception_set);
        if (connlist[loop].outbuf_count) {
          FD_SET(connlist[loop].fd,&output_set);
	}
      }
      loop++;
    }
    FD_SET(sockfd,&input_set);
    if (alarm_list) {
      if (alarm_list->delay>=now_time)
        delay_s.tv_sec=alarm_list->delay-now_time;
      else
        delay_s.tv_sec=0;
      delay_s.tv_usec=0;
      delay_p=&delay_s;
    } else
      delay_p=NULL;
#ifdef USE_WINDOWS
    NT_select(num_fds,&input_set,&output_set,&exception_set,delay_p);
#else /* USE_WINDOWS */
    select(num_fds,&input_set,&output_set,&exception_set,delay_p);
#endif /* USE_WINDOWS */
    set_now_time();
    if (FD_ISSET(sockfd,&input_set)) make_new_conn(sockfd,0);
    loop=0;
    while (loop<num_conns) {
      if (connlist[loop].fd!=(-1))
        if (FD_ISSET(connlist[loop].fd,&exception_set)) {
          obj=connlist[loop].obj;
          immediate_disconnect(loop);
          func=find_function("disconnect",obj,&tmpobj);

#ifdef CYCLE_HARD_MAX
          hard_cycles=0;
#endif /* CYCLE_HARD_MAX */

#ifdef CYCLE_SOFT_MAX
          soft_cycles=0;
#endif /* CYCLE_SOFT_MAX */

          if (func) {
            rts=NULL;
            tmp.type=NUM_ARGS;
            tmp.value.num=0;
            push(&tmp,&rts);
            interp(NULL,tmpobj,NULL,&rts,func);
            free_stack(&rts);
          }
          handle_destruct();
        }
      loop++;
    }
    loop=0;
    while (loop<num_conns) {
      if (connlist[loop].fd!=(-1))
        if (FD_ISSET(connlist[loop].fd,&input_set))
          buffer_input(loop);
      loop++;
    }
    loop=0;
    while (loop<num_conns) {
      if (connlist[loop].fd!=(-1))
        if (FD_ISSET(connlist[loop].fd,&output_set))
          unbuf_output(loop);
      loop++;
    }

#ifdef CYCLE_HARD_MAX
    hard_cycles=0;
#endif /* CYCLE_HARD_MAX */

    do {
      handle_destruct();
      handle_alarm();
      handle_destruct();
      handle_command();
      handle_destruct();
      handle_alarm();
      handle_destruct();
    } while (cmd_head || dest_list);
    unload_data();
  }
}

/**
 * Initializes the network interfaces for the application. It sets the maximum
 * number of file descriptors, determines the network protocol (TCP, IPX, NetBIOS),
 * and configures the socket for the selected protocol. It binds the socket to
 * the appropriate port and starts listening for incoming connections, allocating
 * memory for connection list and setting signal handling for SIGPIPE.
 * 
 * @param port Structure containing network parameters.
 * @param do_single Flag to check if the interface should run in single-user mode.
 * @return Integer status code indicating success or type of failure.
 */
int init_interface(struct net_parms *port, int do_single) {
  int loop=0;
  struct sockaddr_in tcp_server;
#ifdef USE_LINUX_IPX
  struct sockaddr_ipx ipx_server;
#endif /* USE_LINUX_IPX */
#ifdef USE_WINDOWS
  struct sockaddr_ipx ipx_server;
  struct sockaddr_nb netbios_server;
#endif /* USE_WINDOWS */

  set_num_fds();
  if (do_single) return NOSINGLE;
  net_protocol=port->protocol;
  switch (net_protocol) {
    case CI_PROTOCOL_TCP:
#ifdef USE_WINDOWS
      sockfd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	  if (sockfd==INVALID_SOCKET) return NOSOCKET;
#else /* USE_WINDOWS */
      sockfd=socket(AF_INET,SOCK_STREAM,0);
	  if (sockfd<0) return NOSOCKET;
#endif /* USE_WINDOWS */
      tcp_server.sin_family=AF_INET;
      tcp_server.sin_addr.s_addr=INADDR_ANY;
      tcp_server.sin_port=htons(port->tcp_port);
      if (bind(sockfd,(struct sockaddr *) &tcp_server, sizeof(tcp_server)))
        return PORTINUSE;
      break;
    case CI_PROTOCOL_IPX:
#ifdef USE_WINDOWS
      sockfd=socket(AF_IPX,SOCK_SEQPACKET,NSPROTO_SPX);
      if (sockfd==INVALID_SOCKET) return NOSOCKET;
      ipx_server.sa_family=AF_IPX;
      ipx_server.sa_netnum[0]=(unsigned char) ((port->ipx_net) & 0xFF);
      ipx_server.sa_netnum[1]=(unsigned char) (((port->ipx_net) >> 8) & 0xFF);
      ipx_server.sa_netnum[2]=(unsigned char) (((port->ipx_net) >> 16) & 0xFF);
      ipx_server.sa_netnum[3]=(unsigned char) (((port->ipx_net) >> 24) & 0xFF);
      memcpy(ipx_server.sa_nodenum,port->ipx_node,6);
      ipx_server.sa_socket=htons(port->ipx_socket);
      if (bind(sockfd,(struct sockaddr *) &ipx_server, sizeof(ipx_server)))
        return PORTINUSE;
      break;
#else /* USE_WINDOWS */
#ifdef USE_LINUX_IPX
      sockfd=socket(AF_IPX,SOCK_SEQPACKET,0);
      if (sockfd<0) return NOSOCKET;
      ipx_server.sipx_family=AF_IPX;
      ipx_server.sipx_network=htonl(port->ipx_net);
      memcpy(ipx_server.sipx_node,port->ipx_node,6);
      ipx_server.sipx_port=htons(port->ipx_socket);
      ipx_server.sipx_type=IPX_TYPE;
      if (bind(sockfd,(struct sockaddr *) &ipx_server, sizeof(ipx_server)))
        return PORTINUSE;
      break;
#else /* USE_LINUX_IPX */
      return NOPROT;
#endif /* USE_LINUX_IPX */
#endif /* USE_WINDOWS */
      break;
    case CI_PROTOCOL_NETBIOS:
#ifdef USE_WINDOWS
      sockfd=socket(AF_NETBIOS,SOCK_STREAM,0);
      if (sockfd==INVALID_SOCKET) return NOSOCKET;
      netbios_server.snb_family=AF_NETBIOS;
	  netbios_server.snb_type=NETBIOS_UNIQUE_NAME | NETBIOS_REGISTERING;
      loop=0;
	  while (loop<15) {
		netbios_server.snb_name[loop]=port->netbios_node[loop];
		loop++;
	  }
	  netbios_server.snb_name[15]=(unsigned char) port->netbios_port;
      if (bind(sockfd,(struct sockaddr *) &netbios_server,
               sizeof(netbios_server)))
        return PORTINUSE;
      break;
#else /* USE_WINDOWS */
      return NOPROT;
#endif /* USE_WINDOWS */
      break;
    default:
      return NOPROT;
  }
  listen(sockfd,5);
  num_conns=num_fds-(7+MIN_FREE_FILES);
  if (num_conns<1) num_conns=1;
  if (num_conns>MAX_CONNS) num_conns=MAX_CONNS;
  connlist=MALLOC(sizeof(struct connlist_s)*num_conns);
  loop=0;
  while (loop<num_conns) {
    connlist[loop].fd=(-1);
    loop++;
  }
#ifdef SIGPIPE
  signal(SIGPIPE,SIG_IGN);
#endif /* SIGPIPE */
#ifdef USE_WINDOWS
  SelectWSAPrepare(sockfd,SELMODE_LISTEN);
#endif /* USE_WINDOWS */
  return 0;
}

/**
 * Shuts down the network interface. This function iterates through the connection
 * list, immediately disconnecting any active connections. It then closes the main
 * socket and frees the allocated memory for the connection list. For Windows,
 * it additionally performs necessary cleanup for Winsock (Windows Sockets).
 */
void shutdown_interface() {
  int loop=0;

  while (loop<num_conns) {
    if (connlist[loop].fd!=(-1))
      immediate_disconnect(loop);
    loop++;
  }
#ifdef USE_WINDOWS
  closesocket(sockfd);
  WSACleanup();
#else /* USE_WINDOWS */
  close(sockfd);
#endif /* USE_WINDOWS */
  FREE(connlist);
}

/**
 * Returns the port number associated with a device's connection. The function 
 * extracts the port number from the device's connection information based on 
 * the network protocol (TCP, IPX, NetBIOS).
 * 
 * @param obj Pointer to the object representing the device.
 * @return Port number, or -1 if the device is not connected or the protocol is unsupported.
 */
int get_devport(struct object *obj) {
  if (!obj) return (-1);
  if (obj->devnum==-1) return (-1);
  if (connlist[obj->devnum].net_type==CI_PROTOCOL_TCP)
    return ntohs(((struct sockaddr_in *)
                  &(connlist[obj->devnum].address))->sin_port);
#ifdef USE_LINUX_IPX
  if (connlist[obj->devnum].net_type==CI_PROTOCOL_IPX)
    return ntohs(((struct sockaddr_ipx *)
                  &(connlist[obj->devnum].address))->sipx_port);
#endif /* USE_LINUX_IPX */
#ifdef USE_WINDOWS
  if (connlist[obj->devnum].net_type==CI_PROTOCOL_IPX)
    return ntohs(((struct sockaddr_ipx *)
	              &(connlist[obj->devnum].address))->sa_socket);
  if (connlist[obj->devnum].net_type==CI_PROTOCOL_NETBIOS)
    return ((struct sockaddr_nb *) 
            &(connlist[obj->devnum].address))->snb_name[15];
#endif /* USE_WINDOWS */
  return (-1);
}

/**
 * Retrieves the network protocol type of a device's connection. The function 
 * identifies whether the connection is using TCP, IPX, NetBIOS, or an unsupported 
 * protocol.
 * 
 * @param obj Pointer to the object representing the device.
 * @return Network protocol type, or -1 if the device is not connected.
 */
int get_devnet(struct object *obj) {
  if (!obj) return (-1);
  if (obj->devnum==-1) return (-1);
  return connlist[obj->devnum].net_type;
}

/**
 * Retrieves the connection address of a device as a string. The format of the 
 * address depends on the network protocol (TCP, IPX, NetBIOS). For TCP, it's an 
 * IPv4 address; for IPX, a combination of network and node numbers; for NetBIOS, 
 * the NetBIOS name.
 * 
 * @param obj Pointer to the object representing the device.
 * @return Connection address as a string, or "<unknown>" for an unrecognized type.
 */
char *get_devconn(struct object *obj) {
  struct sockaddr_in *tcp_ptr;
#ifdef USE_LINUX_IPX
  struct sockaddr_ipx *ipx_ptr;
  static char buf[ITOA_BUFSIZ+14];
#endif /* USE_LINUX_IPX */
#ifdef USE_WINDOWS
  struct sockaddr_ipx *ipx_ptr;
  struct sockaddr_nb *netbios_ptr;
  static char buf[16];
#endif /* USE_WINDOWS */

  if (!obj) return NULL;
  if (obj->devnum==-1) return NULL;
  if (connlist[obj->devnum].net_type==CI_PROTOCOL_TCP) {
    tcp_ptr=&(connlist[obj->devnum].address.tcp_addr);
    return inet_ntoa(tcp_ptr->sin_addr);
  }
#ifdef USE_LINUX_IPX
  if (connlist[obj->devnum].net_type==CI_PROTOCOL_IPX) {
    ipx_ptr=&(connlist[obj->devnum].address.ipx_addr);
    sprintf(buf,"%lX.%s",(unsigned long) ntohl(ipx_ptr->sipx_network),
            convert_from_6byte(&(ipx_ptr->sipx_node[0])));
    return buf;
  }
#endif /* USE_LINUX_IPX */
#ifdef USE_WINDOWS
  if (connlist[obj->devnum].net_type==CI_PROTOCOL_IPX) {
    unsigned long ipx_net;

    ipx_ptr=&(connlist[obj->devnum].address.ipx_addr);
	ipx_net=(ipx_ptr->sa_netnum[0]) +
	        ((ipx_ptr->sa_netnum[1]) << 8) +
			((ipx_ptr->sa_netnum[2]) << 16) +
			((ipx_ptr->sa_netnum[3]) << 24);
    sprintf(buf,"%lX.%s",ipx_net,
            convert_from_6byte(&(ipx_ptr->sa_nodenum[0])));
    return buf;
  }
  if (connlist[obj->devnum].net_type==CI_PROTOCOL_NETBIOS) {
	int loop;

    netbios_ptr=&(connlist[obj->devnum].address.netbios_addr);
	strncpy(buf,&(netbios_ptr->snb_name[0]),15);
	buf[15]='\0';
	loop=14;
	while (buf[loop]==' ' && loop) buf[loop--]='\0';
	return buf;
  }
#endif /* USE_WINDOWS */
  return "<unknown>";
}

/**
 * Sends a message to a specified device. This function appends the provided message 
 * to the device's output buffer. If the combined length of the existing buffer and 
 * the new message exceeds the maximum buffer length, the buffer is first flushed to 
 * make space. The message is truncated if it is too long, with a notice appended 
 * indicating that the output was flushed.
 * 
 * @param obj Pointer to the object representing the device.
 * @param msg The message to be sent.
 */
void send_device(struct object *obj, char *msg) {
  int len;
  char *tmp;

  if (!obj) return;
  if (obj->devnum==-1) return;
  if (!msg) return;
  len=strlen(msg);
  if (!len) return;
  if (connlist[obj->devnum].outbuf_count+len>MAX_OUTBUF_LEN) flush_device(obj);
  if (connlist[obj->devnum].outbuf_count>MAX_OUTBUF_LEN) return;
  if (connlist[obj->devnum].outbuf_count+len>MAX_OUTBUF_LEN) len+=24;
  if (connlist[obj->devnum].outbuf) {
    tmp=MALLOC(connlist[obj->devnum].outbuf_count+len+1);
    strcpy(tmp,connlist[obj->devnum].outbuf);
    strcat(tmp,msg);
    if (connlist[obj->devnum].outbuf_count+len-24>MAX_OUTBUF_LEN)
      strcat(tmp,"\n*** Output Flushed ***\n");
    FREE(connlist[obj->devnum].outbuf);
    connlist[obj->devnum].outbuf=tmp;
    connlist[obj->devnum].outbuf_count+=len;
  } else {
    tmp=MALLOC(len+1);
    strcpy(tmp,msg);
    if (len-24>MAX_OUTBUF_LEN)
      strcat(tmp,"\n*** Output Flushed ***\n");
    connlist[obj->devnum].outbuf=tmp;
    connlist[obj->devnum].outbuf_count=len;
  }
}

/**
 * Reassigns a device connection from one object to another. If the destination
 * object is already connected, or the source object is not connected, the 
 * operation fails. The source object's connection is transferred to the 
 * destination object, updating connection status accordingly. This is the 
 * primary function used to reconnect a device to an object in soft-code.
 * 
 * @param src Pointer to the source object.
 * @param dest Pointer to the destination object.
 * @return Zero on success, or 1 if the transfer is not possible.
 */
int reconnect_device(struct object *src, struct object *dest) {
  if (dest->devnum!=-1) return 1;
  if (src->devnum==-1) return 1;
  dest->devnum=src->devnum;
  src->flags&=~CONNECTED;
  src->devnum=(-1);
  dest->flags|=CONNECTED;
  connlist[dest->devnum].obj=dest;
  return 0;
}

/**
 * Immediately disconnects a specific device by closing its network connection.
 * It ensures the flushing of the device's output buffer and updates the
 * connection status of the associated object. This action is taken immediately
 * and is irreversible within the current session. This is the primary function
 * used throughout soft-code to disconnect a connection.
 *
 * @param obj Pointer to the object representing the device.
 */
void disconnect_device(struct object *obj) {
  if (obj->devnum==-1) return;
  if (!(obj->flags & CONNECTED)) return;
  immediate_disconnect(obj->devnum);
  obj->flags&=~CONNECTED;
  obj->devnum=(-1);
}

/**
 * Flushes the output buffer for a specific connected device. It is only called
 * from flush_device and is used to handle partial flushing by retaining any
 * unsent data. This ensures that all data in the buffer is attempted to be sent.
 *
 * @param obj Pointer to the object representing the device.
 */
void flush_a_device(struct object *obj) {
  char *tmp;
  int num_written;

  if (obj->devnum==-1) return;
  if (!(obj->flags & CONNECTED)) return;
  num_written=saniflush(connlist[obj->devnum].fd,
                        connlist[obj->devnum].outbuf_count,
                        connlist[obj->devnum].outbuf);
  if (num_written<=0) return;
  if (num_written<connlist[obj->devnum].outbuf_count) {
    tmp=MALLOC(connlist[obj->devnum].outbuf_count-num_written+1);
    strcpy(tmp,connlist[obj->devnum].outbuf+num_written);
    FREE(connlist[obj->devnum].outbuf);
    connlist[obj->devnum].outbuf=tmp;
    connlist[obj->devnum].outbuf_count-=num_written;
  } else {
    connlist[obj->devnum].outbuf_count=0;
    FREE(connlist[obj->devnum].outbuf);
    connlist[obj->devnum].outbuf=NULL;
  }
}

/**
 * Flushes output buffers for all connected devices. This function is primarily
 * called from send_device to ensure that the output buffer is completely sent
 * out, especially in cases of buffer overflow or when preparing for a shutdown.
 */
void flush_device(struct object *obj) {
  int loop;

  if (obj) {
    flush_a_device(obj);
    return;
  }
  loop=0;
  while (loop<num_conns) {
    if (connlist[loop].fd!=(-1)) flush_a_device(connlist[loop].obj);
    loop++;
  }
}

/**
 * Establishes a network connection for a device. This function creates a socket
 * and connects it to a specified address and port, based on the provided network
 * type. The device is then associated with an object, and its connection status
 * is updated. It's used to initiate outbound connections from the application.
 *
 * @param obj Pointer to the object representing the device.
 * @param address IP or hostname to connect to.
 * @param port Port number for the connection.
 * @param net_type Network protocol type (TCP, IPX, etc.).
 * @return 1 on successful connection, 0 otherwise.
 */
int connect_device(struct object *obj, char *address, int port, int net_type) {
  int loop,devnum,socklen;
  SOCKET new_fd;
  struct sockaddr *remote_host;
#ifdef USE_WINDOWS
  unsigned long ioarg;
#endif /* USE_WINDOWS */

  if (!net_type) net_type=net_protocol;
  if (get_af(net_type)==(-1)) return 0;
  if ((obj->flags & CONNECTED) || (obj->devnum!=(-1))) return 0;
  if ((socklen=convert_to_sockaddr(address,port,&remote_host,net_type))<0)
    return 0;
  devnum=(-1);
  loop=0;
  while (loop<num_conns) {
    if (connlist[loop].fd==(-1)) {
      devnum=loop;
      break;
    }
    loop++;
  }
  if (devnum==(-1)) return 0;
#ifdef USE_WINDOWS
  if (net_type==CI_PROTOCOL_IPX)
    new_fd=socket(get_af(net_type),SOCK_SEQPACKET,NSPROTO_SPX);
  else if (net_type==CI_PROTOCOL_TCP)
    new_fd=socket(get_af(net_type),SOCK_STREAM,IPPROTO_TCP);
  else
    new_fd=socket(get_af(net_type),SOCK_STREAM,0);
  if (new_fd==INVALID_SOCKET) return 0;  
#else /* USE_WINDOWS */
#ifdef USE_LINUX_IPX
  if (net_type==CI_PROTOCOL_IPX)
    new_fd=socket(get_af(net_type),SOCK_SEQPACKET,0);
  else
#endif /* USE_LINUX_IPX */
  new_fd=socket(get_af(net_type),SOCK_STREAM,0);
  if (new_fd<0) return 0;
#endif /* USE_WINDOWS */
  if (connect(new_fd,remote_host,socklen)) {
#ifdef USE_WINDOWS
    closesocket(new_fd);
#else /* USE_WINDOWS */
    close(new_fd);
#endif /* USE_WINDOWS */
    return 0;
  }
#ifdef USE_WINDOWS
  ioarg=1;
  if (ioctlsocket(new_fd,FIONBIO,&ioarg)) {
    shutdown(new_fd,2);
	closesocket(new_fd);
	return 0;
  }
#else /* USE_WINDOWS */
  if (fcntl(new_fd,F_SETFL,O_NDELAY)<0) {
    shutdown(new_fd,2);
    close(new_fd);
    return 0;
  }
#endif /* USE_WINDOWS */
  connlist[devnum].fd=new_fd;
  connlist[devnum].inbuf_count=0;
  connlist[devnum].net_type=net_type;
  if (net_type==CI_PROTOCOL_TCP)
    connlist[devnum].address.tcp_addr=*((struct sockaddr_in *) remote_host);
#ifdef USE_LINUX_IPX
  if (net_type==CI_PROTOCOL_IPX)
    connlist[devnum].address.ipx_addr=*((struct sockaddr_ipx *) remote_host);
#endif /* USE_LINUX_IPX */
#ifdef USE_WINDOWS
  if (net_type==CI_PROTOCOL_IPX)
    connlist[devnum].address.ipx_addr=*((struct sockaddr_ipx *) remote_host);
  if (net_type==CI_PROTOCOL_NETBIOS)
    connlist[devnum].address.netbios_addr=*((struct sockaddr_nb *) remote_host);
#endif /* USE_WINDOWS */
  connlist[devnum].obj=obj;
  connlist[devnum].outbuf_count=0;
  connlist[devnum].outbuf=NULL;
  connlist[devnum].conn_time=now_time;
  connlist[devnum].last_input_time=now_time;
  obj->devnum=devnum;
  obj->flags|=CONNECTED;
#ifdef USE_WINDOWS
  SelectWSAPrepare(new_fd,SELMODE_DATA);
#endif /* USE_WINDOWS */
  return 1;
}

/**
 * Calculates the inactive/idle time of a device since its last input. 
 *
 * @param obj Pointer to the object representing the device.
 * @return Idle time in seconds, or -1 if the device is not connected.
 */
long get_devidle(struct object *obj) {
  if ((!(obj->flags & CONNECTED)) || (obj->devnum==(-1))) return -1;
  return now_time-connlist[obj->devnum].last_input_time;
}

/**
 * Retrieves the the total active time of a device connection.
 *
 * @param obj Pointer to the object representing the device.
 * @return Connection time in seconds, or -1 if the device is not connected.
 */
long get_conntime(struct object *obj) {
  if ((!(obj->flags & CONNECTED)) || (obj->devnum==(-1))) return -1;
  return connlist[obj->devnum].conn_time;
}
