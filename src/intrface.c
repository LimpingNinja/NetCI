/* intrface_modern.c - Modern POSIX socket interface */

#define CMPLNG_INTRFCE

#include "config.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

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

/* Connection list and globals */
struct connlist_s *connlist;
int num_conns, num_fds, net_protocol;
int sockfd;

/* Convert hex string to 6-byte MAC address */
int convert_to_6byte(char *s, unsigned char *addr) {
    static unsigned char a[6];
    int len, c, count;
    
    len = strlen(s);
    if (len != 12) return -1;
    
    count = 0;
    while (count < 6) {
        a[count] = 0;
        if (!isxdigit(s[count * 2])) return -1;
        if (isdigit(s[count * 2]))
            c = s[count * 2] - '0';
        else if (isupper(s[count * 2]))
            c = s[count * 2] - 'A' + 10;
        else
            c = s[count * 2] - 'a' + 10;
        a[count] = c << 4;
        
        if (!isxdigit(s[(count * 2) + 1])) return -1;
        if (isdigit(s[(count * 2) + 1]))
            c = s[(count * 2) + 1] - '0';
        else if (isupper(s[(count * 2) + 1]))
            c = s[(count * 2) + 1] - 'A' + 10;
        else
            c = s[(count * 2) + 1] - 'a' + 10;
        a[count] |= c;
        count++;
    }
    memcpy(addr, a, 6);
    return 0;
}

/* Convert 6-byte MAC address to hex string */
char *convert_from_6byte(unsigned char *addr) {
    static char buf[13];
    int loop, c;
    
    loop = 0;
    while (loop < 6) {
        c = addr[loop] & 15;
        if (c < 10)
            c += '0';
        else
            c += 'A' - 10;
        buf[loop * 2 + 1] = c;
        
        c = (addr[loop] >> 4) & 15;
        if (c < 10)
            c += '0';
        else
            c += 'A' - 10;
        buf[loop * 2] = c;
        loop++;
    }
    buf[12] = '\0';
    return buf;
}

/* Set socket to non-blocking mode */
static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/* Set number of file descriptors */
void set_num_fds() {
    num_fds = sysconf(_SC_OPEN_MAX);
    if (num_fds < 0) num_fds = 1024;
}

/* Detach from terminal (daemon mode) */
int detach_session() {
    int retval;
    
    signal(SIGCHLD, SIG_IGN);
    retval = fork();
    if (retval == -1) return -1;
    if (retval) return 1;
    
    signal(SIGHUP, SIG_IGN);
    fclose(stdin);
    fclose(stdout);
    fclose(stderr);
    
    setsid();
    return 0;
}

/* DNS resolution */
char *host_to_addr(char *host, int net_type) {
    struct hostent *h;
    
    if (!net_type) net_type = net_protocol;
    if (net_type != CI_PROTOCOL_TCP) return NULL;
    
    h = gethostbyname(host);
    if (!h || h->h_addrtype != AF_INET || !h->h_addr_list[0])
        return NULL;
    
    return inet_ntoa(*((struct in_addr *)h->h_addr_list[0]));
}

char *addr_to_host(char *addr, int net_type) {
    struct hostent *h;
    struct in_addr tcp_addr;
    
    if (!net_type) net_type = net_protocol;
    if (net_type != CI_PROTOCOL_TCP) return NULL;
    
    tcp_addr.s_addr = inet_addr(addr);
    if ((long)tcp_addr.s_addr == -1) return NULL;
    
    h = gethostbyaddr((char *)&tcp_addr, sizeof(tcp_addr), AF_INET);
    if (!h || !*(h->h_name)) return NULL;
    
    return h->h_name;
}

/* Iterate through connected objects */
struct object *next_who(struct object *obj) {
    long loop;
    
    if (obj)
        loop = obj->devnum;
    else
        loop = -1;
    
    while (++loop < num_conns)
        if (connlist[loop].fd != -1)
            return connlist[loop].obj;
    
    return NULL;
}

