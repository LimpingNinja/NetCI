/* instr.h */

/* contains the definitions for the object-code instructions */

#define NUM_OPERS      38
#define NUM_SCALLS     105

#define COMMA_OPER     0    /*  ,   */
#define EQ_OPER        1    /*  =   */
#define PLEQ_OPER      2    /*  +=  */
#define MIEQ_OPER      3    /*  -=  */
#define MUEQ_OPER      4    /*  *=  */
#define DIEQ_OPER      5    /*  /=  */
#define MOEQ_OPER      6    /*  %=  */
#define ANEQ_OPER      7    /*  &=  */
#define EXEQ_OPER      8    /*  ^=  */
#define OREQ_OPER      9    /*  |=  */
#define LSEQ_OPER      10   /*  <<= */
#define RSEQ_OPER      11   /*  >>= */
#define COND_OPER      12   /*  ?   */
#define OR_OPER        13   /*  ||  */
#define AND_OPER       14   /*  &&  */
#define BITOR_OPER     15   /*  |   */
#define EXOR_OPER      16   /*  ^   */
#define BITAND_OPER    17   /*  &   */
#define CONDEQ_OPER    18   /*  ==  */
#define NOTEQ_OPER     19   /*  !=  */
#define LESS_OPER      20   /*  <   */
#define LESSEQ_OPER    21   /*  <=  */
#define GREAT_OPER     22   /*  >   */
#define GREATEQ_OPER   23   /*  >=  */
#define LS_OPER        24   /*  <<  */
#define RS_OPER        25   /*  >>  */
#define ADD_OPER       26   /*  +   */
#define MIN_OPER       27   /*  -   */
#define MUL_OPER       28   /*  *   */
#define DIV_OPER       29   /*  /   */
#define MOD_OPER       30   /*  %   */
#define NOT_OPER       31   /*  !   */
#define BITNOT_OPER    32   /*  ~   */
#define POSTADD_OPER   33   /*  ++  */
#define PREADD_OPER    34   /*  ++  */
#define POSTMIN_OPER   35   /*  --  */
#define PREMIN_OPER    36   /*  --  */
#define UMIN_OPER      37   /*  -   */

/* System Calls */

/* Object Commands */

#define S_ADD_VERB       38 /* PROTO: add_verb(string action, string func) */
#define S_ADD_XVERB      39 /* PROTO: add_xverb(string action, string func) */
#define S_CALL_OTHER     40 /* call_other(object obj, string func, ...) */
#define S_ALARM          41 /* alarm(int delay, string func) */
#define S_REMOVE_ALARM   42 /* remove_alarm([string func]) */
#define S_CALLER_OBJECT  43 /* caller_object() */
#define S_CLONE_OBJECT   44 /* clone_object(object obj|string path) */
#define S_DESTRUCT       45 /* destruct(object obj) */
#define S_CONTENTS       46 /* contents(object obj) */
#define S_NEXT_OBJECT    47 /* next_object(object obj) */
#define S_LOCATION       48 /* location(object obj) */
#define S_NEXT_CHILD     49 /* next_child(object obj) */
#define S_PARENT         50 /* parent(object obj) */
#define S_NEXT_PROTO     51 /* next_proto(object obj) */
#define S_MOVE_OBJECT    52 /* move_object(object item, object dest) */
#define S_THIS_OBJECT    53 /* this_object() */
#define S_THIS_PLAYER    54 /* this_player() */

/* Flag Setting & Reading */

#define S_SET_INTERACTIVE 55 /* set_interactive(int bool) */
#define S_INTERACTIVE    56 /* interactive(object obj) */
#define S_SET_PRIV       57 /* PRIV: set_priv(object obj, int bool) */
#define S_PRIV           58 /* priv(object obj) */
#define S_IN_EDITOR      59 /* in_editor(object obj) */
#define S_CONNECTED      60 /* connected(object obj) */

/* Device Functions */

#define S_GET_DEVCONN    61 /* get_devconn(object obj) */
#define S_SEND_DEVICE    62 /* send_device(string msg) */
#define S_RECONNECT_DEVICE 63 /* PRIV: reconnect_device(object obj) */
#define S_DISCONNECT_DEVICE 64 /* disconnect device() */

/* Miscellaneous Functions */

#define S_RANDOM         65 /* random(int limit) */
#define S_TIME           66 /* time() */
#define S_MKTIME         67 /* mktime(int tm) */
#define S_TYPEOF         68 /* typeof(var x) */
#define S_COMMAND        69 /* command(string action) */

/* File Handling Functions */

