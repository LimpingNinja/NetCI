/* Left branch of diamond inheritance */

inherit "/test/inheritance/base";

/* Left's own variable */
int left_value;

init() {
    ::init();
    left_value = 300;
}

get_left_value() {
    return left_value;
}

set_left_value(int val) {
    left_value = val;
}

get_type() {
    return "left";
}
