/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		usbvm_voiceNotify.h
 * brief		
 * details	
 *
 * author	Sirrain Zhang
 * version	
 * date		07Jun11
 *
 * history 	\arg	
 */

#ifndef __USBVM_VOICENOTIFY_H__
#define __USBVM_VOICENOTIFY_H__


/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/
#define VOICE_NOTIFY_DE -1                              /* "嘀"音(beep) */
#define VOICE_NOTIFY_PLAY_OPERATION -2                  /* 听取留言操作提示音 */
#define VOICE_NOTIFY_LOCALREC_START_OPERATION -3        /* 本地开始录制问候语操作提示音 */
#define VOICE_NOTIFY_LOCALREC_STOP_OPERATION -4         /* 本地停止录制问候语操作提示音 */
#define VOICE_NOTIFY_VERIFY_LOCALREC -5                 /* 验证本地录制的问候语 */
#define VOICE_NOTIFY_NO_MSG -6						/* 本地播放时没有任何留言提示音 */
#define VOICE_NOTIFY_NO_NEW_MSG -7						/* 本地播放时，有留言但没有未读时提示音 */
#define VOICE_NOTIFY_INPUT_PIN -8						/* NEWUI: 删除所有留言时，输入PIN码提示音 */
#define VOICE_NOTIFY_PIN_ERR -9						/* NEWUI: 删除所有留言时，输入PIN码错误 */
#define VOICE_NOTIFY_NULL -10						/* 注意: NULL 必须是最小值，要用于判断 */


/**************************************************************************************************/
/*                                           FUNCTIONS                                            */
/**************************************************************************************************/
/* 
 * fn		void usbvm_getVoiceNotifyDe( char **ppBuf, unsigned int *pSize )
 * brief	Get beep sound buffer pointer and buffer size
 * details	
 *
 * param[in]	**ppBuf  pointer of beep sound buffer
 * param[in]	*pSize  pointer of beep sound buffer size
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
extern void usbvm_getVoiceNotifyDe( char **ppBuf, unsigned int *pSize );

#endif	/* __USBVM_VOICENOTIFY_H__ */

