# XPDev error-channel and `errno` audit

This report inventories the XPDev APIs that read, write, propagate, or expose
`errno`, and records the resulting pre-1.0 work. It is primarily concerned with
the Windows shared-library ABI: an application and `xpdev-1.dll` built with
separate static CRTs have separate thread-local `errno` storage. An assignment
to `errno` in the DLL is therefore invisible to a caller that reads its own
`errno`.

See [API-FAMILIES.md](API-FAMILIES.md) for the broader map of XPDev's linkable
libraries and logical interfaces. The families below are grouped specifically
by error behavior, so some broader interfaces are split into multiple rows.

The audit distinguishes three cases:

1. **Contractual use:** failure is reported as `-1`, `NULL`, or `FALSE`, and
   callers are expected to inspect an error channel.
2. **Pass-through use:** XPDev returns a CRT or operating-system failure and
   happens to leave that implementation's error state intact, but the XPDev
   header does not state a precise error contract.
3. **Internal use:** XPDev consumes an error to choose behavior or construct a
   diagnostic; the value is not itself part of the caller-facing result.

Merely calling a function that is permitted to modify `errno` does not make
`errno` part of an API contract. Treating all such calls as contractual would
make nearly every file, allocation, parser, and formatting API part of this
list. The pass-through section identifies the cases in which callers are most
likely to depend on the residual value anyway.

## Executive findings

- The Windows semaphore source API has been made safe for separate CRTs.
  Caller-local inline wrappers set the caller's `errno`; DLL implementation
  entry points transport the error through an output parameter.
- `get_errno()` is not a general solution. Under `/MT` it returns the XPDev
  DLL's CRT state, even when the operation that failed was performed by the
  caller's CRT. Existing Synchronet code uses it after caller-side `fread()`,
  `fwrite()`, and related calls, so its current standalone-DLL meaning is
  misleading.
- Windows file-descriptor and `FILE *` APIs have a more fundamental problem
  than `errno`. CRT descriptors and stream objects belong to the CRT instance
  that created them and cannot safely be operated on by another `/MT` module.
- Windows socket APIs use the Winsock error slot, and Windows comio APIs use
  `GetLastError()`. Those are thread state maintained by Windows rather than
  per-CRT `errno`, so they do not have this particular `/MT` defect.
- Unix event and semaphore emulation use `errno` extensively. Normal dynamic
  builds share the process C library, so this is not analogous to separate
  Windows static CRTs, although the APIs still need documented error semantics.
- The hash and encode component libraries contain no direct `errno` use.

## Error-channel matrix

| API family | Platforms | Current error channel | `/MT` DLL status |
| --- | --- | --- | --- |
| Windows semaphore source API | Windows | Caller-local `errno` | Safe by construction |
| XP semaphore fallback | Unix fallback builds | `errno` | Not a Windows CRT issue |
| Event emulation | Unix | `errno` on `WAIT_FAILED`, `FALSE`, or `NULL` | Not a Windows CRT issue |
| File-descriptor wrappers | Windows and Unix | Primarily `-1` plus CRT `errno` | Unsafe on Windows across separate CRTs |
| Path/metadata wrappers | Windows and Unix | Mixed Boolean/sentinel result and residual `errno` | Explicit DLL-side writes are invisible to `/MT` callers |
| Socket helpers | Windows | Winsock error slot | Safe from CRT separation |
| Socket helpers | Unix | `errno`, sometimes returned directly | Shared-libc assumption; semantics are inconsistent |
| Comio | Windows | `GetLastError()` through `COM_ERROR_VALUE` | Safe from CRT separation |
| Comio | Unix | `errno` through `COM_ERROR_VALUE` | Shared-libc assumption |
| `get_errno()` | All | Returns the library's current `errno` | Misleading for caller-side `/MT` CRT operations |

## Detailed inventory

### Semaphore wrappers

On Windows, `semwrap.h` exposes the source-level operations:

- `sem_init()`
- `sem_post()`
- `sem_destroy()`
- `sem_wait()`
- `sem_trywait()`
- `xp_sem_trywait_block()`

These names resolve to caller-local inline functions. The inline functions call
`xpdev_sem_init_impl()`, `xpdev_sem_post_impl()`,
`xpdev_sem_destroy_impl()`, or `xpdev_sem_trywait_block_impl()` with an error
output pointer, then assign the returned error to the caller's `errno`. The DLL
does not export the standard-looking semaphore names.

