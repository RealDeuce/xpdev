cmake_minimum_required(VERSION 3.16)

project(xpdev_package_consumer LANGUAGES C CXX)

set(xpdev_required_components comio encode hash)
if(XPDEV_CONSUMER_EXPECT_AUDIO_BACKENDS)
	list(APPEND xpdev_required_components audio)
endif()
find_package(xpdev CONFIG REQUIRED COMPONENTS ${xpdev_required_components})

set(xpdev_known_audio_backends
	WASAPI COREAUDIO PORTAUDIO PIPEWIRE SDL PULSEAUDIO ALSA OSS)
if(XPDEV_CONSUMER_EXPECT_AUDIO_BACKENDS)
	string(REPLACE "," ";" xpdev_expected_audio_backends
		"${XPDEV_CONSUMER_EXPECT_AUDIO_BACKENDS}")
	set(xpdev_actual_audio_backends ${xpdev_AUDIO_BACKENDS})
	list(SORT xpdev_expected_audio_backends)
	list(SORT xpdev_actual_audio_backends)
	if(NOT xpdev_actual_audio_backends STREQUAL xpdev_expected_audio_backends)
		message(FATAL_ERROR
			"Expected xpdev audio backends ${xpdev_expected_audio_backends}; "
			"package reports ${xpdev_actual_audio_backends}")
	endif()
endif()

# Compile every installed public header as the first and only include in both
# C and C++.  Discovering the files from the installed tree keeps this check in
# sync when the package gains another public header.  xpevent.h is the Unix
# implementation detail included by eventwrap.h and intentionally rejects
# direct inclusion.
get_target_property(xpdev_include_dirs xpdev::shared INTERFACE_INCLUDE_DIRECTORIES)
foreach(include_dir IN LISTS xpdev_include_dirs)
	if(IS_DIRECTORY "${include_dir}/xpdev")
		set(xpdev_include_root "${include_dir}")
		break()
	endif()
endforeach()
if(NOT xpdev_include_root)
	message(FATAL_ERROR "Could not locate XPDev's installed include directory")
endif()

# Keep the generated public header and CMake package metadata in agreement.
file(READ "${xpdev_include_root}/xpdev/xpdev_config.h" xpdev_config_header)

function(xpdev_check_boolean_metadata cmake_variable header_macro
		expected_variable)
	set(expected_value 0)
	if(${cmake_variable})
		set(expected_value 1)
	endif()
	if(NOT xpdev_config_header MATCHES
			"#define ${header_macro} ${expected_value}([\r\n]|$)")
		message(FATAL_ERROR
			"${cmake_variable} disagrees with ${header_macro} in xpdev_config.h")
	endif()
	set(${expected_variable} ${expected_value} PARENT_SCOPE)
endfunction()

xpdev_check_boolean_metadata(xpdev_LINK_LIST_THREADSAFE
	XPDEV_LINK_LIST_THREADSAFE xpdev_expected_link_list_threadsafe)
xpdev_check_boolean_metadata(xpdev_THREAD_SAFE
	XPDEV_THREAD_SAFE_ENABLED xpdev_expected_thread_safe)
xpdev_check_boolean_metadata(xpdev_USE_SYSTEM_PTHREADS
	XPDEV_USE_SYSTEM_PTHREADS xpdev_expected_system_pthreads)
xpdev_check_boolean_metadata(xpdev_USE_NATIVE_POSIX_SEMAPHORES
	XPDEV_USE_NATIVE_POSIX_SEMAPHORES xpdev_expected_native_posix_semaphores)
xpdev_check_boolean_metadata(xpdev_USE_XP_SEMAPHORES
	XPDEV_USE_XP_SEMAPHORES xpdev_expected_xp_semaphores)

set(xpdev_installed_xpsem_header "${xpdev_include_root}/xpdev/xpsem.h")
if(xpdev_USE_XP_SEMAPHORES)
	if(NOT EXISTS "${xpdev_installed_xpsem_header}")
		message(FATAL_ERROR "The XP semaphore fallback header was not installed")
	endif()
	set(xpdev_opaque_sem_source
		"${CMAKE_CURRENT_BINARY_DIR}/opaque_xp_sem.c")
	file(WRITE "${xpdev_opaque_sem_source}"
		"#include <xpdev/xpsem.h>\nint value = sizeof(struct xp_sem);\n")
	try_compile(xpdev_exposed_xp_sem
		"${CMAKE_CURRENT_BINARY_DIR}/opaque_xp_sem_build"
		"${xpdev_opaque_sem_source}"
		CMAKE_FLAGS "-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY"
		LINK_LIBRARIES xpdev::shared)
	if(xpdev_exposed_xp_sem)
		message(FATAL_ERROR
			"xpsem.h exposes the implementation of struct xp_sem")
	endif()
