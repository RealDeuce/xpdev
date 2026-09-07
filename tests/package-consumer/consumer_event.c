#include <stddef.h>

#include <xpdev/eventwrap.h>

int
main(void)
{
	xpevent_t event;

	event = CreateEvent(NULL, FALSE, TRUE, NULL);
	if (event == NULL)
		return 1;
	if (WaitForEvent(event, 0) != WAIT_OBJECT_0)
		return 2;
	if (WaitForEvent(event, 0) != WAIT_TIMEOUT)
		return 3;
	if (!SetEvent(event))
		return 4;
	if (WaitForEvent(event, 0) != WAIT_OBJECT_0)
		return 5;
	if (!CloseEvent(event))
		return 6;

	event = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (event == NULL)
		return 7;
	if (WaitForEvent(event, 0) != WAIT_TIMEOUT)
		return 8;
	if (!SetEvent(event))
		return 9;
	if (WaitForEvent(event, 0) != WAIT_OBJECT_0)
		return 10;
	if (WaitForEvent(event, 0) != WAIT_OBJECT_0)
		return 11;
	if (!ResetEvent(event))
		return 12;
	if (WaitForEvent(event, 0) != WAIT_TIMEOUT)
		return 13;
	if (!CloseEvent(event))
		return 14;
	return 0;
}