The Windows implementation currently reports `EACCES`, `EINVAL`, `ENOMEM`,
`EOVERFLOW`, `ENOSYS`, `EAGAIN`, or `EIO`. Older CRTs that do not define
`EOVERFLOW` or `ENOSYS` receive the documented `ERANGE` or `EINVAL` fallback.

On Unix, `xp_sem_trywait_block()` uses the selected `sem_t` implementation and
translates `ETIMEDOUT` to `EAGAIN`.

When the native POSIX semaphore API is incomplete, `xpsem.c` provides:

- `xp_sem_init()` and `xp_sem_destroy()`
- `xp_sem_open()`, `xp_sem_close()`, and `xp_sem_unlink()`
- `xp_sem_wait()`, `xp_sem_trywait()`, and `xp_sem_timedwait()`
- `xp_sem_post()`

Those functions use `EINVAL`, `EPERM`, `ENOSPC`, `EBUSY`, `ENOSYS`, `EAGAIN`,
and `EOVERFLOW`. They are documented as fallback implementation details, but
they are conditionally installed and exported today; their actual ABI status
must be reconciled before 1.0.

### Event emulation

The Unix implementations of the following Win32-shaped APIs use `errno`:

- `CreateEvent()`
- `SetEvent()`
- `ResetEvent()`
- `CloseEvent()`
- `WaitForEvent()`

They assign pthread return codes directly to `errno`, and also report
`EINVAL`, `EBUSY`, and `EOVERFLOW`. A timeout is represented by `WAIT_TIMEOUT`,
not by an `errno` value. Windows consumers use the operating system's native
event API and `GetLastError()` instead.

### File-descriptor and stream wrappers

The following wrappers expose or preserve errors from `fcntl()`, `flock()`,
`_locking()`, `LockFileEx()`, `_get_osfhandle()`, `fstat()`, `open()`, and
related calls:

- `xp_lockfile()`, `lock()`, `rdlock()`, and `unlock()`
- `filetime()` and, on Unix, `filelength()`
- Unix `sopen()` and `_fsopen()`

Unix `sopen()` normalizes `EWOULDBLOCK` to `EAGAIN`; Unix `_fsopen()` assigns
`EINVAL` for invalid mode strings. The Windows `rdlock()` and
`xp_lockfile()` paths assign `EACCES` when `LockFileEx()` fails, while other
Windows failure paths return `-1` without consistently translating
`GetLastError()` to `errno`.

For Windows `/MT`, caller-local `errno` wrappers alone would not make the
descriptor APIs safe. A descriptor created by the application's CRT is not a
valid input to the DLL's `_get_osfhandle()` or `fstat()` descriptor table. The
same ownership restriction applies more strongly to `FILE *`. Relevant
families include:

- INI APIs accepting or returning `FILE *`
- data-file APIs accepting or returning `FILE *`
- string-list file APIs
- `iniOpenFile()` and `iniCloseFile()`
- any formatter or helper that accepts a stream owned by another module

These APIs require either a shared-CRT rule, caller-local implementation,
Win32-handle/path alternatives, or an explicit XPDev-owned opaque object.

### Directory and path wrappers

The MSVC replacement `opendir()` explicitly assigns `ENOMEM` or `ENOENT`.
`getfattr()` explicitly assigns `ENOENT` on every lookup failure. In an `/MT`
DLL these assignments affect the DLL's CRT, not the caller's.

Several other APIs return the result of CRT filesystem operations and leave
their `errno` behind without documenting whether callers may rely on it:

- `fcdate()`, `fdate()`, `setfdate()`, and `flength()`
- `fexist()`, `isdir()`, `getfmode()`, and `mkpath()`
- `_fullpath()` on Unix
- `CopyFile()` and `delfiles()` on Unix

Boolean existence predicates should not acquire an accidental promise that a
particular `errno` survives. Sentinel-returning metadata and construction APIs
need either a defined error contract or an explicit statement that `errno` is
unspecified.

### Socket helpers

`sockwrap.h` intentionally abstracts the socket error slot:

- On Windows, `SOCKET_ERRNO` and `socket_errno()` use `WSAGetLastError()`;
  `set_socket_errno()` uses `WSASetLastError()`.
- On Unix, they read and write `errno`.

The affected helpers include:

- `socket_check()` and `socket_recvdone()`
- `xp_accept()` and `retry_bind()`
- `nonblocking_connect()`
- `xp_inet_pton()` and `set_socket_errno()`

