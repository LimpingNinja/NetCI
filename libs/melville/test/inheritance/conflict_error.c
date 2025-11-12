/* TEST: This should FAIL to compile
 * Error: variable 'shared_var' defined in both parents
 * 
 * Expected error: "variable 'shared_var' defined in both '/test/inheritance/conflict_parent1' and '/test/inheritance/conflict_parent2'"
 */

inherit "/test/inheritance/conflict_parent1";
inherit "/test/inheritance/conflict_parent2";

int my_var;

init() {
    my_var = 42;
}
