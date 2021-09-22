/* $ID$
 *  Yifeng Tu (ytu@atheros.com)
 *    Oct. 2010
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
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <pjmedia/codec.h>
#include <pjmedia/errno.h>
#include <pjmedia/endpoint.h>
/* 
 * brief	by yuchuwei@20120413
 */
#	if 0
#include <pjmedia/port.h>
#	endif
#include <pjmedia-codec/types.h>
#include <pj/assert.h>
#include <pj/log.h>
#include <pj/pool.h>
#include <pj/string.h>
#include <pj/os.h>
#include <pjmedia-codec/t38.h>
#include "cmsip_assert.h"
#if defined(PJMEDIA_HAS_T38_CODEC) && (PJMEDIA_HAS_T38_CODEC != 0)

#define THIS_FILE   "t38.c"

/* Defines */
#define PTIME           (10)
#define SAMPLES_PER_FRAME   (8000 * PTIME /1000)
#define FRAME_LEN   PTIME                   /* coded frame size is normally 10bytes/10ms */
// #define SID_FRAME_LEN 2                     /* SID frame for T38 is 2 bytes */

/* Tracing */
#ifndef PJ_TRACE
#   define PJ_TRACE 1
#endif

#if PJ_TRACE 
#   define TRACE_(expr)	PJ_LOG(6,expr)
#else
#   define TRACE_(expr)
#endif


/* Prototypes for T38 factory */
static pj_status_t t38_test_alloc(pjmedia_codec_factory *factory, 
				   const pjmedia_codec_info *id );
static pj_status_t t38_default_attr(pjmedia_codec_factory *factory, 
				     const pjmedia_codec_info *id, 
				     pjmedia_codec_param *attr );
static pj_status_t t38_enum_codecs(pjmedia_codec_factory *factory, 
				    unsigned *count, 
				    pjmedia_codec_info codecs[]);
/* 
 * brief	yuchuwei@20120413
 */
#	if 0
static pj_status_t t38_alloc_codec(pjmedia_codec_factory *factory, 
				    const pjmedia_codec_info *id, 
				    pjmedia_codec **p_codec);
static pj_status_t t38_dealloc_codec(pjmedia_codec_factory *factory, 
				      pjmedia_codec *codec );

/* Prototypes for T38 implementation. */
static pj_status_t  t38_codec_open(pjmedia_codec *codec, 
				    pjmedia_codec_param *attr );
static pj_status_t  t38_codec_close(pjmedia_codec *codec );
static pj_status_t  t38_codec_modify(pjmedia_codec *codec, 
				      const pjmedia_codec_param *attr );
static pj_status_t  t38_codec_parse(pjmedia_codec *codec,
				     void *pkt,
				     pj_size_t pkt_size,
				     const pj_timestamp *ts,
				     unsigned *frame_cnt,
				     pjmedia_frame frames[]);
static pj_status_t  t38_codec_encode(pjmedia_codec *codec, 
				      const struct pjmedia_frame *input,
				      unsigned output_buf_len, 
				      struct pjmedia_frame *output);
static pj_status_t  t38_codec_decode(pjmedia_codec *codec, 
				      const struct pjmedia_frame *input,
				      unsigned output_buf_len, 
				      struct pjmedia_frame *output);

/* Definition for T38 codec operations. */
static pjmedia_codec_op t38_op = 
{
    NULL,
    &t38_codec_open,
    &t38_codec_close,
    &t38_codec_modify,
    &t38_codec_parse,
    &t38_codec_encode,
    &t38_codec_decode,
    NULL
};
#	endif

/* Definition for T38 codec factory operations. */
static pjmedia_codec_factory_op t38_factory_op =
{
    &t38_test_alloc,
    &t38_default_attr,
    &t38_enum_codecs,
/* 
 * brief	yuchuwei@20120413
 */
#	if 0
    &t38_alloc_codec,
    &t38_dealloc_codec
#	endif
};


/* T38 factory */
static struct t38_codec_factory
{
    pjmedia_codec_factory    base;
    pjmedia_endpt	                *endpt;
    pj_pool_t		                    *pool;
    pj_mutex_t		                *mutex;
    
