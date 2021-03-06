/*
 * ProFTPD - FTP server daemon
 * Copyright (c) 2006 The ProFTPD Project team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA.
 *
 * As a special exemption, The ProFTPD Project team and other respective
 * copyright holders give permission to link this program with OpenSSL, and
 * distribute the resulting executable, without including the source code for
 * OpenSSL in the source distribution.
 */

/* UTF8 encoding/decoding
 * $Id: //WIFI_SOC/MP/SDK_5_0_0_0/RT288x_SDK/source/user/proftpd/include/utf8.h#1 $
 */

#ifndef PR_UTF8_H
#define PR_UTF8_H

/*
 */
char *pr_utf8_decode(pool *p, const char *in, size_t inlen, size_t *outlen);

/*
 */
char *pr_utf8_encode(pool *p, const char *in, size_t inlen, size_t *outlen);

/* Internal use only. */
int utf8_init(void);
int utf8_free(void);

#endif /* PR_UTF8_H */
