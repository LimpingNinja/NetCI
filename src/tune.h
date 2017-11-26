/* tune.h */

/* this header file contains tune-able parameters. the defaults are
   probably ok, but as the database grows, you may wish to modify
   them */

#define CYCLE_HARD_MAX 1000000

#define CYCLE_SOFT_MAX 100000

#define MAX_CONNS 512      /* max # players that can be connected at
                              one time */
#define MIN_FREE_FILES 3   /* at least this many files are GUARANTEED
                              to be openable by CI objects while the
                              system is running.  3 is a good number. */

#define WRITE_BURST 256    /* number of characters to write over the
                              net at a time */


#define ITOA_BUFSIZ 32     /* max # chars a signed long will take in
                              string form - 32 will cover the max
                              size of a 96-bit signed integer */

#define NUM_ELINES 32      /* number of lines in a block of edit buffer */

#define MAX_OUTBUF_LEN 16359  /* maximum amount of output buffered */

#define OBJ_ALLOC_BLKSIZ 8 /* chunk-size for object allocation */
#define CACHE_SIZE 8    /* number of objects to keep in the cache at one
                              time. it's a soft maximum, can be temporarily
                              overridden */
#define CACHE_HASH 8     /* the size of the hash table to hash cached objects
                              into */
#define MAX_STR_LEN 8191   /* maximum string variable length */
                           /* applies only to constants while compiling */
                           /* no maximum is applied to strings constructed */
                           /* during run-time execution */

#define EBUFSIZ 2047       /* #define expansion buffer size */

#define MAX_DEPTH 8        /* maximum #define recursion */
