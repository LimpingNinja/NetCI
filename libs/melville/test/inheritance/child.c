/* Child class that inherits from base
 * Tests that inherited variables and functions work correctly
 */

inherit "/test/inheritance/base";

/* Child's own variable */
int child_value;

init() {
    /* Call parent init */
    ::init();
    
    /* Initialize own variable */
    child_value = 200;
}

/* Getter for child's variable */
get_child_value() {
    return child_value;
}

/* Setter for child's variable */
set_child_value(int val) {
    child_value = val;
}

/* Test that we can call parent's methods */
test_parent_methods() {
    int result;
    
    syswrite("    [test_parent_methods] Starting test...");
    
    /* Get initial value from parent */
    syswrite("    [test_parent_methods] Calling get_base_value()...");
    result = get_base_value();
    if (!result) {
        syswrite("    [test_parent_methods] ERROR: get_base_value() returned 0 or undefined!");
    } else {
        syswrite("    [test_parent_methods] get_base_value() returned: " + itoa(result));
    }
    if (result != 100) {
        syswrite("    [test_parent_methods] FAIL: Expected 100, got " + itoa(result));
        return 0;  /* FAIL */
    }
    syswrite("    [test_parent_methods] PASS: Initial value is 100");
    
    /* Modify parent's value */
    syswrite("    [test_parent_methods] Calling set_base_value(150)...");
    set_base_value(150);
    result = get_base_value();
    syswrite("    [test_parent_methods] After set, get_base_value() returned: " + itoa(result));
    if (result != 150) {
        syswrite("    [test_parent_methods] FAIL: Expected 150, got " + itoa(result));
        return 0;  /* FAIL */
    }
    syswrite("    [test_parent_methods] PASS: Value changed to 150");
    
    /* Increment parent's value */
    syswrite("    [test_parent_methods] Calling increment_base()...");
    increment_base();
    result = get_base_value();
    syswrite("    [test_parent_methods] After increment, get_base_value() returned: " + itoa(result));
    if (result != 151) {
        syswrite("    [test_parent_methods] FAIL: Expected 151, got " + itoa(result));
        return 0;  /* FAIL */
    }
    syswrite("    [test_parent_methods] PASS: Value incremented to 151");
    
    syswrite("    [test_parent_methods] All checks passed!");
    return 1;  /* SUCCESS */
}

/* Override parent's method */
get_type() {
    return "child";
}

/* Test that we can access parent's version with :: */
test_parent_call() {
    string parent_type;
    string my_type;
    
    syswrite("    [test_parent_call] Starting test...");
    
    syswrite("    [test_parent_call] Calling ::get_type() (parent version)...");
    parent_type = ::get_type();
    syswrite("    [test_parent_call] ::get_type() returned: " + parent_type);
    
    syswrite("    [test_parent_call] Calling get_type() (my version)...");
    my_type = get_type();
    syswrite("    [test_parent_call] get_type() returned: " + my_type);
    
    if (parent_type == "base" && my_type == "child") {
        syswrite("    [test_parent_call] PASS: parent='base', child='child'");
        return "SUCCESS";
    }
    syswrite("    [test_parent_call] FAIL: parent='" + parent_type + "', child='" + my_type + "'");
    return "FAIL";
}
