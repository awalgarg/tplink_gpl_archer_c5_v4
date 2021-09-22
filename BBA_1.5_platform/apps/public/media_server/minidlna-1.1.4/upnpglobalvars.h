/* MiniDLNA project
 *
 * http://sourceforge.net/projects/minidlna/
 *
 * MiniDLNA media server
 * Copyright (C) 2008-2009  Justin Maggard
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
 *
 * Portions of the code from the MiniUPnP project:
 *
 * Copyright (c) 2006-2007, Thomas Bernard
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The name of the author may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __UPNPGLOBALVARS_H__
#define __UPNPGLOBALVARS_H__

#include <time.h>

#include <pthread.h>

#include "minidlnatypes.h"
#include "clients.h"
#include "config.h"

/* Use sqlite amalgamation, modify by zengdongbiao, 20May15. */
//#include <sqlite3.h>
#include "sqlite3.h"
/* End modify by zengdongbiao */

#define MINIDLNA_VERSION "1.1.4"

#ifdef NETGEAR
# define SERVER_NAME "ReadyDLNA"
#else
# define SERVER_NAME "MiniDLNA"
#endif

#define USE_FORK 1
#ifndef USE_ALBUM_RESPONSE
#define DB_VERSION 9
#else /* USE_ALBUM_RESPONSE */
#define DB_VERSION (9 + 1)
#endif /* USE_ALBUM_RESPONSE */

/*added by LY, in 2014.06.19*/
#define USE_SCANNER_THREAD	1
/*end added by LY*/

#ifdef ENABLE_NLS
#define _(string) gettext(string)
#else
#define _(string) (string)
#endif
#define THISORNUL(s) (s ? s : "")

#ifndef PNPX
#define PNPX 0
#endif

