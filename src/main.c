/* main.c */

#ifdef USE_WINDOWS
#include <windows.h>
#endif /* USE_WINDOWS */
#include "config.h"
#include "object.h"
#include "interp.h"
#include "intrface.h"
#include "cache.h"
#include "constrct.h"
#include "clearq.h"
#include "globals.h"
#include "dbhandle.h"
#include "file.h"
#ifdef USE_WINDOWS
#include "winmain.h"
#endif /* USE_WINDOWS */

#ifdef USE_WINDOWS
extern HWND hwnd;
#endif /* USE_WINDOWS */

struct net_parms port;

#ifndef USE_WINDOWS
char *get_base_file_name(char *s) {
  char *retval;

  retval=s;
  while (*s)
    if ((*(s++))=='/') retval=s;
  return retval;
}
#endif /* !USE_WINDOWS */

int split_key_val(char *buf,char *desired_key,char *desired_val) {
  int len,count;

  count=0;
  while (buf[count]) if (buf[count]=='=') break; else count++;
  len=count;
  while (len)
    if (isspace(buf[len-1])) len--;
    else break;
  if (len) strncpy(desired_key,buf,len);
  desired_key[len]='\0';
  if (buf[count]!='=') return 0;
  count++;
  while (buf[count])
    if (isspace(buf[count])) count++;
    else break;
  len=strlen(buf+count);
  while (len)
    if (isspace(buf[count+len-1])) len--;
    else break;
  if (len) strncpy(desired_val,buf+count,len);
  desired_val[len]='\0';
  return 1;
}

int read_ini(char *filename,
             struct net_parms *port,
             char **loadpath,char **savepath,char **panicpath,
             int *do_single,int *detach) {
  FILE *infile;
  char buf[1024],key[1024],val[1024];
  int len,last_had_nl,line_count,count;
  static char ini_load[1024];
  static char ini_save[1024];
  static char ini_panic[1024];
  static char ini_filesystem[1024];
  static char ini_syslog[1024];
  static char ini_xlog[1024];
  static char ini_tmpdb[1024];
  static char ini_title[1024];

  if (!(infile=fopen(filename,"r"))) return -1;
  last_had_nl=1;
  while (fgets(buf,1024,infile)) {
    if (!last_had_nl) {
      len=strlen(buf);
      if (buf[len-1]=='\n') { buf[--len]='\0'; last_had_nl=1; }
      if (buf[len-1]=='\r') buf[--len]='\0';
      continue;
    }
    ++line_count;
    len=strlen(buf);
    if (buf[len-1]=='\n') { buf[--len]='\0'; last_had_nl=1; }
    if (buf[len-1]=='\r') buf[--len]='\0';
    if (buf[0]!='#') {
      count=0;
      while (buf[count] && isspace(buf[count])) count++;
      if (buf[count]) {
        if (split_key_val(buf+count,key,val)) {
          if (!strcmp(key,"load")) {
            strcpy(ini_load,val);
            *loadpath=ini_load;
          } else if (!strcmp(key,"save")) {
            strcpy(ini_save,val);
            *savepath=ini_save;
          } else if (!strcmp(key,"panic")) {
            strcpy(ini_panic,val);
            *panicpath=ini_panic;
          } else if (!strcmp(key,"filesystem")) {
            strcpy(ini_filesystem,val);
            fs_path=ini_filesystem;
	      } else if (!strcmp(key,"syslog")) {
            strcpy(ini_syslog,val);
            syslog_name=ini_syslog;
          } else if (!strcmp(key,"title")) {
		    strcpy(ini_title,val);
			window_title=ini_title;
          } else if (!strcmp(key,"xlog")) {
            strcpy(ini_xlog,val);
            transact_log_name=ini_xlog;
          } else if (!strcmp(key,"xlogsize"))
            transact_log_size=atol(val);
          else if (!strcmp(key,"tmpdb")) {
            strcpy(ini_tmpdb,val);
            tmpdb_name=ini_tmpdb;
          } else if (!strcmp(key,"protocol")) {
            if (!strcmp(val,"tcp")) port->protocol=CI_PROTOCOL_TCP;
            else if (!strcmp(val,"ipx")) port->protocol=CI_PROTOCOL_IPX;
            else if (!strcmp(val,"netbios"))
              port->protocol=CI_PROTOCOL_NETBIOS;
            else return line_count;
          } else if (!strcmp(key,"port"))
            port->tcp_port=atoi(val);
          else if (!strcmp(key,"ipxnet"))
            port->ipx_net=STRTOUL(val,NULL,16);
          else if (!strcmp(key,"ipxnode")) {
            if (convert_to_6byte(val,&(port->ipx_node[0]))) return line_count;
          } else if (!strcmp(key,"ipxsocket"))
            port->ipx_socket=STRTOUL(val,NULL,16);
          else if (!strcmp(key,"node")) {
            int len;

            len=strlen(val);
            strncpy(&(port->netbios_node[0]),val,15);
            while (len<15) port->netbios_node[len++]=' ';
          } else if (!strcmp(key,"nbport"))
            port->netbios_port=atoi(val);
          else {
            fclose(infile);
            return line_count;
          }
        } else {
          if (!strcmp(key,"detach")) *detach=1;
          else if (!strcmp(key,"multi")) *do_single=MULTI;
          else if (!strcmp(key,"single")) *do_single=SINGLE;
          else {
            fclose(infile);
            return line_count;
          }
        }
      }
    }
  }
  fclose(infile);
  return 0;
}

