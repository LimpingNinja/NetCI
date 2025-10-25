/* winmain.c */

#define CMPLNG_INTRFCE

#include "config.h"
#include <winsock.h>
#include "object.h"
#include "interp.h"
#include "intrface.h"
#include "cache.h"
#include "constrct.h"
#include "clearq.h"
#include "globals.h"
#include "dbhandle.h"
#include "file.h"
#include "instr.h"
#include "winmain.h"
#include "resource.h"

int main(int argc, char *argv[]);

#define TIMER_ID 1
#define DIALOG_TIMER_ID 2
#define MAX_OUTPUTWIN_SIZE 100000
#define WSA_ACTIVITY (WM_USER+1)

#define SelectRequestReturn() (SelectReturnRequest=TRUE)
#define SelectClearRequestReturn() (SelectReturnRequest=FALSE)
#define SelectReturnRequested() (SelectReturnRequest)

struct SFDL {
  SOCKET fd;
  int is_read;
  int is_write;
  int is_except;
  struct SFDL *next;
};

struct SEL {
  SOCKET fd;
  int event;
  struct SEL *next;
};

struct SWA {
  SOCKET fd;
  int mode;
  int events;
  struct SWA *next;
};

extern char *scall_array[NUM_SCALLS];
extern struct connlist_s *connlist;
extern int num_conns;
extern struct net_parms port;

typedef struct {
  BYTE bMajor;
  BYTE bMinor;
  WORD wBuildNumber;
  BOOL fDebug;
} WIN32SINFO, far *LPWIN32SINFO;

HWND hwnd,outputwin;
HANDLE hInst;
HFONT FWFont;
HMENU mainmenu;
WSADATA WinSockData;
DWORD WinVersion;
int is_paused,is_saving;
int already_shutdown,dmode,autoload;
char temp_buf[MAX_STR_LEN],bcast_buf[MAX_STR_LEN];
int SelectTimeOutHasOccurred,SelectTimerIsSet;
int DialogTimerID,SelectTimerID;
struct SFDL *SelectFDList;
struct SEL *SelectEventList;
struct SWA *SelectWSAActivated;
int SelectReturnRequest;
long DefaultRefno;
long DefaultVar;
struct file_entry *DefaultFS;
struct proto *DefaultProto;
struct fns *DefaultFunc;
char *OperName[NUM_OPERS]={",","=","+=","-=","*=","/=","%=","&=","^=","|=",
                           "<<=",">>=","?","||","&&","|","^","&","==","!=",
                           "<","<=",">",">=","<<",">>","+","-","*","/","%",
                           "!","~","++ (Post)","++ (Pre)","-- (Post)",
                           "-- (Pre)","- (Unary)" };

void ConstructTitle() {
  strcpy(temp_buf,window_title);
  if (dmode)
    strcat(temp_buf," (debugging)");
  if (is_paused)
    strcat(temp_buf," (paused)");
  if (is_saving)
    strcat(temp_buf," (saving)");
  SetWindowText(hwnd,temp_buf);
}

void AddPausedToTitle() {
  is_paused++;
  ConstructTitle();
}

void RemovePausedFromTitle() {
  is_paused--;
  if (is_paused<0) is_paused=0;
  ConstructTitle();
}

void AddSavingToTitle() {
  is_saving=TRUE;
  ConstructTitle();
}

void RemoveSavingFromTitle() {
  is_saving=FALSE;
  ConstructTitle();
}

BOOL CenterWindow(HWND hwndChild, HWND hwndParent) {
  RECT rChild,rParent;
  int wChild,hChild,wParent,hParent;
  int wScreen,hScreen, xNew,yNew;
  HDC hdc;

  GetWindowRect(hwndChild,&rChild);
  wChild=rChild.right-rChild.left;
  hChild=rChild.bottom-rChild.top;
  GetWindowRect (hwndParent, &rParent);
  wParent=rParent.right-rParent.left;
  hParent=rParent.bottom-rParent.top;
  hdc=GetDC(hwndChild);
  wScreen=GetDeviceCaps(hdc,HORZRES);
  hScreen=GetDeviceCaps(hdc,VERTRES);
  ReleaseDC(hwndChild,hdc);
  xNew=rParent.left+((wParent-wChild)/2);
  if (xNew<0) {
    xNew=0;
  } else if ((xNew+wChild)>wScreen) {
    xNew = wScreen-wChild;
  }
  yNew=rParent.top+((hParent-hChild)/2);
  if (yNew<0) {
    yNew=0;
  } else if ((yNew+hChild)>hScreen) {
    yNew=hScreen-hChild;
  }
  return SetWindowPos(hwndChild,NULL,xNew,yNew,0,0,SWP_NOSIZE | SWP_NOZORDER);
}

void AddText(char *buf) {
  int len,loop,new_len,len_in_window;
  char *buf2;

  if (!outputwin) return;
  len=strlen(buf);
  loop=0;
  new_len=0;
  while (loop<len) {
    if (buf[loop]=='\n') new_len++;
    if (buf[loop]=='\r')
      if (buf[loop+1]=='\n') {
        loop++;
        new_len++;
      } else
        new_len++;
    loop++;
    new_len++;
  }
  buf2=MALLOC(new_len+1);
  loop=0;
  new_len=0;
  while (loop<len) {
    if (buf[loop]=='\n')
      buf2[new_len++]='\r';
    if (buf[loop]=='\r')
      if (buf[loop+1]!='\n') {
        buf2[new_len++]='\r';
        buf2[new_len++]='\n';
        loop++;
        continue;
      } else {
        buf2[new_len++]==buf[loop++];
        buf2[new_len++]==buf[loop++];
        continue;
      }
    buf2[new_len++]=buf[loop++];
  }
  buf2[new_len]='\0';
  len=new_len;
  len_in_window=GetWindowTextLength(outputwin);
  if (len_in_window+len+1>MAX_OUTPUTWIN_SIZE) {
    SendMessage(outputwin,EM_SETSEL,(WPARAM) 0,(LPARAM) len_in_window/2);
    SendMessage(outputwin,EM_REPLACESEL,(WPARAM) 0,(LPARAM) "");
  }
  len_in_window=GetWindowTextLength(outputwin);
  SendMessage(outputwin,EM_SETSEL,(WPARAM) len_in_window,(LPARAM) len_in_window);
  SendMessage(outputwin,EM_REPLACESEL,(WPARAM) 0,(LPARAM) buf2);
  SendMessage(outputwin,EM_SCROLLCARET,0,0);
  FREE(buf2);
}

void console_save() {
  logger(LOG, "console: attempting save");
  if (save_db(save_name))
    logger(LOG_ERROR, "console: save failed");
  else
    logger(LOG, "console: save complete");
}

void console_panic() {
  if (already_shutdown) return;
  already_shutdown=TRUE;
  logger(LOG, "console: attempting panic");
  if (save_db(panic_name)) {
    logger(LOG_ERROR, "console: panic failed");
    logger(LOG_ERROR, "console: system closing with possible data base corruption");
    shutdown_interface();
  } else {
    shutdown_interface();
    logger(LOG, "console: panic complete");
  }
}

void console_shutdown() {
  if (already_shutdown) return;
  already_shutdown=TRUE;
  logger(LOG, "console: attempting shutdown");
  if (save_db(save_name)) {
    logger(LOG_ERROR, "console: shutdown failed");
    logger(LOG, "console: automatically attempting panic");
    if (save_db(panic_name)) {
      logger(LOG_ERROR, "console: panic failed");
      logger(LOG_ERROR, "console: system closing with possible data base corruption");
      shutdown_interface();
    } else {
      shutdown_interface();
      logger(LOG, "console: panic complete");
    }
  } else {
    shutdown_interface();
    logger(LOG, "console: shutdown complete");
  }
}

struct SFDL *SelectSearchList(SOCKET fd) {
  struct SFDL *curr;

  curr=SelectFDList;
  while (curr) {
    if (curr->fd==fd) return curr;
    curr=curr->next;
  }
  return NULL;
}

void SelectClearList() {
  struct SFDL *curr,*next;

  curr=SelectFDList;
  while (curr) {
    next=curr->next;
    FREE(curr);
    curr=next;
  }
  SelectFDList=NULL;
}

void SelectCreateList(fd_set *readfds,fd_set *writefds,fd_set *exceptfds) {
  int loop;
  struct SFDL *curr;

  if (SelectFDList) SelectClearList();
  loop=0;
  if (readfds)
    while (loop<readfds->fd_count) {
      if (!(curr=SelectSearchList(readfds->fd_array[loop]))) {
        curr=MALLOC(sizeof(struct SFDL));
        curr->fd=readfds->fd_array[loop];
        curr->is_read=FALSE;
        curr->is_write=FALSE;
        curr->is_except=FALSE;
        curr->next=SelectFDList;
        SelectFDList=curr;
      }
      curr->is_read=TRUE;
      loop++;
    }
  loop=0;
  if (writefds)
    while (loop<writefds->fd_count) {
      if (!(curr=SelectSearchList(writefds->fd_array[loop]))) {
        curr=MALLOC(sizeof(struct SFDL));
        curr->fd=writefds->fd_array[loop];
        curr->is_read=FALSE;
        curr->is_write=FALSE;
        curr->is_except=FALSE;
        curr->next=SelectFDList;
        SelectFDList=curr;
      }
      curr->is_write=TRUE;
      loop++;
    }
  loop=0;
  if (exceptfds)
    while (loop<exceptfds->fd_count) {
      if (!(curr=SelectSearchList(exceptfds->fd_array[loop]))) {
        curr=MALLOC(sizeof(struct SFDL));
        curr->fd=exceptfds->fd_array[loop];
        curr->is_read=FALSE;
        curr->is_write=FALSE;
        curr->is_except=FALSE;
        curr->next=SelectFDList;
        SelectFDList=curr;
      }
      curr->is_except=TRUE;
      loop++;
    }
}

struct SWA *SelectSearchSWA(SOCKET fd) {
  struct SWA *curr;

  curr=SelectWSAActivated;
  while (curr) {
    if (curr->fd==fd) return curr;
    curr=curr->next;
  }
  return NULL;
}

void SelectKillWriteMsgs() {
  struct SEL *prev_sel,*curr_sel,*temp_sel;

  prev_sel=NULL;
  curr_sel=SelectEventList;
  while (curr_sel) {
    if (curr_sel->event==FD_WRITE) {
      if (prev_sel) prev_sel->next=curr_sel->next;
      else SelectEventList=curr_sel->next;
      temp_sel=curr_sel->next;
      FREE(curr_sel);
      curr_sel=temp_sel;
    } else {
      prev_sel=curr_sel;
      curr_sel=curr_sel->next;
    }
  }
}

void SelectWSAPrepare(SOCKET fd, int mode) {
  struct SWA *new;
  struct SEL *prev_sel,*curr_sel,*temp_sel;

  new=MALLOC(sizeof(struct SWA));
  new->fd=fd;
  new->mode=mode;
  new->events=0;
  new->next=SelectWSAActivated;
  SelectWSAActivated=new;
  WSAAsyncSelect(fd,hwnd,WSA_ACTIVITY,0);
  prev_sel=NULL;
  curr_sel=SelectEventList;
  while (curr_sel) {
    if (curr_sel->fd==fd) {
      if (prev_sel) prev_sel->next=curr_sel->next;
      else SelectEventList=curr_sel->next;
      temp_sel=curr_sel->next;
      FREE(curr_sel);
      curr_sel=temp_sel;
    } else {
      prev_sel=curr_sel;
      curr_sel=curr_sel->next;
    }
  }
}

void SelectWSARemove(SOCKET fd) {
  struct SWA *curr_swa,*prev_swa;
  struct SEL *curr_sel,*prev_sel,*temp_sel;

  curr_swa=SelectSearchSWA(fd);
  if (curr_swa)
    if (curr_swa->events)
      WSAAsyncSelect(fd,hwnd,0,0);
  prev_swa=NULL;
  curr_swa=SelectWSAActivated;
  while (curr_swa) {
    if (curr_swa->fd==fd) {
      if (prev_swa) prev_swa->next=curr_swa->next;
      else SelectWSAActivated=curr_swa->next;
      FREE(curr_swa);
      break;
    }
    prev_swa=curr_swa;
    curr_swa=curr_swa->next;
  }
  prev_sel=NULL;
  curr_sel=SelectEventList;
  while (curr_sel) {
    if (curr_sel->fd==fd) {
      if (prev_sel) prev_sel->next=curr_sel->next;
      else SelectEventList=curr_sel->next;
      temp_sel=curr_sel->next;
      FREE(curr_sel);
      curr_sel=temp_sel;
    } else {
      prev_sel=curr_sel;
      curr_sel=curr_sel->next;
    }
  }
  shutdown(fd,2);
}

void SelectSetTimer(struct timeval *timeout) {
  int milliseconds;

  SelectTimeOutHasOccurred=FALSE;
  if (timeout) {
    milliseconds=(1000*(timeout->tv_sec))+((timeout->tv_usec)/1000);
    if (milliseconds<=0) milliseconds=1;
    SelectTimerID=SetTimer(hwnd,TIMER_ID,milliseconds,NULL);
    SelectTimerIsSet=TRUE;
  }
}

int SelectTimedOut() {
  return SelectTimeOutHasOccurred;
}

void SelectClearTimer() {
  if (SelectTimerIsSet) {
    KillTimer(hwnd,SelectTimerID);
    SelectTimerIsSet=FALSE;
  }
  SelectTimeOutHasOccurred=FALSE;
}

int SelectActivityOccurred(fd_set *readfds, fd_set *writefds, fd_set *exceptfds) {
  struct SEL *curr,*prev;
  struct SFDL *fd_mode;

  prev=NULL;
  curr=SelectEventList;
  while (curr) {
    if (fd_mode=SelectSearchList(curr->fd)) {
      if ((curr->event==FD_READ && fd_mode->is_read) ||
          (curr->event==FD_ACCEPT && fd_mode->is_read) ||
          (curr->event==FD_WRITE && fd_mode->is_write) ||
          (curr->event==FD_CLOSE && fd_mode->is_except)) {
        if ((curr->event==FD_READ || curr->event==FD_ACCEPT) && readfds) {
          FD_SET(curr->fd,readfds);
        } else if (curr->event==FD_WRITE && writefds) {
          FD_SET(curr->fd,writefds);
        } else if (curr->event==FD_CLOSE && exceptfds) {
          FD_SET(curr->fd,exceptfds);
        }
        if (prev) prev->next=curr->next;
        else SelectEventList=curr->next;
        FREE(curr);
        return TRUE;
      }
    }
    prev=curr;
    curr=curr->next;
  }
  return FALSE;
}

void SelectTimerReceived() {
  SelectTimeOutHasOccurred=TRUE;
  KillTimer(hwnd,SelectTimerID);
  SelectTimerIsSet=FALSE;
}

void SelectWSAActivityReceived(SOCKET fd, int event) {
  struct SEL *curr,*prev,*new;
  struct SWA *curr_swa;

  if (!SelectSearchSWA(fd)) return;
  prev=NULL;
  curr=SelectEventList;
  while (curr) {
    prev=curr;
    curr=curr->next;
  }
  new=MALLOC(sizeof(struct SEL));
  new->fd=fd;
  new->event=event;
  new->next=NULL;
  if (prev) prev->next=new;
  else SelectEventList=new;
  curr_swa=SelectSearchSWA(fd);
  if (curr_swa) curr_swa->events=0;
}

