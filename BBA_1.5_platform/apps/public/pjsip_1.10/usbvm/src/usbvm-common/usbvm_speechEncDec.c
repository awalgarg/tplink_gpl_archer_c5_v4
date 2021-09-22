/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		usbvm_speechEncDec.c
 * brief		
 * details	
 *
 * author	Sirrain Zhang
 * version	
 * date		07Jun11
 *
 * history 	\arg	
 */

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>

#include <usbvm_speechEncDec.h>
#include <usbvm_recFileMang.h>
#include <usbvm_g711.h>

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */


/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/
#if(defined(USBVM_DEBUG)&&(USBVM_DEBUG==1))
#define  MARK_PRINT(args...) {printf("============USB VoiceMail============");printf(args);}
#else
#define  MARK_PRINT(args...)
#endif


#define	SIGN_BIT	(0x80)		/* Sign bit for a A-law byte. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */
#define	NSEGS		(8)			/* Number of A-law segments. */
#define	SEG_SHIFT	(4)			/* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */
#define	BIAS		(0x84)		/* Bias for linear code. */

#define G7xx_SWAP16( val ) {val = (((( val ) & 0xff00 ) >> 8)|((( val ) & 0x00ff ) << 8));}

/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/
/* 
 * brief	Sound format
 */
typedef struct
{
   int bytesPerFrame;       /* size of a base frame */
   int msecPerFrame;        /* duration in ms of a base frame */
   int samplesPerSec;       /* smapling frequency, 8k for narrow band codec, and 16k for wide band codec */
} SOUNDFORMAT;

/* 
 * brief	Codec attributes of encoder and decoder
 */
typedef struct
{	
   SOUNDFORMAT soundFormat;  /* sound format */
   int frameSizeToDecoder;   /* frame size to decoder */
   int rtpPayloadType;       /* rtp payload type */
   CODEC_TYPE eptCodec;      /* codec type */
} CODEC_ATTRIBUTE;

/**************************************************************************************************/
/*                                           EXTERN_PROTOTYPES                                    */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           LOCAL_PROTOTYPES                                     */
/**************************************************************************************************/
static int checkNodeInfo(USBVM_REC_FILE_LIST node1, USBVM_REC_FILE_LIST node2);
static int speechDecoder( FILE *fpInput, FILE *fpOutput, CODEC_TYPE codecType, int frameSize, BOOL edianChange );

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/
/* 
 * brief	This table is used to define format attributes of a supported codec
 */
static const CODEC_ATTRIBUTE codecAttributeTable[] =
{
   { {24, 30, 8000}, 240, RTP_PAYLOAD_G7231_63,  CODEC_G7231_63 },
   { {10, 5,  8000}, 100, RTP_PAYLOAD_G726_16, CODEC_G726_16  },
   { {15, 5,  8000}, 150, RTP_PAYLOAD_G726_24, CODEC_G726_24  },
   { {20, 5,  8000}, 200, RTP_PAYLOAD_G726_32,  CODEC_G726_32  },
   { {25, 5,  8000}, 250, RTP_PAYLOAD_G726_40, CODEC_G726_40  },
   { {10, 10, 8000}, 10,  RTP_PAYLOAD_G729A, CODEC_G729A    },
   { {40, 5,  8000}, 400, RTP_PAYLOAD_PCMU,  CODEC_PCMU     },
   { {40, 5,  8000}, 400, RTP_PAYLOAD_PCMA,  CODEC_PCMA     },
   { {0,  0,  0},    0,   RTP_PAYLOAD_NULL,  CODEC_NULL     }  /* This line is required to indicate the end of the list */

};

/**************************************************************************************************/
/*                                           LOCAL_FUNCTIONS                                      */
/**************************************************************************************************/
/* 
 * fn		static int checkNodeInfo(USBVM_REC_FILE_LIST node1, USBVM_REC_FILE_LIST node2)
 * brief	Compare the information of node1 and node2
 * details	
 *
 * param[in]	node1  record node
 * param[in]	node2  record node
 * param[out]	
 *
 * return	equal return 0 else return -1
 * retval	
 *
 * note		
 */
static int checkNodeInfo(USBVM_REC_FILE_LIST node1, USBVM_REC_FILE_LIST node2)
{
	int rc = 0;

	if ((node1->fileTimName != node2->fileTimName) 
		 || (node1->fileSize != node2->fileSize)
		 || (node1->fileDuration != node2->fileDuration)
		 || (node1->payloadType != node2->payloadType)
		 || (node1->readFlag != node2->readFlag)
		 || (strcmp(node1->srcNum, node2->srcNum) != 0)
		 || (strcmp(node1->dstNum, node2->dstNum) != 0))
	{
		rc = -1;
	}

	return rc;
}

