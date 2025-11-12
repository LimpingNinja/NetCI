/* test_sprintf.c - Test suite for sprintf() Phase 1 MVP
 *
 * Tests C-style formatting: %s %d %i %c %o %x %X %%
 * Plus width, precision, alignment, zero-padding, and signs
 */

#include <std.h>

/* Test counters */
int tests_run;
int tests_passed;
int tests_failed;

/* Test helper functions */
test_start(string name) {
    syswrite("=== Testing: " + name + " ===");
}

test_assert(string actual, string expected, string description) {
    tests_run++;
    if (actual == expected) {
        syswrite("  [PASS] " + description);
        tests_passed++;
    } else {
        syswrite("  [FAIL] " + description);
        syswrite("         Expected: '" + expected + "'");
        syswrite("         Got:      '" + actual + "'");
        tests_failed++;
    }
}

/* ========================================================================
 * BASIC STRING FORMATTING (%s)
 * ======================================================================== */

test_basic_strings() {
    string result;
    
    test_start("Basic string interpolation with %s");
    
    /* Test: Single string substitution */
    result = sprintf("Hello %s", "world");
    test_assert(result, "Hello world", 
        "Single %s should substitute 'world'");
    
    /* Test: Multiple string substitutions */
    result = sprintf("%s + %s = %s", "foo", "bar", "foobar");
    test_assert(result, "foo + bar = foobar", 
        "Multiple %s should substitute all strings in order");
}

/* ========================================================================
 * INTEGER FORMATTING (%d, %i)
 * ======================================================================== */

test_integers() {
    string result;
    
    test_start("Integer formatting with %d and %i");
    
    /* Test: Basic decimal integer */
    result = sprintf("%d", 42);
    test_assert(result, "42", 
        "%d should format positive integer");
    
    /* Test: Multiple integers */
    result = sprintf("%d + %d = %d", 2, 3, 5);
    test_assert(result, "2 + 3 = 5", 
        "Multiple %d should format multiple integers");
    
    /* Test: Negative integer with %i */
    result = sprintf("%i", -123);
    test_assert(result, "-123", 
        "%i should format negative integer with sign");
}

/* ========================================================================
 * WIDTH AND ALIGNMENT
 * ======================================================================== */

test_width_and_alignment() {
    string result;
    
    test_start("Field width and alignment modifiers");
    
    /* Test: Right-aligned (default) with width */
    result = sprintf("%10s", "right");
    test_assert(result, "     right", 
        "%10s should right-align string in 10-char field");
    
    /* Test: Left-aligned with - flag */
    result = sprintf("%-10s", "left");
    test_assert(result, "left      ", 
        "%-10s should left-align string in 10-char field");
    
    /* Test: Zero-padded integers */
    result = sprintf("%05d", 42);
    test_assert(result, "00042", 
        "%05d should zero-pad integer to 5 digits");
    
    /* Test: Right-aligned integer with width */
    result = sprintf("%5d", 123);
    test_assert(result, "  123", 
        "%5d should right-align integer in 5-char field");
}

/* ========================================================================
 * PRECISION (.n modifier)
 * ======================================================================== */

test_precision() {
    string result;
    
    test_start("Precision modifier for string truncation");
    
    /* Test: String truncation with precision */
    result = sprintf("%.3s", "truncate");
    test_assert(result, "tru", 
        "%.3s should truncate string to 3 characters");
    
    /* Test: Width and precision combined */
    result = sprintf("%10.3s", "truncate");
    test_assert(result, "       tru", 
        "%10.3s should truncate to 3 chars then right-align in 10-char field");
    
    /* Test: Precision larger than string */
    result = sprintf("%.5s", "short");
    test_assert(result, "short", 
        "%.5s with string shorter than precision should not pad");
}

/* ========================================================================
 * NUMBER BASES (%o, %x, %X)
 * ======================================================================== */

test_number_bases() {
    string result;
    
    test_start("Octal and hexadecimal formatting");
    
    /* Test: All bases for same number */
    result = sprintf("%d %o %x %X", 255, 255, 255, 255);
    test_assert(result, "255 377 ff FF", 
        "Same number in decimal(255), octal(377), hex lowercase(ff), hex uppercase(FF)");
    
    /* Test: Octal conversion */
    result = sprintf("%o", 8);
    test_assert(result, "10", 
        "%o should convert 8 to octal '10'");
    
    /* Test: Hex conversion */
    result = sprintf("%x", 16);
    test_assert(result, "10", 
        "%x should convert 16 to hex '10'");
}