void SelectWSAOn() {
  struct SFDL *curr;
  struct SWA *curr_swa;
  int mode;

  curr=SelectFDList;
  while (curr) {
    mode=0;
    curr_swa=SelectSearchSWA(curr->fd);
    if (!curr_swa) continue;
    if (curr->is_read)
      if (curr_swa->mode==SELMODE_LISTEN)
        mode|=FD_ACCEPT;
      else
        mode|=FD_READ;
    if (curr->is_write)
      mode|=FD_WRITE;
    if (curr->is_except)
      mode|=FD_CLOSE;
    WSAAsyncSelect(curr->fd,hwnd,WSA_ACTIVITY,mode);
    curr_swa->events=mode;
    curr=curr->next;
  }
}

int NT_select(int tablesize,fd_set *readfds,fd_set *writefds,fd_set *exceptfds,
              struct timeval *timeout) {
  MSG msg;

  SelectClearRequestReturn();
  SelectKillWriteMsgs();
  SelectCreateList(readfds,writefds,exceptfds);
  if (readfds) FD_ZERO(readfds);
  if (writefds) FD_ZERO(writefds);
  if (exceptfds) FD_ZERO(exceptfds);
  if (SelectActivityOccurred(readfds,writefds,exceptfds)) {
    SelectClearList();
    return 1;
  }
  SelectWSAOn();
  SelectSetTimer(timeout);
  while (GetMessage(&msg,NULL,0,0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    if (!is_paused) {
      if (SelectReturnRequested()) {
        SelectClearTimer();
        SelectClearList();
        return 0;
      }
      if (SelectActivityOccurred(readfds,writefds,exceptfds)) {
        SelectClearTimer();
        SelectClearList();
        return 1;
      }
      if (SelectTimedOut()) {
        SelectClearTimer();
        SelectClearList();
        return 0;
      }
    }
  }
  SelectClearTimer();
  SelectClearList();
  console_shutdown();
  exit(msg.wParam);  
}

LRESULT APIENTRY DialogAbout(HWND hDlg, UINT message, UINT wParam, UINT lParam) {
  char buf2[1024];

  switch (message) {
    case WM_INITDIALOG:
	  sprintf(temp_buf,"About %s",NETCI_NAME);
      SetWindowText(hDlg,temp_buf);
      CenterWindow(hDlg,GetWindow(hDlg,GW_OWNER));
      sprintf(temp_buf,"%s %d.%d.%d",NETCI_NAME,
                                     (int) (CI_VERSION/10000),
                                     (int) ((CI_VERSION%10000)/100),
                                     (int) (CI_VERSION%100));
      SetDlgItemText(hDlg,IDT_ABOUT_VERSION,temp_buf);
	  sprintf(temp_buf,"Copyright (C) June 2021 by Kevin Morgan\n"
                       "E-Mail as of June 2021: kevin@limping.ninja\r\n"
                       "Compilation date: %s %s\r\n",__TIME__,__DATE__);
#ifdef DEBUG
      strcat(temp_buf,"Compiled with debug settings\r\n");
#endif /* DEBUG */
      if (WinVersion & 0x80000000L) {
	    sprintf(buf2,"\nOperating System:\nWindows v%d.%d\n",
		       (int) (WinVersion & 0x000000FFL),
			   (int) ((WinVersion & 0x0000FF00L)>>8));
		strcat(temp_buf,buf2);
	  } else {
		sprintf(buf2,"\nOperating System:\nWindows NT v%d.%d\n",
		       (int) (WinVersion & 0x000000FFL),
			   (int) ((WinVersion & 0x0000FF00L)>>8));
		strcat(temp_buf,buf2);
	  }
	  strcat(temp_buf,WinSockData.szDescription);
      SetDlgItemText(hDlg,IDT_ABOUT_INFO,temp_buf);
      return TRUE;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
          EndDialog(hDlg,1);
          break;
        default:
          return FALSE;
      }
      return TRUE;
    default:
      return FALSE;
  }
}

struct object *RefnoToObject(signed long refno) {
  struct obj_blk *curr;
  struct object *obj;
  signed long count,index;

  if (refno>(db_top-1)) return NULL;
  if (refno<0) return NULL;
  count=refno/OBJ_ALLOC_BLKSIZ;
  index=refno%OBJ_ALLOC_BLKSIZ;
  curr=obj_list;
  while (count--)
    curr=curr->next;
  obj=&(curr->block[index]);
  return obj;
}

long StringToObject(char *objidbuf) {
  struct object *result;
  unsigned long expected_refno;
  char *numbuf,*pathbuf,*buf;
  int breakpoint;

  buf=MALLOC(strlen(objidbuf)+1);
  strcpy(buf,objidbuf);
  breakpoint=0;
  pathbuf=buf;
  numbuf="";
  while (buf[breakpoint]) {
    if (buf[breakpoint]=='#') {
      numbuf=&(buf[breakpoint+1]);
      buf[breakpoint]='\0';
      break;
    }
    breakpoint++;
  }
  if (*numbuf=='\0') {
    result=find_proto(pathbuf);
  } else {
    expected_refno=atol(numbuf);
    result=RefnoToObject(expected_refno);
    if (result)
      if (*pathbuf) {
        if (result->flags & GARBAGE)
          result=NULL;
        if (strcmp(pathbuf,result->parent->pathname))
          result=NULL;
      }
  }
  FREE(buf);
  if (result)
    return result->refno;
  else
    return (-1);
}

LRESULT APIENTRY DialogObjectSelect(HWND hDlg,UINT message,UINT wParam,UINT lParam) {
  switch (message) {
    case WM_INITDIALOG:
      SendMessage(GetDlgItem(hDlg,IDE_OBJECTSELECTION),EM_LIMITTEXT,
                  (WPARAM) MAX_STR_LEN-1,0);
      CenterWindow(hDlg,GetWindow(hDlg,GW_OWNER));
      SetFocus(GetDlgItem(hDlg,IDE_OBJECTSELECTION));
      return TRUE;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDCANCEL:
          EndDialog(hDlg,0);
          break;
        case IDOK:
          GetDlgItemText(hDlg,IDE_OBJECTSELECTION,temp_buf,MAX_STR_LEN);
          DefaultRefno=StringToObject(temp_buf);
          if (DefaultRefno<0) {
            MessageBox(hDlg,"Invalid Object","Alert",MB_OK);
            return TRUE;
          }
          EndDialog(hDlg,1);
          break;
        default:
          return FALSE;
      }
      return TRUE;
    default:
      return FALSE;
  }
}

char *MakeAbsVarName(struct var_tab *gst,long index) {
  struct var_tab *curr_var;
  struct array_size *curr_array;
  long base,size,array_count;
  char *buf,itoa_buf[ITOA_BUFSIZ+3];

  curr_var=gst;
  base=0;
  while (curr_var) {
    base=curr_var->base;
    size=1;
    curr_array=curr_var->array;
    array_count=0;
    while (curr_array) {
      array_count++;
      size=size*curr_array->size;
      curr_array=curr_array->next;
    }
    if ((index>=base) && (index<=(base+size-1))) break;
    curr_var=curr_var->next;
  }
  if (!curr_var) {
    buf=MALLOC(ITOA_BUFSIZ+11);
    sprintf(buf,"Variable #%ld",(long) index);
    return buf;
  }
  buf=MALLOC(strlen(curr_var->name)+(array_count*(ITOA_BUFSIZ+2))+1);
  strcpy(buf,curr_var->name);
  curr_array=curr_var->array;
  while (curr_array) {
    sprintf(itoa_buf,"[%ld]",(long) curr_array->size);
    strcat(buf,itoa_buf);
    curr_array=curr_array->next;
  }
  return buf;
}

char *MakeVarName(struct var_tab *gst,long index) {
  struct var_tab *curr_var;
  struct array_size *curr_array,*array;
  long base,size,array_count;
  char *buf,itoa_buf[ITOA_BUFSIZ+3];

  curr_var=gst;
  base=0;
  while (curr_var) {
    base=curr_var->base;
    size=1;
    curr_array=curr_var->array;
    array_count=0;
    while (curr_array) {
      array_count++;
      size=size*curr_array->size;
      curr_array=curr_array->next;
    }
    if ((index>=base) && (index<=(base+size-1))) break;
    curr_var=curr_var->next;
  }
  if (!curr_var) {
    buf=MALLOC(ITOA_BUFSIZ+11);
    sprintf(buf,"Variable #%ld",(long) index);
    return buf;
  }
  buf=MALLOC(strlen(curr_var->name)+(array_count*(ITOA_BUFSIZ+2))+1);
  strcpy(buf,curr_var->name);
  array=curr_var->array;
  index=index-curr_var->base;
  while (array) {
    size=1;
    curr_array=array->next;
    while (curr_array) {
      size=size*curr_array->size;
      curr_array=curr_array->next;
    }
    sprintf(itoa_buf,"[%ld]",(long) (index/size));
    strcat(buf,itoa_buf);
    array=array->next;
  }
  return buf;
}

long GetVarSize(long index,struct var_tab *gst) {
  struct var_tab *curr_var;
  struct array_size *curr_array;
  long size;

  curr_var=gst;
  while (curr_var) {
    size=1;
    curr_array=curr_var->array;
    while (curr_array) {
      size*=curr_array->size;
      curr_array=curr_array->next;
    }
    if ((index>=(curr_var->base)) && (index<(curr_var->base+size))) return size;
    curr_var=curr_var->next;
  }
  return 1;
}

char *MakeInstr(struct fns *func,long index,struct var_tab *gst) {
  char *buf,*p1,*p2,*varbuf;
  long len;

  switch (func->code[index].type) {
    case INTEGER:
      buf=MALLOC((2*ITOA_BUFSIZ)+8);
      sprintf(buf,"%ld:\tPUSH %ld",(long) index,
              (long) func->code[index].value.integer);
      return buf;
      break;
    case STRING:
      p1=func->code[index].value.string;
      len=0;
      while (*p1) {
        if (*p1=='\t' || *p1=='\n' || *p1=='\r' || *p1=='\f' ||
            *p1=='\b' || *p1=='\v' || *p1=='\a' || *p1=='\\' ||
            *p1=='"') len++;
        len++;
        p1++;
      }
      buf=MALLOC(ITOA_BUFSIZ+len+10);
      sprintf(buf,"%ld:\tPUSH \"",(long) index);
      p1=func->code[index].value.string;
      p2=buf+strlen(buf);
      while (*p1) {
        switch (*p1) {
          case '\n':
            *(p2++)='\\';
            *(p2++)='n';
            break;
          case '\t':
            *(p2++)='\\';
            *(p2++)='t';
            break;
          case '\r':
            *(p2++)='\\';
            *(p2++)='r';
            break;
          case '\a':
            *(p2++)='\\';
            *(p2++)='a';
            break;
          case '\b':
            *(p2++)='\\';
            *(p2++)='b';
            break;
          case '\f':
            *(p2++)='\\';
            *(p2++)='f';
            break;
          case '\v':
            *(p2++)='\\';
            *(p2++)='v';
            break;
          case '\\':
            *(p2++)='\\';
            *(p2++)='\\';
            break;
          case '"':
            *(p2++)='\\';
            *(p2++)='"';
            break;
          default:
            *(p2++)=(*p1);
            break;
        }
        p1++;
      }
      *(p2++)='"';
      *p2='\0';
      return buf;
      break;
    case OBJECT:
      buf=MALLOC(strlen(func->code[index].value.objptr->parent->pathname)+
                 (2*ITOA_BUFSIZ)+9);
      sprintf(buf,"%ld:\tPUSH %s#%ld",(long) index,
              func->code[index].value.objptr->parent->pathname,
              (long) func->code[index].value.objptr->refno);
      return buf;
      break;
    case ASM_INSTR:
      if (func->code[index].value.instruction<NUM_OPERS) {
        buf=MALLOC(strlen(OperName[func->code[index].value.instruction])+
                   ITOA_BUFSIZ+8);
        sprintf(buf,"%ld:\tOPER %s",(long) index,
                OperName[func->code[index].value.instruction]);
      } else if (func->code[index].value.instruction<(NUM_OPERS+NUM_SCALLS)) {
        buf=MALLOC(strlen(scall_array[func->code[index].value.instruction-
                                      NUM_OPERS])+ITOA_BUFSIZ+8);
        sprintf(buf,"%ld:\tSYS  %s",(long) index,
                scall_array[func->code[index].value.instruction-NUM_OPERS]);
      } else {
        buf=MALLOC((2*ITOA_BUFSIZ)+21);
        sprintf(buf,"%ld:\t???? Instruction #%d",(long) index,
                (int) func->code[index].value.instruction);
      }
      return buf;
      break;
    case GLOBAL_L_VALUE:
      if (func->code[index].value.l_value.size==1) {
        varbuf=MakeVarName(gst,func->code[index].value.l_value.ref);
        buf=MALLOC(strlen(varbuf)+ITOA_BUFSIZ+8);
        sprintf(buf,"%ld:\tGLBL %s",(long) index,varbuf);
        FREE(varbuf);
      } else {
        varbuf=MakeVarName(gst,func->code[index].value.l_value.ref);
        buf=MALLOC(strlen(varbuf)+(2*ITOA_BUFSIZ)+14);
        sprintf(buf,"%ld:\tGLBL %s Size=%ld",(long) index,varbuf,
                (long) func->code[index].value.l_value.size);
        FREE(varbuf);
      }
      return buf;
      break;
    case LOCAL_L_VALUE:
      if (func->code[index].value.l_value.size==1) {
        buf=MALLOC((2*ITOA_BUFSIZ)+18);
        sprintf(buf,"%ld:\tLOCL Variable #%ld",(long) index,
                (long) func->code[index].value.l_value.ref);
      } else {
        buf=MALLOC((3*ITOA_BUFSIZ)+24);
        sprintf(buf,"%ld:\tLOCL Variable #%ld Size=%ld",(long) index,
                (long) func->code[index].value.l_value.ref,
                (long) func->code[index].value.l_value.size);
      }
      return buf;
      break;
    case FUNC_CALL:
      buf=MALLOC(ITOA_BUFSIZ+strlen(func->code[index].value.func_call->funcname)+
                 8);
      sprintf(buf,"%ld:\tCALL %s",(long) index,
              func->code[index].value.func_call->funcname);
      return buf;
      break;
    case NUM_ARGS:
      buf=MALLOC((2*ITOA_BUFSIZ)+8);
      sprintf(buf,"%ld:\tNARG %ld",(long) index,(long) func->code[index].value.num);
      return buf;
      break;
    case ARRAY_SIZE:
      buf=MALLOC((2*ITOA_BUFSIZ)+8);
      sprintf(buf,"%ld:\tARSZ %ld",(long) index,(long) func->code[index].value.num);
      return buf;
      break;
    case JUMP:
      buf=MALLOC((2*ITOA_BUFSIZ)+8);
      sprintf(buf,"%ld:\tJUMP %ld",(long) index,(long) func->code[index].value.num);
      return buf;
      break;
    case BRANCH:
      buf=MALLOC((2*ITOA_BUFSIZ)+8);
      sprintf(buf,"%ld:\tBRCH %ld",(long) index,(long) func->code[index].value.num);
      return buf;
      break;
    case NEW_LINE:
      buf=MALLOC(ITOA_BUFSIZ+8);
      sprintf(buf,"%ld:\tNWLN %ld",(long) index,(long) func->code[index].value.num);
      return buf;
      break;
    case RETURN:
      buf=MALLOC(ITOA_BUFSIZ+7);
      sprintf(buf,"%ld:\tRTRN",(long) index);
      return buf;
      break;
    case LOCAL_REF:
      buf=MALLOC(ITOA_BUFSIZ+7);
      sprintf(buf,"%ld:\tLREF",(long) index);
      return buf;
      break;
    case GLOBAL_REF:
      buf=MALLOC(ITOA_BUFSIZ+7);
      sprintf(buf,"%ld:\tGREF",(long) index);
      return buf;
      break;
    case FUNC_NAME:
      buf=MALLOC(ITOA_BUFSIZ+strlen(func->code[index].value.string)+8);
      sprintf(buf,"%ld:\tCALL %s",(long) index,func->code[index].value.string);
      return buf;
      break;
	case EXTERN_FUNC:
	  buf=MALLOC(ITOA_BUFSIZ+strlen(func->code[index].value.string)+8);
	  sprintf(buf,"%ld:\tEFUN %s",(long) index,func->code[index].value.string);
	  return buf;
	  break;
    default:
      buf=MALLOC((2*ITOA_BUFSIZ)+14);
      sprintf(buf,"%ld:\t???? Type #%d",(long) index,
              (int) func->code[index].type);
      return buf;
  }
}

