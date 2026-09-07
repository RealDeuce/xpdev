#include <windows.h>

#include <xpdev/rwlockwrap.h>

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
	return 0;
}
