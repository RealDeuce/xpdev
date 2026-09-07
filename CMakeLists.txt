cmake_minimum_required(VERSION 3.16)

project(XPDev VERSION 1.0 LANGUAGES C)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	set(XPDEV_BUILD_INSTALLABLE_LIBRARIES_DEFAULT ON)
else()
	set(XPDEV_BUILD_INSTALLABLE_LIBRARIES_DEFAULT OFF)
endif()
option(XPDEV_BUILD_INSTALLABLE_LIBRARIES
	"Build and install the static and shared xpdev libraries"
	${XPDEV_BUILD_INSTALLABLE_LIBRARIES_DEFAULT})
unset(XPDEV_BUILD_INSTALLABLE_LIBRARIES_DEFAULT)

set(WITHOUT_OSS OFF CACHE BOOL "Disable OSS Audio")
set(WITHOUT_SDL_AUDIO OFF CACHE BOOL "Disable SDL Audio fallback")
set(WITHOUT_ALSA OFF CACHE BOOL "Disable ALSA Audio")
set(WITHOUT_PORTAUDIO OFF CACHE BOOL "Disable PortAudio Audio")
set(WITHOUT_PULSEAUDIO OFF CACHE BOOL "Disable PulseAudio Audio")
set(WITHOUT_PIPEWIRE OFF CACHE BOOL "Disable PipeWire Audio")
set(WITHOUT_COREAUDIO OFF CACHE BOOL "Disable Core Audio (macOS)")

include(CheckSymbolExists)
include(CheckIncludeFiles)
include(CheckCSourceCompiles)
find_package(Threads REQUIRED)
if(WIN32)
	set(XPDEV_USE_SYSTEM_PTHREADS_DEFAULT OFF)
	if(MINGW AND CMAKE_C_COMPILER_ID STREQUAL "GNU")
		execute_process(
			COMMAND "${CMAKE_C_COMPILER}" -v
			OUTPUT_VARIABLE XPDEV_MINGW_COMPILER_VERSION
			ERROR_VARIABLE XPDEV_MINGW_COMPILER_VERSION)
		if(NOT XPDEV_MINGW_COMPILER_VERSION MATCHES
				"Thread model: *([^ \r\n]+)")
			message(FATAL_ERROR
				"Unable to determine the MinGW GCC thread model")
		endif()
		set(XPDEV_MINGW_THREAD_MODEL "${CMAKE_MATCH_1}")
		message(STATUS
			"MinGW GCC thread model: ${XPDEV_MINGW_THREAD_MODEL}")
		if(XPDEV_MINGW_THREAD_MODEL STREQUAL "posix")
			set(XPDEV_USE_SYSTEM_PTHREADS_DEFAULT ON)
		endif()
		unset(XPDEV_MINGW_COMPILER_VERSION)
		unset(XPDEV_MINGW_THREAD_MODEL)
	elseif(MINGW AND CMAKE_USE_PTHREADS_INIT)
		set(XPDEV_USE_SYSTEM_PTHREADS_DEFAULT ON)
	endif()
	set(XPDEV_USE_SYSTEM_PTHREADS ${XPDEV_USE_SYSTEM_PTHREADS_DEFAULT} CACHE BOOL
		"Use a detected POSIX pthread implementation instead of XPDev's Win32 wrappers")
	unset(XPDEV_USE_SYSTEM_PTHREADS_DEFAULT)
	if(XPDEV_USE_SYSTEM_PTHREADS AND NOT CMAKE_USE_PTHREADS_INIT)
		message(FATAL_ERROR
			"XPDEV_USE_SYSTEM_PTHREADS requires a POSIX pthread implementation")
	endif()
else()
	set(XPDEV_USE_SYSTEM_PTHREADS TRUE)
endif()
if(NOT WITHOUT_SDL_AUDIO)
	if(WIN32 AND NOT XPDEV_BUILD_INSTALLABLE_LIBRARIES)
		set(SDL2_FOUND ON)
		set(SDL2_INCLUDE_DIRS
			${CMAKE_CURRENT_SOURCE_DIR}/../../3rdp/win32.release/sdl2/include)
	else()
		find_package(SDL2 CONFIG PATHS /Library/Frameworks)
	endif()
endif()
if(NOT WITHOUT_PULSEAUDIO)
	find_package(PulseAudio CONFIG QUIET PATHS /usr/local)
	if(PulseAudio_FOUND)
		set(PULSEAUDIO_FOUND TRUE)
	endif()
endif()
find_package(PkgConfig)
if(PKGCONFIG_FOUND)
	if(NOT WITHOUT_ALSA)
		pkg_check_modules(ALSA alsa)
	endif()
	if(NOT WITHOUT_PORTAUDIO)
		pkg_check_modules(PORTAUDIO portaudio-2.0)
	endif()
	if(NOT WITHOUT_PULSEAUDIO AND NOT PULSEAUDIO_FOUND)
		pkg_check_modules(PULSEAUDIO libpulse)
	endif()
	if(NOT WITHOUT_PIPEWIRE)
		pkg_check_modules(PIPEWIRE libpipewire-0.3)
	endif()
endif()
if(NOT WITHOUT_PORTAUDIO AND NOT PORTAUDIO_FOUND)
	find_path(PORTAUDIO_INCLUDEDIR portaudio.h)
	if(PORTAUDIO_INCLUDEDIR)
		set(PORTAUDIO_FOUND TRUE)
	endif()
endif()

set(SOURCE
    conwrap.c
    dat_file.c
    datewrap.c
    dirwrap.c
    filewrap.c
    genwrap.c
    ini_file.c
    link_list.c
    msg_queue.c
    multisock.c
    named_str_list.c
    netwrap.c
    os_info.c
    rwlockwrap.c
    semfile.c
    semwrap.c
    sockwrap.c
    stbuf.c
    str_list.c
    strwrap.c
    threadwrap.c
    unicode.c
    xp_dl.c
    xpbeep.c
    xpdatetime.c
    xpmap.c
    xpprintf.c
)