#define RESOURCE_PROTOCOL_INFO_VALUES \
	"http-get:*:image/jpeg:DLNA.ORG_PN=JPEG_SM_ICO," \
	"http-get:*:image/jpeg:DLNA.ORG_PN=JPEG_LRG_ICO," \
	"http-get:*:image/jpeg:DLNA.ORG_PN=JPEG_TN," \
	"http-get:*:image/jpeg:DLNA.ORG_PN=JPEG_SM," \
	"http-get:*:image/jpeg:DLNA.ORG_PN=JPEG_MED," \
	"http-get:*:image/jpeg:DLNA.ORG_PN=JPEG_LRG," \
	"http-get:*:image/png:DLNA.ORG_PN=PNG_SM_ICO," \
	"http-get:*:image/png:DLNA.ORG_PN=PNG_LRG_ICO," \
	"http-get:*:image/png:DLNA.ORG_PN=PNG_TN," \
	"http-get:*:image/png:DLNA.ORG_PN=PNG_LRG," \
	"http-get:*:video/mpeg:DLNA.ORG_PN=AVC_TS_HD_50_AC3_ISO," \
	"http-get:*:video/mpeg:DLNA.ORG_PN=AVC_TS_HD_60_AC3_ISO," \
	"http-get:*:video/mpeg:DLNA.ORG_PN=AVC_TS_HP_HD_AC3_ISO," \
	"http-get:*:video/mpeg:DLNA.ORG_PN=AVC_TS_MP_HD_AAC_MULT5_ISO," \
	"http-get:*:video/mpeg:DLNA.ORG_PN=AVC_TS_MP_HD_AC3_ISO," \
	"http-get:*:video/mpeg:DLNA.ORG_PN=AVC_TS_MP_HD_MPEG1_L3_ISO," \
	"http-get:*:video/mpeg:DLNA.ORG_PN=AVC_TS_MP_SD_AAC_MULT5_ISO," \
	"http-get:*:video/mpeg:DLNA.ORG_PN=AVC_TS_MP_SD_AC3_ISO," \
	"http-get:*:video/mpeg:DLNA.ORG_PN=AVC_TS_MP_SD_MPEG1_L3_ISO," \
	"http-get:*:video/mpeg:DLNA.ORG_PN=MPEG_PS_NTSC," \
	"http-get:*:video/mpeg:DLNA.ORG_PN=MPEG_PS_PAL," \
	"http-get:*:video/mpeg:DLNA.ORG_PN=MPEG_TS_HD_NA_ISO," \
	"http-get:*:video/mpeg:DLNA.ORG_PN=MPEG_TS_SD_NA_ISO," \
	"http-get:*:video/mpeg:DLNA.ORG_PN=MPEG_TS_SD_EU_ISO," \
	"http-get:*:video/mpeg:DLNA.ORG_PN=MPEG1," \
	"http-get:*:video/mp4:DLNA.ORG_PN=AVC_MP4_MP_SD_AAC_MULT5," \
	"http-get:*:video/mp4:DLNA.ORG_PN=AVC_MP4_MP_SD_AC3," \
	"http-get:*:video/mp4:DLNA.ORG_PN=AVC_MP4_BL_CIF15_AAC_520," \
	"http-get:*:video/mp4:DLNA.ORG_PN=AVC_MP4_BL_CIF30_AAC_940," \
	"http-get:*:video/mp4:DLNA.ORG_PN=AVC_MP4_BL_L31_HD_AAC," \
	"http-get:*:video/mp4:DLNA.ORG_PN=AVC_MP4_BL_L32_HD_AAC," \
	"http-get:*:video/mp4:DLNA.ORG_PN=AVC_MP4_BL_L3L_SD_AAC," \
	"http-get:*:video/mp4:DLNA.ORG_PN=AVC_MP4_HP_HD_AAC," \
	"http-get:*:video/mp4:DLNA.ORG_PN=AVC_MP4_MP_HD_1080i_AAC," \
	"http-get:*:video/mp4:DLNA.ORG_PN=AVC_MP4_MP_HD_720p_AAC," \
	"http-get:*:video/mp4:DLNA.ORG_PN=MPEG4_P2_MP4_ASP_AAC," \
	"http-get:*:video/mp4:DLNA.ORG_PN=MPEG4_P2_MP4_SP_VGA_AAC," \
	"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_HD_50_AC3," \
	"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_HD_50_AC3_T," \
	"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_HD_60_AC3," \
	"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_HD_60_AC3_T," \
	"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_HP_HD_AC3_T," \
	"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_MP_HD_AAC_MULT5," \
	"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_MP_HD_AAC_MULT5_T," \
	"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_MP_HD_AC3," \
	"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_MP_HD_AC3_T," \
	"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_MP_HD_MPEG1_L3," \
	"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_MP_HD_MPEG1_L3_T," \
	"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_MP_SD_AAC_MULT5," \
	"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_MP_SD_AAC_MULT5_T," \
	"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_MP_SD_AC3," \
	"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_MP_SD_AC3_T," \
	"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_MP_SD_MPEG1_L3," \
	"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_MP_SD_MPEG1_L3_T," \
	"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_HD_NA," \
	"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_HD_NA_T," \
	"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_SD_EU," \
	"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_SD_EU_T," \
	"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_SD_NA," \
	"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_SD_NA_T," \
	"http-get:*:video/x-ms-wmv:DLNA.ORG_PN=WMVSPLL_BASE," \
	"http-get:*:video/x-ms-wmv:DLNA.ORG_PN=WMVSPML_BASE," \
	"http-get:*:video/x-ms-wmv:DLNA.ORG_PN=WMVSPML_MP3," \
	"http-get:*:video/x-ms-wmv:DLNA.ORG_PN=WMVMED_BASE," \
	"http-get:*:video/x-ms-wmv:DLNA.ORG_PN=WMVMED_FULL," \
	"http-get:*:video/x-ms-wmv:DLNA.ORG_PN=WMVMED_PRO," \
	"http-get:*:video/x-ms-wmv:DLNA.ORG_PN=WMVHIGH_FULL," \
	"http-get:*:video/x-ms-wmv:DLNA.ORG_PN=WMVHIGH_PRO," \
	"http-get:*:video/3gpp:DLNA.ORG_PN=MPEG4_P2_3GPP_SP_L0B_AAC," \
	"http-get:*:video/3gpp:DLNA.ORG_PN=MPEG4_P2_3GPP_SP_L0B_AMR," \
	"http-get:*:audio/mpeg:DLNA.ORG_PN=MP3," \
	"http-get:*:audio/x-ms-wma:DLNA.ORG_PN=WMABASE," \
	"http-get:*:audio/x-ms-wma:DLNA.ORG_PN=WMAFULL," \
	"http-get:*:audio/x-ms-wma:DLNA.ORG_PN=WMAPRO," \
	"http-get:*:audio/x-ms-wma:DLNA.ORG_PN=WMALSL," \
	"http-get:*:audio/x-ms-wma:DLNA.ORG_PN=WMALSL_MULT5," \
	"http-get:*:audio/mp4:DLNA.ORG_PN=AAC_ISO_320," \
	"http-get:*:audio/3gpp:DLNA.ORG_PN=AAC_ISO_320," \
	"http-get:*:audio/mp4:DLNA.ORG_PN=AAC_ISO," \
	"http-get:*:audio/mp4:DLNA.ORG_PN=AAC_MULT5_ISO," \
	"http-get:*:audio/L16;rate=44100;channels=2:DLNA.ORG_PN=LPCM," \
	"http-get:*:image/jpeg:*," \
	"http-get:*:image/png:*," \
	"http-get:*:image/bmp:*," \
	"http-get:*:image/gif:*," \
	"http-get:*:image/tif:*," \
	"http-get:*:video/avi:*," \
	"http-get:*:video/divx:*," \
	"http-get:*:video/x-matroska:*," \
	"http-get:*:video/mpeg:*," \
	"http-get:*:video/mp4:*," \
	"http-get:*:video/x-ms-wmv:*," \
	"http-get:*:video/x-msvideo:*," \
	"http-get:*:video/x-flv:*," \
	"http-get:*:video/x-tivo-mpeg:*," \
	"http-get:*:video/quicktime:*," \
	"http-get:*:audio/mp4:*," \
	"http-get:*:audio/x-wav:*," \
	"http-get:*:audio/x-flac:*," \
	"http-get:*:audio/x-monkeys-audio:*," \ 
	"http-get:*:application/ogg:*"

