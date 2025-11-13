/**
 * @file intrface.c
 * @brief Modern POSIX socket interface for NetCI network communications
 *
 * Implements TCP/IP networking using poll() for event-driven I/O with non-blocking sockets.
 * Handles connection lifecycle, data buffering, and integration with NLPC object system.
 */

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

/**
 * @brief Convert hex string to MAC address
 *
 * Parses a 12-character hexadecimal string and converts it to a 6-byte MAC address.
 * Each pair of hex characters represents one byte of the address.
 *
 * @param s Hex string (must be exactly 12 characters, case-insensitive)
 * @param addr Output buffer for 6-byte MAC address
 * @return 0 on success, -1 if string length is invalid or contains non-hex characters
 */
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

/**
 * @brief Convert MAC address to hex string
 *
 * Converts a 6-byte MAC address to a 12-character uppercase hexadecimal string.
 * Uses a static buffer, so subsequent calls will overwrite previous results.
 *
 * @param addr 6-byte MAC address
 * @return Pointer to static buffer containing 12-character hex string (uppercase, null-terminated)
 */
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

/**
 * @brief Set socket to non-blocking mode
 *
 * Configures a socket file descriptor to use non-blocking I/O by setting the O_NONBLOCK flag.
 *
 * @param fd Socket file descriptor
 * @return 0 on success, -1 on error
 */
static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/**
 * @brief Determine maximum number of file descriptors
 *
 * Queries the system for the maximum number of open file descriptors and stores it in num_fds.
 * Falls back to 1024 if the system query fails.
 */
void set_num_fds() {
    num_fds = sysconf(_SC_OPEN_MAX);
    if (num_fds < 0) num_fds = 1024;
}

/**
 * @brief Detach process from terminal to run as daemon
 *
 * Forks the process, closes standard I/O streams, and creates a new session.
 * The parent process exits, leaving the child running as a daemon.
 *
 * @return -1 on fork failure, 1 in parent process (before exit), 0 in child (daemon) process
 */
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

/**
 * @brief Resolve hostname to IP address
 *
 * Performs DNS lookup to convert a hostname to its IPv4 address string.
 * Only supports TCP protocol type.
 *
 * @param host Hostname to resolve
 * @param net_type Protocol type (CI_PROTOCOL_TCP), or 0 to use net_protocol global
 * @return IP address string on success, NULL on failure or unsupported protocol
 */
char *host_to_addr(char *host, int net_type) {
    struct hostent *h;
    
    if (!net_type) net_type = net_protocol;
    if (net_type != CI_PROTOCOL_TCP) return NULL;
    
    h = gethostbyname(host);
    if (!h || h->h_addrtype != AF_INET || !h->h_addr_list[0])
        return NULL;
    
    return inet_ntoa(*((struct in_addr *)h->h_addr_list[0]));
}

/**
 * @brief Resolve IP address to hostname
 *
 * Performs reverse DNS lookup to convert an IPv4 address string to its hostname.
 * Only supports TCP protocol type.
 *
 * @param addr IP address string (e.g., "192.168.1.1")
 * @param net_type Protocol type (CI_PROTOCOL_TCP), or 0 to use net_protocol global
 * @return Hostname string on success, NULL on failure or unsupported protocol
 */
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

/**
 * @brief Iterate through connected objects
 *
 * Returns the next connected object in the connection list, used for iterating through all
 * active connections. Pass NULL to get the first connected object.
 *
 * @param obj Current object, or NULL to start iteration
 * @return Next connected object, or NULL if no more connections
 */
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

/**
 * @brief Write buffered output to socket
 *
 * Attempts to write buffered data to the socket, handling partial writes and EAGAIN conditions.
 * Writes up to WRITE_BURST bytes per call to avoid blocking.
 *
 * @param devnum Connection index in connlist array
 */
static void unbuf_output(int devnum) {
    int num_written;
    char *tmp;
    int write_size;
    
    if (connlist[devnum].outbuf_count == 0) return;
    
    write_size = (connlist[devnum].outbuf_count > WRITE_BURST) ?
                 WRITE_BURST : connlist[devnum].outbuf_count;
    
    logger(LOG_DEBUG, "intrface: writing buffered output");
    num_written = write(connlist[devnum].fd, connlist[devnum].outbuf, write_size);
    
    if (num_written <= 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            logger(LOG_DEBUG, "intrface: write would block, retry later");
            return;  /* Would block, try again later */
        }
        logger(LOG_WARNING, "intrface: write error on socket");
        /* Connection error, will be handled by exception */
        return;
    }
    
    if (num_written < connlist[devnum].outbuf_count) {
        /* Partial write */
        logger(LOG_DEBUG, "intrface: partial write, buffering remaining data");
        tmp = MALLOC(connlist[devnum].outbuf_count - num_written + 1);
        memcpy(tmp, connlist[devnum].outbuf + num_written,
               connlist[devnum].outbuf_count - num_written);
        tmp[connlist[devnum].outbuf_count - num_written] = '\0';
        connlist[devnum].outbuf_count -= num_written;
        FREE(connlist[devnum].outbuf);
        connlist[devnum].outbuf = tmp;
    } else {
        /* Complete write */
        logger(LOG_DEBUG, "intrface: output buffer flushed");
        connlist[devnum].outbuf_count = 0;
        FREE(connlist[devnum].outbuf);
        connlist[devnum].outbuf = NULL;
    }
}

