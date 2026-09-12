#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
. "${script_dir}/sync-upstream-lib.sh"

test_repo="$(mktemp -d "${TMPDIR:-/tmp}/xpdev-sync-test.XXXXXX")"
cleanup() {
	rm -rf -- "$test_repo"
}
trap cleanup EXIT

git init --quiet --initial-branch=main "$test_repo"
git -C "$test_repo" config user.name "XPDev sync test"
git -C "$test_repo" config user.email "sync-test@example.invalid"

mkdir -p "$test_repo/src/xpdev"
printf 'original\n' > "$test_repo/src/xpdev/value"
printf 'base\n' > "$test_repo/unrelated"
git -C "$test_repo" add .
git -C "$test_repo" commit --quiet -m "Base"
base="$(git -C "$test_repo" rev-parse HEAD)"
split="$(git -C "$test_repo" subtree split --quiet \
	--prefix=src/xpdev "$base")"

normalized="$(cd "$test_repo" \
	&& normalize_xpdev_checkpoint "$base" "$split" src/xpdev)"
if test "$normalized" != "$base"; then
	printf 'Non-merge checkpoint changed from %s to %s\n' \
		"$base" "$normalized" >&2
	exit 1
fi

git -C "$test_repo" switch --quiet -c no-op-side "$base"
printf 'side\n' > "$test_repo/side"
git -C "$test_repo" add side
git -C "$test_repo" commit --quiet -m "Unrelated side change"

git -C "$test_repo" switch --quiet -c no-op-main "$base"
printf 'main\n' > "$test_repo/main"
git -C "$test_repo" add main
git -C "$test_repo" commit --quiet -m "Unrelated main change"
no_op_first_parent="$(git -C "$test_repo" rev-parse HEAD)"
git -C "$test_repo" merge --quiet --no-ff -m "No-op XPDev merge" no-op-side
no_op_merge="$(git -C "$test_repo" rev-parse HEAD)"

normalized="$(cd "$test_repo" \
	&& normalize_xpdev_checkpoint "$no_op_merge" "$split" src/xpdev \
		2>/dev/null)"
if test "$normalized" != "$no_op_first_parent"; then
	printf 'No-op merge normalized to %s instead of %s\n' \
		"$normalized" "$no_op_first_parent" >&2
	exit 1
fi

git -C "$test_repo" switch --quiet -c changing-side "$base"
printf 'changed\n' > "$test_repo/src/xpdev/value"
git -C "$test_repo" add src/xpdev/value
git -C "$test_repo" commit --quiet -m "Change XPDev on side"

git -C "$test_repo" switch --quiet -c changing-main "$base"
printf 'other\n' > "$test_repo/other"
git -C "$test_repo" add other
git -C "$test_repo" commit --quiet -m "Unrelated mainline change"
git -C "$test_repo" merge --quiet --no-ff -m "XPDev-changing merge" \
	changing-side
changing_merge="$(git -C "$test_repo" rev-parse HEAD)"
changing_split="$(git -C "$test_repo" subtree split --quiet \
	--prefix=src/xpdev "$changing_merge")"

if (cd "$test_repo" \
		&& normalize_xpdev_checkpoint \
			"$changing_merge" "$changing_split" src/xpdev 2>/dev/null); then
	printf 'XPDev-changing merge was incorrectly accepted as a checkpoint\n' >&2
	exit 1
fi

if (cd "$test_repo" \
		&& normalize_xpdev_checkpoint \
			"$base" "$changing_split" src/xpdev 2>/dev/null); then
	printf 'Mismatched source and split trees were incorrectly accepted\n' >&2
	exit 1
fi

printf 'Upstream checkpoint normalization tests passed.\n'
