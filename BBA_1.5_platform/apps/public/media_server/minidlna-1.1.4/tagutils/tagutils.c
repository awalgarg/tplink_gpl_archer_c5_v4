//=========================================================================
// FILENAME	: tagutils.c
// DESCRIPTION	: MP3/MP4/Ogg/FLAC metadata reader
//=========================================================================
// Copyright (c) 2008- NETGEAR, Inc. All Rights Reserved.
//=========================================================================

/* This program is free software; you can redistribute it and/or modify
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* This file is derived from mt-daapd project */

#include <ctype.h>
#include <errno.h>

#ifndef LITE_MINIDLNA
#include <id3tag.h>
#endif

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/in.h>

#ifndef LITE_MINIDLNA
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <FLAC/metadata.h>
#endif

#include "config.h"

#ifndef LITE_MINIDLNA
#ifdef HAVE_ICONV
#include <iconv.h>
#endif
#endif

/* Use sqlite amalgamation, modify by zengdongbiao, 20May15. */
//#include <sqlite3.h>
#include "sqlite3.h"
/* End modify by zengdongbiao */

#include "tagutils.h"
#include "../metadata.h"
#include "../utils.h"
#include "../log.h"

#ifndef LITE_MINIDLNA

struct id3header {
	unsigned char id[3];
	unsigned char version[2];
	unsigned char flags;
	unsigned char size[4];
} __attribute((packed));

char *winamp_genre[] = {
	/*00*/ "Blues",             "Classic Rock",     "Country",           "Dance",
	       "Disco",             "Funk",             "Grunge",            "Hip-Hop",
	/*08*/ "Jazz",              "Metal",            "New Age",           "Oldies",
	       "Other",             "Pop",              "R&B",               "Rap",
	/*10*/ "Reggae",            "Rock",             "Techno",            "Industrial",
	       "Alternative",       "Ska",              "Death Metal",       "Pranks",
	/*18*/ "Soundtrack",        "Euro-Techno",	"Ambient",           "Trip-Hop",
	       "Vocal",             "Jazz+Funk",        "Fusion",            "Trance",
	/*20*/ "Classical",         "Instrumental",     "Acid",              "House",
	       "Game",              "Sound Clip",       "Gospel",            "Noise",
	/*28*/ "AlternRock",        "Bass",             "Soul",              "Punk",
	       "Space",             "Meditative",       "Instrumental Pop",  "Instrumental Rock",
	/*30*/ "Ethnic",            "Gothic",		"Darkwave",          "Techno-Industrial",
	       "Electronic",        "Pop-Folk",         "Eurodance",         "Dream",
	/*38*/ "Southern Rock",     "Comedy",           "Cult",              "Gangsta",
	       "Top 40",            "Christian Rap",    "Pop/Funk",          "Jungle",
	/*40*/ "Native American",   "Cabaret",          "New Wave",          "Psychedelic",
	       "Rave",              "Showtunes",        "Trailer",           "Lo-Fi",
	/*48*/ "Tribal",            "Acid Punk",        "Acid Jazz",         "Polka",
	       "Retro",             "Musical",          "Rock & Roll",       "Hard Rock",
	/*50*/ "Folk",              "Folk/Rock",        "National folk",     "Swing",
	       "Fast-fusion",       "Bebob",            "Latin",             "Revival",
	/*58*/ "Celtic",            "Bluegrass",        "Avantgarde",        "Gothic Rock",
	       "Progressive Rock",  "Psychedelic Rock", "Symphonic Rock",    "Slow Rock",
	/*60*/ "Big Band",          "Chorus",           "Easy Listening",    "Acoustic",
	       "Humour",            "Speech",           "Chanson",           "Opera",
	/*68*/ "Chamber Music",     "Sonata",           "Symphony",          "Booty Bass",
	       "Primus",            "Porn Groove",      "Satire",            "Slow Jam",
	/*70*/ "Club",              "Tango",            "Samba",             "Folklore",
	       "Ballad",            "Powder Ballad",    "Rhythmic Soul",     "Freestyle",
	/*78*/ "Duet",              "Punk Rock",        "Drum Solo",         "A Capella",
	       "Euro-House",        "Dance Hall",       "Goa",               "Drum & Bass",
	/*80*/ "Club House",        "Hardcore",         "Terror",            "Indie",
	       "BritPop",           "NegerPunk",        "Polsk Punk",        "Beat",
	/*88*/ "Christian Gangsta", "Heavy Metal",      "Black Metal",       "Crossover",
	       "Contemporary C",    "Christian Rock",   "Merengue",          "Salsa",
	/*90*/ "Thrash Metal",      "Anime",            "JPop",              "SynthPop",
	       "Unknown"
};

#define WINAMP_GENRE_UNKNOWN ((sizeof(winamp_genre) / sizeof(winamp_genre[0])) - 1)

#endif

