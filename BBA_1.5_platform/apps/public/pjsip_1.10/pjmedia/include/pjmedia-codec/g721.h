#ifndef __G721_H__
#define __G721_H__

/**
 * @file g721.h
 * @brief G721 Codec
 */

#include <pjmedia-codec/types.h>


PJ_BEGIN_DECL


pj_status_t pjmedia_codec_g721_init(pjmedia_endpt *endpt);



pj_status_t pjmedia_codec_g721_deinit(void);


PJ_END_DECL

/**
 * @}
 */




#endif	/* __G721_H__ */

