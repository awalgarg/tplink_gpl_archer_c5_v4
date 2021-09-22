/******************************************************************************
*
* Copyright (c) 2010 TP-LINK Technologies CO.,LTD.
* All rights reserved.
*
* FILE NAME  :   audio_tp_def.c
* VERSION    :   1.0
* DESCRIPTION:   we defined more audio dlna profile..
*
* AUTHOR     :   HouXB <houxubo@tp-link.net>
* CREATE DATE:   03/28/2011
*
* HISTORY    :
* 01   03/28/2011  HouXB     Create.
*
******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "dlna_internals.h"
#include "profiles.h"
#include "containers.h"

/* Profile for audio media class content */

/* audio aif support. by HouXB, 29Mar11 */
static dlna_profile_t aif = {
  .id = "AIF",
  .mime = MIME_AUDIO_AIF,
  .label = LABEL_AUDIO_2CH
};

static dlna_profile_t *
probe_aif (AVFormatContext *ctx,
             dlna_container_type_t st,
             av_codecs_t *codecs)
{    
  return NULL;
}

/* get dlna profile by extension. by HouXB, 08Oct10 */
static dlna_profile_t *
probe2_aif()
{
	return &aif;
}

dlna_registered_profile_t dlna_profile_audio_aif = {
  .id = DLNA_PROFILE_AUDIO_AIF,
  .class = DLNA_CLASS_AUDIO,
  .extensions = "aif,au,snd,dts,rmi",
  .probe = probe_aif,
  .probe2 = probe2_aif,
  .next = NULL
};


/* audio mp2 support. by HouXB, 29Mar11 */
static dlna_profile_t mp2 = {
  .id = "MP2",
  .mime = MIME_AUDIO_MP2,
  .label = LABEL_AUDIO_2CH
};

static dlna_profile_t *
probe_mp2 (AVFormatContext *ctx,
             dlna_container_type_t st,
             av_codecs_t *codecs)
{    
  return NULL;
}


static dlna_profile_t *
probe2_mp2()
{
	return &mp2;
}

dlna_registered_profile_t dlna_profile_audio_mp2 = {
  .id = DLNA_PROFILE_AUDIO_MP2,
  .class = DLNA_CLASS_AUDIO,
  .extensions = "mp1,mp2,mpa",
  .probe = probe_mp2,
  .probe2 = probe2_mp2,
  .next = NULL
};

/* audio ogg support. by HouXB, 29Mar11 */
static dlna_profile_t ogg = {
  .id = "OGG",
  .mime = MIME_AUDIO_OGG,
  .label = LABEL_AUDIO_2CH
};

static dlna_profile_t *
probe_ogg (AVFormatContext *ctx,
             dlna_container_type_t st,
             av_codecs_t *codecs)
{    
  return NULL;
}


static dlna_profile_t *
probe2_ogg()
{
	return &ogg;
}

dlna_registered_profile_t dlna_profile_audio_ogg = {
  .id = DLNA_PROFILE_AUDIO_OGG,
  .class = DLNA_CLASS_AUDIO,
  .extensions = "ogg,pcm,l16,ra,rm,ram",
  .probe = probe_ogg,
  .probe2 = probe2_ogg,
  .next = NULL
};

/* audio flac support. by HouXB, 29Mar11 */
static dlna_profile_t flac = {
  .id = "FLAC",
  .mime = MIME_AUDIO_FLAC,
  .label = LABEL_AUDIO_2CH
};

static dlna_profile_t *
probe_flac (AVFormatContext *ctx,
             dlna_container_type_t st,
             av_codecs_t *codecs)
{    
  return NULL;
}


static dlna_profile_t *
probe2_flac()
{
	return &flac;
}

dlna_registered_profile_t dlna_profile_audio_flac = {
  .id = DLNA_PROFILE_AUDIO_FLAC,
  .class = DLNA_CLASS_AUDIO,
  .extensions = "flac",
  .probe = probe_flac,
  .probe2 = probe2_flac,
  .next = NULL
};

/* fix aiff play slow bug. by HouXB, 07Apr11 */
static dlna_profile_t aiff = {
  .id = "aiff",
  .mime = MIME_AUDIO_AIFF,
  .label = LABEL_AUDIO_MONO
};

static dlna_profile_t *
probe_aiff (AVFormatContext *ctx,
             dlna_container_type_t st,
             av_codecs_t *codecs)
{    
  return NULL;
}


static dlna_profile_t *
probe2_aiff()
{
	return &aiff;
}

dlna_registered_profile_t dlna_profile_audio_aiff = {
  .id = DLNA_PROFILE_AUDIO_AIFF,
  .class = DLNA_CLASS_AUDIO,
  .extensions = "aiff",
  .probe = probe_aiff,
  .probe2 = probe2_aiff,
  .next = NULL
};