    pjmedia_codec	     codec_list;
    
} t38_codec_factory/* Atheros modified*/, t38_codec_lite_factory/*ycw-pjsip.lite codec list*/;        


/* T38 codec private data. */
struct t38_data
{
    int                     fd;
    pj_bool_t               plc_enabled;
    pj_bool_t               vad_enabled;
};

/* T38 detail info */
static struct {
    int enabled;
    pjmedia_codec_t38_info info;
} t38_detail_info;


/*
 * Initialize and register T38 codec factory to pjmedia endpoint.
 */
pj_status_t pjmedia_codec_t38_init( pjmedia_endpt *endpt )
{
	/*ycw-pjsip-codec*/
	#if 0
    pjmedia_codec_mgr *codec_mgr;
	#else
	pjmedia_codec_mgr* full_codec_mgr;
	pjmedia_codec_mgr* lite_codec_mgr;
	#endif
    pj_status_t status;

    TRACE_((THIS_FILE, "In T38 codec factory init"));

    
    /* Check if inited already */
    if (t38_codec_factory.pool != NULL && t38_codec_lite_factory.pool != NULL)
        return PJ_SUCCESS;

	if (NULL == t38_codec_factory.pool)
	{
    	/* Create T38 codec factory. */
    	t38_codec_factory.base.op = &t38_factory_op;
    	t38_codec_factory.base.factory_data = NULL;
    	t38_codec_factory.endpt = endpt;

    	t38_codec_factory.pool = pjmedia_endpt_create_pool(endpt, "t38", 1000, 1000);
    	if (!t38_codec_factory.pool)
      	  return PJ_ENOMEM;

   	 pj_list_init(&t38_codec_factory.codec_list);

    	/* Create mutex. */
    	status = pj_mutex_create_simple(t38_codec_factory.pool, "t38", 
                &t38_codec_factory.mutex);
    	if (status != PJ_SUCCESS)
      	  goto on_error;

		#if 0
	    /* Get the codec manager. */
	    codec_mgr = pjmedia_endpt_get_codec_mgr(endpt);
	    if (!codec_mgr) {
	        status = PJ_EINVALIDOP;
	        goto on_error;
	    }

	    /* Register codec factory to endpoint. */
	    status = pjmedia_codec_mgr_register_factory(codec_mgr, 
	                    &t38_codec_factory.base);
	    if (status != PJ_SUCCESS) {
	        PJ_LOG(1,(THIS_FILE, "Error: pjmedia_codec_mgr_register_factory, status=%d !\n", status));
	        goto on_error;
	    }
		 #else
		 /* Get the codec manager. */
	    full_codec_mgr = pjmedia_endpt_get_full_codec_mgr(endpt);
	    if (!full_codec_mgr)
		 {
	        status = PJ_EINVALIDOP;
	        goto on_error;
	    }

	    /* Register codec factory to endpoint. */
	    status = pjmedia_codec_mgr_register_factory(full_codec_mgr, 
	                    &t38_codec_factory.base);
	    if (status != PJ_SUCCESS)
		 {
	        PJ_LOG(1,(THIS_FILE, "Error: pjmedia_codec_mgr_register_factory, status=%d !\n", status));
	        goto on_error;
	    }
		 #endif
	}

	if (NULL == t38_codec_lite_factory.pool)
	{
    	/* Create T38 codec factory. */
    	t38_codec_lite_factory.base.op = &t38_factory_op;
    	t38_codec_lite_factory.base.factory_data = NULL;
    	t38_codec_lite_factory.endpt = endpt;

    	t38_codec_lite_factory.pool = pjmedia_endpt_create_pool(endpt, "t38_lite", 1000, 1000);
    	if (!t38_codec_lite_factory.pool)
      	  return PJ_ENOMEM;

   	 pj_list_init(&t38_codec_lite_factory.codec_list);

    	/* Create mutex. */
    	status = pj_mutex_create_simple(t38_codec_lite_factory.pool, "t38", 
                &t38_codec_lite_factory.mutex);
    	if (status != PJ_SUCCESS)
      	  goto on_error;

		
		 /* Get the codec manager. */
	    lite_codec_mgr = pjmedia_endpt_get_lite_codec_mgr(endpt);
	    if (!lite_codec_mgr)
		 {
	        status = PJ_EINVALIDOP;
	        goto on_error;
	    }

	    /* Register codec factory to endpoint. */
	    status = pjmedia_codec_mgr_register_factory(lite_codec_mgr, 
	                    &t38_codec_lite_factory.base);
	    if (status != PJ_SUCCESS)
		 {
	        PJ_LOG(1,(THIS_FILE, "Error: pjmedia_codec_mgr_register_factory, status=%d !\n", status));
	        goto on_error;
	    }
		
	}

    memset(&t38_detail_info, 0, sizeof(t38_detail_info));

    TRACE_((THIS_FILE, "T38 codec factory initialized"));
    
    /* Done. */
    return PJ_SUCCESS;

on_error:
	if (t38_codec_factory.pool)
   {
   	pj_pool_release(t38_codec_factory.pool);		
   	t38_codec_factory.pool = NULL;
	}

	if (t38_codec_lite_factory.pool)
	{
		pj_pool_release(t38_codec_lite_factory.pool);
		t38_codec_lite_factory.pool = NULL;
	}
    return status;
}

