/* $Id: pjmedia.h 3553 2011-05-05 06:14:19Z nanang $ */
/* 
 * Copyright (C) 2008-2011 Teluu Inc. (http://www.teluu.com)
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
#ifndef __PJMEDIA_H__
#define __PJMEDIA_H__

/**
 * @file pjmedia.h
 * @brief PJMEDIA main header file.
 */

#include <pjmedia/types.h>
/* 
 * brief	yuchuwei@20120413
 */
#	if 0
#include <pjmedia/alaw_ulaw.h>
#	endif
#include <pjmedia/circbuf.h>
/* 
 * brief	yuchuwei@20120413
 */
#	if 0
#include <pjmedia/clock.h>
#include <pjmedia/delaybuf.h>
#include <pjmedia/jbuf.h>
#include <pjmedia/plc.h>
#include <pjmedia/port.h>
#include <pjmedia/silencedet.h>
#include <pjmedia/transport_loop.h>
#include <pjmedia/wsola.h>
#	endif

#include <pjmedia/codec.h>
/*ycw-pjsip-delete conference bridge*/
#if 0
#include <pjmedia/conference.h>
#endif
/*ycw-pjsip-delete echo port*/
#if 0
#include <pjmedia/echo.h>

#include <pjmedia/echo_port.h>
#endif
#include <pjmedia/errno.h>
#include <pjmedia/endpoint.h>
#include <pjmedia/g711.h>
/*ycw-pjsip. t38*/
#include <pjmedia-codec/t38.h>
/*ycw-pjsip--delete port*/
#if 0
#include <pjmedia/master_port.h>
#include <pjmedia/null_port.h>
#include <pjmedia/bidirectional.h>

#include <pjmedia/mem_port.h>
#endif
/*ycw-pjsip-delete resample port*/
#if 0
#include <pjmedia/resample.h>
#endif
#include <pjmedia/rtcp.h>
#include <pjmedia/rtcp_xr.h>
#include <pjmedia/rtp.h>
#include <pjmedia/sdp.h>
#include <pjmedia/sdp_neg.h>
#include <pjmedia/session.h>
/*ycw-pjsip-delete sound device*/
#if 0
#include <pjmedia/sound.h>
#include <pjmedia/sound_port.h>
#include <pjmedia/splitcomb.h>
#endif
/*ycw-pjsip-delete stereo port*/
#if 0
#include <pjmedia/stereo.h>
#endif
#include <pjmedia/stream.h>
/*ycw-pjsip*/
#if 0
#include <pjmedia/tonegen.h>
#endif
#include <pjmedia/transport.h>
#include <pjmedia/transport_adapter_sample.h>
#include <pjmedia/transport_ice.h>
#include <pjmedia/transport_srtp.h>
#include <pjmedia/transport_udp.h>
/*ycw-pjsip-delete wav file*/
#if 0
#include <pjmedia/wav_playlist.h>
#include <pjmedia/wav_port.h>
#include <pjmedia/wave.h>
#endif

#endif	/* __PJMEDIA_H__ */

