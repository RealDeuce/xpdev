#ifndef NAMED_STR_LIST_H
#define NAMED_STR_LIST_H

#include "gen_defs.h"
#include "wrapdll.h"

#define NAMED_STR_LIST_LAST_INDEX     (~((size_t)(0)))

#ifdef __cplusplus
extern "C" {
#endif

DLLEXPORT named_string_t *namedStrListInsert(named_string_t ***list, const char *name, const char *value, size_t index);
DLLEXPORT bool namedStrListDelete(named_string_t ***list, size_t index);
DLLEXPORT named_string_t *namedStrListFindName(named_string_t **list, const char *tmpn);
DLLEXPORT bool namedStrListSetName(named_string_t *entry, const char *name);
DLLEXPORT bool namedStrListSetValue(named_string_t *entry, const char *value);
DLLEXPORT bool namedStrListReplace(named_string_t *entry, const char *name, const char *value);
DLLEXPORT named_string_t **namedStrListFree(named_string_t **list);

#ifdef __cplusplus
}
#endif

#endif
