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
 * brief	16bit线性码编码为8比特A律非线性码
 * details	
 *
 * param[in]	pcmVal  16比特线性码
 * param[out]	
 *
 * return	8比特A律非线性码
 * retval	
 *
 * note		
 */
unsigned char linear2Alaw(short pcmVal);

/* 
 * fn		short alaw2Linear(unsigned char	aVal)
 * brief	8bitA律非线性码解码为16比特线性码编码
 * details	
 *
 * param[in]	aVal  8比特A律非线性码
 * param[out]	
 *
 * return	16比特线性码
 * retval	
 *
 * note		
 */
short alaw2Linear(unsigned char	aVal);

/* 
 * fn		unsigned char linear2Ulaw(short pcmVal)
 * brief	16bit 线性码编码为8比特u律非线性码
 * details	
 *
 * param[in]	pcmVal  16比特线性码
 * param[out]	
 *
 * return	8比特u律非线性码
 * retval	
 *
 * note		
 */
unsigned char linear2Ulaw(short pcmVal);

/* 
 * fn		short ulaw2Linear(unsigned char uVal)
 * brief	8bit u律非线性码解码为16bits 线性码编码
 * details	
 *
 * param[in]	uVal  8比特u律非线性码
 * param[out]	
 *
 * return	16比特线性码
 * retval	
 *
 * note		
 */
short ulaw2Linear(unsigned char uVal);

/* 
 * fn		unsigned char alaw2Ulaw(unsigned char aVal)
 * brief	按照A律到U律转化编码表进行A律到U律码型转换
 * details	
 *
 * param[in]	aVal  8比特A律非线性码
 * param[out]	
 *
 * return	8比特U律非线性码
 * retval	
 *
 * note		
 */
unsigned char alaw2Ulaw(unsigned char aVal);

/* 
 * fn		unsigned char ulaw2Alaw(unsigned char uVal)
 * brief	按照U律到A律转化编码表进行U律到A律码型转换
 * details	
 *
 * param[in]	uVal  8比特U律非线性码
 * param[out]	
 *
 * return	8比特A律非线性码
 * retval	
 *
 * note		
 */
unsigned char ulaw2Alaw(unsigned char uVal);


#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */


#endif	/* __USBVM_G711_H__ */

