/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		usbvm_speechEncDec.h
 * brief		
 * details	
 *
 * author	Sirrain Zhang
 * version	
 * date		07Jun11
 *
 * history 	\arg	
 */
 
#ifndef __USBVM_SPEECHENCDEC_H__
#define __USBVM_SPEECHENCDEC_H__

#include <stdio.h>
#include "usbvm_types.h"
#include <usbvm_glbdef.h>

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/
#define PCM_RESOLUTION_16 16  /* 16bits per sample */
#define PCM_RESOLUTION_8 8    /* 8bits per sample */
#define PCM_CHANNEL_MONO 1    /* mono */

#define PCM_FORMAT_TAG   0x01
#define PCMA_FORMAT_TAG  0x06
#define PCMU_FORMAT_TAG  0x07

/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/
/* 
 * brief	Swap 16bit data
 */
#define WAV_SWAP16( val ) {val = (((( val ) & 0xff00 ) >> 8)|((( val ) & 0x00ff ) << 8));}

/* 
 * brief	Swap 32bit data
 */
#define WAV_SWAP32( val ) {val = (((( val ) & 0xff000000 ) >> 24)\
                            | ((( val ) & 0x00ff0000 ) >> 8)\
                            | ((( val ) & 0x0000ff00 ) << 8)\
                            | ((( val ) & 0x000000ff ) << 24));}

/* 
 * brief	Wav audio format
 */
typedef struct 
{
    unsigned short  format_tag;
    unsigned short  channels;           /* 1 = mono, 2 = stereo */
    unsigned long   samplerate;         /* typically: 44100, 32000, 22050, 11025 or 8000*/
    unsigned long   bytes_per_second;   /* SamplesPerSec * BlockAlign*/
    unsigned short  blockalign;         /* Channels * (BitsPerSample / 8)*/
    unsigned short  bits_per_sample;    /* 16 or 8 */
} WAVEAUDIOFORMAT;

/* 
 * brief	Wav transformation status	
 */
typedef enum
{
	WAV_IDLE,                           /* Wav transformation is idle */
	WAV_START,                          /* Wav transformation start */
	WAV_PROCESS,                        /* Wav transformation is in process */
	WAV_FINISHED                        /* Wav transformation finished */
}WAV_STATUS;


/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           FUNCTIONS                                            */
/**************************************************************************************************/
/* 
 * fn		int usbvm_changePcmToWav(char *pSrcFile, char *pDstFile)
 * brief	Transformtion pcm file to wav format file
 * details	
 *
 * param[in]	pSrcFile - src file path
 * param[in/out] pDstFile - dst file path
 *
 * return	
 * retval	
 *
 * note		
 */
int usbvm_changePcmToWav(char *pSrcFile, char *pDstFile);

/* 
 * fn		void usbvm_speechToWavFormatByPos ( int endpt, int nodePos, BOOL bSleep )
 * brief	Transformtion from record speech file to wav format file
 * details	
 *
 * param[in]	endpt  FXS endpoint number
 * param[in]	nodePos  node position in record index list
 * param[in]	bSleep  need sleep or not
 * param[out]
 *
 * return	
 * retval	
 *
 * note		
 */
void usbvm_changeFormatToWav ( int endpt, int fileTimeName, BOOL bSleep);

/* 
 * fn		void usbvm_speechToWavFormatByName ( int endpt, unsigned int fileTimName, BOOL bSleep )
 * brief	Transformtion from record speech file to wav format file
 * details	
 *
 * param[in]	endpt  FXS endpoint number
 * param[in]	fileTimName  record file name in record index list
 * param[in]	bSleep  need sleep or not
 * param[out]
 *
 * return	
 * retval	
 *
 * note		
 */
int usbvm_speechToWavFormatByName ( int endpt, unsigned int fileTimName, BOOL bSleep );

/* 
 * fn		void  usbvm_G711AToG711U( char *pInputBuf, char *pOutputBuf, int frameSize )
 * brief	Tramsformation from G.711A to G.711U
 * details	
 *
 * param[in]	*pInputBuf  pointer of input buffer
 * param[in]	*pOutputBuf  pointer of output buffer
 * param[in]    frameSize  frame size for each tranformation 
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
void  usbvm_G711AToG711U( char *pInputBuf, char *pOutputBuf, int frameSize );

/* 
 * fn		void  usbvm_G711UToG711A( char *pInputBuf, char *pOutputBuf, int frameSize )
 * brief	Tramsformation from G.711U to G.711A
 * details	
 *
 * param[in]	*pInputBuf  pointer of input buffer
 * param[in]	*pOutputBuf  pointer of output buffer
 * param[in]    frameSize  frame size for each tranformation 
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
void  usbvm_G711UToG711A( char *pInputBuf, char *pOutputBuf, int frameSize );

/* 
 * fn		int usbvm_createWavHeader( FILE *fpWav, int formatTag, int channels, int samplerate, int resolution )
 * brief	Make an unfinished RIFF structure for wav file, with null values in the lengths
 * details	
 *
 * param[in]	*fpWav  pointer of wav file
 * param[in]	formatTag  encode type
 * param[in]	channels  mono or stereo
 * param[in]	samplerate  sampling frequency  
 * param[in]	resolution  16bit or 8bit per sample
 * param[out]	
 *
 * return	success return 0, fail return -1
 * retval	
 *
 * note		RIFF format:
 *			0 - 3  Byte(4): "RIFF"
 *			4 - 7  Byte(4): Little-endian 32-bit, whole file size minus 8 bytes
 *			8 - 11 Byte(4): "WAVE" or "AVI "
 *			12 - 15Byte(4): "fmt " or "data"
 *			16 - 19Byte(4): 10H00H00H00H
 *
 *			20 - 21Byte(2): Format Tag
 *			22 - 23Byte(2): the number of Channels
 *			24 - 27Byte(4): sample per sec.
 *			28 - 31Byte(4): average per sec.
 *			32 - 33Byte(2): Block align.
 *			34 - 35Byte(2): Bits per sample
 *
 *			36 - 39Byte(4): "data"
 *			40 - 43Byte(4): little endian 32-bit, data sub section size
 *			44 - ...      : voice date.
 */
int usbvm_createWavHeader( FILE *fpWav, int formatTag, int channels, int samplerate, int resolution );

/* 
 * fn		int usbvm_updateWavHeader( FILE *fpWav )
 * brief	Update the RIFF structure with proper values. CreateWavHeader must be called first
 * details	
 *
 * param[in]	*fpWav  pointer of wav file
 * param[out]	
 *
 * return	success return 0, fail return -1
 * retval	
 *
 * note		RIFF format:
 *			0 - 3  Byte(4): "RIFF"
 *			4 - 7  Byte(4): Little-endian 32-bit, whole file size minus 8 bytes
 *			8 - 11 Byte(4): "WAVE" or "AVI "
 *			12 - 15Byte(4): "fmt " or "data"
 *			16 - 19Byte(4): 10H00H00H00H
 *
 *			20 - 21Byte(2): Format Tag
 *			22 - 23Byte(2): the number of Channels
 *			24 - 27Byte(4): sample per sec.
 *			28 - 31Byte(4): average per sec.
 *			32 - 33Byte(2): Block align.
 *			34 - 35Byte(2): Bits per sample
 *
 *			36 - 39Byte(4): "data"
 *			40 - 43Byte(4): little endian 32-bit, data sub section size
 *			44 - ...      : voice date.
 *			
 */
int usbvm_updateWavHeader( FILE *fpWav );

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif	/* __USBVM_SPEECHENCDEC_H__ */

