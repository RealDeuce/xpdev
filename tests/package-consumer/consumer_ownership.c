#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xpdev/dat_file.h>
#include <xpdev/filewrap.h>
#include <xpdev/ini_file.h>
#include <xpdev/msg_queue.h>
#include <xpdev/named_str_list.h>
#include <xpdev/stbuf.h>
#include <xpdev/str_list.h>
#include <xpdev/strwrap.h>
#include <xpdev/xpbeep.h>
#include <xpdev/xpprintf.h>

static char* create_test_line(const str_list_t columns)
{
	const char* value = columns != NULL && columns[0] != NULL ? columns[0] : "";
	char* line = dataLineAlloc(strlen(value) + 1);
	if (line != NULL)
		strcpy(line, value);
	return line;
}

static int test_local_helpers(void)
{
	void* resized = malloc(1);
	if (resized == NULL)
		return 1;
	resized = realloc_or_free(resized, 32);
	if (resized == NULL)
		return 2;
	free(resized);

#if defined(NEEDS_ASPRINTF)
	char* formatted = NULL;
	if (asprintf(&formatted, "%s-%d", "local", 7) != 7)
		return 3;
	if (strcmp(formatted, "local-7") != 0)
		return 4;
	free(formatted);
#endif

#if defined(NEEDS_STRNDUP)
	char* duplicate = strndup("caller-local", 6);
	if (duplicate == NULL || strcmp(duplicate, "caller") != 0)
		return 5;
	free(duplicate);
#endif

#if defined(NEEDS_GETDELIM)
	FILE* fp = tmpfile();
	char* line = NULL;
	size_t capacity = 0;
	if (fp == NULL)
		return 6;
	fputs("caller stream\n", fp);
	rewind(fp);
	if (getdelim(&line, &capacity, '\n', fp) != 14)
		return 7;
	if (strcmp(line, "caller stream\n") != 0)
		return 8;
	free(line);
	fclose(fp);
#endif
	return 0;
}

static int test_string_families(void)
{
	str_list_t list = strListInit();
	str_list_t source = strListInit();
	char* owned;
	char* removed;

	if (list == NULL || source == NULL)
		return 1;
	owned = strListAllocString(6);
	if (owned == NULL)
		return 2;
	strcpy(owned, "owned");
	if (strListAnnex(&list, owned, STR_LIST_LAST_INDEX) == NULL)
		return 3;
	if (strListReplace(list, 0, "replacement") == NULL)
		return 4;
	removed = strListRemove(&list, 0);
	if (removed == NULL || strcmp(removed, "replacement") != 0)
		return 5;
	strListFreeString(removed);

	if (strListAppend(&source, "merged", STR_LIST_LAST_INDEX) == NULL)
		return 6;
	if (strListMerge(&list, source) != 1)
		return 7;
	strListFreeContainer(source);
	if (strListCount(list) != 1 || strcmp(list[0], "merged") != 0)
		return 8;
	strListFree(&list);
	return 0;
}

static int test_list_and_queue_families(void)
{
	link_list_t list;
	msg_queue_t queue;
	char* payload;
	char* removed;

	if (listInit(&list, 0) == NULL)
		return 1;
	payload = listAllocData(8);
	if (payload == NULL)
		return 2;
	strcpy(payload, "payload");
	if (listAddNodeWithFlags(&list, payload, 0, LINK_LIST_MALLOC, LAST_NODE) == NULL)
		return 3;
	removed = listPopNode(&list);
	if (removed == NULL || strcmp(removed, "payload") != 0)
		return 4;
	listFreeData(removed);
	if (!listFree(&list))
		return 5;

	if (msgQueueInit(&queue, 0) == NULL)
		return 6;
	if (!msgQueueWrite(&queue, "message", 8))
		return 7;
	removed = msgQueueRead(&queue, 0);
	if (removed == NULL || memcmp(removed, "message", 8) != 0)
		return 8;
	msgQueueFreeMessage(removed);
	if (!msgQueueFree(&queue))
		return 9;
	return 0;
}

static int test_ini_and_named_families(void)
{
	static char* enum_names[] = { "zero", "one", NULL };
	named_string_t** named = NULL;
	unsigned count;
	unsigned* enums;
	int* ints;

	enums = parseEnumList("zero,one", ",", enum_names, &count);
	if (enums == NULL || count != 2 || enums[0] != 0 || enums[1] != 1)
		return 1;
	iniFreeEnumList(enums);
	ints = parseIntList("12,-3", ",", &count);
	if (ints == NULL || count != 2 || ints[0] != 12 || ints[1] != -3)
		return 2;
	iniFreeIntList(ints);

	if (namedStrListInsert(&named, "name", "value", NAMED_STR_LIST_LAST_INDEX) == NULL)
		return 3;
	if (!namedStrListSetValue(named[0], "second"))
		return 4;
	if (!namedStrListSetName(named[0], "renamed"))
		return 5;
	if (!namedStrListReplace(named[0], "final-name", "final-value"))
		return 6;
	if (strcmp(named[0]->name, "final-name") != 0
	    || strcmp(named[0]->value, "final-value") != 0)
		return 7;
	named = namedStrListFree(named);
	return named == NULL ? 0 : 8;
}

static int test_data_and_audio_families(void)
{
	str_list_t columns = strListInit();
	str_list_t created;
	char* line;
	int16_t* audio;
	size_t frames;
	unsigned char sample[] = { 0, 255 };

	if (columns == NULL || strListAppend(&columns, "field", 0) == NULL)
		return 1;
	line = csvLineCreator(columns);
	if (line == NULL)
		return 2;
	dataLineFree(line);
	created = dataCreateList(NULL, columns, create_test_line);
	if (created == NULL || created[0] == NULL || strcmp(created[0], "field") != 0)
		return 3;
	strListFree(&created);
	strListFree(&columns);

	audio = xp_audio_buffer_alloc(2);
	if (audio == NULL)
		return 4;
	xp_audio_buffer_free(audio);
	audio = xp_u8mono22k_to_s16stereo44k(sample, sizeof(sample), &frames);
	if (audio == NULL || frames != 4)
		return 5;
	xp_audio_buffer_free(audio);
#if XPDEV_THREAD_SAFE_ENABLED
	audio = xp_audio_buffer_alloc(1);
	if (audio == NULL)
		return 6;
	if (xp_audio_append(-1, audio, 1, NULL))
		return 7;
#endif
	return 0;
}

static int test_stbuf_origin(void)
{
	size_t size = STBUF_SIZE(1);
	void* memory = malloc(size);
	stbuf buffer;

	if (memory == NULL)
		return 1;
	buffer = stbuf_frommem(memory, size, true);
	if (buffer == NULL)
		return 2;
	if (!stbuf_strcpy(&buffer, "grown through the shared library"))
		return 3;
	if (strcmp(buffer->buf, "grown through the shared library") != 0)
		return 4;
	stbuf_free(buffer);
	return 0;
}

int main(void)
{
	char* formatted = xp_asprintf("%s-%d", "xpdev", 1);
	int result;

	if (formatted == NULL || strcmp(formatted, "xpdev-1") != 0)
		return 1;
	xp_asprintf_free(formatted);
	if ((result = test_local_helpers()) != 0)
		return 10 + result;
	if ((result = test_string_families()) != 0)
		return 30 + result;
	if ((result = test_list_and_queue_families()) != 0)
		return 50 + result;
	if ((result = test_ini_and_named_families()) != 0)
		return 70 + result;
	if ((result = test_data_and_audio_families()) != 0)
		return 90 + result;
	if ((result = test_stbuf_origin()) != 0)
		return 110 + result;
	return 0;
}
