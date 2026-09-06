#!/usr/bin/env bash

set -euo pipefail

library="${1:-}"
expected_version="${2:-XPDEV_1.0}"
if test -z "$library" || ! test -f "$library"; then
	printf 'usage: %s <ELF-shared-library> [expected-version]\n' "$0" >&2
	exit 2
fi
if [[ ! "$expected_version" =~ ^XPDEV(_[A-Z]+)?_[0-9]+\.[0-9]+$ ]]; then
	printf 'invalid XPDev symbol version: %s\n' "$expected_version" >&2
	exit 2
fi

fail() {
	printf '::error title=xpdev symbol-version check failed::%s\n' "$*" >&2
	exit 1
}

command -v readelf >/dev/null 2>&1 \
	|| fail "readelf is required"
readelf -h "$library" >/dev/null 2>&1 \
	|| fail "$library is not an ELF object"

version_info="$(readelf -V "$library")"
grep -Fq "$expected_version" <<<"$version_info" \
	|| fail "$library does not define the $expected_version symbol version"

symbol_table="$({ readelf -W -s -D "$library" || readelf --wide --dyn-syms "$library"; })"

unversioned="$(awk -v expected_version="$expected_version" '
		$1 ~ /^[0-9]+:$/ && $7 != "UND" && ($5 == "GLOBAL" || $5 == "WEAK") {
			name = $8
			if (name == "" || name ~ /^XPDEV(_[A-Z]+)?_[0-9]/)
				next
			if (index(name, "@" expected_version) == 0)
				print name
		}' \
	<<<"$symbol_table" | sort -u)"

if test -n "$unversioned"; then
	printf '%s\n' "$unversioned" >&2
	fail "defined dynamic symbols without $expected_version were exported"
fi

invalid_names="$(awk '
		$1 ~ /^[0-9]+:$/ && $7 != "UND" && ($5 == "GLOBAL" || $5 == "WEAK") {
			name = $8
			sub(/@.*/, "", name)
			if (name ~ /^pthread_/ ||
			    name ~ /^(getch|globi|kbhit|msclock|sem_trywait_block|unix_beep)$/ ||
			    name ~ /^_(echo_(off|on)|termios_(reset|setup))$/ ||
			    name ~ /^(alsa_api|init_sdl_audio|pa_api|pu_api|sdl_fillbuf)$/ ||
			    name ~ /^(xp_mixer_pull|xpbeep_load_sdl_funcs|xpbeep_sdl|xptone_close_locked)$/)
				print name
		}' \
	<<<"$symbol_table" | sort -u)"

if test -n "$invalid_names"; then
	printf '%s\n' "$invalid_names" >&2
	fail "platform or internal names leaked into the public ELF ABI"
fi

printf 'All defined dynamic symbols in %s have symbol version %s.\n' \
	"$library" "$expected_version"
