/* Parent 2 for conflict test - ALSO defines 'shared_var'
 * This creates a conflict when both are inherited
 */

int shared_var;  /* CONFLICT! Same name as parent1 */
int parent2_var;

init() {
    shared_var = 200;
    parent2_var = 222;
}

get_shared() {
    return shared_var;
}

get_type() {
    return "parent2";
}
