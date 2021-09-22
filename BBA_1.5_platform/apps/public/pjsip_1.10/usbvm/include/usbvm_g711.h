/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		usbvm_g711.h
 * brief		
 * details	
 *
 * author	Sirrain Zhang
 * version	
 * date		07Jun11
 *
 * history 	\arg	
 */

#ifndef __USBVM_G711_H__
#define __USBVM_G711_H__

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           FUNCTIONS                                            */
/**************************************************************************************************/

/* 
 * fn		unsigned char linear2Alaw(short pcmVal)
 * brief	16bit���������Ϊ8����A�ɷ�������
 * details	
 *
 * param[in]	pcmVal  16����������
 * param[out]	
 *
 * return	8����A�ɷ�������
 * retval	
 *
 * note		
 */
unsigned char linear2Alaw(short pcmVal);

/* 
 * fn		short alaw2Linear(unsigned char	aVal)
 * brief	8bitA�ɷ����������Ϊ16�������������
 * details	
 *
 * param[in]	aVal  8����A�ɷ�������
 * param[out]	
 *
 * return	16����������
 * retval	
 *
 * note		
 */
short alaw2Linear(unsigned char	aVal);

/* 
 * fn		unsigned char linear2Ulaw(short pcmVal)
 * brief	16bit ���������Ϊ8����u�ɷ�������
 * details	
 *
 * param[in]	pcmVal  16����������
 * param[out]	
 *
 * return	8����u�ɷ�������
 * retval	
 *
 * note		
 */
unsigned char linear2Ulaw(short pcmVal);

/* 
 * fn		short ulaw2Linear(unsigned char uVal)
 * brief	8bit u�ɷ����������Ϊ16bits ���������
 * details	
 *
 * param[in]	uVal  8����u�ɷ�������
 * param[out]	
 *
 * return	16����������
 * retval	
 *
 * note		
 */
short ulaw2Linear(unsigned char uVal);

/* 
 * fn		unsigned char alaw2Ulaw(unsigned char aVal)
 * brief	����A�ɵ�U��ת����������A�ɵ�U������ת��
 * details	
 *
 * param[in]	aVal  8����A�ɷ�������
 * param[out]	
 *
 * return	8����U�ɷ�������
 * retval	
 *
 * note		
 */
unsigned char alaw2Ulaw(unsigned char aVal);

/* 
 * fn		unsigned char ulaw2Alaw(unsigned char uVal)
 * brief	����U�ɵ�A��ת����������U�ɵ�A������ת��
 * details	
 *
 * param[in]	uVal  8����U�ɷ�������
 * param[out]	
 *
 * return	8����A�ɷ�������
 * retval	
 *
 * note		
 */
unsigned char ulaw2Alaw(unsigned char uVal);


#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */


#endif	/* __USBVM_G711_H__ */

