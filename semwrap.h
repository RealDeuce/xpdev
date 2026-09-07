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

#ifndef _SEMWRAP_H
#define _SEMWRAP_H

#include "gen_defs.h"
#include "wrapdll.h"

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__unix__)
	#if defined(USE_XP_SEMAPHORES)
		#include "xpsem.h"
		#define     sem_init(x, y, z)   xp_sem_init(x, y, z)
		#define     sem_destroy(x)      xp_sem_destroy(x)
		#define     sem_close(x)        xp_sem_close(x)
		#define     sem_unlink(x)       xp_sem_unlink(x)
		#define     sem_wait(x)         xp_sem_wait(x)
		#define     sem_trywait(x)      xp_sem_trywait(x)
		#define     sem_post(x)         xp_sem_post(x)
		#define     sem_timedwait(x, y)  xp_sem_timedwait(x, y)
		#define     sem_t               xp_sem_t
	#else
		#include <semaphore.h>  /* POSIX semaphores */
	#endif

#elif defined(_WIN32)

	#include <errno.h>
	#include <process.h>    /* _beginthread */

/* POSIX semaphores */
typedef HANDLE sem_t;

/* These are DLL implementation entry points.  error_out transfers the error
 * value without relying on the DLL and caller sharing CRT errno storage. */
DLLEXPORT int xpdev_sem_init_impl(sem_t*, int pshared, unsigned int value, int* error_out);
DLLEXPORT int xpdev_sem_post_impl(sem_t*, int* error_out);
DLLEXPORT int xpdev_sem_destroy_impl(sem_t*, int* error_out);
DLLEXPORT int xpdev_sem_trywait_block_impl(sem_t*, uint32_t timeout, int* error_out);

#if defined(__BORLANDC__)
	#define XPDEV_SEMWRAP_INLINE
#else
	#define XPDEV_SEMWRAP_INLINE inline
#endif

static XPDEV_SEMWRAP_INLINE int
xpdev_sem_result_local(int result, int error)
{
	if (result == -1)
		errno = error;
	return result;
}

static XPDEV_SEMWRAP_INLINE int
xpdev_sem_init_local(sem_t* psem, int pshared, unsigned int value)
{
	int error = 0;
	int result = xpdev_sem_init_impl(psem, pshared, value, &error);
	return xpdev_sem_result_local(result, error);
}

static XPDEV_SEMWRAP_INLINE int
xpdev_sem_post_local(sem_t* psem)
{
	int error = 0;
	int result = xpdev_sem_post_impl(psem, &error);
	return xpdev_sem_result_local(result, error);
}

static XPDEV_SEMWRAP_INLINE int
xpdev_sem_destroy_local(sem_t* psem)
{
	int error = 0;
	int result = xpdev_sem_destroy_impl(psem, &error);
	return xpdev_sem_result_local(result, error);
}

static XPDEV_SEMWRAP_INLINE int
xpdev_sem_trywait_block_local(sem_t* psem, uint32_t timeout)
{
	int error = 0;
	int result = xpdev_sem_trywait_block_impl(psem, timeout, &error);
	return xpdev_sem_result_local(result, error);
}

#undef XPDEV_SEMWRAP_INLINE

	#define sem_init                 xpdev_sem_init_local
	#define sem_post                 xpdev_sem_post_local
	#define sem_destroy              xpdev_sem_destroy_local
	#define xp_sem_trywait_block     xpdev_sem_trywait_block_local
	#define sem_wait(psem)           xp_sem_trywait_block(psem, INFINITE)
	#define sem_trywait(psem)        xp_sem_trywait_block(psem, 0)

#elif defined(__OS2__)  /* These have *not* been tested! */

/* POSIX semaphores */
typedef HEV sem_t;
	#define sem_init(psem, ps, v)         DosCreateEventSem(NULL, psem, 0, 0);
	#define sem_wait(psem)              DosWaitEventSem(*(psem), -1)
	#define sem_post(psem)              DosPostEventSem(*(psem))
	#define sem_destroy(psem)           DosCloseEventSem(*(psem))

#else

	#error "Need semaphore wrappers."

#endif

/* NOT POSIX.  The Windows definition above is caller-local so errno is set in
 * the caller's CRT; Unix exports the implementation normally. */
#if !defined(_WIN32)
DLLEXPORT int xp_sem_trywait_block(sem_t* psem, uint32_t timeout);
#endif


/* Drain all currently available posts (NOT POSIX). */
#define sem_reset(psem)                 while (sem_trywait(psem) == 0)

#if defined(__cplusplus)
}
#endif

#endif  /* Don't add anything after this line */
