set(HASH_SOURCE
	crc16.c
	crc32.c
	fnv1a.c
	md5.c
	sha1.c
	sha256.c
)

set(HASH_HEADER
	crc16.h
	crc32.h
	fnv1a.h
	hash_export.h
	md5.h
	sha1.h
	sha256.h
)

xpdev_add_component(hash SOURCES ${HASH_SOURCE} HEADERS ${HASH_HEADER})
target_compile_definitions(xpdev_hash_shared_library PRIVATE
	MD5_EXPORTS XPDEV_HASH_EXPORTS)
target_compile_definitions(xpdev_hash_shared_library INTERFACE
	MD5_IMPORTS XPDEV_HASH_IMPORTS)
target_compile_definitions(hash PRIVATE XPDEV_HASH_STATIC)
target_compile_definitions(xpdev_hash_static_library PUBLIC XPDEV_HASH_STATIC)
