/* Album art extraction, caching, and scaling
 *
 * Project : minidlna
 * Website : http://sourceforge.net/projects/minidlna/
 * Author  : Justin Maggard
 *
 * MiniDLNA media server
 * Copyright (C) 2008  Justin Maggard
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
#ifndef __ALBUMART_H__
#define __ALBUMART_H__

#ifdef USE_ALBUM_RESPONSE
#define EMBEDDED_ART	1
#define ALBUM_FILE		2
#endif /* USE_ALBUM_RESPONSE */

#ifndef LITE_MINIDLNA
void update_if_album_art(const char *path);
int64_t find_album_art(const char *path, uint8_t *image_data, int image_size);

#ifdef USE_ALBUM_RESPONSE
char *get_embedded_art(uint8_t *image_data, int width, int height, int *pBufSize);
char *get_album_file(const char *path, int width, int height, int *pBufSize, int *pAlbumType);
#endif /* USE_ALBUM_RESPONSE */

#endif /* LITE_MINIDLNA */

#endif