#define DLNA_FLAG_DLNA_V1_5      0x00100000
#define DLNA_FLAG_HTTP_STALLING  0x00200000
#define DLNA_FLAG_TM_B           0x00400000
#define DLNA_FLAG_TM_I           0x00800000
#define DLNA_FLAG_TM_S           0x01000000
#define DLNA_FLAG_LOP_BYTES      0x20000000
#define DLNA_FLAG_LOP_NPT        0x40000000

/* startup time */
extern time_t startup_time;

extern struct runtime_vars_s runtime_vars;
/* runtime boolean flags */
extern uint32_t runtime_flags;
#define INOTIFY_MASK          0x0001
#define TIVO_MASK             0x0002
#define DLNA_STRICT_MASK      0x0004
#define NO_PLAYLIST_MASK      0x0008
#define SYSTEMD_MASK          0x0010
#define MERGE_MEDIA_DIRS_MASK 0x0020

#define SETFLAG(mask)	runtime_flags |= mask
#define GETFLAG(mask)	(runtime_flags & mask)
#define CLEARFLAG(mask)	runtime_flags &= ~mask

extern const char *pidfilename;

extern char uuidvalue[];

#define MODELNAME_MAX_LEN (64)
extern char modelname[];

#define MODELNUMBER_MAX_LEN (16)
extern char modelnumber[];

#define SERIALNUMBER_MAX_LEN (16)
extern char serialnumber[];

#define PRESENTATIONURL_MAX_LEN (64)
extern char presentationurl[];

#if PNPX
extern char pnpx_hwid[];
#endif

/* lan addresses */
extern int n_lan_addr;
extern struct lan_addr_s lan_addr[];
extern int sssdp;

extern const char * minissdpdsocketpath;

/* UPnP-A/V [DLNA] */
extern sqlite3 *db;
/* 
 * brief	Before modifing, the max length limit of friendly_name. Add by zengdongbiao, 08Apr15.
 */
#define ORIGINAL_FRIENDLYNAME_MAX_LEN (64)

/* 
 * brief	After modifing(replace '&' with "&amp;"), the max length limit of griendly_name.
 *			80 > 5* 15 + 4. 15 is the limit from UI. Modify by zengdongbiao, 08Apr15.
 */
#define FRIENDLYNAME_MAX_LEN (80)

extern char friendly_name[FRIENDLYNAME_MAX_LEN];
extern char db_path[];
extern char log_path[];
extern struct media_dir_s *media_dirs;
extern struct album_art_name_s *album_art_names;
extern short int scanning;
extern volatile short int quitting;
extern volatile uint32_t updateID;
extern const char *force_sort_criteria;
extern volatile short int inotify_quitting;

/*added by LY to set control of scanning*/
extern short int allow_scanning;
/*end added by LY*/

#endif
