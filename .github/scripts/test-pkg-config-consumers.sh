#!/bin/sh

set -eu

prefix=$1
expected_audio_backends=$2
pcdir="$prefix/lib/pkgconfig"
source_dir=$(pwd)
output_dir=${RUNNER_TEMP:-${TMPDIR:-/tmp}}/xpdev-pkg-config-consumers
mkdir -p "$output_dir"

executable_suffix=
case $(uname -s) in
	MINGW*|MSYS*|CYGWIN*) executable_suffix=.exe ;;
esac
shared_c="$output_dir/shared-c$executable_suffix"
shared_cxx="$output_dir/shared-cxx$executable_suffix"
static_c="$output_dir/static-c$executable_suffix"
static_cxx="$output_dir/static-cxx$executable_suffix"

export PKG_CONFIG_PATH="$pcdir"
pkg-config --validate "$pcdir"/*.pc
test "$(pkg-config --variable=audio_backends xpdev)" = \
	"$expected_audio_backends"
test "$(pkg-config --variable=audio_backends xpdev-static)" = \
	"$expected_audio_backends"

cc=${CC:-cc}
cxx=${CXX:-c++}

"$cc" "$source_dir/tests/pkg-config-consumer.c" \
	$(pkg-config --cflags --libs xpdev-comio xpdev-encode xpdev-hash) \
	-o "$shared_c"
"$cxx" "$source_dir/tests/pkg-config-consumer.cpp" \
	$(pkg-config --cflags --libs xpdev-comio xpdev-encode xpdev-hash) \
	-o "$shared_cxx"

case $(uname -s) in
	Darwin*)
		DYLD_LIBRARY_PATH="$prefix/lib" "$shared_c"
		DYLD_LIBRARY_PATH="$prefix/lib" "$shared_cxx"
		;;
	MINGW*|MSYS*|CYGWIN*)
		PATH="$prefix/bin:$PATH" "$shared_c"
		PATH="$prefix/bin:$PATH" "$shared_cxx"
		;;
	*)
		LD_LIBRARY_PATH="$prefix/lib" "$shared_c"
		LD_LIBRARY_PATH="$prefix/lib" "$shared_cxx"
		;;
esac

"$cc" "$source_dir/tests/pkg-config-consumer.c" \
	$(pkg-config --cflags --libs \
		xpdev-comio-static xpdev-encode-static xpdev-hash-static) \
	-o "$static_c"
"$static_c"
"$cxx" "$source_dir/tests/pkg-config-consumer.cpp" \
	$(pkg-config --cflags --libs \
		xpdev-comio-static xpdev-encode-static xpdev-hash-static) \
	-o "$static_cxx"
"$static_cxx"
