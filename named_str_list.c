#include <stdlib.h>

#include "gen_defs.h"
#include "genwrap.h"
#include "named_str_list.h"

named_string_t *
namedStrListInsert(named_string_t ***list, const char *name, const char *value, size_t index)
{
	size_t count;
	named_string_t **newlist;
	bool is_new = false;

	if (*list == NULL) {
		count = 0;
		is_new = true;
	}
	else {
		COUNT_LIST_ITEMS((*list), count);
		if (count == NAMED_STR_LIST_LAST_INDEX)
			return NULL;
	}
	if (index == NAMED_STR_LIST_LAST_INDEX)
		index = count;
	if (index > count)
		index = count;
	newlist = (named_string_t **)realloc(*list, (count + 2) * sizeof(named_string_t*));
	if (newlist == NULL)
		return NULL;
	*list = newlist;
	if (is_new)
		(*list)[1] = NULL;
	else
		memmove(&(*list)[index + 1], &(*list)[index], (count - index + 1) * sizeof(named_string_t*));
	(*list)[index] = malloc(sizeof(named_string_t));
	// TODO: If malloc() failed we truncated the list...
	if ((*list)[index]) {
		(*list)[index]->name = strdup(name);
		(*list)[index]->value = strdup(value);
	}
	return (*list)[index];
}

bool
namedStrListDelete(named_string_t ***list, size_t index)
{
	size_t count;
	named_string_t *old;
	named_string_t **newlist;

	COUNT_LIST_ITEMS(*list, count);
	if (count == 0)
		return false;
	if (index == NAMED_STR_LIST_LAST_INDEX)
		index = count - 1;
	if (index >= count)
		return false;
	newlist = (named_string_t **)realloc(*list, (count + 1) * sizeof(named_string_t*));
	if (newlist != NULL)
		*list = newlist;
	old = (*list)[index];
	memmove(&(*list)[index], &(*list)[index + 1], (count - index) * sizeof(named_string_t*));
	free(old->name);
	free(old->value);
	free(old);

	return true;
}

named_string_t *
namedStrListFindName(named_string_t **list, const char *tmpn)
{
	size_t i;
	for (i = 0; list[i]; i++) {
		if (stricmp(tmpn, list[i]->name) == 0)
			return list[i];
	}
	return NULL;
}

static bool
namedStrListSetField(char **field, const char *value)
{
	char *replacement;

	if (field == NULL || value == NULL)
		return false;
	replacement = strdup(value);
	if (replacement == NULL)
		return false;
	free(*field);
	*field = replacement;
	return true;
}

bool
namedStrListSetName(named_string_t *entry, const char *name)
{
	return entry != NULL && namedStrListSetField(&entry->name, name);
}

bool
namedStrListSetValue(named_string_t *entry, const char *value)
{
	return entry != NULL && namedStrListSetField(&entry->value, value);
}

bool
namedStrListReplace(named_string_t *entry, const char *name, const char *value)
{
	char *new_name;
	char *new_value;

	if (entry == NULL || name == NULL || value == NULL)
		return false;
	new_name = strdup(name);
	if (new_name == NULL)
		return false;
	new_value = strdup(value);
	if (new_value == NULL) {
		free(new_name);
		return false;
	}
	free(entry->name);
	free(entry->value);
	entry->name = new_name;
	entry->value = new_value;
	return true;
}

named_string_t **
namedStrListFree(named_string_t **list)
{
	size_t i;

	if (list == NULL)
		return NULL;
	for (i = 0; list[i] != NULL; ++i) {
		free(list[i]->name);
		free(list[i]->value);
		free(list[i]);
	}
	free(list);
	return NULL;
}
