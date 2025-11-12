/* TEST: This should FAIL to compile
 * Error: inherit after code (late inherit)
 * 
 * Expected error: "inherit statement after variable/function declarations"
 */

/* Declare a variable first */
int my_var;

/* NOW try to inherit - this should ERROR */
inherit "/test/inheritance/base";

init() {
    my_var = 42;
}
