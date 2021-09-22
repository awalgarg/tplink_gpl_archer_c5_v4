/* $Id: //depot/sw/branches/NewmaStaging/apps/gateway/services/phone/pjsip/pjmedia/pjmedia-codec/t38.h#1 $ */
/* 
 *          Yifeng Tu (ytu@atheros.com)
 *          Oct 5, 2010
 *
 * Copyright (C) 2008-2009 Teluu Inc. (http://www.teluu.com)
 * Copyright (C) 2003-2008 Benny Prijono <benny@prijono.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
#ifndef __PJMEDIA_CODEC_T38_H__
#define __PJMEDIA_CODEC_T38_H__

/**
 * @file pjmedia-codec/t38.h
 * @brief T.38 codec.
 */

#include <pjmedia-codec/types.h>

/**
 * T.38 detail info
 */
typedef struct {
    unsigned int   max_rate;                 /* the maximum bit rate to use  */
    unsigned int   max_size;                 /* T38 engine jitter buffer size */
    unsigned int   signal_redundancy; /* Redundancy on signaling packets */ 
    unsigned int   image_redundancy; /* Redundancy on image packets */ 
    unsigned int   ecm_enabled;          /* Non-ECM mode or ECM mode */
} pjmedia_codec_t38_info;

/**
 * @defgroup PJMED_T38 T.38 Codec
 * @ingroup PJMEDIA_CODEC_CODECS
 * @brief Implementation of T.38 Codec
 * @{
 *
 * This section describes functions to register and register T.38 codec
 * factory to the codec manager. After the codec factory has been registered,
 * application can use @ref PJMEDIA_CODEC API to manipulate the codec.
 *
 * The T.38 codec implementation is provided as part of pjmedia-codec
 * library, and does not depend on external T.38 codec implementation.
 */

PJ_BEGIN_DECL


/**
 * Initialize and register T.38 codec factory to pjmedia endpoint.
 *
 * @param endpt	    The pjmedia endpoint.
 *
 * @return	    PJ_SUCCESS on success.
 */
extern pj_status_t pjmedia_codec_t38_init(pjmedia_endpt *endpt);


/**
 * Unregister T.38 codec factory from pjmedia endpoint and cleanup
 * resources allocated by the factory.
 *
 * @return	    PJ_SUCCESS on success.
 */
extern pj_status_t pjmedia_codec_t38_deinit(void);



/**
 * T.38 detail info control
 *
 * @return	    PJ_SUCCESS on success.
 */
extern pj_status_t pjmedia_codec_t38_control(pjmedia_codec_t38_info *info);


PJ_END_DECL


/**
 * @}
 */

#endif /* __PJMEDIA_CODEC_T38_H__ */


