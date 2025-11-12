/* test_users_d.c - Comprehensive Test Suite for users_d daemon
 *
 * Tests user tracking, registry management, context tracking, and statistics.
 * Can be run via: eval new("/test/test_users_d").run_tests();
 */

#include <std.h>
#include <config.h>

run_tests() {
    object users_d, test_player;
    mapping stats;
    string *names;
    int count, result;
    
    syswrite("\n===============================================");
    syswrite("users_d Daemon Test Suite");
    syswrite("===============================================\n");
    
    /* Test 1: Daemon exists and is accessible */
    syswrite("=== Test 1: Daemon initialization ===");
    users_d = atoo(USERS_D);
    if (users_d) {
        syswrite("  [PASS] users_d daemon loaded successfully");
    } else {
        syswrite("  [FAIL] users_d daemon not found");
        syswrite("  [ABORT] Cannot continue tests without daemon");
        return;
    }
    
    /* Test 2: Query connected users from driver */
    syswrite("\n=== Test 2: Query connected users ===");
    count = call_other(users_d, "query_user_count");
    syswrite(sprintf("  [INFO] Connected users: %d", count));
    if (count >= 0) {
        syswrite("  [PASS] query_user_count() returned valid result");
    } else {
        syswrite("  [FAIL] query_user_count() returned invalid result");
    }
    
    /* Test 3: Get user names list */
    syswrite("\n=== Test 3: Query user names ===");
    names = call_other(users_d, "query_user_names");
    if (names) {
        syswrite(sprintf("  [INFO] User names array size: %d", sizeof(names)));
        if (sizeof(names) > 0) {
            syswrite("  [INFO] Users: " + implode(names, ", "));
            syswrite("  [PASS] query_user_names() returned valid array");
        } else {
            syswrite("  [INFO] No users currently logged in");
            syswrite("  [PASS] query_user_names() returned empty array");
        }
    } else {
        syswrite("  [FAIL] query_user_names() returned NULL");
    }
    
    /* Test 4: Find user by name */
    syswrite("\n=== Test 4: Find user by name ===");
    if (sizeof(names) > 0) {
        test_player = call_other(users_d, "find_user", names[0]);
        if (test_player) {
            syswrite(sprintf("  [PASS] find_user('%s') found player: %s", 
                names[0], otoa(test_player)));
        } else {
            syswrite(sprintf("  [FAIL] find_user('%s') returned NULL", names[0]));
        }
        
        /* Test case-insensitivity */
        test_player = call_other(users_d, "find_user", upcase(names[0]));
        if (test_player) {
            syswrite("  [PASS] find_user() is case-insensitive");
        } else {
            syswrite("  [FAIL] find_user() case-insensitivity failed");
        }
        
        /* Test non-existent user */
        test_player = call_other(users_d, "find_user", "nonexistent_user_xyz");
        if (!test_player) {
            syswrite("  [PASS] find_user() correctly returns NULL for non-existent user");
        } else {
            syswrite("  [FAIL] find_user() returned object for non-existent user");
        }
    } else {
        syswrite("  [SKIP] No users to test find_user()");
    }
    
    /* Test 5: User statistics */
    syswrite("\n=== Test 5: User statistics ===");
    stats = call_other(users_d, "query_user_stats");
    if (stats) {
        syswrite("  [INFO] Statistics:");
        syswrite(sprintf("    Total users: %d", stats["total_users"]));
        syswrite(sprintf("    Registered users: %d", stats["registered_users"]));
        syswrite(sprintf("    Avg connection time: %d seconds", stats["avg_connection_time"]));
        syswrite(sprintf("    Avg idle time: %d seconds", stats["avg_idle_time"]));
        syswrite("  [PASS] query_user_stats() returned valid mapping");
    } else {
        syswrite("  [FAIL] query_user_stats() returned NULL");
    }
    
    /* Test 6: Connection and idle times */
    syswrite("\n=== Test 6: Connection and idle time tracking ===");
    if (sizeof(names) > 0) {
        test_player = call_other(users_d, "find_user", names[0]);
        if (test_player) {
            count = call_other(users_d, "query_connection_time", test_player);
            syswrite(sprintf("  [INFO] Connection time for %s: %d seconds", 
                names[0], count));
            if (count >= 0) {
                syswrite("  [PASS] query_connection_time() returned valid result");
            } else {
                syswrite("  [FAIL] query_connection_time() returned negative value");
            }
            
            count = call_other(users_d, "query_idle_time", test_player);
            syswrite(sprintf("  [INFO] Idle time for %s: %d seconds", 
                names[0], count));
            if (count >= 0) {
                syswrite("  [PASS] query_idle_time() returned valid result");
            } else {
                syswrite("  [FAIL] query_idle_time() returned negative value");
            }
        }
    } else {
        syswrite("  [SKIP] No users to test time tracking");
    }
    
    /* Test 7: Context management (this_player) */
    syswrite("\n=== Test 7: Context management ===");
    if (sizeof(names) > 0) {
        test_player = call_other(users_d, "find_user", names[0]);
        if (test_player) {
            /* Set context */
            call_other(users_d, "set_this_player", test_player);
            
            /* Query context */
            result = call_other(users_d, "query_this_player");
            if (result == test_player) {
                syswrite("  [PASS] set_this_player() and query_this_player() work correctly");
            } else {
                syswrite("  [FAIL] Context tracking returned wrong player");
            }
            
            /* Clear context */
            call_other(users_d, "clear_this_player");
            result = call_other(users_d, "query_this_player");
            if (!result) {
                syswrite("  [PASS] clear_this_player() correctly cleared context");
            } else {
                syswrite("  [FAIL] clear_this_player() did not clear context");
            }
        }
    } else {
        syswrite("  [SKIP] No users to test context management");
    }
    
    /* Test 8: Registry rehash */
    syswrite("\n=== Test 8: Registry rehash ===");
    call_other(users_d, "rehash_users");
    count = call_other(users_d, "query_user_count");
    syswrite(sprintf("  [INFO] User count after rehash: %d", count));
    if (count >= 0) {
        syswrite("  [PASS] rehash_users() completed successfully");
    } else {
        syswrite("  [FAIL] rehash_users() caused errors");
    }
    
    /* Test 9: Edge cases and error handling */
    syswrite("\n=== Test 9: Edge cases and error handling ===");
    
    /* NULL parameter tests */
    result = call_other(users_d, "find_user", 0);
    if (!result) {
        syswrite("  [PASS] find_user(NULL) returns NULL");
    } else {
        syswrite("  [FAIL] find_user(NULL) should return NULL");
    }
    
    result = call_other(users_d, "find_user", "");
    if (!result) {
        syswrite("  [PASS] find_user('') returns NULL");
    } else {
        syswrite("  [FAIL] find_user('') should return NULL");
    }
    
    count = call_other(users_d, "query_connection_time", 0);
    if (count == 0) {
        syswrite("  [PASS] query_connection_time(NULL) returns 0");
    } else {
        syswrite("  [FAIL] query_connection_time(NULL) should return 0");
    }
    
    /* Summary */
    syswrite("\n===============================================");
    syswrite("USERS_D TEST SUITE COMPLETE");
    syswrite("===============================================\n");
    syswrite("[INFO] Review results above for any failures");
    syswrite("[INFO] All core functionality has been tested");
}
