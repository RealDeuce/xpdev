#include <errno.h>
#include <limits.h>
#include <windows.h>

#include <xpdev/rwlockwrap.h>
#include <xpdev/semwrap.h>

struct invalid_unlock_context {
	rwlock_t *lock;
	LONG result;
};

static DWORD WINAPI
invalid_unlock_thread(void *arg)
{
	struct invalid_unlock_context *context = arg;

	InterlockedExchange(&context->result, rwlock_unlock(context->lock));
	return 0;
}

int
main(void)
{
	struct invalid_unlock_context context;
	DWORD before;
	DWORD after;
	HANDLE thread;
	rwlock_t lock;
	sem_t sem;
	sem_t null_sem = NULL;

	if (!GetProcessHandleCount(GetCurrentProcess(), &before))
		return 1;
	for (unsigned i = 0; i < 4; i++) {
		if (!rwlock_init(&lock))
			return 2;
		if (!rwlock_destroy(&lock))
			return 3;
	}
	if (!GetProcessHandleCount(GetCurrentProcess(), &after) || after != before)
		return 4;

	if (!rwlock_init(&lock))
		return 5;
	if (!rwlock_rdlock(&lock))
		return 6;
	context.lock = &lock;
	context.result = TRUE;
	thread = CreateThread(NULL, 0, invalid_unlock_thread, &context, 0, NULL);
	if (thread == NULL)
		return 7;
	if (WaitForSingleObject(thread, INFINITE) != WAIT_OBJECT_0)
		return 8;
	if (!CloseHandle(thread))
		return 9;
	if (context.result)
		return 10;
	if (!rwlock_unlock(&lock))
		return 11;
	if (!rwlock_destroy(&lock))
		return 12;

	errno = 0;
	if (sem_init(&sem, 0, UINT_MAX) != -1 || errno != EINVAL)
		return 13;
	errno = 0;
	if (sem_init(&sem, 1, 0) != -1 || errno != ENOSYS)
		return 14;
	if (sem_init(&sem, 0, INT_MAX) != 0)
		return 15;
	errno = 0;
	if (sem_post(&sem) != -1 || errno != EOVERFLOW)
		return 16;
	if (sem_trywait(&sem) != 0)
		return 17;
	if (sem_destroy(&sem) != 0 || sem != NULL)
		return 18;
	errno = 0;
	if (sem_wait(&null_sem) != -1 || errno != EINVAL)
		return 19;
	return 0;
}