void DisplayFuncData(HWND hDlg,struct proto *p,struct fns *f) {
  long index;
  char *varbuf;

  if (f) {
    SetDlgItemText(hDlg,IDE_FUNCTION,f->funcname);
    sprintf(temp_buf,"%ld",(long) f->num_args);
    SetDlgItemText(hDlg,IDE_NUMARGS,temp_buf);
    sprintf(temp_buf,"%ld",(long) f->num_locals);
    SetDlgItemText(hDlg,IDE_NUMLOCALS,temp_buf);
    if (f->is_static)
      SetDlgItemText(hDlg,IDT_STATICMSG,"Function Is Static");
    else
      SetDlgItemText(hDlg,IDT_STATICMSG,"Function Is Not Static");
    SendMessage(GetDlgItem(hDlg,IDLB_CODE),LB_RESETCONTENT,0,0);
    index=0;
    while (index<f->num_instr) {
      varbuf=MakeInstr(f,index,p->funcs->gst);
      SendMessage(GetDlgItem(hDlg,IDLB_CODE),LB_ADDSTRING,0,(LPARAM) varbuf);
      FREE(varbuf);
      index++;
    }
  } else {
    SetDlgItemText(hDlg,IDE_FUNCTION,"");
    SetDlgItemText(hDlg,IDE_NUMARGS,"");
    SetDlgItemText(hDlg,IDE_NUMLOCALS,"");
    SetDlgItemText(hDlg,IDT_STATICMSG,"");
    SendMessage(GetDlgItem(hDlg,IDLB_CODE),LB_RESETCONTENT,0,0);
  }
}

void DisplayProtoData(HWND hDlg,struct proto *p,struct fns *f) {
  char *buf,*varbuf;
  struct fns *curr_func;
  long index,size;

  SetDlgItemText(hDlg,IDE_PATH,p->pathname);
  sprintf(temp_buf,"#%ld",(long) p->proto_obj->refno);
  SetDlgItemText(hDlg,IDB_OBJECT,temp_buf);
  curr_func=p->funcs->func_list;
  SendMessage(GetDlgItem(hDlg,IDLB_FUNCS),LB_RESETCONTENT,0,0);
  while (curr_func) {
    SendMessage(GetDlgItem(hDlg,IDLB_FUNCS),LB_ADDSTRING,0,
                (LPARAM) curr_func->funcname);
    curr_func=curr_func->next;
  }
  SendMessage(GetDlgItem(hDlg,IDLB_GLOBALS),LB_RESETCONTENT,0,0);
  index=0;
  while (index<p->funcs->num_globals) {
    size=GetVarSize(index,p->funcs->gst);
    varbuf=MakeAbsVarName(p->funcs->gst,index);
    buf=MALLOC(strlen(varbuf)+ITOA_BUFSIZ+3);
    sprintf(buf,"%ld:\t%s",(long) index,varbuf);
    FREE(varbuf);
    SendMessage(GetDlgItem(hDlg,IDLB_GLOBALS),LB_ADDSTRING,0,(LPARAM) buf);
    FREE(buf);
    index+=size;
  }
  DisplayFuncData(hDlg,p,f);
}

LRESULT APIENTRY DialogProto(HWND hDlg,UINT message,UINT wParam,UINT lParam) {
  int index;
  struct fns *temp_func;
  struct proto *temp_proto;
  char *buf;
  int tabs[1];
  LRESULT APIENTRY DialogObject(HWND hDlg,UINT message,UINT wParam,UINT lParam);

  switch (message) {
    case WM_INITDIALOG:
      tabs[0]=8;
      SendMessage(GetDlgItem(hDlg,IDLB_GLOBALS),LB_SETTABSTOPS,(WPARAM) 1,
                  (LPARAM) tabs);
      if (FWFont)
        SendMessage(GetDlgItem(hDlg,IDLB_CODE),WM_SETFONT,(WPARAM) FWFont,
                    MAKELPARAM(FALSE,0));
      CenterWindow(hDlg,GetWindow(hDlg,GW_OWNER));
      DisplayProtoData(hDlg,DefaultProto,DefaultFunc);
      return TRUE;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDB_DONE:
        case IDCANCEL:
          EndDialog(hDlg,1);
          break;
        case IDB_OBJECT:
          if (!DefaultProto) break;
          if (!DefaultProto->proto_obj) break;
          DefaultRefno=DefaultProto->proto_obj->refno;
          temp_proto=DefaultProto;
          temp_func=DefaultFunc;
          DialogBox(hInst,"DLGObject",hDlg,DialogObject);
          DefaultFunc=temp_func;
          DefaultProto=temp_proto;
          DisplayProtoData(hDlg,DefaultProto,DefaultFunc);
          break;
        case IDLB_FUNCS:
          if (HIWORD(wParam)!=LBN_DBLCLK) return FALSE;
          if (!DefaultProto) break;
          index=SendMessage(GetDlgItem(hDlg,IDLB_FUNCS),LB_GETCURSEL,0,0);
          buf=MALLOC(SendMessage(GetDlgItem(hDlg,IDLB_FUNCS),
                     LB_GETTEXTLEN,(WPARAM) index,0)+1);
          SendMessage(GetDlgItem(hDlg,IDLB_FUNCS),LB_GETTEXT,
                      (WPARAM) index,(LPARAM) buf);
          temp_func=find_fns(buf,DefaultProto->proto_obj);
          FREE(buf);
          if (!temp_func) break;
          DefaultFunc=temp_func;
          DisplayFuncData(hDlg,DefaultProto,DefaultFunc); 
          break;
        case IDB_NEXTPROTO:
          if (!DefaultProto) break;
          if (!(DefaultProto->next_proto)) break;
          DefaultProto=DefaultProto->next_proto;
          DefaultFunc=NULL;
          DisplayProtoData(hDlg,DefaultProto,DefaultFunc);
          return TRUE;
        default:
          return FALSE;
      }
      return TRUE;
    default:
      return FALSE;
  }
}

LRESULT APIENTRY DialogProtoSelect(HWND hDlg,UINT message,UINT wParam,UINT lParam) {
  struct proto *curr_proto;
  struct object *obj;
  int index;
  char *buf;

  switch (message) {
    case WM_INITDIALOG:
      CenterWindow(hDlg,GetWindow(hDlg,GW_OWNER));
      curr_proto=ref_to_obj(0)->parent;
      while (curr_proto) {
        SendMessage(GetDlgItem(hDlg,IDLB_PROTOSELECTION),LB_ADDSTRING,0,
                    (LPARAM) curr_proto->pathname);
        curr_proto=curr_proto->next_proto;
      }
      SetFocus(GetDlgItem(hDlg,IDLB_PROTOSELECTION));
      return TRUE;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDCANCEL:
          EndDialog(hDlg,0);
          break;
        case IDOK:
          index=SendMessage(GetDlgItem(hDlg,IDLB_PROTOSELECTION),LB_GETCURSEL,0,0);
          if (index<0) break;
          buf=MALLOC(SendMessage(GetDlgItem(hDlg,IDLB_PROTOSELECTION),
                     LB_GETTEXTLEN,(WPARAM) index,0)+1);
          SendMessage(GetDlgItem(hDlg,IDLB_PROTOSELECTION),LB_GETTEXT,
                      (WPARAM) index,(LPARAM) buf);
          obj=find_proto(buf);
          FREE(buf);
          if (!obj) break;
          DefaultProto=obj->parent;
          EndDialog(hDlg,1);
          break;
        case IDLB_PROTOSELECTION:
          if (HIWORD(wParam)!=LBN_DBLCLK) return FALSE;
          index=SendMessage(GetDlgItem(hDlg,IDLB_PROTOSELECTION),LB_GETCURSEL,0,0);
          if (index<0) break;
          buf=MALLOC(SendMessage(GetDlgItem(hDlg,IDLB_PROTOSELECTION),
                     LB_GETTEXTLEN,(WPARAM) index,0)+1);
          SendMessage(GetDlgItem(hDlg,IDLB_PROTOSELECTION),LB_GETTEXT,
                      (WPARAM) index,(LPARAM) buf);
          obj=find_proto(buf);
          FREE(buf);
          if (!obj) break;
          DefaultProto=obj->parent;
          EndDialog(hDlg,1);
          break;
        default:
          return FALSE;
      }
      return TRUE;
    default:
      return FALSE;
  }
}

int GetNthConn(int num) {
  int loop,count;

  loop=0;
  count=-1;
  while (loop<num_conns) {
    if (connlist[loop].fd!=(-1)) {
      count++;
      if (count==num) return loop;
    }
    loop++;
  }
  return (-1);
}

char *MakeVarData(struct object *obj, long index) {
  char *buf,*p1,*p2;
  long len;

  switch (obj->globals[index].type) {
    case INTEGER:
      buf=MALLOC(ITOA_BUFSIZ+1);
      sprintf(buf,"%ld",(long) obj->globals[index].value.integer);
      return buf;
      break;
    case STRING:
      p1=obj->globals[index].value.string;
      len=0;
      while (*p1) {
        if (*p1=='\t' || *p1=='\n' || *p1=='\r' || *p1=='\f' ||
            *p1=='\b' || *p1=='\v' || *p1=='\a' || *p1=='\\' ||
            *p1=='"') len++;
        len++;
        p1++;
      }
      buf=MALLOC(len+3);
      p1=obj->globals[index].value.string;
      p2=buf;
      *(p2++)='"';
      while (*p1) {
        switch (*p1) {
          case '\n':
            *(p2++)='\\';
            *(p2++)='n';
            break;
          case '\t':
            *(p2++)='\\';
            *(p2++)='t';
            break;
          case '\r':
            *(p2++)='\\';
            *(p2++)='r';
            break;
          case '\a':
            *(p2++)='\\';
            *(p2++)='a';
            break;
          case '\b':
            *(p2++)='\\';
            *(p2++)='b';
            break;
          case '\f':
            *(p2++)='\\';
            *(p2++)='f';
            break;
          case '\v':
            *(p2++)='\\';
            *(p2++)='v';
            break;
          case '\\':
            *(p2++)='\\';
            *(p2++)='\\';
            break;
          case '"':
            *(p2++)='\\';
            *(p2++)='"';
            break;
          default:
            *(p2++)=(*p1);
            break;
        }
        p1++;
      }
      *(p2++)='"';
      *p2='\0';
      return buf;
      break;
    case OBJECT:
      buf=MALLOC(strlen(obj->globals[index].value.objptr->parent->pathname)+
                 ITOA_BUFSIZ+2);
      sprintf(buf,"%s#%ld",obj->globals[index].value.objptr->parent->pathname,
              (long) obj->globals[index].value.objptr->refno);
      return buf;
      break;
    default:
      buf=MALLOC(ITOA_BUFSIZ+16);
      sprintf(buf,"Illegal Type #%d",(int) obj->globals[index].type);
      return buf;
  }
}

