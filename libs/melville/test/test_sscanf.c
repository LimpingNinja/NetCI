/* test_sscanf.c - Test suite for sscanf() implementation
 *
 * Run with: @call /test_sscanf test_all
 */

#include <std.h>

/* Test counter */
int tests_run;
int tests_passed;
int tests_failed;

/* Test helper functions */
test_start(string name) {
    write("\n=== Testing: " + name + " ===\n");
}

test_assert(int condition, string message) {
    tests_run++;
    if (condition) {
        write("  [PASS] " + message + "\n");
        tests_passed++;
    } else {
        write("  [FAIL] " + message + "\n");
        tests_failed++;
    }
}

test_assert_false(int condition, string message) {
    tests_run++;
    if (!condition) {
        write("  [PASS] " + message + "\n");
        tests_passed++;
    } else {
        write("  [FAIL] " + message + "\n");
        tests_failed++;
    }
}

test_assert_empty_string(string str, string message) {
    tests_run++;
    if (!str || str == "") {
        write("  [PASS] " + message + "\n");
        tests_passed++;
    } else {
        write("  [FAIL] " + message + " (got: '" + str + "')\n");
        tests_failed++;
    }
}

test_assert_zero(int val, string message) {
    tests_run++;
    if (val == 0) {
        write("  [PASS] " + message + "\n");
        tests_passed++;
    } else {
        write("  [FAIL] " + message + " (got: " + itoa(val) + ")\n");
        tests_failed++;
    }
}

/* ========================================================================
 * STRING TESTS (%s)
 * ======================================================================== */

test_string_basic() {
    string what, who;
    int result;
    
    test_start("Basic string matching");
    
    result = sscanf("give sword to bob", "give %s to %s", what, who);
    test_assert(result == 2, "Should match 2 strings");
    test_assert(what == "sword", "First string should be 'sword', got: " + what);
    test_assert(who == "bob", "Second string should be 'bob', got: " + who);
}

test_string_partial() {
    string what, who;
    int result;
    
    test_start("Partial string match");
    
    result = sscanf("give sword", "give %s to %s", what, who);
    test_assert(result == 1, "Should match 1 string");
    test_assert(what == "sword", "First string should be 'sword', got: " + what);
}

test_string_empty() {
    string a, b;
    int result;
    
    test_start("Empty string match");
    
    result = sscanf("  ", "%s %s", a, b);
    test_assert(result == 2, "Should match 2 empty strings");
    test_assert_empty_string(a, "First string should be empty");
    test_assert_empty_string(b, "Second string should be empty");
}

test_string_to_end() {
    string cmd, args;
    int result;
    
    test_start("String matching to end");
    
    result = sscanf("say hello world", "%s %s", cmd, args);
    test_assert(result == 2, "Should match 2 strings");
    test_assert(cmd == "say", "Command should be 'say', got: " + cmd);
    test_assert(args == "hello world", "Args should be 'hello world', got: " + args);
}

/* ========================================================================
 * INTEGER TESTS (%d)
 * ======================================================================== */

test_integer_basic() {
    int level;
    int result;
    
    test_start("Basic integer matching");
    
    result = sscanf("level 42", "level %d", level);
    test_assert(result == 1, "Should match 1 integer");
    test_assert(level == 42, "Level should be 42, got: " + itoa(level));
}

test_integer_negative() {
    int temp;
    int result;
    
    test_start("Negative integer matching");
    
    result = sscanf("temperature -15", "temperature %d", temp);
    test_assert(result == 1, "Should match 1 integer");
    test_assert(temp == -15, "Temperature should be -15, got: " + itoa(temp));
}

test_integer_multiple() {
    int x, y, z;
    int result;
    
    test_start("Multiple integer matching");
    
    result = sscanf("coords 10 20 30", "coords %d %d %d", x, y, z);
    test_assert(result == 3, "Should match 3 integers");
    test_assert(x == 10, "X should be 10, got: " + itoa(x));
    test_assert(y == 20, "Y should be 20, got: " + itoa(y));
    test_assert(z == 30, "Z should be 30, got: " + itoa(z));
}

/* ========================================================================
 * HEX TESTS (%x)
 * ======================================================================== */

test_hex_basic() {
    int color;
    int result;
    
    test_start("Basic hex matching");
    
    result = sscanf("color FF00AA", "color %x", color);
    test_assert(result == 1, "Should match 1 hex value");
    test_assert(color == 16711850, "Color should be 16711850, got: " + itoa(color));
}

test_hex_with_prefix() {
    int value;
    int result;
    
    test_start("Hex with 0x prefix");
    
    result = sscanf("value 0xDEADBEEF", "value %x", value);
    test_assert(result == 1, "Should match 1 hex value");
    test_assert(value == 3735928559, "Value should be 3735928559, got: " + itoa(value));
}

