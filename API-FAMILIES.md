# XPDev API families

XPDev is a collection of C APIs rather than one object model. An API family is
a group of related types and operations with a common contract. The family is
usually represented by one public header, but it is not necessarily a separate
library or a separate allocator domain.

This document is a map of the installed interfaces. The public headers remain
the function-level reference. See [HEAP-OWNERSHIP.md](HEAP-OWNERSHIP.md) for
allocation and transfer rules and [ERRNO-API-AUDIT.md](ERRNO-API-AUDIT.md) for
the current error-channel audit.

## Linkable library families

An installation contains four independently linkable libraries:

| Library family | CMake target | pkg-config module | Interfaces |
| --- | --- | --- | --- |
| Core | `xpdev::xpdev` | `xpdev` | Portability wrappers and all core families listed below |
| Serial communications | `xpdev::comio` | `xpdev-comio` | `comio/comio.h` |
| Hashes and checksums | `xpdev::hash` | `xpdev-hash` | `hash/crc16.h`, `hash/crc32.h`, `hash/fnv1a.h`, `hash/md5.h`, `hash/sha1.h`, and `hash/sha256.h` |
| Encodings | `xpdev::encode` | `xpdev-encode` | `encode/base64.h`, `encode/hex.h`, `encode/lzh.h`, and `encode/utf8.h` |

Each target above selects the shared library. Append `_static` or `_shared` to
the component target name to select a variant explicitly; the core variants are
`xpdev::static` and `xpdev::shared`. The regular pkg-config module selects the
shared library, while its `-static`-suffixed counterpart explicitly selects the
static archive.

These are linkage and ABI boundaries. They do not imply that all operations in
the core library form one ownership family. For example, an audio buffer must
be released by the audio family, while a string-list item must be released by
the string-list family, even though both implementations are in the core
library.

## Core API families

| API family | Public headers | Purpose and boundary |
| --- | --- | --- |
| Fundamental definitions | `gen_defs.h`, `wrapdll.h`, `xpendian.h` | Portable types, feature and export declarations, byte-order helpers, and common macros used by the other families. These headers are infrastructure rather than one callable runtime API. |
| Character-set definitions | `cp437defs.h`, `petdefs.h`, `unicode_defs.h` | Constants and types for CP437, PETSCII, and Unicode code points. |
| General portability helpers | `genwrap.h`, `strwrap.h` | Small string, number, formatting, timer, process, and missing-CRT compatibility operations that do not fit a stateful object family. |
| String lists | `str_list.h` | Null-terminated vectors of strings, including mutation, sorting, splitting, joining, block conversion, and stream I/O. Lists, strings annexed into lists, and string-list blocks have distinct transfer rules. |
| Named string lists | `named_str_list.h` | Null-terminated vectors of name/value entries. Entry fields remain owned by this family and are changed through its setters. |
| INI data | `ini_file.h` | Reading, querying, updating, parsing, and writing INI content represented as streams, string lists, or parsed section structures. Use the matching `iniFree*` operation for owned results. |
| Delimited data | `dat_file.h` | CSV- and tab-delimited record parsing, creation, and stream I/O through parser and creator callbacks. Callback results and record arrays follow the data-line family contract. |
| Linked lists | `link_list.h` | Optionally synchronized node lists with borrowed or family-owned payloads, tags, node traversal, extraction, and merging. |
| Message queues | `msg_queue.h` | In-process synchronized queues built on linked lists. Writes copy messages; read and find operations transfer owned messages, while peek operations borrow them. |
| Growable buffers | `stbuf.h` | Sized byte buffers using either the XPDev allocator or allocator callbacks captured from caller-supplied storage. |
| Allocating formatters | `xpprintf.h` | `xp_asprintf` formatting and incremental start/next/end formatting. Results form their own ownership family. |
| Dates and times | `datewrap.h`, `xpdatetime.h` | Portable legacy date/time wrappers plus typed date, time, time-zone, ISO parsing, conversion, and formatting operations. |
| Paths and files | `dirwrap.h`, `filewrap.h` | Directory, path, metadata, file-length, descriptor-locking, and missing-platform compatibility operations. |
| File mappings | `xpmap.h` | File-backed mapping objects created with `xpmap()` and destroyed with `xpunmap()`. |
| File semaphores | `semfile.h` | Files used as process signals, including management of lists of semaphore-file paths. |
| Threads and protected integers | `threadwrap.h` | Thread creation and naming helpers, the selected pthread compatibility surface, one-time initialization, and atomic/protected integer operations. |
| Locks, semaphores, and events | `rwlockwrap.h`, `semwrap.h`, `eventwrap.h` | Cross-platform synchronization objects. The public source API may resolve to a native platform API, an inline adapter, or an XPDev implementation according to the generated configuration. |
| Console | `conwrap.h` | Local terminal setup, echo, keyboard polling, and character input wrappers. |
| Networking and sockets | `netwrap.h`, `sockwrap.h`, `multisock.h`, `haproxy.h` | Host/address utilities, portable socket operations and errors, multi-listener sets, and HAProxy protocol constants. |
| Dynamic loading | `xp_dl.h` | Portable library open, symbol lookup, and close operations. |
| Operating-system information | `os_info.h` | Caller-buffer queries for operating-system version, CPU architecture, and command shell. |
| Audio and tones | `xpbeep.h` | Tone generation, sample conversion and playback, audio handles and queues, and their audio-buffer ownership family. Availability and backends are reported by the generated configuration. |
| Unicode conversion | `unicode.h` | Unicode width and classification plus conversion to legacy single-byte character sets. UTF-8 operations are in the separate encode library. |

