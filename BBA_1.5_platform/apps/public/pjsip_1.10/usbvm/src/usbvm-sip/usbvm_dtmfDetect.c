#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <usbvm_g711.h>
#include <usbvm_dtmfDetect.h>

#ifdef __cplusplus
extern "C"
{
#endif

const short COEF[ENERGY_LEN]={
		27980, 26956, 25701, 24219, 19073, 16325,13085, 9315 
		};

const short KEYS[DTMF_NUMBERS]={
		0x0301,		// '0'  
		0x0000,		// '1'
		0x0001,		// '2'
		0x0002,		// '3'
		0x0100,		// '4'
		0x0101,		// '5'
		0x0102,		// '6'
		0x0200,		// '7'
		0x0201,		// '8'
		0x0202,		// '9'
		0x0300,		// 'E' = '*'
		0x0302,		// 'F' = '#'
		0x0003,		// 'A'
		0x0103,		// 'B'
		0x0203,		// 'C'
		0x0303		// 'D'

		};

DTMFDet_t m_dtmfTab;

static inline short MUL_S16_S16(short x, short y);
static short DIV_S16_S16(short x, short y);
static void init(DTMFDet_t *pTab);
static void gaincrtl(short* pSample, int frameLen);
static void goertzel(short x, DTMFDet_t *pTab);
static void getEnergy(DTMFDet_t *pTab);
static void maxSearch(short *rowMax, short *colMax, short *rowMax2, short *colMax2, unsigned short *row, unsigned short *col, DTMFDet_t *pTab);
static BOOL strengthCheck(short rowMax, short colMax, DTMFDet_t *pTab);
static BOOL twistCheck(short rowMax, short colMax);
static BOOL relCheck(short rowMax, short colMax, short rowMax2, short colMax2);


void dtmfDetInit( void )
{
	init(&m_dtmfTab);	
}



BOOL dtmfDet(unsigned char *framBuf, int frameLen, unsigned int rtpPayloadType, unsigned short *pNum)
{
	int i;
	short rowMax, colMax, rowMax2, colMax2;
	unsigned short row, col, numMap, currentNum = 0;
	const short *pKEYS = KEYS;
	BOOL ret = FALSE;
	short linearCode[MAX_FRAME_SIZE]={0};

	for(i=0; i<frameLen; i++)
	{
		if (rtpPayloadType == RTP_PAYLOAD_PCMU)
		{
			linearCode[i] = ulaw2Linear(framBuf[i]);
		}
		else if (rtpPayloadType == RTP_PAYLOAD_PCMA)
		{
			linearCode[i] = alaw2Linear(framBuf[i]);
		}
			
	}

	gaincrtl(linearCode, frameLen);

	for(i=0; i<frameLen; i++)
	{
		goertzel(linearCode[i], &m_dtmfTab);
	}

	getEnergy(&m_dtmfTab);

	maxSearch(&rowMax, &colMax, &rowMax2, &colMax2, &row, &col, &m_dtmfTab);

	if(strengthCheck(rowMax, colMax, &m_dtmfTab) == FALSE)
	{
		return FALSE;
	}
	if(twistCheck(rowMax, colMax) == FALSE)
	{
		return FALSE;
	}
	if(relCheck(rowMax, colMax, rowMax2, colMax2) == FALSE)
	{
		return FALSE;
	}
		
	numMap = (row<<8) + col;
	for(i=0; i<DTMF_NUMBERS; i++)
	{
		if(numMap == *pKEYS++)
		{
			currentNum = (unsigned short)i;
			break;
		}
	}

	if(currentNum == m_dtmfTab.digitLast)
	{
		ret = TRUE;
		if(pNum)
			*pNum = currentNum;
		m_dtmfTab.digitLast = 16;
		m_dtmfTab.pauseCnt = 0;
	}
	else
		m_dtmfTab.digitLast = currentNum;

	return ret;
}

inline short MUL_S16_S16(short x, short y)
{
	short ret = (short)((x*y)>>15);
	return ret;
}

short DIV_S16_S16(short x, short y)
{
	int iteration;
	short var_out;
	short L_num;
	short L_denom;

	if (y == 0)
	{
		printf("Division by 0, Fatal error!\n");
		return 0;
	}

	if (x == 0)
	{
		var_out = 0;
		return var_out;
	}

	if (x == y) 
	{
		var_out = 0x7fff;
		return var_out;
	}

	var_out = 0;
	L_num = (int)(x);
	L_denom = (int)(y);

	for(iteration=0; iteration<15; iteration++)
	{
		var_out <<=1;
		L_num <<= 1;

		if (L_num >= L_denom)
		{
			L_num -= L_denom;
			var_out += 1;
		}
	}

	return var_out;
}

void init(DTMFDet_t *pTab)
{
	pTab->goertzelCnt = MAX_LOOPS;
	pTab->digitLast = 16;
	pTab->pauseCnt = PAUSE_TIME;
//	pTab->pOldSample = pTab->gainBuf;
//	memset(pTab->gainBuf, 0, GAINBUF_LEN*sizeof(short));
	memset(pTab->taps, 0, TAPS_LEN*sizeof(short));
	memset(pTab->energy, 0, ENERGY_LEN*sizeof(short));
	return;
}

/*
GAIN_AMP(n) = (1/32)*[abs(x(n))+abs(x(n+1))+...+abs(x(n+31))]

if( GAIN_AMP > GAIN_THR )
	y(n) = (GAIN_THR/GAIN_AMP) * x(n)
else
	y(n) = x(n)
*/

void gaincrtl(short* pSample, int frameLen)
{
	int i = 0;
	unsigned int sum = 0;
	short gainAmp = 0;
	short factor = 0;
	
	for(i=0; i<frameLen; i++)
	{
		sum += abs(pSample[i]);
	}

	if(sum > 0x0060*frameLen)
	{
		gainAmp = sum>>8;
		factor = DIV_S16_S16(0x0060*frameLen>>8, gainAmp);
		for(i=0; i<frameLen; i++)
			pSample[i] = MUL_S16_S16(factor, pSample[i]);
	}

	return;
}

/* vk(n) = 2*coef*vk(n-1) - vk(n-2) + x(n) */
void goertzel(short x, DTMFDet_t *pTab)
{
	int i;
	short vk=0, *pTaps;
	const short *pCOEF;

	pTaps = pTab->taps;
	pCOEF = COEF;
	for(i=0; i<ENERGY_LEN; i++)
	{
		vk = x - *pTaps++;
		vk >>= 1;
		vk += MUL_S16_S16(*pCOEF++, *pTaps++);
		vk <<= 1;
		*(pTaps-2) = *(pTaps-1);
		*(pTaps-1) = vk;
	}

	return;
}

/* y(N)y*(N) = vk(N)*vk(N) - 2*coef*vk(N)*vk(N-1) + vk(N-1)*vk(N-1) */

void getEnergy(DTMFDet_t *pTab)
{
	int i;
	short *pTaps, *pEnergy;
	const short *pCOEF;

	pTaps = pTab->taps;
	pEnergy = pTab->energy;
	pCOEF = COEF;
	for(i=0; i<ENERGY_LEN; i++)
	{
		*pEnergy = MUL_S16_S16(*pTaps, *pTaps);
		pTaps++;
		*pEnergy += MUL_S16_S16(*pTaps, *pTaps);
		*pEnergy >>= 1;
		*pEnergy -= MUL_S16_S16(MUL_S16_S16(*pTaps, *(pTaps-1)), *pCOEF);
		*pEnergy <<= 1;
		pTaps++; pEnergy++; pCOEF++;
	}

	memset(pTab->taps, 0, TAPS_LEN*sizeof(short));	//reset taps to 0


	return;
}

void maxSearch(short *rowMax, short *colMax, short *rowMax2, short *colMax2,
	unsigned short *row, unsigned short *col, DTMFDet_t *pTab)
{
	int i;
	short *pEnergy = pTab->energy;

	*rowMax = *pEnergy;
	*rowMax2 = *pEnergy++;
	*row = 0;
	for(i=1; i<4; i++)
	{
		if(*rowMax < *pEnergy)
		{
			*row = i;
			*rowMax2 = *rowMax;
			*rowMax = *pEnergy;
		}
		else if(*rowMax2 < *pEnergy)
		{
			*rowMax2 = *pEnergy;
		}
		else if(i == 1)
			*rowMax2 = *pEnergy;
			
		pEnergy++;
	}

	*colMax = *pEnergy;
	*colMax2 = *pEnergy++;
	*col = 0;
	for(i=1; i<4; i++)
	{
		if(*colMax < *pEnergy)
		{
			*col = i;
			*colMax2 = *colMax;
			*colMax = *pEnergy;
		}
		else if(*colMax2 < *pEnergy)
		{
			*colMax2 = *pEnergy;
		}
		else if(i == 1)
			*colMax2 = *pEnergy;

		pEnergy++;
	}

	return;
}

/*
bool pauseCheck(short rowMax, short colMax, DTMFDet_t *pTab)
{
	if(pTab->pauseCnt == PAUSE_TIME)
		return true;

	return false;
}
*/

BOOL strengthCheck(short rowMax, short colMax, DTMFDet_t *pTab)
{
	if(rowMax + colMax > THR_SIG)
	{
		if(pTab->pauseCnt == PAUSE_TIME)
			return TRUE;
		return FALSE;
	}
	else if(rowMax + colMax > THR_PAU)
		return FALSE;
	else
	{
		if(pTab->pauseCnt != PAUSE_TIME)
			pTab->pauseCnt++;
		return FALSE;
	}
}

BOOL twistCheck(short rowMax, short colMax)
{
	short tmp;

	if(rowMax > colMax)
	{
		tmp = THR_TWI1;
		if(MUL_S16_S16(tmp, rowMax) < colMax)
			return TRUE;
	}
	else
	{
		tmp = THR_TWI2;
		if(MUL_S16_S16(tmp, colMax) < rowMax)
			return TRUE;
	}

	return FALSE;
}

BOOL relCheck(short rowMax, short colMax, short rowMax2, short colMax2)
{
	short tmp = THR_ROWREL;

	if(MUL_S16_S16(tmp, rowMax) < rowMax2)
	{
		return FALSE;
	}

	tmp = THR_COLREL;

	if(MUL_S16_S16(tmp, colMax) < colMax2)
	{
		return FALSE;
	}
	return TRUE;
}

#ifdef __cplusplus
}
#endif