/* 
 * fn		static int speechDecoder( FILE *fpInput, FILE *fpOutput, CODEC_TYPE codecType, int frameSize, BOOL edianChange )
 * brief	Speech decoder transformation processing
 * details	
 *
 * param[in]	*fpInput  pointer of input file 
 * param[in]	*fpOutput  pointer of output file
 * param[in]	frameSize  fram size for codec transformation
 * param[in]	codecType  codec type
 * param[in]	endianChange  to use big endian or little endian
 * param[out]	
 *
 * return	success return 0, fail return -1
 * retval	
 *
 * note		
 */
static int speechDecoder( FILE *fpInput, FILE *fpOutput, CODEC_TYPE codecType, int frameSize, BOOL edianChange )
{
	unsigned char frameBuf[512];
	
	switch (codecType)
	{
	case CODEC_PCMU:
		if (fread(frameBuf, frameSize, 1, fpInput) == 1)
		{
			fwrite(frameBuf, frameSize, 1, fpOutput);
		}
		break;
		
	case CODEC_PCMA:
		if (fread(frameBuf, frameSize, 1, fpInput) == 1)
		{
			fwrite(frameBuf, frameSize, 1, fpOutput);
		}
		break;

	default:
		break;
	}

	return 0;
}

/**************************************************************************************************/
/*                                           PUBLIC_FUNCTIONS                                     */
/**************************************************************************************************/
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
int usbvm_createWavHeader( FILE *fpWav, int formatTag, int channels, int samplerate, int resolution )
{
    int length;
    WAVEAUDIOFORMAT format;
	MARK_PRINT("fpWav=NULL %s,formatTag %d,channel %d,samplerate %d,resolution %d ",
		fpWav==NULL?"T":"F",
		formatTag,
		channels,
		samplerate,
		resolution);
    /* Use a zero length for the chunk sizes for now, then modify when finished */

    if(NULL == fpWav)
	{
        return -1;
	}

    format.format_tag = formatTag;
	format.channels = channels;
	format.samplerate = samplerate;
	format.bits_per_sample = resolution;
	format.blockalign = channels * (resolution/8);
	format.bytes_per_second = format.samplerate * format.blockalign;
	
#if PJ_IS_BIG_ENDIAN
	WAV_SWAP16(format.format_tag);    
	WAV_SWAP16(format.channels);   
	WAV_SWAP32(format.samplerate);   
	WAV_SWAP16(format.bits_per_sample);    
	WAV_SWAP16(format.blockalign);    
	WAV_SWAP32(format.bytes_per_second);
#endif
	
    fseek(fpWav, 0, SEEK_SET);

    fwrite("RIFF\0\0\0\0WAVEfmt ", sizeof(char), 16, fpWav); /* Write RIFF, WAVE, and fmt  headers */

    length = 16;  /* what we want the order in file is 10H00H00H00H */

#if PJ_IS_BIG_ENDIAN
	WAV_SWAP32(length);
#endif 

    fwrite(&length, 1, sizeof(long), fpWav); 				/* Length of Format (always 16) */
    fwrite(&format, 1, sizeof(format), fpWav);

    fwrite("data\0\0\0\0", sizeof(char), 8, fpWav); 		/* Write data chunk */

    return 0;
}


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
int usbvm_updateWavHeader( FILE *fpWav )
{
    int filelen, riff_length, data_length;

    if(NULL == fpWav)
	{
        return -1;
	}
	
    /* Get the length of the file */
    fseek( fpWav, 0, SEEK_END );
    filelen = ftell( fpWav );

    if(filelen <= 0)
	{
        return -1;
	}

    riff_length = filelen - 8;
    data_length = filelen - 44;

	/* Modify riff length */
    fseek(fpWav, 4, SEEK_SET);

#if PJ_IS_BIG_ENDIAN
	WAV_SWAP32(riff_length);
#endif 

    fwrite(&riff_length, 1, sizeof(long), fpWav);
	
	/* Modify wav data length */
    fseek(fpWav, 40, SEEK_SET);

#if PJ_IS_BIG_ENDIAN
	WAV_SWAP32(data_length);
#endif 

    fwrite(&data_length, 1, sizeof(long), fpWav);

    /* Reset file position for appending data */
    fseek(fpWav, 0, SEEK_END);

    return 0;
}

