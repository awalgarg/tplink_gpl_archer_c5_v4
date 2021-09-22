/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		usbvm_glbdef.h
 * brief		
 * details	
 *
 * author	Sirrain Zhang
 * version	
 * date		19Jul11
 *
 * history 	\arg	
 */


#ifndef __USBVM_GLBDEF_H__
#define __USBVM_GLBDEF_H__

#include <sched.h>
#include <pj/compat/m_auto.h>		/* for byte order */

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/
/* If USBVM_DEBUG==1, will print debug info and assertion is on
 *    USBVM_DEBUG==10, whole message
 */
#define USBVM_DEBUG 0


#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef UNKNOWN
#define UNKNOWN -1
#endif

#ifndef PJ_IS_BIG_ENDIAN
#error  "======== usb voice mail MUST get byte order ======="
#endif



/* dingcheng-pjsip:add CHANNELS NUM MACRO */
#ifndef NUM_FXS_CHANNELS
#define NUM_FXS_CHANNELS 2
#endif

#ifndef NUM_DECT_CHANNELS
#define NUM_DECT_CHANNELS 0
#endif

#ifndef NUM_VOICEAPP_CHANNELS
#define NUM_VOICEAPP_CHANNELS 0
#endif



#ifndef ENDPT_NUM
#define ENDPT_NUM (NUM_FXS_CHANNELS + NUM_DECT_CHANNELS + NUM_VOICEAPP_CHANNELS + 1)
#endif

#define CMM_COMPILE  1                  /* Compile CMM or not */

#define TASK_PRIORITY_LOWEST    (sched_get_priority_min(SCHED_FIFO))
#define TASK_PRIORITY_MED_LOW   ((sched_get_priority_max(SCHED_FIFO) + 1)/4)
#define TASK_PRIORITY_MED       ((sched_get_priority_max(SCHED_FIFO) + 1)/2)
#define TASK_PRIORITY_MED_HIGH  ((sched_get_priority_max(SCHED_FIFO) + 1)/4 * 3)
#define TASK_PRIORITY_HIGHEST   (sched_get_priority_max(SCHED_FIFO))

#define USB_VOICEMAIL_ENDIS_MASK 2   // bits->1*
#define USB_VOICEMAIL_DISABLE    0   // bits->0*
#define USB_VOICEMAIL_ENABLE     2   // bits->1*

#define USB_VOICEMAIL_MODE_MASK  1   // bits->*1
#define USB_VOICEMAIL_NOANS      0   // bits->*0
#define USB_VOICEMAIL_UNCON      1   // bits->*1

#define USB_VOICEMAIL_ENDIS_MODE_MASK 3   // bits->11
#define USB_VOICEMAIL_DISABLE_NOANS   0   // bits->00
#define USB_VOICEMAIL_DISABLE_UNCON   1   // bits->01
#define USB_VOICEMAIL_ENABLE_NOANS    2   // bits->10
#define USB_VOICEMAIL_ENABLE_UNCON    3   // bits->11

#ifndef MIN
#define MIN(a,b)     (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b)     (((a) > (b)) ? (a) : (b))
#endif

#define RTP_PAYLOAD_PCMU      0
#define RTP_PAYLOAD_PCMA      8
#define RTP_PAYLOAD_G726_16   98
#define RTP_PAYLOAD_G726_24   97
#define RTP_PAYLOAD_G726_32   2
#define RTP_PAYLOAD_G726_40   96
#define RTP_PAYLOAD_G729A     18
#define RTP_PAYLOAD_G729      18
#define RTP_PAYLOAD_G7231_63  4 
#define RTP_PAYLOAD_G7231_53  4
#define RTP_PAYLOAD_NULL      255

#define CNG_PAYLOAD_TYPE  13
#define CNG_PAYLOAD_TYPE_OLD 19 	/* the old CNG pt is 19,before RFC3389 */
#define CNG_LEVEL 0x4e		/* the suggested cng level  in RFC3389 is 78. */
#define CNG_TIMEVAL 200		/* in ms */
#define PCMU_MUTE_DATA    0xFF /* raw data is 0 */
#define PCMA_MUTE_DATA    0xD5 /* raw data is 0 */


