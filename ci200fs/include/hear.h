/* hear.h */

#define HEAR_SAY          1             /* Wizard says "Hi There!" */
#define HEAR_POSE         2             /* Wizard waves. */
#define HEAR_CONNECT      3             /* Wizard has connected. */
#define HEAR_DISCONNECT   4             /* Wizard has disconnected. */
#define HEAR_ENTER        5             /* Wizard has arrived. */
#define HEAR_LEAVE        6             /* Wizard has left. */
#define HEAR_ATTACK       7             /* Wizard starts attacking Phil! */
#define HEAR_DIE          8             /* Phil has died. */
#define HEAR_KILLOTHER    9             /* Tiny: Wizard killed Phil! */
#define HEAR_KILLYOU     10             /* Tiny: Wizard killed you! */
#define HEAR_KILLER      11             /* Tiny: You killed Phil! */
#define HEAR_WHISPER     12             /* Wizard whispers "Hi There!" */
#define HEAR_PAGE        13             /* Wizard pages: Hi There! */
#define HEAR_HOME        14             /* Wizard goes home. */
#define HEAR_FAIL        15             /* Wizard can't do whatever. */
#define HEAR_GET         16             /* Wizard gets whatever. */
#define HEAR_DROP        17             /* Wizard drops whatever. */
#define HEAR_LOOK        18             /* Wizard looks around. */
#define HEAR_STAGE       19             /* Wizard [to you]: Hi! */
#define HEAR_PASTE       20             /* pasting... */
#define HEAR_HIT         21             /* Wizard hits Phil hard! */
#define HEAR_MISS        22             /* Wizard misses Phil! */
#define HEAR_NOSPACE     23             /* Wizard's ball */

#define tell_player(X,Y,Z,W,V) ((X).hear((Y),(Z),(W),(V)))

#define tell_room(X,Y,Z,W,V) call_other(SYSOBJ,                          \
                                        "tell_room",                     \
                                        (X),(Y),(Z),(W),(V))

#define tell_room_except(X,Y,Z,W,V) call_other(SYSOBJ,                   \
                                               "tell_room_except",       \
                                               (X),(Y),(Z),(W),(V))

#define tell_room_except2(X,Y,Z,W,V) call_other(SYSOBJ,                  \
                                                "tell_room_except2",     \
                                                (X),(Y),(Z),(W),(V))
