# XPDev 1.0 release checklist

This is the working list of issues to investigate, decisions to make, and
release gates to satisfy before publishing XPDev 1.0. It is intentionally
broader than a bug list: 1.0 freezes library names, public symbol names,
ownership rules, much of the C ABI, and the compatibility policy for future
1.x releases.

Priority labels have the following meanings:

- **P0** must be resolved before 1.0.
- **P1** requires an explicit decision before 1.0. The decision may be to
  document and defer the work.
- **P2** is desirable validation or polish and need not block the release if
  its omission is recorded.

An item is not complete merely because it works on one build. ABI items are
complete only after the contract is documented and guarded by a test or an
export/baseline check.

## Current hard blockers

- [x] **P0: Restore a green build on `main`.** Commit `f45ce83c0` passes normal
  MSVC, static-CRT (`/MT`) MSVC, both MinGW thread models, macOS, and ELF. The
  Windows semaphore wrappers now transfer error values out of the DLL and set
  `errno` in caller-local inline code.
- [ ] **P0: Decide how Windows DLL APIs report `errno`.** A DLL and application
  built with separate static CRTs have different thread-local `errno` storage.
  Assigning `errno` inside `xpdev-1.dll` therefore cannot satisfy a caller that
  reads its own `errno`, even though the function correctly returns `-1`.
  `semwrap.c` is the demonstrated failure, but this must be audited across all
  exported functions that set or promise `errno`, including directory, file,
  socket, event, and compatibility wrappers.
  - If `/MT` DLL consumers are supported, use an interface that transfers the
    error explicitly and sets `errno` in caller-local code, or expose an
    XPDev-specific error query. Calling `_set_errno()` in the DLL is not a fix;
    it still addresses the DLL's CRT.
  - If they are not supported, remove the misleading `/MT` promise, document
    the shared-CRT requirement, and retain a CI test that rejects unsupported
    configurations cleanly.
  - The current inventory, platform error-channel matrix, and concrete release
    gates are in [`ERRNO-API-AUDIT.md`](ERRNO-API-AUDIT.md). In particular,
    `get_errno()` is misleading after caller-side CRT failures under `/MT`, and
    Windows CRT descriptors and `FILE *` objects cannot be repaired merely by
    transporting an error number.
- [ ] **P0: Make installed headers carry their ABI configuration without
  depending on CMake target flags.** `xpdev::shared` and `xpdev::static`
  propagate `XPDEV_USE_CONFIG_H` and the ABI-affecting definitions. A consumer
  invoking a compiler and linker directly can instead take the legacy
  `gen_defs.h` path and see different linked-list, semaphore, thread, or atomic
  types. Decide whether non-CMake consumption is supported. If it is, installed
  headers must reliably include the installed `xpdev_config.h` without making
  Synchronet's in-tree non-CMake builds require a generated file.

## ABI and public type freeze

- [ ] **P0: Inventory every exported function and public aggregate type.** Save
  a reviewed 1.0 manifest for the core and each component. For public structs,
  record `sizeof`, alignment, and relevant field offsets for every supported
  platform/toolchain configuration.
- [ ] **P0: Resolve the `protected_int*` ABI.** `threadwrap.h` selects among
  C11 `_Atomic`, C++ `std::atomic`, MSVC support, and mutex-backed structs.
  These representations are not promised to be layout-compatible, yet the
  types are accepted by exported functions and `protected_int32_t` is embedded
  in public `msg_queue_t`. Prefer opaque storage or library-owned objects over
  exposing a compiler-selected atomic representation.
- [ ] **P0: Decide whether `link_list_t`, `list_node_t`, and `msg_queue_t`
  remain public layouts.** They expose implementation fields and, in threaded
  builds, platform-selected `pthread_mutex_t` and `sem_t` representations.
  MSVC, MinGW with POSIX threads, and MinGW with Win32 wrappers can consequently
  produce incompatible layouts under the same DLL name. Make them opaque, or
  explicitly define and enforce the supported binary combinations.
- [ ] **P0: Remove or constrain the `list_node_tag_t` override.** Defining the
  `list_node_tag_t` preprocessor macro changes public structure layout and
  exported function signatures. A shared-library consumer must not be able to
  silently select a different ABI. A fixed-width type plus explicitly named
  conversion/access APIs would make the contract testable.
- [ ] **P1: Freeze or hide the `stbuf` layout.** `STBUF_SIZE()`,
  `STBUF_OFFSET`, flexible-array storage, and allocator callback fields are all
  public ABI. The callbacks solve cross-CRT heap ownership, but adding or
  reordering fields after 1.0 would break caller-created buffers. Decide
  whether this layout is deliberately permanent or whether an opaque/new
  constructor design is required before release.
