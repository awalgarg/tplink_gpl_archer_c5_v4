/* Image manipulation functions
 *
 * Project : minidlna
 * Website : http://sourceforge.net/projects/minidlna/
 * Author  : Justin Maggard
 *
 * MiniDLNA media server
 * Copyright (C) 2009  Justin Maggard
 *
 * This file is part of MiniDLNA.
 *
 * MiniDLNA is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * MiniDLNA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MiniDLNA. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __IMAGE_UTILS_H__
#define __IMAGE_UTILS_H__

#include <inttypes.h>

#define ROTATE_NONE 0x0
#define ROTATE_90   0x1
#define ROTATE_180  0x2
#define ROTATE_270  0x4

/* 
 * brief	Optimize memory use when make albumart. Add by zengdongbiao, 25May15.
 */
#ifndef MEMORY_OPTIMIZE_FOR_ALBUMART
#define	MEMORY_OPTIMIZE_FOR_ALBUMART
#endif

typedef uint32_t pix;

typedef struct {
	int32_t width;
	int32_t height;
	pix     *buf;
} image_s;

#ifndef LITE_MINIDLNA

void
image_free(image_s *pimage);

int
image_get_jpeg_date_xmp(const char * path, char ** date);

int
image_get_jpeg_resolution(const char * path, int * width, int * height);

image_s *
image_new_from_jpeg(const char *path, int is_file, const uint8_t *ptr, int size, int scale, int resize);

image_s *
image_resize(image_s * src_image, int32_t width, int32_t height);

unsigned char *
image_save_to_jpeg_buf(image_s * pimage, int * size);

char *
image_save_to_jpeg_file(image_s * pimage, char * path);

#ifdef MEMORY_OPTIMIZE_FOR_ALBUMART
/* 
 * fn		int image_getJpegSize(const char *path, int is_file, const uint8_t *buf, int size, int *width, int *height)
 * brief	Get jpeg image size(width and height).
 * details	
 *
 * param[in]	path		file path.
 * param[in]	is_file		image is file format or in buf.
 * param[in]	buf			image buf.
 * param[in]	size		size of	image buf.
 * param[out]	width		image width.
 * param[out]	height		image height.
 *
 * return	
 * retval	0	get size successfully.
 *			-1	error.
 * note		Written by zengdongbiao, 25May15.
 */
int image_getJpegSize(const char *path, int is_file, const uint8_t *buf, int size, int *width, int *height);
#endif /* MEMORY_OPTIMIZE_FOR_ALBUMART */

#endif /* #ifndef LITE_MINIDLNA */

#endif /* __IMAGE_UTILS_H__ */