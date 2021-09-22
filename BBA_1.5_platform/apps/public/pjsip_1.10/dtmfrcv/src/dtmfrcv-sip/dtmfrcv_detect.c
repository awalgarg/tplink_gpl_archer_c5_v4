/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		dtmfrcv_detect.c
 * brief		
 * details	
 *
 * author		Huang Lei
 * version	
 * date		30Oct11
 *
 * history 	\arg	
 */
#include <dtmfrcv_detect.h>



#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/
#define	SIGN_BIT	(0x80)		/* A-law 符号位  */
#define	QUANT_MASK	(0xf)		/* 段内量化值域  */
#define	NSEGS		(8)		    /* A-law 段落号. */
#define	SEG_SHIFT	(4)		    /* 段落左移位量  */
#define	SEG_MASK	(0x70)		/* 段落码区域.   */
#define	BIAS		(0x84)		/* 线性码偏移值  */
#define CLIP        (8159)      /* 最大量化级数量 */
/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/
const short DR_COEF[ENERGY_LEN]={
		27980, 26956, 25701, 24219, 19073, 16325,13085, 9315 
		};
		
const short DR_KEYS[DTMF_NUMBERS]={
		0x0301,		/* '0' */ 
		0x0000,		/* '1' */
		0x0001,		/* '2' */
		0x0002,		/* '3' */
		0x0100,		/* '4' */
		0x0101,		/* '5' */
		0x0102,		/* '6' */
		0x0200,		/* '7' */
		0x0201,		/* '8' */
		0x0202,		/* '9' */
		0x0300,		/* 'E' = '*' */
		0x0302,		/* 'F' = '#' */
		0x0003,		/* 'A' */
		0x0103,		/* 'B' */
		0x0203,		/* 'C' */
		0x0303		/* 'D' */

		};
/**************************************************************************************************/
/*                                           EXTERN_PROTOTYPES                                    */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           LOCAL_PROTOTYPES                                     */
/**************************************************************************************************/
static inline short MUL_S16_S16(short x, short y);
static short DIV_S16_S16(short x, short y);
static void DR_init(DR_Det_t *pTab);
static void gaincrtl(short* pSample, int frameLen);
static void goertzel(short x, DR_Det_t *pTab);
static void getEnergy(DR_Det_t *pTab);
static void maxSearch(short *rowMax, short *colMax, short *rowMax2, short *colMax2, unsigned short *row, unsigned short *col, DR_Det_t *pTab);
static BOOL strengthCheck(short rowMax, short colMax, DR_Det_t *pTab);
static BOOL twistCheck(short rowMax, short colMax);
static BOOL relCheck(short rowMax, short colMax, short rowMax2, short colMax2);
static short DR_ulaw2Linear(unsigned char uVal);
static short DR_alaw2Linear(unsigned char	aVal);
/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/

DR_Det_t m_DR_Tab;

/**************************************************************************************************/
/*                                           LOCAL_FUNCTIONS                                      */
/**************************************************************************************************/
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

