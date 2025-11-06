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

/* ========================================================================
 * INITIALIZATION
 * ======================================================================== */

static init() {
    /* Initialize command table */
    cmd_table = ([]);
    
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
    int result, space_pos;
    
    syslog("DEBUG: cmd_d.process_command called - player: " + otoa(player) + ", input: '" + input + "'");
    
    if (!player || !input) {
        syslog("DEBUG: cmd_d - player or input is null, returning");
        return;
    }
    
    /* Parse verb and arguments - simple split on first space */
    space_pos = instr(input, 1, " ");  /* NetCI uses 1-based indexing */
    if (space_pos == 0) {
        /* No space found, entire input is the verb */
        verb = input;
        args = NULL;
    } else {
        /* Split: verb is before space, args is after */
        verb = leftstr(input, space_pos - 1);
        args = rightstr(input, strlen(input) - space_pos);
    }
        
    /* Get search path based on player's role */
    search_path = get_search_path(player);
    
    /* Find command in search path */
    cmd_path = find_command(verb, search_path);
    syslog("DEBUG: cmd_d - find_command returned: " + (cmd_path ? cmd_path : "NULL"));
    
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
    
    /* Execute command using execute() callback
     * Commands now use this_player() internally, so we only pass args
     */
    if (args) {
        result = cmd_ob.execute(args);
    } else {
        result = cmd_ob.execute();
    }
    
    /* Commands return 1 on success, 0 on failure */
    if (!result) {
        player.listen("Command failed.\n");
    }
}

/* ========================================================================
 * COMMAND DISCOVERY
 * ======================================================================== */

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
        
        /* Check if file starts with _ and ends with .c */
        if (strlen(file) > 3 && leftstr(file, 1) == "_") {
            /* Remove leading _ and trailing .c */
            if (rightstr(file, 2) == ".c") {
                /* midstr is 1-indexed! pos=2 to skip the _, len=strlen-3 to remove _.c */
                verb = midstr(file, 2, strlen(file) - 3);
                cmds = cmds + ({ verb });
            }
        }
    }
    
    /* Update command table */
    cmd_table[dir] = cmds;
    
    return 1;
}

/* Get directory listing
 * NOTE: ls() calls listen() on each file, we need a different approach
 * For now, we'll use a simple file existence check pattern
 */
get_dir_list(dir) {
    string *common_verbs, *files, verb, path;
    int i, stat_result;
    
    /* TODO: This is a workaround until we implement proper directory scanning
     * For now, check for common command verbs
     */
    common_verbs = ({ "look", "go", "get", "drop", "inventory", "quit", 
                      "say", "emote", "who", "help", "save" });
    
    files = ({});
    
    for (i = 0; i < sizeof(common_verbs); i++) {
        verb = common_verbs[i];
        path = dir + "/_" + verb + ".c";
        stat_result = fstat(path);
        if (stat_result > 0) {
            files = files + ({ "_" + verb + ".c" });
        }
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
