cmake_minimum_required(VERSION 3.16)

project(xpdev_package_consumer LANGUAGES C)

find_package(xpdev CONFIG REQUIRED COMPONENTS comio encode hash)

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