/* Write buffered output to socket */
static void unbuf_output(int devnum) {
    int num_written;
    char *tmp;
    int write_size;
    
    if (connlist[devnum].outbuf_count == 0) return;
    
    write_size = (connlist[devnum].outbuf_count > WRITE_BURST) ?
                 WRITE_BURST : connlist[devnum].outbuf_count;
    
    num_written = write(connlist[devnum].fd, connlist[devnum].outbuf, write_size);
    
    if (num_written <= 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;  /* Would block, try again later */
        /* Connection error, will be handled by exception */
        return;
    }
    
    if (num_written < connlist[devnum].outbuf_count) {
        /* Partial write */
        tmp = MALLOC(connlist[devnum].outbuf_count - num_written + 1);
        memcpy(tmp, connlist[devnum].outbuf + num_written,
               connlist[devnum].outbuf_count - num_written);
        tmp[connlist[devnum].outbuf_count - num_written] = '\0';
        connlist[devnum].outbuf_count -= num_written;
        FREE(connlist[devnum].outbuf);
        connlist[devnum].outbuf = tmp;
    } else {
        /* Complete write */
        connlist[devnum].outbuf_count = 0;
        FREE(connlist[devnum].outbuf);
        connlist[devnum].outbuf = NULL;
    }
}

/* Update current time */
void set_now_time() {
    now_time = time2int(time(NULL));
}

/* Immediately disconnect a device */
void immediate_disconnect(int devnum) {
    if (devnum == -1) return;
    
    if (connlist[devnum].obj->flags & IN_EDITOR)
        remove_from_edit(connlist[devnum].obj);
    
    connlist[devnum].obj->flags &= ~CONNECTED;
    
    /* Try to flush remaining output */
    if (connlist[devnum].outbuf_count > 0) {
        write(connlist[devnum].fd, connlist[devnum].outbuf,
              connlist[devnum].outbuf_count);
    }
    
    shutdown(connlist[devnum].fd, SHUT_RDWR);
    close(connlist[devnum].fd);
    
    connlist[devnum].fd = -1;
    connlist[devnum].obj->devnum = -1;
    connlist[devnum].outbuf_count = 0;
    
    if (connlist[devnum].outbuf) {
        FREE(connlist[devnum].outbuf);
        connlist[devnum].outbuf = NULL;
    }
}

/* Accept new connection */
static void make_new_conn(int listen_fd) {
    int devnum;
    int new_fd;
    struct sockaddr_in tcp_addr;
    socklen_t addr_len;
    struct object *boot_obj, *tmpobj;
    struct var_stack *rts;
    struct var tmp;
    struct fns *func;
    
    boot_obj = ref_to_obj(0);
    devnum = -1;
    
    logger(LOG_DEBUG, "intrface: accepting new connection");
    
    /* Accept the connection */
    addr_len = sizeof(tcp_addr);
    new_fd = accept(listen_fd, (struct sockaddr *)&tcp_addr, &addr_len);
    
    if (new_fd < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            logger(LOG_DEBUG, "intrface: spurious wakeup (EAGAIN)");
            return;
        }
        logger(LOG_ERROR, "intrface: accept() failed");
        return;
    }
    
    logger(LOG_DEBUG, "intrface: connection accepted, setting non-blocking");
    
    /* Set non-blocking */
    if (set_nonblocking(new_fd) < 0) {
        logger(LOG_ERROR, "intrface: failed to set non-blocking mode");
        close(new_fd);
        return;
    }
    
    /* Find free slot */
    for (int i = 0; i < num_conns; i++) {
        if (connlist[i].fd == -1) {
            devnum = i;
            break;
        }
    }
    
    if (devnum == -1 || boot_obj->devnum != -1) {
        logger(LOG_WARNING, "intrface: connection rejected (no slots or boot busy)");
        close(new_fd);
        return;
    }
    
    /* Initialize connection */
    connlist[devnum].fd = new_fd;
    connlist[devnum].inbuf_count = 0;
    connlist[devnum].address.tcp_addr = tcp_addr;
    connlist[devnum].net_type = CI_PROTOCOL_TCP;
    connlist[devnum].obj = boot_obj;
    connlist[devnum].outbuf_count = 0;
    connlist[devnum].outbuf = NULL;
    connlist[devnum].conn_time = now_time;
    connlist[devnum].last_input_time = now_time;
    
    boot_obj->devnum = devnum;
    boot_obj->flags |= CONNECTED;
    
    logger(LOG_DEBUG, "intrface: calling boot object connect()");
    
    /* Call connect function */
    func = find_function("connect", boot_obj, &tmpobj);
    if (func) {
        tmp.type = NUM_ARGS;
        tmp.value.num = 0;
        rts = NULL;
        push(&tmp, &rts);
        interp(NULL, tmpobj, NULL, &rts, func);
        free_stack(&rts);
        logger(LOG_DEBUG, "intrface: boot connect() completed");
    } else {
        logger(LOG_WARNING, "intrface: boot object has no connect() function");
    }
    
    handle_destruct();
}