void SetObjectEntries(HWND hDlg,long objnum) {
  struct object *obj;
  long loop;
  char *varname,*vardata,*varbuf;
  struct verb *curr_verb;
  struct ref_list *curr_ref;

  obj=RefnoToObject(objnum);
  if (!obj) {
    obj=RefnoToObject(0);
    objnum=0;
  }
  if ((!(obj->flags & GARBAGE)) && autoload)
    load_data(obj);
  sprintf(temp_buf,"#%ld",(long) objnum);
  SetDlgItemText(hDlg,IDB_REFNO,temp_buf);
  if (obj->parent)
    SetDlgItemText(hDlg,IDB_PATH,obj->parent->pathname);
  else
    SetDlgItemText(hDlg,IDB_PATH,"");
  if (obj->devnum!=(-1)) {
    sprintf(temp_buf,"%d",(int) obj->devnum);
    SetDlgItemText(hDlg,IDE_DEVNUM,temp_buf);
    SetDlgItemText(hDlg,IDE_IPADDR,get_devconn(obj));
  } else {
    SetDlgItemText(hDlg,IDE_DEVNUM,"No Device Attached");
    SetDlgItemText(hDlg,IDE_IPADDR,"");
  }
  temp_buf[0]='\0';
  if (obj->flags & CONNECTED) strcat(temp_buf," Connected");
  if (obj->flags & IN_EDITOR) strcat(temp_buf," InEditor");
  if (obj->flags & GARBAGE) strcat(temp_buf," Garbage");
  if (obj->flags & INTERACTIVE) strcat(temp_buf," Interactive");
  if (obj->flags & LOCALVERBS) strcat(temp_buf," LocalVerbs");
  if (obj->flags & PRIV) strcat(temp_buf," Priv");
  if (obj->flags & PROTOTYPE) strcat(temp_buf," Prototype");
  if (obj->flags & RESIDENT) strcat(temp_buf," Resident");
  if (temp_buf[0]) SetDlgItemText(hDlg,IDE_FLAGS,&(temp_buf[1]));
  else SetDlgItemText(hDlg,IDE_FLAGS,"");
  if (obj->parent) {
    sprintf(temp_buf,"#%ld",(long) obj->parent->proto_obj->refno);
    SetDlgItemText(hDlg,IDB_PARENT,temp_buf);
  } else SetDlgItemText(hDlg,IDB_PARENT,"");
  if (obj->next_child) {
    sprintf(temp_buf,"#%ld",(long) obj->next_child->refno);
    SetDlgItemText(hDlg,IDB_NEXTCHILD,temp_buf);
  } else SetDlgItemText(hDlg,IDB_NEXTCHILD,"");
  if (obj->location) {
    sprintf(temp_buf,"%s#%ld",obj->location->parent->pathname,
            (long) obj->location->refno);
    SetDlgItemText(hDlg,IDB_LOCATION,temp_buf);
  } else SetDlgItemText(hDlg,IDB_LOCATION,"");
  if (obj->contents) {
    sprintf(temp_buf,"%s#%ld",obj->contents->parent->pathname,
            (long) obj->contents->refno);
    SetDlgItemText(hDlg,IDB_CONTENTS,temp_buf);
  } else SetDlgItemText(hDlg,IDB_CONTENTS,"");
  if (obj->next_object) {
    if (obj->next_object->parent)
      sprintf(temp_buf,"%s#%ld",obj->next_object->parent->pathname,
              (long) obj->next_object->refno);
    else
      sprintf(temp_buf,"#%ld",(long) obj->next_object->refno);
    SetDlgItemText(hDlg,IDB_NEXTOBJECT,temp_buf);
  } else SetDlgItemText(hDlg,IDB_NEXTOBJECT,"");
  switch (obj->obj_state) {
    case DIRTY:
      SetDlgItemText(hDlg,IDE_CACHESTATUS,"Dirty");
      break;
    case IN_DB:
      sprintf(temp_buf,"In Database at %ld",(long) obj->file_offset);
      SetDlgItemText(hDlg,IDE_CACHESTATUS,temp_buf);
      break;
    case IN_CACHE:
      sprintf(temp_buf,"In Cache File at %ld",(long) obj->file_offset);
      SetDlgItemText(hDlg,IDE_CACHESTATUS,temp_buf);
      break;
    case FROM_DB:
      sprintf(temp_buf,"Loaded From Database at %ld",(long) obj->file_offset);
      SetDlgItemText(hDlg,IDE_CACHESTATUS,temp_buf);
      break;
    case FROM_CACHE:
      sprintf(temp_buf,"Loaded From Cache File at %ld",(long) obj->file_offset);
      SetDlgItemText(hDlg,IDE_CACHESTATUS,temp_buf);
      break;
    default:
      sprintf(temp_buf,"Unknown Status=%d File Offset=%ld",(int) obj->obj_state,
              (long) obj->file_offset);
      SetDlgItemText(hDlg,IDE_CACHESTATUS,temp_buf);
      break;
  }
  SendMessage(GetDlgItem(hDlg,IDLB_VARS),LB_RESETCONTENT,0,0);
  if (obj->flags & RESIDENT) {
    loop=0;
    while (loop<obj->parent->funcs->num_globals) {
      varname=MakeVarName(obj->parent->funcs->gst,loop);
      vardata=MakeVarData(obj,loop);
      varbuf=MALLOC(strlen(varname)+strlen(vardata)+2);
      strcpy(varbuf,varname);
      strcat(varbuf,"=");
      strcat(varbuf,vardata);
      FREE(varname);
      FREE(vardata);
      SendMessage(GetDlgItem(hDlg,IDLB_VARS),LB_ADDSTRING,0,(LPARAM) varbuf);
      FREE(varbuf);
      loop++;
    }
  } else
    if (!(obj->flags & GARBAGE))
      SendMessage(GetDlgItem(hDlg,IDLB_VARS),LB_ADDSTRING,0,
                  (LPARAM) "(Data Not Loaded)");
  SendMessage(GetDlgItem(hDlg,IDLB_VERBS),LB_RESETCONTENT,0,0);
  curr_verb=obj->verb_list;
  while (curr_verb) {
    varbuf=MALLOC(strlen(curr_verb->verb_name)+strlen(curr_verb->function)+8);
    if (curr_verb->is_xverb)
      strcpy(varbuf,"X\t");
    else
      strcpy(varbuf,"\t");
    strcat(varbuf,curr_verb->verb_name);
    strcat(varbuf,"\t");
    strcat(varbuf,curr_verb->function);
    SendMessage(GetDlgItem(hDlg,IDLB_VERBS),LB_ADDSTRING,0,(LPARAM) varbuf);
    FREE(varbuf);
    curr_verb=curr_verb->next;
  }
  SendMessage(GetDlgItem(hDlg,IDLB_REFS),LB_RESETCONTENT,0,0);
  if (obj->flags & RESIDENT) {
    curr_ref=obj->refd_by;
    while (curr_ref) {
      varname=MakeVarName(curr_ref->ref_obj->parent->funcs->gst,curr_ref->ref_num);
      varbuf=MALLOC(strlen(curr_ref->ref_obj->parent->pathname)+
                    ITOA_BUFSIZ+strlen(varname)+3);
      sprintf(varbuf,"%s#%ld\t%s",curr_ref->ref_obj->parent->pathname,
              (long) curr_ref->ref_obj->refno,varname);
      FREE(varname);
      SendMessage(GetDlgItem(hDlg,IDLB_REFS),LB_ADDSTRING,0,(LPARAM) varbuf);
      FREE(varbuf);
      curr_ref=curr_ref->next;
    }
  } else
    if (!(obj->flags & GARBAGE))
      SendMessage(GetDlgItem(hDlg,IDLB_REFS),LB_ADDSTRING,0,
                  (LPARAM) "(Data Not Loaded)");    
}

int MoveObject(struct object *item,struct object *dest) {
  struct object *curr,*prev;

  if (dest) {
    curr=dest;
    while (curr) {
      if (curr==item) {
        return 0;
      }
      curr=curr->location;
    }
  }
  if (item->location) {
    curr=item->location->contents;
    if (curr==item)
      item->location->contents=item->next_object;
    else
      while (curr) {
        prev=curr;
        curr=curr->next_object;
        if (curr==item) {
          prev->next_object=curr->next_object;
          break;
        }
      }
  }
  item->next_object=NULL;
  item->location=dest;
  if (dest) {
    item->next_object=dest->contents;
    dest->contents=item;
  }
  return 1;
}

LRESULT APIENTRY DialogFlags(HWND hDlg,UINT message,UINT wParam,UINT lParam) {
  struct object *obj;

  switch (message) {
    case WM_INITDIALOG:
      obj=RefnoToObject(DefaultRefno);
      if (obj->flags & GARBAGE)
        SendMessage(GetDlgItem(hDlg,IDCB_GARBAGE),BM_SETCHECK,(WPARAM) 1,0);
      if (obj->flags & INTERACTIVE)
        SendMessage(GetDlgItem(hDlg,IDCB_INTERACTIVE),BM_SETCHECK,(WPARAM) 1,0);
      if (obj->flags & CONNECTED)
        SendMessage(GetDlgItem(hDlg,IDCB_CONNECTED),BM_SETCHECK,(WPARAM) 1,0);
      if (obj->flags & PROTOTYPE)
        SendMessage(GetDlgItem(hDlg,IDCB_PROTOTYPE),BM_SETCHECK,(WPARAM) 1,0);
      if (obj->flags & IN_EDITOR)
        SendMessage(GetDlgItem(hDlg,IDCB_INEDITOR),BM_SETCHECK,(WPARAM) 1,0);
      if (obj->flags & PRIV)
        SendMessage(GetDlgItem(hDlg,IDCB_PRIV),BM_SETCHECK,(WPARAM) 1,0);
      if (obj->flags & RESIDENT)
        SendMessage(GetDlgItem(hDlg,IDCB_RESIDENT),BM_SETCHECK,(WPARAM) 1,0);
      CenterWindow(hDlg,GetWindow(hDlg,GW_OWNER));
      break;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDCANCEL:
          EndDialog(hDlg,0);
          break;
        case IDOK:
          obj=RefnoToObject(DefaultRefno);
          if (obj->refno==0 && !SendMessage(GetDlgItem(hDlg,IDCB_PRIV),
                                            BM_GETCHECK,0,0)) {
            MessageBox(hDlg,"Boot object must be PRIV","Set Flags",MB_OK);
            break;
          }
          if (SendMessage(GetDlgItem(hDlg,IDCB_INTERACTIVE),BM_GETCHECK,0,0))
            obj->flags|=INTERACTIVE;
          else
            obj->flags&=~INTERACTIVE;
          if (SendMessage(GetDlgItem(hDlg,IDCB_PRIV),BM_GETCHECK,0,0))
            obj->flags|=PRIV;
          else
            obj->flags&=~PRIV;
          EndDialog(hDlg,1);
          break;
        default:
          return FALSE;
      }
      return TRUE;
    default:
      return FALSE;
  }     
}

LRESULT APIENTRY DialogMoveObject(HWND hDlg,UINT message,UINT wParam,UINT lParam) {
  struct object *obj;

  switch (message) {
    case WM_INITDIALOG:
      SendMessage(GetDlgItem(hDlg,IDE_OBJECTSELECTION),EM_LIMITTEXT,
                  (WPARAM) MAX_STR_LEN-1,0);
      CenterWindow(hDlg,GetWindow(hDlg,GW_OWNER));
      SetFocus(GetDlgItem(hDlg,IDE_OBJECTSELECTION));
      return TRUE;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDCANCEL:
          EndDialog(hDlg,0);
          break;
        case IDOK:
          GetDlgItemText(hDlg,IDE_OBJECTSELECTION,temp_buf,MAX_STR_LEN);
          if (strcmp(temp_buf,"void")) {
            DefaultRefno=StringToObject(temp_buf);
            if (DefaultRefno<0) {
              MessageBox(hDlg,"Invalid Object","Alert",MB_OK);
              return TRUE;
            }
            obj=RefnoToObject(DefaultRefno);
            if (obj->flags & GARBAGE) {
              MessageBox(hDlg,"Invalid Object","Alert",MB_OK);
              return TRUE;
            }
          } else
            DefaultRefno=(-1);
          EndDialog(hDlg,1);
          break;
        default:
          return FALSE;
      }
      return TRUE;
    default:
      return FALSE;
  }
}

LRESULT APIENTRY DialogVariable(HWND hDlg,UINT message,UINT wParam,UINT lParam) {
  struct object *obj,*obj2;
  long temp_var,temp_refno,int_value;
  char *leftover,*str2,*pos1,*pos2;
  struct ref_list *tmpref;

  switch (message) {
    case WM_INITDIALOG:
      obj=RefnoToObject(DefaultRefno);
      load_data(obj);
      switch (obj->globals[DefaultVar].type) {
        case INTEGER:
          CheckRadioButton(hDlg,IDRB_INTEGER,IDRB_OBJECT,IDRB_INTEGER);
          sprintf(temp_buf,"%ld",(long) obj->globals[DefaultVar].value.integer);
          SetDlgItemText(hDlg,IDE_VARIABLE,temp_buf);
          break;
        case OBJECT:
          CheckRadioButton(hDlg,IDRB_INTEGER,IDRB_OBJECT,IDRB_OBJECT);
          sprintf(temp_buf,"%s#%ld",
                  obj->globals[DefaultVar].value.objptr->parent->pathname,
                  (long) obj->globals[DefaultVar].value.objptr->refno);
          SetDlgItemText(hDlg,IDE_VARIABLE,temp_buf);
          break;
        case STRING:
          CheckRadioButton(hDlg,IDRB_INTEGER,IDRB_OBJECT,IDRB_STRING);
          int_value=0;
          pos1=obj->globals[DefaultVar].value.string;
          while (*pos1) {
            int_value++;
            if (*pos1=='\n') int_value++;
            else if (*pos1=='\t') int_value++;
            else if (*pos1=='\r') int_value++;
            else if (*pos1=='\a') int_value++;
            else if (*pos1=='\b') int_value++;
            else if (*pos1=='\f') int_value++;
            else if (*pos1=='\v') int_value++;
            else if (*pos1=='\\') int_value++;
            pos1++;
          }
          str2=MALLOC(int_value+1);
          pos1=obj->globals[DefaultVar].value.string;
          pos2=str2;
          while (*pos1) {
            switch (*pos1) {
              case '\n':
               *(pos2++)='\\';
               *(pos2++)='n';
               break;
              case '\t':
                *(pos2++)='\\';
                *(pos2++)='t';
                break;
              case '\r':
                *(pos2++)='\\';
                *(pos2++)='r';
                break;
              case '\a':
                *(pos2++)='\\';
                *(pos2++)='a';
                break;
              case '\b':
                *(pos2++)='\\';
                *(pos2++)='b';
                break;
              case '\f':
                *(pos2++)='\\';
                *(pos2++)='f';
                break;
              case '\v':
                *(pos2++)='\\';
                *(pos2++)='v';
                break;
              case '\\':
                *(pos2++)='\\';
                *(pos2++)='\\';
              default:
                *(pos2++)=*pos1;
                break;
            }
            pos1++;
          }
          *pos2='\0';
          SetDlgItemText(hDlg,IDE_VARIABLE,str2);
          FREE(str2);
          break;
        default:
          EndDialog(hDlg,0);
          return TRUE;
      }
      CenterWindow(hDlg,GetWindow(hDlg,GW_OWNER));
      break;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDCANCEL:
          EndDialog(hDlg,0);
          break;
        case IDB_VIEW:
          temp_refno=DefaultRefno;
          temp_var=DefaultVar;
          if (IsDlgButtonChecked(hDlg,IDRB_OBJECT)) {
            GetDlgItemText(hDlg,IDE_VARIABLE,temp_buf,MAX_STR_LEN-1);
            DefaultRefno=StringToObject(temp_buf);
            if (DefaultRefno>=0)
              DialogBox(hInst,"DlgObject",hDlg,DialogObject);
            DefaultRefno=temp_refno;
            DefaultVar=temp_var;
          }
          break;
        case IDOK:
          if (IsDlgButtonChecked(hDlg,IDRB_INTEGER)) {
            GetDlgItemText(hDlg,IDE_VARIABLE,temp_buf,MAX_STR_LEN-1);
            int_value=strtol(temp_buf,&leftover,10);
            if (leftover)
              if (*leftover)
                break;
            obj=RefnoToObject(DefaultRefno);
            clear_global_var(obj,DefaultVar);
            obj->globals[DefaultVar].type=INTEGER;
            obj->globals[DefaultVar].value.integer=int_value;
          } else if (IsDlgButtonChecked(hDlg,IDRB_STRING)) {
            int_value=SendMessage(GetDlgItem(hDlg,IDE_VARIABLE),WM_GETTEXTLENGTH,0,0);
            if (int_value<=0) break;
            leftover=MALLOC(int_value+1);
            GetDlgItemText(hDlg,IDE_VARIABLE,leftover,int_value+1);
            str2=MALLOC(int_value+1);
            pos1=leftover;
            pos2=str2;
            while (*pos1) {
              if (*pos1=='\\') {
                if (*(pos1+1)) {
                  pos1++;
                  if (*pos1=='a') *(pos2++)='\a';
                  else if (*pos1=='b') *(pos2++)='\b';
                  else if (*pos1=='f') *(pos2++)='\f';
                  else if (*pos1=='v') *(pos2++)='\v';
                  else if (*pos1=='n') *(pos2++)='\n';
                  else if (*pos1=='t') *(pos2++)='\t';
                  else if (*pos1=='r') *(pos2++)='\r';
                  else *(pos2++)=(*pos1);
                } else {
                  *(pos2++)='\\';
                }
              } else {
                *(pos2++)=(*pos1);
              }
              pos1++;
            }
            *pos2='\0';
            obj=RefnoToObject(DefaultRefno);
            clear_global_var(obj,DefaultVar);
            obj->globals[DefaultVar].type=STRING;
            obj->globals[DefaultVar].value.string=copy_string(str2);
            FREE(str2);
            FREE(leftover);
          } else {
            GetDlgItemText(hDlg,IDE_VARIABLE,temp_buf,MAX_STR_LEN-1);
            temp_refno=StringToObject(temp_buf);
            if (temp_refno<0) break;
            obj2=RefnoToObject(temp_refno);
            if (obj2->flags & GARBAGE) break;
            obj=RefnoToObject(DefaultRefno);
            clear_global_var(obj,DefaultVar);
            obj->globals[DefaultVar].type=OBJECT;
            obj->globals[DefaultVar].value.objptr=obj2;
            load_data(obj2);
            obj2->obj_state=DIRTY;
            tmpref=MALLOC(sizeof(struct ref_list));
            tmpref->ref_obj=obj;
            tmpref->ref_num=DefaultVar;
            tmpref->next=obj2->refd_by;
            obj2->refd_by=tmpref;
          }
          obj->obj_state=DIRTY;
          EndDialog(hDlg,1);
          break;
        default:
          return FALSE;
      }
      return TRUE;
    default:
      return FALSE;
  }
}