set(HEADER
	conwrap.h
	cp437defs.h
	dat_file.h
	datewrap.h
	dirwrap.h
	eventwrap.h
	filewrap.h
	gen_defs.h
	genwrap.h
	haproxy.h
	ini_file.h
	link_list.h
	msg_queue.h
	multisock.h
	named_str_list.h
	netwrap.h
	os_info.h
	petdefs.h
	rwlockwrap.h
	semfile.h
	semwrap.h
	sockwrap.h
	stbuf.h
	str_list.h
	strwrap.h
	threadwrap.h
	unicode_defs.h
	unicode.h
	wrapdll.h
	xp_dl.h
	xpbeep.h
	xpdatetime.h
	xpendian.h
	xpmap.h
	xpprintf.h
)

if(NOT WIN32)
	list(APPEND SOURCE xpevent.c)
	# Unix-only implementation header included by eventwrap.h.
	list(APPEND HEADER xpevent.h)
endif()

check_symbol_exists(strlcpy string.h HAS_STRLCPY)
check_symbol_exists(asprintf stdio.h HAS_ASPRINTF)
check_symbol_exists(vasprintf stdio.h HAS_VASPRINTF)
check_symbol_exists(getdelim stdio.h HAS_GETDELIM)
check_symbol_exists(strndup string.h HAS_STRNDUP)
check_symbol_exists(strcasestr string.h HAS_STRCASESTR)
check_symbol_exists(strtok_r string.h HAS_STRTOK_R)
check_symbol_exists(random stdlib.h HAS_RANDOM_FUNC) 
check_symbol_exists(srandomdev stdlib.h HAS_SRANDOMDEV_FUNC)
check_symbol_exists(mkstemp stdlib.h HAS_MKSTEMP_FUNC)

check_include_files(inttypes.h HAS_INTTYPES_H)
check_include_files(stdint.h HAS_STDINT_H)
check_include_files(dev/speaker/speaker.h HAS_DEV_SPEAKER_SPEAKER_H)
check_include_files(dev/machine/speaker.h HAS_MACHINE_SPEAKER_H)
check_include_files(dev/speaker/spkr.h HAS_MACHINE_SPKR_H)
if(NOT WITHOUT_OSS)
	check_include_files(sys/soundcard.h HAS_SYS_SOUNDCARD_H) 
	check_include_files(soundcard.h HAS_SOUNDCARD_H) 
	check_include_files(linux/soundcard.h HAS_LINUX_SOUNDCARD_H) 
endif()