/* Buffer input from connection */
static void buffer_input(int conn_num) {
    static char buf[MAX_STR_LEN];
    struct var_stack *rts;
    struct var tmp;
    struct fns *func;
    int retlen;
    struct object *obj, *tmpobj;
    
    retlen = read(connlist[conn_num].fd, buf, MAX_STR_LEN - 2);
    
    if (retlen <= 0) {
        if (retlen == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
            /* Connection closed or error */
            obj = connlist[conn_num].obj;
            immediate_disconnect(conn_num);
            
            func = find_function("disconnect", obj, &tmpobj);
            if (func) {
                rts = NULL;
                tmp.type = NUM_ARGS;
                tmp.value.num = 0;
                push(&tmp, &rts);
                interp(NULL, tmpobj, NULL, &rts, func);
                free_stack(&rts);
            }
            handle_destruct();
        }
        return;
    }
    
    /* Process input */
    for (int i = 0; i < retlen; i++) {
        if (buf[i] == '\n') {
            connlist[conn_num].inbuf[connlist[conn_num].inbuf_count] = '\0';
            if (connlist[conn_num].obj->flags & IN_EDITOR)
                do_edit_command(connlist[conn_num].obj, connlist[conn_num].inbuf);
            else
                queue_command(connlist[conn_num].obj, connlist[conn_num].inbuf);
            connlist[conn_num].inbuf_count = 0;
        } else if (buf[i] != '\r' && (isgraph(buf[i]) || buf[i] == ' ')) {
            if (connlist[conn_num].inbuf_count < MAX_STR_LEN - 2) {
                connlist[conn_num].inbuf[connlist[conn_num].inbuf_count] = buf[i];
                connlist[conn_num].inbuf_count++;
            }
        }
    }
    
    connlist[conn_num].last_input_time = now_time;
}