int set_time_offset() {
  struct tm t1995,utc,*tmp_tm;
  time_t local1995,tmp1,tmp2;
  long utc_offset;

  if ((tmp1=time(NULL))==(-1)) return 1;
  if (!(tmp_tm=gmtime(&tmp1))) return 1;
  utc=*tmp_tm;
  if ((tmp2=mktime(&utc))==(-1)) return 1;
  utc_offset=(long) tmp2-tmp1;
  t1995.tm_sec=0;
  t1995.tm_min=0;
  t1995.tm_hour=0;
  t1995.tm_mday=1;
  t1995.tm_mon=0;
  t1995.tm_year=95;
  t1995.tm_wday=0;
  t1995.tm_yday=0;
  t1995.tm_isdst=0;
  if ((local1995=mktime(&t1995))==(-1)) return 1;
  time_offset=((long) local1995)-utc_offset;
  return 0;
}

int main(int argc, char *argv[]) {
  int count,do_single,do_create,detach;
  char *loadpath,*savepath,*panicpath,*arg,*inifile;
  signed long loop;
  int retval;
  struct object *obj,*tmpobj;
  struct var tmp;
  struct var_stack *rts;
  struct fns *func;
#ifdef USE_WINDOWS
  char errbuf[1024];
  FILE *testfile;
#endif /* USE_WINDOWS */

  window_title=NETCI_NAME;
  if (set_time_offset()) {
#ifdef USE_WINDOWS
    ConstructTitle();
    MessageBox(hwnd,"Couldn't set time offset","Error",MB_OK | MB_ICONSTOP);
#else /* USE_WINDOWS */
    fprintf(stderr,"%s: couldn't set time offset\n",argv[0]);
#endif /* USE_WINDOWS */
    exit(0);
  }
  fs_path=NULL;
  syslog_name=NULL;
  transact_log_name=NULL;
  tmpdb_name=NULL;
  transact_log_size=0;
  detach=0;
  port.protocol=DEFAULT_PROTOCOL;
  port.tcp_port=TCP_PORT;
  port.ipx_net=IPX_NET;
  if (convert_to_6byte(IPX_NODE,&(port.ipx_node[0])))
    convert_to_6byte("000000000001",&(port.ipx_node[0]));
  port.ipx_socket=IPX_SOCKET;
  strncpy(port.netbios_node,NETBIOS_NODE,15);
  count=strlen(NETBIOS_NODE);
  while (count<15) port.netbios_node[count++]=' ';
  port.netbios_port=NETBIOS_PORT;
  count=1;
  do_create=0;
  do_single=DEFAULT_INTERFACE;
  loadpath=NULL;
  savepath=NULL;
  panicpath=NULL;
  inifile=NULL;
  noisy=0;
  if (argc==2)
    if (*(argv[1])!='-') inifile=argv[1];
  if (inifile) {
    if ((retval=read_ini(inifile,&port,&loadpath,&savepath,
                                    &panicpath,&do_single,&detach))==(-1)) {
#ifdef USE_WINDOWS
      ConstructTitle();
      sprintf(errbuf,"Couldn't read %s",inifile);
      MessageBox(hwnd,errbuf,"Error",MB_OK | MB_ICONSTOP);
#else /* USE_WINDOWS */
      fprintf(stderr,"%s: couldn't read %s\n",argv[0],inifile);
#endif /* USE_WINDOWS */
      exit(0);
    }
  } else retval=read_ini(INIFILE,&port,&loadpath,&savepath,&panicpath,
                         &do_single,&detach);
  if (retval>0) {
#ifdef USE_WINDOWS
    ConstructTitle();
    sprintf(errbuf,"Error parsing %s at line #%ld",
	        (inifile?inifile:INIFILE),(long) retval);
    MessageBox(hwnd,errbuf,"Error",MB_OK | MB_ICONSTOP);
#else /* USE_WINDOWS */
    fprintf(stderr,"%s: error parsing %s at line #%ld\n",argv[0],
            (inifile?inifile:INIFILE),(long) retval);
#endif /* USE_WINDOWS */
    exit(0);
  }
  if (!inifile) while (count<argc) {
    arg=argv[count];
    if (!strcmp(arg,"-noisy"))
      noisy=1;
    else if (!strcmp(arg,"-detach"))
      detach=1;
    else if (!strncmp(arg,"-load=",6))
      loadpath=&(arg[6]);
    else if (!strncmp(arg,"-save=",6))
      savepath=&(arg[6]);
    else if (!strncmp(arg,"-panic=",7))
      panicpath=&(arg[7]);
    else if (!strncmp(arg,"-title=",7))
      window_title=&(arg[7]);
    else if (!strncmp(arg,"-filesystem=",12))
      fs_path=&(arg[12]);
    else if (!strncmp(arg,"-syslog=",8))
      syslog_name=&(arg[8]);
    else if (!strncmp(arg,"-xlog=",6))
      transact_log_name=&(arg[6]);
    else if (!strncmp(arg,"-tmpdb=",7))
      tmpdb_name=&(arg[7]);
    else if (!strcmp(arg,"-create"))
      do_create=1;
    else if (!strcmp(arg,"-single"))
      do_single=SINGLE;
    else if (!strcmp(arg,"-multi"))
      do_single=MULTI;
    else if (!strncmp(arg,"-port=",6))
      port.tcp_port=atoi(&(arg[6]));
    else if (!strncmp(arg,"-xlogsize=",10))
      transact_log_size=atol(&(arg[10]));
    else if (!strcmp(arg,"-protocol=tcp"))
      port.protocol=CI_PROTOCOL_TCP;
    else if (!strcmp(arg,"-protocol=ipx"))
      port.protocol=CI_PROTOCOL_IPX;
    else if (!strcmp(arg,"-protocol=netbios"))
      port.protocol=CI_PROTOCOL_NETBIOS;
    else if (!strncmp(arg,"-ipxnet=",8))
      port.ipx_net=STRTOUL(&(arg[8]),NULL,16);
    else if (!strncmp(arg,"-ipxnode=",9)) {
      if (convert_to_6byte(&(arg[9]),&(port.ipx_node[0]))) {
#ifdef USE_WINDOWS
        ConstructTitle();
        MessageBox(hwnd,"Bad IPX node","Error",MB_OK | MB_ICONSTOP);
#else /* USE_WINDOWS */
        fprintf(stderr,"%s: bad ipx node\n",argv[0]);
#endif /* USE_WINDOWS */
        exit(0);
      }
    } else if (!strncmp(arg,"-ipxsocket=",11))
      port.ipx_socket=STRTOUL(&(arg[11]),NULL,16);
    else if (!strncmp(arg,"-node=",6)) {
      int len;

      len=strlen(&(arg[6]));
      strncpy(&(port.netbios_node[0]),&(arg[6]),15);
      while (len<15) port.netbios_node[len++]=' ';
    } else if (!strncmp(arg,"-nbport=",8)) {
      port.netbios_port=atoi(&(arg[8]));
#ifndef USE_WINDOWS
    } else if (!strcmp(arg,"-version")) {
      fprintf(stderr,"\n");
      fprintf(stderr,"%s: version %d.%d.%d\n",
              get_base_file_name(argv[0]),
              (int) (CI_VERSION/10000),
              (int) ((CI_VERSION%10000)/100),
              (int) (CI_VERSION%100));
      fprintf(stderr,"\n");
      fprintf(stderr,"Copyright (C) November 2021 by Kevin Morgan\n");
      fprintf(stderr,"E-Mail as of November 2021: kevin@limping.ninja\n");
      fprintf(stderr,"Github: https://github.com/LimpingNinja/NetCI\n");  
      fprintf(stderr,"Compilation date: %s %s\n",__TIME__,__DATE__);
#ifdef USE_GCC
      fprintf(stderr,"Compiled by gcc\n");
#endif /* USE_GCC */
#ifdef USE_CC
      fprintf(stderr,"Compiled by cc -ansi\n");
#endif /* USE_CC */
#ifdef USE_NSL
      fprintf(stderr,"Linked with nsl library\n");
#endif /* USE_NSL */
#ifdef USE_SOCKET
      fprintf(stderr,"Linked with socket library\n");
#endif /* USE_SOCKET */
#ifdef USE_SEEK_SET
      fprintf(stderr,"Assumed SEEK_SET to be zero\n");
#endif /* USE_SEEK_SET */
#ifdef USE_POSIX
      fprintf(stderr,"Compiled for POSIX system\n");
#endif /* USE_POSIX */
#ifdef USE_BSD
      fprintf(stderr,"Compiled for BSD system\n");
#endif /* USE_BSD */
#ifdef USE_LINUX
      fprintf(stderr,"Compiled for Linux system\n");
#endif /* USE_LINUX */
#ifdef USE_LINUX_IPX
      fprintf(stderr,"Compiled with Linux IPX support\n");
#endif /* USE_LINUX_IPX */
#ifdef DEBUG
      fprintf(stderr,"Compiled with debug settings\n");
#endif /* DEBUG */
      fprintf(stderr,"\n");
      exit(0);
#endif /* !USE_WINDOWS */
    } else {
#ifdef USE_WINDOWS
      ConstructTitle();
      MessageBox(hwnd,"Bad command line arguments","Error",MB_OK |
	             MB_ICONSTOP);
#else /* USE_WINDOWS */
      fprintf(stderr,"usage: %s inifile\n"
              "       %s [-load=filename] [-save=filename] [-panic="
              "filename]\n             [-filesystem=directory] "
              "[-syslog=filename] [-xlog=filename]\n             "
              "[-xlogsize=number] [-tmpdb=filename] [-create] [-multi] "
              "[-single]\n             [-detach] [-noisy] [-version] "
              "[-protocol=tcp|ipx|netbios]\n             [-port=number] "
              "[-ipxnet=hex] [-ipxnode=hex] [-ipxsocket=hex]\n"
              "             [-node=name] [-nbport=number]\n",
              argv[0],argv[0]);
#endif /* USE_WINDOWS */
      exit(0);
    }
    count++;
  }
  if (!fs_path) fs_path=FS_PATH;
  if (!syslog_name) syslog_name=SYSLOG_NAME;
  if (!transact_log_name) transact_log_name=TRANSACT_LOG_NAME;
  if (!tmpdb_name) tmpdb_name=TMPDB_NAME;
#ifdef USE_WINDOWS
  ConstructTitle();
#endif /* USE_WINDOWS */
  if (transact_log_size<0) {
    logger(LOG_ERROR, " system: xlogsize must be positive");
    exit(0);
  }
  if (transact_log_size==0) transact_log_size=TRANSACT_LOG_SIZE;
  if (do_create) detach=0;
  logger(LOG, " system: starting up");
#ifndef USE_WINDOWS
  if (detach) {
    retval=detach_session();
    if (retval==(-1)) {
      logger(LOG_ERROR, " system: unable to detach session");
      exit(0);
    } else if (retval==1) {
      exit(0);
    }
    noisy=0;
  }
#endif /* !USE_WINDOWS */
  init_globals(loadpath,savepath,panicpath);
  if ((retval=init_interface(&port,do_single))) {
    if (retval==NOSINGLE) {
      logger(LOG_ERROR, " system: single-user mode not supported");
#ifdef USE_WINDOWS
      MessageBox(hwnd,"Single-user mode not supported","Error",
	             MB_OK | MB_ICONSTOP);
#endif /* USE_WINDOWS */
    } else if (retval==NOMULTI) {
      logger(LOG_ERROR, " system: multi-user mode not supported");
#ifdef USE_WINDOWS
      MessageBox(hwnd,"Multi-user mode not supported","Error",
	             MB_OK | MB_ICONSTOP);
#endif /* USE_WINDOWS */
    } else if (retval==PORTINUSE) {
      logger(LOG_ERROR, " system: port in use");
#ifdef USE_WINDOWS
      MessageBox(hwnd,"Port in use","Error",
	             MB_OK | MB_ICONSTOP);
#endif /* USE_WINDOWS */
    } else if (retval==NOSOCKET) {
      logger(LOG_ERROR, " system: couldn't create socket");
#ifdef USE_WINDOWS
      MessageBox(hwnd,"Couldn't create socket","Error",
	             MB_OK | MB_ICONSTOP);
#endif /* USE_WINDOWS */
    } else if (retval==NOPROT) {
      logger(LOG_ERROR, " system: network protocol not supported");
#ifdef USE_WINDOWS
      MessageBox(hwnd,"Network protocol not supported","Error",
	             MB_OK | MB_ICONSTOP);
#endif /* USE_WINDOWS */
    } else {
      logger(LOG_ERROR, " system: unspecified interface initialization error");
#ifdef USE_WINDOWS
      MessageBox(hwnd,"Unspecified interface initialization error","Error",
	             MB_OK | MB_ICONSTOP);
#endif /* USE_WINDOWS */
	}
    exit(0);
  }
#ifdef USE_WINDOWS
  if (!do_create)
    if (!(testfile=fopen(loadpath,"r"))) {
      sprintf(errbuf,"Unable to load database %s. Would you like to "
		             "create a new database to be saved as %s?",
                     loadpath,savepath);
	  if ((testfile=fopen(savepath,"r"))) {
		fclose(testfile);
        strcat(errbuf,"\r\nWARNING: Existing database ");
		strcat(errbuf,savepath);
		strcat(errbuf," will be overwritten");
	  }
	  if (MessageBox(hwnd,errbuf,"Create Database",MB_ICONQUESTION |
	                 MB_YESNO)==IDYES)
        do_create=1;
      else {
        logger(LOG, "console: database creation refused");
        exit(0);
      }
    } else fclose(testfile);
#endif /* USE_WINDOWS */
  if (do_create) {
    if (create_db()) {
      logger(LOG_ERROR, " system: database creation failed");
      shutdown_interface();
#ifdef USE_WINDOWS
      MessageBox(hwnd,"Database creation failed","Error",MB_OK |
                 MB_ICONSTOP);
#endif /* USE_WINDOWS */
      exit(0);
    } else {
      logger(LOG, " system: database creation complete");
      if (save_db(save_name)) {
        logger(LOG_ERROR, " system: save failed");
		shutdown_interface();
#ifdef USE_WINDOWS
		MessageBox(hwnd,"Save failed","Error",MB_OK | MB_ICONSTOP);
#endif /* USE_WINDOWS */
        exit(0);
	  }	else {
#ifndef USE_WINDOWS
        logger(LOG, " system: shutting down");
	    shutdown_interface();
		exit(0);
#endif /* !USE_WINDOWS */
	  }
    }
  } else {
    if (init_db()) {
      logger(LOG_ERROR, " system: database initialization failed");
      shutdown_interface();
#ifdef USE_WINDOWS
      MessageBox(hwnd,"Database initialization failed","Error",
	             MB_OK | MB_ICONSTOP);
#endif /* USE_WINDOWS */
      exit(0);
    }
  }
  logger(LOG, " system: startup complete");
  loop=0;
  now_time=time2int(time(NULL));
  tmp.type=NUM_ARGS;
  tmp.value.num=0;
  while (loop<db_top) {
    obj=ref_to_obj(loop);
    if (obj) {
      if (obj->flags & CONNECTED) {
        obj->flags&=~CONNECTED;
        rts=NULL;
        func=find_function("disconnect",obj,&tmpobj);

#ifdef CYCLE_HARD_MAX
        hard_cycles=0;
#endif /* CYCLE_HARD_MAX */

#ifdef CYCLE_SOFT_MAX
        soft_cycles=0;
#endif /* CYCLE_SOFT_MAX */

        if (func) {
          push(&tmp,&rts);
          interp(NULL,tmpobj,NULL,&rts,func);
          free_stack(&rts);
        }
        handle_destruct();
      }
    }
    handle_destruct();
    loop++;
  }
  handle_input();
  logger(LOG_ERROR, " system: return from handle_input()");
  shutdown_interface();
  exit(0);
  return 0;
}
