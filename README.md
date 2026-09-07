# xpdev

This is a standalone mirror of selected libraries from the Synchronet source
repository. [`src/xpdev`](https://github.com/SynchronetBBS/sbbs/tree/master/src/xpdev)
is located at the repository root, while `src/comio`, `src/hash`, and
`src/encode` are mirrored into the matching `comio`, `hash`, and `encode`
subdirectories.

The imported histories retain their original authors, dates, messages, and
ancestry. The GPL-licensed uuencode and yEnc implementations are excluded from
every imported `encode` revision, so neither their files nor their historical
blobs are reachable from this repository.

## Building

CMake is the only supported build system:

```sh
cmake -S . -B build
cmake --build build
cmake --install build
```

Optional audio backends can be disabled at configure time with the
`WITHOUT_ALSA`, `WITHOUT_COREAUDIO`, `WITHOUT_OSS`, `WITHOUT_PIPEWIRE`,
`WITHOUT_PORTAUDIO`, `WITHOUT_PULSEAUDIO`, and `WITHOUT_SDL_AUDIO` options.
SDL audio is a fallback: it is enabled only when no native or dedicated audio
backend was detected and an SDL2-compatible development package is available.
This currently makes SDL the normal backend on Haiku without imposing its
entry-point requirements on platforms with direct audio support.
Windows builds prefer XPDev's thin Win32 pthread compatibility implementation.
Set `XPDEV_USE_SYSTEM_PTHREADS=ON` to use a detected POSIX pthread library when
interoperability with that implementation is required.

On Unix, CMake uses the platform's native POSIX semaphore implementation when
the complete API wrapped by `semwrap.h`, including `sem_timedwait()`, is
declared and linkable. macOS deliberately uses XPDev's semaphore implementation:
Darwin provides named POSIX semaphores, but its declared `sem_init()` for the
required unnamed semaphores returns `ENOSYS`. The namespaced `xp_sem_*` API
remains available on Unix regardless of which implementation `semwrap.h`
selects. The generated header and CMake package report the selection through
`XPDEV_USE_NATIVE_POSIX_SEMAPHORES`, `XPDEV_USE_XP_SEMAPHORES`,
`xpdev_USE_NATIVE_POSIX_SEMAPHORES`, and `xpdev_USE_XP_SEMAPHORES`.

Use `cmake --install build --prefix <path>` to override the platform's default
installation prefix.

The default build and install provide both static and shared variants of xpdev
and its three optional component libraries. CMake consumers can select
`xpdev::static` or `xpdev::shared`; `xpdev::xpdev` selects the shared library:

```cmake
find_package(xpdev CONFIG REQUIRED)
target_link_libraries(my_program PRIVATE xpdev::xpdev)
```

The comio, hash, and encode APIs are separate, opt-in libraries. Linking only
`xpdev::xpdev` does not place their generic API names in a program's dynamic
symbol scope. Each component has default/shared and static targets:

```cmake
find_package(xpdev CONFIG REQUIRED COMPONENTS hash encode)
target_link_libraries(my_program PRIVATE xpdev::hash xpdev::encode)
# Explicit alternatives: xpdev::hash_shared and xpdev::hash_static
```

### Component API naming limitations

The component APIs retain their upstream C symbol names. Some of those names,
particularly in the hash and encoding APIs, are generic enough to collide with
other implementations used by the same program. Splitting them into opt-in
libraries limits the damage: an application that does not link a component does
not acquire its symbols. It does not turn those symbols into a namespace.

This distinction matters in several places:

- A linker can still see an ambiguous or duplicate symbol when XPDev and
  another library both provide the same base name.
- Static archives have no symbol-version mechanism at all.
- ELF version nodes identify the selected component after a dynamic link has
  been resolved, but do not make the source-level or initial linker name
  unique.
- macOS and Windows export controls restrict which names are public, but do not
  qualify the public names that remain.
- Name-based lookup such as `dlsym()` still operates on the generic base name.

Consumers should therefore link only the components they need and should not
rely on the component split or ELF symbol versions to disambiguate two APIs
with the same C name.

XPDev deliberately does not invent prefixed aliases for these APIs. Synchronet
may add namespaced replacements in the future, and those replacements may be
part of the main XPDev library rather than these compatibility components.
Choosing names or ownership pre-emptively here could conflict with that future
upstream API. A later release can import upstream's namespaced interface while
preserving whatever compatibility the released component ABI requires.

The shared libraries use ABI SONAME 1. Their release filenames are versioned as
`libxpdev.so.1.0`, `libxpdev-comio.so.1.0`, `libxpdev-hash.so.1.0`, and
`libxpdev-encode.so.1.0` on ELF platforms, with the usual SONAME and development
symlinks. ABI-compatible 1.x releases retain SONAME 1. The main ELF library's
exports carry `XPDEV_1.0`; component exports carry `XPDEV_COMIO_1.0`,
`XPDEV_HASH_1.0`, or `XPDEV_ENCODE_1.0`. These component nodes make a linked
ELF reference require the selected component's version namespace and give the
dynamic loader precise ABI requirements. They do not resolve ambiguity while
initially linking two libraries that both define the same unversioned base
symbol; only namespacing the base symbol can guarantee that.

On macOS, CMake records compatibility version 1 and current version 1.0 in each
dylib; on Windows, the ABI major is part of each DLL filename. Explicit export
lists keep component implementation globals out of the shared-library ABI.
The main shared library's visibility is hidden by default: only declarations
marked `DLLEXPORT` are public, so implementation globals are not part of its ABI.
Compatibility shims are exported only on systems where xpdev supplies their
implementation; native platform functions are imported from the platform.
XPDev-specific portability extensions use the `xp_` prefix so they do not
occupy libc, pthread, curses, or other platform namespaces.

When adding API in an ABI-compatible 1.x release, add its exact symbol names to
a new node in the corresponding `cmake/xpdev*.map` file that inherits from that
library's 1.0 node; existing symbols remain at their original versions. CI
rejects ELF libraries that expose any defined dynamic symbol without their
library's expected `XPDEV_*` version.

Public headers are installed under `include/xpdev` and can be included as, for
example, `#include <xpdev/genwrap.h>`. They automatically include the generated
`xpdev_config.h` where needed, preserving the feature macros used to compile the
library. The header also provides stable `XPDEV_*` capability macros. ABI-
affecting choices such as linked-list thread safety and the semaphore type must
not be overridden by consumers; the generated header rejects conflicting
legacy definitions.

The installed CMake package exposes the same configuration through variables
such as `xpdev_AUDIO_ENABLED`, `xpdev_AUDIO_BACKENDS`, `xpdev_HAS_STDINT_H`, and
per-backend variables such as `xpdev_WITH_ALSA`. An application that requires
at least one audio backend can request the `audio` component:

```cmake
find_package(xpdev QUIET COMPONENTS audio)
if(NOT xpdev_FOUND)
	# A FetchContent fallback can be declared here.
endif()
```

An audio-component mismatch is reported before installed targets are imported,
so a fallback can create the normal xpdev target names without collisions.

## License

The included libraries are distributed under the GNU Library General Public
License, version 2 or (at your option) any later version
(`LGPL-2.0-or-later`). See [LICENSE](LICENSE). Some imported source files carry
separate permissive license notices in their file headers. Synchronet's
GPL-licensed `uucode` and `yenc` sources are intentionally not part of this
repository.

## Upstream synchronization

The [upstream sync workflow](.github/workflows/sync-upstream.yml) runs every six
hours and can also be started manually. It fetches only `master` from
`SynchronetBBS/sbbs`. It projects `src/xpdev` to the repository root with
`git subtree split`, and projects `src/comio`, `src/hash`, and the LGPL portion
of `src/encode` into their subdirectories with `git-filter-repo`. New projected
commits are merged into `main` with their full history.

Clean merges are pushed automatically. A content conflict, changed/missing
upstream path, fetch error, or rejected push fails the Actions job instead of
silently skipping an update. Enable **Read and write permissions** for workflows
under the repository's Actions settings so the job can push.

To run the integration locally from a clean checkout:

```sh
pipx install git-filter-repo==2.47.0
./.github/scripts/sync-upstream.sh
```

The script leaves a conflicted merge in place for normal manual resolution.
