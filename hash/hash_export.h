#ifndef XPDEV_HASH_EXPORT_H
#define XPDEV_HASH_EXPORT_H

#if defined(_WIN32) && !defined(XPDEV_HASH_STATIC)
	#if defined(XPDEV_HASH_EXPORTS)
		#define XPDEV_HASH_EXPORT __declspec(dllexport)
	#else
		#define XPDEV_HASH_EXPORT __declspec(dllimport)
	#endif
#else
	#define XPDEV_HASH_EXPORT
#endif

#endif
