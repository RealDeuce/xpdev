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

foreach(linkage IN ITEMS shared static)
	add_executable(xpdev_${linkage}_consumer consumer.c)
	target_link_libraries(xpdev_${linkage}_consumer PRIVATE
		xpdev::comio_${linkage}
		xpdev::encode_${linkage}
		xpdev::hash_${linkage})
	add_test(NAME xpdev_${linkage}_consumer
		COMMAND xpdev_${linkage}_consumer)
endforeach()
