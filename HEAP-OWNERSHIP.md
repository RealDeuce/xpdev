# XPDev heap ownership

XPDev APIs use family-specific ownership rather than a universal allocator.
When a shared library may resize or release memory, that memory must originate
from the same API family. When a shared library returns owned memory, callers
must use that family's release function.

This rule is observable on Windows when a DLL and its caller use separate
static CRT instances. It also applies when XPDev objects are linked into and
re-exported from a larger DLL. A direct static-library consumer whose objects
all use a compatible CRT may retain the documented legacy uses of ordinary
`malloc()`.

## Ownership matrix

| API or result | Ownership contract |
| --- | --- |
| `realloc_or_free` | Header-local operation using the caller's `realloc` and `free`; it is not a shared-library symbol. |
| fallback `asprintf`, `vasprintf`, `getdelim`, and `strndup` | Header-local compatibility functions. Their results remain ordinary caller allocations and use ordinary `free`. |
| `xp_asprintf`, `xp_vasprintf`, and the start/next/end formatter | XPDev-owned; release with `xp_asprintf_free`. The next/end operations may resize or consume earlier family results. |
| `xp_audio_buffer_alloc` and `xp_u8mono22k_to_s16stereo44k` | Audio-family buffers. Release with `xp_audio_buffer_free` or transfer to `xp_audio_append`. Append consumes the buffer on every return path. `xp_audio_play` only borrows and copies its input. |
| `strListInit` and functions returning `str_list_t` | String-list family. Release the complete list with `strListFree` or the relevant higher-level family destructor. |
| `strListAnnex` | Transfers the string pointer without copying. If XPDev may delete, replace, or free it through a DLL, allocate it with `strListAllocString`. A borrowed pointer is valid only when removed without XPDev freeing or resizing it. |
| `strListRemove`/`strListPop` | Transfers an owned item to the caller. Use `strListFreeString` when the item was created by the string-list family. |
| `strListMerge` | Transfers the source strings to the destination but not the source pointer vector. Retire that source vector with `strListFreeContainer`; its pointer entries must no longer be used because the destination owns the strings. |
| string-list block functions and `strListCombine(NULL, ...)` | Block family. Create/copy, append, and release only through `strListCreateBlock`, `strListCopyBlock`, `strListAppendBlock`, and `strListFreeBlock`. |
| `listAddNodeData` and `listAddNodeString` payloads | Linked-list data family. A removed payload is released with `listFreeData`. |
| `listAddNodeWithFlags` with `LINK_LIST_MALLOC`, or a list using `LINK_LIST_ALWAYS_FREE` | XPDev may release the payload. It must originate from `listAllocData` across a shared-library boundary, and on Windows even for static links because this family uses the Win32 process heap. Unowned nodes continue to carry borrowed application pointers. |
| `msgQueueRead` and `msgQueueFind` | Returned message becomes caller-owned; release it with `msgQueueFreeMessage`. `msgQueuePeek` remains borrowed. `msgQueueWrite` copies its input. |
| `parseEnumList`, `iniReadEnumList`, and `iniGetEnumList` | Release with `iniFreeEnumList`. |
| `parseIntList`, `iniReadIntList`, and `iniGetIntList` | Release with `iniFreeIntList`. |
| `dataLineCreator_t`, `csvLineCreator`, and `tabLineCreator` | Creator results belong to the data-line family. Allocate custom callback results with `dataLineAlloc`; direct recipients use `dataLineFree`. `dataCreateList` consumes callback results. |
| `dataLineParser_t` | A custom parser returns a string-list-family list. `dataListFree` eventually destroys it. |
| `dataParseList` and `dataReadFile` | Release the returned record array and its parsed lists with `dataListFree`. |
| `namedStrListInsert` and INI named-string-list readers | Entries and their fields are owned by the named-list family. Use `namedStrListSetName`, `namedStrListSetValue`, or atomic `namedStrListReplace`; release with `namedStrListFree` or `iniFreeNamedStringList`. Field pointers are readable but not independently owned. |
| `stbuf_malloc` and `stbuf_zalloc` | Release with `stbuf_free`; resizing stays on the XPDev allocator. |
| `stbuf_frommem(..., true)` | The header-local constructor captures the caller's `realloc` and `free` functions. Later stbuf operations use those callbacks, including when invoked through a DLL. |
| `stbuf_frommem(..., false)` | Caller retains the storage. XPDev never resizes or frees it. |
| `getNameServerList`, semfile list functions, and INI string/parsed/fast-list functions | Already have family destructors (`freeNameServerList`, `semfile_list_free`, and the corresponding `iniFree*` function). |
| `glob`, `opendir`, maps, events, rwlocks, semaphores, message-queue objects, and multisocket sets | Already paired with `globfree`, `closedir`, `xpunmap`, close/destroy/free, or `xpms_destroy`. Intermediate pointers returned by accessors are borrowed. |
| `_fullpath(NULL, ...)` | The allocating fallback is Unix-only. It uses the process's normal libc allocator and remains compatible with ordinary `free`. |

Pointers returned into caller-provided buffers, static tables, object fields, or
input strings are borrowed unless the API is listed above as returning owned
memory.

## Synchronet migration checklist

Synchronet is not expected to dynamically link XPDev. The migration is still
needed where XPDev implementation objects are linked into `sbbs.dll` and an
allocation is re-exported or otherwise reaches a different module.

- Change `syncterm/audio_apc.c` and `conio/cterm_cterm.c` producers whose
  buffers are consumed by `xp_audio_append` to use the audio buffer family.
- Change numeric-list releases in `sbbs3/websrvr.cpp`, `sbbs3/rechocfg.c`, and
  `syncterm/bbslist.c` to `iniFreeEnumList` or `iniFreeIntList`.
- Change owned queue results in `sbbs3/js_queue.cpp` and `sbbs3/js_mqtt.cpp`
  to `msgQueueFreeMessage`.
- Audit every `strListRemove`, `strListPop`, `listPopNode`, and `listShiftNode`
  result by its origin. Use `strListFreeString` or `listFreeData` only for
  family-owned results; borrowed application payloads remain application-owned.
- Replace the raw source-vector frees following `strListMerge` in
  `sbbs3/scfgsave.c`, `sbbs3/un_qwk.cpp`, and `sbbs3/un_rep.cpp` with
  `strListFreeContainer`.
- Replace raw frees of `xp_asprintf`/`xp_vasprintf` results, including
  `sftp/sftp_str.c`, with `xp_asprintf_free`.
- Replace direct frees and assignments of named-list fields in
  `syncterm/bbslist.c` with the named-list setters or replacement function.
- Preserve temporary borrowed Annex uses that remove the pointer before any
  XPDev delete, replacement, or list destruction operation.
- When an SBBS DLL wrapper exposes an XPDev-owned result, expose or invoke the
  matching family release operation through the same module.

## Separate CRT-object audit

`FILE *` is also module-sensitive with separate Windows CRT instances, but it
is not heap ownership. XPDev functions accepting caller-created `FILE *`
require a separate review. Existing open/close pairs such as
`dataOpenFile`/`dataCloseFile` and `iniOpenFile`/`iniCloseFile` already provide
a safe same-module path.