/* 
 * fn		void usbvm_speechToWavFormatByPos ( int endpt, int nodePos, BOOL bSleep )
 * brief	Transformtion from record speech file to wav format file
 * details	
 *
 * param[in]	endpt  FXS endpoint number
 * param[in]	fileTimeName  name of the record file to be transformed
 * param[in]	bSleep  need sleep or not
 * param[out]
 *
 * return	
 * retval	
 *
 * note		
 */
void usbvm_changeFormatToWav ( int endpt, int fileTimeName, BOOL bSleep)
{
	int i = 0;
	int formatTag = 0;
	int sampleRate = 0;
	int resolution = 0;
	int bytesPerFrame = 0;
	int fileSizeLeft = 0;
	unsigned int filesize = 0;
	char wavFilePathName[200];
	char wavFileName[100];
	char cmd[100];
	BOOL formatMatch = FALSE;
	CODEC_TYPE codecType = CODEC_NULL; 
	FILE *fpWav = NULL;
	FILE *fpRecord = NULL;
	char recFilePathName[100];
	USBVM_REC_FILE_NODE recFileNode;
	/*  [Huang Lei] [14Nov12]  */
	MARK_PRINT("entering usbvm_changeFormatToWav()\n");

	sprintf(recFilePathName, "%s%08x", USB_REC_FILE_DIR, fileTimeName);
	if (access(recFilePathName, F_OK) == 0)
	{
		if (access(recFilePathName, R_OK | W_OK) != 0)
		{
			chmod(recFilePathName, 0777);										
		}

		if ((fpRecord = fopen(recFilePathName, "rb+")) != NULL)
		{
/*20130618_update*/
			if (fread_USBVM_REC_FILE_NODE(&recFileNode, sizeof(USBVM_REC_FILE_NODE), 1, fpRecord) != 1)
			{
				fclose(fpRecord);
				fpRecord = NULL;
				MARK_PRINT("recFilePathName:%s, fread failed!\n", recFilePathName);
				return;
			}

			/* If record index header is not correct, delete the record file */
			if (usbvm_chkSum(&recFileNode) != 0)
			{
				fclose(fpRecord);
				fpRecord = NULL;
				MARK_PRINT("recFilePathName:%s is not correct, change to wav failed !\n", recFilePathName);
				return;
			}
		}
		else
		{
			MARK_PRINT("recFilePathName:%s, can't read !\n", recFilePathName);
			return;
		}
	}
	else
	{
		MARK_PRINT("recFilePathName:%s doesn't exist!\n", recFilePathName);
		return;
	}

	/* Search the codecFormatTable to find eptCodec and get its attributes */
	while (codecAttributeTable[i].eptCodec != CODEC_NULL)
	{
		if (codecAttributeTable[i].rtpPayloadType == recFileNode.payloadType)
		{
			formatMatch = TRUE;
			break;
		}
		i++;
	}
	/* If the codec is supported, then do transformation */
	if (formatMatch)
	{
		sampleRate = codecAttributeTable[i].soundFormat.samplesPerSec;
		codecType = codecAttributeTable[i].eptCodec;
		fileSizeLeft = recFileNode.fileSize;
		MARK_PRINT("fileSize is %d\n",fileSizeLeft);
		switch (codecType)
		{
		case CODEC_PCMU:
			formatTag = PCMU_FORMAT_TAG;
			resolution = PCM_RESOLUTION_8;
			break;
		case CODEC_PCMA:
			formatTag = PCMA_FORMAT_TAG;
			resolution = PCM_RESOLUTION_8;
			break;
		default:
			formatTag = PCM_FORMAT_TAG;
			resolution = PCM_RESOLUTION_16;
			break;
		}

		/* Create directory for wav file if the directory doesn't exist */
		strcpy(wavFilePathName, USB_WAV_FILE_DIR);
		if (access(wavFilePathName, F_OK) !=0)
		{
			sprintf(cmd,"mkdir -p %s",wavFilePathName);
			system(cmd);
			chmod(wavFilePathName, 0777);
		}
		/* Wav file name format is xxx_xxx_xxxx-xx-xx xx.xx'xx'' */
		usbvm_wavNameFormat(recFileNode, wavFileName);
		strcat(wavFilePathName, wavFileName);

		MARK_PRINT("usbvm_speechToWavFormatByName:%s\n", wavFilePathName);
		/* If the wav file doesn't exist, do transformation */
		if (access(wavFilePathName, F_OK) !=0)
		{
			fseek(fpRecord, sizeof(USBVM_REC_FILE_NODE), SEEK_SET);
			if ((fpWav = fopen(wavFilePathName, "wb+")) == NULL)
			{
				MARK_PRINT("Failed to open %s\n", wavFilePathName);
				fclose(fpRecord);
				fpRecord = NULL;
				return ;
			}
			/* Create wav file header */
			usbvm_createWavHeader(fpWav, formatTag, PCM_CHANNEL_MONO, sampleRate, resolution);
			/* Do decoder transformation */

			while (fileSizeLeft > 0)
			{
				if (usbvm_checkUSBDevExist())
				{
					/* Make sure there is no recording or playing in process */
#if 0
					if ((usbvm_checkUSBvoiceMailIsIDLE() == 0))
#endif
					{
						/* 
						* brief	frameSizeToDecoder <= 400
						*/
						if (fileSizeLeft >= codecAttributeTable[i].frameSizeToDecoder)
						{
							bytesPerFrame = codecAttributeTable[i].frameSizeToDecoder;
						}
						else
						{
							bytesPerFrame = fileSizeLeft;
						}							
						/* Decoder tranformation for each frame */
						speechDecoder(fpRecord, fpWav, codecType, bytesPerFrame, TRUE);
						fileSizeLeft -= bytesPerFrame;
						/* sleep after each frame transformation finished */
						if (bSleep)
						{
							usleep(5 * 1000);
						}
					}
				}
				else
				{/*  [Huang Lei] [14Nov12]  */
					MARK_PRINT("usb device can not be found!!!exit transforming wav");
					break;
				}
			}
			/* Update wav file header when transformation finished */
			usbvm_updateWavHeader(fpWav);
			fclose(fpWav);
			filesize = getFileSize(wavFilePathName);
			if(filesize != 0)
			{
				usbvm_rewriteUsedSize(filesize, TRUE);
			}
		}
		else
		{
			MARK_PRINT("file:%s has already existed\n",wavFilePathName);
		}
		/*
		usbvm_copyFromRAMToUSB(endpt, wavFileName, WAV_TYPE);
		*/
		MARK_PRINT("usbvm_speechToWavFormatByName finished\n");
	}

	if (fpRecord != NULL)
	{
		fclose(fpRecord);
		fpRecord=NULL;
	}
}

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
int usbvm_changePcmToWav(char *pSrcFile, char *pDstFile)
{
	int bytesPerFrame = 0;
	int fileSizeLeft = 0;
	unsigned int filesize = 0;
	FILE *fpWav = NULL;
	FILE *fpRecord = NULL;
	
	if (access(pSrcFile, F_OK) == 0)
	{
		if ((fpRecord = fopen(pSrcFile, "r")) == NULL)
		{
			MARK_PRINT("pSrcFile:%s, can't read !\n", pSrcFile);
			return -1;
		}
	}
	else
	{
		MARK_PRINT("pSrcFile:%s, does not exist!\n", pSrcFile);
		return -1;
	}

	fileSizeLeft = getFileSize(pSrcFile);

	/* If the wav file doesn't exist, do transformation */
	if (access(pDstFile, F_OK) !=0)
	{
		if ((fpWav = fopen(pDstFile, "wb+")) == NULL)
		{
			MARK_PRINT("Failed to open %s\n", pDstFile);
			fclose(fpRecord);
			fpRecord = NULL;
			return -1;
		}
		
		/* Create wav file header */
		usbvm_createWavHeader(fpWav, PCMU_FORMAT_TAG, PCM_CHANNEL_MONO, 8000, PCM_RESOLUTION_8);
		/* Do decoder transformation */
		while (fileSizeLeft > 0)
		{
			if (fileSizeLeft >= 400)
			{
				bytesPerFrame = 400;
			}
			else
			{
				bytesPerFrame = fileSizeLeft;
			}							
			/* Decoder tranformation for each frame */
			speechDecoder(fpRecord, fpWav, CODEC_PCMU, bytesPerFrame, TRUE);
			fileSizeLeft -= bytesPerFrame;
		}
		/* Update wav file header when transformation finished */
		usbvm_updateWavHeader(fpWav);
		
		fclose(fpWav);
	}
	else
	{
		MARK_PRINT("file:%s has already existed\n", pDstFile);
	}

	if (fpRecord != NULL)
	{
		fclose(fpRecord);
		fpRecord=NULL;
	}

	return 0;
}

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
int usbvm_speechToWavFormatByName ( int endpt, unsigned int fileTimName, BOOL bSleep )
{
	USBVM_REC_FILE_NODE node;
	USBVM_REC_FILE_NODE recNodeTmp;
	int nodePos = 0;
	int i;
	FILE* fpIndex = NULL;
	int lockRet = -1;
	int rc =0;
	char recFilePathName[100];
	FILE* fpRec =NULL;
	int recNumInAll = 0;
	
	lockRet = usbvm_openIndexFpAndLock(endpt, &fpIndex, "rb+", WR_LOCK_WHOLE);
	if(lockRet == -1)
	{
		MARK_PRINT("HL_DBG: Lock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
		return -1;
	}
		recNumInAll = usbvm_getRecNumInAll(endpt, fpIndex);
		/* Find node position accroding to file name */
		if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE, SEEK_SET) == -1)
		{				
			lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
			if(lockRet == -1)
			{
				MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			}
			return -1;
		}
		for (i = 1; i <= recNumInAll; i++)
		{
/*20130618_update*/
			if (fread_USBVM_REC_FILE_NODE(&node, sizeof(USBVM_REC_FILE_NODE), 1, fpIndex) != 1)
			{
				lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);	
				if(lockRet == -1)
				{
					MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
				}
				return -1;
			}

			if (node.fileTimName == fileTimName)
			{
				nodePos = i;
				break;
			}
		}

		if (i > recNumInAll)
		{
			lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
			if(lockRet == -1)
			{
				MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			}
			return -1;
		}
