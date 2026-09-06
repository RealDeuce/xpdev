#!/usr/bin/env bash

set -euo pipefail

library="${1:-}"
if test -z "$library" || ! test -f "$library"; then
	printf 'usage: %s <ELF-shared-library>\n' "$0" >&2
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
grep -q 'XPDEV_1\.0' <<<"$version_info" \
	|| fail "$library does not define the XPDEV_1.0 symbol version"

symbol_table="$({ readelf -W -s -D "$library" || readelf --wide --dyn-syms "$library"; })"

unversioned="$(awk '
		$1 ~ /^[0-9]+:$/ && $7 != "UND" && ($5 == "GLOBAL" || $5 == "WEAK") {
			name = $8
			if (name == "" || name ~ /^XPDEV_[0-9]/)
				next
			if (name !~ /@XPDEV_[0-9]/)
				print name
		}' \
	<<<"$symbol_table" | sort -u)"

if test -n "$unversioned"; then
	printf '%s\n' "$unversioned" >&2
	fail "defined dynamic symbols without an XPDEV version were exported"
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

printf 'All defined dynamic symbols in %s have XPDEV symbol versions.\n' \
	"$library"
