# xpdev

This is a standalone mirror of [`src/xpdev`](https://github.com/SynchronetBBS/sbbs/tree/master/src/xpdev)
from the Synchronet source repository. Its history was produced with `git subtree
split`, so commits that changed the xpdev subtree retain their original authors,
dates, messages, and ancestry, with `src/xpdev` relocated to this repository's
root.

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
