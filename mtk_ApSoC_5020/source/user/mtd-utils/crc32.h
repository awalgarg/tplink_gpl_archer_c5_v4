#ifndef CRC32_H
#define CRC32_H

/* $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/mtd-utils/crc32.h#1 $ */

#include <linux/types.h>

extern const __u32 crc32_table[256];

/* Return a 32-bit CRC of the contents of the buffer. */

static inline __u32 
crc32(__u32 val, const void *ss, int len)
{
	const unsigned char *s = ss;
        while (--len >= 0)
                val = crc32_table[(val ^ *s++) & 0xff] ^ (val >> 8);
        return val;
}

#endif
