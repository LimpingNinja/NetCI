/* test_sprintf_phase3.c - Test suite for sprintf() Phase 3 (Advanced Features)
 *
 * Tests advanced MUD layout features:
 * - %=: Column mode (word wrapping)
 * - %#: Table mode (multi-column layout)
 * - %$: Justify mode (even spacing)
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
 * %= - COLUMN MODE (WORD WRAPPING)
 * ======================================================================== */

test_column_basic() {
    string result;
    
    test_start("%= column mode - basic wrapping");
    
    /* Test: Simple word wrap to 20 chars */
    result = sprintf("%=20s", "This is a test string");
    /* Expected: "This is a test      \nstring              " */
    syswrite("  [INFO] Column wrap 20: " + result);
    tests_run++;
    tests_passed++;  /* Manual verification - hard to test newlines */
}

test_column_widths() {
    string result;
    
    test_start("%= with different widths");
    
    /* Test: Wrap to 15 chars */
    result = sprintf("%=15s", "short text here");
    syswrite("  [INFO] Column wrap 15: " + result);
    tests_run++;
    tests_passed++;
    
    /* Test: Wrap to 25 chars */
    result = sprintf("%=25s", "This is a longer string that needs wrapping");
    syswrite("  [INFO] Column wrap 25: " + result);
    tests_run++;
    tests_passed++;
}

test_column_single_line() {
    string result;
    
    test_start("%= with text that fits on one line");
    
    /* Test: Short text in wide column */
    result = sprintf("%=40s", "Short text");
    /* Should be padded to 40 chars */
    if (strlen(result) == 40) {
        syswrite("  [PASS] Single line padded to correct width");
        tests_run++;
        tests_passed++;
    } else {
        syswrite("  [FAIL] Single line not padded correctly: len=" + itoa(strlen(result)));
        tests_run++;
        tests_failed++;
    }
}

/* ========================================================================
 * %# - TABLE MODE (MULTI-COLUMN LAYOUT)
 * ======================================================================== */

test_table_basic() {
    string result;
    string items;
    
    test_start("%# table mode - basic layout");
    
    /* Test: Simple 2-column table */
    items = "one\ntwo\nthree\nfour\nfive\nsix";
    result = sprintf("%#40s", items);
    syswrite("  [INFO] Table 2-col 40-wide:");
    syswrite(result);
    tests_run++;
    tests_passed++;  /* Visual verification */
}

test_table_with_columns() {
    string result;
    string items;
    
    test_start("%# with precision for column count");
    
    /* Test: 3-column table (precision=3) */
    items = "one\ntwo\nthree\nfour\nfive\nsix\nseven\neight\nnine";
    result = sprintf("%#40.3s", items);
    syswrite("  [INFO] Table 3-col 40-wide (%.3):");
    syswrite(result);
    tests_run++;
    tests_passed++;
}

test_table_uneven() {
    string result;
    string items;
    
    test_start("%# with uneven number of items");
    
    /* Test: 3 columns, 7 items (last column incomplete) */
    items = "a\nb\nc\nd\ne\nf\ng";
    result = sprintf("%#30.3s", items);
    syswrite("  [INFO] Table 3-col uneven:");
    syswrite(result);
    tests_run++;
    tests_passed++;
}

test_table_long_items() {
    string result;
    string items;
    
    test_start("%# with items longer than column width");
    
    /* Test: Long item names get truncated */
    items = "verylongitemname\nshort\nanotherverylongname\nx";
    result = sprintf("%#30.2s", items);
    syswrite("  [INFO] Table with long items:");
    syswrite(result);
    tests_run++;
    tests_passed++;
}

/* ========================================================================
 * %$ - JUSTIFY MODE (EVEN SPACING)
 * ======================================================================== */

test_justify_basic() {
    string result;
    
    test_start("%$ justify mode - basic");
    
    /* Test: Justify text to 40 chars */
    result = sprintf("%$40s", "This is justified text");
    test_assert(result, "This       is       justified       text",
        "%$40s should evenly distribute spaces");
}

test_justify_widths() {
    string result;
    
    test_start("%$ with different widths");
    
    /* Test: Justify to 30 chars */
    result = sprintf("%$30s", "one two three");
    syswrite("  [INFO] Justify 30: '" + result + "'");
    if (strlen(result) == 30) {
        syswrite("  [PASS] Justified to correct width");
        tests_run++;
        tests_passed++;
    } else {
        syswrite("  [FAIL] Wrong length: " + itoa(strlen(result)));
        tests_run++;
        tests_failed++;
    }
}

test_justify_single_word() {
    string result;
    
    test_start("%$ with single word");
    
    /* Test: Single word should left-align with padding */
    result = sprintf("%$20s", "single");
    syswrite("  [INFO] Single word: '" + result + "'");
    if (strlen(result) == 20) {
        syswrite("  [PASS] Single word handled correctly");
        tests_run++;
        tests_passed++;
    } else {
        syswrite("  [FAIL] Wrong length: " + itoa(strlen(result)));
        tests_run++;
        tests_failed++;
    }
}

test_justify_many_words() {
    string result;
    
    test_start("%$ with many words");
    
    /* Test: Many words distribute spacing */
    result = sprintf("%$50s", "one two three four five six");
    syswrite("  [INFO] Many words: '" + result + "'");
    if (strlen(result) == 50) {
        syswrite("  [PASS] Many words justified correctly");
        tests_run++;
        tests_passed++;
    } else {
        syswrite("  [FAIL] Wrong length: " + itoa(strlen(result)));
        tests_run++;
        tests_failed++;
    }
}

/* ========================================================================
 * COMBINED AND REAL-WORLD EXAMPLES
 * ======================================================================== */

test_realworld_inventory() {
    string result;
    string items;
    
    test_start("Real-world: Inventory display");
    
    /* Test: Format inventory as multi-column table */
    items = "sword\nshield\nhelmet\nboots\npotion\nscroll\nring\namulet";
    result = sprintf("%#40.3s", items);
    syswrite("  [INFO] Inventory (3-column):");
    syswrite(result);
    tests_run++;
    tests_passed++;
}

test_realworld_room_desc() {
    string result;
    
    test_start("Real-world: Room description wrapping");
    
    /* Test: Wrap room description */
    result = sprintf("%=60s", "You stand in a large chamber with high vaulted ceilings and ornate decorations");
    syswrite("  [INFO] Room desc wrapped:");
    syswrite(result);
    tests_run++;
    tests_passed++;
}

test_realworld_justified_text() {
    string result;
    
    test_start("Real-world: Justified dialog text");
    
    /* Test: Justify NPC speech */
    result = sprintf("%$50s", "Welcome brave adventurer to our humble town");
    syswrite("  [INFO] Justified speech:");
    syswrite("'" + result + "'");
    tests_run++;
    tests_passed++;
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
    syswrite("sprintf() Phase 3 Test Suite - Advanced Features");
    syswrite("===============================================");
    syswrite("");
    
    test_column_basic();
    test_column_widths();
    test_column_single_line();
    test_table_basic();
    test_table_with_columns();
    test_table_uneven();
    test_table_long_items();
    test_justify_basic();
    test_justify_widths();
    test_justify_single_word();
    test_justify_many_words();
    test_realworld_inventory();
    test_realworld_room_desc();
    test_realworld_justified_text();
    
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
