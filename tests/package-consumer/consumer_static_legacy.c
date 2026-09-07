#include <stdlib.h>
#include <string.h>

#include <xpdev/link_list.h>
#include <xpdev/stbuf.h>
#include <xpdev/str_list.h>
#include <xpdev/xpbeep.h>

int main(void)
{
	link_list_t nodes;
	str_list_t strings = strListInit();
	char* string = malloc(4);
#if defined(_WIN32)
	void* node_data = listAllocData(4);
#else
	void* node_data = malloc(4);
#endif
	void* storage = malloc(STBUF_SIZE(1));
	stbuf buffer;

	if (strings == NULL || string == NULL || node_data == NULL || storage == NULL)
		return 1;
	strcpy(string, "old");
	if (strListAnnex(&strings, string, STR_LIST_LAST_INDEX) == NULL)
		return 2;
	if (strListReplace(strings, 0, "legacy static allocation") == NULL)
		return 3;
	strListFree(&strings);

	memcpy(node_data, "raw", 4);
	if (listInit(&nodes, 0) == NULL)
		return 4;
	if (listAddNodeWithFlags(&nodes, node_data, 0, LINK_LIST_MALLOC, LAST_NODE) == NULL)
		return 5;
	if (!listFree(&nodes))
		return 6;

	buffer = stbuf_frommem(storage, STBUF_SIZE(1), true);
	if (buffer == NULL || !stbuf_strcpy(&buffer, "legacy static buffer"))
		return 7;
	stbuf_free(buffer);

#if XPDEV_THREAD_SAFE_ENABLED
	{
		int16_t* audio = malloc(XPBEEP_FRAMESIZE);
		if (audio == NULL)
			return 8;
		if (xp_audio_append(-1, audio, 1, NULL))
			return 9;
	}
#endif
	return 0;
}
