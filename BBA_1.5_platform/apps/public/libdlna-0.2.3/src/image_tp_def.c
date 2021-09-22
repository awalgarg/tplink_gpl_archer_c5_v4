/******************************************************************************
*
* Copyright (c) 2010 TP-LINK Technologies CO.,LTD.
* All rights reserved.
*
* FILE NAME  :   image_tp_def.c
* VERSION    :   1.0
* DESCRIPTION:   we defined more image dlna profile..
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

/* image bmp support. by HouXB, 29Mar11 */
static dlna_profile_t bmp = {
  .id = "BMP",
  .mime = MIME_IMAGE_BMP,
  .label = LABEL_IMAGE_PICTURE
};


static dlna_profile_t *
probe_bmp (AVFormatContext *ctx,
            dlna_container_type_t st,
            av_codecs_t *codecs)
{  
  return NULL;
}

/* get dlna profile by extension. by HouXB, 08Oct10 */
static dlna_profile_t *
probe2_bmp ()
{
	return &bmp;
}

dlna_registered_profile_t dlna_profile_image_bmp = {
	.id = DLNA_PROFILE_IMAGE_BMP,
	.class = DLNA_CLASS_IMAGE,
	.extensions = "bmp,ico",
	.probe = probe_bmp,
  	.probe2 = probe2_bmp,
	.next = NULL
};

/* image gif support. by HouXB, 29Mar11 */
static dlna_profile_t gif = {
  .id = "GIF",
  .mime = MIME_IMAGE_GIF,
  .label = LABEL_IMAGE_PICTURE
};


static dlna_profile_t *
probe_gif (AVFormatContext *ctx,
            dlna_container_type_t st,
            av_codecs_t *codecs)
{  
  return NULL;
}

static dlna_profile_t *
probe2_gif ()
{
	return &gif;
}

dlna_registered_profile_t dlna_profile_image_gif = {
	.id = DLNA_PROFILE_IMAGE_GIF,
	.class = DLNA_CLASS_IMAGE,
	.extensions = "gif,pcd,pnm,ppm",
	.probe = probe_gif,
  	.probe2 = probe2_gif,
	.next = NULL
};

/* image tif support. by HouXB, 29Mar11 */
static dlna_profile_t tif = {
  .id = "TIF",
  .mime = MIME_IMAGE_TIFF,
  .label = LABEL_IMAGE_PICTURE
};


static dlna_profile_t *
probe_tif (AVFormatContext *ctx,
            dlna_container_type_t st,
            av_codecs_t *codecs)
{  
  return NULL;
}

static dlna_profile_t *
probe2_tif ()
{
	return &tif;
}

dlna_registered_profile_t dlna_profile_image_tif = {
	.id = DLNA_PROFILE_IMAGE_TIF,
	.class = DLNA_CLASS_IMAGE,
	.extensions = "qti,qtf,qtif,tif,tiff",
	.probe = probe_tif,
  	.probe2 = probe2_tif,
	.next = NULL
};