LRESULT APIENTRY DialogObject(HWND hDlg,UINT message,UINT wParam,UINT lParam) {
  struct object *obj,*dest;
  struct verb *curr_verb;
  long count,index,temp_refno;
  struct ref_list *curr_ref;
  int tabs[2];

  switch (message) {
    case WM_INITDIALOG:
      tabs[0]=10;
      tabs[1]=52;
      SendMessage(GetDlgItem(hDlg,IDLB_VERBS),LB_SETTABSTOPS,(WPARAM) 2,
                  (LPARAM) tabs);
      tabs[0]=32;
      tabs[1]=33;
      SendMessage(GetDlgItem(hDlg,IDLB_REFS),LB_SETTABSTOPS,(WPARAM) 2,
                  (LPARAM) tabs);
      CenterWindow(hDlg,GetWindow(hDlg,GW_OWNER));
      if (DefaultRefno<0 || DefaultRefno>=db_top) DefaultRefno=0;
      SetObjectEntries(hDlg,DefaultRefno);
      break;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDB_DONE:
        case IDCANCEL:
          EndDialog(hDlg,1);
          break;
        case IDB_DESTROY:
          obj=RefnoToObject(DefaultRefno);
          if (!obj) break;
          if (obj->flags & GARBAGE) break;
          if (obj->refno==0) {
            MessageBox(hDlg,"The boot object is indestructible","Destroy",MB_OK);
            break;
          }
          sprintf(temp_buf,"Are you sure you want to destroy %s#%ld ?",
                  obj->parent->pathname,(long) obj->refno);
          if (MessageBox(hDlg,temp_buf,"Destroy",MB_OKCANCEL)==IDOK) {
            queue_for_destruct(obj);
            SelectRequestReturn();
            EndDialog(hDlg,1);
          }
          break;
        case IDB_FLAGS:
          obj=RefnoToObject(DefaultRefno);
          if (!obj) break;
          if (obj->flags & GARBAGE) break;
          if (DialogBox(hInst,"DlgFlags",hDlg,DialogFlags)) {
            SetObjectEntries(hDlg,DefaultRefno);
            SelectRequestReturn();
          }
          break;
        case IDB_MOVE:
          obj=RefnoToObject(DefaultRefno);
          if (!obj) break;
          if (obj->flags & GARBAGE) break;
          temp_refno=DefaultRefno;
          if (DialogBox(hInst,"DlgMoveObject",hDlg,DialogMoveObject)) {
            if (DefaultRefno==(-1)) dest=NULL;
            else dest=ref_to_obj(DefaultRefno);
            obj=ref_to_obj(temp_refno);
            if (!MoveObject(obj,dest))
              MessageBox(hDlg,"Move Failed","Move Object",MB_OK);
            else {
              SetObjectEntries(hDlg,temp_refno);
              SelectRequestReturn();
            }
          }
          DefaultRefno=temp_refno;
          break;
        case IDB_LOAD:
          obj=RefnoToObject(DefaultRefno);
          if (!obj) break;
          if (obj->flags & GARBAGE) break;
          load_data(obj);
          SetObjectEntries(hDlg,DefaultRefno);
          break;
        case IDB_PARENT:
          obj=RefnoToObject(DefaultRefno);
          if (!obj) break;
          if (!(obj->parent)) break;
          DefaultRefno=obj->parent->proto_obj->refno;
          SetObjectEntries(hDlg,DefaultRefno);
          break;
        case IDB_NEXTCHILD:
          obj=RefnoToObject(DefaultRefno);
          if (!obj) break;
          if (!(obj->next_child)) break;
          DefaultRefno=obj->next_child->refno;
          SetObjectEntries(hDlg,DefaultRefno);
          break;
        case IDB_LOCATION:
          obj=RefnoToObject(DefaultRefno);
          if (!obj) break;
          if (!(obj->location)) break;
          DefaultRefno=obj->location->refno;
          SetObjectEntries(hDlg,DefaultRefno);
          break;
        case IDB_CONTENTS:
          obj=RefnoToObject(DefaultRefno);
          if (!obj) break;
          if (!(obj->contents)) break;
          DefaultRefno=obj->contents->refno;
          SetObjectEntries(hDlg,DefaultRefno);
          break;
        case IDB_NEXTOBJECT:
          obj=RefnoToObject(DefaultRefno);
          if (!obj) break;
          if (!(obj->next_object)) break;
          DefaultRefno=obj->next_object->refno;
          SetObjectEntries(hDlg,DefaultRefno);
          break;
        case IDLB_REFS:
          if (HIWORD(wParam)!=LBN_DBLCLK) return FALSE;
          obj=RefnoToObject(DefaultRefno);
          if (!obj) break;
          if (!(obj->refd_by)) break;
          curr_ref=obj->refd_by;
          index=SendMessage(GetDlgItem(hDlg,IDLB_REFS),LB_GETCURSEL,0,0);
          if (index<0) break;
          count=0;
          while (curr_ref && count<index) {
            curr_ref=curr_ref->next;
            count++;
          }
          if (!curr_ref) break;
          DefaultRefno=curr_ref->ref_obj->refno;
          SetObjectEntries(hDlg,DefaultRefno);
          break;
        case IDLB_VARS:
          if (HIWORD(wParam)!=LBN_DBLCLK) return FALSE;
          index=SendMessage(GetDlgItem(hDlg,IDLB_VARS),LB_GETCURSEL,0,0);
          if (index<0) break;
          DefaultVar=index;
          temp_refno=DefaultRefno;
          DialogBox(hInst,"DlgVariable",hDlg,DialogVariable);
          SetObjectEntries(hDlg,temp_refno);
          SelectRequestReturn();
          DefaultRefno=temp_refno;
          break;
        case IDLB_VERBS:
          if (HIWORD(wParam)!=LBN_DBLCLK) return FALSE;
          index=SendMessage(GetDlgItem(hDlg,IDLB_VERBS),LB_GETCURSEL,0,0);
          if (index<0) break;
          obj=RefnoToObject(DefaultRefno);
          if (!obj) break;
          if (obj->flags & GARBAGE) break;
          curr_verb=obj->verb_list;
          while (curr_verb && index--) curr_verb=curr_verb->next;
          if (!curr_verb) break;
          temp_refno=DefaultRefno;
          DefaultProto=obj->parent;
          DefaultFunc=find_fns(curr_verb->function,obj);
          DialogBox(hInst,"DLGProto",hDlg,DialogProto);
          DefaultRefno=temp_refno;
          SetObjectEntries(hDlg,DefaultRefno);
          break;
        case IDB_PATH:
          obj=RefnoToObject(DefaultRefno);
          if (!obj) break;
          if (!(obj->parent)) break;
          DefaultProto=obj->parent;
          DefaultFunc=NULL;
          temp_refno=DefaultRefno;
          DialogBox(hInst,"DLGProto",hDlg,DialogProto);
          DefaultRefno=temp_refno;
          SetObjectEntries(hDlg,DefaultRefno);
          break;
        default:
          return FALSE;
      }
      return TRUE;
    default:
      return FALSE;
  }
}

LRESULT APIENTRY DialogConnections(HWND hDlg,UINT message,UINT wParam,UINT lParam) {
  int loop,tabs[2];

  switch (message) {
    case WM_INITDIALOG:
      tabs[0]=24;
      tabs[1]=88;
      SendMessage(GetDlgItem(hDlg,IDLB_CONLIST),LB_SETTABSTOPS,(WPARAM) 2,
                  (LPARAM) tabs);
      CenterWindow(hDlg,GetWindow(hDlg,GW_OWNER));
      loop=0;
      while (loop<num_conns) {
        if (connlist[loop].fd!=(-1)) {
          sprintf(temp_buf,"%d\t%s\t%s#%ld",
                  (int) loop,
                  get_devconn(connlist[loop].obj),
                  connlist[loop].obj->parent->pathname,
                  (long) connlist[loop].obj->refno);
          SendMessage(GetDlgItem(hDlg,IDLB_CONLIST),LB_ADDSTRING,0,(LPARAM) temp_buf);
        }
        loop++;
      }
      sprintf(temp_buf,"Maximum Number of Connections: %d",(int) num_conns);
      SetDlgItemText(hDlg,IDT_CONMSG1,temp_buf);
      return TRUE;
    case WM_COMMAND:
      switch(LOWORD(wParam)) {
        case IDB_BOOT:
          loop=SendMessage(GetDlgItem(hDlg,IDLB_CONLIST),LB_GETCURSEL,0,0);
          if (loop<0 || loop>=num_conns) break;
          loop=GetNthConn(loop);
          if (loop<0) break;
          SelectWSAActivityReceived(connlist[loop].fd,FD_CLOSE);
          EndDialog(hDlg,1);
          break;
        case IDB_VIEW:
          loop=SendMessage(GetDlgItem(hDlg,IDLB_CONLIST),LB_GETCURSEL,0,0);
          if (loop<0 || loop>=num_conns) break;
          loop=GetNthConn(loop);
          if (loop<0) break;
          DefaultRefno=connlist[loop].obj->refno;
          DialogBox(hInst,"DLGObject",hDlg,DialogObject);
          break;
        case IDB_DONE:
        case IDCANCEL:
          EndDialog(hDlg,1);
          break;
        case IDLB_CONLIST:
          if (HIWORD(wParam)!=LBN_DBLCLK) return FALSE;
          loop=SendMessage(GetDlgItem(hDlg,IDLB_CONLIST),LB_GETCURSEL,0,0);
          if (loop<0 || loop>=num_conns) break;
          loop=GetNthConn(loop);
          if (loop<0) break;
          DefaultRefno=connlist[loop].obj->refno;
          DialogBox(hInst,"DLGObject",hDlg,DialogObject);
          break;
        default:
          return FALSE;
      }
      return TRUE;
    default:
      return FALSE;
  }
}

LRESULT APIENTRY DialogBroadcast(HWND hDlg,UINT message,UINT wParam,UINT lParam) {
  switch (message) {
    case WM_INITDIALOG:
      SendMessage(GetDlgItem(hDlg,IDE_BROADCAST),EM_LIMITTEXT,
                  (WPARAM) MAX_STR_LEN-101,0);
      CenterWindow(hDlg,GetWindow(hDlg,GW_OWNER));
      SetFocus(GetDlgItem(hDlg,IDE_BROADCAST));
      return TRUE;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDOK:
          GetDlgItemText(hDlg,IDE_BROADCAST,bcast_buf,MAX_STR_LEN-100);
          EndDialog(hDlg,1);
          break;
        case IDCANCEL:
          EndDialog(hDlg,0);
          break;
        default:
          return FALSE;
      }
      return TRUE;
    default:
      return FALSE;
  }
}

LRESULT APIENTRY DialogSelect(HWND hDlg,UINT message,UINT wParam,UINT lParam) {
  struct SWA *curr_swa;
  struct SFDL *curr_sfdl;
  struct SEL *curr_sel;

  switch (message) {
    case WM_INITDIALOG:
      CenterWindow(hDlg,GetWindow(hDlg,GW_OWNER));
      if (SelectTimerIsSet)
        SetDlgItemText(hDlg,IDT_SELECTMSG1,"Timer Is Set");
      else
        SetDlgItemText(hDlg,IDT_SELECTMSG1,"Timer Is Not Set");
      if (SelectTimeOutHasOccurred)
        SetDlgItemText(hDlg,IDT_SELECTMSG2,"Time-Out Has Occurred");
      else
        SetDlgItemText(hDlg,IDT_SELECTMSG2,"Time-Out Has Not Occurred");
      if (SelectReturnRequested())
        SetDlgItemText(hDlg,IDT_SELECTMSG3,"Return From Select Requested");
      else
        SetDlgItemText(hDlg,IDT_SELECTMSG3,"No Return From Select Requested");
      if (SelectWSAActivated) {
        curr_swa=SelectWSAActivated;
        while (curr_swa) {
          sprintf(temp_buf,"FD=%d",(int) curr_swa->fd);
          if (curr_swa->mode==SELMODE_LISTEN)
            strcat(temp_buf," Mode=Listen");
          else if (curr_swa->mode==SELMODE_DATA)
            strcat(temp_buf," Mode=Data");
          else sprintf(temp_buf,"FD=%d Unknown Mode=#%d",(int) curr_swa->fd,
                       (int) curr_swa->mode);
          if (curr_swa->events & FD_ACCEPT) strcat(temp_buf," Accept");
          if (curr_swa->events & FD_READ) strcat(temp_buf," Read");
          if (curr_swa->events & FD_WRITE) strcat(temp_buf," Write");
          if (curr_swa->events & FD_CLOSE) strcat(temp_buf," Close");
          SendMessage(GetDlgItem(hDlg,IDLB_SWA),LB_ADDSTRING,0,(LPARAM) temp_buf);
          curr_swa=curr_swa->next;
        }
      } else
        SendMessage(GetDlgItem(hDlg,IDLB_SWA),LB_ADDSTRING,0,(LPARAM) "(None)");
      if (SelectFDList) {
        curr_sfdl=SelectFDList;
        while (curr_sfdl) {
          sprintf(temp_buf,"FD=%d",(int) curr_sfdl->fd);
          if (curr_sfdl->is_read) strcat(temp_buf," Read");
          if (curr_sfdl->is_write) strcat(temp_buf," Write");
          if (curr_sfdl->is_except) strcat(temp_buf," Exception");
          SendMessage(GetDlgItem(hDlg,IDLB_SFDL),LB_ADDSTRING,0,(LPARAM) temp_buf);
          curr_sfdl=curr_sfdl->next;
        } 
      } else
        SendMessage(GetDlgItem(hDlg,IDLB_SFDL),LB_ADDSTRING,0,(LPARAM) "(None)");
      if (SelectEventList) {
        curr_sel=SelectEventList;
        while (curr_sel) {
          sprintf(temp_buf,"FD=%d",(int) curr_sel->fd);
          if (curr_sel->event==FD_READ) strcat(temp_buf," Read");
          else if (curr_sel->event==FD_WRITE) strcat(temp_buf," Write");
          else if (curr_sel->event==FD_CLOSE) strcat(temp_buf," Close");
          else if (curr_sel->event==FD_ACCEPT) strcat(temp_buf," Accept");
          else sprintf(temp_buf,"FD= %d Unknown Event #%d",(int) curr_sel->fd,
                       (int) curr_sel->event);
          SendMessage(GetDlgItem(hDlg,IDLB_SEL),LB_ADDSTRING,0,(LPARAM) temp_buf);
          curr_sel=curr_sel->next;
        }
      } else
        SendMessage(GetDlgItem(hDlg,IDLB_SEL),LB_ADDSTRING,0,(LPARAM) "(None)");
      return TRUE;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDB_DONE:
        case IDCANCEL:
          EndDialog(hDlg,1);
          break;
        default:
          return FALSE;
      }
      return TRUE;
    default:
      return FALSE;
  }
}

