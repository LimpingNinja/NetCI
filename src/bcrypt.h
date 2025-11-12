/* bcrypt.h - Password hashing using bcrypt algorithm
 * 
 * Based on OpenBSD's bcrypt implementation
 * Portable version for NetCI
 */

#ifndef BCRYPT_H
#define BCRYPT_H

/* Bcrypt hash length (including salt and settings) */
#define BCRYPT_HASHSIZE 64

/* Generate a bcrypt hash from a password
 * Returns: newly allocated hash string (caller must FREE)
 * Returns NULL on error
 */
char *bcrypt_hash(const char *password);

/* Verify a password against a bcrypt hash
 * Returns: 1 if password matches, 0 if not
 */
int bcrypt_verify(const char *password, const char *hash);

#endif /* BCRYPT_H */