elseif(EXISTS "${xpdev_installed_xpsem_header}")
	message(FATAL_ERROR
		"xpsem.h was installed even though the XP semaphore fallback is disabled")
endif()

if(xpdev_USE_NATIVE_POSIX_SEMAPHORES AND xpdev_USE_XP_SEMAPHORES)
	message(FATAL_ERROR
		"xpdev cannot use native POSIX and XPDev replacement semaphores together")
endif()
if(XPDEV_CONSUMER_REQUIRE_NATIVE_POSIX_SEMAPHORES
		AND NOT xpdev_USE_NATIVE_POSIX_SEMAPHORES)
	message(FATAL_ERROR "The installed xpdev package does not use native POSIX semaphores")
endif()
if(XPDEV_CONSUMER_REQUIRE_XP_SEMAPHORES AND NOT xpdev_USE_XP_SEMAPHORES)
	message(FATAL_ERROR "The installed xpdev package does not use XPDev semaphores")
endif()

# Reject attempts to change the semaphore type selected when the library was
# built.  Defining the legacy switch is a conflict for native builds; forcing
# it to zero is a conflict for XP semaphore builds.
set(conflicting_semaphore_source
	"${CMAKE_CURRENT_BINARY_DIR}/conflicting_xp_semaphores.c")
file(WRITE "${conflicting_semaphore_source}"
	"#include <xpdev/semwrap.h>\nint main(void) { return 0; }\n")
if(xpdev_USE_XP_SEMAPHORES)
	set(conflicting_semaphore_definition -DUSE_XP_SEMAPHORES=0)
else()
	set(conflicting_semaphore_definition -DUSE_XP_SEMAPHORES)
endif()
try_compile(xpdev_accepted_conflicting_xp_semaphores
	"${CMAKE_CURRENT_BINARY_DIR}/conflicting_xp_semaphores_build"
	"${conflicting_semaphore_source}"
	CMAKE_FLAGS "-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY"
	COMPILE_DEFINITIONS ${conflicting_semaphore_definition}
	LINK_LIBRARIES xpdev::shared
	OUTPUT_VARIABLE conflicting_semaphore_output)
if(xpdev_accepted_conflicting_xp_semaphores)
	message(FATAL_ERROR
		"xpdev_config.h accepted an ABI-incompatible USE_XP_SEMAPHORES")
endif()
if(NOT conflicting_semaphore_output MATCHES
		"USE_XP_SEMAPHORES conflicts with this XPDev library's ABI")
	message(FATAL_ERROR
		"The conflicting USE_XP_SEMAPHORES probe failed for an unexpected reason:\n"
		"${conflicting_semaphore_output}")
endif()
message(STATUS
	"Verified rejection of an ABI-incompatible USE_XP_SEMAPHORES")

foreach(backend IN LISTS xpdev_known_audio_backends)
	set(cmake_backend ${backend})
	if(backend STREQUAL "SDL")
		set(cmake_backend SDL_AUDIO)
	endif()
	set(expected_value 0)
	if(backend IN_LIST xpdev_AUDIO_BACKENDS)
		set(expected_value 1)
	endif()
	if(xpdev_WITH_${cmake_backend})
		set(metadata_value 1)
	else()
		set(metadata_value 0)
	endif()
	if(NOT metadata_value EQUAL expected_value)
		message(FATAL_ERROR
			"xpdev_WITH_${cmake_backend} disagrees with xpdev_AUDIO_BACKENDS")
	endif()
	if(NOT xpdev_config_header MATCHES
			"#define XPDEV_WITH_${cmake_backend} ${expected_value}([\r\n]|$)")
		message(FATAL_ERROR
			"xpdev_config.h disagrees about XPDEV_WITH_${cmake_backend}")
	endif()
endforeach()

file(GLOB_RECURSE xpdev_public_headers
	RELATIVE "${xpdev_include_root}"
	"${xpdev_include_root}/xpdev/*.h")
