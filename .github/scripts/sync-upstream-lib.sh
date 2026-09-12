#!/usr/bin/env bash

# Normalize an upstream source commit for use as git-subtree-mainline.
#
# git subtree's prior-split scan assumes that git-subtree-mainline names a
# commit with at most one parent.  When it names a merge, git subtree excludes
# only that commit's first-parent history.  A parent from another side of the
# merge can then be visited without its ancestors and make check_parents()
# recurse through the old history until the shell's recursion limit is hit.
#
# Move a merge checkpoint down its first-parent chain while the projected tree
# is unchanged.  The tree comparison also verifies that the recorded source
# really corresponds to the recorded split.  If a merge itself changed the
# projected directory, there is no safe incremental checkpoint for this
# mechanism and the caller must perform a full projection instead.
normalize_xpdev_checkpoint() {
	local source="$1"
	local split="$2"
	local prefix="$3"
	local source_tree
	local split_tree
	local parent_line
	local -a commit_and_parents
	local first_parent
	local first_parent_tree

	source_tree="$(git rev-parse "${source}:${prefix}" 2>/dev/null)" \
		|| return 1
	split_tree="$(git rev-parse "${split}^{tree}" 2>/dev/null)" \
		|| return 1
	if test "$source_tree" != "$split_tree"; then
		printf 'Recorded source and split trees for %s differ; full projection is required.\n' \
			"$prefix" >&2
		return 1
	fi

	while :; do
		parent_line="$(git rev-list --parents -n 1 "$source")" \
			|| return 1
		read -r -a commit_and_parents <<< "$parent_line"
		if test "${#commit_and_parents[@]}" -lt 3; then
			printf '%s\n' "$source"
			return 0
		fi

		first_parent="${commit_and_parents[1]}"
		first_parent_tree="$(git rev-parse \
			"${first_parent}:${prefix}" 2>/dev/null)" \
			|| return 1
		if test "$first_parent_tree" != "$source_tree"; then
			printf 'Merge checkpoint %s changed %s; full projection is required.\n' \
				"$(git rev-parse --short=12 "$source")" "$prefix" >&2
			return 1
		fi

		printf 'Normalizing no-op %s merge checkpoint %s to first parent %s.\n' \
			"$prefix" "$(git rev-parse --short=12 "$source")" \
			"$(git rev-parse --short=12 "$first_parent")" >&2
		source="$first_parent"
	done
}