LRESULT APIENTRY DialogRTSet(HWND hDlg, UINT message, UINT wParam, UINT lParam) {
  char netbios_buf[16];
  int loop;

  switch (message) {
    case WM_INITDIALOG:
      CenterWindow(hDlg,GetWindow(hDlg,GW_OWNER));
	  if (port.protocol==CI_PROTOCOL_CONSOLE) {
	    SetDlgItemText(hDlg,IDE_SPROT,"Console");
		SetDlgItemText(hDlg,IDE_SPORT,"n/a");
	  } else if (port.protocol==CI_PROTOCOL_TCP) {
	    SetDlgItemText(hDlg,IDE_SPROT,"TCP/IP");
		sprintf(temp_buf,"port %d",(int) port.tcp_port);
		SetDlgItemText(hDlg,IDE_SPORT,temp_buf);
	  } else if (port.protocol==CI_PROTOCOL_IPX) {
	    SetDlgItemText(hDlg,IDE_SPROT,"IPX/SPX");
		sprintf(temp_buf,"%lX.%s,%lX",(long) port.ipx_net,
		        convert_from_6byte(&(port.ipx_node[0])),
				(long) port.ipx_socket);
		SetDlgItemText(hDlg,IDE_SPORT,temp_buf);
	  } else if (port.protocol=CI_PROTOCOL_NETBIOS) {
	    SetDlgItemText(hDlg,IDE_SPROT,"NetBIOS");
		loop=0;
		while (loop<15) {
		  netbios_buf[loop]=port.netbios_node[loop];
		  loop++;
		}
		netbios_buf[loop]='\0';
		do {
		  loop--;
		  if (netbios_buf[loop]==' ') netbios_buf[loop]='\0';
		  else break;
		} while (loop);
		sprintf(temp_buf,"%s,%d",netbios_buf,(int) port.netbios_port);
		SetDlgItemText(hDlg,IDE_SPORT,temp_buf);
	  } else {
	    SetDlgItemText(hDlg,IDE_SPROT,"<unknown>");
		SetDlgItemText(hDlg,IDE_SPORT,"<unknown>");
	  }
      SetDlgItemText(hDlg,IDE_SLOAD,load_name);
      SetDlgItemText(hDlg,IDE_SSAVE,save_name);
      SetDlgItemText(hDlg,IDE_SPANIC,panic_name);
      SetDlgItemText(hDlg,IDE_SMUDLIB,fs_path);
      SetDlgItemText(hDlg,IDE_SSYSLOG,syslog_name);
      SetDlgItemText(hDlg,IDE_SCACHE,transact_log_name);
	  sprintf(temp_buf,"%ld bytes",(long) transact_log_size);
	  SetDlgItemText(hDlg,IDE_SLOGSIZE,temp_buf);
      SetDlgItemText(hDlg,IDE_STEMP,tmpdb_name);
      return TRUE;
    case WM_COMMAND:
      switch(LOWORD(wParam)) {
        case IDB_DONE:
        case IDCANCEL:
          EndDialog(hDlg,1);
          break;
        default:
          return FALSE;
      }
      return TRUE;
    default:
      return FALSE;
  }
}

void RefreshAlarmList(HWND hDlg) {
  struct alarmq *curr;
  struct tm *time_s;
  time_t the_time;

  SendMessage(GetDlgItem(hDlg,IDLB_LIST),LB_RESETCONTENT,0,0);
  curr=alarm_list;
  while (curr) {
    the_time=int2time(curr->delay);
    time_s=localtime(&the_time);
    sprintf(temp_buf,"%2d/%02d/%2d %2d:%02d:%02d %s\t%s#%ld\t%s",
            (int) time_s->tm_mon+1,
            (int) time_s->tm_mday,
            (int) time_s->tm_year,
            (int) (((time_s->tm_hour)%12)?((time_s->tm_hour)%12):12),
            (int) time_s->tm_min,
            (int) time_s->tm_sec,
            ((time_s->tm_hour<12) ? "AM":"PM"),
            curr->obj->parent->pathname,
            (long) curr->obj->refno,
            curr->funcname);
    SendMessage(GetDlgItem(hDlg,IDLB_LIST),LB_ADDSTRING,0,(LPARAM) temp_buf);
    curr=curr->next;
  }
}

LRESULT APIENTRY DialogNewAlarm(HWND hDlg,UINT message,UINT wParam,UINT lParam) {
  long delay,temp_refno;
  struct object *obj;
  char *buf;

  switch (message) {
    case WM_INITDIALOG:
      CenterWindow(hDlg,GetWindow(hDlg,GW_OWNER));
      SetFocus(GetDlgItem(hDlg,IDE_DELAY));
      return TRUE;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDCANCEL:
          EndDialog(hDlg,0);
          break;
        case IDOK:
          GetDlgItemText(hDlg,IDE_DELAY,temp_buf,MAX_STR_LEN-1);
          delay=strtol(temp_buf,&buf,10);
          if (buf)
            if (*buf) {
              MessageBox(hDlg,"Illegal delay value","Alarm",MB_OK);
              break;
            }
          if (delay<0) {
            MessageBox(hDlg,"Illegal delay value","Alarm",MB_OK);
            break;
          }
          delay+=time2int(time(NULL));
          GetDlgItemText(hDlg,IDE_OBJECT,temp_buf,MAX_STR_LEN-1);
          temp_refno=StringToObject(temp_buf);
          if (temp_refno<0) {
            MessageBox(hDlg,"Illegal object value","Alarm",MB_OK);
            break;
          }
          obj=RefnoToObject(temp_refno);
          if (!obj) {
            MessageBox(hDlg,"Illegal object value","Alarm",MB_OK);
            break;
          }
          if (obj->flags & GARBAGE) {
            MessageBox(hDlg,"Illegal object value","Alarm",MB_OK);
            break;
          }
          GetDlgItemText(hDlg,IDE_FUNCTION,temp_buf,MAX_STR_LEN-1);
          if (!find_fns(temp_buf,obj)) {
            MessageBox(hDlg,"Illegal function value","Alarm",MB_OK);
            break;
          }
          db_queue_for_alarm(obj,delay,temp_buf);
          SelectRequestReturn();
          EndDialog(hDlg,1);
          break;
        default:
          return FALSE;
      }
      return TRUE;
    default:
      return FALSE;
  }
}

LRESULT APIENTRY DialogAlarm(HWND hDlg,UINT message,UINT wParam,UINT lParam) {
  long index;
  struct alarmq *curr;
  struct tm *time_s;
  time_t the_time;
  char *buf;
  struct object *obj;
  int tabs[2];

  switch (message) {
    case WM_INITDIALOG:
      tabs[0]=80;
      tabs[1]=128;
      SendMessage(GetDlgItem(hDlg,IDLB_LIST),LB_SETTABSTOPS,(WPARAM) 2,
                  (LPARAM) tabs);
      DialogTimerID=SetTimer(hDlg,DIALOG_TIMER_ID,1000,NULL);
      CenterWindow(hDlg,GetWindow(hDlg,GW_OWNER));
      RefreshAlarmList(hDlg);
      the_time=time(NULL);
      time_s=localtime(&the_time);
      sprintf(temp_buf,"Current Time: %2d/%02d/%2d %2d:%02d:%02d %s",
              (int) time_s->tm_mon+1,
              (int) time_s->tm_mday,
              (int) time_s->tm_year,
              (int) (((time_s->tm_hour)%12)?((time_s->tm_hour)%12):12),
              (int) time_s->tm_min,
              (int) time_s->tm_sec,
              ((time_s->tm_hour<12) ? "AM":"PM"));
      SetDlgItemText(hDlg,IDT_TEXT,temp_buf);
      return TRUE;
    case WM_TIMER:
      if (wParam!=DialogTimerID) return FALSE;
      the_time=time(NULL);
      time_s=localtime(&the_time);
      sprintf(temp_buf,"Current Time: %2d/%02d/%2d %2d:%02d:%02d %s",
              (int) time_s->tm_mon+1,
              (int) time_s->tm_mday,
              (int) time_s->tm_year,
              (int) (((time_s->tm_hour)%12)?((time_s->tm_hour)%12):12),
              (int) time_s->tm_min,
              (int) time_s->tm_sec,
              ((time_s->tm_hour<12) ? "AM":"PM"));
      SetDlgItemText(hDlg,IDT_TEXT,temp_buf);
      KillTimer(hDlg,DialogTimerID);
      DialogTimerID=SetTimer(hDlg,DIALOG_TIMER_ID,1000,NULL);
      return TRUE;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDLB_LIST:
          if (HIWORD(wParam)!=LBN_DBLCLK) return FALSE;
          index=SendMessage(GetDlgItem(hDlg,IDLB_LIST),LB_GETCURSEL,0,0);
          if (index<0) break;
          curr=alarm_list;
          while (curr && (index--)) curr=curr->next;
          if (!curr) break;
          DefaultRefno=curr->obj->refno;
          DialogBox(hInst,"DLGObject",hDlg,DialogObject);
          RefreshAlarmList(hDlg);
          break;
        case IDB_NEW:
          if (DialogBox(hInst,"DLGNewAlarm",hDlg,DialogNewAlarm))
            RefreshAlarmList(hDlg);
          break;
        case IDB_DELETE:
          index=SendMessage(GetDlgItem(hDlg,IDLB_LIST),LB_GETCURSEL,0,0);
          if (index<0) break;
          curr=alarm_list;
          while (curr && (index--)) curr=curr->next;
          if (!curr) break;
          buf=copy_string(curr->funcname);
          obj=curr->obj;
          sprintf(temp_buf,"Are you sure you want to destroy alarm \""
                  "%s\" on object %s#%ld ?",buf,obj->parent->pathname,
                  (long) obj->refno);
          if (MessageBox(hDlg,temp_buf,"Destroy",MB_OKCANCEL)==IDOK) {
            remove_alarm(obj,buf);
            RefreshAlarmList(hDlg);
          }
          FREE(buf);
          break;
        case IDB_DONE:
        case IDCANCEL:
          KillTimer(hDlg,DialogTimerID);
          EndDialog(hDlg,1);
          break;
        default:
          return FALSE;
      }
      return TRUE;
    default:
      return FALSE;
  }
}

void RefreshCommandList(HWND hDlg) {
  struct cmdq *curr;
  char *buf,*pos1,*pos2;
  int len;

  SendMessage(GetDlgItem(hDlg,IDLB_LIST),LB_RESETCONTENT,0,0);
  curr=cmd_head;
  while (curr) {
    len=0;
    pos1=curr->cmd;
    while (*pos1) {
      len++;
      if (*pos1=='\a' || *pos1=='\n' || *pos1=='\t' || *pos1=='\r' ||
          *pos1=='\b' || *pos1=='\f' || *pos1=='\v' || *pos1=='\\' ||
          *pos1=='\"') len++;
      pos1++;
    }
    buf=MALLOC(len+strlen(curr->obj->parent->pathname)+ITOA_BUFSIZ+5);
    sprintf(buf,"%s#%ld\t\"",curr->obj->parent->pathname,
            (long) curr->obj->refno);
    pos2=buf+strlen(buf);
    pos1=curr->cmd;
    while (*pos1) {
      switch (*pos1) {
        case '\n':
          *(pos2++)='\\';
          *(pos2++)='n';
          break;
        case '\t':
          *(pos2++)='\\';
          *(pos2++)='t';
          break;
        case '\r':
          *(pos2++)='\\';
          *(pos2++)='r';
          break;
        case '\a':
          *(pos2++)='\\';
          *(pos2++)='a';
          break;
        case '\b':
          *(pos2++)='\\';
          *(pos2++)='b';
          break;
        case '\f':
          *(pos2++)='\\';
          *(pos2++)='f';
          break;
        case '\v':
          *(pos2++)='\\';
          *(pos2++)='v';
          break;
        case '\\':
          *(pos2++)='\\';
          *(pos2++)='\\';
          break;
        case '\"':
          *(pos2++)='\\';
          *(pos2++)='\"';
          break;
        default:
          *(pos2++)=(*pos1);
          }
          pos1++;
    }
    *(pos2++)='\"';
    *pos2='\0';
    SendMessage(GetDlgItem(hDlg,IDLB_LIST),LB_ADDSTRING,0,(LPARAM) buf);
    FREE(buf);
    curr=curr->next;
  }
}

LRESULT APIENTRY DialogNewCommand(HWND hDlg,UINT message,UINT wParam,UINT lParam) {
  char *buf,*pos1,*pos2;
  long len,temp_refno;
  struct object *obj;

  switch (message) {
    case WM_INITDIALOG:
      CenterWindow(hDlg,GetWindow(hDlg,GW_OWNER));
      SetFocus(GetDlgItem(hDlg,IDE_OBJECT));
      return TRUE;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDCANCEL:
          EndDialog(hDlg,0);
          break;
        case IDOK:
          GetDlgItemText(hDlg,IDE_OBJECT,temp_buf,MAX_STR_LEN-1);
          temp_refno=StringToObject(temp_buf);
          if (temp_refno<0) {
            MessageBox(hDlg,"Illegal object value","Command",MB_OK);
            break;
          }
          obj=RefnoToObject(temp_refno);
          if (!obj) {
            MessageBox(hDlg,"Illegal object value","Command",MB_OK);
            break;
          }
          if (obj->flags & GARBAGE) {
            MessageBox(hDlg,"Illegal object value","Command",MB_OK);
            break;
          }
          GetDlgItemText(hDlg,IDE_COMMAND,temp_buf,MAX_STR_LEN-1);
          len=strlen(temp_buf);
          buf=MALLOC(len+1);
          pos1=temp_buf;
          pos2=buf;
          while (*pos1) {
            if (*pos1=='\\') {
              if (*(pos1+1)) {
                pos1++;
                if (*pos1=='a') *(pos2++)='\a';
                else if (*pos1=='b') *(pos2++)='\b';
                else if (*pos1=='f') *(pos2++)='\f';
                else if (*pos1=='v') *(pos2++)='\v';
                else if (*pos1=='n') *(pos2++)='\n';
                else if (*pos1=='t') *(pos2++)='\t';
                else if (*pos1=='r') *(pos2++)='\r';
                else *(pos2++)=(*pos1);
              } else {
                *(pos2++)='\\';
              }
            } else {
              *(pos2++)=(*pos1);
            }
            pos1++;
          }
          *pos2='\0';
          queue_command(obj,buf);
          FREE(buf);
          SelectRequestReturn();
          EndDialog(hDlg,1);
          break;
        default:
          return FALSE;
      }
      return TRUE;
    default:
      return FALSE;
  }
}

