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
```

Optional audio backends can be disabled at configure time with the
`WITHOUT_ALSA`, `WITHOUT_COREAUDIO`, `WITHOUT_OSS`, `WITHOUT_PIPEWIRE`,
`WITHOUT_PORTAUDIO`, `WITHOUT_PULSEAUDIO`, and `WITHOUT_SDL_AUDIO` options.

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
