#ifndef SHA1_H
#define SHA1_H

/*
   SHA-1 in C
   By Steve Reid <steve@edmweb.com>
   100% Public Domain
 */

#include <stddef.h>     /* size_t */
#include <gen_defs.h>   /* uint32_t */
#include "hash_export.h"

#define SHA1_DIGEST_SIZE 20

typedef struct
{
	uint32_t state[5];
	uint32_t count[2];
	uint8_t buffer[64];
} SHA1_CTX;

#ifdef __cplusplus
extern "C" {
#endif

XPDEV_HASH_EXPORT void SHA1Transform(
	uint32_t state[5],
	const uint8_t buffer[64]
	);

XPDEV_HASH_EXPORT void SHA1Init(
	SHA1_CTX * context
	);

XPDEV_HASH_EXPORT void SHA1Update(
	SHA1_CTX * context,
	const void * data,
	size_t len
	);

XPDEV_HASH_EXPORT void SHA1Final(
	SHA1_CTX * context,
	uint8_t digest[SHA1_DIGEST_SIZE]
	);

XPDEV_HASH_EXPORT void SHA1_calc(
	uint8_t *hash_out,
	const void *str,
	size_t len);

XPDEV_HASH_EXPORT char* SHA1_hex(char* to, const uint8_t digest[SHA1_DIGEST_SIZE]);

#ifdef __cplusplus
}
#endif

#endif /* SHA1_H */
