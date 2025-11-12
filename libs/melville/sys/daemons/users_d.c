/* users_d.c - User Tracking Daemon for Melville/NetCI
 *
 * Centralized user tracking and management using the users() efun.
 *
 * Responsibilities:
 * - Track all logged-in users
 * - Provide fast user lookup by name
 * - Manage this_player() context
 * - Track connection and idle times
 * - Provide user statistics
 *
 * Architecture:
 * - Uses users() efun for efficient player discovery
 * - Maintains mapping for O(1) name lookups
 * - Event-driven updates on login/logout
 * - Integrated with cmd_hook for context tracking
 */

#include <std.h>
#include <config.h>
#include <roles.h>

/* User registry - mapping of lowercase names to player objects */
mapping user_registry;

/* Connection metadata */
mapping connection_times;
mapping last_command;

/* Current player context (set by cmd_hook) */
object current_player;

/* ========================================================================
 * INITIALIZATION
 * ======================================================================== */

static init() {
    /* Initialize data structures */
    user_registry = ([]);
    connection_times = ([]);
    last_command = ([]);
    current_player = 0;
    
    /* Initial scan of connected users */
    rehash_users();
}

/* ========================================================================
 * USER TRACKING - LOGIN/LOGOUT EVENTS
 * ======================================================================== */

/* Called when a user successfully logs in
 * Should be called from user.c after authentication
 */
user_logged_in(player) {
    string name;
    int now;
    
    if (!player) return;
    
    name = player->query_name();
    if (!name) return;
    
    /* Normalize to lowercase for case-insensitive lookup */
    name = downcase(name);
    
    /* Add to registry */
    user_registry[name] = player;
    
    /* Initialize timestamps */
    now = time();
    connection_times[name] = now;
    last_command[name] = now;
    
    syslog("users_d: User logged in: " + name);
}

/* Called when a user logs out
 * Should be called from user.c/player.c on disconnect
 */
user_logged_out(player) {
    string name;
    
    if (!player) return;
    
    name = player->query_name();
    if (!name) return;
    
    /* Normalize to lowercase */
    name = downcase(name);
    
    /* Remove from registry */
    map_delete(user_registry, name);
    map_delete(connection_times, name);
    map_delete(last_command, name);
    
    /* Clear context if this was current player */
    if (current_player == player) {
        current_player = 0;
    }
    
    syslog("users_d: User logged out: " + name);
}

/* ========================================================================
 * CONTEXT MANAGEMENT - this_player() TRACKING
 * ======================================================================== */

/* Set current player context - called by cmd_hook before command execution */
set_this_player(player) {
    string name;
    
    current_player = player;
    
    if (!player) return;
    
    /* Update last command timestamp */
    name = player->query_name();
    if (name) {
        name = downcase(name);
        last_command[name] = time();
    }
}

/* Clear current player context - called by cmd_hook after command execution */
clear_this_player() {
    current_player = 0;
}

/* Get current player context - alternative to this_player() efun */
query_this_player() {
    return current_player;
}

/* ========================================================================
 * USER LOOKUP
 * ======================================================================== */

/* Find user by name (case-insensitive)
 * Returns player object or 0 if not found
 */
find_user(name) {
    object player;
    object *all_users;
    string pname;
    int i;
    
    if (!name || name == "") return 0;
    
    /* Normalize name */
    name = downcase(trim(name));
    
    /* Check registry first (fast O(1) lookup) */
    if (member(user_registry, name)) {
        player = user_registry[name];
        if (player) {
            /* Try to verify - if this fails, fall through to users() scan */
            pname = call_other(player, "query_name");
            if (pname && downcase(pname) == name) {
                return player;
            }
        }
        /* If validation failed or player was 0, fall through to users() scan */
    }
    
    /* Fallback: search through users() efun (slower but accurate)
     * This handles cases where registry is out of sync
     */
    all_users = users();
    if (!all_users) return 0;
    
    for (i = 0; i < sizeof(all_users); i++) {
        if (all_users[i]) {
            pname = call_other(all_users[i], "query_name");
            if (pname) {
                if (downcase(pname) == name) {
                    /* Found it - update registry */
                    user_registry[name] = all_users[i];
                    return all_users[i];
                }
            }
        }
    }
    
    return 0;
}