LRESULT APIENTRY DialogCommand(HWND hDlg,UINT message,UINT wParam,UINT lParam) {
  long index;
  struct cmdq *curr,*prev;
  int tabs[2];

  switch (message) {
    case WM_INITDIALOG:
      tabs[0]=60;
      tabs[1]=61;
      SendMessage(GetDlgItem(hDlg,IDLB_LIST),LB_SETTABSTOPS,(WPARAM) 2,
                  (LPARAM) tabs);
      CenterWindow(hDlg,GetWindow(hDlg,GW_OWNER));
      RefreshCommandList(hDlg);
      return TRUE;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDLB_LIST:
          if (HIWORD(wParam)!=LBN_DBLCLK) return FALSE;
          index=SendMessage(GetDlgItem(hDlg,IDLB_LIST),LB_GETCURSEL,0,0);
          if (index<0) break;
          curr=cmd_head;
          while (curr && (index--)) curr=curr->next;
          if (!curr) break;
          DefaultRefno=curr->obj->refno;
          DialogBox(hInst,"DLGObject",hDlg,DialogObject);
          RefreshCommandList(hDlg);
          break;
        case IDB_DONE:
        case IDCANCEL:
          EndDialog(hDlg,1);
          break;
        case IDB_NEW:
          if (DialogBox(hInst,"DLGNewCommand",hDlg,DialogNewCommand))
            RefreshCommandList(hDlg);
          break;
        case IDB_DELETE:
          index=SendMessage(GetDlgItem(hDlg,IDLB_LIST),LB_GETCURSEL,0,0);
          if (index<0) break;
          curr=cmd_head;
          prev=NULL;
          while (curr && (index--)) {
            prev=curr;
            curr=curr->next;
          }
          if (!curr) break;
          sprintf(temp_buf,"Are you sure you want to destroy command "
                  "by object %s#%ld ?",curr->obj->parent->pathname,
                  (long) curr->obj->refno);
          if (MessageBox(hDlg,temp_buf,"Destroy",MB_OKCANCEL)==IDOK) {
            if (prev) prev->next=curr->next;
            else cmd_head=curr->next;
            if (cmd_tail==curr) cmd_tail=prev;
            if (curr->cmd) FREE(curr->cmd);
            FREE(curr);
            RefreshCommandList(hDlg);
          }
          break;
        default:
          return FALSE;
      }
      return TRUE;
    default:
      return FALSE;
  }
}

void RefreshDestructList(HWND hDlg) {
  struct destq *curr;

  curr=dest_list;
  SendMessage(GetDlgItem(hDlg,IDLB_LIST),LB_RESETCONTENT,0,0);
  while (curr) {
    sprintf(temp_buf,"%s#%ld",curr->obj->parent->pathname,
            (long) curr->obj->refno);
    SendMessage(GetDlgItem(hDlg,IDLB_LIST),LB_ADDSTRING,0,(LPARAM) temp_buf);
    curr=curr->next;
  }
}

LRESULT APIENTRY DialogDestruct(HWND hDlg,UINT message,UINT wParam,UINT lParam) {
  long index;
  struct destq *curr;

  switch (message) {
    case WM_INITDIALOG:
      CenterWindow(hDlg,GetWindow(hDlg,GW_OWNER));
      RefreshDestructList(hDlg);
      return TRUE;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDLB_LIST:
          if (HIWORD(wParam)!=LBN_DBLCLK) return FALSE;
          index=SendMessage(GetDlgItem(hDlg,IDLB_LIST),LB_GETCURSEL,0,0);
          if (index<0) break;
          curr=dest_list;
          while (curr && (index--)) curr=curr->next;
          if (!curr) break;
          DefaultRefno=curr->obj->refno;
          DialogBox(hInst,"DLGObject",hDlg,DialogObject);
          RefreshDestructList(hDlg);
          break;
        case IDB_DONE:
        case IDCANCEL:
          EndDialog(hDlg,1);
          break;
        default:
          return FALSE;
      }
      return TRUE;
    default:
      return FALSE;
  }
}

LRESULT APIENTRY DialogCTSet(HWND hDlg, UINT message, UINT wParam, UINT lParam) {
  switch (message) {
    case WM_INITDIALOG:
      CenterWindow(hDlg,GetWindow(hDlg,GW_OWNER));
#ifdef CYCLE_SOFT_MAX
      sprintf(temp_buf,"%d cycles",(int) CYCLE_SOFT_MAX);
      SetDlgItemText(hDlg,IDE_SSCM,temp_buf);
#else /* CYCLE_SOFT_MAX */
      SetDlgItemText(hDlg,IDE_SSCM,"Disabled");
#endif /* CYCLE_SOFT_MAX */
#ifdef CYCLE_HARD_MAX
      sprintf(temp_buf,"%d cycles",(int) CYCLE_HARD_MAX);
      SetDlgItemText(hDlg,IDE_SHCM,temp_buf);
#else /* CYCLE_HARD_MAX */
      SetDlgItemText(hDlg,IDE_SHCM,"Disabled");
#endif /* CYCLE_HARD_MAX */
      sprintf(temp_buf,"%d",(int) MIN_FREE_FILES);
      SetDlgItemText(hDlg,IDE_SMFF,temp_buf);
      sprintf(temp_buf,"%d",(int) MAX_CONNS);
      SetDlgItemText(hDlg,IDE_SMC,temp_buf);
      sprintf(temp_buf,"%d bytes",(int) WRITE_BURST);
      SetDlgItemText(hDlg,IDE_SWB,temp_buf);
      sprintf(temp_buf,"%d bytes",(int) MAX_OUTBUF_LEN);
      SetDlgItemText(hDlg,IDE_SMOL,temp_buf);
      sprintf(temp_buf,"%d objects",(int) CACHE_SIZE);
      SetDlgItemText(hDlg,IDE_SCS,temp_buf);
      sprintf(temp_buf,"%d",(int) CACHE_HASH);
      SetDlgItemText(hDlg,IDE_SCH,temp_buf);
      sprintf(temp_buf,"%d",(int) OBJ_ALLOC_BLKSIZ);
      SetDlgItemText(hDlg,IDE_SOAB,temp_buf);
      sprintf(temp_buf,"%d",(int) NUM_ELINES);
      SetDlgItemText(hDlg,IDE_SNE,temp_buf);
      sprintf(temp_buf,"%d bytes",(int) MAX_STR_LEN);
      SetDlgItemText(hDlg,IDE_SMSL,temp_buf);
      sprintf(temp_buf,"%d bytes",(int) EBUFSIZ);
      SetDlgItemText(hDlg,IDE_SE,temp_buf);
      sprintf(temp_buf,"%d",(int) MAX_DEPTH);
      SetDlgItemText(hDlg,IDE_SMD,temp_buf);
      strcpy(temp_buf,DB_IDENSTR);
      temp_buf[strlen(DB_IDENSTR)-1]='\0';
      SetDlgItemText(hDlg,IDE_SDI,temp_buf);
      return TRUE;
    case WM_COMMAND:
      switch(LOWORD(wParam)) {
        case IDB_DONE:
        case IDCANCEL:
          EndDialog(hDlg,1);
          break;
        default:
          return FALSE;
      }
      return TRUE;
    default:
      return FALSE;
  }
}

char *MakeFSEName(struct file_entry *fse) {
  struct file_entry *curr;
  int len;
  char *buf,*buf2;

  if (!fse) fse=root_dir;
  if (fse==root_dir) {
    buf=MALLOC(2);
    buf[0]='/';
    buf[1]='\0';
  } else {
    curr=fse;
    len=0;
    while (curr!=root_dir) {
      len+=1+strlen(curr->filename);
      curr=curr->parent;
    }
    buf=MALLOC(len+1);
    curr=fse;
    buf[0]='\0';
    while (curr!=root_dir) {
      buf2=copy_string(buf);
      buf[0]='/';
      buf[1]='\0';
      strcat(buf,curr->filename);
      strcat(buf,buf2);
      FREE(buf2);
      curr=curr->parent;
    }
  }
  return buf;
}

void RefreshFSDialog(HWND hDlg) {
  struct object *obj;
  struct file_entry *curr;
  char *buf;

  if (!DefaultFS) DefaultFS=root_dir;
  buf=MakeFSEName(DefaultFS);
  SetDlgItemText(hDlg,IDE_FILE,buf);
  FREE(buf);
  temp_buf[0]='\0';
  if (DefaultFS->flags & DIRECTORY) strcat(temp_buf," Directory");
  if (DefaultFS->flags & READ_OK) strcat(temp_buf," Read");
  if (DefaultFS->flags & WRITE_OK) strcat(temp_buf," Write");
  if (temp_buf[0]) SetDlgItemText(hDlg,IDE_FLAGS,temp_buf+1);
  else SetDlgItemText(hDlg,IDE_FLAGS,"");
  obj=RefnoToObject(DefaultFS->owner);
  if (obj) {
    if (obj->flags & GARBAGE)
      sprintf(temp_buf,"#%ld",(long) obj->refno);
    else
      sprintf(temp_buf,"%s#%ld",obj->parent->pathname,(long) obj->refno);
  } else
    sprintf(temp_buf,"#%ld",(long) DefaultFS->owner);
  SetDlgItemText(hDlg,IDB_OWNER,temp_buf);
  SendMessage(GetDlgItem(hDlg,IDLB_CONTENTS),LB_RESETCONTENT,0,0);
  curr=DefaultFS->contents;
  while (curr) {
    sprintf(temp_buf,"%s\t%s\t%s\t%s",
            ((curr->flags & DIRECTORY)?"d":"-"),
            ((curr->flags & READ_OK)?"r":"-"),
            ((curr->flags & WRITE_OK)?"w":"-"),
            curr->filename);
    SendMessage(GetDlgItem(hDlg,IDLB_CONTENTS),LB_ADDSTRING,0,(LPARAM) temp_buf);
    curr=curr->next_file;
  }
}

LRESULT APIENTRY DialogUnhide(HWND hDlg,UINT message,UINT wParam,UINT lParam) {
  long owner;
  int flags;
  struct object *obj;
  char *buf1,*buf2;

  switch (message) {
    case WM_INITDIALOG:
      CenterWindow(hDlg,GetWindow(hDlg,GW_OWNER));
      SetFocus(GetDlgItem(hDlg,IDE_FILE));
      return TRUE;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDOK:
          if (!(DefaultFS->flags & DIRECTORY)) {
            MessageBox(hDlg,"Must unhide from a directory","Unhide",MB_OK);
            break;
          }
          GetDlgItemText(hDlg,IDE_OWNER,temp_buf,MAX_STR_LEN-1);
          owner=StringToObject(temp_buf);
          if (owner<0) {
            MessageBox(hDlg,"Illegal object value","Unhide",MB_OK);
            break;
          }
          obj=RefnoToObject(owner);
          if (!obj) {
            MessageBox(hDlg,"Illegal object value","Unhide",MB_OK);
            break;
          }
          if (obj->flags & GARBAGE) {
            MessageBox(hDlg,"Illegal object value","Unhide",MB_OK);
            break;
          }
          flags=0;
          if (IsDlgButtonChecked(hDlg,IDCB_DIRECTORY))
            flags|=DIRECTORY;
          else
            flags&=~DIRECTORY;
          if (IsDlgButtonChecked(hDlg,IDCB_READ))
            flags|=READ_OK;
          else
            flags&=~READ_OK;
          if (IsDlgButtonChecked(hDlg,IDCB_WRITE))
            flags|=WRITE_OK;
          else
            flags&=~WRITE_OK;
          GetDlgItemText(hDlg,IDE_FILE,temp_buf,MAX_STR_LEN-1);
          buf1=MakeFSEName(DefaultFS);
          buf2=MALLOC(strlen(buf1)+strlen(temp_buf)+2);
          strcpy(buf2,buf1);
          if (DefaultFS!=root_dir) strcat(buf2,"/");
          strcat(buf2,temp_buf);
          FREE(buf1);
          if (unhide(buf2,obj,flags))
            MessageBox(hDlg,"Unhide failed","Unhide",MB_OK);
          FREE(buf2);
          EndDialog(hDlg,1);
          break;
        case IDCANCEL:
          EndDialog(hDlg,0);
          break;
        default:
          return FALSE;
      }
      return TRUE;
    default:
      return FALSE;
  }
}

LRESULT APIENTRY DialogModify(HWND hDlg,UINT message,UINT wParam,UINT lParam) {
  struct object *obj;
  long owner;

  switch (message) {
    case WM_INITDIALOG:
      CenterWindow(hDlg,GetWindow(hDlg,GW_OWNER));
      if (DefaultFS->flags & DIRECTORY)
        CheckDlgButton(hDlg,IDCB_DIRECTORY,1);
      else
        CheckDlgButton(hDlg,IDCB_DIRECTORY,0);
      if (DefaultFS->flags & READ_OK)
        CheckDlgButton(hDlg,IDCB_READ,1);
      else
        CheckDlgButton(hDlg,IDCB_READ,0);
      if (DefaultFS->flags & WRITE_OK)
        CheckDlgButton(hDlg,IDCB_WRITE,1);
      else
        CheckDlgButton(hDlg,IDCB_WRITE,0);
      obj=RefnoToObject(DefaultFS->owner);
      if (!obj) obj=ref_to_obj(0);
      if (obj->parent)
        sprintf(temp_buf,"%s#%ld",obj->parent->pathname,(long) obj->refno);
      else
        sprintf(temp_buf,"#%ld",(long) obj->refno);
      SetDlgItemText(hDlg,IDE_OWNER,temp_buf);
      return TRUE;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDOK:
          GetDlgItemText(hDlg,IDE_OWNER,temp_buf,MAX_STR_LEN-1);
          owner=StringToObject(temp_buf);
          if (owner<0) {
            MessageBox(hDlg,"Illegal object value","Modify",MB_OK);
            break;
          }
          if (owner!=DefaultFS->owner) {
            obj=RefnoToObject(owner);
            if (!obj) {
              MessageBox(hDlg,"Illegal object value","Modify",MB_OK);
              break;
            }
            if (obj->flags & GARBAGE) {
              MessageBox(hDlg,"Illegal object value","Modify",MB_OK);
              break;
            }
          }
          DefaultFS->owner=owner;
          if (IsDlgButtonChecked(hDlg,IDCB_READ))
            DefaultFS->flags|=READ_OK;
          else
            DefaultFS->flags&=~READ_OK;
          if (IsDlgButtonChecked(hDlg,IDCB_WRITE))
            DefaultFS->flags|=WRITE_OK;
          else
            DefaultFS->flags&=~WRITE_OK;
          EndDialog(hDlg,1);
          break;
        case IDCANCEL:
          EndDialog(hDlg,0);
          break;
        default:
          return FALSE;
      }
      return TRUE;
    default:
      return FALSE;
  }
}

