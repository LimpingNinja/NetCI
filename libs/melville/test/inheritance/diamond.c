/* Diamond inheritance test
 * Inherits from both left and right, which both inherit from base
 * Tests that base_value is shared (single storage) across both paths
 */

inherit "/test/inheritance/left";
inherit "/test/inheritance/right";

/* Diamond's own variable */
int diamond_value;

init() {
    /* Call both parent inits */
    left::init();
    right::init();
    
    diamond_value = 500;
}

get_diamond_value() {
    return diamond_value;
}

/* Test that base_value is shared across both inheritance paths */
test_shared_storage() {
    int val1, val2;
    
    /* Set via left's inherited method */
    set_base_value(999);
    
    /* Read via right's inherited method - should be same value! */
    val1 = get_base_value();
    
    /* Also test direct access */
    val2 = base_value;
    
    if (val1 == 999 && val2 == 999) {
        return 1;  /* SUCCESS - single shared storage */
    }
    
    return 0;  /* FAIL - separate storage (wrong!) */
}

/* Test that all variables are accessible */
test_all_variables() {
    /* Should have access to:
     * - base_value (from base, via both paths)
     * - left_value (from left)
     * - right_value (from right)
     * - diamond_value (own)
     */
    
    base_value = 111;
    left_value = 222;
    right_value = 333;
    diamond_value = 444;
    syswrite("Values are:");
    syswrite("base_value: " + itoa(base_value));
    syswrite("left_value: " + itoa(left_value));
    syswrite("right_value: " + itoa(right_value));
    syswrite("diamond_value: " + itoa(diamond_value));
    
    if (base_value == 111 && 
        left_value == 222 && 
        right_value == 333 && 
        diamond_value == 444) {
        return 1;  /* SUCCESS */
    }
    
    return 0;  /* FAIL */
}

get_type() {
    return "diamond";
}
