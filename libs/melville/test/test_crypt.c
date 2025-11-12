/* test_crypt.c - Test the crypt() efun for password hashing */

run_tests() {
    string password1, password2, hash1, hash2;
    int result;
    
    syswrite("\n===============================================");
    syswrite("crypt() efun Test Suite");
    syswrite("===============================================\n");
    
    /* Test 1: Basic password hashing */
    syswrite("=== Test 1: Hash a password ===");
    password1 = "test_password_123";
    hash1 = crypt(password1);
    
    if (strlen(hash1) > 0) {
        syswrite("  [PASS] Password hashed successfully");
        syswrite(sprintf("  [INFO] Hash length: %d characters", strlen(hash1)));
    } else {
        syswrite("  [FAIL] Failed to hash password");
    }
    
    /* Test 2: Same password creates different hashes (random salt) */
    syswrite("\n=== Test 2: Random salt generation ===");
    hash2 = crypt(password1);
    
    if (hash1 != hash2) {
        syswrite("  [PASS] Same password produces different hashes (random salts)");
        syswrite("  [INFO] Hash1: " + hash1);
        syswrite("  [INFO] Hash2: " + hash2);
    } else {
        syswrite("  [FAIL] Same password produced identical hashes");
    }
    
    /* Test 3: Password verification - correct password */
    syswrite("\n=== Test 3: Verify correct password ===");
    result = crypt(password1, hash1);
    
    if (result == 1) {
        syswrite("  [PASS] Correct password verified successfully");
    } else {
        syswrite("  [FAIL] Failed to verify correct password");
    }
    
    /* Test 4: Password verification - wrong password */
    syswrite("\n=== Test 4: Reject incorrect password ===");
    password2 = "wrong_password";
    result = crypt(password2, hash1);
    
    if (result == 0) {
        syswrite("  [PASS] Incorrect password correctly rejected");
    } else {
        syswrite("  [FAIL] Incorrect password was accepted!");
    }
    
    /* Test 5: Test with different passwords */
    syswrite("\n=== Test 5: Different passwords ===");
    password2 = "another_password_456";
    hash2 = crypt(password2);
    
    result = crypt(password2, hash2);
    if (result == 1) {
        syswrite("  [PASS] Second password hashed and verified correctly");
    } else {
        syswrite("  [FAIL] Second password verification failed");
    }
    
    /* Test 6: Cross-verification (password1 vs hash2) */
    result = crypt(password1, hash2);
    if (result == 0) {
        syswrite("  [PASS] Cross-verification correctly rejected");
    } else {
        syswrite("  [FAIL] Cross-verification incorrectly accepted");
    }
    
    /* Test 7: Test hash_password() and verify_password() from auto.c */
    syswrite("\n=== Test 7: auto.c wrapper functions ===");
    hash1 = hash_password(password1);
    result = verify_password(password1, hash1);
    
    if (result == 1) {
        syswrite("  [PASS] hash_password() and verify_password() work correctly");
    } else {
        syswrite("  [FAIL] auto.c wrapper functions failed");
    }
    
    syswrite("\n===============================================");
    syswrite("CRYPT TEST SUITE COMPLETE");
    syswrite("===============================================\n");
}