/*
 * Prototype
 */
#include "tagutils-mp3.h"
#include "tagutils-aac.h"
#include "tagutils-ogg.h"
#include "tagutils-flc.h"
#include "tagutils-asf.h"
#include "tagutils-wav.h"
#include "tagutils-pcm.h"

#ifndef LITE_MINIDLNA

#include "libav.h"

/* 
 * fn		static int _get_apetags(char *filename, struct song_metadata *psong)
 * brief	Get ape(audio file format) tags and file informations.
 * details	
 *
 * param[in]	filename	ape file name.
 * param[out]	psong		ape file informations.
 *
 * return	
 * retval		0	successfully.
 *				-1	error.
 * note		written by zengdongbiao, 26May15.
 */
static int _get_apetags(char *filename, struct song_metadata *psong);


static int _get_tags(char *file, struct song_metadata *psong);
static int _get_fileinfo(char *file, struct song_metadata *psong);


/*
 * Typedefs
 */

typedef struct {
	char* type;
	int (*get_tags)(char* file, struct song_metadata* psong);
	int (*get_fileinfo)(char* file, struct song_metadata* psong);
} taghandler;

static taghandler taghandlers[] = {
	{ "aac", _get_aactags, _get_aacfileinfo                                  },
	{ "mp3", _get_mp3tags, _get_mp3fileinfo                                  },
	{ "flc", _get_flctags, _get_flcfileinfo                                  },
	{ "ogg", 0,            _get_oggfileinfo                                  },
	{ "asf", 0,            _get_asffileinfo                                  },
	{ "wav", _get_wavtags, _get_wavfileinfo                                  },
	{ "pcm", 0,            _get_pcmfileinfo                                  },
	{ "ape", _get_apetags,    0									             },
	{ NULL,  0 }
};

#endif

//*********************************************************************************
#include "tagutils-misc.c"
#include "tagutils-mp3.c"
#include "tagutils-aac.c"
#include "tagutils-ogg.c"
#include "tagutils-flc.c"
#include "tagutils-asf.c"
#include "tagutils-wav.c"
#include "tagutils-pcm.c"
#include "tagutils-plist.c"

//*********************************************************************************
// freetags()
#define MAYBEFREE(a) { if((a)) free((a)); };
void
freetags(struct song_metadata *psong)
{
	int role;

	MAYBEFREE(psong->path);
	MAYBEFREE(psong->image);
	MAYBEFREE(psong->title);
	MAYBEFREE(psong->album);
	MAYBEFREE(psong->genre);
	MAYBEFREE(psong->comment);
	for(role = ROLE_START; role <= ROLE_LAST; role++)
	{
		MAYBEFREE(psong->contributor[role]);
		MAYBEFREE(psong->contributor_sort[role]);
	}
	MAYBEFREE(psong->grouping);
	MAYBEFREE(psong->mime);
	MAYBEFREE(psong->dlna_pn);
	MAYBEFREE(psong->tagversion);
	MAYBEFREE(psong->musicbrainz_albumid);
	MAYBEFREE(psong->musicbrainz_trackid);
	MAYBEFREE(psong->musicbrainz_artistid);
	MAYBEFREE(psong->musicbrainz_albumartistid);
}

#ifndef LITE_MINIDLNA

/* 
 * fn		static int _get_apetags(char *filename, struct song_metadata *psong)
 * brief	Get ape(audio file format) tags and file informations.
 * details	
 *
 * param[in]	filename	ape file name.
 * param[out]	psong		ape file informations.
 *
 * return	
 * retval		0	successfully.
 *				-1	error.
 * note		written by zengdongbiao, 26May15.
 */
