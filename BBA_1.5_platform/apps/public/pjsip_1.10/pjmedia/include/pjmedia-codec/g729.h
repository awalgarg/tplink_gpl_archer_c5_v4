#ifndef __G729_H__
#define __G729_H__

/**
 * @file g729.h
 * @brief G729 Codec
 */

#include <pjmedia-codec/types.h>


PJ_BEGIN_DECL


pj_status_t pjmedia_codec_g729_init(pjmedia_endpt *endpt);



pj_status_t pjmedia_codec_g729_deinit(void);


PJ_END_DECL

/**
 * @}
 */




#endif	/* __G729_H__ */

