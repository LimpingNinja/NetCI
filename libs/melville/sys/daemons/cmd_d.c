/* cmd_d.c - Command Daemon for Melville/NetCI
 *
 * This daemon manages command discovery and execution.
 * Based on the TMI-2 cmd_d.c pattern.
 *
 * Responsibilities:
 * - Maintain command table (mapping of directories to command lists)
 * - Find commands based on player privileges
 * - Execute commands with proper security
 * - Support command rehashing when new commands are added
 *
 * Architecture:
 * - Commands are stored in /cmds/{player,builder,wizard,admin}/
 * - Each command is a file named _<verb>.c with a do_command() function
 * - Command table caches available commands per directory
 * - Players can only access commands for their role level and below
 */

#include <std.h>
#include <config.h>
#include <roles.h>

/* Command table: mapping of directory paths to arrays of command names */
mapping cmd_table;

/* Command aliases: mapping of short forms to full commands */
mapping aliases;

/* ========================================================================
 * INITIALIZATION
 * ======================================================================== */

static init() {
    /* Initialize command table */
    cmd_table = ([]);
    
    /* Initialize command aliases */
    aliases = ([
        "i": "inventory",
        "l": "look",
        "n": "go north",
        "s": "go south",
        "e": "go east",
        "w": "go west",
        "u": "go up",
        "d": "go down",
        "ne": "go northeast",
        "nw": "go northwest",
        "se": "go southeast",
        "sw": "go southwest"
    ]);
    
    /* Load all command directories */
    rehash(PLAYER_CMD_DIR);
    rehash(BUILDER_CMD_DIR);
    rehash(WIZARD_CMD_DIR);
    rehash(ADMIN_CMD_DIR);
}

/* ========================================================================
 * COMMAND PROCESSING
 * ======================================================================== */

/* Main entry point - called from player.cmd_hook()
 * Processes a command line and executes the appropriate command
 */
process_command(player, input) {
    string verb, args, cmd_path, *search_path;
    object cmd_ob;
    int result;
    
    if (!player || !input) return;
    
    /* Check for command aliases and expand if found */
    if (aliases[input]) {
        input = aliases[input];
    }
    
    /* Parse verb and arguments using sscanf */
    if (sscanf(input, "%s %s", verb, args) == 2) {
        /* Got both verb and args */
    } else if (sscanf(input, "%s", verb) == 1) {
        /* Got only verb, no args */
        args = NULL;
    } else {
        /* Empty input */
        verb = NULL;
        args = NULL;
    }
        
    /* Get search path based on player's role */
    search_path = get_search_path(player);
    
    /* Find command in search path */
    cmd_path = find_command(verb, search_path);
    
    if (!cmd_path) {
        /* Command not found */

        player.listen("What?\n");
        return;
    }
    
    /* Load command object using get_object() from auto.c
     * This will compile if needed and return the prototype
     * Tests the auto-object delegation to boot.c!
     */
    cmd_ob = get_object(cmd_path);
    
    if (!cmd_ob) {
        player.listen("Error loading command.\n");
        syslog("Failed to load command: " + cmd_path);
        return;
    }
    
    /* Grant privilege to admin commands */
    if (leftstr(cmd_path, 12) == "/cmds/admin/") {
        set_priv(cmd_ob, 1);
    }
    
    /* Execute command using do_command(player, args) callback
     * Pass player explicitly rather than relying on this_player()
     */
    result = cmd_ob.do_command(player, args);
    
    /* Commands return 1 on success, 0 on failure */
    if (!result) {
        player.listen("Command failed.\n");
    }
}

/* ========================================================================
 * COMMAND DISCOVERY
 * ======================================================================== */

/* Get a command based on player and verb */
get_command(object player, string verb) {
    string *search_path;
    
    search_path = get_search_path(player);
    return find_command(verb, search_path);
}

/* Get all commands organized by role directory
 * Returns mapping: ([ "admin": ({ "eval", "exec" }), "player": ({ "say", "look" }) ])
 */
get_commands(object player) {
    string *search_path, dir, role;
    mapping result;
    int i, pos;
    
    search_path = get_search_path(player);
    result = ([]);
    
    for (i = 0; i < sizeof(search_path); i++) {
        dir = search_path[i];
        
        /* Ensure directory is in command table */
        if (!cmd_table[dir]) {
            rehash(dir);
        }
        
        /* Add commands from this directory using just the role name */
        if (cmd_table[dir]) {
            /* Extract role from path: "/cmds/admin" -> "admin" */
            pos = strlen(dir) - 1;
            while (pos >= 0 && midstr(dir, pos, pos) != "/") {
                pos = pos - 1;
            }
            role = midstr(dir, pos + 1, strlen(dir) - 1);
            result[role] = cmd_table[dir];
        }
    }
    
    return result;
}

/* Get flat array of all commands available to player
 * Returns array: ({ "eval", "exec", "rehash", "say", "look", "who", ... })
 */
get_player_commands(object player) {
    string *search_path, dir, *cmds;
    string *result;
    int i;
    
    search_path = get_search_path(player);
    result = ({});
    
    for (i = 0; i < sizeof(search_path); i++) {
        dir = search_path[i];
        
        /* Ensure directory is in command table */
        if (!cmd_table[dir]) {
            rehash(dir);
        }
        
        /* Append commands from this directory */
        if (cmd_table[dir]) {
            cmds = cmd_table[dir];
            if (cmds) {
                result = result + cmds;
            }
        }
    }
    
    return result;
}

/* Get all commands within a specific role
 * Returns array: ({ "eval", "exec", "rehash" })
 */