- [ ] **P1: Choose the extension policy for `xp_audio_opts_t`.** It is passed
  from caller to library without a size or version field. If the current
  structure is frozen for 1.0, future options should use a new entry point or a
  separately versioned options object rather than extending the structure and
  reading beyond an old caller's allocation.
- [ ] **P1: Review other public by-value and caller-allocated structures.** At
  minimum include `ini_style_t`, `named_string_t`, date/time structures,
  socket structures, and the MD5/SHA context structures. Decide which are
  intentionally stable value types and which should become opaque.
- [ ] **P1: Document compiler ABI requirements for variadic and `va_list`
  APIs.** `xp_vasprintf()` and the formatter family cross the library boundary
  using compiler ABI types. Confirm supported C/C++ compiler combinations on
  each platform or replace the cross-boundary portion with a safer contract.
- [ ] **P1: Review Windows calling conventions and type widths.** Public
  callbacks and functions currently rely mostly on the compiler default
  calling convention. Confirm x86 as well as x64 behavior, and eliminate
  public uses of platform-width types where a fixed-width XPDev type is the
  actual contract.

## Export and symbol-version contract

- [ ] **P0: Create exact export manifests for the core library on ELF, macOS,
  and Windows.** Component libraries already have exact platform export lists,
  while core checks are primarily a version check plus forbidden-name lists.
  A new accidental `DLLEXPORT` should fail CI even when its name was not
  anticipated by a blacklist.
- [ ] **P0: Reconcile the manifests with public headers.** Every intended
  shared-library API must be exported everywhere it is supported; private,
  fallback, and platform-native symbols must be absent. Conditional APIs must
  be listed with their precise platform rules.
- [x] **P0: Extend the MinGW Win32-thread export test to cover all ownership
  APIs and caller-local forbidden symbols.** Both MinGW thread-model jobs now
  verify the ownership API and Windows semaphore implementation exports while
  rejecting caller-local and legacy semaphore names.
- [ ] **P0: Capture an ABI baseline from the release candidate.** Use an ABI
  comparison tool where practical and archive the export/type reports so 1.1
  can be checked against the actual 1.0 binaries rather than reconstructed
  expectations.
- [ ] **P1: Verify symbol-version evolution with a synthetic 1.1 node.** An
  added ELF API should be assigned to a new component-specific 1.1 node that
  inherits from 1.0; existing symbols must remain at their 1.0 versions.
- [ ] **P1: Confirm the library version policy.** ABI-compatible 1.x releases
  retain SONAME 1 and may advance the full/current version. ABI breaks require
  SONAME/DLL-major 2. Symbol versions describe individual ABI introductions;
  they do not require a SONAME bump and do not solve generic base-name
  collisions.

## Ownership and CRT-boundary review

- [ ] **P0: Complete the Synchronet migration in `HEAP-OWNERSHIP.md` before
  importing the ownership APIs back from upstream.** In particular, update
  audio producers, numeric-list frees, queue-message frees, `strListMerge`
  source-vector frees, formatter frees, and named-list field replacements.
- [ ] **P0: Audit every exported owned pointer against the ownership matrix.**
  Each result needs an unambiguous same-family release operation, and every
  consuming function must state whether ownership transfers on failure as well
  as success.
- [ ] **P0: Audit every callback that transfers an allocation.** Confirm that
  the producer can allocate in the family used by the receiving library and
  that failure paths do not mix allocators.
- [ ] **P0: Update `mixertest.c` for the audio ownership contract.** Its helper
  still allocates buffers transferred to `xp_audio_append()` with raw
  `malloc()` instead of `xp_audio_buffer_alloc()`.
- [ ] **P0: Resolve `FILE *` crossing Windows CRT boundaries.** INI, data-file,
  string-list, and file wrappers accept or return CRT stream objects. Passing a
  `FILE *` between separate CRT instances is not safe merely because heap
  allocation has been fixed. Options include requiring a shared CRT, adding
  path/descriptor/handle-based APIs, or keeping stream operations caller-local.
- [ ] **P0: Resolve CRT file descriptors crossing Windows CRT boundaries.**
  `filetime()`, `xp_lockfile()`, `lock()`, `rdlock()`, and `unlock()` may
  operate in the DLL on a descriptor created in an `/MT` caller. The DLL's
  `fstat()` and `_get_osfhandle()` use a different descriptor table. Follow
  the disposition and tests in [`ERRNO-API-AUDIT.md`](ERRNO-API-AUDIT.md).