/* ========================================================================
 * SIGN MODIFIERS (+, space)
 * ======================================================================== */

test_signs() {
    string result;
    
    test_start("Sign modifiers for integers");
    
    /* Test: Force sign with + flag */
    result = sprintf("%+d", 42);
    test_assert(result, "+42", 
        "%+d should prefix positive numbers with '+'");
    
    /* Test: + flag with negative number */
    result = sprintf("%+d", -42);
    test_assert(result, "-42", 
        "%+d with negative number shows '-' sign");
    
    /* Test: Space flag for positive numbers */
    result = sprintf("% d", 42);
    test_assert(result, " 42", 
        "% d should prefix positive numbers with space");
    
    /* Test: Space flag with negative number */
    result = sprintf("% d", -42);
    test_assert(result, "-42", 
        "% d with negative number shows '-' sign (not space)");
}

/* ========================================================================
 * CHARACTER FORMATTING (%c)
 * ======================================================================== */

test_characters() {
    string result;
    
    test_start("Character formatting from ASCII codes");
    
    /* Test: Single character from ASCII */
    result = sprintf("%c", 65);
    test_assert(result, "A", 
        "%c should convert ASCII 65 to 'A'");
    
    /* Test: Multiple characters */
    result = sprintf("%c%c%c", 72, 105, 33);
    test_assert(result, "Hi!", 
        "Multiple %c should convert ASCII codes 72,105,33 to 'Hi!'");
}

/* ========================================================================
 * PERCENT SIGN ESCAPING (%%)
 * ======================================================================== */

test_percent_escaping() {
    string result;
    
    test_start("Literal percent sign with %%");
    
    /* Test: Single percent */
    result = sprintf("%%");
    test_assert(result, "%", 
        "%% should produce single literal '%'");
    
    /* Test: Percent in context */
    result = sprintf("100%% complete");
    test_assert(result, "100% complete", 
        "%% in string should produce literal '%' character");
}

/* ========================================================================
 * REAL-WORLD EXAMPLES
 * ======================================================================== */

test_real_world_examples() {
    string result;
    
    test_start("Real-world MUD formatting examples");
    
    /* Test: Item listing format */
    result = sprintf("%-20s %5d gold", "Short Sword", 150);
    test_assert(result, "Short Sword            150 gold", 
        "Item listing: left-aligned name (20 chars), right-aligned price (5 chars)");
    
    /* Test: Stats display format */
    result = sprintf("HP: %3d/%3d  MP: %3d/%3d", 45, 100, 30, 75);
    test_assert(result, "HP:  45/100  MP:  30/ 75", 
        "Stats display: current/max HP and MP with right-aligned 3-digit numbers");
}

/* ========================================================================
 * EDGE CASES
 * ======================================================================== */

test_edge_cases() {
    string result;
    
    test_start("Edge cases and error handling");
    
    /* Test: Empty format string */
    result = sprintf("");
    test_assert(result, "", 
        "Empty format string should return empty string");
    
    /* Test: No format specifiers */
    result = sprintf("no format specs");
    test_assert(result, "no format specs", 
        "String with no format specifiers should pass through unchanged");
    
    /* Test: Not enough arguments */
    result = sprintf("%s %d", "only one arg");
    test_assert(result, "only one arg <?>", 
        "Missing argument should be replaced with placeholder '<>'");
}

/* ========================================================================
 * MAIN TEST RUNNER
 * ======================================================================== */

init() {
    tests_run = 0;
    tests_passed = 0;
    tests_failed = 0;
    
    syswrite("");
    syswrite("===============================================");
    syswrite("sprintf() Phase 1 MVP Test Suite");
    syswrite("===============================================");
    syswrite("");
    
    test_basic_strings();
    test_integers();
    test_width_and_alignment();
    test_precision();
    test_number_bases();
    test_signs();
    test_characters();
    test_percent_escaping();
    test_real_world_examples();
    test_edge_cases();
    
    syswrite("");
    syswrite("===============================================");
    syswrite("TEST SUITE COMPLETE");
    syswrite("===============================================");
    syswrite("Tests Run:    " + itoa(tests_run));
    syswrite("Tests Passed: " + itoa(tests_passed));
    syswrite("Tests Failed: " + itoa(tests_failed));
    
    if (tests_failed == 0) {
        syswrite("");
        syswrite("*** ALL TESTS PASSED ***");
    } else {
        syswrite("");
        syswrite("*** SOME TESTS FAILED ***");
    }
    syswrite("===============================================");
    syswrite("");
    
    destruct(this_object());
}
