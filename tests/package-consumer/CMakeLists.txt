cmake_minimum_required(VERSION 3.16)

project(xpdev_package_consumer LANGUAGES C CXX)

find_package(xpdev CONFIG REQUIRED COMPONENTS comio encode hash)

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
