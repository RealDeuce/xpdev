#!/usr/bin/env bash

set -euo pipefail

upstream_url="${UPSTREAM_URL:-https://github.com/SynchronetBBS/sbbs.git}"
upstream_branch="${UPSTREAM_BRANCH:-master}"
upstream_remote="${UPSTREAM_REMOTE:-upstream-sbbs}"
xpdev_prefix="src/xpdev"
filter_repo_version="2.47.0"

fail() {
	printf '::error title=Synchronet upstream sync failed::%s\n' "$*" >&2
	exit 1
}

git rev-parse --is-inside-work-tree >/dev/null 2>&1 \
	|| fail "Run this script from an xpdev Git checkout"

if test -n "$(git status --porcelain)"; then
	fail "The checkout is not clean; commit or stash changes before syncing"
fi

if ! command -v git-filter-repo >/dev/null 2>&1; then
	fail "git-filter-repo ${filter_repo_version} is required to project comio, hash, and encode"
fi

if git remote get-url "$upstream_remote" >/dev/null 2>&1; then
	git remote set-url "$upstream_remote" "$upstream_url"
else
	git remote add "$upstream_remote" "$upstream_url"
fi

printf 'Fetching %s branch %s (without tags)...\n' \
	"$upstream_url" "$upstream_branch"
git fetch --no-tags --prune "$upstream_remote" \
	"+refs/heads/${upstream_branch}:refs/remotes/${upstream_remote}/${upstream_branch}"

upstream_ref="refs/remotes/${upstream_remote}/${upstream_branch}"
for path in "$xpdev_prefix" src/comio src/hash src/encode; do
	git cat-file -e "${upstream_ref}:${path}" 2>/dev/null \
		|| fail "${path} does not exist at ${upstream_ref}"
done

printf 'Projecting %s to the repository root...\n' "$xpdev_prefix"
xpdev_commit="$(git subtree split --quiet --prefix="$xpdev_prefix" "$upstream_ref")"
git cat-file -e "${xpdev_commit}^{commit}" 2>/dev/null \
	|| fail "git subtree split did not produce an xpdev commit"

if ! git merge-base --is-ancestor "$xpdev_commit" HEAD; then
	short_commit="$(git rev-parse --short=12 "$xpdev_commit")"
	printf 'Merging projected xpdev commit %s...\n' "$short_commit"
	if ! git merge --no-ff -m \
		"Merge src/xpdev from SynchronetBBS/sbbs@${short_commit}" \
		"$xpdev_commit"; then
		printf 'Conflicted paths:\n' >&2
		git status --short >&2
		fail "Upstream xpdev changes conflict with standalone changes; manual resolution is required"
	fi
else
	printf 'xpdev is already synchronized at %s.\n' \
		"$(git rev-parse --short=12 "$xpdev_commit")"
fi

# Project the three supporting directories together. Their paths remain as
# comio/, hash/, and encode/, while unrelated sbbs content and the GPL uuencode
# and yEnc implementations are removed from every historical tree.
repo_root="$(git rev-parse --show-toplevel)"
projection_repo="$(mktemp -d "${TMPDIR:-/tmp}/xpdev-support-sync.XXXXXX")"
cleanup() {
	rm -rf -- "$projection_repo"
}
trap cleanup EXIT

printf 'Projecting src/comio, src/hash, and the LGPL portion of src/encode...\n'
git clone --quiet --shared --no-checkout "$repo_root" "$projection_repo"
git -C "$projection_repo" fetch --quiet "$repo_root" \
	"${upstream_ref}:refs/heads/upstream-support"
git -C "$projection_repo" filter-repo --force \
	--refs refs/heads/upstream-support \
	--filename-callback '
if filename in (b"src/encode/uucode.c", b"src/encode/uucode.h",
                b"src/encode/yenc.c", b"src/encode/yenc.h"):
    return None
if filename.startswith((b"src/comio/", b"src/hash/", b"src/encode/")):
    return filename[4:]
return None
' >/dev/null

if git -C "$projection_repo" rev-list --objects upstream-support \
		| grep -Eiq '(^|/)(uucode|yenc)(\.|/|$)'; then
	fail "The support projection retained an excluded uuencode or yEnc path"
fi

git fetch --quiet --no-tags "$projection_repo" \
	refs/heads/upstream-support
support_commit="$(git rev-parse FETCH_HEAD)"
git cat-file -e "${support_commit}^{commit}" 2>/dev/null \
	|| fail "git-filter-repo did not produce a support commit"

if ! git merge-base --is-ancestor "$support_commit" HEAD; then
	short_commit="$(git rev-parse --short=12 "$support_commit")"
	printf 'Merging projected support commit %s...\n' "$short_commit"
	if ! git merge --no-ff -m \
		"Merge comio, hash, and encode from SynchronetBBS/sbbs@${short_commit}" \
		"$support_commit"; then
		printf 'Conflicted paths:\n' >&2
		git status --short >&2
		fail "Upstream support-library changes conflict with standalone changes; manual resolution is required"
	fi
else
	printf 'comio, hash, and encode are already synchronized at %s.\n' \
		"$(git rev-parse --short=12 "$support_commit")"
fi

printf 'All upstream projects synchronized successfully.\n'
