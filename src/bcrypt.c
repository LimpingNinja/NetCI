/* bcrypt.c - Simple password hashing using system crypt() with salt generation
 * 
 * This is a simplified implementation that uses the system's crypt() function
 * with randomly generated salts. On systems with bcrypt support, it will use
 * bcrypt. Otherwise, it falls back to the best available algorithm.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef __APPLE__
/* macOS doesn't have crypt() in the standard library anymore */
/* We'll use a simple SHA-256 based approach */
#include <CommonCrypto/CommonDigest.h>
#define USE_COMMON_CRYPTO 1
#else
/* Linux and other Unix systems */
#define _GNU_SOURCE
#include <crypt.h>
#endif

#include "bcrypt.h"

/* Generate a random salt for hashing
 * Returns: newly allocated salt string
 */
static char *generate_salt() {
    static const char charset[] = 
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789./";
    char *salt;
    int i;
    
    /* For bcrypt format: $2b$10$<22 character salt> */
    salt = (char *)malloc(30);
    if (!salt) return NULL;
    
    /* Initialize random seed if not already done */
    static int seeded = 0;
    if (!seeded) {
        srand(time(NULL) ^ getpid());
        seeded = 1;
    }
    
#ifdef USE_COMMON_CRYPTO
    /* Simple salt format for our custom implementation */
    strcpy(salt, "$SHA$");
    for (i = 0; i < 16; i++) {
        salt[5 + i] = charset[rand() % (sizeof(charset) - 1)];
    }
    salt[21] = '$';
    salt[22] = '\0';
#else
    /* bcrypt format: $2b$10$<22 chars> */
    strcpy(salt, "$2b$10$");
    for (i = 0; i < 22; i++) {
        salt[7 + i] = charset[rand() % (sizeof(charset) - 1)];
    }
    salt[29] = '\0';
#endif
    
    return salt;
}

#ifdef USE_COMMON_CRYPTO
/* macOS implementation using CommonCrypto SHA-256 */
static char *sha256_crypt(const char *password, const char *salt) {
    CC_SHA256_CTX ctx;
    unsigned char hash[CC_SHA256_DIGEST_LENGTH];
    char *result;
    int i;
    
    /* Combine salt and password */
    CC_SHA256_Init(&ctx);
    CC_SHA256_Update(&ctx, salt, strlen(salt));
    CC_SHA256_Update(&ctx, password, strlen(password));
    CC_SHA256_Final(hash, &ctx);
    
    /* Do multiple rounds (similar to bcrypt's cost factor) */
    for (i = 0; i < 1000; i++) {
        CC_SHA256_Init(&ctx);
        CC_SHA256_Update(&ctx, hash, CC_SHA256_DIGEST_LENGTH);
        CC_SHA256_Final(hash, &ctx);
    }
    
    /* Format: $SHA$<salt>$<hex hash> */
    result = (char *)malloc(128);
    if (!result) return NULL;
    
    /* Salt already ends with $, don't add another */
    snprintf(result, 128, "%s", salt);
    int offset = strlen(result);
    for (i = 0; i < CC_SHA256_DIGEST_LENGTH; i++) {
        snprintf(result + offset + (i * 2), 3, "%02x", hash[i]);
    }
    
    return result;
}
#endif

/* Generate a bcrypt hash from a password */
char *bcrypt_hash(const char *password) {
    char *salt, *hash, *result;
    
    if (!password) return NULL;
    
    /* Generate random salt */
    salt = generate_salt();
    if (!salt) return NULL;
    
#ifdef USE_COMMON_CRYPTO
    /* Use our SHA-256 implementation */
    result = sha256_crypt(password, salt);
    free(salt);
    return result;
#else
    /* Use system crypt() */
    struct crypt_data data;
    data.initialized = 0;
    
    hash = crypt_r(password, salt, &data);
    if (!hash) {
        free(salt);
        return NULL;
    }
    
    /* Copy the hash */
    result = (char *)malloc(strlen(hash) + 1);
    if (result) {
        strcpy(result, hash);
    }
    
    free(salt);
    return result;
#endif
}

/* Verify a password against a bcrypt hash */
int bcrypt_verify(const char *password, const char *hash) {
    char *check_hash;
    int match = 0;
    
    if (!password || !hash) return 0;
    
#ifdef USE_COMMON_CRYPTO
    /* Extract salt from hash */
    char salt[32];
    const char *dollar1, *dollar2;
    
    /* Find the salt portion: $SHA$<salt>$<hash> */
    /* We need to find: first $ (position 0), second $ (after SHA), third $ (before hash) */
    dollar1 = strchr(hash, '$');           /* First $ at position 0 */
    if (!dollar1) return 0;
    dollar1 = strchr(dollar1 + 1, '$');    /* Second $ after "SHA" */
    if (!dollar1) return 0;
    dollar2 = strchr(dollar1 + 1, '$');    /* Third $ before hash */
    if (!dollar2) return 0;
    
    /* Copy salt including delimiters: $SHA$<salt>$ */
    int salt_len = (dollar2 - hash) + 1;  /* +1 to include the third $ */
    if (salt_len >= sizeof(salt)) return 0;
    strncpy(salt, hash, salt_len);
    salt[salt_len] = '\0';
    
    /* Hash the password with the extracted salt */
    check_hash = sha256_crypt(password, salt);
    if (!check_hash) return 0;
    
    /* Compare */
    match = (strcmp(check_hash, hash) == 0);
    free(check_hash);
    
#else
    /* Use system crypt() with the hash as salt (it contains the salt) */
    struct crypt_data data;
    data.initialized = 0;
    
    check_hash = crypt_r(password, hash, &data);
    if (!check_hash) return 0;
    
    /* Compare hashes */
    match = (strcmp(check_hash, hash) == 0);
#endif
    
    return match;
}