/*
 * Unregister T38 codec factory from pjmedia endpoint and deinitialize
 * the T38 codec library.
 */
pj_status_t pjmedia_codec_t38_deinit(void)
{
	/*ycw-pjsip-codec*/
	#if 0
    pjmedia_codec_mgr *codec_mgr;
	#else
	pjmedia_codec_mgr* full_codec_mgr;
	pjmedia_codec_mgr* lite_codec_mgr;
	#endif
    pj_status_t status;

    if (t38_codec_factory.pool == NULL && t38_codec_lite_factory.pool == NULL)
        return PJ_SUCCESS;

	if (t38_codec_factory.pool)
	{
		/*ycw-pjsip-codec*/
		#if 0
	    /* Get the codec manager. */
	    codec_mgr = pjmedia_endpt_get_codec_mgr(t38_codec_factory.endpt);
	    if (!codec_mgr) {
	        pj_pool_release(t38_codec_factory.pool);
	        t38_codec_factory.pool = NULL;
	        return PJ_EINVALIDOP;
	    }

	    /* Unregister T38 codec factory. */
	    status = pjmedia_codec_mgr_unregister_factory(codec_mgr,
							  &t38_codec_factory.base);
		 #else
		 /* Get the codec manager. */
	    full_codec_mgr = pjmedia_endpt_get_full_codec_mgr(t38_codec_factory.endpt);
	    if (!full_codec_mgr)
		 {
	        pj_pool_release(t38_codec_factory.pool);
	        t38_codec_factory.pool = NULL;
	        return PJ_EINVALIDOP;
	    }

	    /* Unregister T38 codec factory. */
	    status = pjmedia_codec_mgr_unregister_factory(full_codec_mgr,
							  &t38_codec_factory.base);
		 #endif
	    
	    /* Destroy mutex. */
	    pj_mutex_destroy(t38_codec_factory.mutex);

	    /* Destroy pool. */
	    pj_pool_release(t38_codec_factory.pool);
	    t38_codec_factory.pool = NULL;
	}

	if (t38_codec_lite_factory.pool)
	{
		 /* Get the codec manager. */
	    lite_codec_mgr = pjmedia_endpt_get_lite_codec_mgr(t38_codec_lite_factory.endpt);
	    if (!lite_codec_mgr)
		 {
	        pj_pool_release(t38_codec_lite_factory.pool);
	        t38_codec_lite_factory.pool = NULL;
	        return PJ_EINVALIDOP;
	    }

	    /* Unregister T38 codec factory. */
	    status = pjmedia_codec_mgr_unregister_factory(lite_codec_mgr,
							  &t38_codec_lite_factory.base);
		    
	    /* Destroy mutex. */
	    pj_mutex_destroy(t38_codec_lite_factory.mutex);

	    /* Destroy pool. */
	    pj_pool_release(t38_codec_lite_factory.pool);
	    t38_codec_lite_factory.pool = NULL;
	}
    
    TRACE_((THIS_FILE, "T38 codec factory shutdown"));
    return status;
}

