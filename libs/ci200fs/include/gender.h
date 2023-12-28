/* gender.h */

#define GENDER_NONE 0
#define GENDER_MALE 1
#define GENDER_FEMALE 2
#define GENDER_NEUTER 3
#define GENDER_PLURAL 4
#define GENDER_SPIVAK 5

#define gender_subs(X,Y) call_other(SYSOBJ,"gender_subs",(X),(Y))
