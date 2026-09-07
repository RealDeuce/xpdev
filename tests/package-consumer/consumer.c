#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * Some C libraries expose their conforming functions through fortified
 * macros.  Remove those system definitions after stdio.h has declared the
 * functions so this test can detect only macros introduced by XPDev.
 */
#ifdef snprintf
	#undef snprintf
#endif
#ifdef vsnprintf
	#undef vsnprintf
#endif

#include <xpdev/comio/comio.h>
#include <xpdev/encode/base64.h>
#include <xpdev/genwrap.h>
#include <xpdev/hash/crc32.h>
#include <xpdev/link_list.h>
#if XPDEV_USE_XP_SEMAPHORES
#include <xpdev/xpsem.h>
#endif

#if XPDEV_LINK_LIST_THREADSAFE != XPDEV_CONSUMER_EXPECT_LINK_LIST_THREADSAFE
	#error "XPDev linked-list ABI metadata does not match its public header"
#endif
#if XPDEV_THREAD_SAFE_ENABLED != XPDEV_CONSUMER_EXPECT_THREAD_SAFE
	#error "XPDev thread-safety metadata does not match its public header"
#endif
#if XPDEV_USE_SYSTEM_PTHREADS != XPDEV_CONSUMER_EXPECT_SYSTEM_PTHREADS
	#error "XPDev pthread metadata does not match its public header"
#endif
#if XPDEV_USE_NATIVE_POSIX_SEMAPHORES != XPDEV_CONSUMER_EXPECT_NATIVE_POSIX_SEMAPHORES
	#error "XPDev native POSIX semaphore metadata does not match its public header"
#endif
#if XPDEV_USE_XP_SEMAPHORES != XPDEV_CONSUMER_EXPECT_XP_SEMAPHORES
	#error "XPDev semaphore ABI metadata does not match its public header"
#endif
#if XPDEV_USE_NATIVE_POSIX_SEMAPHORES && XPDEV_USE_XP_SEMAPHORES
	#error "XPDev selected two incompatible semaphore implementations"
#endif

#if XPDEV_LINK_LIST_THREADSAFE
	#ifndef LINK_LIST_THREADSAFE
		#error "XPDev CMake target did not propagate LINK_LIST_THREADSAFE"
	#endif
#elif defined(LINK_LIST_THREADSAFE)
	#error "XPDev CMake target propagated an incompatible LINK_LIST_THREADSAFE"
#endif

#if XPDEV_THREAD_SAFE_ENABLED
	#ifndef XPDEV_THREAD_SAFE
		#error "XPDev CMake target did not propagate XPDEV_THREAD_SAFE"
	#endif
#elif defined(XPDEV_THREAD_SAFE)
	#error "XPDev CMake target propagated an incompatible XPDEV_THREAD_SAFE"
#endif

#if XPDEV_USE_XP_SEMAPHORES
	#ifndef USE_XP_SEMAPHORES
		#error "XPDev CMake target did not propagate USE_XP_SEMAPHORES"
	#endif
#elif defined(USE_XP_SEMAPHORES)
	#error "XPDev CMake target propagated an incompatible USE_XP_SEMAPHORES"
#endif

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
#if XPDEV_USE_XP_SEMAPHORES
	xp_sem_t sem;
	xp_sem_t null_sem = NULL;
	int sem_value;
#endif

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
#if XPDEV_USE_XP_SEMAPHORES
	/* Exercise the conditional implementation used by semwrap's fallback. */
	if (xp_sem_init(&sem, 0, 0) != 0)
		return 8;
	errno = 0;
	if (xp_sem_trywait(&sem) != -1 || errno != EAGAIN)
		return 9;
	if (xp_sem_post(&sem) != 0)
		return 10;
	if (xp_sem_getvalue(&sem, &sem_value) != 0 || sem_value != 1)
		return 11;
	if (xp_sem_wait(&sem) != 0)
		return 12;
	errno = 0;
	if (xp_sem_setvalue(&sem, -1) != -1 || errno != EINVAL)
		return 13;
	if (xp_sem_getvalue(&sem, &sem_value) != 0 || sem_value != 0)
		return 14;
	if (xp_sem_destroy(&sem) != 0)
		return 15;
	errno = 0;
	if (xp_sem_wait(&null_sem) != -1 || errno != EINVAL)
		return 16;
	if (xp_sem_init(&sem, 0, INT_MAX) != 0)
		return 17;
	errno = 0;
	if (xp_sem_post(&sem) != -1 || errno != EOVERFLOW)
		return 18;
	if (xp_sem_destroy(&sem) != 0)
		return 19;
	errno = 0;
	if (xp_sem_init(&sem, 0, UINT_MAX) != -1 || errno != EINVAL)
		return 20;
#endif
	return 0;
}
