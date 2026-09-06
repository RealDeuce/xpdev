# xpdev

This is a standalone mirror of [`src/xpdev`](https://github.com/SynchronetBBS/sbbs/tree/master/src/xpdev)
from the Synchronet source repository. Its history was produced with `git subtree
split`, so commits that changed the xpdev subtree retain their original authors,
dates, messages, and ancestry, with `src/xpdev` relocated to this repository's
root.

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
Use `cmake --install build --prefix <path>` to override the platform's default
installation prefix.

The default build and install provide both static and shared libraries. CMake
consumers can select `xpdev::static` or `xpdev::shared`; `xpdev::xpdev` selects
the shared library:

```cmake
find_package(xpdev CONFIG REQUIRED)
target_link_libraries(my_program PRIVATE xpdev::xpdev)
```

The shared library uses ABI SONAME 1. Its release filename is versioned as
`libxpdev.so.1.0` on ELF platforms, with the usual `libxpdev.so.1` SONAME and
`libxpdev.so` development symlinks. ABI-compatible 1.x releases retain SONAME 1.

Public headers are installed under `include/xpdev` and can be included as, for
example, `#include <xpdev/genwrap.h>`. They automatically include the generated
`xpdev_config.h` where needed, preserving the feature macros used to compile the
library. The header also provides stable `XPDEV_*` capability macros.

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

xpdev is distributed under the GNU Library General Public License, version 2
or (at your option) any later version (`LGPL-2.0-or-later`). See [LICENSE](LICENSE).
Some imported source files carry separate permissive license notices in their
file headers.

## Upstream synchronization

The [upstream sync workflow](.github/workflows/sync-upstream.yml) runs every six
hours and can also be started manually. It fetches only `master` from
`SynchronetBBS/sbbs`, projects `src/xpdev` with the same `git subtree split`
operation used to create this repository, and merges new projected commits into
`main`.

Clean merges are pushed automatically. A content conflict, changed/missing
upstream path, fetch error, or rejected push fails the Actions job instead of
silently skipping an update. Enable **Read and write permissions** for workflows
under the repository's Actions settings so the job can push.

To run the integration locally from a clean checkout:

```sh
./.github/scripts/sync-upstream.sh
```

The script leaves a conflicted merge in place for normal manual resolution.
