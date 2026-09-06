set(ENCODE_SOURCE
	base64.c
	hex.c
	lzh.c
	utf8.c
)

set(ENCODE_HEADER
	base64.h
	encode_export.h
	hex.h
	lzh.h
	utf8.h
)

xpdev_add_component(encode SOURCES ${ENCODE_SOURCE} HEADERS ${ENCODE_HEADER})
target_compile_definitions(xpdev_encode_shared_library PRIVATE
	B64_EXPORTS LZH_EXPORTS XPDEV_ENCODE_EXPORTS)
target_compile_definitions(xpdev_encode_shared_library INTERFACE
	B64_IMPORTS XPDEV_ENCODE_IMPORTS)
target_compile_definitions(encode PRIVATE XPDEV_ENCODE_STATIC)
target_compile_definitions(xpdev_encode_static_library PUBLIC
	XPDEV_ENCODE_STATIC)
