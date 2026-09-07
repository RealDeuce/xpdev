/* Semaphore-related cross-platform development wrappers */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright Rob Swindell - http://www.synchro.net/copyright.html			*
 *																			*
 * This library is free software; you can redistribute it and/or			*
 * modify it under the terms of the GNU Lesser General Public License		*
 * as published by the Free Software Foundation; either version 2			*
 * of the License, or (at your option) any later version.					*
 * See the GNU Lesser General Public License for more details: lgpl.txt or	*
 * http://www.fsf.org/copyleft/lesser.html									*
 *																			*
 * For Synchronet coding style and modification guidelines, see				*
 * http://www.synchro.net/source.html										*
 *																			*
 * Note: If this box doesn't appear square, then you need to fix your tabs.	*
 ****************************************************************************/

#include <errno.h>
#include "semwrap.h"

/* Older Windows CRTs, including Borland's, do not define every POSIX errno
 * value used by the semaphore API.  Keep the precise value where it exists
 * and otherwise use the closest errno understood by that CRT. */
#if defined(EOVERFLOW)
	#define XPDEV_SEM_EOVERFLOW EOVERFLOW
#else
	#define XPDEV_SEM_EOVERFLOW ERANGE
#endif
#if defined(ENOSYS)
	#define XPDEV_SEM_ENOSYS ENOSYS
#else
	#define XPDEV_SEM_ENOSYS EINVAL
#endif

#if defined(__unix__)

#include <sys/time.h>   /* timespec */
#include <stdlib.h> /* NULL */

int
xp_sem_trywait_block(sem_t *sem, uint32_t timeout)
{
	int             retval;
	long            nanoseconds;
	struct timespec abstime;
	struct timeval  currtime;

	if (gettimeofday(&currtime, NULL) != 0)
		return -1;
	abstime.tv_sec = currtime.tv_sec + (time_t)(timeout / 1000);
	nanoseconds = currtime.tv_usec * 1000 + (long)(timeout % 1000) * 1000000;
	abstime.tv_sec += nanoseconds / 1000000000;
	abstime.tv_nsec = nanoseconds % 1000000000;

	retval = sem_timedwait(sem, &abstime);
	if (retval && errno == ETIMEDOUT)
		errno = EAGAIN;
	return retval;
}

#elif defined(_WIN32)

#include <limits.h>     /* INT_MAX */

static int
sem_error(int* error_out, int error)
{
	if (error_out != NULL)
		*error_out = error;
	return -1;
}

static int
win32_error(DWORD error, int* error_out)
{
	switch (error) {
		case ERROR_ACCESS_DENIED:
			return sem_error(error_out, EACCES);
		case ERROR_INVALID_HANDLE:
		case ERROR_INVALID_PARAMETER:
			return sem_error(error_out, EINVAL);
		case ERROR_NOT_ENOUGH_MEMORY:
		case ERROR_OUTOFMEMORY:
			return sem_error(error_out, ENOMEM);
		case ERROR_TOO_MANY_POSTS:
			return sem_error(error_out, XPDEV_SEM_EOVERFLOW);
		default:
			return sem_error(error_out, EIO);
	}
}

#if defined(__BORLANDC__)
	#pragma argsused
#endif
int xpdev_sem_init_impl(sem_t* psem, int pshared, unsigned int value, int* error_out)
{
	if (psem == NULL || value > INT_MAX)
		return sem_error(error_out, EINVAL);
	if (pshared != 0)
		return sem_error(error_out, XPDEV_SEM_ENOSYS);
	if ((*(psem) = CreateSemaphore(NULL, value, INT_MAX, NULL)) == NULL)
		return win32_error(GetLastError(), error_out);

	return 0;
}

int xpdev_sem_trywait_block_impl(sem_t* psem, uint32_t timeout, int* error_out)
{
	DWORD result;

	if (psem == NULL || *psem == NULL)
		return sem_error(error_out, EINVAL);
	result = WaitForSingleObject(*psem, timeout);
	switch (result) {
		case WAIT_OBJECT_0:
			return 0;
		case WAIT_TIMEOUT:
			return sem_error(error_out, EAGAIN);
		case WAIT_FAILED:
			return win32_error(GetLastError(), error_out);
		default:
			return sem_error(error_out, EIO);
	}
}

int xpdev_sem_post_impl(sem_t* psem, int* error_out)
{
	if (psem == NULL || *psem == NULL)
		return sem_error(error_out, EINVAL);
	if (ReleaseSemaphore(*psem, 1, NULL) == TRUE)
		return 0;

	return win32_error(GetLastError(), error_out);
}

int xpdev_sem_destroy_impl(sem_t* psem, int* error_out)
{
	if (psem == NULL || *psem == NULL)
		return sem_error(error_out, EINVAL);
	if (CloseHandle(*psem) == TRUE) {
		*psem = NULL;
		return 0;
	}
	return win32_error(GetLastError(), error_out);
}

#endif /* _WIN32 */