#define S_COMPILE_OBJECT 70 /* compile_object(string path) */
#define S_EDIT           71 /* edit(string path) */
#define S_CAT            72 /* cat(string path) */
#define S_LS             73 /* ls(string path) */
#define S_RM             74 /* rm(string path) */
#define S_CP             75 /* cp(string path, string destpath) */
#define S_MV             76 /* mv(string path, string newpath) */
#define S_MKDIR          77 /* mkdir(string path) */
#define S_RMDIR          78 /* rmdir(string path) */
#define S_HIDE           79 /* PRIV: hide(string path) */
#define S_UNHIDE         80 /* PRIV: unhide(string path, object owner,
                                            int flags) */
#define S_CHOWN          81 /* chown(string path, object owner) */
#define S_SYSLOG         82 /* PRIV: syslog(string msg) */

/* String Manipulation */

#define S_SSCANF         83 /* sscanf(string s, string format, ...) */
#define S_SPRINTF        84 /* sprintf(string s, string format, ...) */
#define S_MIDSTR         85 /* midstr(string s, int pos, int len) */
#define S_STRLEN         86 /* strlen(s) */
#define S_LEFTSTR        87 /* leftstr(string s, int len) */
#define S_RIGHTSTR       88 /* rightstr(string s, int len) */
#define S_SUBST          89 /* subst(string s, int pos, int len, string s2) */
#define S_INSTR          90 /* instr(string s, int startpos, string search) */
#define S_OTOA           91 /* otoa(object obj) */
#define S_ITOA           92 /* itoa(int val) */
#define S_ATOI           93 /* atoi(string s) */
#define S_ATOO           94 /* atoo(string s) */
#define S_UPCASE         95 /* upcase(string s) */
#define S_DOWNCASE       96 /* downcase(string s) */
#define S_IS_LEGAL       97 /* is_legal(string s) */

/* Stuff I Forgot and Added Later */

#define S_OTOI           98 /* otoi(object o) */
#define S_ITOO           99 /* itoo(int i) */
#define S_CHMOD         100 /* chmod(string path, int flags) */
#define S_FREAD         101 /* fread(string pathname, int pos) */
#define S_FWRITE        102 /* fwrite(string pathname, string s) */
#define S_REMOVE_VERB   103 /* remove_verb(string action) */
#define S_FERASE        104 /* ferase(string pathname) */
#define S_CHR           105 /* chr(int c) */
#define S_ASC           106 /* asc(string c) */
#define S_SYSCTL        107 /* PRIV: sysctl(int oper, ...) */
#define S_PROTOTYPE     108 /* prototype(object obj) */
#define S_ITERATE       109 /* iterate(object obj, string func, ...) */
#define S_NEXT_WHO      110 /* next_who(object obj) */
#define S_GET_DEVIDLE   111 /* get_devidle(object obj) */
#define S_GET_CONNTIME  112 /* get_conntime(object obj) */
#define S_CONNECT_DEVICE 113 /* connect_device(string address,int port) */
#define S_FLUSH_DEVICE  114 /* flush_device() */
#define S_ATTACH        115 /* attach(object obj) */
#define S_THIS_COMPONENT 116 /* this_component() */
#define S_DETACH        117 /* detach(object obj) */
#define S_TABLE_GET     118 /* table_get(string key) */
#define S_TABLE_SET     119 /* table_set(string key, string datum) */
#define S_TABLE_DELETE  120 /* table_delete(string key) */
#define S_FSTAT         121 /* fstat(string filename) */
#define S_FOWNER        122 /* fowner(string filename) */
#define S_GET_HOSTNAME  123 /* get_hostname(string ipaddr) */
#define S_GET_ADDRESS   124 /* get_address(string hostname) */
#define S_SET_LOCALVERBS 125 /* set_localverbs(int bool) */
#define S_LOCALVERBS    126 /* localverbs(object obj) */
#define S_NEXT_VERB     127 /* next_verb(object obj, string verb) */
#define S_GET_DEVPORT   128 /* get_devport(object obj) */
#define S_GET_DEVNET    129 /* get_devnet(object obj) */
#define S_REDIRECT_INPUT 130 /* redirect_input(string funcname) */
#define S_GET_INPUT_FUNC 131 /* get_input_func() */
#define S_GET_MASTER    132 /* get_master(object obj) */
#define S_IS_MASTER     133 /* is_master(object obj) */

/* Array Functions */

#define S_SIZEOF        135 /* sizeof(array arr) - returns array length */
#define S_IMPLODE       136 /* implode(array arr, string sep) - join array into string */
#define S_EXPLODE       137 /* explode(string str, string sep) - split string into array */
#define S_MEMBER_ARRAY  138 /* member_array(mixed elem, array arr) - find element in array */
#define S_SORT_ARRAY    139 /* sort_array(array arr) - sort array in place */
#define S_REVERSE       140 /* reverse(array arr) - reverse array in place */
#define S_UNIQUE_ARRAY  141 /* unique_array(array arr) - remove duplicates */
#define S_ARRAY_LITERAL 142 /* ({ elem1, elem2, ... }) - create array literal */
