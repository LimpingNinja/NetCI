/* std.h - Standard includes for Melville/NetCI */

/* typeof() Type Constants (from ci200fs) */
#define INT_T       0
#define STRING_T    1
#define OBJECT_T    2

/* Object Types */
#define TYPE_UNKNOWN    1
#define TYPE_OBJECT     2
#define TYPE_ROOM       3
#define TYPE_PLAYER     4
#define TYPE_NPC        5
#define TYPE_ITEM       6
#define TYPE_CONTAINER  7
#define TYPE_DOOR       8
#define TYPE_EXIT       9

/* Message Types (for tell_room, tell_player) */
#define HEAR_SAY        1
#define HEAR_EMOTE      2
#define HEAR_WHISPER    3
#define HEAR_SHOUT      4
#define HEAR_TELL       5
#define HEAR_ENTER      6
#define HEAR_LEAVE      7
#define HEAR_ACTION     8

/* Log Levels */
#define LOG_DEBUG       0
#define LOG_INFO        1
#define LOG_WARN        2
#define LOG_ERROR       3
#define LOG_FATAL       4

/* Gender Constants */
#define GENDER_NEUTER   0
#define GENDER_MALE     1
#define GENDER_FEMALE   2
#define GENDER_PLURAL   3

/* Common String Constants */
#define NEWLINE         "\n"
#define SPACE           " "
#define EMPTY_STRING    ""

/* Boolean Constants */
#define TRUE    1
#define FALSE   0
#define NULL    0
