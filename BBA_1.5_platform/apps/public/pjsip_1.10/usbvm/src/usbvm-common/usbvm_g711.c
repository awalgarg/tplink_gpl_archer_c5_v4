/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		usbvm_g711.c
 * brief		
 * details	
 *
 * author	Sirrain Zhang
 * version	
 * date		03Jun11
 *
 * history 	\arg	
 */

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

/**************************************************************************************************/
/*                                           EXTERN_PROTOTYPES                                    */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           LOCAL_PROTOTYPES                                     */
/**************************************************************************************************/
static short search(short val, short *table, short size);

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/
/* 
 * brief	A律编码解码预制表
 */
static short l_segAend[8] = {0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF};

/* 
 * brief	U律编码解码预制表
 */
static short l_segUend[8] = {0x3F, 0x7F, 0xFF, 0x1FF,0x3FF, 0x7FF, 0xFFF, 0x1FFF};

/* 
 * brief 	U律到A律转化编码表
 */
static unsigned char l_u2a[128] = {
	1,	1,	2,	2,	3,	3,	4,	4,
	5,	5,	6,	6,	7,	7,	8,	8,
	9,	10,	11,	12,	13,	14,	15,	16,
	17,	18,	19,	20,	21,	22,	23,	24,
	25,	27,	29,	31,	33,	34,	35,	36,
	37,	38,	39,	40,	41,	42,	43,	44,
	46,	48,	49,	50,	51,	52,	53,	54,
	55,	56,	57,	58,	59,	60,	61,	62,
	64,	65,	66,	67,	68,	69,	70,	71,
	72,	73,	74,	75,	76,	77,	78,	79,
	80,	82,	83,	84,	85,	86,	87,	88,
	89,	90,	91,	92,	93,	94,	95,	96,
	97,	98,	99,	100,	101,	102,	103,	104,
	105,	106,	107,	108,	109,	110,	111,	112,
	113,	114,	115,	116,	117,	118,	119,	120,
	121,	122,	123,	124,	125,	126,	127,	128};
	
/* 
 * brief	A律到U律转化编码表
 */
static unsigned char l_a2u[128] = {	
	1,	3,	5,	7,	9,	11,	13,	15,
	16,	17,	18,	19,	20,	21,	22,	23,
	24,	25,	26,	27,	28,	29,	30,	31,
	32,	32,	33,	33,	34,	34,	35,	35,
	36,	37,	38,	39,	40,	41,	42,	43,
	44,	45,	46,	47,	48,	48,	49,	49,
	50,	51,	52,	53,	54,	55,	56,	57,
	58,	59,	60,	61,	62,	63,	64,	64,
	65,	66,	67,	68,	69,	70,	71,	72,
	73,	74,	75,	76,	77,	78,	79,	80,
	80,	81,	82,	83,	84,	85,	86,	87,
	88,	89,	90,	91,	92,	93,	94,	95,
	96,	97,	98,	99,	100,	101,	102,	103,
	104,	105,	106,	107,	108,	109,	110,	111,
	112,	113,	114,	115,	116,	117,	118,	119,
	120,	121,	122,	123,	124,	125,	126,	127};

/**************************************************************************************************/
/*                                           LOCAL_FUNCTIONS                                      */
/**************************************************************************************************/
/* 
 * fn		static short search(short val, short *table, short size)
 * brief	该子程序寻找段落码
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
static short search(short val, short *table, short size)
{  
	short i;
	for (i = 0; i < size; i++) 
	{
		if (val <= *table++)
		{
			return (i);
		}
	}
	
	return (size);
}



/**************************************************************************************************/
/*                                           PUBLIC_FUNCTIONS                                     */
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
 /*  输入与输出码型对应表
 *	线性码输入       	   非线性码输出
 *	----------------      --------------
 *	0000000wxyza			000wxyz
 *	0000001wxyza			001wxyz
 *	000001wxyzab			010wxyz
 *	00001wxyzabc			011wxyz
 *	0001wxyzabcd			100wxyz
 *	001wxyzabcde			101wxyz
 *	01wxyzabcdef			110wxyz
 *	1wxyzabcdefg			111wxyz
 */
unsigned char linear2Alaw(short pcmVal)
{
	short		mask;
	short		seg;
	unsigned char	aval;

	pcmVal = pcmVal >> 3;

	if (pcmVal >= 0)
	{
		mask = 0xD5;		/* sign (7th) bit = 1 */
	} 
	else 
	{
		mask = 0x55;		/* sign bit = 0 */
		pcmVal = -pcmVal - 1;
	}
	/* 按照输入绝对值求取段落号. */
	seg = search(pcmVal, l_segAend, 8);
	
	/* 将求取的符号位, 段落号, 段内量化值组成一个8比特数输出 */
	if (seg >= 8)
	{
		return (unsigned char) (0x7F ^ mask);
	}
	else 
	{
		aval = (unsigned char) seg << SEG_SHIFT;
		if (seg < 2)
		{
			aval |= (pcmVal >> 1) & QUANT_MASK;
		}
		else
		{
			aval |= (pcmVal >> seg) & QUANT_MASK;
		}
		
		return (aval ^ mask);
	}
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
short alaw2Linear(unsigned char	aVal)
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
 /*
 *	线性码输入       	   非线性码输出
 *	----------------      --------------
 *	00000001wxyza			000wxyz
 *	0000001wxyzab			001wxyz
 *	000001wxyzabc			010wxyz
 *	00001wxyzabcd			011wxyz
 *	0001wxyzabcde			100wxyz
 *	001wxyzabcdef			101wxyz
 *	01wxyzabcdefg			110wxyz
 *	1wxyzabcdefgh			111wxyz
 */
unsigned char linear2Ulaw(short pcmVal)
{
	short		mask;
	short		seg;
	unsigned char	uval;

	/* Get the sign and the magnitude of the value. */
	pcmVal = pcmVal >> 2;
	if (pcmVal < 0)
	{
		pcmVal = -pcmVal;
		mask = 0x7F;
	} 
	else 
	{
		mask = 0xFF;
	}
    
	if ( pcmVal > CLIP ) pcmVal = CLIP;		/* 削波 */
	pcmVal += (BIAS >> 2);

	/* Convert the scaled magnitude to segment number. */
	seg = search(pcmVal, l_segUend, 8);
	
	/* 将求取的符号位, 段落号, 段内量化值组成一个8比特数输出 */
	if (seg >= 8)
	{
		return (unsigned char) (0x7F ^ mask);
	}
	else 
	{	
		uval = (unsigned char) (seg << 4) | ((pcmVal >> (seg + 1)) & 0xF);
		
		return (uval ^ mask);
	}

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
short ulaw2Linear(unsigned char uVal)
{
	short		t;
	
	uVal = ~uVal;
	t = ((uVal & QUANT_MASK) << 3) + BIAS;
	t <<= ((unsigned)uVal & SEG_MASK) >> SEG_SHIFT;

	return ((uVal & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}


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
unsigned char alaw2Ulaw(unsigned char aVal)
{
	aVal &= 0xff;
	
	return (unsigned char) ((aVal & 0x80) 
		? (0xFF ^ l_a2u[aVal ^ 0xD5]) : (0x7F ^ l_a2u[aVal ^ 0x55]));
}


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
unsigned char ulaw2Alaw(unsigned char uVal)
{
	uVal &= 0xff;
	
	return (unsigned char) ((uVal & 0x80) 
		? (0xD5 ^ (l_u2a[0xFF ^ uVal] - 1)) : (unsigned char) (0x55 ^ (l_u2a[0x7F ^ uVal] - 1)));
}

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

