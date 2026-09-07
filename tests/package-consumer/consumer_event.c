#include <xpdev/eventwrap.h>

static BOOL
reject_event(void *unused)
{
	(void)unused;
	return FALSE;
}

int
main(void)
{
	xpevent_t event = CreateEvent(NULL, FALSE, TRUE, NULL);

	if (event == NULL)
		return 1;
	event->verify = reject_event;
	if (WaitForEvent(event, 0) != WAIT_TIMEOUT)
		return 2;
	event->verify = NULL;
	if (WaitForEvent(event, 0) != WAIT_OBJECT_0)
		return 3;
	if (!CloseEvent(event))
		return 4;
	return 0;
}
