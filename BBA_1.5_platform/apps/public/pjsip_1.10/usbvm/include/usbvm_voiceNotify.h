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
#define VOICE_NOTIFY_DE -1                              /* "��"��(beep) */
#define VOICE_NOTIFY_PLAY_OPERATION -2                  /* ��ȡ���Բ�����ʾ�� */
#define VOICE_NOTIFY_LOCALREC_START_OPERATION -3        /* ���ؿ�ʼ¼���ʺ��������ʾ�� */
#define VOICE_NOTIFY_LOCALREC_STOP_OPERATION -4         /* ����ֹͣ¼���ʺ��������ʾ�� */
#define VOICE_NOTIFY_VERIFY_LOCALREC -5                 /* ��֤����¼�Ƶ��ʺ��� */
#define VOICE_NOTIFY_NO_MSG -6						/* ���ز���ʱû���κ�������ʾ�� */
#define VOICE_NOTIFY_NO_NEW_MSG -7						/* ���ز���ʱ�������Ե�û��δ��ʱ��ʾ�� */
#define VOICE_NOTIFY_INPUT_PIN -8						/* NEWUI: ɾ����������ʱ������PIN����ʾ�� */
#define VOICE_NOTIFY_PIN_ERR -9						/* NEWUI: ɾ����������ʱ������PIN����� */
#define VOICE_NOTIFY_NULL -10						/* ע��: NULL ��������Сֵ��Ҫ�����ж� */


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

