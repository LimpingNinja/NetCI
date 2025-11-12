/* eval.c - Admin command to evaluate LPC expressions */

init() {
    /* Grant privilege to this admin command */
    set_priv(this_object(), 1);
}

query_help() {
    return "Evaluate LPC code dynamically\n" +
           "Category: admin\n\n" +
           "Usage:\n" +
           "  eval <code>\n\n" +
           "Evaluates LPC code dynamically and displays results.\n" +
           "The code is compiled into a temporary eval object.\n\n" +
           "Examples:\n" +
           "  eval 5 + 3\n" +
           "  eval sizeof(({1,2,3,4,5}))\n" +
           "  eval users()\n" +
           "  eval new(\"/test/test_users_d\").run_tests();\n\n" +
           "Note: This is an admin-only command. Use with caution!\n";
}

do_command(player, str) {
    object eval_obj;
    string func_code;
    int result;
    
    if (!player) return 0;
    
    if (!str || str == "") {
        player.listen("Usage: eval <code>\n");
        player.listen("Type 'help eval' for more information.\n");
        return 1;
    }
    
    /* Try as statement first (supports function calls like new().run_tests()) */
    func_code = "eval_stmt() { " + str + " }";
    eval_obj = compile_string(func_code);
    
    if (eval_obj) {
        /* Inherit privilege from this command object */
        set_priv(eval_obj, priv(this_object()));
        
        /* Execute as statement */
        eval_obj.eval_stmt();
        player.listen("Statement executed.\n");
        destruct(eval_obj);
        return 1;
    }
    
    /* Fall back to expression (supports simple expressions like 5 + 3) */
    func_code = "eval_expr() { return (" + str + "); }";
    eval_obj = compile_string(func_code);
    
    if (!eval_obj) {
        player.listen("Evaluation failed: Syntax error in code.\n");
        return 1;
    }
    
    /* Inherit privilege from this command object */
    set_priv(eval_obj, priv(this_object()));
    
    /* Execute the eval function and capture result */
    result = eval_obj.eval_expr();
    
    /* Display result */
    player.listen(sprintf("Result: %O\n", result));
    
    /* Clean up */
    destruct(eval_obj);
    
    return 1;
}

/* Legacy interface for backward compatibility */
execute(str) {
    return do_command(this_player(), str);
}