- [ ] **P1: Check other CRT-owned objects and state.** Include locale state,
  environment pointers, `DIR`/glob compatibility objects, `va_list`, and any
  object that is created by one module and operated on by another.
- [ ] **P1: Add allocation-failure tests.** Exercise partial construction and
  transfer failures in string lists, linked lists, named-string lists, data
  callbacks, audio queues, and formatter operations with a controllable failing
  allocator or equivalent hooks.

## API correctness and semantics

- [ ] **P0: Fix or explicitly disposition current compiler warnings.** The
  local MinGW Win32-thread build reports possible returns of local buffers from
  `iniReadSString()`/`iniGetSString()`, 64-bit pointer-to-`long` truncation in
  `xp_asprintf_next()`, and potential over-read diagnostics in MD5/SHA code.
  Determine which are real defects and suppress only demonstrated false
  positives.
- [ ] **P0: Fix `namedStrListInsert()` allocation-failure corruption.** The
  source already notes that failure after growing/moving the vector can
  truncate or corrupt the list. Verify name/value duplication failures too.
- [ ] **P0: Define `strListMerge()` failure semantics.** It can partially move
  strings if destination-vector growth fails, while returning only a count and
  leaving ownership difficult to determine. Make partial transfer explicit or
  make the operation atomic.
- [ ] **P0: Audit integer overflow in allocation sizing.** Cover list-vector
  growth, `count * sizeof(...)`, string/block concatenation, audio frame sizes,
  and parser/formatter growth.
- [ ] **P1: Normalize error conventions.** Some wrapper families return
  booleans, some return `-1` plus `errno`, some return pthread-style error
  numbers, and some return Win32 errors. Document each family and remove cases
  where nominally compatible functions have observably different conventions.
- [ ] **P1: Decide the supported status of Borland, OS/2, and other legacy
  branches.** Borland lacks `EOVERFLOW` and `ENOSYS`; `semwrap.c` now has
  private fallbacks. OS/2 semaphore wrappers are explicitly untested. Either
  add a credible compile test and support policy or state that these branches
  are retained upstream compatibility code outside the standalone 1.0 support
  matrix.
- [ ] **P1: Set a truthful minimum Windows version.** CMake currently publishes
  Windows XP-era `_WIN32_WINNT`, `WINVER`, and `MSVCRT_VERSION` values while the
  native audio path uses WASAPI, which is newer. Select the actual minimum and
  test it.
- [ ] **P1: Review thread, event, rwlock, and semaphore destruction under
  contention.** Add stress tests for wait/wake behavior, invalid unlocks,
  destruction, timeouts, spurious wakeups, and handle/resource leaks.
- [ ] **P1: Decide whether standard-looking compatibility names belong in the
  public DLL ABI.** Keep native platform implementations unshadowed and prefer
  `xp_` names for XPDev-specific semantics. Preserve the established rule that
  standard `snprintf()`/`vsnprintf()` are never macro-rerouted.

## Build, install, and package behavior

- [x] **P0: Test the installed package without CMake targets.** CI compiles and
  runs C and C++ consumers directly against installed headers and all four
  static/shared libraries. Linux, macOS, MSVC with both runtime-library modes,
  and MinGW with both POSIX and Win32 thread models exercise this independently
  of the CMake imported targets.
- [ ] **P0: Test a no-audio installation and fallback consumer.** Verify that
  `find_package(xpdev QUIET COMPONENTS audio)` fails before importing targets,
  then demonstrate that a `FetchContent` fallback can create the normal XPDev
  target names without collision.
- [ ] **P0: Verify static dependency closure.** A consumer of every installed
  static target should receive all required system libraries and compile
  definitions through CMake metadata on each platform.
- [ ] **P0: Verify relocatable installs.** Test a staged `DESTDIR`, a non-default
  prefix, a non-default `CMAKE_INSTALL_LIBDIR`, moving the install tree, and
  both single- and multi-config generators.
- [x] **P1: Provide and test `pkg-config` metadata.** Installations provide
  shared and explicit `-static` modules for core, comio, encode, and hash, with
  complete static-link dependencies and relocatable paths. Windows modules
  distinguish its separately named static archives and DLL import libraries
  and their different consumer definitions. CI validates the metadata and
  exercises native, MSVC, and MinGW consumers.
