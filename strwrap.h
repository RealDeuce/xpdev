#ifndef _STRWRAP_H_
#define _STRWRAP_H_

#include <string.h>
#include <stdlib.h>
#include "gen_defs.h"
#include "wrapdll.h"

#if !defined _MSC_VER && !defined __BORLANDC__

#if defined(__cplusplus)
extern "C" {
#endif
DLLEXPORT char* itoa(int val, char* str, int radix);
DLLEXPORT char* ltoa(long val, char* str, int radix);
#if defined(__cplusplus)
}
#endif

#if (!defined(__MINGW32__)) || (__GNUC__ < 5)
#define strset(x, y) memset(x, y, strlen(x))
#endif

#endif

#if defined(NEEDS_STRNDUP)
static inline char* xpdev_strndup_local(const char* str, size_t maxlen)
{
	size_t len = 0;
	char* copy;

	while (len < maxlen && str[len] != '\0')
		++len;
	copy = (char*)malloc(len + 1);
	if (copy != NULL) {
		memcpy(copy, str, len);
		copy[len] = '\0';
	}
	return copy;
}
#define strndup xpdev_strndup_local
#endif

#if (defined(_MSC_VER) || defined(__MSVCRT__)) && defined(_WIN32) && !defined(_MSC_VER)
#if defined(__cplusplus)
extern "C" {
#endif
size_t strnlen(const char *s, size_t maxlen);
#if defined(__cplusplus)
}
#endif
#endif

#if defined(__cplusplus)
extern "C" {
#endif
#if defined(__EMSCRIPTEN__)
char * strdup(const char *str);
char * strtok_r(char *str, const char *delim, char **saveptr);
#endif
#if defined(__cplusplus)
}
#endif

#endif
