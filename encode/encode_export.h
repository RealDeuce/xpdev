#ifndef XPDEV_ENCODE_EXPORT_H
#define XPDEV_ENCODE_EXPORT_H

#if defined(_WIN32) && !defined(XPDEV_ENCODE_STATIC)
	#if defined(XPDEV_ENCODE_EXPORTS)
		#define XPDEV_ENCODE_EXPORT __declspec(dllexport)
	#else
		#define XPDEV_ENCODE_EXPORT __declspec(dllimport)
	#endif
#else
	#define XPDEV_ENCODE_EXPORT
#endif

#endif
