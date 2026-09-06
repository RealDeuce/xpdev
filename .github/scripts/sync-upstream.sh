#!/usr/bin/env bash

set -euo pipefail

upstream_url="${UPSTREAM_URL:-https://github.com/SynchronetBBS/sbbs.git}"
upstream_branch="${UPSTREAM_BRANCH:-master}"
upstream_prefix="${UPSTREAM_PREFIX:-src/xpdev}"
upstream_remote="${UPSTREAM_REMOTE:-upstream-sbbs}"

fail() {
	printf '::error title=Upstream xpdev sync failed::%s\n' "$*" >&2
	exit 1
}

git rev-parse --is-inside-work-tree >/dev/null 2>&1 \
	|| fail "Run this script from an xpdev Git checkout"

if test -n "$(git status --porcelain)"; then
	fail "The checkout is not clean; commit or stash changes before syncing"
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
git cat-file -e "${upstream_ref}:${upstream_prefix}" 2>/dev/null \
	|| fail "${upstream_prefix} does not exist at ${upstream_ref}"

printf 'Projecting %s to the repository root...\n' "$upstream_prefix"
split_commit="$(git subtree split --quiet --prefix="$upstream_prefix" "$upstream_ref")"
git cat-file -e "${split_commit}^{commit}" 2>/dev/null \
	|| fail "git subtree split did not produce a commit"

if git merge-base --is-ancestor "$split_commit" HEAD; then
	printf 'Already synchronized at %s.\n' "$split_commit"
	exit 0
fi

short_commit="$(git rev-parse --short=12 "$split_commit")"
printf 'Merging projected upstream commit %s...\n' "$short_commit"
if ! git merge --no-ff -m \
	"Merge src/xpdev from SynchronetBBS/sbbs@${short_commit}" \
	"$split_commit"; then
	printf 'Conflicted paths:\n' >&2
	git status --short >&2
	fail "Upstream changes conflict with standalone changes; manual resolution is required"
fi

printf 'Upstream xpdev changes merged successfully.\n'
