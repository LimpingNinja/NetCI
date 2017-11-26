/* winmain.h */

#define SELMODE_LISTEN 1
#define SELMODE_DATA 0

void AddSavingToTitle();
void RemoveSavingFromTitle();
void ConstructTitle();
void AddText(char *buf);
int NT_select(int tablesize,FD_SET *readfds,FD_SET *writefds,FD_SET *exceptfds,
              struct timeval *timeout);
void SelectWSARemove(SOCKET fd);
void SelectWSAPrepare(SOCKET fd,int mode);