function(xpdev_configure_target target)
	target_compile_features(${target} PUBLIC c_std_11)
	target_include_directories(${target} PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
	target_compile_definitions(${target} PUBLIC XPDEV_USE_CONFIG_H)
	target_link_libraries(${target} PUBLIC ${CMAKE_DL_LIBS})
	if(XPDEV_USE_SYSTEM_PTHREADS)
		target_link_libraries(${target} PUBLIC Threads::Threads)
	endif()
	if(XPDEV_POSIX_SEMAPHORE_LIBRARY)
		target_link_libraries(${target} PUBLIC ${XPDEV_POSIX_SEMAPHORE_LIBRARY})
	endif()

	if(WIN32)
		target_compile_definitions(${target} PUBLIC
			_WIN32 _WIN32_WINNT=0x0501 WINVER=0x0501
			MSVCRT_VERSION=0x0501 _WIN32_IE=0x0500)
		# ole32 / uuid are needed by the WASAPI code in xpbeep.c.
		target_link_libraries(${target} PUBLIC
			iphlpapi ws2_32 winmm netapi32 ole32 uuid)
	elseif(CMAKE_SYSTEM_NAME STREQUAL "Haiku")
		target_compile_definitions(${target} PUBLIC __unix__)
		target_link_libraries(${target} PUBLIC m network)
	elseif(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
		target_link_libraries(${target} PUBLIC m)
		if(NOT WITHOUT_OSS)
			target_link_libraries(${target} PUBLIC ossaudio)
		endif()
	elseif(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
		target_link_libraries(${target} PUBLIC m)
		if(NOT WITHOUT_OSS)
			target_link_libraries(${target} PUBLIC ossaudio)
		endif()
	elseif(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
		target_link_libraries(${target} PUBLIC socket m)
	elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
		target_compile_definitions(${target} PUBLIC __unix__ __DARWIN__)
		target_link_libraries(${target} PUBLIC m)
	elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
		target_compile_definitions(${target} PUBLIC __unix__)
		target_link_libraries(${target} PUBLIC m)
	elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
		target_compile_definitions(${target} PUBLIC
			_DEFAULT_SOURCE _BSD_SOURCE SPEED_MACROS_ONLY _GNU_SOURCE
			_FILE_OFFSET_BITS=64)
		target_link_libraries(${target} PUBLIC m)
	else()
		target_link_libraries(${target} PUBLIC m)
	endif()

	# MSVC requires /experimental:c11atomics to enable <stdatomic.h> + _Atomic.
	# This is PUBLIC because threadwrap.h exposes _Atomic types to C consumers.
	if(MSVC)
		target_compile_options(${target} PUBLIC
			$<$<COMPILE_LANGUAGE:C>:/experimental:c11atomics>)
	endif()

	if(XPDEV_USE_XP_SEMAPHORES)
		target_compile_definitions(${target} PUBLIC USE_XP_SEMAPHORES)
	endif()
	if(XPDEV_PREFER_POLL)
		target_compile_definitions(${target} PUBLIC PREFER_POLL)
	endif()
	if(XPDEV_LINK_LIST_THREADSAFE)
		target_compile_definitions(${target} PUBLIC LINK_LIST_THREADSAFE)
	endif()
	if(XPDEV_THREAD_SAFE_ENABLED)
		target_compile_definitions(${target} PUBLIC XPDEV_THREAD_SAFE)
	endif()

	if(XPDEV_HAS_INTTYPES_H)
		target_compile_definitions(${target} PUBLIC HAS_INTTYPES_H)
	endif()
	if(XPDEV_HAS_STDINT_H)
		target_compile_definitions(${target} PUBLIC HAS_STDINT_H)
	endif()

	if(XPDEV_WITH_OSS)
		target_compile_definitions(${target} PUBLIC
			SOUNDCARD_H_IN=${XPDEV_SOUNDCARD_H_IN})
	endif()

	if(HAS_DEV_SPEAKER_SPEAKER_H)
		target_compile_definitions(${target} PRIVATE HAS_DEV_SPEAKER_SPEAKER_H)
	endif()
	if(HAS_MACHINE_SPEAKER_H)
		target_compile_definitions(${target} PRIVATE HAS_MACHINE_SPEAKER_H)
	endif()
	if(HAS_MACHINE_SPKR_H)
		target_compile_definitions(${target} PRIVATE HAS_MACHINE_SPKR_H)
	endif()
	if(EXISTS /dev/urandom)
		target_compile_definitions(${target} PRIVATE
			HAS_DEV_URANDOM URANDOM_DEV="/dev/urandom")
	endif()
	if(EXISTS /dev/random)
		target_compile_definitions(${target} PRIVATE
			HAS_DEV_RANDOM RANDOM_DEV="/dev/random")
	endif()
	if(HAS_RANDOM_FUNC)
		target_compile_definitions(${target} PRIVATE HAS_RANDOM_FUNC)
	endif()
	if(HAS_SRANDOMDEV_FUNC)
		target_compile_definitions(${target} PRIVATE HAS_SRANDOMDEV_FUNC)
	endif()
	if(HAS_MKSTEMP_FUNC)
		target_compile_definitions(${target} PRIVATE DISABLE_MKSTEMP_DEFINE)
	endif()

	if(XPDEV_WITH_SDL_AUDIO)
		if(TARGET SDL2::SDL2)
			target_include_directories(${target} PUBLIC
				"$<BUILD_INTERFACE:$<TARGET_PROPERTY:SDL2::SDL2,INTERFACE_INCLUDE_DIRECTORIES>>"
				"$<INSTALL_INTERFACE:$<TARGET_PROPERTY:SDL2::SDL2,INTERFACE_INCLUDE_DIRECTORIES>>")
		else()
			target_include_directories(${target} PUBLIC
				"$<BUILD_INTERFACE:${SDL2_INCLUDE_DIRS}>")
		endif()
		target_compile_definitions(${target} PUBLIC WITH_SDL_AUDIO)
		if(XPDEV_STATIC_SDL)
			target_compile_definitions(${target} PUBLIC STATIC_SDL)
			target_link_libraries(${target} PUBLIC SDL2::SDL2)
		endif()
	endif()

	if(XPDEV_WITH_PORTAUDIO)
		target_include_directories(${target} PRIVATE
			${PORTAUDIO_INCLUDE_DIRS} ${PORTAUDIO_INCLUDEDIR})
		target_compile_definitions(${target} PUBLIC WITH_PORTAUDIO)
	endif()
	if(XPDEV_WITH_PULSEAUDIO)
		target_include_directories(${target} PRIVATE
			${PULSEAUDIO_INCLUDE_DIRS} ${PULSEAUDIO_INCLUDE_DIR})
		target_compile_definitions(${target} PUBLIC WITH_PULSEAUDIO)
	endif()
	if(XPDEV_WITH_PIPEWIRE)
		target_include_directories(${target} PRIVATE ${PIPEWIRE_INCLUDE_DIRS})
		target_compile_definitions(${target} PUBLIC WITH_PIPEWIRE)
	endif()
	if(XPDEV_WITH_COREAUDIO)
		target_compile_definitions(${target} PUBLIC WITH_COREAUDIO)
		target_link_libraries(${target} PUBLIC ${AUDIOTOOLBOX_FRAMEWORK})
	endif()
	if(XPDEV_WITH_ALSA)
		if(ALSA_FOUND)
			target_include_directories(${target} PRIVATE ${ALSA_INCLUDE_DIRS})
		endif()
		target_compile_definitions(${target} PUBLIC USE_ALSA_SOUND)
	endif()
	if(XPDEV_NEEDS_STRLCPY)
		target_compile_definitions(${target} PUBLIC NEEDS_STRLCPY)
	endif()
	if(XPDEV_NEEDS_ASPRINTF)
		target_compile_definitions(${target} PUBLIC NEEDS_ASPRINTF)
	endif()
	if(XPDEV_NEEDS_VASPRINTF)
		target_compile_definitions(${target} PUBLIC NEEDS_VASPRINTF)
	endif()
	if(XPDEV_NEEDS_GETDELIM)
		target_compile_definitions(${target} PUBLIC NEEDS_GETDELIM)
	endif()
	if(XPDEV_NEEDS_STRNDUP)
		target_compile_definitions(${target} PUBLIC NEEDS_STRNDUP)
	endif()
	if(XPDEV_NEEDS_STRCASESTR)
		target_compile_definitions(${target} PUBLIC NEEDS_STRCASESTR)
	endif()
	if(XPDEV_NEEDS_STRTOK_R)
		target_compile_definitions(${target} PUBLIC NEEDS_STRTOK_R)
	endif()
	if(XPDEV_WITHOUT_CRYPTLIB)
		target_compile_definitions(${target} PUBLIC WITHOUT_CRYPTLIB)
	endif()
endfunction()

if(NOT WITHOUT_COREAUDIO AND APPLE)
	find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
endif()
if(NOT WITHOUT_ALSA AND NOT ALSA_FOUND)
	check_include_files(alsa/asoundlib.h USE_ALSA_SOUND)
endif()

# Resolve the actual compiled-in audio capabilities once, then use this state
# for the libraries, generated C header, and installed CMake package metadata.
set(XPDEV_WITH_WASAPI FALSE)
if(WIN32)
	set(XPDEV_WITH_WASAPI TRUE)
endif()

set(XPDEV_WITH_OSS FALSE)
set(XPDEV_SOUNDCARD_H_IN 0)
if(NOT WITHOUT_OSS)
	if(HAS_SYS_SOUNDCARD_H)
		set(XPDEV_WITH_OSS TRUE)
		set(XPDEV_SOUNDCARD_H_IN 1)
	elseif(HAS_SOUNDCARD_H)
		set(XPDEV_WITH_OSS TRUE)
		set(XPDEV_SOUNDCARD_H_IN 2)
	elseif(HAS_LINUX_SOUNDCARD_H)
		set(XPDEV_WITH_OSS TRUE)
		set(XPDEV_SOUNDCARD_H_IN 3)
	endif()
endif()

set(XPDEV_WITH_ALSA FALSE)
if(NOT WITHOUT_ALSA AND (ALSA_FOUND OR USE_ALSA_SOUND))
	set(XPDEV_WITH_ALSA TRUE)
endif()
set(XPDEV_WITH_PORTAUDIO FALSE)
if(NOT WITHOUT_PORTAUDIO AND PORTAUDIO_FOUND)
	set(XPDEV_WITH_PORTAUDIO TRUE)
endif()
set(XPDEV_WITH_PULSEAUDIO FALSE)
if(NOT WITHOUT_PULSEAUDIO AND PULSEAUDIO_FOUND)
	set(XPDEV_WITH_PULSEAUDIO TRUE)
endif()
set(XPDEV_WITH_PIPEWIRE FALSE)
if(NOT WITHOUT_PIPEWIRE AND PIPEWIRE_FOUND)
	set(XPDEV_WITH_PIPEWIRE TRUE)
endif()
set(XPDEV_WITH_COREAUDIO FALSE)
if(NOT WITHOUT_COREAUDIO AND APPLE AND AUDIOTOOLBOX_FRAMEWORK)
	set(XPDEV_WITH_COREAUDIO TRUE)
endif()

# SDL is the fallback for targets where no native or dedicated audio backend
# was found. In particular, this currently makes it the normal Haiku backend
# without imposing SDL's entry-point requirements on other platforms.
set(XPDEV_WITH_SDL_AUDIO FALSE)
set(XPDEV_NON_SDL_AUDIO_ENABLED FALSE)
foreach(backend IN ITEMS WASAPI COREAUDIO PORTAUDIO PIPEWIRE PULSEAUDIO ALSA OSS)
	if(XPDEV_WITH_${backend})
		set(XPDEV_NON_SDL_AUDIO_ENABLED TRUE)
	endif()
endforeach()
if(NOT WITHOUT_SDL_AUDIO AND SDL2_FOUND AND NOT XPDEV_NON_SDL_AUDIO_ENABLED)
	set(XPDEV_WITH_SDL_AUDIO TRUE)
	list(APPEND SOURCE sdlfuncs.c)
	if(WIN32 AND WITHOUT_SDL)
		list(APPEND SOURCE SDL_win32_main.c)
	endif()
endif()
unset(XPDEV_NON_SDL_AUDIO_ENABLED)

set(XPDEV_AUDIO_BACKENDS "")
foreach(backend IN ITEMS WASAPI COREAUDIO PORTAUDIO PIPEWIRE SDL_AUDIO
		PULSEAUDIO ALSA OSS)
	if(XPDEV_WITH_${backend})
		string(REPLACE "_AUDIO" "" backend_name "${backend}")
		list(APPEND XPDEV_AUDIO_BACKENDS "${backend_name}")
	endif()
endforeach()
set(XPDEV_AUDIO_ENABLED FALSE)
if(XPDEV_AUDIO_BACKENDS)
	set(XPDEV_AUDIO_ENABLED TRUE)
endif()

set(XPDEV_HAS_INTTYPES_H ${HAS_INTTYPES_H})
set(XPDEV_HAS_STDINT_H ${HAS_STDINT_H})
set(XPDEV_LINK_LIST_THREADSAFE TRUE)
set(XPDEV_THREAD_SAFE_ENABLED TRUE)
set(XPDEV_USE_NATIVE_POSIX_SEMAPHORES FALSE)
set(XPDEV_USE_XP_SEMAPHORES FALSE)
set(XPDEV_PREFER_POLL FALSE)
if(NOT WIN32)
	set(XPDEV_PREFER_POLL TRUE)
endif()

# semwrap.h can use the platform's POSIX semaphore ABI only when the complete
# API it wraps is declared and linkable.  Darwin is deliberately excluded:
# it declares sem_init(), but Apple's implementation returns ENOSYS because
# unnamed POSIX semaphores are not supported.
set(XPDEV_COMPLETE_POSIX_SEMAPHORE_TEST [=[
#define _POSIX_C_SOURCE 200112L
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <time.h>
int main(void)
{
	sem_t sem;
	sem_t *named;
	struct timespec timeout = {0, 0};
	(void)sem_init(&sem, 0, 0);
	(void)sem_destroy(&sem);
	named = sem_open("/xpdev-cmake-probe", O_CREAT, 0600, 0);
	(void)sem_close(named);
	(void)sem_unlink("/xpdev-cmake-probe");
	(void)sem_wait(&sem);
	(void)sem_trywait(&sem);
	(void)sem_post(&sem);
	(void)sem_timedwait(&sem, &timeout);
	return 0;
}
]=])
if(UNIX AND NOT APPLE)
	set(XPDEV_SAVED_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
	set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})
	check_c_source_compiles("${XPDEV_COMPLETE_POSIX_SEMAPHORE_TEST}"
		XPDEV_HAVE_COMPLETE_POSIX_SEMAPHORE_API)
	if(NOT XPDEV_HAVE_COMPLETE_POSIX_SEMAPHORE_API)
		set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_THREAD_LIBS_INIT} rt)
		check_c_source_compiles("${XPDEV_COMPLETE_POSIX_SEMAPHORE_TEST}"
			XPDEV_HAVE_COMPLETE_POSIX_SEMAPHORE_API_WITH_RT)
		if(XPDEV_HAVE_COMPLETE_POSIX_SEMAPHORE_API_WITH_RT)
			set(XPDEV_HAVE_COMPLETE_POSIX_SEMAPHORE_API TRUE)
			set(XPDEV_POSIX_SEMAPHORE_LIBRARY rt)
		endif()
	endif()
	set(CMAKE_REQUIRED_LIBRARIES "${XPDEV_SAVED_CMAKE_REQUIRED_LIBRARIES}")
	unset(XPDEV_SAVED_CMAKE_REQUIRED_LIBRARIES)
endif()
unset(XPDEV_COMPLETE_POSIX_SEMAPHORE_TEST)

if(XPDEV_HAVE_COMPLETE_POSIX_SEMAPHORE_API AND NOT APPLE)
	set(XPDEV_USE_NATIVE_POSIX_SEMAPHORES TRUE)
elseif(UNIX)
	set(XPDEV_USE_XP_SEMAPHORES TRUE)
endif()
if(XPDEV_USE_XP_SEMAPHORES)
	# These are fallback implementation details rather than a portable API.
	list(APPEND SOURCE xpsem.c)
	list(APPEND HEADER xpsem.h)
endif()
set(XPDEV_NEEDS_STRLCPY FALSE)
if(NOT HAS_STRLCPY)
	set(XPDEV_NEEDS_STRLCPY TRUE)
endif()
set(XPDEV_NEEDS_ASPRINTF FALSE)
if(NOT HAS_ASPRINTF)
	set(XPDEV_NEEDS_ASPRINTF TRUE)
endif()
set(XPDEV_NEEDS_VASPRINTF FALSE)
if(NOT HAS_VASPRINTF)
	set(XPDEV_NEEDS_VASPRINTF TRUE)
endif()
set(XPDEV_NEEDS_GETDELIM FALSE)
if(NOT HAS_GETDELIM)
	set(XPDEV_NEEDS_GETDELIM TRUE)
endif()
set(XPDEV_NEEDS_STRNDUP FALSE)
if(NOT HAS_STRNDUP)
	set(XPDEV_NEEDS_STRNDUP TRUE)
endif()
set(XPDEV_NEEDS_STRCASESTR FALSE)
if(NOT HAS_STRCASESTR)
	set(XPDEV_NEEDS_STRCASESTR TRUE)
endif()
set(XPDEV_NEEDS_STRTOK_R FALSE)
if(NOT HAS_STRTOK_R)
	set(XPDEV_NEEDS_STRTOK_R TRUE)
endif()
set(XPDEV_STATIC_SDL FALSE)
if(XPDEV_WITH_SDL_AUDIO AND CMAKE_SYSTEM_NAME MATCHES "Darwin")
	set(XPDEV_STATIC_SDL TRUE)
endif()
set(XPDEV_WITHOUT_CRYPTLIB FALSE)
if(WITHOUT_CRYPTLIB)
	set(XPDEV_WITHOUT_CRYPTLIB TRUE)
endif()
set(XPDEV_ELF_SYMBOL_VERSIONING FALSE)
if(CMAKE_SYSTEM_NAME MATCHES "^(FreeBSD|GNU|Linux|NetBSD)$")
	set(XPDEV_ELF_SYMBOL_VERSIONING TRUE)
endif()

configure_file(cmake/xpdev_config.h.in xpdev_config.h @ONLY)
configure_file(cmake/xpdevFeatures.cmake.in xpdevFeatures.cmake @ONLY)

# Preserve the object target expected by Synchronet source-tree consumers.
add_library(xpdev OBJECT EXCLUDE_FROM_ALL ${SOURCE})
set_target_properties(xpdev PROPERTIES POSITION_INDEPENDENT_CODE ON)
xpdev_configure_target(xpdev)

if(NOT XPDEV_BUILD_INSTALLABLE_LIBRARIES)
	return()
endif()

add_library(xpdev_static_library STATIC ${SOURCE})
add_library(xpdev_shared_library SHARED ${SOURCE})
xpdev_configure_target(xpdev_static_library)
xpdev_configure_target(xpdev_shared_library)

set_target_properties(xpdev_static_library PROPERTIES
	EXPORT_NAME static
	OUTPUT_NAME xpdev
	POSITION_INDEPENDENT_CODE ON)
set_target_properties(xpdev_shared_library PROPERTIES
	C_VISIBILITY_PRESET hidden
	EXPORT_NAME shared
	OUTPUT_NAME xpdev
	SOVERSION ${PROJECT_VERSION_MAJOR}
	VERSION ${PROJECT_VERSION})
if(XPDEV_ELF_SYMBOL_VERSIONING)
	target_link_options(xpdev_shared_library PRIVATE
		"LINKER:--undefined-version"
		"LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/cmake/xpdev.map")
	set_property(TARGET xpdev_shared_library APPEND PROPERTY LINK_DEPENDS
		${CMAKE_CURRENT_SOURCE_DIR}/cmake/xpdev.map)
endif()
if(WIN32)
	set_target_properties(xpdev_static_library PROPERTIES OUTPUT_NAME xpdev_static)
	set_target_properties(xpdev_shared_library PROPERTIES
		OUTPUT_NAME xpdev-${PROJECT_VERSION_MAJOR})
	target_compile_definitions(xpdev_shared_library PRIVATE WRAPPER_EXPORTS)
	target_compile_definitions(xpdev_shared_library INTERFACE WRAPPER_IMPORTS)
endif()

add_library(xpdev_default INTERFACE)
target_link_libraries(xpdev_default INTERFACE xpdev_shared_library)
set_target_properties(xpdev_default PROPERTIES EXPORT_NAME xpdev)

add_library(xpdev::xpdev ALIAS xpdev_default)
add_library(xpdev::static ALIAS xpdev_static_library)
add_library(xpdev::shared ALIAS xpdev_shared_library)

function(xpdev_add_component component)
	cmake_parse_arguments(COMPONENT "" "" "SOURCES;HEADERS" ${ARGN})
	if(NOT COMPONENT_SOURCES OR NOT COMPONENT_HEADERS)
		message(FATAL_ERROR "xpdev_add_component(${component}) requires SOURCES and HEADERS")
	endif()

	set(object_target "${component}")
	set(static_target "xpdev_${component}_static_library")
	set(shared_target "xpdev_${component}_shared_library")
	set(default_target "xpdev_${component}_default")

	# Preserve the object targets used by Synchronet source-tree consumers.
	add_library(${object_target} OBJECT EXCLUDE_FROM_ALL ${COMPONENT_SOURCES})
	target_compile_features(${object_target} PUBLIC c_std_11)
	target_include_directories(${object_target} PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR})
	target_link_libraries(${object_target} PUBLIC xpdev)

	add_library(${static_target} STATIC ${COMPONENT_SOURCES})
	add_library(${shared_target} SHARED ${COMPONENT_SOURCES})
	foreach(target IN ITEMS ${static_target} ${shared_target})
		target_compile_features(${target} PUBLIC c_std_11)
		target_include_directories(${target} PUBLIC
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
	endforeach()
	target_link_libraries(${static_target} PUBLIC xpdev_static_library)
	target_link_libraries(${shared_target} PUBLIC xpdev_shared_library)

	set_target_properties(${static_target} PROPERTIES
		EXPORT_NAME "${component}_static"
		OUTPUT_NAME "xpdev-${component}"
		POSITION_INDEPENDENT_CODE ON)
	set_target_properties(${shared_target} PROPERTIES
		EXPORT_NAME "${component}_shared"
		OUTPUT_NAME "xpdev-${component}"
		SOVERSION ${PROJECT_VERSION_MAJOR}
		VERSION ${PROJECT_VERSION})

	if(XPDEV_ELF_SYMBOL_VERSIONING)
		set(version_script
			"${PROJECT_SOURCE_DIR}/cmake/xpdev-${component}.map")
		target_link_options(${shared_target} PRIVATE
			"LINKER:--version-script=${version_script}")
		set_property(TARGET ${shared_target} APPEND PROPERTY LINK_DEPENDS
			"${version_script}")
	elseif(APPLE)
		set(export_list
			"${PROJECT_SOURCE_DIR}/cmake/xpdev-${component}.exports")
		target_link_options(${shared_target} PRIVATE
			"LINKER:-exported_symbols_list,${export_list}")
		set_property(TARGET ${shared_target} APPEND PROPERTY LINK_DEPENDS
			"${export_list}")
	elseif(WIN32)
		target_sources(${shared_target} PRIVATE
			"${PROJECT_SOURCE_DIR}/cmake/xpdev-${component}.def")
		set_target_properties(${static_target} PROPERTIES
			OUTPUT_NAME "xpdev-${component}_static")
		set_target_properties(${shared_target} PROPERTIES
			OUTPUT_NAME "xpdev-${component}-${PROJECT_VERSION_MAJOR}")
	endif()

	add_library(${default_target} INTERFACE)
	target_link_libraries(${default_target} INTERFACE ${shared_target})
	set_target_properties(${default_target} PROPERTIES EXPORT_NAME ${component})
	add_library(xpdev::${component} ALIAS ${default_target})
	add_library(xpdev::${component}_static ALIAS ${static_target})
	add_library(xpdev::${component}_shared ALIAS ${shared_target})

	install(TARGETS ${static_target} ${shared_target} ${default_target}
		EXPORT xpdevTargets
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
	install(FILES ${COMPONENT_HEADERS}
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/xpdev/${component}")
endfunction()

add_subdirectory(comio)
add_subdirectory(hash)
add_subdirectory(encode)

set(XPDEV_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/xpdev"
	CACHE STRING "Directory for xpdev CMake package files")

if(UNIX OR WIN32)
	set(XPDEV_INSTALL_PKGCONFIGDIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
		CACHE STRING "Directory for xpdev pkg-config files")

	if(IS_ABSOLUTE "${XPDEV_INSTALL_PKGCONFIGDIR}")
		set(XPDEV_PC_PREFIX "${CMAKE_INSTALL_PREFIX}")
	else()
		file(RELATIVE_PATH XPDEV_PC_PREFIX_FROM_PCFILEDIR
			"${CMAKE_CURRENT_BINARY_DIR}/xpdev-pc-root/${XPDEV_INSTALL_PKGCONFIGDIR}"
			"${CMAKE_CURRENT_BINARY_DIR}/xpdev-pc-root")
		string(REGEX REPLACE "/$" "" XPDEV_PC_PREFIX_FROM_PCFILEDIR
			"${XPDEV_PC_PREFIX_FROM_PCFILEDIR}")
		set(XPDEV_PC_PREFIX
			"\${pcfiledir}/${XPDEV_PC_PREFIX_FROM_PCFILEDIR}")
	endif()
	if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
		set(XPDEV_PC_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
	else()
		set(XPDEV_PC_LIBDIR "\${prefix}/${CMAKE_INSTALL_LIBDIR}")
	endif()
	if(IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}")
		set(XPDEV_PC_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}")
	else()
		set(XPDEV_PC_INCLUDEDIR "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
	endif()

	set(XPDEV_PC_CFLAGS "-DXPDEV_USE_CONFIG_H")
	set(XPDEV_PC_PUBLIC_LIBS "")
	set(XPDEV_PC_PRIVATE_LIBS "")
	macro(xpdev_pc_append_public_library library)
		if(NOT "${library}" STREQUAL "")
			if("${library}" MATCHES "^-")
				string(APPEND XPDEV_PC_PUBLIC_LIBS " ${library}")
			else()
				string(APPEND XPDEV_PC_PUBLIC_LIBS " -l${library}")
			endif()
		endif()
	endmacro()
	macro(xpdev_pc_append_private_library library)
		if(NOT "${library}" STREQUAL "")
			if("${library}" MATCHES "^-")
				string(APPEND XPDEV_PC_PRIVATE_LIBS " ${library}")
			else()
				string(APPEND XPDEV_PC_PRIVATE_LIBS " -l${library}")
			endif()
		endif()
	endmacro()

	foreach(thread_flag IN LISTS CMAKE_THREAD_LIBS_INIT)
		xpdev_pc_append_public_library("${thread_flag}")
		if(thread_flag MATCHES "(^| )-pthread($| )")
			string(APPEND XPDEV_PC_CFLAGS " -pthread")
		endif()
	endforeach()
	foreach(library IN LISTS CMAKE_DL_LIBS)
		xpdev_pc_append_private_library("${library}")
	endforeach()
	if(XPDEV_POSIX_SEMAPHORE_LIBRARY)
		xpdev_pc_append_public_library("${XPDEV_POSIX_SEMAPHORE_LIBRARY}")
	endif()

	if(WIN32)
		string(APPEND XPDEV_PC_CFLAGS
			" -D_WIN32 -D_WIN32_WINNT=0x0501 -DWINVER=0x0501 -DMSVCRT_VERSION=0x0501 -D_WIN32_IE=0x0500")
		if(MSVC)
			string(APPEND XPDEV_PC_CFLAGS " /experimental:c11atomics")
		endif()
		foreach(library IN ITEMS iphlpapi ws2_32 winmm netapi32 ole32 uuid)
			xpdev_pc_append_private_library("${library}")
		endforeach()
	elseif(CMAKE_SYSTEM_NAME STREQUAL "Haiku")
		string(APPEND XPDEV_PC_CFLAGS " -D__unix__")
		xpdev_pc_append_private_library(m)
		xpdev_pc_append_private_library(network)
	elseif(CMAKE_SYSTEM_NAME MATCHES "^(NetBSD|OpenBSD)$")
		xpdev_pc_append_private_library(m)
		if(NOT WITHOUT_OSS)
			xpdev_pc_append_private_library(ossaudio)
		endif()
	elseif(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
		xpdev_pc_append_private_library(socket)
		xpdev_pc_append_private_library(m)
	elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
		string(APPEND XPDEV_PC_CFLAGS " -D__unix__ -D__DARWIN__")
		xpdev_pc_append_private_library(m)
		if(XPDEV_WITH_COREAUDIO)
			string(APPEND XPDEV_PC_PRIVATE_LIBS
				" -framework AudioToolbox")
		endif()
	elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
		string(APPEND XPDEV_PC_CFLAGS " -D__unix__")
		xpdev_pc_append_private_library(m)
	elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
		string(APPEND XPDEV_PC_CFLAGS
			" -D_DEFAULT_SOURCE -D_BSD_SOURCE -DSPEED_MACROS_ONLY -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64")
		xpdev_pc_append_private_library(m)
	else()
		xpdev_pc_append_private_library(m)
	endif()
	string(STRIP "${XPDEV_PC_PUBLIC_LIBS}" XPDEV_PC_PUBLIC_LIBS)
	string(STRIP "${XPDEV_PC_PRIVATE_LIBS}" XPDEV_PC_PRIVATE_LIBS)

	set(XPDEV_PC_REQUIRES "")
	if(XPDEV_WITH_SDL_AUDIO)
		set(XPDEV_PC_REQUIRES "sdl2")
	endif()
	string(REPLACE ";" "," XPDEV_PC_AUDIO_BACKENDS
		"${XPDEV_AUDIO_BACKENDS}")
	if(XPDEV_AUDIO_ENABLED)
		set(XPDEV_PC_AUDIO_ENABLED true)
	else()
		set(XPDEV_PC_AUDIO_ENABLED false)
	endif()
	set(XPDEV_PC_FILES "")
	if(WIN32)
		set(XPDEV_PC_NAME "XPDev")
		set(XPDEV_PC_LIBRARY "xpdev-${PROJECT_VERSION_MAJOR}")
		set(XPDEV_PC_LINK_LIBS "")
		if(XPDEV_PC_PUBLIC_LIBS)
			set(XPDEV_PC_LINK_LIBS " ${XPDEV_PC_PUBLIC_LIBS}")
		endif()
		set(XPDEV_PC_FILE_PRIVATE_LIBS "")
		set(XPDEV_PC_VARIANT_CFLAGS " -DWRAPPER_IMPORTS")
		configure_file(cmake/xpdev.pc.in xpdev.pc @ONLY)
		list(APPEND XPDEV_PC_FILES
			${CMAKE_CURRENT_BINARY_DIR}/xpdev.pc)

		set(XPDEV_PC_NAME "XPDev static")
		set(XPDEV_PC_LIBRARY "xpdev_static")
		string(STRIP
			"${XPDEV_PC_PUBLIC_LIBS} ${XPDEV_PC_PRIVATE_LIBS}"
			XPDEV_PC_LINK_LIBS)
		if(XPDEV_PC_LINK_LIBS)
			set(XPDEV_PC_LINK_LIBS " ${XPDEV_PC_LINK_LIBS}")
		endif()
		set(XPDEV_PC_FILE_PRIVATE_LIBS "")
		set(XPDEV_PC_VARIANT_CFLAGS "")
		configure_file(cmake/xpdev.pc.in xpdev-static.pc @ONLY)
		list(APPEND XPDEV_PC_FILES
			${CMAKE_CURRENT_BINARY_DIR}/xpdev-static.pc)
	else()
		set(XPDEV_PC_NAME "XPDev")
		set(XPDEV_PC_LIBRARY "xpdev")
		set(XPDEV_PC_LINK_LIBS "")
		if(XPDEV_PC_PUBLIC_LIBS)
			set(XPDEV_PC_LINK_LIBS " ${XPDEV_PC_PUBLIC_LIBS}")
		endif()
		set(XPDEV_PC_FILE_PRIVATE_LIBS "")
		if(XPDEV_PC_PRIVATE_LIBS)
			set(XPDEV_PC_FILE_PRIVATE_LIBS
				" ${XPDEV_PC_PRIVATE_LIBS}")
		endif()
		set(XPDEV_PC_VARIANT_CFLAGS "")
		configure_file(cmake/xpdev.pc.in xpdev.pc @ONLY)
		list(APPEND XPDEV_PC_FILES
			${CMAKE_CURRENT_BINARY_DIR}/xpdev.pc)
	endif()

	foreach(component IN ITEMS comio encode hash)
		set(XPDEV_PC_COMPONENT "${component}")
		if(component STREQUAL "comio")
			set(XPDEV_PC_COMPONENT_DESCRIPTION
				"XPDev serial communications library")
		elseif(component STREQUAL "encode")
			set(XPDEV_PC_COMPONENT_DESCRIPTION
				"XPDev data encoding library")
		else()
			set(XPDEV_PC_COMPONENT_DESCRIPTION
				"XPDev hash algorithm library")
		endif()
		set(XPDEV_PC_COMPONENT_NAME "XPDev ${component}")
		set(XPDEV_PC_COMPONENT_REQUIRES "xpdev = ${PROJECT_VERSION}")
		set(XPDEV_PC_COMPONENT_LIBRARY "xpdev-${component}")
		set(XPDEV_PC_COMPONENT_CFLAGS "")
		if(WIN32)
			set(XPDEV_PC_COMPONENT_LIBRARY
				"xpdev-${component}-${PROJECT_VERSION_MAJOR}")
			if(component STREQUAL "comio")
				set(XPDEV_PC_COMPONENT_CFLAGS "-DCOMIO_IMPORTS")
			elseif(component STREQUAL "encode")
				set(XPDEV_PC_COMPONENT_CFLAGS
					"-DB64_IMPORTS -DXPDEV_ENCODE_IMPORTS")
			else()
				set(XPDEV_PC_COMPONENT_CFLAGS
					"-DMD5_IMPORTS -DXPDEV_HASH_IMPORTS")
			endif()
		endif()
		configure_file(cmake/xpdev-component.pc.in
			"xpdev-${component}.pc" @ONLY)
		list(APPEND XPDEV_PC_FILES
			${CMAKE_CURRENT_BINARY_DIR}/xpdev-${component}.pc)

		if(WIN32)
			set(XPDEV_PC_COMPONENT_NAME "XPDev ${component} static")
			set(XPDEV_PC_COMPONENT_REQUIRES
				"xpdev-static = ${PROJECT_VERSION}")
			set(XPDEV_PC_COMPONENT_LIBRARY "xpdev-${component}_static")
			set(XPDEV_PC_COMPONENT_CFLAGS "")
			if(component STREQUAL "encode")
				set(XPDEV_PC_COMPONENT_CFLAGS "-DXPDEV_ENCODE_STATIC")
			elseif(component STREQUAL "hash")
				set(XPDEV_PC_COMPONENT_CFLAGS "-DXPDEV_HASH_STATIC")
			endif()
			configure_file(cmake/xpdev-component.pc.in
				"xpdev-${component}-static.pc" @ONLY)
			list(APPEND XPDEV_PC_FILES
				${CMAKE_CURRENT_BINARY_DIR}/xpdev-${component}-static.pc)
		endif()
	endforeach()
endif()

set(XPDEV_CONFIG_NEEDS_SDL2 FALSE)
if(XPDEV_WITH_SDL_AUDIO)
	set(XPDEV_CONFIG_NEEDS_SDL2 TRUE)
endif()
configure_package_config_file(
	cmake/xpdevConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/xpdevConfig.cmake
	INSTALL_DESTINATION ${XPDEV_INSTALL_CMAKEDIR})
write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/xpdevConfigVersion.cmake
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY SameMajorVersion)

install(TARGETS xpdev_static_library xpdev_shared_library xpdev_default
	EXPORT xpdevTargets
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${HEADER} ${CMAKE_CURRENT_BINARY_DIR}/xpdev_config.h
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/xpdev)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
	set(XPDEV_LICENSE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../LICENSE")
	set(XPDEV_LICENSE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../LICENSE")
else()
	message(FATAL_ERROR "Unable to locate the xpdev license file")
endif()
install(FILES "${XPDEV_LICENSE_FILE}"
	DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/xpdev)
install(EXPORT xpdevTargets
	FILE xpdevTargets.cmake
	NAMESPACE xpdev::
	DESTINATION ${XPDEV_INSTALL_CMAKEDIR})
install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/xpdevConfig.cmake
	${CMAKE_CURRENT_BINARY_DIR}/xpdevConfigVersion.cmake
	${CMAKE_CURRENT_BINARY_DIR}/xpdevFeatures.cmake
	DESTINATION ${XPDEV_INSTALL_CMAKEDIR})
if(XPDEV_PC_FILES)
	install(FILES ${XPDEV_PC_FILES}
		DESTINATION ${XPDEV_INSTALL_PKGCONFIGDIR})
endif()
