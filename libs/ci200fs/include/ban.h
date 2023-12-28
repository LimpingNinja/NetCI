/* ban.h */

/* This object holds the core of the ban code */

int BanObjNum;

#define BANOBJ (BanObjNum?itoo(BanObjNum):                       \
		itoo(BanObjNum=otoi(atoo("/sys/ban"))))

#define site_status(X) call_other(BANOBJ,"site_status",(X))

#define reload_sites() call_other(BANOBJ,"reload_sites")

#define list_sites() call_other(BANOBJ,"list_sites")