/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/
/* Voice codec types */
typedef enum
{
   CODEC_NULL  = (0),        /* NULL */
   CODEC_PCMU,               /* G.711 ulaw */
   CODEC_PCMA,               /* G.711 alaw */
   CODEC_G726_16,            /* G.726 - 16 kbps */
   CODEC_G726_24,            /* G.726 - 24 kbps */
   CODEC_G726_32,            /* G.726 - 32 kbps */
   CODEC_G726_40,            /* G.726 - 40 kbps */
   CODEC_G7231_53,           /* G.723.1 - 5.3 kbps */
   CODEC_G7231_63,           /* G.723.1 - 6.3 kbps */
   CODEC_G7231A_53,          /* G.723.1A - 5.3 kbps */
   CODEC_G7231A_63,          /* G.723.1A - 6.3 kbps */
   CODEC_G729A,              /* G.729A */
   CODEC_G729B,              /* G.729B */
   CODEC_G711_LINEAR,        /* Linear media queue data */
   CODEC_G728,               /* G.728 */
   CODEC_G729,               /* G.729 */
   CODEC_G729E,              /* G.729E */
   CODEC_BV16,               /* BRCM Broadvoice - 16 kbps */
   CODEC_BV32,               /* BRCM Broadvoice - 32 kbps */
   CODEC_NTE,                /* Named telephone events */
   CODEC_ILBC_20,            /* iLBC speech coder - 20 ms frame / 15.2 kbps */
   CODEC_ILBC_30,            /* iLBC speech coder - 30 ms frame / 13.3 kbps */
   CODEC_G7231_53_VAR,       /* G723.1 variable rates (preferred=5.3) */
   CODEC_G7231_63_VAR,       /* G723.1 variable rates (preferred=6.3) */
   CODEC_G7231_VAR,          /* G723.1 variable rates */
   CODEC_T38,                /* T.38 fax relay */
   CODEC_T38_MUTE,           /* Mute before switching to T.38 fax relay */
   CODEC_RED,                /* Redundancy - RFC 2198 */
   CODEC_G722_MODE_1,        /* G.722 Mode 1 64 kbps */
   CODEC_LINPCM128,          /* Narrowband linear PCM @ 128 Kbps */
   CODEC_LINPCM256,          /* Wideband linear PCM @ 256 Kbps */

   CODEC_GSMAMR_12K,         /* GSM AMR codec @ 12.2 kbps */
   CODEC_GSMAMR_10K,         /* GSM AMR codec @ 10.2 kbps */
   CODEC_GSMAMR_795,         /* GSM AMR codec @ 7.95 kbps */
   CODEC_GSMAMR_740,         /* GSM AMR codec @ 7.4 kbps */
   CODEC_GSMAMR_670,         /* GSM AMR codec @ 6.7 kbps */
   CODEC_GSMAMR_590,         /* GSM AMR codec @ 5.9 kbps */
   CODEC_GSMAMR_515,         /* GSM AMR codec @ 5.15 kbps */
   CODEC_GSMAMR_475,         /* GSM AMR codec @ 4.75 kbps */

   CODEC_AMRWB_66,           /* AMR WB codec @ 6.6 kbps */
   CODEC_AMRWB_885,          /* AMR WB codec @ 8.85 kbps */
   CODEC_AMRWB_1265,         /* AMR WB codec @ 12.65 kbps */
   CODEC_AMRWB_1425,         /* AMR WB codec @ 14.25 kbps */
   CODEC_AMRWB_1585,         /* AMR WB codec @ 15.85 kbps */
   CODEC_AMRWB_1825,         /* AMR WB codec @ 18.25 kbps */
   CODEC_AMRWB_1985,         /* AMR WB codec @ 19.85 kbps */
   CODEC_AMRWB_2305,         /* AMR WB codec @ 23.05 kbps */
   CODEC_AMRWB_2385,         /* AMR WB codec @ 23.85 kbps */
   
   /* Maximum number of codec types. */
   CODEC_MAX_TYPES,

   /* Place-holder for dynamic codec types that haven't been mapped yet */
   CODEC_DYNAMIC        = (0xffff),

   /* Place-holder for unknown codec types that should be ignored/removed from list */
   CODEC_UNKNOWN        = (0xfffe)
} CODEC_TYPE;

/* RTP named telephone events, as defined in RFC2833 */
typedef enum
{
   /* DTMF named events */
   RTP_NTE_DTMF0     = 0,           /* DTMF Tone 0 event */
   RTP_NTE_DTMF1     = 1,           /* DTMF Tone 1 event */
   RTP_NTE_DTMF2     = 2,           /* DTMF Tone 2 event */
   RTP_NTE_DTMF3     = 3,           /* DTMF Tone 3 event */
   RTP_NTE_DTMF4     = 4,           /* DTMF Tone 4 event */
   RTP_NTE_DTMF5     = 5,           /* DTMF Tone 5 event */
   RTP_NTE_DTMF6     = 6,           /* DTMF Tone 6 event */
   RTP_NTE_DTMF7     = 7,           /* DTMF Tone 7 event */
   RTP_NTE_DTMF8     = 8,           /* DTMF Tone 8 event */
   RTP_NTE_DTMF9     = 9,           /* DTMF Tone 9 event */
   RTP_NTE_DTMFS     = 10,          /* DTMF Tone * event */
   RTP_NTE_DTMFH     = 11,          /* DTMF Tone # event */
   RTP_NTE_DTMFA     = 12,          /* DTMF Tone A event */
   RTP_NTE_DTMFB     = 13,          /* DTMF Tone B event */
   RTP_NTE_DTMFC     = 14,          /* DTMF Tone C event */
   RTP_NTE_DTMFD     = 15,          /* DTMF Tone D event */

   /* LCS named events */
   RTP_NTE_RING      = 144,         /* Ringing event */
   RTP_NTE_ONHOOK    = 149,         /* Onhook event */
   RTP_NTE_OSI       = 159,         /* Open switch interval event */

   /* GR303 named events */
   RTP_NTE_RINGOFF   = 149,         /* Ringing off event */
   RTP_NTE_OFFHOOK   = 159          /* Offhook event */

} RTP_NTE;

/*20130618_update*/
#ifndef __FILE_NO_DIR__
#define __FILE_NO_DIR__		((strrchr(__FILE__, '/') ?: __FILE__-1)+1)
#endif /* __FILE_NO_DIR__ */
#if defined(USBVM_DEBUG) && (USBVM_DEBUG >= 7)
#define TT_PRINT(format, args...)		printf("%s:%d " format "\n", __FILE_NO_DIR__, __LINE__, ##args)
#else
#define TT_PRINT(format, args...) 
#endif /* defined(USBVM_DEBUG) && (USBVM_DEBUG >= 7) */


#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */


#endif	/* __USBVM_GLBDEF_H__ */

