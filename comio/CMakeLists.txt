set(COMIO_SOURCE
	comio.c
)

set(COMIO_HEADER
	comio.h
)

if(WIN32)
	list(APPEND COMIO_SOURCE comio_win32.c)
else()
	list(APPEND COMIO_SOURCE comio_nix.c)
endif()

xpdev_add_component(comio SOURCES ${COMIO_SOURCE} HEADERS ${COMIO_HEADER})
target_compile_definitions(xpdev_comio_shared_library PRIVATE COMIO_EXPORTS)
target_compile_definitions(xpdev_comio_shared_library INTERFACE COMIO_IMPORTS)