get_role_commands(string role) {
    string dir;
    
    if (!role) return ({});
    
    /* Build directory path */
    dir = "/cmds/" + role;
    
    /* Ensure directory is in command table */
    if (!cmd_table[dir]) {
        rehash(dir);
    }
    
    /* Return commands for this role */
    if (cmd_table[dir]) {
        return cmd_table[dir];
    }
    
    return ({});
}

/* Get all commands from all roles
 * Returns array: ({ "eval", "exec", "rehash", "say", "look", "who", ... })
 */
get_all_commands() {
    string *dirs, *cmds;
    string *result;
    int i;
    
    dirs = keys(cmd_table);
    result = ({});
    
    for (i = 0; i < sizeof(dirs); i++) {
        if (cmd_table[dirs[i]]) {
            cmds = cmd_table[dirs[i]];
            if (cmds) {
                result = result + cmds;
            }
        }
    }
    
    return result;
}

/* Find a command in the given search path
 * Returns full path to command file (without .c extension)
 */
find_command(string verb, string *search_path) {
    string dir, cmd_file, *cmds;
    int i;
    
    if (!verb || !search_path) return NULL;
    
    /* Search each directory in order */
    for (i = 0; i < sizeof(search_path); i++) {
        dir = search_path[i];
        
        /* Ensure directory is in command table */
        if (!cmd_table[dir]) {
            rehash(dir);
        }
        
        /* Check if command exists in this directory */
        if (cmd_table[dir]) {
            cmds = cmd_table[dir];
            if (cmds && member_array(verb, cmds) != -1) {
                /* Found it! */
                cmd_file = dir + "/_" + verb;
                return cmd_file;
            }
        }
    }
    
    return NULL;
}

/* Get command search path based on player's role
 * Higher roles can access lower role commands
 */
get_search_path(player) {
    string *path;
    int role;
    
    if (!player) return ({});
    
    role = player.query_role();
    
    /* Build search path from highest to lowest privilege */
    path = ({});
    
    if (role >= ROLE_ADMIN) {
        path = path + ({ ADMIN_CMD_DIR });
    }
    
    if (role >= ROLE_WIZARD) {
        path = path + ({ WIZARD_CMD_DIR });
    }
    
    if (role >= ROLE_BUILDER) {
        path = path + ({ BUILDER_CMD_DIR });
    }
    
    /* Everyone gets player commands */
    path = path + ({ PLAYER_CMD_DIR });
    
    return path;
}

/* ========================================================================
 * COMMAND TABLE MANAGEMENT
 * ======================================================================== */

/* Rehash a command directory - scan for _*.c files
 * Returns 1 on success, 0 on failure
 */
rehash(dir) {
    string *files, *cmds, file, verb;
    int i;
    
    if (!dir) return 0;
    
    /* Check if directory exists using fstat */
    if (!fstat(dir)) {
        return 0;
    }
    
    /* Get directory listing */
    files = get_dir_list(dir);
    if (!files) {
        cmd_table[dir] = ({});
        return 1;
    }
    
    /* Extract command names from _*.c files */
    cmds = ({});
    for (i = 0; i < sizeof(files); i++) {
        file = files[i];
        
        /* Extract command name from _verb.c pattern using sscanf */
        if (sscanf(file, "_%s.c", verb) == 1) {
            cmds = cmds + ({ verb });
        }
    }
    
    /* Update command table */
    cmd_table[dir] = cmds;
    
    return 1;
}

/* Get directory listing using get_dir() efun
 * Returns array of filenames in the directory
 */
get_dir_list(dir) {
    string *files;
    
    /* Use get_dir() to get actual directory contents */
    files = get_dir(dir);
    
    if (!files) {
        syslog("WARNING: cmd_d.get_dir_list() - could not read directory: " + dir);
        return ({});
    }
    
    return files;
}

/* ========================================================================
 * UTILITY FUNCTIONS
 * ======================================================================== */

/* Parse input into verb and arguments
 * Returns 1 if arguments found, 0 if just verb
 */
parse_input(input, verb, args) {
    int space_pos;
    
    if (!input) return 0;
    
    /* Find first space */
    space_pos = instr(input, 0, " ");
    
    if (space_pos == -1) {
        /* No arguments */
        verb = input;
        args = NULL;
        return 0;
    }
    
    /* Split into verb and args */
    verb = leftstr(input, space_pos);
    args = rightstr(input, strlen(input) - space_pos - 1);
    
    return 1;
}

/* List all commands in a directory */
list_commands(dir) {
    if (!cmd_table[dir]) {
        rehash(dir);
    }
    
    return cmd_table[dir];
}

/* Show all available commands for a player */
show_commands(player) {
    string *search_path, dir, *cmds, output;
    int i, j;
    
    if (!player) return NULL;
    
    search_path = get_search_path(player);
    output = "Available commands:\n\n";
    
    for (i = 0; i < sizeof(search_path); i++) {
        dir = search_path[i];
        
        if (!cmd_table[dir]) {
            rehash(dir);
        }
        
        cmds = cmd_table[dir];
        if (cmds && sizeof(cmds) > 0) {
            output = output + "From " + dir + ":\n";
            
            for (j = 0; j < sizeof(cmds); j++) {
                output = output + "  " + cmds[j];
                
                /* Add newline every 4 commands for readability */
                if ((j + 1) % 4 == 0) {
                    output = output + "\n";
                } else {
                    output = output + "  ";
                }
            }
            
            output = output + "\n\n";
        }
    }
    
    return output;
}

/* Rehash all command directories */
rehash_all() {
    rehash(PLAYER_CMD_DIR);
    rehash(BUILDER_CMD_DIR);
    rehash(WIZARD_CMD_DIR);
    rehash(ADMIN_CMD_DIR);
    
    return 1;
}
