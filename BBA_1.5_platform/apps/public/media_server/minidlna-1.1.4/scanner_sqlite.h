/* Media table definitions for SQLite database
 *
 * Project : minidlna
 * Website : http://sourceforge.net/projects/minidlna/
 * Author  : Douglas Carmichael
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
 */

char create_objectTable_sqlite[] = "CREATE TABLE OBJECTS ("
					"ID INTEGER PRIMARY KEY AUTOINCREMENT, "
					"OBJECT_ID TEXT UNIQUE NOT NULL, "
					"PARENT_ID TEXT NOT NULL, "
					"REF_ID TEXT DEFAULT NULL, "
					"CLASS TEXT NOT NULL, "
#ifndef LITE_MINIDLNA
					"DETAIL_ID INTEGER DEFAULT NULL, "
                                        "NAME TEXT DEFAULT NULL);";
#else /* LITE_MINIDLNA */
								"DETAIL_ID INTEGER DEFAULT NULL);";
#endif /* LITE_MINIDLNA */

char create_detailTable_sqlite[] = "CREATE TABLE DETAILS ("
					"ID INTEGER PRIMARY KEY AUTOINCREMENT, "
					"PATH TEXT DEFAULT NULL, "
					"SIZE INTEGER, "
					"TIMESTAMP INTEGER, "
					"TITLE TEXT COLLATE NOCASE, "
#ifndef LITE_MINIDLNA
					"DURATION TEXT, "
					"BITRATE INTEGER, "
					"SAMPLERATE INTEGER, "
					"CREATOR TEXT COLLATE NOCASE, "
					"ARTIST TEXT COLLATE NOCASE, "
					"ALBUM TEXT COLLATE NOCASE, "
					"GENRE TEXT COLLATE NOCASE, "
					"COMMENT TEXT, "
					"CHANNELS INTEGER, "
					"DISC INTEGER, "
					"TRACK INTEGER, "
					"DATE DATE, "
					"RESOLUTION TEXT, "
					"THUMBNAIL BOOL DEFAULT 0, "
					"ALBUM_ART INTEGER DEFAULT 0, "
					"ROTATION INTEGER, "
					"DLNA_PN TEXT, "
#endif /* LITE_MINIDLNA */
                                        "MIME TEXT);";

#ifndef LITE_MINIDLNA
char create_albumArtTable_sqlite[] = "CREATE TABLE ALBUM_ART ("
					"ID INTEGER PRIMARY KEY AUTOINCREMENT, "
#ifndef USE_ALBUM_RESPONSE
					"COUNT INTERGER DEFAULT	1,"
#else /* USE_ALBUM_RESPONSE */
					"RESOLUTION TEXT, "
#endif /* USE_ALBUM_RESPONSE */
					"PATH TEXT NOT NULL"
                    ");";
#endif /* LITE_MINIDLNA */

char create_captionTable_sqlite[] = "CREATE TABLE CAPTIONS ("
					"ID INTEGER PRIMARY KEY, "
					"PATH TEXT NOT NULL"
					");";

char create_bookmarkTable_sqlite[] = "CREATE TABLE BOOKMARKS ("
					"ID INTEGER PRIMARY KEY, "
					"SEC INTEGER"
					");";


char create_playlistTable_sqlite[] = "CREATE TABLE PLAYLISTS ("
					"ID INTEGER PRIMARY KEY AUTOINCREMENT, "
					"NAME TEXT NOT NULL, "
					"PATH TEXT NOT NULL, "
					"ITEMS INTEGER DEFAULT 0, "
					"FOUND INTEGER DEFAULT 0"
					");";

char create_settingsTable_sqlite[] = "CREATE TABLE SETTINGS ("
					"KEY TEXT NOT NULL, "
					"VALUE TEXT NOT NULL"	/* Modify by zengdongbiao, 05May15. */
					");";

#ifdef USE_UPDATE
/* For database update, add by zengdongbiao, 05May15.  */
char create_watchdirsTable_sqlite[] = "CREATE TABLE WATCHDIRS ("
					"PATH TEXT NOT NULL, "
					"TIMESTAMP INTEGER"
					");";
/* End add by zengdongbiao  */
#endif /* USE_UPDATE */

