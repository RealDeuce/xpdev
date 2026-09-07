#include <xpdev/comio/comio.h>
#include <xpdev/encode/base64.h>
#include <xpdev/hash/sha256.h>
#include <xpdev/threadwrap.h>

int
main(void)
{
	char version[64];
	char encoded[8];
	SHA256_CTX context;
	unsigned char digest[SHA256_DIGEST_SIZE];
	pthread_mutex_t mutex;

	if(comVersion(version, sizeof(version)) == NULL || version[0] == '\0')
		return 1;
	if(b64_encode(encoded, sizeof(encoded), "x", 1) < 0)
		return 2;
	SHA256Init(&context);
	SHA256Update(&context, "x", 1);
	SHA256Final(&context, digest);
	if(!xp_pthread_mutex_init(&mutex, false))
		return 3;
	if(pthread_mutex_destroy(&mutex) != 0)
		return 4;
	return digest[0] == 0;
}
