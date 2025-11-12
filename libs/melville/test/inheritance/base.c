/* Base class for inheritance testing
 * Defines a single variable and methods to manipulate it
 */

/* Global variable owned by base */
int base_value;

init() {
    base_value = 100;
}

/* Getter */
get_base_value() {
    return base_value;
}

/* Setter */
set_base_value(int val) {
    base_value = val;
}

/* Increment */
increment_base() {
    base_value = base_value + 1;
}

/* Test function that can be overridden */
get_type() {
    return "base";
}