/**
 * T.38 detail info control
 *
 * @return	    PJ_SUCCESS on success.
 */
pj_status_t pjmedia_codec_t38_control(pjmedia_codec_t38_info *info)
{
    PJ_ASSERT_RETURN(info, PJ_EINVAL);

    t38_detail_info.enabled = 1;
    t38_detail_info.info = *info;
    return PJ_SUCCESS;
}

/* 
 * Check if factory can allocate the specified codec. 
 */
static pj_status_t t38_test_alloc(pjmedia_codec_factory *factory, 
                    const pjmedia_codec_info *info )
{
    PJ_UNUSED_ARG(factory);

    #if 1
    /* Check payload type. */
    if (info->pt != PJMEDIA_RTP_PT_T38)
        return PJMEDIA_CODEC_EUNSUP;
    #endif

    return PJ_SUCCESS;
}

/*
 * Generate default attribute.
 */
static pj_status_t t38_default_attr( pjmedia_codec_factory *factory, 
				      const pjmedia_codec_info *id, 
				      pjmedia_codec_param *attr )
{
    PJ_UNUSED_ARG(factory);
    
    pj_bzero(attr, sizeof(pjmedia_codec_param));

    #if 1
    attr->info.pt = PJMEDIA_RTP_PT_T38;
    #endif
    
    attr->info.channel_cnt = 1;
    attr->info.clock_rate = 8000;
    attr->info.max_bps = 8000;
    attr->info.avg_bps = 8000;
    attr->info.pcm_bits_per_sample = 16;
    attr->info.frm_ptime = PTIME;
    
    /* this is very specific to T38 */
    attr->info.fmt_id = PJMEDIA_FORMAT_T38;

	/*ycw-pjsip-ptime*/
	 #if 0
    attr->setting.frm_per_pkt = 1;
	 #endif
    attr->setting.vad = 0;  /*disable plc */
    attr->setting.plc = 0;   /* disable  vad */

    return PJ_SUCCESS;
}

/*
 * Enum codecs supported by this factory (i.e. only T38!).
 */
static pj_status_t t38_enum_codecs(pjmedia_codec_factory *factory, 
                    unsigned *count, 
                    pjmedia_codec_info codecs[])
{
    PJ_ASSERT_RETURN(factory==&t38_codec_factory.base || factory==&t38_codec_lite_factory.base, PJ_EINVAL);
    PJ_ASSERT_RETURN(codecs && *count > 0, PJ_EINVAL);

    pj_bzero(&codecs[0], sizeof(pjmedia_codec_info));
    codecs[0].encoding_name = pj_str("t38");
    #if 1
    codecs[0].pt = PJMEDIA_RTP_PT_T38;
    #endif
    codecs[0].type = PJMEDIA_TYPE_IMAGE;
    codecs[0].clock_rate = 8000;
    codecs[0].channel_cnt = 1;

    *count = 1;

    return PJ_SUCCESS;
}

/* 
 * brief	yuchuwei@20120413
 */
#	if 0
/*
 * Allocate a new T38 codec instance.
 */
static pj_status_t t38_alloc_codec(pjmedia_codec_factory *factory, 
				    const pjmedia_codec_info *id,
				    pjmedia_codec **p_codec)
{
	PJ_UNUSED_ARG(factory);
	PJ_UNUSED_ARG(id);
	PJ_UNUSED_ARG(p_codec);
	#if 0
    pjmedia_codec *codec;
    struct t38_data *t38_data;

    PJ_ASSERT_RETURN(factory && id && p_codec, PJ_EINVAL);
    PJ_ASSERT_RETURN(factory == &t38_codec_factory.base, PJ_EINVAL);

    pj_mutex_lock(t38_codec_factory.mutex);

    /* Get free nodes, if any. */
    if (!pj_list_empty(&t38_codec_factory.codec_list)) {
        codec = t38_codec_factory.codec_list.next;
        pj_list_erase(codec);
    } else {
        codec = PJ_POOL_ZALLOC_T(t38_codec_factory.pool, pjmedia_codec);
        PJ_ASSERT_RETURN(codec != NULL, PJ_ENOMEM);
        codec->op = &t38_op;
        codec->factory = factory;

        t38_data = PJ_POOL_ZALLOC_T(t38_codec_factory.pool, struct t38_data);
        codec->codec_data = t38_data;
    }
    pj_mutex_unlock(t38_codec_factory.mutex);

    *p_codec = codec;
	 #endif
    return PJ_SUCCESS;
}