LRESULT APIENTRY DialogFileSystem(HWND hDlg,UINT message,UINT wParam,UINT lParam) {
  int index;
  struct file_entry *curr;
  struct object *obj;
  char *buf;
  int tabs[3];

  switch (message) {
    case WM_INITDIALOG:
      tabs[0]=6;
      tabs[1]=12;
      tabs[2]=24;
      SendMessage(GetDlgItem(hDlg,IDLB_CONTENTS),LB_SETTABSTOPS,(WPARAM) 3,
                  (LPARAM) tabs);
      if (!DefaultFS) DefaultFS=root_dir;
      CenterWindow(hDlg,GetWindow(hDlg,GW_OWNER));
      RefreshFSDialog(hDlg);
      return TRUE;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDB_BACK:
          if (!(DefaultFS->parent)) break;
          DefaultFS=DefaultFS->parent;
          RefreshFSDialog(hDlg);
          break;
        case IDB_HIDE:
          index=SendMessage(GetDlgItem(hDlg,IDLB_CONTENTS),LB_GETCURSEL,0,0);
          if (index<0) break;
          curr=DefaultFS->contents;
          while (curr && index--) curr=curr->next_file;
          if (!curr) break;
          buf=MakeFSEName(curr);
          sprintf(temp_buf,"Are you sure you want to hide %s ?",buf);
          if (MessageBox(hDlg,temp_buf,"Hide",MB_OKCANCEL)==IDOK) {
            if (hide(buf))
              MessageBox(hDlg,"Hide failed","Hide",MB_OK);
            RefreshFSDialog(hDlg);
          }
          FREE(buf);
          break;
        case IDB_UNHIDE:
          if (!(DefaultFS->flags & DIRECTORY)) break;
          if (DialogBox(hInst,"DLGUnhide",hDlg,DialogUnhide))
            RefreshFSDialog(hDlg);
          break;
        case IDB_DELETE:
          index=SendMessage(GetDlgItem(hDlg,IDLB_CONTENTS),LB_GETCURSEL,0,0);
          if (index<0) break;
          curr=DefaultFS->contents;
          while (curr && index--) curr=curr->next_file;
          if (!curr) break;
          if (curr==root_dir) {
            MessageBox(hDlg,"Can't delete root directory","Delete",MB_OK);
            break;
          }
          buf=MakeFSEName(curr);
          sprintf(temp_buf,"Are you sure you want to delete %s ?",buf);
          if (MessageBox(hDlg,temp_buf,"Delete",MB_OKCANCEL)==IDOK) {
            if (curr->flags & DIRECTORY) {
              if (remove_dir(buf,ref_to_obj(0)))
                MessageBox(hDlg,"Delete failed","Delete",MB_OK);
            } else
              if (remove_file(buf,ref_to_obj(0)))
                MessageBox(hDlg,"Delete failed","Delete",MB_OK);
            RefreshFSDialog(hDlg);
          }
          FREE(buf);
          break;
        case IDB_MODIFY:
          if (DialogBox(hInst,"DLGModify",hDlg,DialogModify))
            RefreshFSDialog(hDlg);
          break;
        case IDB_OWNER:
          obj=RefnoToObject(DefaultFS->owner);
          if (!obj) break;
          DefaultRefno=obj->refno;
          DialogBox(hInst,"DLGObject",hDlg,DialogObject);
          break;
        case IDLB_CONTENTS:
          if (HIWORD(wParam)!=LBN_DBLCLK) return FALSE;
          index=SendMessage(GetDlgItem(hDlg,IDLB_CONTENTS),LB_GETCURSEL,0,0);
          if (index<0) break;
          curr=DefaultFS->contents;
          while (curr && index--) curr=curr->next_file;
          if (!curr) break;
          DefaultFS=curr;
          RefreshFSDialog(hDlg);
          break;
        case IDB_DONE:
        case IDCANCEL:
          EndDialog(hDlg,1);
          break;
        default:
          return FALSE;
      }
      return TRUE;
    default:
      return FALSE;
  }
}

LONG APIENTRY MainWndProc(HWND hWnd,UINT message,UINT wParam,LONG lParam) {
  RECT clientrect;
  struct object *curr_who;

  switch (message) {
    case WSA_ACTIVITY:
      SelectWSAActivityReceived(wParam,WSAGETSELECTEVENT(lParam));
      break;
    case WM_TIMER:
      if (wParam==SelectTimerID) SelectTimerReceived();
      else return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    case WM_CREATE:
      GetClientRect(hWnd,&clientrect);
      outputwin=CreateWindow("edit",
                             NULL,
                             WS_BORDER | WS_CHILD | WS_VSCROLL | WS_VISIBLE |
                               ES_LEFT | ES_AUTOVSCROLL | ES_MULTILINE |
                               ES_NOHIDESEL | ES_READONLY | ES_AUTOHSCROLL |
                               WS_HSCROLL,
                             0,
                             0,
                             clientrect.right,
                             clientrect.bottom,
                             hWnd,
                             (HMENU) ID_OUTPUTWIN,
                             hInst,
                             NULL);
      FWFont=CreateFont(0,0,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,ANSI_CHARSET,
                        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
                        FIXED_PITCH | FF_DONTCARE,NULL);
      if (FWFont)
        SendMessage(outputwin,WM_SETFONT,(WPARAM) FWFont,MAKELPARAM(FALSE,0));
      SendMessage(outputwin,EM_FMTLINES,(WPARAM) FALSE,0);
      SendMessage(outputwin,EM_LIMITTEXT,(WPARAM) MAX_OUTPUTWIN_SIZE,0);
      SetWindowText(outputwin,"");
      break;
    case WM_SIZE:
      if (!outputwin)
        return DefWindowProc(hWnd,message,wParam,lParam);
      SetWindowPos(outputwin,NULL,0,0,LOWORD(lParam),HIWORD(lParam),
                   SWP_DRAWFRAME | SWP_NOZORDER);
      break;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDM_SAVE:
          console_save();
          break;
        case IDM_SHUTDOWN:
          if (MessageBox(hwnd,"Are you sure you want to shut down the system?",
                         "Shutdown",MB_OKCANCEL)==IDOK) {
            console_shutdown();
            exit(0);
          }
          break;
        case IDM_PANIC:
          if (MessageBox(hwnd,"Are you sure you want to panic?","Panic",
                         MB_OKCANCEL)==IDOK) {
            console_panic();
            exit(0);
          }
          break;
        case IDM_PAUSE:
          AddPausedToTitle();
          ModifyMenu(mainmenu,IDM_PAUSE,MF_BYCOMMAND | MF_STRING,
                     IDM_UNPAUSE,"Unpause Execution");
          break;
        case IDM_UNPAUSE:
          RemovePausedFromTitle();
          ModifyMenu(mainmenu,IDM_UNPAUSE,MF_BYCOMMAND | MF_STRING,
                     IDM_PAUSE,"Pause Execution");
          break;
        case IDM_AUTOLOAD:
          if (autoload) {
            autoload=FALSE;
            ModifyMenu(mainmenu,IDM_AUTOLOAD,MF_BYCOMMAND | MF_STRING |
                       MF_UNCHECKED,IDM_AUTOLOAD,"Auto Load Data");
          } else {
            autoload=TRUE;
            ModifyMenu(mainmenu,IDM_AUTOLOAD,MF_BYCOMMAND | MF_STRING |
                       MF_CHECKED,IDM_AUTOLOAD,"Auto Load Data");
          }
          break;
        case IDM_CONNECTIONS:
          AddPausedToTitle();
          DialogBox(hInst,"DLGConnections",hWnd,DialogConnections);
          RemovePausedFromTitle();
          break;
        case IDM_FILESYSTEM:
          AddPausedToTitle();
          DefaultFS=root_dir;
          DialogBox(hInst,"DLGFileSystem",hWnd,DialogFileSystem);
          RemovePausedFromTitle();
          break;
        case IDM_ABOUT:
          AddPausedToTitle();
          DialogBox(hInst,"DLGAbout",hWnd,DialogAbout);
          RemovePausedFromTitle();
          break;
        case IDM_BROADCAST:
          AddPausedToTitle();
          if (DialogBox(hInst,"DLGBroadcast",hWnd,DialogBroadcast)) {
            RemovePausedFromTitle();
            curr_who=next_who(NULL);
            sprintf(temp_buf,"console: broadcast: %s",bcast_buf);
            while (curr_who) {
              send_device(curr_who,temp_buf);
              send_device(curr_who,"\n");
              curr_who=next_who(curr_who);
            }
            logger(LOG, temp_buf);
            SelectRequestReturn();
          } else {
            RemovePausedFromTitle();
          }
          break;
        case IDM_RTSET:
          AddPausedToTitle();
          DialogBox(hInst,"DLGRTSet",hWnd,DialogRTSet);
          RemovePausedFromTitle();
          break;
        case IDM_CTSET:
          AddPausedToTitle();
          DialogBox(hInst,"DLGCTSet",hWnd,DialogCTSet);
          RemovePausedFromTitle();
          break;
        case IDM_DMON:
          dmode=1;
          ConstructTitle();
          EnableMenuItem(GetMenu(hWnd),IDM_DMOFF,MF_ENABLED);
          EnableMenuItem(GetMenu(hWnd),IDM_DMON,MF_GRAYED);
          break;
        case IDM_DMOFF:
          dmode=0;
          ConstructTitle();
          EnableMenuItem(GetMenu(hWnd),IDM_DMOFF,MF_GRAYED);
          EnableMenuItem(GetMenu(hWnd),IDM_DMON,MF_ENABLED);
          break;
        case IDM_SELECT:
          AddPausedToTitle();
          DialogBox(hInst,"DLGSelect",hWnd,DialogSelect);
          RemovePausedFromTitle();
          break;
        case IDM_MEMUSAGE:
          AddPausedToTitle();
#ifdef __MEMWATCH__
          DisplayMemUsage();
#else  /* __MEMWATCH__ */
          MessageBox(hWnd,"MemWatch Not Enabled","MemWatch",MB_OK);
#endif /* __MEMWATCH__ */
          RemovePausedFromTitle();
          break;
        case IDM_OBJECT:
          AddPausedToTitle();
          if (DialogBox(hInst,"DLGObjectSelect",hWnd,DialogObjectSelect))
            DialogBox(hInst,"DLGObject",hWnd,DialogObject);
          RemovePausedFromTitle();
          break;
        case IDM_PROTO:
          AddPausedToTitle();
          if (DialogBox(hInst,"DLGProtoSelect",hWnd,DialogProtoSelect)) {
            DefaultFunc=NULL;
            DialogBox(hInst,"DLGProto",hWnd,DialogProto);
          }
          RemovePausedFromTitle();
          break;
        case IDM_ALARM:
          AddPausedToTitle();
          DialogBox(hInst,"DLGAlarm",hWnd,DialogAlarm);
          RemovePausedFromTitle();
          break;
        case IDM_COMMAND:
          AddPausedToTitle();
          DialogBox(hInst,"DLGCommand",hWnd,DialogCommand);
          RemovePausedFromTitle();
          break;
        case IDM_DESTRUCT:
          AddPausedToTitle();
          DialogBox(hInst,"DLGDestruct",hWnd,DialogDestruct);
          RemovePausedFromTitle();
          break;
        default:
          return DefWindowProc(hWnd,message,wParam,lParam);
          break;
      }
      break;
    case WM_DESTROY:
      console_shutdown();
      PostQuitMessage(0);
      break;
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
  }
  return 0;
}

BOOL InitInstance(HANDLE hInstance,int nCmdShow) {
  hInst=hInstance;
  hwnd=CreateWindow("NetCIWClass",NETCI_NAME,WS_OVERLAPPEDWINDOW,
                    CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,
                    NULL,NULL,hInstance,NULL);
  if (!hwnd) return FALSE;
  ShowWindow(hwnd,nCmdShow);
  UpdateWindow(hwnd);
  return TRUE;
}

BOOL InitApplication(HANDLE hInstance) {
  WNDCLASS wc;

  wc.style=0;
  wc.lpfnWndProc=(WNDPROC) MainWndProc;
  wc.cbClsExtra=0;
  wc.cbWndExtra=0;
  wc.hIcon=LoadIcon(hInstance,"NetCIIcon");
  wc.hInstance=hInstance;
  wc.hCursor=LoadCursor(NULL,IDC_ARROW);
  wc.hbrBackground=GetStockObject(WHITE_BRUSH);
  wc.lpszMenuName="NetCIMenu";
  wc.lpszClassName="NetCIWClass";
  return RegisterClass(&wc);
}

void parse_command_line(char *cmd_line,int *p_argc,char ***p_argv) {
  int argc,loop,len;
  char **argv,*buf,*curr_arg;

  if (!cmd_line) {
    buf=strcpy(MALLOC(strlen(NETCI_NAME)+1),NETCI_NAME);
    argv=MALLOC(sizeof(char *));
    argv[0]=buf;
    argc=1;
  } else {
    buf=strcpy(MALLOC(strlen(cmd_line)+1),cmd_line);
    loop=0;
    argc=2;
    if (!(*cmd_line)) argc=1;
    while (buf[loop]) {
      if (buf[loop]==' ') {
        argc++;
        buf[loop]='\0';
      }
      loop++;
    }
    loop=1;
    curr_arg=buf;
    argv=MALLOC(argc*sizeof(char *));
    while (loop<argc) {
      len=strlen(curr_arg);
      argv[loop]=strcpy(MALLOC(len+1),curr_arg);
      curr_arg+=len+1;
      loop++;
    }
    FREE(buf);
    argv[0]=strcpy(MALLOC(strlen(NETCI_NAME)+1),NETCI_NAME);
  }
  *p_argc=argc;
  *p_argv=argv;
}

WINAPI WinMain(HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  int argc;
  char **argv;
  WORD VersionRequest;

  WinVersion=GetVersion();
  VersionRequest=MAKEWORD(1,1);
  if (WSAStartup(VersionRequest,&WinSockData)) {
	MessageBox(NULL,"Unable to start WinSock","Error",MB_OK | MB_ICONSTOP);
	exit(0);
  }
  if (WinSockData.wVersion!=MAKEWORD(1,1)) {
    WSACleanup();
	MessageBox(NULL,"WinSock v1.1 not supported","Error",MB_OK | MB_ICONSTOP);
	exit(0);
  }
  SelectWSAActivated=NULL;
  SelectFDList=NULL;
  SelectEventList=NULL;
  SelectTimeOutHasOccurred=FALSE;
  SelectTimerIsSet=FALSE;
  already_shutdown=FALSE;
  autoload=TRUE;
  DefaultRefno=0;
  DefaultProto=NULL;
  DefaultFunc=NULL;
  DefaultFS=NULL;
#ifdef DEBUG
  dmode=1;
#else /* DEBUG */
  dmode=0;
#endif /* DEBUG */
  is_paused=0;
  is_saving=FALSE;
  hInst=NULL;
  hwnd=NULL;
  outputwin=NULL;
  if (!hPrevInstance)
    if (!InitApplication(hInstance))
      return FALSE;
  if (!InitInstance(hInstance,nCmdShow))
    return FALSE;
  mainmenu=GetMenu(hwnd);
#ifdef __MEMWATCH__
  InitMemWatch(hwnd);
#else /* __MEMWATCH__ */
  DeleteMenu(mainmenu,IDM_MEMUSAGE,MF_BYCOMMAND);
  DrawMenuBar(hwnd);
#endif /* __MEMWATCH__ */
  window_title=NULL;
  parse_command_line(lpCmdLine,&argc,&argv);
#ifdef DEBUG
  EnableMenuItem(GetMenu(hwnd),IDM_DMOFF,MF_ENABLED);
  EnableMenuItem(GetMenu(hwnd),IDM_DMON,MF_GRAYED);
#else /* DEBUG */
  EnableMenuItem(GetMenu(hwnd),IDM_DMOFF,MF_GRAYED);
  EnableMenuItem(GetMenu(hwnd),IDM_DMON,MF_ENABLED);
  DeleteMenu(mainmenu,3,MF_BYPOSITION);
  DrawMenuBar(hwnd);
#endif /* DEBUG */
  DeleteMenu(mainmenu,IDM_UNPAUSE,MF_BYCOMMAND);
  main(argc,argv);
  exit(0);
  return 0;
}
