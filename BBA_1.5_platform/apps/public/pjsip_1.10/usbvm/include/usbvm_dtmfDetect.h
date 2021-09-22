
#ifndef _DTMF_DETECT_H_
#define _DTMF_DETECT_H_

#include <usbvm_glbdef.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_FRAME_SIZE	240
#define MAX_LOOPS	160 		//max count for DFT loops

#define THR_SIG 	3200	//	2800	//threshold for tone (-23dB) :YDN-065
#define THR_PAU		800	//	400		//threshold for pause energy

#define THR_TWI1	(800*32768/10000)	//threshold for twist (-10dB) : <<YDN-065(-6dB)>>
#define THR_TWI2	(800*32768/10000)	//threshold for twist (-10dB) : <<YDN-065(-6dB)>>

#define THR_ROWREL	(5000*32768/10000)	//threshold for row's relative peak ratio
#define THR_COLREL	(5000*32768/10000)	//threshold for col's relative peak ratio

//#define THR_ROW2nd	(200*32768/10000)	//threshold for row's 2nd harmonic ratio
//#define THR_COL2nd	(200*32768/10000)	//threshold for col's 2nd harmonic ratio

#define PAUSE_TIME	1

#define GAIN_THR	0x0060*FRAME_SIZE	//gain control threshold

#define GAINBUF_LEN	(32)
#define TAPS_LEN		(8*2)
#define ENERGY_LEN	(8)
#define DTMF_NUMBERS	(16)

#define ULAW
//#define DEBUG

#ifndef BOOL
#define BOOL int
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef struct
{
	int goertzelCnt;
	unsigned short digitLast;
	int pauseCnt;
//	short *pOldSample;
//	short gainBuf[GAINBUF_LEN];
	short taps[TAPS_LEN];
	short energy[ENERGY_LEN];
} DTMFDet_t;

void dtmfDetInit( void );

BOOL dtmfDet(unsigned char *framBuf, int frameLen, unsigned int rtpPayloadType, unsigned short *pNum);

#ifdef __cplusplus
}
#endif

#endif

