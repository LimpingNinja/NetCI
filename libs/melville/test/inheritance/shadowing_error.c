/* TEST: This should FAIL to compile
 * Error: child tries to redeclare a variable that parent already has
 * 
 * Expected error: "variable 'base_value' already defined in ancestor '/test/inheritance/base'"
 */

inherit "/test/inheritance/base";

/* Try to redeclare parent's variable - should ERROR */
int base_value;

int my_var;

init() {
    my_var = 42;
}