void DR_init(DR_Det_t *pTab)
{
	pTab->goertzelCnt = MAX_LOOPS;
	pTab->digitLast = 16;
	pTab->pauseCnt = PAUSE_TIME;
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
void goertzel(short x, DR_Det_t *pTab)
{
	int i;
	short vk=0, *pTaps;
	const short *pCOEF;

	pTaps = pTab->taps;
	pCOEF = DR_COEF;
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

void getEnergy(DR_Det_t *pTab)
{
	int i;
	short *pTaps, *pEnergy;
	const short *pCOEF;

	pTaps = pTab->taps;
	pEnergy = pTab->energy;
	pCOEF = DR_COEF;
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

	memset(pTab->taps, 0, TAPS_LEN*sizeof(short));	/*reset taps to 0 */


	return;
}

void maxSearch(short *rowMax, short *colMax, short *rowMax2, short *colMax2,
	unsigned short *row, unsigned short *col, DR_Det_t *pTab)
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



BOOL strengthCheck(short rowMax, short colMax, DR_Det_t *pTab)
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
short DR_ulaw2Linear(unsigned char uVal)
{
	short		t;
	
	uVal = ~uVal;
	t = ((uVal & QUANT_MASK) << 3) + BIAS;
	t <<= ((unsigned)uVal & SEG_MASK) >> SEG_SHIFT;

	return ((uVal & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}

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
short DR_alaw2Linear(unsigned char	aVal)
{
	short		t;
	short		seg;
	
	aVal ^= 0x55;
	t = (aVal & QUANT_MASK) << 4;
	seg = ((unsigned)aVal & SEG_MASK) >> SEG_SHIFT;
	switch (seg)
	{
	case 0:
		t += 8;
		break;
		
	case 1:
		t += 0x108;
		break;
		
	default:
		t += 0x108;
		t <<= seg - 1;
	}
	
	return ((aVal & SIGN_BIT) ? t : -t);
}
/**************************************************************************************************/
/*                                           PUBLIC_FUNCTIONS                                     */
/**************************************************************************************************/
/* 
 * fn		void DR_DetectInit()
 * brief	call init() to initialize a DR_Det_t tab
 * details	
 *
 * param[in]	
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
void DR_DetectInit()
{
	DR_init(&m_DR_Tab);	
}

/* 
 * fn		BOOL DR_Detect(unsigned char *framBuf, int frameLen, unsigned int rtpPayloadType, unsigned short *pNum) 
 * brief	Parse a dtmf number from a frame buffer.
 * details	
 *
 * param[in]	framBuf 
 * param[in]	frameLen 
 * param[in]	rtpPayloadType
 * param[out]	pNum : dtmf number.
 *
 * return	
 * retval	
 *
 * note		
 */
BOOL DR_Detect(unsigned char *framBuf, int frameLen, unsigned int rtpPayloadType, unsigned short *pNum)
{
	int i;
	short rowMax, colMax, rowMax2, colMax2;
	unsigned short row, col, numMap, currentNum = 0;
	const short *pKEYS = DR_KEYS;
	BOOL ret = FALSE;
	short linearCode[MAX_FRAME_SIZE]={0};
	if(frameLen>MAX_FRAME_SIZE)
	{
		HL_DR_PRINT(" frameLen:%d is too long!!!",frameLen);
		return FALSE;
	}
	for(i=0; i<frameLen; i++)
	{
		if (rtpPayloadType == RTP_PAYLOAD_PCMU)
		{
			linearCode[i] = DR_ulaw2Linear(framBuf[i]);
		} 
		else if (rtpPayloadType == RTP_PAYLOAD_PCMA)
		{
			linearCode[i] = DR_alaw2Linear(framBuf[i]);
		}
	}

	gaincrtl(linearCode, frameLen);

	for(i=0; i<frameLen; i++)
	{
		goertzel(linearCode[i], &m_DR_Tab);
	}

	getEnergy(&m_DR_Tab);

	maxSearch(&rowMax, &colMax, &rowMax2, &colMax2, &row, &col, &m_DR_Tab);

	if(strengthCheck(rowMax, colMax, &m_DR_Tab) == FALSE)
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
	if(currentNum == m_DR_Tab.digitLast)
	{
		ret = TRUE;
		if(pNum)
			*pNum = currentNum;
		m_DR_Tab.digitLast = 16;
		m_DR_Tab.pauseCnt = 0;
	}
	else
		m_DR_Tab.digitLast = currentNum;

	return ret;
}


/* 
 * fn		void DR_getNegPayloadType( pjmedia_session *session, DTMF_RECEIVER_RTPTYPE* pPayloadTypeDR )
 * brief	Get negotiation rtp payload type and telephone event type after SDP negotiation.
 * details	
 *
 * param[in]	session  SDP media session
 * param[out]	*pPayloadTypeDR  the buffer to get the payload types of voice and telephone event
 *
 * return	none
 * retval	
 *
 * note		yuchuwei modify @ 2011-4-4
 */
void DR_getNegPayloadType( pjmedia_session *session, DTMF_RECEIVER_RTPTYPE* pPayloadTypeDR )
{
	/*ycw-usbvm*/
	pj_assert(session);	
	/* Get payload type from session media info */	
	pPayloadTypeDR->SdpPayloadType = session->stream_info[0].tx_pt;
		pPayloadTypeDR->SdpTelEvtPt_RX= session->stream_info[0].rx_event_pt;
		pPayloadTypeDR->SdpTelEvtPt_TX = session->stream_info[0].tx_event_pt;
	HL_DR_PRINT("got SdpPayloadType(%d),SdpTelEvtPt_RX(%d), SdpTelEvtPt_TX(%d)",
	pPayloadTypeDR->SdpPayloadType,pPayloadTypeDR->SdpTelEvtPt_RX,pPayloadTypeDR->SdpTelEvtPt_TX);
}


/* 
 * fn		BOOL DR_sendCmMsgDTMF() 
 * brief	
 * details	
 *
 * param[in]	
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
void DR_sendCmMsgDTMF(int cnxId, unsigned short dtmfNum, CMSIP_DTMF_SEND_TYPE dtmfType)
{
CMSIP_DETECT_REMOTE_DTMF *pCmdetectedDTMF = NULL;
	CMSIP_MSG cmSipMsg;
	
	if ((cnxId < 0) || (cnxId >= PJSUA_MAX_CALLS))
	{
		return ;
	}
		/* Notify cm that sip has detected a remote DTMF */
		cmSipMsg.type = CMSIP_REQUEST_DETECT_REMOTE_DTMF;
		cmSipMsg.type |= CMSIP_MESSAGE_REQUEST;
		pCmdetectedDTMF = (CMSIP_DETECT_REMOTE_DTMF*)cmSipMsg.body;
		pCmdetectedDTMF->callIndex = cnxId;
		pCmdetectedDTMF->dtmfNum = dtmfNum;
		pCmdetectedDTMF->type = dtmfType;
		cmsip_sockSend(g_cmsip_cliSockfd, (char *)&cmSipMsg, sizeof(cmSipMsg));
#ifdef HL_DEBUG
		HL_DR_PRINT("sip sending msg:CMSIP_REQUEST_DETECT_REMOTE_DTMF. callId:%d, dtmf:%d", 
			cnxId, dtmfNum);
#endif

}
/**************************************************************************************************/
/*                                           GLOBAL_FUNCTIONS                                     */
/**************************************************************************************************/

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