/* ========================================================================
 * USER QUERIES
 * ======================================================================== */

/* Get array of all connected users (from driver) */
query_users() {
    return users();
}

/* Get count of connected users */
query_user_count() {
    object *all_users;
    
    all_users = users();
    if (!all_users) return 0;
    
    return sizeof(all_users);
}

/* Get array of user names (sorted) */
query_user_names() {
    object *all_users;
    string *names;
    string name;
    int i;
    
    all_users = users();
    if (!all_users) return ({});
    
    names = ({});
    for (i = 0; i < sizeof(all_users); i++) {
        if (all_users[i]) {
            name = call_other(all_users[i], "query_name");
            if (name) {
                names += ({ name });
            }
        }
    }
    
    return sort_array(names);
}

/* ========================================================================
 * STATISTICS AND METADATA
 * ======================================================================== */

/* Get connection time in seconds for a player */
query_connection_time(player) {
    string name;
    int conn_time;
    
    if (!player) return 0;
    
    name = call_other(player, "query_name");
    if (!name) return 0;
    
    name = downcase(name);
    
    /* Check if we have connection time */
    conn_time = connection_times[name];
    if (!conn_time) return 0;
    
    return time() - conn_time;
}

/* Get idle time in seconds for a player */
query_idle_time(player) {
    string name;
    int last_cmd;
    
    if (!player) return 0;
    
    name = call_other(player, "query_name");
    if (!name) return 0;
    
    name = downcase(name);
    
    /* Check if we have last command time */
    last_cmd = last_command[name];
    if (!last_cmd) return 0;
    
    return time() - last_cmd;
}

/* Get statistics about all users */
query_user_stats() {
    mapping stats;
    object *all_users;
    string *names;
    int i;
    int total_conn_time;
    int total_idle_time;
    int count;
    int conn_time;
    int idle_time;
    
    stats = ([]);
    
    /* Get user count from driver */
    all_users = users();
    stats["total_users"] = all_users ? sizeof(all_users) : 0;
    
    /* Get registered user count */
    names = keys(user_registry);
    stats["registered_users"] = sizeof(names);
    
    /* Calculate averages */
    total_conn_time = 0;
    total_idle_time = 0;
    count = 0;
    
    for (i = 0; i < sizeof(names); i++) {
        if (connection_times[names[i]] && last_command[names[i]]) {
            conn_time = time() - connection_times[names[i]];
            idle_time = time() - last_command[names[i]];
            
            total_conn_time += conn_time;
            total_idle_time += idle_time;
            count++;
        }
    }
    
    if (count > 0) {
        stats["avg_connection_time"] = total_conn_time / count;
        stats["avg_idle_time"] = total_idle_time / count;
    } else {
        stats["avg_connection_time"] = 0;
        stats["avg_idle_time"] = 0;
    }
    
    return stats;
}

/* ========================================================================
 * MAINTENANCE
 * ======================================================================== */

/* Rebuild user registry from users() efun
 * Useful if registry gets out of sync or after daemon reload
 */
rehash_users() {
    object *all_users;
    string name;
    int i;
    int now;
    
    syslog("users_d: Rehashing user registry...");
    
    /* Get all connected users from driver */
    all_users = users();
    if (!all_users) {
        syslog("users_d: No users connected");
        return;
    }
    
    /* Clear and rebuild registry */
    user_registry = ([]);
    
    now = time();
    
    for (i = 0; i < sizeof(all_users); i++) {
        if (all_users[i]) {
            name = call_other(all_users[i], "query_name");
            if (name) {
                /* Normalize name */
                name = downcase(name);
                
                /* Add to registry */
                user_registry[name] = all_users[i];
                
                /* Initialize metadata if missing */
                if (!connection_times[name]) {
                    connection_times[name] = now;
                }
                if (!last_command[name]) {
                    last_command[name] = now;
                }
            }
        }
    }
    
    syslog("users_d: Rehashed " + itoa(sizeof(keys(user_registry))) + " users");
}
