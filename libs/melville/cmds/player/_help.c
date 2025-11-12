/* _help.c - Display help topics from text files
 *
 * Syntax: help [topic]
 *
 * Display help information from /help/ directory.
 */

#include <std.h>
#include <config.h>

/* Main command execution */
do_command(player, args) {
    string topic, help_path, help_text, output, cmd_path;
    object cmd_d, cmd_obj;
    
    if (!player) return 0;
    
    /* Default to index if no topic given */
    if (!args || args == "") {
        topic = "index";
    } else {
        topic = downcase(trim(args));
    }
    
    /* Build path to help file */
    help_path = HELP_DIR + topic;
    
    /* Read the help file */
    help_text = read_file(help_path);
    
    /* If no text file, check if it's a command */
    if (!help_text || help_text == "") {
        cmd_d = get_object(CMD_D);
        if (cmd_d) {
            cmd_path = cmd_d.get_command(player, topic);
            if (cmd_path) {
                cmd_obj = get_object(cmd_path);
                if (cmd_obj) {
                    help_text = cmd_obj.query_help();
                }
            }
        }
        
        /* Still no help found */
        if (!help_text || help_text == "") {
            write("No help available for topic: " + topic + "\n");
            write("Type 'help' to see the help index.\n");
            return 1;
        }
    }
    
    /* Build output with nice header */
    output = "\n";
    output = output + "===============================================\n";
    output = output + "  Help: " + topic + "\n";
    output = output + "===============================================\n\n";
    output = output + help_text;
    
    /* Ensure trailing newline */
    if (rightstr(output, 1) != "\n") {
        output = output + "\n";
    }
    
    write(output);
    return 1;
}

/* Help text */
query_help() {
    return "Display help topics\n" +
           "Category: information\n\n" +
           "Usage:\n" +
           "  help [topic]\n\n" +
           "Display help information for various topics.\n" +
           "Type 'help' alone to see the help index.\n\n" +
           "Examples:\n" +
           "  help          - Show help index\n" +
           "  help commands - Show available commands\n" +
           "  help say      - Show help for the say command\n";
}