/*
 * Free codec.
 */
static pj_status_t t38_dealloc_codec(pjmedia_codec_factory *factory, 
				      pjmedia_codec *codec )
{
	PJ_UNUSED_ARG(factory);
	PJ_UNUSED_ARG(codec);
	#if 0
    struct t38_data *t38_data;

    PJ_ASSERT_RETURN(factory && codec, PJ_EINVAL);
    PJ_ASSERT_RETURN(factory == &t38_codec_factory.base, PJ_EINVAL);

    t38_data = (struct t38_data*) codec->codec_data;

    /* Close codec, if it's not closed. */
    t38_codec_close(codec);

    /* Put in the free list to reuse */
    pj_mutex_lock(t38_codec_factory.mutex);
    pj_list_push_front(&t38_codec_factory.codec_list, codec);
    pj_mutex_unlock(t38_codec_factory.mutex);
   #endif 
    return PJ_SUCCESS;
}
/*
 * Open codec.
 */
static pj_status_t t38_codec_open(pjmedia_codec *codec, 
				   pjmedia_codec_param *attr )
{
	 /*ycw-pjsip*/
	 PJ_UNUSED_ARG(codec);
	 PJ_UNUSED_ARG(attr);
    
    return PJ_SUCCESS;
}

/*
 * Close codec.
 */
static pj_status_t t38_codec_close( pjmedia_codec *codec )
{
	PJ_UNUSED_ARG(codec);
	
   return PJ_SUCCESS;
}


/*
 * Modify codec settings.
 */
static pj_status_t  t38_codec_modify(pjmedia_codec *codec, 
				     const pjmedia_codec_param *attr )
{
	PJ_UNUSED_ARG(codec);
	PJ_UNUSED_ARG(attr);
	
   return PJ_SUCCESS;
}

/*
 * Get frames in the packet.
 * For T38, the packet will pass to T38 decoder unmodified, as the T38 decoder understand
 * the real format. This is true at least for UDPTL. (to do for RTP and TCP !!).
 */
static pj_status_t  t38_codec_parse(pjmedia_codec *codec,
             void *pkt,
             pj_size_t pkt_size,
             const pj_timestamp *ts,
             unsigned *frame_cnt,
             pjmedia_frame frames[])
{
    unsigned count = 0;

    PJ_UNUSED_ARG(codec);

    PJ_ASSERT_RETURN(frame_cnt, PJ_EINVAL);

    TRACE_((THIS_FILE, "T38 parse(): input len=%d, frame count = %d", pkt_size, *frame_cnt));

    frames[count].type = PJMEDIA_FRAME_TYPE_AUDIO;
    frames[count].buf = pkt;
    frames[count].timestamp.u64 = ts->u64 + count * SAMPLES_PER_FRAME;
    frames[count].size = pkt_size;

    ++count;
    *frame_cnt = count;

    return PJ_SUCCESS;
}

/*
 * Encode frame.
 */
static pj_status_t t38_codec_encode(pjmedia_codec *codec, 
				     const struct pjmedia_frame *input,
				     unsigned output_buf_len, 
				     struct pjmedia_frame *output)
{
	PJ_UNUSED_ARG(codec);
	PJ_UNUSED_ARG(input);
	PJ_UNUSED_ARG(output_buf_len);
	PJ_UNUSED_ARG(output);
	
   return PJ_SUCCESS;
}

/*
 * Decode frame.
 */
static pj_status_t t38_codec_decode(pjmedia_codec *codec, 
                 const struct pjmedia_frame *input,
                 unsigned output_buf_len, 
                 struct pjmedia_frame *output)
{
	PJ_UNUSED_ARG(codec);
	PJ_UNUSED_ARG(input);
	PJ_UNUSED_ARG(output_buf_len);
	PJ_UNUSED_ARG(output);
	
   return PJ_SUCCESS;
}
#	endif

#endif // PJMEDIA_HAS_T38_CODEC