- [ ] **P1: Clean harmless CMake/workflow duplication.** There are duplicated
  Haiku link-library calls, duplicated `static_target` assignment, and a
  duplicated macOS consumer build command. Add linting so these do not
  accumulate through upstream merges.
- [ ] **P1: Confirm static/shared naming on every platform.** Check development
  symlinks, import libraries, Windows `_static` names, component names, debug
  configurations, and coexistence of all variants in one prefix.
- [ ] **P1: Install or package the useful project documentation.** At minimum
  consider the README, ownership contract, release notes, and license alongside
  the CMake package.

## Platform and configuration coverage

- [ ] **P0: Keep the present release matrix green:** Linux/ELF with all normal
  audio backends, macOS/CoreAudio plus Homebrew PortAudio, MSVC `/MD`, MSVC
  `/MT` if supported, MinGW UCRT with POSIX threads, and MinGW with Win32 thread
  wrappers.
- [ ] **P1: Add GCC and Clang Linux builds**, including warnings-as-errors for
  project code and ASan/UBSan installed-consumer runs.
- [ ] **P1: Add a native FreeBSD build.** Also decide whether OpenBSD and
  NetBSD are release-supported or best-effort; both have distinct OSS/linking
  branches in CMake.
- [ ] **P1: Add a Haiku build with SDL audio.** This is the configuration that
  validates SDL as the backend of last resort rather than as a competing
  backend on mainstream platforms.
- [ ] **P1: Consider Cygwin coverage.** Confirm that its normal pthread and
  POSIX semaphore types are selected without leaking XPDev's Win32 wrappers.
- [ ] **P1: Add at least one 32-bit build.** It should exercise pointer/`long`,
  `size_t`, structure-layout, calling-convention, and DLL export assumptions.
- [ ] **P2: Add an ARM and/or big-endian compile/test configuration** for
  endian helpers, hash implementations, serialization, and alignment.
- [ ] **P2: Evaluate MacPorts separately from Homebrew.** Add it only if it
  exposes materially different dependency discovery, prefixes, or link
  behavior; do not duplicate the same compiler/backend coverage without a
  reason.
- [ ] **P1: Test meaningful audio configurations.** Cover each direct backend,
  multiple simultaneous Linux backends, no backend, and SDL-only fallback.
  Hardware-independent CI should at least validate load/open failure behavior,
  conversion, mixing, queueing, and cleanup.

## Test depth and quality gates

- [ ] **P0: Wire the repository's standalone test programs into CTest or retire
  them.** `wraptest.c`, `mixertest.c`, and `hash/fnv1a_test.c` currently exist
  outside the normal root build/test path.
- [ ] **P0: Add focused unit tests for public contracts.** Priority areas are
  INI/data parsing, formatter edge cases, list ownership and mutation,
  semaphore timeout/error behavior, thread/rwlock/event semantics, Unicode and
  endian conversion, and every hash/encode known-answer vector.
- [ ] **P1: Add fuzzing targets** for parsers, format processing, Unicode,
  base64/hex/LZH decoding, and list/block conversion.
- [ ] **P1: Add concurrency stress and sanitizer runs.** Use ThreadSanitizer
  where supported for message queues, linked lists, audio stream queues,
  semaphores, rwlocks, and events. Run leak detection on a platform where the
  selected AddressSanitizer supports it.
- [ ] **P1: Test resource exhaustion and cleanup.** Check file descriptors,
  Windows handles, threads, mutexes, events, semaphores, audio objects, dynamic
  libraries, and mappings over repeated init/destroy cycles.
- [ ] **P1: Make all public headers compile first and alone in supported C and
  C++ language modes.** This gate exists in the package consumer; retain it for
  every supported compiler and ensure it also covers the no-CMake consumption
  mode if that mode is supported.

## Licensing and provenance

- [ ] **P0: Perform a final per-file license audit**, including generated and
  newly written headers. Record the public-domain origin and upstream LGPL
  treatment of the LRZ-derived code so the conclusion does not depend on oral
  history.
- [ ] **P0: Re-verify that GPL uuencode and yEnc files and blobs are unreachable
  from every ref.** The check must cover commit history, not only the current
  working tree, and should become part of sync/release automation.
- [ ] **P0: Verify the distributed source and binary packages contain the
  correct LGPL license and any required permissive notices.** Confirm that
  dynamically and statically linking the component libraries does not omit a
  required notice.
- [ ] **P1: Add machine-readable license metadata or a repeatable license-scan
  report** if it can be maintained without misclassifying the historical
  public-domain/permissive files.

## Upstream synchronization

