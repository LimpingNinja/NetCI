/* rehash.c - Rehash command directories to discover new commands */

init() {
    /* Grant privilege to this admin command */
    set_priv(this_object(), 1);
}

query_help() {
    return "Rehash command directories\n" +
           "Category: admin\n\n" +
           "Usage:\n" +
           "  rehash\n\n" +
           "Scans command directories and rebuilds the command table.\n" +
           "Use this after adding new commands to make them available.\n\n" +
           "This is an admin-only command.\n";
}

execute(string str) {
    object cmd_d;
    
    /* Get the command daemon */
    cmd_d = find_object("/sys/daemons/cmd_d");
    
    if (!cmd_d) {
        write("Error: Command daemon not found.\n");
        return 0;
    }
    
    /* Rehash all command directories */
    write("Rehashing command directories...\n");
    cmd_d.rehash("/cmds/player");
    cmd_d.rehash("/cmds/builder");
    cmd_d.rehash("/cmds/wizard");
    cmd_d.rehash("/cmds/admin");
    write("Rehash complete!\n");
    
    return 1;
}
