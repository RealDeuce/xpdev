/* CCITT 16-bit CRC table and calculation macro */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright Rob Swindell - http://www.synchro.net/copyright.html			*
 *																			*
 * This library is free software; you can redistribute it and/or			*
 * modify it under the terms of the GNU Lesser General Public License		*
 * as published by the Free Software Foundation; either version 2			*
 * of the License, or (at your option) any later version.					*
 * See the GNU Lesser General Public License for more details: lgpl.txt or	*
 * http://www.fsf.org/copyleft/lesser.html									*
 *																			*
 * For Synchronet coding style and modification guidelines, see				*
 * http://www.synchro.net/source.html										*
 *																			*
 * Note: If this box doesn't appear square, then you need to fix your tabs.	*
 ****************************************************************************/

#ifndef _CRC16_H_
#define _CRC16_H_

#include <stddef.h>
#include <stdint.h>
#include "hash_export.h"

#ifdef __cplusplus
extern "C" {
#endif

extern XPDEV_HASH_EXPORT uint16_t crc16tbl[];

XPDEV_HASH_EXPORT uint16_t crc16(const char* data, size_t len);
XPDEV_HASH_EXPORT uint16_t icrc16(uint16_t crc, const char* data, size_t len);

#ifdef __cplusplus
}
#endif

#define ucrc16(ch, crc) (crc16tbl[((crc >> 8) & 0xff) ^ (unsigned char)ch] ^ (crc << 8))

#endif  /* Don't add anything after this line */