- [ ] **P0: Run a final manual upstream sync immediately before the release
  candidate**, resolve conflicts, and run the complete release matrix on the
  exact resulting commit.
- [ ] **P0: Require the sync workflow to run sufficient gates before pushing.**
  It currently performs a no-audio Linux build and ELF symbol checks, but not
  the installed-package tests or cross-platform matrix. Decide whether it
  should open a pull request instead of pushing when an upstream change alters
  ABI-sensitive files.
- [ ] **P0: Add guards for release-only files and policy changes.** An upstream
  projection must not silently remove CMake packaging, export maps, generated
  configuration, ownership APIs, tests, licensing exclusions, or this release
  checklist.
- [ ] **P1: Exercise conflict and failure paths.** Test changed/missing upstream
  directories, encode exclusions, non-fast-forward pushes, projection-tool
  failures, and a real content conflict. Each must fail loudly and leave useful
  diagnostics.
- [ ] **P1: Verify repository shape and history.** Only `main` should be
  published as the maintained branch, imported ancestry must remain reachable,
  and no unwanted upstream branches or tags should appear as public refs.
- [ ] **P1: Define the post-1.0 upstream ABI policy.** Upstream source changes
  cannot automatically gain exports or alter frozen layouts merely because
  they merge cleanly. Such changes need an ABI review and, where appropriate,
  a new symbol-version node or major version.

## Release mechanics and documentation

- [ ] **P0: Choose consistent release numbering.** Decide whether the first tag
  and GitHub release are `v1.0` or `v1.0.0`, and align `project(VERSION ...)`,
  full library filenames/current versions, package-version files, and release
  notes.
- [ ] **P0: Write release notes and an API/ABI compatibility statement.** List
  supported platforms/toolchains, thread and semaphore selections, audio
  backend behavior, ownership rules, component naming hazards, and explicitly
  unsupported combinations.
- [ ] **P0: Build and test from the release source archive**, not only from a
  Git checkout. Install it into a clean prefix and build both C and C++ example
  consumers against every requested component and linkage variant.
- [ ] **P0: Freeze upstream automation while selecting and tagging the release
  commit.** A scheduled sync must not move `main` between final validation and
  tag creation.
- [ ] **P0: Create an annotated or signed tag and preserve release artifacts.**
  Record checksums and the ABI/export manifests used for the 1.0 baseline.
- [ ] **P1: Decide whether official binary artifacts are in scope.** If they
  are, define supported architectures, runtime-library choices, debug-symbol
  handling, reproducibility expectations, and signing/notarization.
- [ ] **P1: Add a changelog/version-policy document** describing compatible
  1.x additions, symbol-version inheritance, deprecation, SONAME changes, and
  the handling of future upstream namespaced hash/encode APIs.

## Established decisions to preserve

These are not open work unless new evidence requires revisiting them:

- [x] CMake is the standalone repository's only supported build system.
- [x] The repository tracks the projected history of upstream `master` as
  local `main`; comio, hash, and the LGPL encode subset occupy separate
  subdirectories.
- [x] GPL uuencode and yEnc sources are excluded from the encode projection.
- [x] Core, comio, hash, and encode install as separate static and shared
  libraries so generic component symbols are opt-in.
- [x] The shared libraries use ABI major/SONAME 1, with component-specific ELF
  1.0 symbol-version nodes.
- [x] Core visibility is hidden by default, and component libraries have
  explicit platform export controls.
- [x] Configuration is installed as both a generated C header and CMake package
  metadata, including audio, thread, semaphore, and compatibility features.
- [x] macOS uses XPDev semaphores because unnamed POSIX semaphores are not
  implemented; other Unix platforms use native POSIX semaphores when the full
  wrapped API is available.
- [x] `sem_getvalue()`, `xp_sem_getvalue()`, and `xp_sem_setvalue()` are outside
  the portable XPDev contract.
- [x] Win32 wrappers are preferred where available; MinGW toolchains that use
  POSIX threads retain their native pthread ABI.
- [x] Unix events, XP semaphores, and Windows rwlocks use opaque handles where
  implemented.
- [x] Heap ownership uses family-specific allocators/releases, caller-local
  compatibility helpers, and installed-package ownership tests. The detailed
  contract lives in `HEAP-OWNERSHIP.md`.
- [x] Public headers do not redefine standard `snprintf()` or `vsnprintf()`;
  `safe_snprintf()` remains an explicitly selected compatibility API.
- [x] Native audio backends are preferred. SDL is the backend of last resort,
  currently intended primarily for Haiku.
