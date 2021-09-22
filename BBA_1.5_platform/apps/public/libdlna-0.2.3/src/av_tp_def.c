/******************************************************************************
*
* Copyright (c) 2010 TP-LINK Technologies CO.,LTD.
* All rights reserved.
*
* FILE NAME  :   av_tp_def.c
* VERSION    :   1.0
* DESCRIPTION:   we defined more vedio dlna profile..
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

#include "dlna_internals.h"
#include "profiles.h"

/* vedio avi format support. by HouXB, 29Mar11 */
static dlna_profile_t avi = {
	/*每添加一种格式的profile 必须在DLNA标准有有定义相对应,DLNA__Media_Formats 文档,
	 Added by LI CHENGLONG , 2011-Dec-07.*/
  .id = "AVI",
  .mime = MIME_VIDEO_AVI,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t *
probe_avi (AVFormatContext *ctx dlna_unused,
             dlna_container_type_t st dlna_unused,
             av_codecs_t *codecs)
{  
  return NULL;
}

/* get dlna profile by extension. by HouXB, 08Oct10 */
static dlna_profile_t *
probe2_avi ()
{
	return &avi;
}

dlna_registered_profile_t dlna_profile_av_avi = {
  .id = DLNA_PROFILE_AV_AVI,
  .class = DLNA_CLASS_AV,
  .extensions = "avi,dv,divx,mjpg,mjpeg",
  .probe = probe_avi,
  .probe2 = probe2_avi, 
  .next = NULL
};


/* vedio vob format support. by HouXB, 29Mar11 */
static dlna_profile_t vob = {
  .id = "VOB",
  .mime = MIME_VIDEO_VOB,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t *
probe_vob (AVFormatContext *ctx dlna_unused,
             dlna_container_type_t st dlna_unused,
             av_codecs_t *codecs)
{  
  return NULL;
}

/* get dlna profile by extension. by HouXB, 08Oct10 */
static dlna_profile_t *
probe2_vob ()
{
	return &vob;
}

dlna_registered_profile_t dlna_profile_av_vob = {
  .id = DLNA_PROFILE_AV_VOB,
  .class = DLNA_CLASS_AV,
  .extensions = "vob",
  .probe = probe_vob,
  .probe2 = probe2_vob, 
  .next = NULL
};

/* vedio m4v format support. by HouXB, 29Mar11 */
static dlna_profile_t m4v = {
  .id = "M4V",
  .mime = MIME_VIDEO_M4V,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t *
probe_m4v (AVFormatContext *ctx dlna_unused,
             dlna_container_type_t st dlna_unused,
             av_codecs_t *codecs)
{  
  return NULL;
}

/* get dlna profile by extension. by HouXB, 08Oct10 */
static dlna_profile_t *
probe2_m4v ()
{
	return &m4v;
}

dlna_registered_profile_t dlna_profile_av_m4v = {
  .id = DLNA_PROFILE_AV_M4V,
  .class = DLNA_CLASS_AV,
  .extensions = "m4v,m4p,mp4ps,ogm",
  .probe = probe_m4v,
  .probe2 = probe2_m4v, 
  .next = NULL
};


/* vedio mkv format support. by HouXB, 29Mar11 */
static dlna_profile_t mkv = {
  .id = "MKV",
  .mime = MIME_VIDEO_MKV,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t *
probe_mkv (AVFormatContext *ctx dlna_unused,
             dlna_container_type_t st dlna_unused,
             av_codecs_t *codecs)
{  
  return NULL;
}

/* get dlna profile by extension. by HouXB, 08Oct10 */
static dlna_profile_t *
probe2_mkv ()
{
	return &mkv;
}

dlna_registered_profile_t dlna_profile_av_mkv = {
  .id = DLNA_PROFILE_AV_MKV,
  .class = DLNA_CLASS_AV,
  .extensions = "mkv,rmvb,qt",
  .probe = probe_mkv,
  .probe2 = probe2_mkv, 
  .next = NULL
};


/* fix play mov slow bug. by HouXB, 07Apr11 */
static dlna_profile_t mov = {
  .id = "MOV",
  .mime = MIME_VIDEO_MOV,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t *
probe_mov (AVFormatContext *ctx dlna_unused,
             dlna_container_type_t st dlna_unused,
             av_codecs_t *codecs)
{  
  return NULL;
}

/* get dlna profile by extension. by HouXB, 08Oct10 */
static dlna_profile_t *
probe2_mov ()
{
	return &mov;
}

dlna_registered_profile_t dlna_profile_av_mov = {
  .id = DLNA_PROFILE_AV_MOV,
  .class = DLNA_CLASS_AV,
  .extensions = "mov",
  .probe = probe_mov,
  .probe2 = probe2_mov, 
  .next = NULL
};