/**
 * @brief Update current time global
 *
 * Updates the now_time global variable with the current system time converted to integer format.
 */
void set_now_time() {
    now_time = time2int(time(NULL));
}

/**
 * @brief Immediately disconnect a device
 *
 * Closes the socket connection, flushes remaining output, and cleans up connection resources.
 * Logs the disconnection with IP address and object details.
 *
 * @param devnum Connection index in connlist array, or -1 to do nothing
 */
void immediate_disconnect(int devnum) {
    char logbuf[256];
    char *ip_addr;
    
    if (devnum == -1) return;
    
    /* Get IP address for logging */
    ip_addr = inet_ntoa(connlist[devnum].address.tcp_addr.sin_addr);
    
    sprintf(logbuf, "intrface: disconnecting %s from obj #%ld (%s)",
            ip_addr,
            (long)connlist[devnum].obj->refno,
            connlist[devnum].obj->parent ? connlist[devnum].obj->parent->pathname : "no-parent");
    logger(LOG, logbuf);
    
    if (connlist[devnum].obj->flags & IN_EDITOR)
        remove_from_edit(connlist[devnum].obj);
    
    connlist[devnum].obj->flags &= ~CONNECTED;
    
    /* Try to flush remaining output */
    if (connlist[devnum].outbuf_count > 0) {
        sprintf(logbuf, "intrface: flushing %d bytes before disconnect",
                connlist[devnum].outbuf_count);
        logger(LOG_DEBUG, logbuf);
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

/**
 * @brief Send IAC command sequence
 *
 * Sends a 3-byte telnet IAC sequence (IAC + command + option)
 *
 * @param conn_num Connection index
 * @param command Telnet command (WILL/WONT/DO/DONT)
 * @param option Telnet option code
 */
static void send_iac(int conn_num, unsigned char command, unsigned char option) {
    unsigned char buf[3];
    buf[0] = TELNET_IAC;
    buf[1] = command;
    buf[2] = option;
    write(connlist[conn_num].fd, buf, 3);
}

/**
 * @brief Send IAC GA (Go Ahead)
 *
 * Sends IAC GA if client has not negotiated SGA
 *
 * @param conn_num Connection index
 */
static void send_iac_ga(int conn_num) {
    unsigned char buf[2];
    if (!connlist[conn_num].opt_sga) {
        buf[0] = TELNET_IAC;
        buf[1] = TELNET_GA;
        write(connlist[conn_num].fd, buf, 2);
    }
}

/**
 * @brief Send MSSP (MUD Server Status Protocol) data
 *
 * Sends MSSP variables via IAC SB MSSP ... IAC SE
 * Merges runtime data with custom mudlib data from mssp_data mapping
 *
 * @param conn_num Connection index
 */
static void send_mssp(int conn_num) {
    unsigned char buf[2048];  /* Larger buffer for custom data */
    int len = 0;
    int active_players = 0;
    long uptime;
    char uptime_str[32];
    extern struct heap_mapping *mssp_data;
    
    /* Count active players */
    for (int i = 0; i < num_conns; i++) {
        if (connlist[i].fd != -1 && connlist[i].obj) {
            active_players++;
        }
    }
    
    /* Calculate uptime */
    uptime = now_time - boot_time;
    sprintf(uptime_str, "%ld", uptime);
    
    /* Build MSSP packet: IAC SB MSSP VAR VAL ... IAC SE */
    buf[len++] = TELNET_IAC;
    buf[len++] = TELNET_SB;
    buf[len++] = TELOPT_MSSP;
    
    /* MSSP_VAR = 1, MSSP_VAL = 2 */
    #define MSSP_VAR 1
    #define MSSP_VAL 2
    
    /* NAME */
    buf[len++] = MSSP_VAR;
    len += sprintf((char *)(buf + len), "NAME");
    buf[len++] = MSSP_VAL;
    len += sprintf((char *)(buf + len), "NetCI MUD");
    
    /* PLAYERS */
    buf[len++] = MSSP_VAR;
    len += sprintf((char *)(buf + len), "PLAYERS");
    buf[len++] = MSSP_VAL;
    len += sprintf((char *)(buf + len), "%d", active_players);
    
    /* UPTIME */
    buf[len++] = MSSP_VAR;
    len += sprintf((char *)(buf + len), "UPTIME");
    buf[len++] = MSSP_VAL;
    len += sprintf((char *)(buf + len), "%s", uptime_str);
    
    /* CODEBASE */
    buf[len++] = MSSP_VAR;
    len += sprintf((char *)(buf + len), "CODEBASE");
    buf[len++] = MSSP_VAL;
    len += sprintf((char *)(buf + len), "NetCI");
    
    /* FAMILY */
    buf[len++] = MSSP_VAR;
    len += sprintf((char *)(buf + len), "FAMILY");
    buf[len++] = MSSP_VAL;
    len += sprintf((char *)(buf + len), "LPMud");
    
    /* LANGUAGE */
    buf[len++] = MSSP_VAR;
    len += sprintf((char *)(buf + len), "LANGUAGE");
    buf[len++] = MSSP_VAL;
    len += sprintf((char *)(buf + len), "NLPC");
    
    /* Add custom MSSP variables from mudlib if set */
    extern struct mssp_var *mssp_vars;
    extern int mssp_var_count;
    
    if (mssp_vars && mssp_var_count > 0) {
        for (int i = 0; i < mssp_var_count; i++) {
            /* Only add if we have room (leave 100 bytes for safety) */
            if (len + strlen(mssp_vars[i].name) + strlen(mssp_vars[i].value) + 10 < 1948) {
                buf[len++] = MSSP_VAR;
                len += sprintf((char *)(buf + len), "%s", mssp_vars[i].name);
                buf[len++] = MSSP_VAL;
                len += sprintf((char *)(buf + len), "%s", mssp_vars[i].value);
            }
        }
    }
    
    /* End MSSP subnegotiation */
    buf[len++] = TELNET_IAC;
    buf[len++] = TELNET_SE;
    
    write(connlist[conn_num].fd, buf, len);
    connlist[conn_num].opt_mssp = 1;
    
    logger(LOG_DEBUG, "intrface: sent MSSP data");
}

/**
 * @brief Handle telnet option negotiation
 *
 * Responds to IAC DO/DONT/WILL/WONT requests
 *
 * @param conn_num Connection index
 * @param command Negotiation command
 * @param option Option being negotiated
 */
static void handle_telnet_negotiation(int conn_num, unsigned char command, unsigned char option) {
    char logbuf[128];
    
    sprintf(logbuf, "intrface: telnet %s %d",
            command == TELNET_DO ? "DO" :
            command == TELNET_DONT ? "DONT" :
            command == TELNET_WILL ? "WILL" : "WONT",
            option);
    logger(LOG_DEBUG, logbuf);
    
    switch (command) {
        case TELNET_DO:
            /* Client requests we enable option */
            switch (option) {
                case TELOPT_ECHO:
                    /* We will control echo */
                    send_iac(conn_num, TELNET_WILL, TELOPT_ECHO);
                    connlist[conn_num].opt_echo = 1;
                    logger(LOG_DEBUG, "intrface: echo enabled");
                    break;
                case TELOPT_SGA:
                    /* We will suppress go-ahead */
                    send_iac(conn_num, TELNET_WILL, TELOPT_SGA);
                    connlist[conn_num].opt_sga = 1;
                    logger(LOG_DEBUG, "intrface: SGA enabled");
                    break;
                case TELOPT_MSSP:
                    /* Send MSSP data */
                    send_iac(conn_num, TELNET_WILL, TELOPT_MSSP);
                    send_mssp(conn_num);
                    break;
                default:
                    /* Refuse unknown options */
                    send_iac(conn_num, TELNET_WONT, option);
                    break;
            }
            break;
            
        case TELNET_DONT:
            /* Client requests we disable option */
            switch (option) {
                case TELOPT_ECHO:
                    connlist[conn_num].opt_echo = 0;
                    send_iac(conn_num, TELNET_WONT, TELOPT_ECHO);
                    break;
                case TELOPT_SGA:
                    connlist[conn_num].opt_sga = 0;
                    send_iac(conn_num, TELNET_WONT, TELOPT_SGA);
                    break;
                default:
                    send_iac(conn_num, TELNET_WONT, option);
                    break;
            }
            break;
            
        case TELNET_WILL:
            /* Client offers to enable option */
            switch (option) {
                case TELOPT_NAWS:
                    /* Accept window size negotiation */
                    send_iac(conn_num, TELNET_DO, TELOPT_NAWS);
                    connlist[conn_num].opt_naws = 1;
                    logger(LOG_DEBUG, "intrface: NAWS accepted");
                    break;
                case TELOPT_TTYPE:
                    /* Accept terminal type and request it */
                    send_iac(conn_num, TELNET_DO, TELOPT_TTYPE);
                    connlist[conn_num].opt_ttype = 1;
                    logger(LOG_DEBUG, "intrface: TTYPE accepted");
                    
                    /* Send subnegotiation to request terminal type string */
                    /* IAC SB TTYPE SEND IAC SE */
                    {
                        unsigned char ttype_req[6];
                        ttype_req[0] = TELNET_IAC;
                        ttype_req[1] = TELNET_SB;
                        ttype_req[2] = TELOPT_TTYPE;
                        ttype_req[3] = 1;  /* SEND command */
                        ttype_req[4] = TELNET_IAC;
                        ttype_req[5] = TELNET_SE;
                        write(connlist[conn_num].fd, ttype_req, 6);
                        logger(LOG_DEBUG, "intrface: sent TTYPE SEND request");
                    }
                    break;
                default:
                    /* Refuse other options */
                    send_iac(conn_num, TELNET_DONT, option);
                    break;
            }
            break;
            
        case TELNET_WONT:
            /* Client refuses option */
            /* Just acknowledge, nothing to do */
            break;
    }
}

/**
 * @brief Handle telnet subnegotiation
 *
 * Processes IAC SB ... IAC SE sequences (NAWS, etc)
 *
 * @param conn_num Connection index
 */
static void handle_telnet_subnegotiation(int conn_num) {
    unsigned char opt = connlist[conn_num].sb_opt;
    unsigned char *buf = connlist[conn_num].sb_buf;
    int len = connlist[conn_num].sb_len;
    char logbuf[128];
    
    switch (opt) {
        case TELOPT_NAWS:
            /* Window size: 2 bytes width, 2 bytes height (big endian) */
            if (len >= 4) {
                connlist[conn_num].win_width = (buf[0] << 8) | buf[1];
                connlist[conn_num].win_height = (buf[2] << 8) | buf[3];
                sprintf(logbuf, "intrface: NAWS %dx%d",
                        connlist[conn_num].win_width,
                        connlist[conn_num].win_height);
                logger(LOG_DEBUG, logbuf);
            }
            break;
            
        case TELOPT_TTYPE:
            /* Terminal type response */
            if (len > 1 && buf[0] == 0) {  /* IS command */
                int term_len = len - 1;
                if (term_len >= 64) term_len = 63;
                memcpy(connlist[conn_num].term_type, buf + 1, term_len);
                connlist[conn_num].term_type[term_len] = '\0';
                sprintf(logbuf, "intrface: TTYPE %s", connlist[conn_num].term_type);
                logger(LOG_DEBUG, logbuf);
            }
            break;
            
        default:
            sprintf(logbuf, "intrface: unknown subnegotiation %d", opt);
            logger(LOG_DEBUG, logbuf);
            break;
    }
}

/**
 * @brief Accept new incoming connection
 *
 * Accepts a connection on the listening socket, assigns it to the boot object, and calls the
 * boot object's connect() function. Logs connection details including IP address and object.
 *
 * @param listen_fd Listening socket file descriptor
 */
static void make_new_conn(int listen_fd) {
    int devnum;
    int new_fd;
    struct sockaddr_in tcp_addr;
    socklen_t addr_len;
    struct object *boot_obj, *tmpobj;
    struct var_stack *rts;
    struct var tmp;
    struct fns *func;
    char logbuf[256];
    char *ip_addr;
    
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
    
    /* Get IP address for logging */
    ip_addr = inet_ntoa(tcp_addr.sin_addr);
    
    sprintf(logbuf, "intrface: connection from %s:%d",
            ip_addr, ntohs(tcp_addr.sin_port));
    logger(LOG_DEBUG, logbuf);
    
    /* Set non-blocking */
    if (set_nonblocking(new_fd) < 0) {
        sprintf(logbuf, "intrface: failed to set non-blocking for %s", ip_addr);
        logger(LOG_ERROR, logbuf);
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
        sprintf(logbuf, "intrface: rejected %s (no slots or boot busy)", ip_addr);
        logger(LOG_WARNING, logbuf);
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
    
    /* Initialize telnet protocol state */
    connlist[devnum].telnet_state = TELNET_STATE_DATA;
    connlist[devnum].telnet_opt = 0;
    connlist[devnum].sb_opt = 0;
    connlist[devnum].sb_len = 0;
    connlist[devnum].opt_echo = 0;
    connlist[devnum].opt_sga = 0;
    connlist[devnum].opt_mssp = 0;
    connlist[devnum].opt_naws = 0;
    connlist[devnum].opt_ttype = 0;
    connlist[devnum].win_width = 80;
    connlist[devnum].win_height = 24;
    connlist[devnum].term_type[0] = '\0';  /* No terminal type yet */
    
    boot_obj->devnum = devnum;
    boot_obj->flags |= CONNECTED;
    
    /* Send initial telnet negotiations */
    logger(LOG_DEBUG, "intrface: sending initial telnet negotiations");
    send_iac(devnum, TELNET_WILL, TELOPT_ECHO);
    send_iac(devnum, TELNET_WILL, TELOPT_SGA);
    send_iac(devnum, TELNET_DO, TELOPT_TTYPE);  /* Request terminal type */
    send_iac(devnum, TELNET_DO, TELOPT_NAWS);   /* Request window size */
    
    sprintf(logbuf, "intrface: %s connected to obj #%ld (boot)",
            ip_addr, (long)boot_obj->refno);
    logger(LOG, logbuf);
    
    /* Call connect function */
    func = find_function("connect", boot_obj, &tmpobj);
    if (func) {
        logger(LOG_DEBUG, "intrface: calling boot connect()");
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

/**
 * @brief Read and buffer input from connection
 *
 * Reads data from the socket, processes line-by-line input, and queues commands for execution.
 * Handles connection closure and errors, calling the disconnect() function when needed.
 *
 * @param conn_num Connection index in connlist array
 */
static void buffer_input(int conn_num) {
    static char buf[MAX_STR_LEN];
    struct var_stack *rts;
    struct var tmp;
    struct fns *func;
    int retlen;
    struct object *obj, *tmpobj;
    char logbuf[256];
    char *ip_addr;
    
    retlen = read(connlist[conn_num].fd, buf, MAX_STR_LEN - 2);
    
    if (retlen <= 0) {
        if (retlen == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
            /* Connection closed or error */
            ip_addr = inet_ntoa(connlist[conn_num].address.tcp_addr.sin_addr);
            
            if (retlen == 0) {
                sprintf(logbuf, "intrface: %s closed connection (obj #%ld)",
                        ip_addr, (long)connlist[conn_num].obj->refno);
                logger(LOG, logbuf);
            } else {
                sprintf(logbuf, "intrface: read error from %s (obj #%ld)",
                        ip_addr, (long)connlist[conn_num].obj->refno);
                logger(LOG_WARNING, logbuf);
            }
            
            obj = connlist[conn_num].obj;
            immediate_disconnect(conn_num);
            
            func = find_function("disconnect", obj, &tmpobj);
            if (func) {
                logger(LOG_DEBUG, "intrface: calling disconnect() function");
                rts = NULL;
                tmp.type = NUM_ARGS;
                tmp.value.num = 0;
                push(&tmp, &rts);
                interp(NULL, tmpobj, NULL, &rts, func);
                free_stack(&rts);
            }
            handle_destruct();
        } else {
            logger(LOG_DEBUG, "intrface: read would block");
        }
        return;
    }
    
    /* Log received data at debug level */
    sprintf(logbuf, "intrface: received %d bytes from obj #%ld",
            retlen, (long)connlist[conn_num].obj->refno);
    logger(LOG_DEBUG, logbuf);
    
    /* Process input with telnet IAC state machine */
    for (int i = 0; i < retlen; i++) {
        unsigned char ch = (unsigned char)buf[i];
        
        switch (connlist[conn_num].telnet_state) {
            case TELNET_STATE_DATA:
                /* Normal data processing */
                if (ch == TELNET_IAC) {
                    connlist[conn_num].telnet_state = TELNET_STATE_IAC;
                } else if (ch == '\n') {
                    connlist[conn_num].inbuf[connlist[conn_num].inbuf_count] = '\0';
                    if (connlist[conn_num].obj->flags & IN_EDITOR)
                        do_edit_command(connlist[conn_num].obj, connlist[conn_num].inbuf);
                    else
                        queue_command(connlist[conn_num].obj, connlist[conn_num].inbuf);
                    connlist[conn_num].inbuf_count = 0;
                } else if (ch != '\r' && (isgraph(ch) || ch == ' ')) {
                    if (connlist[conn_num].inbuf_count < MAX_STR_LEN - 2) {
                        connlist[conn_num].inbuf[connlist[conn_num].inbuf_count] = ch;
                        connlist[conn_num].inbuf_count++;
                    }
                }
                break;
                
            case TELNET_STATE_IAC:
                /* Received IAC, determine command */
                if (ch == TELNET_IAC) {
                    /* Escaped IAC (255 255 = literal 255) */
                    if (connlist[conn_num].inbuf_count < MAX_STR_LEN - 2) {
                        connlist[conn_num].inbuf[connlist[conn_num].inbuf_count] = ch;
                        connlist[conn_num].inbuf_count++;
                    }
                    connlist[conn_num].telnet_state = TELNET_STATE_DATA;
                } else if (ch == TELNET_WILL) {
                    connlist[conn_num].telnet_state = TELNET_STATE_WILL;
                } else if (ch == TELNET_WONT) {
                    connlist[conn_num].telnet_state = TELNET_STATE_WONT;
                } else if (ch == TELNET_DO) {
                    connlist[conn_num].telnet_state = TELNET_STATE_DO;
                } else if (ch == TELNET_DONT) {
                    connlist[conn_num].telnet_state = TELNET_STATE_DONT;
                } else if (ch == TELNET_SB) {
                    connlist[conn_num].telnet_state = TELNET_STATE_SB;
                    connlist[conn_num].sb_len = 0;
                } else if (ch == TELNET_GA || ch == TELNET_SE) {
                    /* Ignore GA, SE without SB */
                    connlist[conn_num].telnet_state = TELNET_STATE_DATA;
                } else {
                    /* Unknown command, ignore */
                    connlist[conn_num].telnet_state = TELNET_STATE_DATA;
                }
                break;
                
            case TELNET_STATE_WILL:
                connlist[conn_num].telnet_opt = ch;
                handle_telnet_negotiation(conn_num, TELNET_WILL, ch);
                connlist[conn_num].telnet_state = TELNET_STATE_DATA;
                break;
                
            case TELNET_STATE_WONT:
                connlist[conn_num].telnet_opt = ch;
                handle_telnet_negotiation(conn_num, TELNET_WONT, ch);
                connlist[conn_num].telnet_state = TELNET_STATE_DATA;
                break;
                
            case TELNET_STATE_DO:
                connlist[conn_num].telnet_opt = ch;
                handle_telnet_negotiation(conn_num, TELNET_DO, ch);
                connlist[conn_num].telnet_state = TELNET_STATE_DATA;
                break;
                
            case TELNET_STATE_DONT:
                connlist[conn_num].telnet_opt = ch;
                handle_telnet_negotiation(conn_num, TELNET_DONT, ch);
                connlist[conn_num].telnet_state = TELNET_STATE_DATA;
                break;
                
            case TELNET_STATE_SB:
                /* Start of subnegotiation - first byte is option */
                connlist[conn_num].sb_opt = ch;
                connlist[conn_num].sb_len = 0;
                connlist[conn_num].telnet_state = TELNET_STATE_SB_IAC;
                break;
                
            case TELNET_STATE_SB_IAC:
                /* In subnegotiation, collecting data */
                if (ch == TELNET_IAC) {
                    /* Might be end of subnegotiation or escaped IAC */
                    /* Next byte will tell us */
                    int next_i = i + 1;
                    if (next_i < retlen) {
                        unsigned char next_ch = (unsigned char)buf[next_i];
                        if (next_ch == TELNET_SE) {
                            /* End of subnegotiation */
                            handle_telnet_subnegotiation(conn_num);
                            connlist[conn_num].telnet_state = TELNET_STATE_DATA;
                            i++; /* Skip SE */
                        } else if (next_ch == TELNET_IAC) {
                            /* Escaped IAC in subnegotiation */
                            if (connlist[conn_num].sb_len < 256) {
                                connlist[conn_num].sb_buf[connlist[conn_num].sb_len++] = TELNET_IAC;
                            }
                            i++; /* Skip second IAC */
                        }
                    }
                } else {
                    /* Regular data in subnegotiation */
                    if (connlist[conn_num].sb_len < 256) {
                        connlist[conn_num].sb_buf[connlist[conn_num].sb_len++] = ch;
                    }
                }
                break;
        }
    }
    
    connlist[conn_num].last_input_time = now_time;
}

/**
 * @brief Main network event loop
 *
 * Continuously polls for network events using poll(), handles new connections, processes I/O,
 * executes commands, and manages alarms. This is the core of the network interface.
 */
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
        /* Auto-save removed - database no longer exists */
        /* Use save_object() in mudlib for selective persistence */
        
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
            logger(LOG_DEBUG, "intrface: incoming connection detected");
            make_new_conn(sockfd);
        }
        
        /* Check client connections */
        for (int i = 1; i < nfds; i++) {
            int conn_num = -1;
            char logbuf[256];
            char *ip_addr;
            
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
                ip_addr = inet_ntoa(connlist[conn_num].address.tcp_addr.sin_addr);
                
                if (fds[i].revents & POLLERR) {
                    sprintf(logbuf, "intrface: socket error on %s (obj #%ld)",
                            ip_addr, (long)connlist[conn_num].obj->refno);
                    logger(LOG_WARNING, logbuf);
                } else {
                    sprintf(logbuf, "intrface: hangup on %s (obj #%ld)",
                            ip_addr, (long)connlist[conn_num].obj->refno);
                    logger(LOG, logbuf);
                }
                
                obj = connlist[conn_num].obj;
                immediate_disconnect(conn_num);
                
                func = find_function("disconnect", obj, &tmpobj);
                if (func) {
                    logger(LOG_DEBUG, "intrface: calling disconnect() function");
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
                logger(LOG_DEBUG, "intrface: data available for reading");
                buffer_input(conn_num);
            }
            
            /* Handle output */
            if (fds[i].revents & POLLOUT) {
                logger(LOG_DEBUG, "intrface: socket ready for writing");
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

/**
 * @brief Initialize network interface
 *
 * Creates and configures the listening socket, sets up non-blocking mode, binds to the specified
 * port, and allocates the connection list. Logs initialization progress.
 *
 * @param port Network parameters including protocol and TCP port
 * @param do_single If non-zero, requests single-user mode (not supported)
 * @return 0 on success, NOSINGLE/NOPROT/NOSOCKET/PORTINUSE on error
 */
int init_interface(struct net_parms *port, int do_single) {
    struct sockaddr_in tcp_server;
    int opt = 1;
    
    logger(LOG_INFO, "intrface: initializing network interface");
    set_num_fds();
    
    if (do_single) {
        logger(LOG_ERROR, "intrface: single-user mode not supported");
        return NOSINGLE;
    }
    
    net_protocol = port->protocol;
    
    if (net_protocol != CI_PROTOCOL_TCP) {
        logger(LOG_ERROR, "intrface: unsupported protocol");
        return NOPROT;
    }
    
    /* Create socket */
    logger(LOG_DEBUG, "intrface: creating TCP socket");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        logger(LOG_ERROR, "intrface: failed to create socket");
        return NOSOCKET;
    }
    
    /* Set socket options */
    logger(LOG_DEBUG, "intrface: setting SO_REUSEADDR");
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    /* Set non-blocking */
    logger(LOG_DEBUG, "intrface: setting non-blocking mode");
    if (set_nonblocking(sockfd) < 0) {
        logger(LOG_ERROR, "intrface: failed to set non-blocking mode");
        close(sockfd);
        return NOSOCKET;
    }
    
    /* Bind */
    memset(&tcp_server, 0, sizeof(tcp_server));
    tcp_server.sin_family = AF_INET;
    tcp_server.sin_addr.s_addr = INADDR_ANY;
    tcp_server.sin_port = htons(port->tcp_port);
    
    logger(LOG_DEBUG, "intrface: binding to port");
    if (bind(sockfd, (struct sockaddr *)&tcp_server, sizeof(tcp_server)) < 0) {
        logger(LOG_ERROR, "intrface: port already in use");
        close(sockfd);
        return PORTINUSE;
    }
    
    /* Listen */
    logger(LOG_DEBUG, "intrface: listening for connections");
    if (listen(sockfd, 5) < 0) {
        logger(LOG_ERROR, "intrface: listen() failed");
        close(sockfd);
        return NOSOCKET;
    }
    
    /* Allocate connection list */
    num_conns = num_fds - (7 + MIN_FREE_FILES);
    if (num_conns < 1) num_conns = 1;
    if (num_conns > MAX_CONNS) num_conns = MAX_CONNS;
    
    logger(LOG_INFO, "intrface: ready for connections");
    
    connlist = MALLOC(sizeof(struct connlist_s) * num_conns);
    for (int i = 0; i < num_conns; i++) {
        connlist[i].fd = -1;
    }
    
    signal(SIGPIPE, SIG_IGN);
    
    return 0;
}

/**
 * @brief Shutdown network interface
 *
 * Disconnects all active connections, closes the listening socket, and frees connection list.
 * Logs shutdown progress.
 */
void shutdown_interface() {
    logger(LOG_INFO, "intrface: shutting down network interface");
    
    for (int i = 0; i < num_conns; i++) {
        if (connlist[i].fd != -1)
            immediate_disconnect(i);
    }
    
    close(sockfd);
    FREE(connlist);
    
    logger(LOG_INFO, "intrface: network interface shutdown complete");
}

/**
 * @brief Get device remote port number
 *
 * Returns the remote port number of the connection associated with the object.
 *
 * @param obj Object with active connection
 * @return Remote port number, or -1 if object is not connected
 */
int get_devport(struct object *obj) {
    if (!obj || obj->devnum == -1) return -1;
    return ntohs(connlist[obj->devnum].address.tcp_addr.sin_port);
}

/**
 * @brief Get device network protocol type
 *
 * Returns the network protocol type (CI_PROTOCOL_TCP) for the connection.
 *
 * @param obj Object with active connection
 * @return Protocol type, or -1 if object is not connected
 */
int get_devnet(struct object *obj) {
    if (!obj || obj->devnum == -1) return -1;
    return connlist[obj->devnum].net_type;
}

/**
 * @brief Get device IP address
 *
 * Returns the remote IP address string of the connection associated with the object.
 *
 * @param obj Object with active connection
 * @return IP address string, or NULL if object is not connected
 */
char *get_devconn(struct object *obj) {
    if (!obj || obj->devnum == -1) return NULL;
    return inet_ntoa(connlist[obj->devnum].address.tcp_addr.sin_addr);
}

/**
 * @brief Send data to device
 *
 * Appends message to the object's output buffer. If buffer exceeds MAX_OUTBUF_LEN, flushes first.
 * Data is sent asynchronously when socket becomes writable.
 *
 * @param obj Object with active connection
 * @param msg Message string to send (null-terminated)
 */
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

/**
 * @brief Send prompt to connected device with IAC GA if needed
 *
 * Sends a prompt string and appends IAC GA (Go Ahead) if the client
 * has not negotiated SGA (Suppress Go Ahead). This ensures proper
 * telnet protocol compliance for line-mode terminals.
 *
 * @param obj Object with active connection
 * @param prompt Prompt string to send (null-terminated)
 */
void send_prompt(struct object *obj, char *prompt) {
    if (!obj || obj->devnum == -1 || !prompt) return;
    
    /* Send the prompt text */
    send_device(obj, prompt);
    
    /* Send IAC GA if SGA not negotiated */
    if (!connlist[obj->devnum].opt_sga) {
        unsigned char ga_seq[2] = { TELNET_IAC, TELNET_GA };
        int len;
        char *tmp;
        
        /* Append IAC GA to outbuf */
        if (connlist[obj->devnum].outbuf) {
            len = connlist[obj->devnum].outbuf_count + 2;
            tmp = MALLOC(len + 1);
            memcpy(tmp, connlist[obj->devnum].outbuf, connlist[obj->devnum].outbuf_count);
            tmp[connlist[obj->devnum].outbuf_count] = ga_seq[0];
            tmp[connlist[obj->devnum].outbuf_count + 1] = ga_seq[1];
            tmp[len] = '\0';
            FREE(connlist[obj->devnum].outbuf);
            connlist[obj->devnum].outbuf = tmp;
            connlist[obj->devnum].outbuf_count = len;
        } else {
            tmp = MALLOC(3);
            tmp[0] = ga_seq[0];
            tmp[1] = ga_seq[1];
            tmp[2] = '\0';
            connlist[obj->devnum].outbuf = tmp;
            connlist[obj->devnum].outbuf_count = 2;
        }
    }
}

/**
 * @brief Transfer connection from one object to another
 *
 * Moves an active connection from the source object to the destination object, typically used
 * when transitioning from boot object to player object. Logs the transfer with object details.
 *
 * @param src Source object (must have active connection)
 * @param dest Destination object (must not have a connection)
 * @return 0 on success, 1 if destination is busy or source is not connected
 */
int reconnect_device(struct object *src, struct object *dest) {
    char logbuf[256];
    char *ip_addr;
    
    if (dest->devnum != -1) return 1;
    if (src->devnum == -1) return 1;
    
    ip_addr = inet_ntoa(connlist[src->devnum].address.tcp_addr.sin_addr);
    
    sprintf(logbuf, "intrface: %s reconnecting from obj #%ld (%s) to obj #%ld (%s)",
            ip_addr,
            (long)src->refno,
            src->parent ? src->parent->pathname : "no-parent",
            (long)dest->refno,
            dest->parent ? dest->parent->pathname : "no-parent");
    logger(LOG, logbuf);
    
    dest->devnum = src->devnum;
    src->flags &= ~CONNECTED;
    src->devnum = -1;
    dest->flags |= CONNECTED;
    connlist[dest->devnum].obj = dest;
    
    return 0;
}

/**
 * @brief Disconnect device by request
 *
 * Programmatically disconnects an object's connection, typically called from NLPC code.
 * Logs the disconnection with IP address and object details.
 *
 * @param obj Object with active connection
 */
void disconnect_device(struct object *obj) {
    char logbuf[256];
    char *ip_addr;
    
    if (obj->devnum == -1) return;
    if (!(obj->flags & CONNECTED)) return;
    
    ip_addr = inet_ntoa(connlist[obj->devnum].address.tcp_addr.sin_addr);
    
    sprintf(logbuf, "intrface: disconnecting %s from obj #%ld (%s) by request",
            ip_addr,
            (long)obj->refno,
            obj->parent ? obj->parent->pathname : "no-parent");
    logger(LOG, logbuf);
    
    immediate_disconnect(obj->devnum);
    obj->flags &= ~CONNECTED;
    obj->devnum = -1;
}

/**
 * @brief Flush output buffer to socket
 *
 * Immediately attempts to write buffered output for the specified object, or all connections
 * if obj is NULL. Used to ensure data is sent before critical operations.
 *
 * @param obj Object to flush, or NULL to flush all connections
 */
void flush_device(struct object *obj) {
    char logbuf[256];
    
    if (obj) {
        if (obj->devnum != -1) {
            sprintf(logbuf, "intrface: flushing output for obj #%ld",
                    (long)obj->refno);
            logger(LOG_DEBUG, logbuf);
            unbuf_output(obj->devnum);
        }
        return;
    }
    
    logger(LOG_DEBUG, "intrface: flushing all connections");
    for (int i = 0; i < num_conns; i++) {
        if (connlist[i].fd != -1)
            unbuf_output(i);
    }
}

/**
 * @brief Establish outbound connection
 *
 * Creates a new outbound TCP connection from the object to the specified remote address and port.
 * Logs connection attempts and results with full details.
 *
 * @param obj Object to associate with the connection (must not be connected)
 * @param address Remote IP address string
 * @param port Remote port number
 * @param net_type Protocol type (CI_PROTOCOL_TCP), or 0 to use net_protocol global
 * @return 1 on success, 0 on failure
 */
int connect_device(struct object *obj, char *address, int port, int net_type) {
    int devnum, new_fd;
    struct sockaddr_in remote_host;
    char logbuf[256];
    
    if (!net_type) net_type = net_protocol;
    if (net_type != CI_PROTOCOL_TCP) {
        sprintf(logbuf, "intrface: obj #%ld outbound failed (unsupported protocol)",
                (long)obj->refno);
        logger(LOG_WARNING, logbuf);
        return 0;
    }
    if ((obj->flags & CONNECTED) || (obj->devnum != -1)) {
        sprintf(logbuf, "intrface: obj #%ld outbound failed (already connected)",
                (long)obj->refno);
        logger(LOG_WARNING, logbuf);
        return 0;
    }
    
    sprintf(logbuf, "intrface: obj #%ld initiating outbound to %s:%d",
            (long)obj->refno, address, port);
    logger(LOG, logbuf);
    
    /* Find free slot */
    devnum = -1;
    for (int i = 0; i < num_conns; i++) {
        if (connlist[i].fd == -1) {
            devnum = i;
            break;
        }
    }
    
    if (devnum == -1) {
        sprintf(logbuf, "intrface: obj #%ld outbound to %s:%d failed (no free slots)",
                (long)obj->refno, address, port);
        logger(LOG_WARNING, logbuf);
        return 0;
    }
    
    /* Create socket */
    new_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (new_fd < 0) {
        sprintf(logbuf, "intrface: obj #%ld outbound to %s:%d failed (socket creation)",
                (long)obj->refno, address, port);
        logger(LOG_ERROR, logbuf);
        return 0;
    }
    
    /* Connect */
    memset(&remote_host, 0, sizeof(remote_host));
    remote_host.sin_family = AF_INET;
    remote_host.sin_port = htons(port);
    remote_host.sin_addr.s_addr = inet_addr(address);
    
    logger(LOG_DEBUG, "intrface: connecting to remote host");
    if (connect(new_fd, (struct sockaddr *)&remote_host, sizeof(remote_host)) < 0) {
        sprintf(logbuf, "intrface: obj #%ld outbound to %s:%d failed (refused)",
                (long)obj->refno, address, port);
        logger(LOG_WARNING, logbuf);
        close(new_fd);
        return 0;
    }
    
    /* Set non-blocking */
    if (set_nonblocking(new_fd) < 0) {
        sprintf(logbuf, "intrface: obj #%ld outbound to %s:%d failed (non-blocking)",
                (long)obj->refno, address, port);
        logger(LOG_ERROR, logbuf);
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
    
    sprintf(logbuf, "intrface: obj #%ld (%s) connected to %s:%d",
            (long)obj->refno,
            obj->parent ? obj->parent->pathname : "no-parent",
            address, port);
    logger(LOG, logbuf);
    
    return 1;
}

/**
 * @brief Get connection idle time
 *
 * Returns the number of seconds since the last input was received from the connection.
 *
 * @param obj Object with active connection
 * @return Idle time in seconds, or -1 if object is not connected
 */
long get_devidle(struct object *obj) {
    if (!(obj->flags & CONNECTED) || obj->devnum == -1) return -1;
    return now_time - connlist[obj->devnum].last_input_time;
}

/**
 * @brief Get connection establishment time
 *
 * Returns the timestamp when the connection was established.
 *
 * @param obj Object with active connection
 * @return Connection time as integer timestamp, or -1 if object is not connected
 */
long get_conntime(struct object *obj) {
    if (!(obj->flags & CONNECTED) || obj->devnum == -1) return -1;
    return connlist[obj->devnum].conn_time;
}
