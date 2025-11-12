/* Parent 1 for conflict test - defines 'shared_var' */

int shared_var;
int parent1_var;

init() {
    shared_var = 100;
    parent1_var = 111;
}

get_shared() {
    return shared_var;
}

get_type() {
    return "parent1";
}