list(REMOVE_ITEM xpdev_public_headers xpdev/xpevent.h)

foreach(header IN LISTS xpdev_public_headers)
	string(MAKE_C_IDENTIFIER "${header}" header_id)
	foreach(language IN ITEMS c cpp)
		set(source "${CMAKE_CURRENT_BINARY_DIR}/header_${header_id}.${language}")
		file(WRITE "${source}" "#include <${header}>\n")
		add_library(header_${header_id}_${language} OBJECT "${source}")
		target_link_libraries(header_${header_id}_${language} PRIVATE
			xpdev::shared
			xpdev::comio_shared
			xpdev::encode_shared
			xpdev::hash_shared)
	endforeach()
endforeach()

enable_testing()

add_executable(xpdev_cpp_atomic_consumer consumer_cpp.cpp)
target_link_libraries(xpdev_cpp_atomic_consumer PRIVATE xpdev::shared)
add_test(NAME xpdev_cpp_atomic_consumer COMMAND xpdev_cpp_atomic_consumer)

if(WIN32)
	set(xpdev_opaque_rwlock_source
		"${CMAKE_CURRENT_BINARY_DIR}/opaque_rwlock.c")
	file(WRITE "${xpdev_opaque_rwlock_source}"
		"#include <xpdev/rwlockwrap.h>\nint value = sizeof(*((rwlock_t)0));\n")
	try_compile(xpdev_exposed_rwlock
		"${CMAKE_CURRENT_BINARY_DIR}/opaque_rwlock_build"
		"${xpdev_opaque_rwlock_source}"
		CMAKE_FLAGS "-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY"
		LINK_LIBRARIES xpdev::shared)
	if(xpdev_exposed_rwlock)
		message(FATAL_ERROR
			"rwlockwrap.h exposes the Windows rwlock implementation")
	endif()

	add_executable(xpdev_windows_wrapper_consumer consumer_windows.c)
	target_link_libraries(xpdev_windows_wrapper_consumer PRIVATE xpdev::shared)
	add_test(NAME xpdev_windows_wrapper_consumer
		COMMAND xpdev_windows_wrapper_consumer)
elseif(UNIX)
	set(xpdev_opaque_event_source
		"${CMAKE_CURRENT_BINARY_DIR}/opaque_event.c")
	file(WRITE "${xpdev_opaque_event_source}"
		"#include <xpdev/eventwrap.h>\nint value = sizeof(*((xpevent_t)0));\n")
	try_compile(xpdev_exposed_event
		"${CMAKE_CURRENT_BINARY_DIR}/opaque_event_build"
		"${xpdev_opaque_event_source}"
		CMAKE_FLAGS "-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY"
		LINK_LIBRARIES xpdev::shared)
	if(xpdev_exposed_event)
		message(FATAL_ERROR
			"eventwrap.h exposes the Unix xpevent implementation")
	endif()

	add_executable(xpdev_event_consumer consumer_event.c)
	target_link_libraries(xpdev_event_consumer PRIVATE xpdev::shared)
	add_test(NAME xpdev_event_consumer COMMAND xpdev_event_consumer)
endif()

foreach(linkage IN ITEMS shared static)
	add_executable(xpdev_${linkage}_consumer consumer.c)
	target_compile_definitions(xpdev_${linkage}_consumer PRIVATE
		XPDEV_CONSUMER_EXPECT_LINK_LIST_THREADSAFE=${xpdev_expected_link_list_threadsafe}
		XPDEV_CONSUMER_EXPECT_THREAD_SAFE=${xpdev_expected_thread_safe}
		XPDEV_CONSUMER_EXPECT_SYSTEM_PTHREADS=${xpdev_expected_system_pthreads}
		XPDEV_CONSUMER_EXPECT_NATIVE_POSIX_SEMAPHORES=${xpdev_expected_native_posix_semaphores}
		XPDEV_CONSUMER_EXPECT_XP_SEMAPHORES=${xpdev_expected_xp_semaphores})
	target_link_libraries(xpdev_${linkage}_consumer PRIVATE
		xpdev::comio_${linkage}
		xpdev::encode_${linkage}
		xpdev::hash_${linkage})
	add_test(NAME xpdev_${linkage}_consumer
		COMMAND xpdev_${linkage}_consumer)
endforeach()
