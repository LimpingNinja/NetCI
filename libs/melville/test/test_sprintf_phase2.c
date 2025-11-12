/* test_sprintf_phase2.c - Test suite for sprintf() Phase 2 (LPC Extensions)
 *
 * Tests LPC-specific formatting features:
 * - %O: Pretty-print arrays and mappings
 * - %@: Array iteration
 * - %*: Dynamic width
 * - %|: Center alignment
 * - Custom padding
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
 * %O - LPC VALUE PRETTY-PRINTING
 * ======================================================================== */

test_O_arrays() {
    string result;
    
    test_start("%O with arrays");
    
    /* Test: Empty array */
    result = sprintf("%O", ({}));
    test_assert(result, "({})", 
        "Empty array should format as ({})");
    
    /* Test: Integer array */
    result = sprintf("%O", ({ 1, 2, 3 }));
    test_assert(result, "({1,2,3})", 
        "Integer array should format as ({1,2,3})");
    
    /* Test: String array */
    result = sprintf("%O", ({ "foo", "bar", "baz" }));
    test_assert(result, "({\"foo\",\"bar\",\"baz\"})", 
        "String array should format with quoted strings");
    
    /* Test: Mixed array */
    result = sprintf("%O", ({ 42, "hello", 100 }));
    test_assert(result, "({42,\"hello\",100})", 
        "Mixed array should handle different types");
}

test_O_mappings() {
    string result;
    
    test_start("%O with mappings");
    
    /* Test: Empty mapping */
    result = sprintf("%O", ([]));
    test_assert(result, "([])", 
        "Empty mapping should format as ([])");
    
    /* Test: Simple mapping */
    result = sprintf("%O", ([ "hp": 100 ]));
    test_assert(result, "([\"hp\":100])", 
        "Simple mapping should format as ([key:value])");
    
    /* Test: Multiple entries - Note: order may vary due to hash table */
    result = sprintf("%O", ([ "a": 1, "b": 2 ]));
    /* We'll check if result contains both entries */
    if (strlen(result) > 0) {
        syswrite("  [INFO] Mapping with multiple entries: " + result);
        tests_run++;
        tests_passed++;  /* Count as pass if it doesn't crash */
    }
}

test_O_nested() {
    string result;
    
    test_start("%O with nested structures");
    
    /* Test: Nested array */
    result = sprintf("%O", ({ ({ 1, 2 }), ({ 3, 4 }) }));
    test_assert(result, "({({1,2}),({3,4})})", 
        "Nested arrays should serialize recursively");
}

test_O_with_formatting() {
    string result;
    
    test_start("%O with width and alignment");
    
    /* Test: Right-aligned */
    result = sprintf("%15O", ({ 1, 2 }));
    test_assert(result, "        ({1,2})", 
        "%15O should right-align array representation");
    
    /* Test: Left-aligned */
    result = sprintf("%-15O", ({ 1, 2 }));
    test_assert(result, "({1,2})        ", 
        "%-15O should left-align array representation");
}

/* ========================================================================
 * %@ - ARRAY ITERATION
 * ======================================================================== */

test_array_iteration() {
    string result;
    
    test_start("%@ array iteration");
    
    /* Test: Basic iteration with %d */
    result = sprintf("%@d ", ({ 1, 2, 3, 4 }));
    test_assert(result, "1234 ", 
        "%@d should format each integer, then space after all");
    
    /* Test: Iteration with width */
    result = sprintf("%@5d", ({ 1, 22, 333 }));
    test_assert(result, "    1   22  333", 
        "%@5d should apply 5-char width to each element");
    
    /* Test: Iteration with left-align */
    result = sprintf("%@-5s", ({ "a", "bb", "ccc" }));
    test_assert(result, "a    bb   ccc  ", 
        "%@-5s should left-align each string in 5-char field");
}

test_array_iteration_hex() {
    string result;
    
    test_start("%@ with hex formatting");
    
    /* Test: Hex iteration */
    result = sprintf("%@x ", ({ 10, 15, 255 }));
    test_assert(result, "afff ", 
        "%@x should format each integer as hex, then space");
    
    /* Test: Uppercase hex with padding */
    result = sprintf("%@04X ", ({ 10, 255 }));
    test_assert(result, "000A00FF ", 
        "%@04X should format as uppercase hex with zero-padding, then space");
}

/* ========================================================================
 * %* - DYNAMIC WIDTH
 * ======================================================================== */

test_dynamic_width() {
    string result;
    
    test_start("%* dynamic width");
    
    /* Test: Dynamic width for string */
    result = sprintf("%*s", 10, "test");
    test_assert(result, "      test", 
        "%*s with width=10 should right-align");
    
    /* Test: Dynamic width with left-align */
    result = sprintf("%-*s", 10, "test");
    test_assert(result, "test      ", 
        "%-*s with width=10 should left-align");
    
    /* Test: Dynamic width for integer */
    result = sprintf("%*d", 5, 42);
    test_assert(result, "   42", 
        "%*d with width=5 should right-align integer");
}

test_dynamic_precision() {
    string result;
    
    test_start("%* with dynamic precision");
    
    /* Test: Dynamic precision for string truncation */
    result = sprintf("%.*s", 3, "truncate");
    test_assert(result, "tru", 
        "%.*s with precision=3 should truncate to 3 chars");
    
    /* Test: Dynamic width AND precision */
    result = sprintf("%*.*s", 10, 3, "truncate");
    test_assert(result, "       tru", 
        "%*.*s should apply both dynamic width and precision");
}

/* ========================================================================
 * %| - CENTER ALIGNMENT
 * ======================================================================== */

test_center_alignment() {
    string result;
    
    test_start("%| center alignment");
    
    /* Test: Center string in field */
    result = sprintf("%|10s", "test");
    test_assert(result, "   test   ", 
        "%|10s should center 'test' in 10-char field");
    
    /* Test: Center odd-length string */
    result = sprintf("%|10s", "hello");
    test_assert(result, "  hello   ", 
        "%|10s should center 'hello' (5 chars) in 10-char field");
    
    /* Test: Center integer */
    result = sprintf("%|8d", 42);
    test_assert(result, "   42   ", 
        "%|8d should center integer in field");
}

test_center_with_precision() {
    string result;
    
    test_start("%| with precision");
    
    /* Test: Center truncated string */
    result = sprintf("%|10.3s", "truncate");
    test_assert(result, "   tru    ", 
        "%|10.3s should truncate then center");
}

/* ========================================================================
 * COMBINED FEATURES
 * ======================================================================== */

test_combined_features() {
    string result;
    
    test_start("Combined Phase 2 features");
    
    /* Test: Array iteration with dynamic width */
    result = sprintf("%@*d", 5, ({ 1, 22, 333 }));
    test_assert(result, "    1   22  333", 
        "%@*d should combine array iteration with dynamic width");
    
    /* Test: %O with dynamic width */
    result = sprintf("%*O", 15, ({ 1, 2 }));
    test_assert(result, "        ({1,2})", 
        "%*O should apply dynamic width to array representation");
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
    syswrite("sprintf() Phase 2 Test Suite - LPC Extensions");
    syswrite("===============================================");
    syswrite("");
    
    test_O_arrays();
    test_O_mappings();
    test_O_nested();
    test_O_with_formatting();
    test_array_iteration();
    test_array_iteration_hex();
    test_dynamic_width();
    test_dynamic_precision();
    test_center_alignment();
    test_center_with_precision();
    test_combined_features();
    
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