`xpevent.h` and `xpsem.h` can also be installed when XPDev supplies the
corresponding Unix fallback. They describe that selected implementation and
are not additional portable families. Consumers should include `eventwrap.h`
and `semwrap.h`, which select the configured native or fallback implementation.

## Component API families

The optional libraries preserve their upstream C interfaces and generic symbol
names. Link only the components a program uses; the component split is an ABI
boundary, not a C namespace.

`hash/hash_export.h` and `encode/encode_export.h` provide the component export
and import declarations used by their public headers. Like `wrapdll.h` in the
core library, they are linkage infrastructure rather than callable API
families.

### Serial communications

`comio/comio.h` defines one handle-based family for opening and configuring
serial devices, modem-control signals, and buffered or byte-oriented I/O. On
Windows its platform error channel is `GetLastError()` through
`COM_ERROR_VALUE`; on Unix that macro reads `errno`.

### Hashes and checksums

The hash library contains independent CRC-16, CRC-32, FNV-1a, MD5, SHA-1, and
SHA-256 families. Incremental hash APIs keep state in caller-provided context
objects, and convenience functions operate on caller-provided input and output
buffers. `fcrc32()` is the exception that consumes a caller-provided `FILE *`.

### Encodings

The encode library contains base64, escaped hexadecimal, LZH, and UTF-8
families. Encoding and compression calls use caller-provided buffers or mutate
a caller-provided string; they do not return separately allocated output.

## Compatibility surfaces

Several families deliberately expose names shaped like an existing platform
API. Depending on the platform and configured capabilities, a name can refer
to the native function, a header-local adapter, or an XPDev fallback. Examples
include pthread and semaphore operations on Windows, event operations on Unix,
and missing CRT string, directory, and file functions.

Include the installed XPDev wrapper header and use the generated
`xpdev_config.h` selected by it. Do not include a conditional implementation
header directly, assume that every source-level name is an exported symbol, or
override an ABI-affecting feature macro in consumer code.

## Objects, memory, and errors do not cross by analogy

Similar return types do not make two APIs interchangeable ownership families.
A `char *` may be a borrowed pointer, a caller buffer, a normal libc
allocation, a string-list allocation, or an allocating-formatter result. Use
only the creator's documented transfer or release operation.

The same principle applies to stateful objects and errors:

- Create, operate on, and destroy opaque or platform-owned objects through the
  same family.
- A caller-owned `FILE *` or CRT file descriptor is not an XPDev-owned object;
  separate Windows CRT instances require additional care documented in the
  error-channel audit.
- Do not infer `errno` from a `NULL`, Boolean, or sentinel result unless that
  family's contract identifies it as the error channel. Socket and comio
  families deliberately use platform-specific error slots.