test_hex_lowercase() {
    int val;
    int result;
    
    test_start("Lowercase hex matching");
    
    result = sscanf("val abc123", "val %x", val);
    test_assert(result == 1, "Should match 1 hex value");
    test_assert(val == 11256099, "Value should be 11256099, got: " + itoa(val));
}

/* ========================================================================
 * SKIP TESTS (%*x)
 * ======================================================================== */

test_skip_string() {
    string x, y;
    int result;
    
    test_start("Skip string specifier");
    
    result = sscanf("a b c", "%*s %s %s", x, y);
    test_assert(result == 2, "Should match 2 strings (skipping first)");
    test_assert(x == "b", "First captured should be 'b', got: " + x);
    test_assert(y == "c", "Second captured should be 'c', got: " + y);
}

test_skip_integer() {
    int x, y;
    int result;
    
    test_start("Skip integer specifier");
    
    result = sscanf("values 10 20 30", "values %*d %d %d", x, y);
    test_assert(result == 2, "Should match 2 integers (skipping first)");
    test_assert(x == 20, "First captured should be 20, got: " + itoa(x));
    test_assert(y == 30, "Second captured should be 30, got: " + itoa(y));
}

/* ========================================================================
 * LITERAL TESTS (%%)
 * ======================================================================== */

test_literal_percent() {
    int pct;
    int result;
    
    test_start("Literal percent sign");
    
    result = sscanf("100% complete", "%d%% complete", pct);
    test_assert(result == 1, "Should match 1 integer");
    test_assert(pct == 100, "Percentage should be 100, got: " + itoa(pct));
}

/* ========================================================================
 * MIXED TESTS
 * ======================================================================== */

test_mixed_types() {
    string cmd;
    int amount;
    string item;
    int result;
    
    test_start("Mixed type matching");
    
    result = sscanf("buy 5 potions", "%s %d %s", cmd, amount, item);
    test_assert(result == 3, "Should match 3 values");
    test_assert(cmd == "buy", "Command should be 'buy', got: " + cmd);
    test_assert(amount == 5, "Amount should be 5, got: " + itoa(amount));
    test_assert(item == "potions", "Item should be 'potions', got: " + item);
}

test_hex_and_string() {
    int color;
    string name;
    int result;
    
    test_start("Hex and string matching");
    
    result = sscanf("setcolor FF0000 red", "setcolor %x %s", color, name);
    test_assert(result == 2, "Should match 2 values");
    test_assert(color == 16711680, "Color should be 16711680, got: " + itoa(color));
    test_assert(name == "red", "Name should be 'red', got: " + name);
}

/* ========================================================================
 * EDGE CASE TESTS
 * ======================================================================== */

test_no_match() {
    string x;
    int result;
    
    test_start("No match case");
    
    result = sscanf("hello", "goodbye %s", x);
    test_assert(result == 0, "Should match 0 values");
}

test_empty_input() {
    string x;
    int result;
    
    test_start("Empty input");
    
    result = sscanf("", "%s", x);
    test_assert(result == 1, "Should match 1 empty string");
    test_assert_empty_string(x, "String should be empty");
}

/* ========================================================================
 * MAIN TEST RUNNER
 * ======================================================================== */

test_all() {
    tests_run = 0;
    tests_passed = 0;
    tests_failed = 0;
    
    write("\n");
    write("╔════════════════════════════════════════════════════════════╗\n");
    write("║          NLPC sscanf() Test Suite                         ║\n");
    write("╚════════════════════════════════════════════════════════════╝\n");
    
    /* Run all tests */
    test_string_basic();
    test_string_partial();
    test_string_empty();
    test_string_to_end();
    
    test_integer_basic();
    test_integer_negative();
    test_integer_multiple();
    
    test_hex_basic();
    test_hex_with_prefix();
    test_hex_lowercase();
    
    test_skip_string();
    test_skip_integer();
    
    test_literal_percent();
    
    test_mixed_types();
    test_hex_and_string();
    
    test_no_match();
    test_empty_input();
    
    /* Print summary */
    write("\n");
    write("╔════════════════════════════════════════════════════════════╗\n");
    write("║                     Test Summary                           ║\n");
    write("╠════════════════════════════════════════════════════════════╣\n");
    write("║  Total Tests:  " + itoa(tests_run) + "\n");
    write("║  Passed:       " + itoa(tests_passed) + "\n");
    write("║  Failed:       " + itoa(tests_failed) + "\n");
    write("╚════════════════════════════════════════════════════════════╝\n");
    
    if (tests_failed == 0) {
        write("\n✓ All tests passed!\n\n");
    } else {
        write("\n✗ Some tests failed.\n\n");
    }
}