#if 1
	if (usbvm_chkSum(&node) != 0)
	{
		rc = -1;
	}
	/* If the record index node is correct */
	if (rc == 0)
	{
		/* Check whether the record file header is correct */	
		sprintf(recFilePathName, "%s%08x", USB_REC_FILE_DIR, node.fileTimName);
		if (access(recFilePathName, F_OK) == 0)
		{
			if (access(recFilePathName, R_OK | W_OK) != 0)
			{
				chmod(recFilePathName, 0777);										
			}
		
			if ((fpRec = fopen(recFilePathName, "rb+")) != NULL)
			{
/*20130618_update*/
				if (fread_USBVM_REC_FILE_NODE(&recNodeTmp, sizeof(USBVM_REC_FILE_NODE), 1, fpRec) != 1)
				{
					fclose(fpRec);
					fpRec=NULL;
					lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
					if(lockRet == -1)
					{
						MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
					}
					return -1;
				}
				
				/* If record index header is not correct, delete the record file */
				if (usbvm_chkSum(&recNodeTmp) != 0)
				{
					MARK_PRINT("HL_DBG:record index header is not correct! in %s: %d \n", __FUNCTION__,__LINE__);
					fclose(fpRec);
					fpRec=NULL;
					lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
					if(lockRet == -1)
					{
						MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
					}
					return -1;
				}
				/* If record index header info is not compatible with record index node info, 
				delete the record file */
				else if (checkNodeInfo(&node, &recNodeTmp) != 0)
		{
					MARK_PRINT("HL_DBG:record index header is not correct! in %s: %d \n", __FUNCTION__,__LINE__);
					fclose(fpRec);
					fpRec=NULL;
					usbvm_delRecIndexListByPos(endpt, nodePos,fpIndex);
					rc = -3;
		}

			}
			else
			{
				fclose(fpRec);
				fpRec=NULL;
				lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
				if(lockRet == -1)
				{
					MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
				}
				return -1;
			}
		}
		else /*can NOT access record file*/
		{
			MARK_PRINT("recFilePathName:%s doesn't exist!\n", recFilePathName);
			rc = usbvm_delRecIndexListOnly(endpt, nodePos,fpIndex);
			if (rc < 0)
			{
				lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
				if(lockRet == -1)
				{
					MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
				}
				return -1;
			}
		}
	}
	else  /* If the record index node is not correct, delete it from record index list */
	{
		rc = usbvm_delRecIndexListOnly(endpt, nodePos,fpIndex);
		if (rc < 0)
		{
			lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
			if(lockRet == -1)
			{
				MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			}
			return -1;
		}
	}
#endif
		/* Record node exist in record index list */
		lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
		usbvm_changeFormatToWav(endpt, node.fileTimName, bSleep);
		if(lockRet == -1)
		{
			MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
	}
		if(fpRec!=NULL)
		{
			fclose(fpRec);
			fpRec = NULL;
		}
	return rc;
}

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
void  usbvm_G711AToG711U( char *pInputBuf, char *pOutputBuf, int frameSize )
{	
	int i;
	
	for (i = 0; i < frameSize; i++)
	{
		pOutputBuf[i] = alaw2Ulaw(pInputBuf[i]);
	}

}


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
void  usbvm_G711UToG711A( char *pInputBuf, char *pOutputBuf, int frameSize )
{	
	int i;
	
	for (i = 0; i < frameSize; i++)
	{
		pOutputBuf[i] = ulaw2Alaw(pInputBuf[i]);
	}

}

/**************************************************************************************************/
/*                                           GLOBAL_FUNCTIONS                                     */
/**************************************************************************************************/

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */


