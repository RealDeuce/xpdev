#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <xpdev/comio/comio.h>
#include <xpdev/encode/base64.h>
#include <xpdev/genwrap.h>
#include <xpdev/hash/crc32.h>

#ifdef snprintf
	#error "XPDev public headers must not redefine snprintf"
#endif
#ifdef vsnprintf
	#error "XPDev public headers must not redefine vsnprintf"
#endif

int main(void)
{
	char encoded[8];
	char truncated[2];
	char version[64];

	if (comVersion(version, sizeof(version)) != version || version[0] == '\0')
		return 1;
	if (b64_encode(encoded, sizeof(encoded), "abc", 3) != 4)
		return 2;
	if (memcmp(encoded, "YWJj", 4) != 0)
		return 3;
	if (crc32("abc", 3) != 0x352441c2U)
		return 4;
	/* Exercise an exported data symbol, not just hash functions. */
	if (crc32tbl[0] != 0)
		return 5;
	if (snprintf(truncated, sizeof(truncated), "%s", "abc") != 3)
		return 6;
	if (strcmp(truncated, "a") != 0)
		return 7;
	return 0;
}
