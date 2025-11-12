/* Right branch of diamond inheritance */

inherit "/test/inheritance/base";

/* Right's own variable */
int right_value;

init() {
    ::init();
    right_value = 400;
}

get_right_value() {
    return right_value;
}

set_right_value(int val) {
    right_value = val;
}

get_type() {
    return "right";
}