`nonblocking_connect()` returns the socket error number directly rather than
promising that the error remains in `errno`. `retry_bind()` consumes the
current error mainly for logging. `socket_strerror()` takes an explicit error
number and preserves the Windows Winsock error slot while formatting it.

The Windows side is not affected by CRT-local `errno`, but the family still
needs one documented convention: some functions return an error number, some
return a Boolean or sentinel and leave the platform slot, and some consume the
slot internally.

### Comio

`comio.h` exposes the caller-side `COM_ERROR_VALUE` macro:

- Windows expands it to `GetLastError()`.
- Unix expands it to `errno`.

Most failed `comOpen()`, configuration, modem-control, read/write, purge, and
drain operations leave their underlying platform error available through that
macro. `comReadBuf()` and `comReadLine()` intentionally combine polling,
timeout, and read failure into a returned byte count, so their residual error
state is not a reliable failure contract.

Because Windows comio uses Win32 handles and `GetLastError()`, it does not have
the separate-CRT `errno` or CRT-descriptor problems described above.

### `get_errno()`

`get_errno()` is exported from the core library and implemented as:

```c
int get_errno(void)
{
    return errno;
}
```

Its old comment describes it as a thunk to the correct multithreaded C library
implementation. That was meaningful when XPDev and its consumer were built
into one program using one CRT. It is ambiguous in a standalone shared
library.

With `/MT`:

- after an XPDev DLL operation, `get_errno()` observes the DLL's CRT state;
- after a caller-side CRT operation, it observes unrelated DLL state;
- reading the caller's `errno` has the inverse limitation for errors assigned
  in the DLL.

Synchronet callers use `get_errno()` after their own `fread()`, `fwrite()`,
and file-locking operations. Preserving that intended source behavior requires
`get_errno()` to resolve in the caller, most naturally as a macro or static
inline function, rather than as a DLL call. Any DLL-error query should have a
different, explicit name and a specified lifetime.

### Internal consumers

These functions use an error internally without establishing a general error
channel for their callers:

- `listFree()` checks whether semaphore destruction failed with `EBUSY`.
- `iniReadFiles()` embeds an include-file `fopen()` error in a generated
  comment.
- `os_version()` embeds an `uname()` failure value in its result string.
- multisocket setup reads `errno` while logging close-on-exec and bind errors.

The list semaphore helpers also call errno-bearing semaphore operations and
reduce the result to a Boolean. Unless documented otherwise, their residual
`errno` should be treated as incidental.

## Required decisions and release gates

### P0 decisions

1. **Define the Windows CRT support contract.** Current CI deliberately tests
   a shared XPDev DLL with both `/MD` and `/MT` consumers, so the implemented
   direction is to support separate CRTs. If that remains the release policy,
   every public API must honor it; isolated fixes are insufficient.
2. **Make `get_errno()` caller-local or retire it.** Test it after a failing
   caller-side CRT operation under `/MT`. Do not replace it with another DLL
   accessor bearing the same generic meaning.
3. **Disposition every Windows `FILE *` and CRT-descriptor API.** Merely moving
   `errno` assignment to a header does not solve object ownership. Unsupported
   combinations must fail at compile/configure time or be clearly excluded
   from the public DLL contract.
4. **Define the Windows file/path error contract.** For APIs that remain in the
   DLL, choose explicit returned errors, caller-local wrappers with transported
   errors, or a documented Win32 error channel. Do not silently mix
   `GetLastError()` and DLL-local `errno`.
5. **Reconcile conditional fallback exports.** Decide whether `xp_sem_*` is a
   public macOS ABI or a private implementation behind `semwrap.h`, then make
   headers, export manifests, symbol versions, and documentation agree.

### Required tests

- Run error-contract consumers against both `/MD` and `/MT` XPDev DLLs.
- Verify `get_errno()` after caller-side and DLL-side failures.
- Test semaphore invalid arguments, timeout, overflow, and unsupported
  process-sharing errors in the caller's CRT.
- Test every retained file/path wrapper with invalid handles, descriptors,
  paths, contention, and permission failures.
- Add exact Windows export checks so caller-local compatibility names cannot
  reappear as DLL exports.
- Test socket and comio error preservation independently from CRT `errno`.

### Documentation rule

Every public operation that can fail should document exactly one of:

- an error number returned directly;
- `errno` in the calling module;
- the Winsock error slot;
- `GetLastError()`;
- an explicit XPDev error object/query; or
- no inspectable error detail beyond its primary result.

Residual `errno` from an undocumented implementation call is not an API.
