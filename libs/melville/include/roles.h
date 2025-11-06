/* roles.h - Role definitions and flags for Melville/NetCI */

/* Role Levels (hierarchical) */
#define ROLE_PLAYER     1
#define ROLE_BUILDER    2
#define ROLE_PROGRAMMER 4
#define ROLE_WIZARD     8
#define ROLE_ADMIN      16

/* Role Flags (bitmask - can have multiple) */
#define FLAG_PLAYER     1       /* Basic player */
#define FLAG_BUILDER    2       /* Can build areas */
#define FLAG_PROGRAMMER 4       /* Can write code */
#define FLAG_WIZARD     8       /* Full wizard powers */
#define FLAG_ADMIN      16      /* Administrator */
#define FLAG_IMMORTAL   32      /* Cannot be killed */
#define FLAG_INVISIBLE  64      /* Invisible to players */
#define FLAG_GUEST      128     /* Guest account */

/* Permission Flags (for files/objects) */
#define PERM_READ       1       /* Can be read */
#define PERM_WRITE      2       /* Can be written */
#define PERM_EXECUTE    4       /* Can be executed */
#define PERM_DELETE     8       /* Can be deleted */

/* Helper Macros */
#define IS_ADMIN(obj)       ((obj).get_flags() & FLAG_ADMIN)
#define IS_WIZARD(obj)      ((obj).get_flags() & FLAG_WIZARD)
#define IS_PROGRAMMER(obj)  ((obj).get_flags() & FLAG_PROGRAMMER)
#define IS_BUILDER(obj)     ((obj).get_flags() & FLAG_BUILDER)
#define IS_PLAYER(obj)      ((obj).get_flags() & FLAG_PLAYER)

#define HAS_FLAG(obj, flag) ((obj).get_flags() & (flag))
