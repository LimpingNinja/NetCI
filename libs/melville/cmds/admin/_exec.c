/* exec.c - Admin command to execute LPC code statements */

init() {
    /* Grant privilege to this admin command */
    set_priv(this_object(), 1);
}

query_help() {
    return "Execute LPC code statements\n" +
           "Category: admin\n\n" +
           "Usage:\n" +
           "  exec <code>\n\n" +
           "Executes LPC code statements dynamically.\n" +
           "Unlike 'eval', this command supports full statements.\n\n" +
           "Examples:\n" +
           "  exec syslog(\"Hello from exec!\");\n" +
           "  exec for (int i = 0; i < 5; i++) write(\"Count: \" + itoa(i));\n\n" +
           "Note: This is an admin-only command. Use with extreme caution!\n";
}

do_command(player, str) {
    object eval_obj;
    string func_code;
    int result;
    
    if (!player) return 0;
    
    if (!str || str == "") {
        player.listen("Usage: exec <code>\n");
        player.listen("Type 'help exec' for more information.\n");
        return 1;
    }
    
    /* Wrap the code in an exec() function */
    func_code = "exec_stmt() { " + str + " }";
    
    /* Compile the code */
    eval_obj = compile_string(func_code);
    
    if (!eval_obj) {
        player.listen("Execution failed: Syntax error in code.\n");
        return 1;
    }
    
    /* Inherit privilege from this command object */
    set_priv(eval_obj, priv(this_object()));
    
    /* Execute the function and capture return value */
    result = eval_obj.exec_stmt();
    
    /* Display result if one was returned */
    if (result) {
        player.listen(sprintf("Result: %O\n", result));
    } else {
        player.listen("Execution complete.\n");
    }
    
    /* Clean up */
    destruct(eval_obj);
    
    return 1;
}

/* Legacy interface for backward compatibility */
execute(str) {
    return do_command(this_player(), str);
}
