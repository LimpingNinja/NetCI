
Section 8 - tell_room

#include <sys.h>
#include <hear.h>

tell_room(room,actor,actee,type,message)

object room,actor,actee;
int type;
string message;

This sends a specified message to every hear() function defined on every
object in the specified room.

See Also: hear(9), tell_room_except(8), tell_room_except2(8)