static int _get_apetags(char *filename, struct song_metadata *psong)
{
	AVFormatContext *ctx = NULL;
	AVCodecContext *ac = NULL;
	int audio_stream = -1;
	int ret = 0;
	int i = 0;
	
	ret = lav_open(&ctx, filename);
	if( ret != 0 )
	{
		char err[128];
		av_strerror(ret, err, sizeof(err));
		DPRINTF(E_WARN, L_METADATA, "Opening %s failed! [%s]\n", filename, err);
		/* if avformat_open_input() successful, then close. */
		if (ctx)
		{
			lav_close(ctx);
		}
		/*  end add by zengdongbiao */
		return -1;
	}

	for( i=0; i<ctx->nb_streams; i++)
	{
		if( audio_stream == -1 &&
		    ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO )
		{
			audio_stream = i;
			ac = ctx->streams[audio_stream]->codec;
			continue;
		}
	}

	if (!ac || strcmp(ctx->iformat->name, "ape") != 0)
    {
        DPRINTF(E_WARN, L_METADATA, "%s error format\n", filename);
        lav_close(ctx);
        return -1;
    }

	if( ctx->metadata )
	{
		AVDictionaryEntry *tag = NULL;

		while( (tag = av_dict_get(ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)) )
		{
			//DPRINTF(E_WARN, L_METADATA, "  %-16s: %s\n", tag->key, tag->value);
			if( strcasecmp(tag->key, "title") == 0 )
			{
				psong->title = escape_tag(trim(tag->value), 1);
			}
			else if( strcasecmp(tag->key, "genre") == 0 )
			{
				psong->genre = escape_tag(trim(tag->value), 1);
			}
			else if( strcasecmp(tag->key, "artist") == 0 )
			{
				psong->contributor[ROLE_ARTIST] = escape_tag(trim(tag->value), 1);
			}
			else if( strcasecmp(tag->key, "comment") == 0 )
			{
				psong->comment = escape_tag(trim(tag->value), 1);
			}
			else if( strcasecmp(tag->key, "track") == 0 )
			{
				psong->track = atoi(trim(tag->value));
			}
			else if( strcasecmp(tag->key, "Album") == 0 )
			{
				psong->album = escape_tag(trim(tag->value), 1);
			}
		}
	}
	
	if( ctx->bit_rate > 8 )
	{
		//psong->bitrate = ac->bit_rate; /* ac->bit_rate is 0  */
		psong->bitrate = ctx->bit_rate / 8;
	}
	if( ctx->duration > 0 ) 
	{
		psong->song_length = (int)(ctx->duration / (AV_TIME_BASE/1000)); /* ms */
	}

	psong->samplerate = ac->sample_rate;
	psong->channels = ac->channels;
	//DPRINTF(E_WARN, L_METADATA, "psong->samplerate: %d\n"
	//						   "psong->channels: %d\n",
	//						   psong->samplerate,
	//						   psong->channels);
	
	lav_close(ctx);

	return 0;
}

// _get_fileinfo
static int
_get_fileinfo(char *file, struct song_metadata *psong)
{
	taghandler *hdl;

	// dispatch to appropriate tag handler
	for(hdl = taghandlers; hdl->type; ++hdl)
		if(!strcmp(hdl->type, psong->type))
			break;

	if(hdl->get_fileinfo)
		return hdl->get_fileinfo(file, psong);

	return 0;
}


static void
_make_composite_tags(struct song_metadata *psong)
{
	int len;

	len = 1;

	if(!psong->contributor[ROLE_ARTIST] &&
	   (psong->contributor[ROLE_BAND] || psong->contributor[ROLE_CONDUCTOR]))
	{
		if(psong->contributor[ROLE_BAND])
			len += strlen(psong->contributor[ROLE_BAND]);
		if(psong->contributor[ROLE_CONDUCTOR])
			len += strlen(psong->contributor[ROLE_CONDUCTOR]);

		len += 3;

		psong->contributor[ROLE_ARTIST] = (char*)calloc(len, 1);
		if(psong->contributor[ROLE_ARTIST])
		{
			if(psong->contributor[ROLE_BAND])
				strcat(psong->contributor[ROLE_ARTIST], psong->contributor[ROLE_BAND]);

			if(psong->contributor[ROLE_BAND] && psong->contributor[ROLE_CONDUCTOR])
				strcat(psong->contributor[ROLE_ARTIST], " - ");

			if(psong->contributor[ROLE_CONDUCTOR])
				strcat(psong->contributor[ROLE_ARTIST], psong->contributor[ROLE_CONDUCTOR]);
		}
	}

#if 0 // already taken care of by scanner.c
	if(!psong->title)
	{
		char *suffix;
		psong->title = strdup(psong->basename);
		suffix = strrchr(psong->title, '.');
		if(suffix) *suffix = '\0';
	}
#endif
}


/*****************************************************************************/
// _get_tags
static int
_get_tags(char *file, struct song_metadata *psong)
{
	taghandler *hdl;

	// dispatch
	for(hdl = taghandlers ; hdl->type ; ++hdl)
		if(!strcasecmp(hdl->type, psong->type))
			break;

	if(hdl->get_tags)
	{
		return hdl->get_tags(file, psong);
	}

	return 0;
}

/*****************************************************************************/
// readtags
int
readtags(char *path, struct song_metadata *psong, struct stat *stat, char *lang, char *type)
{
	char *fname;

	if(lang_index == -1)
		lang_index = _lang2cp(lang);

	memset((void*)psong, 0, sizeof(struct song_metadata));
	psong->path = strdup(path);
	psong->type = type;

	fname = strrchr(psong->path, '/');
	psong->basename = fname ? fname + 1 : psong->path;

	if(stat)
	{
		if(!psong->time_modified)
			psong->time_modified = stat->st_mtime;
		psong->file_size = stat->st_size;
	}

	// get tag
	if( _get_tags(path, psong) == 0 )
	{
		_make_composite_tags(psong);
	}
	
	// get fileinfo
	return _get_fileinfo(path, psong);
}

#endif