/* Main event loop */
void handle_input() {
    struct pollfd *fds;
    int nfds;
    int timeout;
    struct var tmp;
    struct var_stack *rts;
    struct fns *func;
    struct object *obj, *tmpobj;
    
    set_now_time();
    
    /* Allocate poll array */
    fds = MALLOC(sizeof(struct pollfd) * (num_conns + 1));
    
    while (1) {
        /* Auto-save check */
        if (cache_top > transact_log_size) {
            logger(LOG, "  cache: auto-saving");
            if (save_db(NULL))
                logger(LOG, "  cache: auto-save failed");
            else
                logger(LOG, "  cache: auto-save completed");
        }
        
        /* Build poll array */
        nfds = 0;
        
        /* Listening socket */
        fds[nfds].fd = sockfd;
        fds[nfds].events = POLLIN;
        fds[nfds].revents = 0;
        nfds++;
        
        /* Client connections */
        for (int i = 0; i < num_conns; i++) {
            if (connlist[i].fd != -1) {
                fds[nfds].fd = connlist[i].fd;
                fds[nfds].events = POLLIN | POLLERR | POLLHUP;
                if (connlist[i].outbuf_count > 0)
                    fds[nfds].events |= POLLOUT;
                fds[nfds].revents = 0;
                nfds++;
            }
        }
        
        /* Calculate timeout */
        if (alarm_list) {
            if (alarm_list->delay >= now_time)
                timeout = (alarm_list->delay - now_time) * 1000;
            else
                timeout = 0;
        } else {
            timeout = -1;  /* Infinite */
        }
        
        /* Poll */
        int ret = poll(fds, nfds, timeout);
        set_now_time();
        
        if (ret < 0) {
            if (errno == EINTR) continue;
            logger(LOG_ERROR, "intrface: poll() error");
            continue;
        }
        
        /* Check listening socket */
        if (fds[0].revents & POLLIN) {
            make_new_conn(sockfd);
        }
        
        /* Check client connections */
        for (int i = 1; i < nfds; i++) {
            int conn_num = -1;
            
            /* Find connection number */
            for (int j = 0; j < num_conns; j++) {
                if (connlist[j].fd == fds[i].fd) {
                    conn_num = j;
                    break;
                }
            }
            
            if (conn_num == -1) continue;
            
            /* Handle errors/hangup */
            if (fds[i].revents & (POLLERR | POLLHUP)) {
                obj = connlist[conn_num].obj;
                immediate_disconnect(conn_num);
                
                func = find_function("disconnect", obj, &tmpobj);
                if (func) {
                    rts = NULL;
                    tmp.type = NUM_ARGS;
                    tmp.value.num = 0;
                    push(&tmp, &rts);
                    interp(NULL, tmpobj, NULL, &rts, func);
                    free_stack(&rts);
                }
                handle_destruct();
                continue;
            }
            
            /* Handle input */
            if (fds[i].revents & POLLIN) {
                buffer_input(conn_num);
            }
            
            /* Handle output */
            if (fds[i].revents & POLLOUT) {
                unbuf_output(conn_num);
            }
        }
        
        /* Process commands and alarms */
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
    
    FREE(fds);
}

/* Initialize network interface */
int init_interface(struct net_parms *port, int do_single) {
    struct sockaddr_in tcp_server;
    int opt = 1;
    
    set_num_fds();
    
    if (do_single) return NOSINGLE;
    
    net_protocol = port->protocol;
    
    if (net_protocol != CI_PROTOCOL_TCP)
        return NOPROT;
    
    /* Create socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return NOSOCKET;
    
    /* Set socket options */
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    /* Set non-blocking */
    if (set_nonblocking(sockfd) < 0) {
        close(sockfd);
        return NOSOCKET;
    }
    
    /* Bind */
    memset(&tcp_server, 0, sizeof(tcp_server));
    tcp_server.sin_family = AF_INET;
    tcp_server.sin_addr.s_addr = INADDR_ANY;
    tcp_server.sin_port = htons(port->tcp_port);
    
    if (bind(sockfd, (struct sockaddr *)&tcp_server, sizeof(tcp_server)) < 0) {
        close(sockfd);
        return PORTINUSE;
    }
    
    /* Listen */
    if (listen(sockfd, 5) < 0) {
        close(sockfd);
        return NOSOCKET;
    }
    
    /* Allocate connection list */
    num_conns = num_fds - (7 + MIN_FREE_FILES);
    if (num_conns < 1) num_conns = 1;
    if (num_conns > MAX_CONNS) num_conns = MAX_CONNS;
    
    connlist = MALLOC(sizeof(struct connlist_s) * num_conns);
    for (int i = 0; i < num_conns; i++) {
        connlist[i].fd = -1;
    }
    
    signal(SIGPIPE, SIG_IGN);
    
    return 0;
}

/* Shutdown network interface */
void shutdown_interface() {
    for (int i = 0; i < num_conns; i++) {
        if (connlist[i].fd != -1)
            immediate_disconnect(i);
    }
    
    close(sockfd);
    FREE(connlist);
}

/* Get device port */
int get_devport(struct object *obj) {
    if (!obj || obj->devnum == -1) return -1;
    return ntohs(connlist[obj->devnum].address.tcp_addr.sin_port);
}

/* Get device network type */
int get_devnet(struct object *obj) {
    if (!obj || obj->devnum == -1) return -1;
    return connlist[obj->devnum].net_type;
}

/* Get device connection address */
char *get_devconn(struct object *obj) {
    if (!obj || obj->devnum == -1) return NULL;
    return inet_ntoa(connlist[obj->devnum].address.tcp_addr.sin_addr);
}

/* Send data to device */
void send_device(struct object *obj, char *msg) {
    int len;
    char *tmp;
    
    if (!obj || obj->devnum == -1 || !msg) return;
    
    len = strlen(msg);
    if (len == 0) return;
    
    /* Check buffer size */
    if (connlist[obj->devnum].outbuf_count + len > MAX_OUTBUF_LEN) {
        flush_device(obj);
    }
    
    if (connlist[obj->devnum].outbuf_count > MAX_OUTBUF_LEN) return;
    
    if (connlist[obj->devnum].outbuf_count + len > MAX_OUTBUF_LEN)
        len = MAX_OUTBUF_LEN - connlist[obj->devnum].outbuf_count;
    
    /* Append to buffer */
    if (connlist[obj->devnum].outbuf) {
        tmp = MALLOC(connlist[obj->devnum].outbuf_count + len + 1);
        memcpy(tmp, connlist[obj->devnum].outbuf, connlist[obj->devnum].outbuf_count);
        memcpy(tmp + connlist[obj->devnum].outbuf_count, msg, len);
        tmp[connlist[obj->devnum].outbuf_count + len] = '\0';
        FREE(connlist[obj->devnum].outbuf);
        connlist[obj->devnum].outbuf = tmp;
        connlist[obj->devnum].outbuf_count += len;
    } else {
        tmp = MALLOC(len + 1);
        memcpy(tmp, msg, len);
        tmp[len] = '\0';
        connlist[obj->devnum].outbuf = tmp;
        connlist[obj->devnum].outbuf_count = len;
    }
}

/* Reconnect device to different object */
int reconnect_device(struct object *src, struct object *dest) {
    if (dest->devnum != -1) return 1;
    if (src->devnum == -1) return 1;
    
    dest->devnum = src->devnum;
    src->flags &= ~CONNECTED;
    src->devnum = -1;
    dest->flags |= CONNECTED;
    connlist[dest->devnum].obj = dest;
    
    return 0;
}

/* Disconnect device */
void disconnect_device(struct object *obj) {
    if (obj->devnum == -1) return;
    if (!(obj->flags & CONNECTED)) return;
    
    immediate_disconnect(obj->devnum);
    obj->flags &= ~CONNECTED;
    obj->devnum = -1;
}

/* Flush device output */
void flush_device(struct object *obj) {
    if (obj) {
        if (obj->devnum != -1)
            unbuf_output(obj->devnum);
        return;
    }
    
    for (int i = 0; i < num_conns; i++) {
        if (connlist[i].fd != -1)
            unbuf_output(i);
    }
}

/* Connect device (outbound) */
int connect_device(struct object *obj, char *address, int port, int net_type) {
    int devnum, new_fd;
    struct sockaddr_in remote_host;
    
    if (!net_type) net_type = net_protocol;
    if (net_type != CI_PROTOCOL_TCP) return 0;
    if ((obj->flags & CONNECTED) || (obj->devnum != -1)) return 0;
    
    /* Find free slot */
    devnum = -1;
    for (int i = 0; i < num_conns; i++) {
        if (connlist[i].fd == -1) {
            devnum = i;
            break;
        }
    }
    
    if (devnum == -1) return 0;
    
    /* Create socket */
    new_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (new_fd < 0) return 0;
    
    /* Connect */
    memset(&remote_host, 0, sizeof(remote_host));
    remote_host.sin_family = AF_INET;
    remote_host.sin_port = htons(port);
    remote_host.sin_addr.s_addr = inet_addr(address);
    
    if (connect(new_fd, (struct sockaddr *)&remote_host, sizeof(remote_host)) < 0) {
        close(new_fd);
        return 0;
    }
    
    /* Set non-blocking */
    if (set_nonblocking(new_fd) < 0) {
        close(new_fd);
        return 0;
    }
    
    /* Initialize connection */
    connlist[devnum].fd = new_fd;
    connlist[devnum].inbuf_count = 0;
    connlist[devnum].net_type = net_type;
    connlist[devnum].address.tcp_addr = remote_host;
    connlist[devnum].obj = obj;
    connlist[devnum].outbuf_count = 0;
    connlist[devnum].outbuf = NULL;
    connlist[devnum].conn_time = now_time;
    connlist[devnum].last_input_time = now_time;
    
    obj->devnum = devnum;
    obj->flags |= CONNECTED;
    
    return 1;
}

/* Get device idle time */
long get_devidle(struct object *obj) {
    if (!(obj->flags & CONNECTED) || obj->devnum == -1) return -1;
    return now_time - connlist[obj->devnum].last_input_time;
}

/* Get connection time */
long get_conntime(struct object *obj) {
    if (!(obj->flags & CONNECTED) || obj->devnum == -1) return -1;
    return connlist[obj->devnum].conn_time;
}
