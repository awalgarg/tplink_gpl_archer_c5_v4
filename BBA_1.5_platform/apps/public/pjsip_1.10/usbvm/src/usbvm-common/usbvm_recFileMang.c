/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		usbvm_recFileMang.c
 * brief		
 * details	
 *
 * author	Sirrain Zhang
 * version	
 * date		03Jun11
 *
 * history 	\arg	09Sep11,huanglei,modified usbvm_insertRecIndexList(),added some comments
 */

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <string.h>
#include <sys/time.h>
#include <dirent.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <pthread.h>
#include <arpa/inet.h>

#include <usbvm_recFileMang.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#define __LINUX_OS_FC__
#define INCLUDE_DECT
#ifndef INCLUDE_VOIP
#define INCLUDE_VOIP
#endif
#include "os_msg.h"

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */
/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/
#ifndef USBVM_DEBUG
#define USBVM_DEBUG 0
#endif
/* ifdef DEBUG_DETAILED, not only the errors, but also the details of functions will be printed */
#if (USBVM_DEBUG >= 2)
#define DEBUG_DETAILED
#endif
#if (USBVM_DEBUG >= 6)
#define DEBUG_DETAILED_HIGH	1
#endif /* USBVM_DEBUG==6 */


#if USBVM_DEBUG
#define  MARK_PRINT(args...) {printf("=== USBVM FILE %d ===", __LINE__);printf(args);}
#else 
#define  MARK_PRINT(args...)
#endif
#if USBVM_DEBUG
/* 
 * brief	assert only in this file
 */
#define USBVM_ASSERT(x)					   \
   do { 							   \
	   if (!(x)){					   \
		printf("HL_DBG: usbvm assert! %s:%d \033[0;33;41m (%s)--- \033[0m \n" , __FILE__, __LINE__,#x);\
	 	abort();\
	   }\
   } while(0)
#else
#define USBVM_ASSERT(x)	
#endif


/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           EXTERN_PROTOTYPES                                    */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           LOCAL_PROTOTYPES                                     */
/**************************************************************************************************/
static int checkNodeInfo(USBVM_REC_FILE_LIST node1, USBVM_REC_FILE_LIST node2);
static void strToLower( char *pstr );
static BOOL checkRecFileName( char *pstr );
static int filter( const struct dirent *pdir );
static void wavFIFOInit( void );
static void reBuildRecIndexList( int endpt, FILE* fpIndex);
static void sigAlrm(int signo);

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/
static int l_recCurNode[ENDPT_NUM];
static char l_recFilePathName[ENDPT_NUM][200];

static int l_recNumInAll;
static int l_recNotReadStartPos;
static int l_recNotReadNum;

static FILE *l_fpRecIndex;
static pthread_mutex_t l_mutexIndexFileOperation;

static USBVM_WAV_FIFO_INFO l_wavFIFOBuf[WAV_FIFO_SIZE];
static int l_fifoBufHead;
static int l_fifoBufTail;
static int l_fifoBufState;


/**************************************************************************************************/
/*                                           LOCAL_FUNCTIONS                                      */
/**************************************************************************************************/
static int sendMsgToVoipServer(USBVM_STATUC_CHANGE_TYPE type, int unReadNum)
{
    CMSG_FD msgFd;
	CMSG_BUFF msg;
	USBVM_RECORD_STATUS_CHANGE_MSG *pStatusChnageMsg;

    memset(&msgFd, 0 , sizeof(CMSG_FD));
	memset(&msg, 0 , sizeof(CMSG_BUFF));

	msg.type = CMSG_VOIP_USBMAIL_UNREAD_COUNT;

	pStatusChnageMsg = (USBVM_RECORD_STATUS_CHANGE_MSG *)msg.content;
	pStatusChnageMsg->type = type;
	pStatusChnageMsg->unreadCount = unReadNum;

	if (0 != msg_connCliAndSend(CMSG_ID_VOIP, &msgFd, &msg))
    {
        printf("sendMsgToVoipServer: send message error!\n");
        return 0;
    }

	return 1;
}


/*
 * brief		read USBVM_REC_FILE_NODE from a file.
 *
 * note		BosaZhong@25Apr2013, add, for compatible.
 */
size_t fread_USBVM_REC_FILE_NODE(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	int ret = 0;
	USBVM_REC_FILE_NODE tmpNode;
	USBVM_REC_FILE_NODE * pOut = NULL;

	USBVM_ASSERT(sizeof(USBVM_REC_FILE_NODE) == (size * nmemb));

	ret = fread(&tmpNode, sizeof(USBVM_REC_FILE_NODE), 1, stream);
	if (ret != 1)
	{
		return -1;
	}

	pOut = (USBVM_REC_FILE_NODE *)ptr;

	pOut->fileTimName  = (unsigned int) ntohl(tmpNode.fileTimName);
	pOut->fileSize     = (unsigned int) ntohl(tmpNode.fileSize);
	pOut->fileDuration = (unsigned int) ntohl(tmpNode.fileDuration);

	memcpy(pOut->srcNum, tmpNode.srcNum, sizeof(char) * MAX_DIGITS_NUM);
	memcpy(pOut->dstNum, tmpNode.dstNum, sizeof(char) * MAX_DIGITS_NUM);

	pOut->payloadType  = (unsigned int) ntohl(tmpNode.payloadType);
	pOut->readFlag     = (unsigned int) ntohl(tmpNode.readFlag);
	pOut->checkSum     = (unsigned int) ntohl(tmpNode.checkSum);

#ifdef DEBUG_DETAILED_HIGH
	MARK_PRINT("\n\n");
	MARK_PRINT("-------- read  USBVM_REC_FILE_NODE(host side) ----------\n");
	MARK_PRINT("fileTimName  = %10u - 0x%x\n", pOut->fileTimName, pOut->fileTimName);
	MARK_PRINT("fileSize     = %10u - 0x%x\n", pOut->fileSize, pOut->fileSize);
	MARK_PRINT("fileDuration = %10u - 0x%x\n", pOut->fileDuration, pOut->fileDuration);
	MARK_PRINT("srcNum       = (%s)\n", pOut->srcNum);
	MARK_PRINT("dstNum       = (%s)\n", pOut->dstNum);
	MARK_PRINT("payloadType  = %10u - 0x%x\n", pOut->payloadType, pOut->payloadType);
	MARK_PRINT("readFlag     = %10u - 0x%x\n", pOut->readFlag, pOut->readFlag);
	MARK_PRINT("checkSum     = %10u - 0x%x\n", pOut->checkSum, pOut->checkSum);
	MARK_PRINT("-------- endof  USBVM_REC_FILE_NODE(host side) ---------\n");
	MARK_PRINT("\n\n");
#endif /* DEBUG_DETAILED_HIGH */	
	return ret;	
}


/*
 * brief		write USBVM_REC_FILE_NODE from a file.
 *
 * note		BosaZhong@25Apr2013, add, for compatible.
 */
size_t fwrite_USBVM_REC_FILE_NODE(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	int ret = 0;
	USBVM_REC_FILE_NODE tmpNode;
	USBVM_REC_FILE_NODE * pSrc = NULL;

	USBVM_ASSERT(sizeof(USBVM_REC_FILE_NODE) == (size * nmemb));

	pSrc = (USBVM_REC_FILE_NODE *) ptr;


	tmpNode.fileTimName  = (unsigned int) htonl(pSrc->fileTimName);
	tmpNode.fileSize     = (unsigned int) htonl(pSrc->fileSize);
	tmpNode.fileDuration = (unsigned int) htonl(pSrc->fileDuration);

	memcpy(tmpNode.srcNum, pSrc->srcNum, sizeof(char) * MAX_DIGITS_NUM);
	memcpy(tmpNode.dstNum, pSrc->dstNum, sizeof(char) * MAX_DIGITS_NUM);

	tmpNode.payloadType  = (unsigned int) htonl(pSrc->payloadType);
	tmpNode.readFlag     = (unsigned int) htonl(pSrc->readFlag);
	tmpNode.checkSum     = (unsigned int) htonl(pSrc->checkSum);

	ret = fwrite(&tmpNode, sizeof(USBVM_REC_FILE_NODE), 1, stream);

#ifdef DEBUG_DETAILED_HIGH
	MARK_PRINT("\n\n");
	MARK_PRINT("---------write USBVM_REC_FILE_NODE(host side) ----------\n");
	MARK_PRINT("fileTimName  = %10u - 0x%x\n", pSrc->fileTimName, pSrc->fileTimName);
	MARK_PRINT("fileSize     = %10u - 0x%x\n", pSrc->fileSize, pSrc->fileSize);
	MARK_PRINT("fileDuration = %10u - 0x%x\n", pSrc->fileDuration, pSrc->fileDuration);
	MARK_PRINT("srcNum       = (%s)\n", pSrc->srcNum);
	MARK_PRINT("dstNum       = (%s)\n", pSrc->dstNum);
	MARK_PRINT("payloadType  = %10u - 0x%x\n", pSrc->payloadType, pSrc->payloadType);
	MARK_PRINT("readFlag     = %10u - 0x%x\n", pSrc->readFlag, pSrc->readFlag);
	MARK_PRINT("checkSum     = %10u - 0x%x\n", pSrc->checkSum, pSrc->checkSum);
	MARK_PRINT("-------- endof  USBVM_REC_FILE_NODE(host side) ---------\n");
	MARK_PRINT("\n\n");
#endif /* DEBUG_DETAILED_HIGH */

	return ret;
}


/*
 * brief		read a integer of net side byte order and transfer it to host side byte order.
 *
 * note		BosaZhong@25Apr2013, add, for compatible.
 */
size_t fread_HostIntFromNetByteOrder(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	int tmp = 0;
	int ret = 0;

	USBVM_ASSERT(sizeof(int) == (size * nmemb));

	ret = fread(&tmp, sizeof(int), 1, stream);
	if (ret != 1)
	{
		return -1;
	}
	
	*((int *)ptr) = ntohl(tmp);

	return ret;
}


/*
 * brief		transfer a integer of host side byte order to net side byte order, then write
 *			it to a file.
 *
 * note		BosaZhong@25Apr2013, add, for compatible.
 */
size_t fwrite_HostIntToNetByteOrder(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	int tmp = 0;
	int ret = 0;

	USBVM_ASSERT(sizeof(int) == (size * nmemb));

	tmp = *((int *)ptr);
	tmp = htonl(tmp);
	ret = fwrite(&tmp, sizeof(int), 1, stream);

	return ret;
}


static void sigAlrm(int signo)
{
	/* nothing to do, just wake up fcntl */
	return;
}
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
	if ((node1->fileTimName != node2->fileTimName) 
		 || (node1->fileSize != node2->fileSize)
		 || (node1->fileDuration != node2->fileDuration)
		 || (node1->payloadType != node2->payloadType)
		 || (node1->readFlag != node2->readFlag)
		 || (strcmp(node1->srcNum, node2->srcNum) != 0)
		 || (strcmp(node1->dstNum, node2->dstNum) != 0))
	{
		return -1;
	}
	return 0;
}


/* 
 * fn		static void strToLower( char *pstr )
 * brief	Tranform string to lower case
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
static void strToLower( char *pstr )
{
	char c;
	int i = 0;
	
	while (*(pstr + i) != '\0')
	{
		c = *(pstr + i);
		if (isupper((int)c))
		{		
			*(pstr + i) = tolower((int)c);
		}
		i++;
	}
}


/* 
 * fn		static BOOL checkRecFileName( char *pstr )
 * brief	Check record file name's format
 * details	File name is a unsigned int with hex format
 *
 * param[in]	*pstr  pointer to file name string
 * param[out]	
 *
 * return	accordant return 1 else return 0
 * retval	
 *
 * note		
 */
static BOOL checkRecFileName( char *pstr )
{
	int i;
	int len = strlen(pstr);
	
	if (len == 8)
	{		
		for (i = 0; i < len; i++)
		{
			if (isxdigit(*(pstr+i)) == 0)
			{
				return FALSE;
			}
		}
		
		if (i == len)
		{
			return TRUE;
		}
	}
	
	return FALSE;
}

/* 
 * fn		static int filter( const struct dirent *pdir )
 * brief	Filter the file under the directory that not accord with the specified format 
 * details	
 *
 * param[in]	*pdir  directory under which files are to be filtered
 * param[out]	
 *
 * return	accordant return 1 else return 0
 * retval	
 *
 * note		
 */
static int filter( const struct dirent *pdir )
{
	
	if (!strcmp(pdir->d_name, "."))
	{
		return 0;
	}
	else if (!strcmp(pdir->d_name, ".."))
	{
		return 0;
	}
	else if (checkRecFileName((char *)(pdir->d_name)) == FALSE)
	{
		return 0;
	}

	return 1;
}

/* 
 * fn		static void wavFIFOInit( void )
 * brief	Initialize the wav FIFO buffer
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
static void wavFIFOInit( void )
{

	/* initialize fifo */
	l_fifoBufHead = 0;
	l_fifoBufTail = 0;
	l_fifoBufState = WAV_FIFO_EMPTY;
}

/* 
 * fn		static void reBuildRecIndexList( int endpt )
 * brief	Rebuild record index file when system restart or hotplug happened
 * details	
 *
 * param[in]	endpt  FXS endpoint number
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
static void reBuildRecIndexList( int endpt,FILE* fpIndex)
{
/*20130618_update*/
#define USBVM_FILE_NAME_CMP_LEN				32

	struct dirent **nameList = NULL;
	int i=0, j=0, total=0;
	char recFilePathName[100]={0};
	char fileTimName[200]={0};
	USBVM_REC_FILE_NODE recNode, indexNode;
	FILE *fpRec = NULL;
	BOOL bNoIndexNode = FALSE;
	BOOL brecovFromTaint = FALSE;
	char cmd[100]={0};
	int notReadNumCount=0;		/* to get real notReadNum and position when index is not correct */
	int notReadPos=0;
	int recordNumCount=0;	
	char fileNameCmp[USBVM_FILE_NAME_CMP_LEN]={0};	/* 从16进制文件名转换而来，用于和实际的文件名比较 */
	/* 用于判断是否是重复条目，虽然概率小，但测试时出现过，是错误操作累积的结果*/
	int indexFileTimeNameLast = 0; 

	#ifdef DEBUG_DETAILED
	MARK_PRINT("entering reBuildRecIndexList endpt:%d [%s:%d]\n", endpt,__FUNCTION__,__LINE__);
	#endif
	USBVM_ASSERT(endpt>=0&&endpt<ENDPT_NUM);

	recordNumCount = l_recNumInAll;
	
	/* Check the integrity of each record index node in record index file */
	for (i = 0; i < l_recNumInAll; i++)
	{
		USBVM_ASSERT(fpIndex!=NULL);
		
		if (fpIndex != NULL)
		{
			if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE 
					+(l_recNumInAll - 1 - i) * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
			{
				return ;
			}
			TT_PRINT("calling fread_USBVM_REC_FILE_NODE");
			if (fread_USBVM_REC_FILE_NODE(&indexNode, sizeof(USBVM_REC_FILE_NODE), 1, fpIndex) != 1)
			{
				return ;
			}		
		}
		/* Check whether record index node info is correct */
		if (usbvm_chkSum(&indexNode) == 0)
		{
			if(indexFileTimeNameLast == indexNode.fileTimName)
			{
				/* Delete record index node only,if it is the same as last time */
				if (usbvm_delRecIndexListOnly(endpt, l_recNumInAll - i, fpIndex) < 0)
				{
					return ;
				}
				i--;
				recordNumCount--;
				brecovFromTaint = TRUE;
				continue;
			}
			else
			{
				indexFileTimeNameLast = indexNode.fileTimName;
			}		
			if(indexNode.readFlag == FALSE)
			{
				notReadPos = l_recNumInAll - i;
				notReadNumCount++;
			}
			usbvm_wavPathFormat(indexNode, recFilePathName);
			/* Record file exist */
			if (access(recFilePathName, F_OK) == 0) 
			{
				
				if (access(recFilePathName, R_OK | W_OK) != 0)
				{
					chmod(recFilePathName, 0777);
				}	
			}
			else  /* Record file doesn't exist */
			{

				if (usbvm_delRecIndexListOnly(endpt, l_recNumInAll - i, fpIndex) < 0)
				{
					return;
				}
				i--;
				recordNumCount--;
				brecovFromTaint = TRUE;
			}
		}
		else  /* Record index node info is not correct */
		{
			if (usbvm_delRecIndexListOnly(endpt, l_recNumInAll - i, fpIndex) < 0)
			{
				return;
			}
			i--;
			recordNumCount--;
			brecovFromTaint = TRUE;
		}
	}
		
		
	if (l_recNumInAll == 0)
	{
		bNoIndexNode = TRUE;
	}

	if (brecovFromTaint == TRUE)
	{
		printf("USB Voice MailBox-- Recovered From Tainted Files!\n");
	}

	/* 比较index中的和实际的，如果不同,则重写  */
	/* 此处的recordNumCount，notReadNumCount是一个一个检查的实际可用的文件数量 */
	/*l_recNumInAll、l_recNotReadNum全局变量是保存在index头部的数值，直接读出的，可能不对 */
	/* 满足以下条件之一需要重写:
		1、index头部的总数与实际有效录音文件数量不匹配
		2、index头部的未读数量与实际文件未读数量不匹配
		3、index头部的未读开始的偏移量与index条目中的不匹配
		4、总数为0，但未读数量和偏移量有一个不为0
		5、未读数量为0但偏移量不为0
		6、未读偏移量是0，但未读总数不为0
		7、bNoIndexNode==TRUE，即应该为0，但实际值不一定是0，应当重写一遍*/
	if((l_recNumInAll != recordNumCount)
		||(l_recNotReadNum != notReadNumCount)
		|| (l_recNotReadStartPos != notReadPos)
		||((l_recNumInAll==0)&&((l_recNotReadNum!=0)||(l_recNotReadStartPos)))
		||((l_recNotReadNum==0)&&(l_recNotReadStartPos!=0))
		||((l_recNotReadNum!=0)&&(l_recNotReadStartPos==0))
		||(bNoIndexNode == TRUE))
		{
#ifdef DEBUG_DETAILED
			MARK_PRINT("rewrite index! endpt[%d],l_recNumInAll[%d:%d],l_recNotReadNum[%d:%d],l_recNotReadStartPos[%d:%d]\n",
				endpt,
				l_recNumInAll,
				recordNumCount,
				l_recNotReadNum,
				notReadNumCount,
				l_recNotReadStartPos,
				notReadPos);
#endif
			/* 先将这两个值改为实际值  */
			l_recNumInAll = recordNumCount;
			l_recNotReadNum = notReadNumCount;

			if(l_recNumInAll==0)
			{
				l_recNotReadNum=0;
				l_recNotReadStartPos=0;
			}
			else if (l_recNotReadNum == 0)
			{
				l_recNotReadStartPos = 0;
			}
			else
			{
				int k=0;
				for (k = 0; k < l_recNumInAll; k++)
				{
					if (fpIndex != NULL)
					{
						if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE 
								+  k * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
						{
							return ;
						}
/*20130618_update*/
						if (fread_USBVM_REC_FILE_NODE(&indexNode, sizeof(USBVM_REC_FILE_NODE), 1, fpIndex) != 1)
						{
							return ;
						}		
					}
					/* 不用再check，因index的条目已经恢复了一遍  */
					if(indexNode.readFlag==FALSE)
					{
						l_recNotReadStartPos = k+1;
						break;
					}
				}

				if (k == l_recNumInAll)
				{
					l_recNotReadStartPos = l_recNotReadNum = 0;
				}
				else
				{
					l_recNotReadNum = l_recNumInAll - l_recNotReadStartPos + 1;
				}	
			}
		
			/* rewrite indexfile  */
			if (fseek(fpIndex, 0, SEEK_SET) == -1)
			{
				MARK_PRINT("Failed to seek index file \n");
				return ;
			}
			/* Write record number inall */
			if (fwrite_HostIntToNetByteOrder(&l_recNumInAll, sizeof(int), 1, fpIndex) != 1)
			{
				MARK_PRINT("Failed to write index file \n");
				return ;
			}
			/* Write the start position of the not read node */
			if (fwrite_HostIntToNetByteOrder(&l_recNotReadStartPos, sizeof(int), 1, fpIndex) != 1)
			{
				MARK_PRINT("Failed to write index file \n");
				return ;
			}
			/* Write the number of the not read nodes */
			if (fwrite_HostIntToNetByteOrder(&l_recNotReadNum, sizeof(int), 1, fpIndex) != 1)
			{
				MARK_PRINT("Failed to write index file \n");
				return ;
			}						
		}

	#ifdef DEBUG_DETAILED
	MARK_PRINT("exit %s, endpt:%d, l_recNumInAll:%d, l_recNotReadNum:%d, l_recNotReadStartPos:%d\n", 
		__FUNCTION__, endpt, l_recNumInAll, l_recNotReadNum, l_recNotReadStartPos);
	#endif
/*Edited by huanglei, assert index header info. 2012/01/13*/

	USBVM_ASSERT(l_recNumInAll>=0);
	USBVM_ASSERT(l_recNotReadNum>=0);
	USBVM_ASSERT(l_recNotReadStartPos>=0);
	if(l_recNumInAll==0)
	{
		USBVM_ASSERT(l_recNotReadNum==0&&l_recNotReadStartPos==0);
	}
	else if(l_recNotReadNum>0)
	{
		USBVM_ASSERT(l_recNotReadStartPos>0);
		USBVM_ASSERT(l_recNumInAll>=l_recNotReadNum);
		USBVM_ASSERT(l_recNumInAll>=l_recNotReadStartPos);
		USBVM_ASSERT(l_recNumInAll+1 >= l_recNotReadStartPos+l_recNotReadNum);
		/* 未读偏移量和未读数量相加，大于总数量的话，是一个bug，会播放不存在的文件 */
	}
	else
	{
		USBVM_ASSERT(l_recNotReadStartPos==0);
	}

/*end huanglei*/
	sync();
	return ;
/*20130618_update*/
#undef USBVM_FILE_NAME_CMP_LEN
}


/**************************************************************************************************/
/*                                           PUBLIC_FUNCTIONS                                     */
/**************************************************************************************************/

/*
 *	fn			int getFileSize(const char * filePathName)
 *
 *	brief		get file size
 *
 *	param[in]	filePathName		--	file path name
 *
 *	return		SINT32
 */
int getFileSize(const char * filePathName)
{
	struct stat statBuf;

	if (stat(filePathName, &statBuf) == -1)
	{
		MARK_PRINT("USB Voice MailBox--Get stat on %s Error\n", filePathName); 
		return -1;
	}

	if (S_ISDIR(statBuf.st_mode))
	{
		return 0;
	}

	if (S_ISREG(statBuf.st_mode))
	{
		return statBuf.st_size;
	}

	return 0;
}

/* 
 * fn		void usbvm_escapeCharHandle( char *pstr )
 * brief	Handle escape character if come with space or single quote
 * details	when call comand "rm file",we should add escape character '\'
 *			to file before space or single quote 
 *
 * param[in]	*pstr  pointer to string
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
void usbvm_escapeCharHandle( char *pstr )
{
	char *pstrTmp = pstr;
	char strTmp[100];
	
	while(*pstrTmp != '\0')
	{
		if ((*pstrTmp == ' ') || (*pstrTmp == '\''))
		{
			strcpy(strTmp, pstrTmp);
			*pstrTmp = '\\';
			pstrTmp++;
			strcpy(pstrTmp, strTmp);
		}
		pstrTmp++;
	}
}

/* 
 * fn		FILE *usbvm_getRecIndexFileFp( int endpt )
 * brief	Get record index file pointer according to endpt
 * details	
 *
 * param[in]	endpt  FXS endpoint number
 * param[out]	
 *
 * return	record index file pointer
 * retval	
 *
 * note		
 */
FILE *usbvm_getRecIndexFileFp( int endpt, char* mode)
{
	char recIndexFilePathName[100];
	FILE *fp = NULL;
	
	sprintf(recIndexFilePathName, "%srecordIndex", USB_INDEX_FILE_DIR);

/*20130618_update*/
	TT_PRINT("will open file: %s", recIndexFilePathName);

		if (access(recIndexFilePathName, R_OK | W_OK) != 0)
		{
			chmod(recIndexFilePathName, 0777);
		}		

		fp = fopen(recIndexFilePathName, mode);

		if (fp == NULL)
		{
			MARK_PRINT("Failed to open an exist file %s for writing\n", recIndexFilePathName);
			return fp;
		}
		else
		{
			return fp;
		}
	
}

/* 
 * fn		int usbvm_lockHandle(int fd,int cmd,int type, off_t offset,int whence, off_t len)
 * brief	call fcntl() with args
 * details	
 *
 * param[in]	int fd: file descriptor;
 *				int cmd: F_SETLK, F_SETLKW, F_GETLK, etc.
 *				int type:  F_RDLCK, F_WRLCK, F_UNLCK, etc.
 *				off_t offset: the start position of the block to be locked.refer to whence.
 *				int whence: SEEK_SET, SEEK_END etc.
 *				off_t len: the lenth of block to be locked.
 * param[out]	
 *
 * return	int
 * retval	failed; -1; same as fcntl();
 *
 * note		
 */
int usbvm_lockHandle(int fd,int cmd,int type, off_t offset,int whence, off_t len)
{	
	struct flock lock;	
	lock.l_type = type;	
	lock.l_len = len;
	lock.l_start = offset;	
	lock.l_whence = whence;
	return (fcntl(fd, cmd, &lock));
}

/* 
 * fn		void usbvm_setRecCurNode( int endpt, int recCurNode )
 * brief	Set record current node position
 * details	
 *
 * param[in]	endpt  endpoint number
 * param[in]	recCurNode  record current node position
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
void usbvm_setRecCurNode( int endpt, int recCurNode )
{
	if ((endpt >= 0) && (endpt < ENDPT_NUM))
	{
		l_recCurNode[endpt] = recCurNode;
	}
}

/* 
 * fn		int usbvm_getRecCurNode( int endpt )
 * brief	Get record current node position
 * details	
 *
 * param[in]	endpt  endpoint number
 * param[out]	
 *
 * return	current node position
 * retval	
 *
 * note		
 */
int usbvm_getRecCurNode( int endpt )
{
	if ((endpt >= 0) && (endpt < ENDPT_NUM))
	{
	
		return l_recCurNode[endpt];
	}
	else
	{
		return 0;
	}
}

/* 
 * fn		void usbvm_setRecNumInAll( int endpt, int recNumInAll )
 * brief	Set record list number in all
 * details	
 *
 * param[in]	endpt  endpoint number
 * param[in]	recNumInAll  record list number
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
void usbvm_setRecNumInAll( int endpt, int recNumInAll )
{
	if ((endpt >= 0) && (endpt < ENDPT_NUM))
	{
		l_recNumInAll = recNumInAll;
	}
}

/* 
 * fn		int usbvm_getRecNumInAll( int endpt )
 * brief	Get record number in all
 * details	
 *
 * param[in]	endpt  endpoint number
 * param[out]	
 *
 * return	record list number
 * retval	
 *
 * note		edited by huanglei ,2011/09/08
*/

int usbvm_getRecNumInAll( int endpt,FILE* fpIndex )
{
	int recNumInAll=0;
	int recNotReadNum=0;
	int recNotReadStartPos=0;
	if ((endpt >= 0) && (endpt < ENDPT_NUM))
	{
		usbvm_getStaticVarsFromIndexFile(endpt, &recNumInAll,
									&recNotReadNum, &recNotReadStartPos, fpIndex);
		l_recNumInAll = recNumInAll;
		#ifdef DEBUG_DETAILED
		MARK_PRINT("\nHL_DBG: NumInAll= %d, in %s: %d\n",\
			l_recNumInAll,__FUNCTION__,__LINE__);
		#endif
		return l_recNumInAll;
	} 

	else
	{
		return 0;
	}
}


/* 
 * fn		void usbvm_getStaticsFromIndexFile( int endpt , int *recInAll, int *notReadInAll, int *notReadStart ) 
 * brief	Get record statics from index file for other process
 * details	
 *
 * param[in]	endpt  endpoint number
 * param[out]	recInAll  record number inall
 * param[out]	notReadInAll  not read record number inall
 * param[out]	notReadStart  not read start position
 *
 * return	record list number
 * retval	
 *
 * note		
 */
int usbvm_getStaticsFromIndexFile( int endpt , int *pRecInAll, int *pNotReadInAll, int *pNotReadStart )
{

	FILE* fpIndex = NULL;
	int ret = -1;

#ifdef DEBUG_DETAILED
	MARK_PRINT("HL_DBG: Entering %s: %d\n", __FUNCTION__,__LINE__);
#endif

	if(usbvm_checkUSBDevExist() != TRUE)
	{
		return ret;
	}


	ret = usbvm_openIndexFpAndLock(endpt, &fpIndex, "rb", RD_LOCK_HEADER);
	if(ret!=-1)
	{
		if(fpIndex != NULL)
		{
			ret = usbvm_getStaticVarsFromIndexFile(endpt, pRecInAll, pNotReadInAll, pNotReadStart, fpIndex);
			ret = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, RD_LOCK_HEADER);
			if(ret == -1)
			{
				MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			}
		}
		else
		{
			MARK_PRINT("HL_DBG:fpIndex==NULL or lock index failed, [%s:%d]\n",__FUNCTION__,__LINE__);
			ret = -1;
		}
	}
	else
	{
		MARK_PRINT("HL_DBG: Lock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
	}

	return ret;
}
/* 
 * fn		void usbvm_getStaticsFromIndexFile( int endpt , int *recInAll, int *notReadInAll, int *notReadStart ) 
 * brief	Get record statics from index file for other process
 * details	
 *
 * param[in]	endpt  endpoint number
 * param[out]	recInAll  record number inall
 * param[out]	notReadInAll  not read record number inall
 * param[out]	notReadStart  not read start position
 *
 * return	record list number
 * retval	
 *
 * note		
 */
int usbvm_getStaticVarsFromIndexFile( int endpt , int *pRecInAll, int *pNotReadInAll, int *pNotReadStart,FILE* fpIndex)
{
	USBVM_ASSERT(endpt>=0 && endpt<ENDPT_NUM);
	if ((endpt >= 0) && (endpt < ENDPT_NUM))
{
	if (fpIndex != NULL)
	{
		if (fseek(fpIndex, 0, SEEK_SET) == -1)
		{						
			return -1;
		}
/*20130618_update*/
		/* Read record number inall */
		if (fread_HostIntFromNetByteOrder(pRecInAll, sizeof(int), 1, fpIndex) != 1)
		{
			return -1;
		}
		/* Read the start position of the not read node */
		if (fread_HostIntFromNetByteOrder(pNotReadStart, sizeof(int), 1, fpIndex) != 1)
		{
			return -1;
		
		}
		/* Read the number of the not read nodes */
		if (fread_HostIntFromNetByteOrder(pNotReadInAll, sizeof(int), 1, fpIndex) != 1)
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}
	}
	/*Edited by huanglei, assert index header info. 2012/01/13*/

	USBVM_ASSERT(*pRecInAll>=0);
	USBVM_ASSERT(*pNotReadInAll>=0);
	USBVM_ASSERT(*pNotReadStart>=0);
	if(*pRecInAll==0)
	{
		USBVM_ASSERT((*pNotReadInAll==0)&&(*pNotReadStart==0));
	}
	else if(*pNotReadInAll>0)
	{
		USBVM_ASSERT(*pRecInAll>=*pNotReadInAll);
		USBVM_ASSERT(*pRecInAll>=*pNotReadStart);
		USBVM_ASSERT(*pNotReadStart>0);
	}
	else
	{
		USBVM_ASSERT(*pNotReadStart==0);
	}

	/* Update the global vars after everytime read */
	l_recNumInAll = *pRecInAll;
	l_recNotReadStartPos = *pNotReadStart;
	l_recNotReadNum = *pNotReadInAll;
/*20130618_update*/
	TT_PRINT("recInAll(%d) NotReadStart(%d) NotReadInAll(%d)",
		*pRecInAll, *pNotReadStart, *pNotReadInAll);
		
	return 0;
}

/* 
 * fn		void usbvm_setRecNotReadStartPos( int endpt, int recNotReadStartPos )
 * brief	Set record start position that has not been read
 * details	
 *
 * param[in]	endpt  endpoint number
 * param[in]	recNotReadStartPos  record not read start position	
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
void usbvm_setRecNotReadStartPos( int endpt, int recNotReadStartPos )
{
	if ((endpt >= 0) && (endpt < ENDPT_NUM))
	{
		l_recNotReadStartPos = recNotReadStartPos;
	}
}

/* 
 * fn		int usbvm_getRecNotReadStartPos( int endpt )
 * brief	Get record start position that has not been read
 * details	
 *
 * param[in]	endpt  endpoint number
 * param[out]	
 *
 * return	 record not read start position
 * retval	
 *
 * note		
 */
int usbvm_getRecNotReadStartPos( int endpt,FILE* fpIndex )
{
	int recNumInAll=0;
	int recNotReadNum=0;
	int recNotReadStartPos=0;
	if ((endpt >= 0) && (endpt < ENDPT_NUM))
	{
		usbvm_getStaticVarsFromIndexFile(endpt, &recNumInAll, &recNotReadNum, &recNotReadStartPos,fpIndex);
		l_recNotReadStartPos = recNotReadStartPos;
		#ifdef DEBUG_DETAILED
		MARK_PRINT("\nHL_DBG: l_recNotReadStartPos= %d,recNotReadNum=%d in %s: %d\n",\
			l_recNotReadStartPos,recNotReadNum,__FUNCTION__,__LINE__);
		#endif
		return l_recNotReadStartPos;
	} 
	else
	{
		return 0;
	}
}

/* 
 * fn		void usbvm_setRecNotReadNum( int endpt, int recNotReadNum )
 * brief	Set record number that haven't been read
 * details	
 *
 * param[in]	endpt  endpoint number
 * param[in]	recNotReadNum  record not read number		
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
void usbvm_setRecNotReadNum( int endpt, int recNotReadNum )
{
	if ((endpt >= 0) && (endpt < ENDPT_NUM))
	{
		l_recNotReadNum = recNotReadNum;
	}
}

/* 
 * fn		int usbvm_getRecNotReadNum( int endpt )
 * brief	Get record not read number
 * details	
 *
 * param[in]	endpt  endpoint number
 * param[out]	
 *
 * return	record not read number
 * retval	
 *
 * note		
 */
int usbvm_getRecNotReadNum( int endpt, FILE* fpIndex )
{
	int recNumInAll=0;
	int recNotReadNum=0;
	int recNotReadStartPos=0;
	if ((endpt >= 0) && (endpt < ENDPT_NUM))
	{
		usbvm_getStaticVarsFromIndexFile(endpt, &recNumInAll, &recNotReadNum, &recNotReadStartPos, fpIndex);
		l_recNotReadNum = recNotReadNum;
		#ifdef DEBUG_DETAILED
		MARK_PRINT("\nHL_DBG: recNotReadNum= %d, recNotReadStartPos=%d,in %s: %d\n",\
			l_recNotReadNum,recNotReadStartPos,__FUNCTION__,__LINE__);
		#endif
		return l_recNotReadNum;
	} 
	else
	{
		return 0;
	}
}


/* 
 * fn		void usbvm_calChkSum( USBVM_REC_FILE_LIST precNode )
 * brief	Calculate the chkSum of record index node
 * details	
 *
 * param[in]	precNode  pointer of record index node
 * param[out]	precNode->checkSum
 *
 * return	
 * retval	
 *
 * note		
 */
void usbvm_calChkSum( USBVM_REC_FILE_LIST precNode )
{
/*20130618_update*/
	unsigned int uTemp = 0;
	unsigned int chkSumTemp = 0;
	int i;
	unsigned int *pdata = (unsigned int *)(precNode);
	
	for (i = 0; i < (sizeof(USBVM_REC_FILE_NODE)/4 - 1); i++)
	{
/*20130618_update*/
		if ((i >= USBVM_REC_NODE_NUMBER_START) && (i <= USBVM_REC_NODE_NUMBER_END))
		{
			uTemp = (unsigned int)ntohl(*(pdata + i));
			chkSumTemp += ~uTemp;
		}
		else
		{
		chkSumTemp += ~(*(pdata + i));
	}	
	}	

	precNode->checkSum = chkSumTemp;
	
}

/* 
 * fn		int usbvm_chkSum( USBVM_REC_FILE_LIST precNode )
 * brief	Check whether the chkSum of the record index node is correct
 * details	
 *
 * param[in]	precNode  pointer of record index node
 * param[out]	
 *
 * return	correct return 0 else return -1
 * retval	
 *
 * note		
 */
int usbvm_chkSum( USBVM_REC_FILE_LIST precNode )
{
	unsigned int checkSumTemp = 0;
	int i;
	unsigned int *pdata = (unsigned int *)(precNode);
/*20130618_update*/
	unsigned int uTemp = 0;
	int ret = 0;

	/* Calculate the chkSum of the record index node including chkSum segment */
	for (i = 0; i < (sizeof(USBVM_REC_FILE_NODE)/4); i++)
	{
/*20130618_update*/
		if ((i >= USBVM_REC_NODE_NUMBER_START) && (i <= USBVM_REC_NODE_NUMBER_END))
		{
			uTemp = (unsigned int)ntohl(*(pdata + i));
			checkSumTemp += ~uTemp;
		}
		else
		{
		checkSumTemp += ~(*(pdata + i));
	}	
	}	

	if (checkSumTemp == (unsigned int)0xFFFFFFFF)
	{
		ret = 0;
	}
	else
	{
		ret = -1;
	}
/*20130618_update*/
	TT_PRINT("usbvm_chkSum return %d (0 -- OK, -1 -- failed)", ret);

	return ret;
}


/* 
 * fn		void usbvm_createRecIndexList( void )
 * brief	Create record index list
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
void usbvm_createRecIndexList( void )
{
	int i;
	char recIndexFilePathName[100];
	char cmd[100];
	int status;
	int lockRet = -1;
	int indexSize = 0;
	BOOL rebuildNewIndex;
	struct stat st;
	char filepathname[200] = {0};

	/* USB device does not exist */
	if (access(USBDISK_DIR, F_OK) != 0)
	{
		return;
	}

	/* Create usbmail dirs */
	snprintf(filepathname, sizeof(filepathname), "%s", USB_INDEX_FILE_DIR);
	if (access(filepathname, F_OK) != 0)
	{
		sprintf(cmd,"mkdir -p %s", filepathname);
		system(cmd);
		if (access(filepathname, F_OK) !=0)
		{
			MARK_PRINT("failed to create %s\n", filepathname);
			return;
		}
		chmod(filepathname, 0777);
	}	

	snprintf(filepathname, sizeof(filepathname), "%s", USB_REC_FILE_DIR);
	if (access(filepathname, F_OK) != 0)
	{
		sprintf(cmd,"mkdir -p %s", filepathname);
		system(cmd);
		if (access(filepathname, F_OK) !=0)
		{
			MARK_PRINT("failed to create %s\n", filepathname);
			return;
		}
		chmod(filepathname, 0777);
	}	

	snprintf(filepathname, sizeof(filepathname), "%s", USB_NOT_FILE_CUSTOM_DIR);
	if (access(filepathname, F_OK) != 0)
	{
		sprintf(cmd,"mkdir -p %s", filepathname);
		system(cmd);
		if (access(filepathname, F_OK) !=0)
		{
			MARK_PRINT("failed to create %s\n", filepathname);
			return;
		}
		chmod(filepathname, 0777);
	}		

	for (i = 0; i < ENDPT_NUM; i++)
	{
		l_recCurNode[i] = 0;
	}
	
	MARK_PRINT("create index...\n");
	
	rebuildNewIndex = FALSE;

	/* Create mutex for index list operation */
	status = pthread_mutex_init((pthread_mutex_t *)&l_mutexIndexFileOperation, NULL );
	USBVM_ASSERT( status == 0 );
	
	l_recNumInAll = 0;
	l_recNotReadStartPos = 0;
	l_recNotReadNum = 0;
	
	sprintf(recIndexFilePathName, "%s", USB_INDEX_FILE_DIR);
	/* check if the dir is not link */
	if(lstat(USBDISK_DIR,&st)==0)
	{
		if(!S_ISLNK(st.st_mode)&&S_ISDIR(st.st_mode))
		{
			snprintf(cmd,100,"rm -rf %s",USBDISK_DIR);
			system(cmd);
			MARK_PRINT("Unexpected file type of %s, ought to be a symbol link",
				USBDISK_DIR);
			return;
		}
	}
	else
	{
		return;
	}
			
	strcat(recIndexFilePathName, "recordIndex");
	MARK_PRINT("name '%s'\n", recIndexFilePathName);
	/* Record index already exist */
	if (access(recIndexFilePathName, F_OK) == 0) 
	{
		/* If index file exist, read global info we need */
		if (access(recIndexFilePathName, R_OK | W_OK) != 0)
		{
			chmod(recIndexFilePathName, 0777);
		}		
		indexSize = usbvm_getFileSize(recIndexFilePathName);
/*20130618_update*/
		TT_PRINT("usbvm_getFileSize(%s) return %d", recIndexFilePathName, indexSize);
		
		lockRet = usbvm_openIndexFpAndLock(0, &l_fpRecIndex, "rb+", WR_LOCK_WHOLE);
		if(lockRet == -1)
		{
			MARK_PRINT("HL_DBG: usbvm_openIndexFpAndLock failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			return;
		}

		if (fseek(l_fpRecIndex, 0, SEEK_SET) == -1)
		{
			MARK_PRINT("Failed to seek an exist file %s\n", recIndexFilePathName);						
			rebuildNewIndex = TRUE;
		}
/*20130618_update*/
		/* Read record number inall */
		if (fread_HostIntFromNetByteOrder(&l_recNumInAll, sizeof(int), 1, l_fpRecIndex) != 1)
		{
			MARK_PRINT("Failed to read an exist file %s\n", recIndexFilePathName);
			rebuildNewIndex = TRUE;
		}
/*20130618_update*/
		TT_PRINT("get record number inall %d", l_recNumInAll);
/*20130618_update*/			
		/* Read the start position of the not read node */
		if (fread_HostIntFromNetByteOrder(&l_recNotReadStartPos, sizeof(int), 1, l_fpRecIndex) != 1)
		{
			MARK_PRINT("Failed to read an exist file %s\n", recIndexFilePathName);
			rebuildNewIndex = TRUE;
		}
/*20130618_update*/
		TT_PRINT("get start position of not read %d", l_recNotReadStartPos);
		
		/* Read the number of the not read nodes */
		if (fread_HostIntFromNetByteOrder(&l_recNotReadNum, sizeof(int), 1, l_fpRecIndex) != 1)
		{
			MARK_PRINT("Failed to read an exist file %s\n", recIndexFilePathName);
			rebuildNewIndex = TRUE;
		}
/*20130618_update*/
		TT_PRINT("get number of not read %d", l_recNotReadNum);

		/* Rebuild record index file */
		/* Validity check of global information in record index file */
		if ((l_recNotReadStartPos > l_recNumInAll)
			|| (l_recNotReadNum > l_recNumInAll)
			|| ((l_recNotReadNum + l_recNotReadStartPos) > (l_recNumInAll + 1))
			|| (l_recNumInAll < 0)
			|| (l_recNotReadNum < 0)
			|| (l_recNotReadStartPos < 0))
		{
			l_recNumInAll = 0;
			l_recNotReadNum = 0;
			l_recNotReadStartPos = 0;
			rebuildNewIndex = TRUE;
		}
		if(indexSize < (RECLIST_INDEX_AREA_SIZE + l_recNumInAll * sizeof(USBVM_REC_FILE_NODE)))
		{
			l_recNumInAll = 0;
			l_recNotReadNum = 0;
			l_recNotReadStartPos = 0;
			rebuildNewIndex = TRUE;
		}

		if(rebuildNewIndex == FALSE)
		{	/* 如果没有问题，则直接reBuildRecIndexList */
/*20130618_update*/
				TT_PRINT("calling reBuildRecIndexList(%d, %d)", 0, fileno(l_fpRecIndex));
				reBuildRecIndexList(0,l_fpRecIndex);
			errno = 0;
/*20130618_update*/
			TT_PRINT("calling truncate(%s, %d)", recIndexFilePathName, 
				RECLIST_INDEX_AREA_SIZE + l_recNumInAll * sizeof(USBVM_REC_FILE_NODE));	
			
			int ret=truncate(recIndexFilePathName,
				RECLIST_INDEX_AREA_SIZE + l_recNumInAll * sizeof(USBVM_REC_FILE_NODE));
			if(ret!=0)
			{
				MARK_PRINT("HL_DBG: truncate: ret=%d, %s\n",ret, strerror(errno));
			}
/*20130618_update*/
			TT_PRINT("calling usbvm_closeIndexFpAndUnlock");
			
			lockRet = usbvm_closeIndexFpAndUnlock(0, l_fpRecIndex, WR_LOCK_WHOLE);
			if(lockRet == -1)
			{
				MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			}
		}
		else
		{
			lockRet = usbvm_closeIndexFpAndUnlock(0, l_fpRecIndex, WR_LOCK_WHOLE);
			if(lockRet == -1)
			{
				MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			}
		}
		l_fpRecIndex = NULL;
	}
	else
	{
		/* 若不存在则直接重写 */
		rebuildNewIndex = TRUE;
	}
	/* 将index清空，并调用reBuildRecIndexList重写 */
	if(rebuildNewIndex == TRUE)
	{
		char zeroBytes[88];
		memset(zeroBytes,0,88);
		MARK_PRINT("rebuilding for endpt %d ...\n", i);
		/* If index file doesn't exit, create and initialize it */
			lockRet = usbvm_openIndexFpAndLock(0, &l_fpRecIndex, "wb+", WR_LOCK_WHOLE);
		if(lockRet == -1)
		{
			MARK_PRINT("HL_DBG: Lock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			return;
		}
		if (fseek(l_fpRecIndex, 0, SEEK_SET) == -1)
		{
			MARK_PRINT("Failed to seek an exist file %s\n", recIndexFilePathName);
				lockRet = usbvm_closeIndexFpAndUnlock(0, l_fpRecIndex, WR_LOCK_WHOLE);
			if(lockRet == -1)
			{
				MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			}
			l_fpRecIndex = NULL;
			return ;
		}
		l_recNumInAll = 0;
		l_recNotReadStartPos = 0;
		l_recNotReadNum = 0;
		/* Write record number inall */
		if (fwrite_HostIntToNetByteOrder(&l_recNumInAll, sizeof(int), 1, l_fpRecIndex) != 1)
		{
			MARK_PRINT("Failed to write an exist file %s\n", recIndexFilePathName);
				lockRet = usbvm_closeIndexFpAndUnlock(0, l_fpRecIndex, WR_LOCK_WHOLE);
			if(lockRet == -1)
			{
				MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			}
			l_fpRecIndex = NULL;
			return ;
		}
		/* Write the start position of the not read node */
		if (fwrite_HostIntToNetByteOrder(&l_recNotReadStartPos, sizeof(int), 1, l_fpRecIndex) != 1)
		{
			MARK_PRINT("Failed to write an exist file %s\n", recIndexFilePathName);
				lockRet = usbvm_closeIndexFpAndUnlock(0, l_fpRecIndex, WR_LOCK_WHOLE);
			if(lockRet == -1)
			{
				MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			}
			l_fpRecIndex=NULL;
			return ;
		}
		/* Write the number of the not read nodes */
		if (fwrite_HostIntToNetByteOrder(&l_recNotReadNum, sizeof(int), 1, l_fpRecIndex) != 1)
		{
			MARK_PRINT("Failed to write an exist file %s\n", recIndexFilePathName);
				lockRet = usbvm_closeIndexFpAndUnlock(0, l_fpRecIndex, WR_LOCK_WHOLE);
			if(lockRet == -1)
			{
				MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			}
			l_fpRecIndex=NULL;
			return ;
		}		
		/* Write zero up to length 100, then after 100,the listnode can be added.*/
		if (fwrite(&zeroBytes, 88, 1, l_fpRecIndex) != 1)
		{
			MARK_PRINT("Failed to write an exist file %s\n", recIndexFilePathName);
				lockRet = usbvm_closeIndexFpAndUnlock(0, l_fpRecIndex, WR_LOCK_WHOLE);
			if(lockRet == -1)
			{
				MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			}
			l_fpRecIndex=NULL;
			return ;
		}		
			lockRet = usbvm_closeIndexFpAndUnlock(0, l_fpRecIndex, WR_LOCK_WHOLE);
		if(lockRet == -1)
		{
			MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
		}
		/* 以rb+方式打开重新写，因w+方式如果不小心覆盖了，会造成严重问题 */
			lockRet = usbvm_openIndexFpAndLock(0, &l_fpRecIndex, "rb+", WR_LOCK_WHOLE);
		if(lockRet == -1)
		{
			MARK_PRINT("HL_DBG: usbvm_openIndexFpAndLock failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			return;
		}
		/* Rebuild record index file */
			reBuildRecIndexList(0,l_fpRecIndex);
			lockRet = usbvm_closeIndexFpAndUnlock(0, l_fpRecIndex, WR_LOCK_WHOLE);
		if(lockRet == -1)
		{
			MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
		}
		errno = 0;
		int ret=truncate(recIndexFilePathName,
			RECLIST_INDEX_AREA_SIZE + l_recNumInAll * sizeof(USBVM_REC_FILE_NODE));
		if(ret!=0)
		{
			MARK_PRINT("HL_DBG: truncate: ret=%d,errno=%d,[%s:%d]\n",
				ret,errno,__FUNCTION__,__LINE__);
		}
		l_fpRecIndex = NULL;
	}
}


/* 
 * fn		void usbvm_destoryRecIndexList( void )
 * brief	Destory record index list
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
void usbvm_destoryRecIndexList( void )
{
	#ifdef DEBUG_DETAILED
	MARK_PRINT("entering usbvm_destoryRecIndexList\n");
	#endif

	/* l_fpRecIndex should has been closed and reset to NULL before here,if not, 
		check whether the file has not been closed or the var has not been reset. */
	USBVM_ASSERT(l_fpRecIndex==NULL);
	l_recNumInAll = 0;
	l_recNotReadStartPos = 0;
	l_recNotReadNum = 0;
	
	pthread_mutex_destroy((pthread_mutex_t *)&(l_mutexIndexFileOperation));	
}

/* 
 * fn		void usbvm_recFileMangInit( void )
 * brief	File management initialization including record index file and FIFO buffer 
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
void usbvm_recFileMangInit( void )
{
	int status;
	status = pthread_mutex_init((pthread_mutex_t *)&l_mutexIndexFileOperation, NULL );
	USBVM_ASSERT( status == 0 );
	wavFIFOInit();
			
}

/* 
 * fn		void usbvm_recFileMagDeInit( void )
 * brief	File management deinitialization
 * details	not implemented
 *
 * param[in]	
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
void usbvm_recFileMangDeInit( void )
{
	pthread_mutex_destroy((pthread_mutex_t *)&(l_mutexIndexFileOperation));
}


/* 
 * fn		int usbvm_insertRecIndexList( int endpt, USBVM_REC_FILE_LIST precNode )
 * brief	Insert record index node to record index file
 * details	
 *
 * param[in]	endpt  FXS endpoint number
 * param[in]	precNode  pointer of record index node
 * param[out]
 *
 * return	
 * retval	
 *
 * note		2011/09/09: huanglei, debug. Solved a problem in moving nodes and rewrite.
 */
int usbvm_insertRecIndexList( int endpt, USBVM_REC_FILE_LIST precNode, FILE* fpIndex )
{
	int i;
	USBVM_REC_FILE_NODE node;
	int nodePosInsert = 0;
	int rc = 0;
		
		if (fpIndex != NULL)
		{
			
			MARK_PRINT("entering usbvm_insertRecIndexList filename:%08x\n", precNode->fileTimName);
			
			/* Find node position to insert */
			for (i = 1; i <= l_recNumInAll; i++)
			{
				if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE 
						+ (i - 1) * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
				{
					return -1;
				}
/*20130618_update*/
				if (fread_USBVM_REC_FILE_NODE(&node, sizeof(USBVM_REC_FILE_NODE), 1, fpIndex) != 1)
				{
					return -1;
				}
				/* Find the insert position according to the record time of the record file */
				if (precNode->fileTimName < node.fileTimName)
				{
					nodePosInsert = i;
					break;
				}

			}

/*Edited by huanglei, assert index header info. 2012/01/13*/

	USBVM_ASSERT(l_recNumInAll>=0);
	USBVM_ASSERT(l_recNotReadNum>=0);
	USBVM_ASSERT(l_recNotReadStartPos>=0);
	if(l_recNumInAll==0)
	{
		USBVM_ASSERT(l_recNotReadNum==0&&l_recNotReadStartPos==0);
	}
	else if(l_recNotReadNum>0)
	{
		USBVM_ASSERT(l_recNotReadStartPos>0);
		USBVM_ASSERT(l_recNumInAll>=l_recNotReadNum);
		USBVM_ASSERT(l_recNumInAll>=l_recNotReadStartPos);
		USBVM_ASSERT(l_recNotReadStartPos + l_recNotReadNum<=l_recNumInAll+1);
	}
	else
	{
		USBVM_ASSERT(l_recNotReadStartPos==0);
	}

/*end huanglei*/
			if ( i > l_recNumInAll)
			{	/*Modified by huanglei,if inserting node is the biggest,
				it should be inserted at the end of the list */
				nodePosInsert = i;
			}
			
			/* Move subsequent nodes backward one by one before insert it */
			/*insertpos n, the n-th record will be rewrite. move backward from n to max by one, 
			max-n+1 times in all. For example ,there are 8 recs,now 4th rec is going to be insert ,
			move 4-8 backward, it will take 5 times*/
			for (i = 0; i < l_recNumInAll - nodePosInsert + 1; i++)
			{
				/* Read the node from last */
				if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE 
					+ (l_recNumInAll - i - 1) * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
				{
					return -1;
				}
/*20130618_update*/
				if (fread_USBVM_REC_FILE_NODE(&node, sizeof(USBVM_REC_FILE_NODE), 1, fpIndex) != 1)
				{
					return -1;
				}
				
				/* Write the node into the next postion */	
				if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE 
						+ (l_recNumInAll - i) * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
				{
					return -1;
				}
					
/*20130618_update*/					
				if (fwrite_USBVM_REC_FILE_NODE(&node, sizeof(USBVM_REC_FILE_NODE), 1, fpIndex) != 1)
				{
					return -1;
				}
					
			}
			 
			/* Insert the node */
			if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE 
					+  (nodePosInsert - 1) * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
			{
				return -1;
			}
/*20130618_update*/				
			if (fwrite_USBVM_REC_FILE_NODE(precNode, sizeof(USBVM_REC_FILE_NODE), 1, fpIndex) != 1)
			{
				return -1;
			}

			/* Recalculate the global record number inall */
			l_recNumInAll++;
			if (fseek(fpIndex, 0, SEEK_SET) == -1)
			{
				return -1;
			}
/*20130618_update*/
			if (fwrite_HostIntToNetByteOrder(&l_recNumInAll, sizeof(int), 1, fpIndex) != 1)
			{
				return -1;
			}
				
			
			/* Judge the node insert has been read or not */
			if (precNode->readFlag == TRUE)
			{
				/* Insert position is before the start position of not read node */
				if (nodePosInsert <= l_recNotReadStartPos)
				{
					/* Recalculate the global start position of not read node */
					l_recNotReadStartPos++;
					if (fseek(fpIndex, 4, SEEK_SET) == -1)
					{
						return -1;
					}
/*20130618_update*/
					if (fwrite_HostIntToNetByteOrder(&l_recNotReadStartPos, sizeof(int), 1, fpIndex) != 1)
					{
						return -1;
					}
						
				}

			}
			else  /* Insert position is after the start position of not read node */
			{
				if ((nodePosInsert <= l_recNotReadStartPos) || (l_recNotReadStartPos == 0))
				{
					/* Recalculate the global start position of not read node */
					l_recNotReadStartPos = nodePosInsert;
					if (fseek(fpIndex, 4, SEEK_SET) == -1)
					{
						return -1;
					}
/*20130618_update*/						
					if (fwrite_HostIntToNetByteOrder(&l_recNotReadStartPos, sizeof(int), 1, fpIndex) != 1)
					{
						return -1;
					}
				}

				/* Recalculate the global number of not read nodes */
				l_recNotReadNum++;
				if (fseek(fpIndex, 8, SEEK_SET) == -1)
				{
					return -1;
				}
/*20130618_update*/
				if (fwrite_HostIntToNetByteOrder(&l_recNotReadNum, sizeof(int), 1, fpIndex) != 1)
				{
					return -1;
				}
			}
			
			rc = nodePosInsert;		
		}
		else
		{
			rc = -1;
		}
	
	/*Edited by huanglei, assert index header info. 2012/01/13*/

	USBVM_ASSERT(l_recNumInAll>=0);
	USBVM_ASSERT(l_recNotReadNum>=0);
	USBVM_ASSERT(l_recNotReadStartPos>=0);
	if(l_recNumInAll==0)
	{
		USBVM_ASSERT(l_recNotReadNum==0&&l_recNotReadStartPos==0);
	}
	else if(l_recNotReadNum>0)
	{
		USBVM_ASSERT(l_recNotReadStartPos>0);
		USBVM_ASSERT(l_recNumInAll>=l_recNotReadNum);
		USBVM_ASSERT(l_recNumInAll>=l_recNotReadStartPos);
		USBVM_ASSERT(l_recNumInAll+1 >= l_recNotReadStartPos+l_recNotReadNum);
	}
	else
	{
		USBVM_ASSERT(l_recNotReadStartPos==0);
	}

/*end huanglei*/		

	sync();
	return rc;
}


/* 
 * fn		int usbvm_addRecIndexList( int endpt, USBVM_REC_FILE_LIST precNode )
 * brief	Add record index node to the end of reord index list
 * details	
 *
 * param[in]	endpt  FXS endpoint number
 * param[in]	precNode  pointer of record index node
 * param[out]
 *
 * return	success return 0, fail return -1
 * retval	
 *
 * note		Edited by huanglei, added reading file. 2011/09/13
 */
int usbvm_addRecIndexList( int endpt, USBVM_REC_FILE_LIST precNode , FILE* fpIndex)
{
	int rc = 0;
	/*Read the local args at first,then edit them. Write them back to file after editing*/
	int recNumInAll = 0;
	int notReadNumInAll = 0;
	int notReadStartPos = 0;
	
	usbvm_getStaticVarsFromIndexFile(endpt, &recNumInAll, &notReadNumInAll, &notReadStartPos, fpIndex);

	if((l_recNumInAll<0)||(l_recNotReadNum<0)||(l_recNotReadStartPos<0))
	{
		/*if recNumInAll==0, it's all right. But less than 0, 
		there must be some unkown mistakes.*/
		MARK_PRINT("HL_DBG:Error read index data,%s:%d\n",__FUNCTION__,__LINE__);
		return -1;
	}

		if (fpIndex != NULL)
		{
			/* Write record index node */
			if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE 
					+ l_recNumInAll * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
			{
				return -1;
			}
/*20130618_update*/
			if (fwrite_USBVM_REC_FILE_NODE(precNode, sizeof(USBVM_REC_FILE_NODE), 1, fpIndex) != 1)
			{
				return -1;
			}

			/* Recalculate the global record number inall */
			l_recNumInAll++;
			if (fseek(fpIndex, 0, SEEK_SET) == -1)
			{
				return -1;
			}
/*20130618_update*/
			if (fwrite_HostIntToNetByteOrder(&l_recNumInAll, sizeof(int), 1, fpIndex) != 1)
			{
				return -1;
			}
			
			if (precNode->readFlag == FALSE)
			{
				/* Recalculate the global start position of not read node */
				if (l_recNotReadStartPos == 0)
				{
					l_recNotReadStartPos = l_recNumInAll;
					if (fseek(fpIndex, 4, SEEK_SET) == -1)
					{
						return -1;
					}
/*20130618_update*/
					if (fwrite_HostIntToNetByteOrder(&l_recNotReadStartPos, sizeof(int), 1, fpIndex) != 1)
					{
						return -1;
					}
				}
				/* Recalculate the global number of not read nodes */
				l_recNotReadNum++;
				if (fseek(fpIndex, 8, SEEK_SET) == -1)
				{
					return -1;					
				}
/*20130618_update*/
				if (fwrite_HostIntToNetByteOrder(&l_recNotReadNum, sizeof(int), 1, fpIndex) != 1)
				{
					return -1;
				}

				sendMsgToVoipServer(USBVM_SC_NEW, l_recNotReadNum);
			}
			rc = l_recNumInAll;		
		}
		else
		{
			rc = -1;
		}
	
/*Edited by huanglei, assert index header info. 2012/01/13*/

	USBVM_ASSERT(l_recNumInAll>=0);
	USBVM_ASSERT(l_recNotReadNum>=0);
	USBVM_ASSERT(l_recNotReadStartPos>=0);
	if(l_recNumInAll==0)
	{
		USBVM_ASSERT(l_recNotReadNum==0&&l_recNotReadStartPos==0);
	}
	else if(l_recNotReadNum>0)
	{
		USBVM_ASSERT(l_recNotReadStartPos>0);
		USBVM_ASSERT(l_recNumInAll>=l_recNotReadNum);
		USBVM_ASSERT(l_recNumInAll>=l_recNotReadStartPos);
		USBVM_ASSERT(l_recNumInAll+1 >= l_recNotReadStartPos+l_recNotReadNum);
	}
	else
	{
		USBVM_ASSERT(l_recNotReadStartPos==0);
	}

/*end huanglei*/
	sync();
	return rc;
}

/* 
 * brief	voiceApp don't need notify voicemail status change at this function
 *			it's done at voip_client.
 */
int usbvm_delRecIndexListByName2_Action(int endpt, unsigned int fileTimName, int notify)
{
	USBVM_REC_FILE_NODE node;
	USBVM_WAV_FIFO_INFO wavFIFOIndex;
	int i = 0, nodePos = 0;
	char *wavFilePathName;
	char wavNameStr[100];
	int rc = 0;
	char cmd[100];
	unsigned int readFlag;
	int recNumInAll = 0;
	int recNotReadNum = 0;
	int recNotReadStartPos = 0;
	char recFilePathName[200];
	FILE* fpIndex = NULL;
	int lockRet = -1;
	UINT64 delta = 0;
	USBVM_STATUC_CHANGE_TYPE changeType = USBVM_SC_DEL_READ;
	
#ifdef DEBUG_DETAILED
	MARK_PRINT("entering usbvm_delRecIndexListByName2:%08x\n", fileTimName);
#endif
	if(endpt==-1)
	{
		return -1;
	}
	lockRet = usbvm_openIndexFpAndLock(endpt, &fpIndex, "rb+", WR_LOCK_WHOLE);
	if(lockRet == -1)
	{

		MARK_PRINT("HL_DBG: Lock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
		return -1;
	}
	usbvm_getStaticVarsFromIndexFile(endpt, &recNumInAll, &recNotReadNum, &recNotReadStartPos,fpIndex);

	if (fpIndex != NULL)
	{
		/* Find node position to delete */
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
			rc = -1;
		}

		/* Delete record file if it exit */
		readFlag = node.readFlag;
		usbvm_wavPathFormat(node, recFilePathName);
		if (access(recFilePathName, F_OK) == 0)
		{
			char tmpFileName[200] = {0};

			strcpy(tmpFileName, recFilePathName);
			usbvm_escapeCharHandle(tmpFileName);
			
			sprintf(cmd,"rm -f %s", tmpFileName);
			system(cmd);
			delta += node.fileSize;
		}

		/* Move subsequent nodes forward one by one after delete it */
		for (i = 0; i < (recNumInAll - nodePos); i++)
		{
			if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE 
					+ (nodePos + i) * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
			{
				lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
				if(lockRet == -1)
			{
				MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			}
				return -1;
			}
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
			if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE 
					+ (nodePos + i - 1) * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
			{
				lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
				if(lockRet == -1)
			{
				MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			}
				return -1;
			}				
/*20130618_update*/				
			if (fwrite_USBVM_REC_FILE_NODE(&node, sizeof(USBVM_REC_FILE_NODE), 1, fpIndex) != 1)
			{
				lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
				if(lockRet == -1)
			{
				MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			}
				return -1;
			}					
		}
		
		/*  [Huang Lei] [28Nov12] reduce usbvm used size */
		usbvm_rewriteUsedSize(delta, FALSE);

		wavFIFOIndex.fileTimeName= fileTimName;
		usbvm_wavFIFODelIndexInfo(wavFIFOIndex);

		/* Recalculate the global variable in record index file */
		if (recNotReadNum > 0)
		{
			/* The node to delete is behind the global start position of not read node */
			if (nodePos > recNotReadStartPos)
			{
				changeType = USBVM_SC_DEL_UNREAD;
				if (readFlag == FALSE)
				{
					/* Decrease global number of not read nodes by 1 */
					recNotReadNum--;
					if (fseek(fpIndex, 8, SEEK_SET) == -1)
					{
						lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
						if(lockRet == -1)
			{
				MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			}
						return -1;
					}							
/*20130618_update*/							
					if (fwrite_HostIntToNetByteOrder(&recNotReadNum, sizeof(int), 1, fpIndex) != 1)
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
			/* The node to delete is the global start position of not read node */
			else if (nodePos == recNotReadStartPos)
			{
				/* Decrease global number of not read nodes by 1 */
				recNotReadNum--;
				changeType = USBVM_SC_DEL_UNREAD;
				if (fseek(fpIndex, 8, SEEK_SET) == -1)
				{
					lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
					if(lockRet == -1)
			{
				MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			}
					return -1;
				}							
/*20130618_update*/
				if (fwrite_HostIntToNetByteOrder(&recNotReadNum, sizeof(int), 1, fpIndex) != 1)
				{
					lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
					if(lockRet == -1)
			{
				MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			}
					return -1;
				}
				/* Find the not read node from current position and update it */
				if (recNotReadStartPos < recNumInAll)
				{
					recNotReadStartPos = usbvm_schNotReadNodeFromCurPos(endpt, recNotReadStartPos, fpIndex);
				}
				else
				{
					recNotReadStartPos = 0;
				}
				if (fseek(fpIndex, 4, SEEK_SET) == -1)
				{
					lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
					if(lockRet == -1)
			{
				MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			}
					return -1;
				}	
/*20130618_update*/	
				if (fwrite_HostIntToNetByteOrder(&recNotReadStartPos, sizeof(int), 1, fpIndex) != 1)
				{
					lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
					if(lockRet == -1)
			{
				MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			}
					return -1;
				}	

			}
			/* The node to delete is before the global start position of not read node */
			else if (nodePos < recNotReadStartPos)
			{
				/* Decrease global start position of not read node by 1 */
				recNotReadStartPos--;
				if (fseek(fpIndex, 4, SEEK_SET) == -1)
				{
					lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
					if(lockRet == -1)
			{
				MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			}
					return -1;
				}	
/*20130618_update*/	
				if (fwrite_HostIntToNetByteOrder(&recNotReadStartPos, sizeof(int), 1, fpIndex) != 1)
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
		/* Decrease global number of record nodes inall by 1 */
		recNumInAll--;
		if (fseek(fpIndex, 0, SEEK_SET) == -1)
		{
			lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
			if(lockRet == -1)
			{
				MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			}
			return -1;
		}	
/*20130618_update*/	
		if (fwrite_HostIntToNetByteOrder(&recNumInAll, sizeof(int), 1, fpIndex) != 1)
		{
			lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
			if(lockRet == -1)
			{
				MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			}
			return -1;
		}	
		
		if (notify)
		{
			sendMsgToVoipServer(changeType, recNotReadNum);
		}
		rc = 0;
	}
	else
	{
		rc = -1;
	}
	lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
	if(lockRet == -1)
	{
		MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
	}

	sync();
	return rc;
}

/* 
 * fn		int usbvm_delRecIndexListByName2( int endpt, unsigned int fileTimName )
 * brief	Delete the record node by name and its relevant record file for other process
 * details	
 *
 * param[in]	endpt  FXS endpoint
 * param[in]	fileTimName  record file name to delete expressed by current time with hex
 * param[out]
 *
 * return	success return 0, fail return -1
 * retval	
 *
 * note		
 */
int usbvm_delRecIndexListByName2( int endpt, unsigned int fileTimName)
{
	return usbvm_delRecIndexListByName2_Action(endpt, fileTimName, 1);
}

/* 
 * fn		int usbvm_delRecIndexListByPos( int endpt, int nodePos )
 * brief	Delete the record node by position and its relevant record file
 * details	nodepos should start with 1. if nodepos==0, this func will not work
 *			properly. please check nodepos before calling this func.
 *
 * param[in]	endpt  FXS endpoint number
 * param[in]	nodePos  node position to delete
 * param[out]
 *
 * return	success return 0, fail return -1
 * retval	
 *
 * note		
 */
int usbvm_delRecIndexListByPos( int endpt, int nodePos, FILE* fpIndex)
{
	USBVM_REC_FILE_NODE node;
	USBVM_WAV_FIFO_INFO wavFIFOIndex;	
	int i;
	char wavFilePathName[200] = {0};
	char wavNameStr[100];
	int rc = 0;
	char cmd[100];
	unsigned int readFlag;
	UINT64 delta = 0;
	int recNumInAll=0;
	int recNotReadNum=0;
	int recNotReadStartPos=0;
#ifdef DEBUG_DETAILED
MARK_PRINT("HL_DBG: entering %s:%d\n",__FUNCTION__,__LINE__);
#endif
	
	if ((endpt >= 0) && (endpt < ENDPT_NUM))
	{
		usbvm_getStaticVarsFromIndexFile(
				endpt, &recNumInAll, &recNotReadNum, &recNotReadStartPos, fpIndex);
		l_recNumInAll = recNumInAll;
		l_recNotReadNum = recNotReadNum;
		l_recNotReadStartPos = recNotReadStartPos;
	
		MARK_PRINT("HL_DBG: NumInAll= %d,NotRead=%d,NotReadSPos=%d in %s:%d\n",\
		l_recNumInAll,l_recNotReadNum,\
		l_recNotReadStartPos,__FUNCTION__,__LINE__);
	}
	else
	{
		return -1;
	}
	USBVM_ASSERT(l_recNumInAll>=0);
	USBVM_ASSERT(l_recNotReadNum>=0);
	USBVM_ASSERT(l_recNotReadStartPos>=0);
	if(l_recNumInAll==0)
	{
		USBVM_ASSERT(l_recNotReadNum==0&&l_recNotReadStartPos==0);
	}
	else if(l_recNotReadNum>0)
	{
		USBVM_ASSERT(l_recNotReadStartPos>0);
		USBVM_ASSERT(l_recNumInAll>=l_recNotReadNum);
		USBVM_ASSERT(l_recNumInAll>=l_recNotReadStartPos);
		USBVM_ASSERT(l_recNumInAll+1 >= l_recNotReadStartPos+l_recNotReadNum);
	}
	else
	{
		USBVM_ASSERT(l_recNotReadStartPos==0);
	}

	if (l_recNumInAll==0)
	{	/*if recNumInAll is 0,there is nothing to be deleted*/
		return -2;
	}
	if (nodePos <= 0)
	{	/*nodePos should be bigger than 0*/
		return -1;
	}
		

		if (fpIndex!= NULL)
		{
			/* Delete record file if it exist */
			if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE 
						+ (nodePos - 1) * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
			{
				return -1;
			}	
/*20130618_update*/	
			if (fread_USBVM_REC_FILE_NODE(&node, sizeof(USBVM_REC_FILE_NODE), 1, fpIndex) != 1)
			{
				return -1;
			}	
			readFlag = node.readFlag;
			delta = node.fileSize;	/*  [Huang Lei] [28Nov12]  */
			usbvm_wavPathFormat(node, wavFilePathName);

			if (access(wavFilePathName, F_OK) == 0)
			{
				char tmpFileName[200] = {0};

				strcpy(tmpFileName, wavFilePathName);
				usbvm_escapeCharHandle(tmpFileName);
				
				sprintf(cmd,"rm -f %s", tmpFileName);
				system(cmd);
			}
			/* Move subsequent nodes forward one by one after delete it */
			for (i = 0; i < (l_recNumInAll - nodePos); i++)
			{
				if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE 
							+ (nodePos + i) * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
				{
					return -1;
				}	
/*20130618_update*/	
				if (fread_USBVM_REC_FILE_NODE(&node, sizeof(USBVM_REC_FILE_NODE), 1, fpIndex) != 1)
				{
					return -1;
				}	
				if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE 
							+ (nodePos + i - 1) * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
				{
					return -1;
				}	
/*20130618_update*/	
				if (fwrite_USBVM_REC_FILE_NODE(&node, sizeof(USBVM_REC_FILE_NODE), 1, fpIndex) != 1)
				{
					return -1;
				}	
			}
			/*  [Huang Lei] [28Nov12] fileSize of index is not changed. subtract the size of 
				recordFile and	wavFile.*/
			if(delta != 0)
			{
				usbvm_rewriteUsedSize(delta,FALSE);
			}
			wavFIFOIndex.fileTimeName = node.fileTimName;
			usbvm_wavFIFODelIndexInfo(wavFIFOIndex);
			/* Recalculate the global variable in record index file */
			if (l_recNotReadNum > 0)
			{
				/* The node to delete is behind the global start position of not read node */
				if (nodePos > l_recNotReadStartPos)
				{
					if (readFlag == FALSE)
					{
						/* Decrease global number of not read nodes by 1 */
						l_recNotReadNum--;
						if (fseek(fpIndex, 8, SEEK_SET) == -1)
						{
							return -1;
						}	
/*20130618_update*/
						if (fwrite_HostIntToNetByteOrder(&l_recNotReadNum, sizeof(int), 1, fpIndex) != 1)
						{
							return -1;
						}	
						sendMsgToVoipServer(USBVM_SC_DEL_UNREAD, l_recNotReadNum);
					}
				}
				/* The node to delete is the global start position of not read node */
				else if (nodePos == l_recNotReadStartPos)
				{
					/* Decrease global number of not read nodes by 1 */
					l_recNotReadNum--;
					if (fseek(fpIndex, 8, SEEK_SET) == -1)
					{
						return -1;
					}	
/*20130618_update*/	
					if (fwrite_HostIntToNetByteOrder(&l_recNotReadNum, sizeof(int), 1, fpIndex) != 1)
					{
						return -1;
					}
					sendMsgToVoipServer(USBVM_SC_DEL_UNREAD, l_recNotReadNum);
					/* Find the not read node from current position and update it */
					if (l_recNotReadStartPos < l_recNumInAll)
					{
						l_recNotReadStartPos = usbvm_schNotReadNodeFromCurPos2(
								endpt, l_recNotReadStartPos, fpIndex);
					}
					else
					{
						l_recNotReadStartPos = 0;
					}
					if (fseek(fpIndex, 4, SEEK_SET) == -1)
					{
						return -1;
					}	
/*20130618_update*/	
					if (fwrite_HostIntToNetByteOrder(&l_recNotReadStartPos, sizeof(int), 1, fpIndex) != 1)
					{
						return -1;
					}	


				}
				/* The node to delete is behind the global start position of not read node */
				else if (nodePos < l_recNotReadStartPos)
				{
					/* Decrease global start position of not read node by 1 */
					l_recNotReadStartPos--;
					sendMsgToVoipServer(USBVM_SC_DEL_READ, l_recNotReadNum);
					if (fseek(fpIndex, 4, SEEK_SET) == -1)
					{
						return -1;
					}	
/*20130618_update*/	
					if (fwrite_HostIntToNetByteOrder(&l_recNotReadStartPos, sizeof(int), 1, fpIndex) != 1)
					{
						return -1;
					}	
				}
			}
			/* Decrease global number of record nodes inall by 1 */
			l_recNumInAll--;
			if (fseek(fpIndex, 0, SEEK_SET) == -1)
			{
				return -1;
			}	
/*20130618_update*/	
			if (fwrite_HostIntToNetByteOrder(&l_recNumInAll, sizeof(int), 1, fpIndex) != 1)
			{
				return -1;
			}	
		}
		else
		{
			rc = -1;
		}
	/*Edited by huanglei, assert index header info. 2012/01/13*/

	USBVM_ASSERT(l_recNumInAll>=0);
	USBVM_ASSERT(l_recNotReadNum>=0);
	USBVM_ASSERT(l_recNotReadStartPos>=0);
	if(l_recNumInAll==0)
	{
		USBVM_ASSERT(l_recNotReadNum==0&&l_recNotReadStartPos==0);
	}
	else if(l_recNotReadNum>0)
	{
		USBVM_ASSERT(l_recNotReadStartPos>0);
		USBVM_ASSERT(l_recNumInAll>=l_recNotReadNum);
		USBVM_ASSERT(l_recNumInAll>=l_recNotReadStartPos);
		USBVM_ASSERT(l_recNumInAll+1 >= l_recNotReadStartPos+l_recNotReadNum);
	}
	else
	{
		USBVM_ASSERT(l_recNotReadStartPos==0);
	}

/*end huanglei*/

	sync();
	return rc;
}


/* 
 * fn		int usbvm_delRecIndexListOnly( int endpt, int nodePos )
 * brief	Delete the record index node only by position
 * details	nodepos should start with 1. if nodepos==0, this func will not work,
 *			and make lots of troubles. please check nodepos before calling this func.
 *
 * param[in]	endpt  FXS endpoint number
 * param[in]	nodePos  node position to delete
 * param[out]
 *
 * return	success return 0, fail return -1
 * retval	
 *
 * note		
 */
int usbvm_delRecIndexListOnly( int endpt, int nodePos, FILE* fpIndex )
{
	USBVM_REC_FILE_NODE node;
	int i;
	int rc = 0;
	unsigned int readFlag;
	int recNumInAll=0;
	int recNotReadNum=0;
	int recNotReadStartPos=0;
#ifdef DEBUG_DETAILED
MARK_PRINT("HL_DBG: entering %s:%d\n",__FUNCTION__,__LINE__);
#endif
	
	if ((endpt >= 0) && (endpt < ENDPT_NUM))
	{
		usbvm_getStaticVarsFromIndexFile(endpt, &recNumInAll, &recNotReadNum, &recNotReadStartPos, fpIndex);
	
		MARK_PRINT("HL_DBG: NumInAll= %d,NotRead=%d,NotReadSPos=%d in %s:%d\n",\
		l_recNumInAll,l_recNotReadNum,\
		l_recNotReadStartPos,__FUNCTION__,__LINE__);
	}
	if (l_recNumInAll==0)
	{	/*if recNumInAll is 0,there is nothing to be deleted*/
		return -1;
	}
/*Edited by huanglei, assert index header info. 2012/01/13*/

	USBVM_ASSERT(l_recNumInAll>=0);
	USBVM_ASSERT(l_recNotReadNum>=0);
	USBVM_ASSERT(l_recNotReadStartPos>=0);
	if(l_recNumInAll==0)
	{
		USBVM_ASSERT(l_recNotReadNum==0&&l_recNotReadStartPos==0);
	}
	else if(l_recNotReadNum>0)
	{
		USBVM_ASSERT(l_recNotReadStartPos>0);
		USBVM_ASSERT(l_recNumInAll>=l_recNotReadNum);
		USBVM_ASSERT(l_recNumInAll>=l_recNotReadStartPos);
		USBVM_ASSERT(l_recNumInAll+1 >= l_recNotReadStartPos+l_recNotReadNum);
	}
	else
	{
		USBVM_ASSERT(l_recNotReadStartPos==0);
	}

/*end huanglei*/

		if (fpIndex != NULL)
		{
			/* Read record index node from record index file */
			
			if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE
					+ (nodePos -1) * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
			{				
				return -1;
			}	
/*20130618_update*/	
			if (fread_USBVM_REC_FILE_NODE(&node, sizeof(USBVM_REC_FILE_NODE), 1, fpIndex) != 1)				
			{
				return -1;
			}	
			readFlag = node.readFlag;
			
			/* Move subsequent nodes forward one by one after delete it */
			for (i = 0; i < (l_recNumInAll - nodePos); i++)
			{
				if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE 
						+ (nodePos + i) * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
				{
					return -1;
				}	
/*20130618_update*/	
				if (fread_USBVM_REC_FILE_NODE(&node, sizeof(USBVM_REC_FILE_NODE), 1, fpIndex) != 1)
				{
					return -1;
				}	
					
				if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE 
						+ (nodePos + i - 1) * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
				{
					return -1;
				}	
/*20130618_update*/
				if (fwrite_USBVM_REC_FILE_NODE(&node, sizeof(USBVM_REC_FILE_NODE), 1, fpIndex) != 1)
				{
					return -1;
				}	
					
			}
			
			/* Recalculate the global variable in record index file */
			if (l_recNotReadNum > 0)
			{
				/* The node to delete is behind the global start position of not read node */
				if (nodePos > l_recNotReadStartPos)
				{
					if (readFlag == FALSE)
					{
						/* Decrease global number of not read nodes by 1 */
						l_recNotReadNum--;
						if (fseek(fpIndex, 8, SEEK_SET) == -1)
						{
							return -1;
						}	
/*20130618_update*/					
						if (fwrite_HostIntToNetByteOrder(&l_recNotReadNum, sizeof(int), 1, fpIndex) != 1)
						{
							return -1;
						}	

					}
				}
				/* The node to delete is the global start position of not read node */
				else if (nodePos == l_recNotReadStartPos)
				{
					/* Decrease global number of not read nodes by 1 */
					l_recNotReadNum--;
					if (fseek(fpIndex, 8, SEEK_SET) == -1)
					{
						return -1;
					}	
/*20130618_update*/	
					if (fwrite_HostIntToNetByteOrder(&l_recNotReadNum, sizeof(int), 1, fpIndex) != 1)
					{
						return -1;
					}
					/* Find the not read node from current position and update it */
					if (l_recNotReadStartPos < l_recNumInAll)
					{
						l_recNotReadStartPos = usbvm_schNotReadNodeFromCurPos2(
							endpt, l_recNotReadStartPos,fpIndex);
					}
					else
					{
						l_recNotReadStartPos = 0;
					}
					if (fseek(fpIndex, 4, SEEK_SET) == -1)
					{
						return -1;
					}	
/*20130618_update*/		
					if (fwrite_HostIntToNetByteOrder(&l_recNotReadStartPos, sizeof(int), 1, fpIndex) != 1)
					{
						return -1;
					}	
						
				}
				/* The node to delete is before the global start position of not read node */
				else if (nodePos < l_recNotReadStartPos)
				{
					l_recNotReadStartPos--;
					if (fseek(fpIndex, 4, SEEK_SET) == -1)
					{
						return -1;
					}	
/*20130618_update*/	
					if (fwrite_HostIntToNetByteOrder(&l_recNotReadStartPos, sizeof(int), 1, fpIndex) != 1)
					{
						return -1;
					}	
						
				}
			}
			/* Decrease global number of record nodes inall by 1 */
			if(l_recNumInAll == 0)
			{
				return -1;
			}
			l_recNumInAll--;
		#ifdef DEBUG_DETAILED
			MARK_PRINT("usbvm_delRecIndexListOnly l_recNumInAll:%d\n", l_recNumInAll);
		#endif
			if (fseek(fpIndex, 0, SEEK_SET) == -1)
			{
				return -1;
			}	
/*20130618_update*/	
			if (fwrite_HostIntToNetByteOrder(&l_recNumInAll, sizeof(int), 1, fpIndex) != 1)
			{
				return -1;
			}
		}
		else
		{
			rc = -1;
		}
	/*Edited by huanglei, assert index header info. 2012/01/13*/

	USBVM_ASSERT(l_recNumInAll>=0);
	USBVM_ASSERT(l_recNotReadNum>=0);
	USBVM_ASSERT(l_recNotReadStartPos>=0);
	if(l_recNumInAll==0)
	{
		USBVM_ASSERT(l_recNotReadNum==0&&l_recNotReadStartPos==0);
	}
	else if(l_recNotReadNum>0)
	{
		USBVM_ASSERT(l_recNotReadStartPos>0);
		USBVM_ASSERT(l_recNumInAll>=l_recNotReadNum);
		USBVM_ASSERT(l_recNumInAll>=l_recNotReadStartPos);
		USBVM_ASSERT(l_recNumInAll+1 >= l_recNotReadStartPos+l_recNotReadNum);
	}
	else
	{
		USBVM_ASSERT(l_recNotReadStartPos==0);
	}

/*end huanglei*/

	sync();
	return rc;
}


/* 
 * fn		int usbvm_delRecIndexListBackward( int endpt, int nodePos )
 * brief	Delete all the record nodes before specified position and their relevant record files
 * details	
 *
 * param[in]	endpt  FXS endpoint number
 * param[in]	nodePos  specified node position to delete backward
 * param[out]
 *
 * return	success return 0, fail return -1
 * retval	
 *
 * note		
 */
int usbvm_delRecIndexListBackward( int endpt, int nodePos, FILE* fpIndex)
{
	USBVM_REC_FILE_NODE node;
	USBVM_WAV_FIFO_INFO wavFIFOIndex;	
	int i = 0;
	char wavFilePathName[200] = {0};
	int rc = 0;
	char cmd[100] = { 0 };
	unsigned int notReadCount = 0;
	UINT64 delta = 0;
	if(endpt==-1)
	{
		return -1;
	}
#ifdef DEBUG_DETAILED
	MARK_PRINT("l_recNumInAll(%d)\n l_recNotReadNum(%d)\n l_recNotReadStartPos(%d)\n",
		l_recNumInAll,
		l_recNotReadNum,
		l_recNotReadStartPos);
#endif
	/*Edited by huanglei, assert index header info. 2012/01/13*/

	USBVM_ASSERT(l_recNumInAll>=0);
	USBVM_ASSERT(l_recNotReadNum>=0);
	USBVM_ASSERT(l_recNotReadStartPos>=0);
	if(l_recNumInAll==0)
	{
		USBVM_ASSERT(l_recNotReadNum==0&&l_recNotReadStartPos==0);
	}
	else if(l_recNotReadNum>0)
	{
		USBVM_ASSERT(l_recNotReadStartPos>0);
		USBVM_ASSERT(l_recNumInAll>=l_recNotReadNum);
		USBVM_ASSERT(l_recNumInAll>=l_recNotReadStartPos);
		USBVM_ASSERT(l_recNumInAll+1 >= l_recNotReadStartPos+l_recNotReadNum);
	}
	else
	{
		USBVM_ASSERT(l_recNotReadStartPos==0);
	}

/*end huanglei*/

		if (fpIndex != NULL)
		{
			for (i = 0; i < nodePos; i++)
			{
				/* Delete record file if it exit */
				if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE 
							+ i * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
				{
					return -1;
				}
/*20130618_update*/
				if (fread_USBVM_REC_FILE_NODE(&node, sizeof(USBVM_REC_FILE_NODE), 1, fpIndex) != 1)
				{
					return -1;
				}
				if (node.readFlag == FALSE)
				{
					notReadCount++;
				}

				usbvm_wavPathFormat(node, wavFilePathName);
				if (access(wavFilePathName, F_OK) == 0)
				{
					char tmpFileName[200] = {0};

					strcpy(tmpFileName, wavFilePathName);
					usbvm_escapeCharHandle(tmpFileName);
					
					sprintf(cmd,"rm -f %s", tmpFileName);
					system(cmd);
					delta += node.fileSize;
				}
				wavFIFOIndex.endpt = node.fileTimName;
				wavFIFOIndex.fileTimeName= i + 1;
				usbvm_wavFIFODelIndexInfo(wavFIFOIndex);			
			}
			/* Move subsequent nodes forward one by one after delete them */
			for (i = 0; i < (l_recNumInAll - nodePos); i++)
			{
				if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE 
							+ (nodePos + i) * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
				{
					return -1;
				}					
/*20130618_update*/					
				if (fread_USBVM_REC_FILE_NODE(&node, sizeof(USBVM_REC_FILE_NODE), 1, fpIndex) != 1)
				{
					return -1;
				}					
				if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE 
							+  i * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
				{
					return -1;
				}					
/*20130618_update*/					
				if (fwrite_USBVM_REC_FILE_NODE(&node, sizeof(USBVM_REC_FILE_NODE), 1, fpIndex) != 1)
				{
					return -1;
				}					
			}
			/*  [Huang Lei] [28Nov12] rewrite the used size of usbvm */
			usbvm_rewriteUsedSize(delta, FALSE);
			l_recNumInAll = l_recNumInAll - nodePos;
			if (l_recNotReadNum > 0)
			{
				/* Recalculate the global number of not read nodes */
				l_recNotReadNum -= notReadCount;
				if (fseek(fpIndex, 8, SEEK_SET) == -1)
				{
					return -1;
				}						
/*20130618_update*/
				if (fwrite_HostIntToNetByteOrder(&l_recNotReadNum, sizeof(int), 1, fpIndex) != 1)
				{
					return -1;
				}	
				
				/* The specified node is behind or equal the global start position of not read node */
				if (nodePos >= l_recNotReadStartPos)
				{											

					/* Recalculate the global start position of not read node */
					l_recNotReadStartPos = 1;
					l_recNotReadStartPos = usbvm_schNotReadNodeFromCurPos2(
								endpt, l_recNotReadStartPos,fpIndex);
					if (fseek(fpIndex, 4, SEEK_SET) == -1)
					{
						return -1;
					}						
/*20130618_update*/
					if (fwrite_HostIntToNetByteOrder(&l_recNotReadStartPos, sizeof(int), 1, fpIndex) != 1)
					{
						return -1;
					}						
		
				}
				/* The specified node is before the global start position of not read node */
				else if (nodePos < l_recNotReadStartPos)
				{
					/* Recalculate the global start position of not read node */
					l_recNotReadStartPos -= nodePos;
					if (fseek(fpIndex, 4, SEEK_SET) == -1)
					{
						return -1;
					}
/*20130618_update*/
					if (fwrite_HostIntToNetByteOrder(&l_recNotReadStartPos, sizeof(int), 1, fpIndex) != 1)
					{
						return -1;
					}
						
				}
			}
			if (notReadCount > 0)
			{
				sendMsgToVoipServer(USBVM_SC_DEL_UNREAD, l_recNotReadNum);
			}
			else
			{
				sendMsgToVoipServer(USBVM_SC_DEL_READ, l_recNotReadNum);
			}
			
			/* Recalculate the global number of record nodes inall */
			if (fseek(fpIndex, 0, SEEK_SET) == -1)
			{
				return -1;
			}
/*20130618_update*/				
			if (fwrite_HostIntToNetByteOrder(&l_recNumInAll, sizeof(int), 1, fpIndex) != 1)
			{
				return -1;
			}				
		}
		else
		{
			rc = -1;
		}
		
#ifdef DEBUG_DETAILED
	MARK_PRINT("l_recNumInAll(%d)\n l_recNotReadNum(%d)\n l_recNotReadStartPos(%d)\n",
		l_recNumInAll,
		l_recNotReadNum,
		l_recNotReadStartPos);
#endif
	/*Edited by huanglei, assert index header info. 2012/01/13*/

	USBVM_ASSERT(l_recNumInAll>=0);
	USBVM_ASSERT(l_recNotReadNum>=0);
	USBVM_ASSERT(l_recNotReadStartPos>=0);
	if(l_recNumInAll==0)
	{
		USBVM_ASSERT(l_recNotReadNum==0&&l_recNotReadStartPos==0);
	}
	else if(l_recNotReadNum>0)
	{
		USBVM_ASSERT(l_recNotReadStartPos>0);
		USBVM_ASSERT(l_recNumInAll>=l_recNotReadNum);
		USBVM_ASSERT(l_recNumInAll>=l_recNotReadStartPos);
		USBVM_ASSERT(l_recNumInAll+1 >= l_recNotReadStartPos+l_recNotReadNum);
	}
	else
	{
		USBVM_ASSERT(l_recNotReadStartPos==0);
	}

/*end huanglei*/

	sync();
	return rc;
}

/* 
 * fn		int usbvm_delAllRecIndexList( int endpt)
 * brief	Delete all the record nodes 
 * details	
 *
 * param[in]	endpt  FXS endpoint number
 * param[out]
 *
 * return	success return 0, fail return -1
 * retval	
 *
 * note		
 */
int usbvm_delAllRecIndexList(int endpt)
{
	int lockRet = -1;
	FILE* fpIndex = NULL;
	int recAll = 0;
	int ret = -1;
	
	lockRet = usbvm_openIndexFpAndLock(endpt, &fpIndex, "rb+", WR_LOCK_WHOLE);
	if(lockRet != -1)
	{
		if ((recAll = usbvm_getRecNumInAll(endpt, fpIndex)) > 0)
		{		
			ret = usbvm_delRecIndexListBackward(endpt, recAll, fpIndex);
		}
		
		lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, WR_LOCK_WHOLE);
		if(lockRet == -1)
		{
			MARK_PRINT("HL_DBG: Unlock index file failed! ");
		}
	}
	else
	{
		MARK_PRINT("HL_DBG: Lock index file failed! ");
	}

	return ret;
}

/* 
 * fn		int usbvm_schRecIndexListLatestExpired( int endpt, unsigned int expiredDays )
 * brief	Search the expired record node position
 * details	
 *
 * param[in]	endpt  FXS endpoint number
 * param[in]	expiredDays  expired days that a record can survive
 * param[out]	
 *
 * return	expired record exist return the latest expired node postion, else return -1
 * retval	
 *
 * note		/ Huang Lei 2012/10/26 : do not use current time to substruct a expiredtime.
 *			if the time is too small, such as 1970/1/1/00:00:01,the result is not correct.
 *			Now directly substruct with two time stamps,and compare the result with expiredTime.
 *			
 */
int usbvm_schRecIndexListLatestExpired( int endpt, unsigned int expiredDays, FILE* fpIndex )
{
	USBVM_REC_FILE_NODE node;
	unsigned int expiredTime = 0;
	unsigned int latestExpFileName = 0;
	int i = 0, latestToutNodePos = 0;
	int rc = 0;

	expiredTime = expiredDays * 24 * 60 * 60;
	/* Edited by Huang Lei, 2012/10/26*/ 
	/* here's the old version */
	/* 
	latestExpFileName = time(NULL) - expiredTime;
	*/
	 /* here's the new version */
	latestExpFileName = (unsigned int)time(NULL);
	/* Edit End */

		if (fpIndex != NULL)
		{
			/* Search the latest expired record position */
			for (i = 0; i < l_recNumInAll; i++)
			{
				if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE 
							+  i * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
				{
					rc = -1;
					break;
				}
/*20130618_update*/
				if (fread_USBVM_REC_FILE_NODE(&node, sizeof(USBVM_REC_FILE_NODE), 1, fpIndex) != 1)
				{
					rc = -1;
					break;
				}

				if ((i == 0)
					&& ((latestExpFileName < node.fileTimName)	/* current time <file's time */
						||(latestExpFileName - node.fileTimName < expiredTime)))  
				/* no expired record */
				{
					rc = -1;
					break;
				}
				else if ((latestExpFileName < node.fileTimName)	/* current time <file's time */
								||(latestExpFileName - node.fileTimName < expiredTime))
				/* some expired, the others(after 'i') didn't */
				{
					latestToutNodePos = i;
					break;
				}
				else if ((i == l_recNumInAll - 1) 
					&& (latestExpFileName - node.fileTimName >= expiredTime)) 
				{	/* all expired */
					latestToutNodePos = l_recNumInAll;
					break;
				}
			}
			
			if (rc == 0)
			{
				rc = latestToutNodePos;
			}

		}
	

	return rc;
}


/* 
 * fn		int usbvm_schNotReadNodeFromCurPos( int endpt, int nodePos )
 * brief	Search the first not-read node from current position
 * details	if not-read node doesn't exist,return the current node
 *
 * param[in]	endpt  FXS endpoint number
 * param[in]	nodePos  the start position to search from
 * param[out]
 *
 * return	success return >0, else return 0
 * retval	
 *
 * note		l_recNumInAll is changed to recNumInAll,which is got by reading from file.
 */
int usbvm_schNotReadNodeFromCurPos( int endpt, int nodePos, FILE* fpIndex)
{
	int i;
	USBVM_REC_FILE_NODE node;
	int retNodePos = 0;
	int recNumInAll = 0;
	int notReadNumInAll = 0;
	int notReadStartPos = 0;
	/*Get static 'recNumInAll' by reading index file*/
	if ((endpt >= 0) && (endpt < ENDPT_NUM))
	{
		usbvm_getStaticVarsFromIndexFile(endpt, &recNumInAll, &notReadNumInAll, &notReadStartPos, fpIndex);
		if(0 == recNumInAll)
		{
			MARK_PRINT("HL_DBG: read index,recNumInAll is 0, %s:%d\n",__FUNCTION__,__LINE__);
			return 0;
		}
	}

	if (fpIndex != NULL)
	{
		for (i = 0; i < recNumInAll - nodePos + 1; i++)
		{
			if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE 
				+ (nodePos + i - 1) * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
			{
				retNodePos = 0;
				break;
			}
/*20130618_update*/
			if (fread_USBVM_REC_FILE_NODE(&node, sizeof(USBVM_REC_FILE_NODE), 1, fpIndex) != 1)
			{
				retNodePos = 0;
				break;
			}
			if (node.readFlag == FALSE)
			{
				retNodePos = nodePos + i;
				break;
			}
		}
		
	}

	return retNodePos;	
}


/* 
 * fn		int usbvm_schNotReadNodeFromCurPos2( int endpt, int nodePos, FILE *fp )
 * brief	Search the first not-read node from current position
 * details	
 *
 * param[in]	endpt  FXS endpoint number
 * param[in]	nodePos  the start position to search from	
 * param[in]	*fp  record index file
 *
 * return	success return >0, else return 0
 * retval	
 *
 * note		
 */
int usbvm_schNotReadNodeFromCurPos2( int endpt, int nodePos, FILE *fp )
{
	int i;
	USBVM_REC_FILE_NODE node;
	int retNodePos = 0;

	if (fp != NULL)
	{
		for (i = 0; i < l_recNumInAll - nodePos + 1; i++)
		{
			if (fseek(fp, RECLIST_INDEX_AREA_SIZE 
					+ (nodePos + i - 1) * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
			{
				retNodePos = 0;
				break;
			}
/*20130618_update*/			if (fread_USBVM_REC_FILE_NODE(&node, sizeof(USBVM_REC_FILE_NODE), 1, fp) != 1)
			{
				retNodePos = 0;
				break;
			}
			if (node.readFlag == FALSE)
			{
				retNodePos = nodePos + i;
				break;
			}
		}
	}

	return retNodePos;	
}


/* 
 * fn		int usbvm_schNotReadNodeFromHead( int endpt, FILE *fp )
 * brief	Search the first not-read node from head
 * details	
 *
 * param[in]	endpt  FXS endpoint number
 * param[in]	*fp  record index file	
 * param[out]	
 *
 * return	success return >0, else return 0
 * retval	
 *
 * note		
 */
int usbvm_schNotReadNodeFromHead( int endpt, FILE *fp )
{
	int i;
	USBVM_REC_FILE_NODE node;
	int retNodePos = 0;
	
	if (fp != NULL)
	{
		for (i = 0; i < l_recNumInAll; i++)
		{
			if (fseek(fp, RECLIST_INDEX_AREA_SIZE +  (i) * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
			{
				retNodePos = 0;
				break;
			}
/*20130618_update*/
			if (fread_USBVM_REC_FILE_NODE(&node, sizeof(USBVM_REC_FILE_NODE), 1, fp) != 1)
			{
				retNodePos = 0;
				break;				
			}
			if (node.readFlag == FALSE)
			{
				retNodePos = i + 1;
				break;
			}
		}
	}
		

	return retNodePos;	
}


/* 
 * fn		int usbvm_updRecIndexListInfoAfterR( int endpt, USBVM_REC_FILE_LIST precNode )
 * brief	Update record index node info and record file header after read
 * details	
 *
 * param[in]	endpt  FXS endpoint number
 * param[out]	precNode  pointer of record index node 
 *
 * return	success return 0, fail return -1
 * retval	
 *
 * note		
 */
int usbvm_updRecIndexListInfoAfterR( int endpt, USBVM_REC_FILE_LIST precNode,FILE* fpIndex)
{
	FILE *fp = NULL;
	int rc = 0;
	USBVM_REC_FILE_NODE recNodeTmp;
#ifdef DEBUG_DETAILED
	MARK_PRINT("entering usbvm_updRecIndexListInfoAfterR endpt:%d, filename:%08x readflag:%d\n", 
		endpt, precNode->fileTimName, precNode->readFlag);
#endif
	if(endpt==-1)
	{
		return -1;
	}
	rc = usbvm_getStaticVarsFromIndexFile(endpt, 
			&l_recNumInAll, &l_recNotReadNum, &l_recNotReadStartPos, fpIndex);
	if(rc == -1)
	{

		MARK_PRINT("HL_DBG: get usbvm globals error! [%s:%d]\n",__FUNCTION__,__LINE__);
		return -1;
	}

	USBVM_ASSERT(l_recNumInAll>=0);
	USBVM_ASSERT(l_recNotReadNum>=0);
	USBVM_ASSERT(l_recNotReadStartPos>=0);
	if(l_recNumInAll==0)
	{
		USBVM_ASSERT(l_recNotReadNum==0&&l_recNotReadStartPos==0);
	}
	else if(l_recNotReadNum>0)
	{
#ifdef DEBUG_DETAILED
		MARK_PRINT("HL_DBG: l_recNotReadStartPos=%d, [%s:%d]\n",
			l_recNotReadStartPos,__FUNCTION__,__LINE__);
#endif
		
		USBVM_ASSERT(l_recNotReadStartPos>0);
		USBVM_ASSERT(l_recNumInAll>=l_recNotReadNum);
		USBVM_ASSERT(l_recNumInAll>=l_recNotReadStartPos);
		USBVM_ASSERT(l_recNumInAll+1 >= l_recNotReadStartPos+l_recNotReadNum);
	}
	else
	{
		USBVM_ASSERT(l_recNotReadStartPos==0);
	}

	
		if (fpIndex != NULL)
		{
			if(l_recCurNode[endpt]==0)
			{
				MARK_PRINT("HL_DBG: l_recCurNode[endpt]==0, leaving function:%s\n",__FUNCTION__);
				return -1;
			}

			if ((precNode->readFlag == FALSE) && (precNode->fileTimName != 0))
			{
				/* Check the node in Indexfile is right or not,before rewrite!  */
				if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE 
						+ (l_recCurNode[endpt] - 1) * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
				{
					return -1;
				}
/*20130618_update*/
				if (fread_USBVM_REC_FILE_NODE(&recNodeTmp, sizeof(USBVM_REC_FILE_NODE), 1, fpIndex) != 1)
				{
					return -1;
				}
				if(recNodeTmp.fileTimName != precNode->fileTimName)
				{
				/* If the filenames are not the same, don't write any data to this place*/
					return -1;
				}

				
				/* Update the record index node in record index file */
				precNode->readFlag = TRUE;
				usbvm_calChkSum(precNode);
				if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE 
						+ (l_recCurNode[endpt] - 1) * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
				{
					return -1;
				}
/*20130618_update*/
				if (fwrite_USBVM_REC_FILE_NODE(precNode, sizeof(USBVM_REC_FILE_NODE), 1, fpIndex) != 1)
				{
					return -1;
				}
				/* Recalculate the global number of not read record nodes */
				l_recNotReadNum--;
				if (fseek(fpIndex, 8, SEEK_SET) == -1)
				{
					return -1;
				}
/*20130618_update*/
				if (fwrite_HostIntToNetByteOrder(&l_recNotReadNum, sizeof(int), 1, fpIndex) != 1)
				{
					return -1;
				}
				sendMsgToVoipServer(USBVM_SC_LISTEN, l_recNotReadNum);
				
				/* Recalculate the global start position of not read record node */
				if (l_recNotReadNum == 0)
				{
					l_recNotReadStartPos = 0;
				}
				else
				{
					l_recNotReadStartPos = usbvm_schNotReadNodeFromHead(endpt, fpIndex);
				}
				if (fseek(fpIndex, 4, SEEK_SET) == -1)
				{
					return -1;
				}					
/*20130618_update*/					
				if (fwrite_HostIntToNetByteOrder(&l_recNotReadStartPos, sizeof(int), 1, fpIndex) != 1)
				{
					return -1;
				}
			}
			

		}
		else
		{
			rc = -1;
		}
#ifdef DEBUG_DETAILED
		MARK_PRINT("exit %s, l_recNotReadNum:%d, l_recNotReadStartPos:%d\n",
		__FUNCTION__,
		l_recNotReadNum,
		l_recNotReadStartPos);
#endif
	
	
	/*Edited by huanglei, assert index header info. 2012/01/13*/

	USBVM_ASSERT(l_recNumInAll>=0);
	USBVM_ASSERT(l_recNotReadNum>=0);
	USBVM_ASSERT(l_recNotReadStartPos>=0);
	if(l_recNumInAll==0)
	{
		USBVM_ASSERT(l_recNotReadNum==0&&l_recNotReadStartPos==0);
	}
	else if(l_recNotReadNum>0)
	{
		USBVM_ASSERT(l_recNotReadStartPos>0);
		USBVM_ASSERT(l_recNumInAll>=l_recNotReadNum);
		USBVM_ASSERT(l_recNumInAll>=l_recNotReadStartPos);
		USBVM_ASSERT(l_recNumInAll+1 >= l_recNotReadStartPos+l_recNotReadNum);
	}
	else
	{
		USBVM_ASSERT(l_recNotReadStartPos==0);
	}

	/* end huanglei */

	sync();
	return rc;

}


/* 
 * fn		int usbvm_getRecIndexListNodeInfo( int endpt, int nodePos, USBVM_REC_FILE_LIST precNode )
 * brief	Get the specified record file info
 * details	
 *
 * param[in]	endpt  FXS endpoit number 
 * param[in]	nodePos  specified node position to get info
 * param[out]	precNode  pointer of record index node
 *
 * return	succeed return 0, else return < 0
 * retval	
 *
 * note		
 */
int usbvm_getRecIndexListNodeInfo( int endpt, int nodePos, USBVM_REC_FILE_LIST precNode)
{
	int rc = 0;
	int nodePos_real=nodePos+1;
	FILE* fpIndex = NULL;
	int lockRet = -1;
#ifdef DEBUG_DETAILED
	/* Added by huanglei for debug print, 2012/01/30 */
	MARK_PRINT("HL_DBG: HTTP! ertering [%s:%d]\n",__FUNCTION__,__LINE__);
	/* end huanglei */
#endif
	
	if(endpt==-1)
	{
		return -1;
	}
	lockRet = usbvm_openIndexFpAndLock(endpt, &fpIndex, "rb", RD_LOCK_BODY);
	if(lockRet!=-1)
	{
			
		if (nodePos_real > 0)
		{
			/* Check whether the record index node in record index file is correct */
			if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE 
						+ (nodePos_real-1) * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
			{
/*20130618_update*/
				TT_PRINT("calling usbvm_closeIndexFpAndUnlock...");
				lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, RD_LOCK_BODY);
				if(lockRet == -1)
				{
					MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
				}
				return -1;
			}
/*20130618_update*/
			if (fread_USBVM_REC_FILE_NODE(precNode, sizeof(USBVM_REC_FILE_NODE), 1, fpIndex) != 1)
			{
				lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, RD_LOCK_BODY);
				if(lockRet == -1)
				{
					MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
				}
				return -1;
			}
/*20130618_update*/
			TT_PRINT("calling usbvm_chkSum...");
				
			if (usbvm_chkSum(precNode) != 0)
			{
/*20130618_update*/
				TT_PRINT("usbvm_chkSum return != 0, ret -1");
				rc = -1;
			}

		}
		else
		{
			lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, RD_LOCK_BODY);
			if(lockRet == -1)
			{
				MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			}
			return -1;
		}

		lockRet = usbvm_closeIndexFpAndUnlock(endpt, fpIndex, RD_LOCK_BODY);
		if(lockRet == -1)
		{
			MARK_PRINT("HL_DBG: Unlock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
			return -1;
		}
	}
	else
	{
		MARK_PRINT("HL_DBG: Lock index file failed! [%s:%d]\n",__FUNCTION__,__LINE__);
		return -1;
	}
	return rc;
}


/* 
 * fn		int usbvm_getRecIndexListNodeInfoAndFd( int endpt, int nodePos, USBVM_REC_FILE_LIST precNode, FILE **fppRecord )
 * brief	Get the specified record file info
 * details	
 *
 * param[in]	endpt  FXS endpoit number 
 * param[in]	nodePos  specified node position to get info
 * param[out]	**fppRecord  pointer of record file
 * param[out]	precNode  pointer of record index node
 *
 * return	success return 0, else return < 0
 * retval	
 *
 * note		
 */
int usbvm_getRecIndexListNodeInfoAndFd( int endpt, int nodePos, 
		USBVM_REC_FILE_LIST precNode, FILE **fppRecord, FILE* fpIndex )
{
	int rc = 0;
	USBVM_REC_FILE_NODE recNodeTmp;
	char recFilePathName[100];
#ifdef DEBUG_DETAILED
	MARK_PRINT("HL_DBG: Entering %s: %d, nodepos=%d,\n", __FUNCTION__,__LINE__,nodePos);
#endif
	if(endpt==-1)
	{
		return -1;
	}
	
	if (fpIndex != NULL)
	{
		if (nodePos > 0)
		{
			/* Check whether the record index node in record index file is correct */
			if (fseek(fpIndex, RECLIST_INDEX_AREA_SIZE 
					+ (nodePos - 1) * sizeof(USBVM_REC_FILE_NODE), SEEK_SET) == -1)
			{
				return -11;
			}
			errno = 0;
/*20130618_update*/
			int ret=fread_USBVM_REC_FILE_NODE(precNode, sizeof(USBVM_REC_FILE_NODE), 1, fpIndex);
			if ( ret != 1)
			{
				MARK_PRINT("HL_DBG: errno(%d),ret(%d),[%s:%d]\n",errno,ret,__FUNCTION__,__LINE__);
				return -12;
			}
				

			if((usbvm_chkSum(precNode) != 0)
				||(precNode->fileSize ==0))
			{
				rc = -13;
			}

		}
		
	}
	else
	{
		return -14;
	}
	/* If the record index node is correct */
	if (rc == 0)
	{
		/* Check whether the record file header is correct */
		usbvm_wavPathFormat(*precNode, recFilePathName);

		if (access(recFilePathName, F_OK) == 0)
		{
			if (access(recFilePathName, R_OK | W_OK) != 0)
			{
				chmod(recFilePathName, 0777);										
			}
		
			if ((*fppRecord = fopen(recFilePathName, "rb+")) != NULL)
			{
				if (fseek(*fppRecord, 0, SEEK_SET) == -1)
				{
					fclose(*fppRecord);
					*fppRecord = NULL;
					return -9;
				}
			}
			else
			{
				rc = -4;
			}			
		}		
		else
		{
			MARK_PRINT("recFilePathName:%s doesn't exist!\n", recFilePathName);
			rc = -5;
			if (usbvm_delRecIndexListOnly(endpt, nodePos, fpIndex) < 0)
			{
				rc = -6;
			}
		}			
	}
	else  /* If the record index node is not correct, delete it from record index list */
	{
		if (usbvm_delRecIndexListOnly(endpt, nodePos, fpIndex) < 0)
		{
			rc = -7;
		}
	}

	if (rc!=0)
	{
		MARK_PRINT("%s retval:%d\n", __FUNCTION__, rc);
	}
	return rc;
}


/* 
 * fn		unsigned int usbvm_getFileSize( const char *pfilePathName ) 
 * brief	Get file size of the specified file
 * details	
 *
 * param[in]	*pfilePathName  file name with absolute path
 * param[out]	
 *
 * return	success return filesize, fail return 0
 * retval	
 *
 * note		
 */
unsigned int usbvm_getFileSize( const char *pfilePathName )
{ 
	struct stat statbuf; 
	
	if (stat(pfilePathName,&statbuf)==-1) 
	{ 
		MARK_PRINT("usbvm_getFileSize Get stat on %s Error\n", pfilePathName); 
		return(0); 
	}
	/* Is a directory */
	if (S_ISDIR(statbuf.st_mode))
	{
		return(0); 
	}
	/* Is a regular file */
	if (S_ISREG(statbuf.st_mode))
	{
		return (statbuf.st_size); 	
	}
	
	return(0); 
}


/* 
 * fn		void usbvm_wavFIFOPutIndexInfo( USBVM_WAV_FIFO_INFO indexInfo )
 * brief	Input a WAV node to WAV FIFO buffer
 * details	
 *
 * param[in]	indexInfo  WAV node input to FIFO
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
void usbvm_wavFIFOPutIndexInfo( USBVM_WAV_FIFO_INFO indexInfo )
{
	/* WAV FIFO buffer is not full */
	if (l_fifoBufState != WAV_FIFO_FULL)			
	{
		/* Add the  WAV node to the tail of FIFO buffer */
		l_wavFIFOBuf[l_fifoBufTail].fileTimeName = indexInfo.fileTimeName;
		l_fifoBufState = WAV_FIFO_DATA;

		l_fifoBufTail++;
	#ifdef DEBUG_DETAILED
		MARK_PRINT("wav FIFO buffer available, len=%d\n", l_fifoBufTail - l_fifoBufHead)
	#endif
		if (l_fifoBufTail == WAV_FIFO_SIZE)
		{
			l_fifoBufTail = 0;
		}

		if (l_fifoBufTail == l_fifoBufHead)
		{
			l_fifoBufState = WAV_FIFO_FULL;
		}
	}

#ifdef DEBUG_DETAILED
	int i ;
/* Added by huanglei for debug print, 2012/02/02 */
MARK_PRINT("HL_DBG: headPos=%d,tailPos=%d, [%s:%d]\n",l_fifoBufHead,l_fifoBufTail,__FUNCTION__,__LINE__);
	
	for(i= l_fifoBufHead;i<=l_fifoBufTail;i++)
	{
		/* Added by huanglei for debug print, 2012/02/02 */
		MARK_PRINT("HL_DBG: l_wavFIFOBuf[%d].endpt=%d,l_wavFIFOBuf[%d].filetimename=%08x [%s:%d]\n",
		i,l_wavFIFOBuf[i].endpt,i,l_wavFIFOBuf[i].fileTimeName,__FUNCTION__,__LINE__);
		/* end huanglei */
	}
#endif
}

/* 
 * fn		int usbvm_wavFIFOGetIndexInfo( USBVM_WAV_FIFO_INFO *pIndexInfo )
 * brief	Get the WAV node from WAV FIFO buffer
 * details	
 *
 * param[in]	
 * param[out]	*pIndexInfo  pointer of WAV node
 *
 * return	there is at least one node in FIFO return TRUE, else return FALSE
 * retval	
 *
 * note		
 */
int usbvm_wavFIFOGetIndexInfo( USBVM_WAV_FIFO_INFO *pIndexInfo )
{
	/* WAV FIFO buffer is not empty */
	if (l_fifoBufState != WAV_FIFO_EMPTY)		
	{
		/* Get the WAV node from the head of FIFO */
		pIndexInfo->fileTimeName= l_wavFIFOBuf[l_fifoBufHead].fileTimeName;
		l_fifoBufState = WAV_FIFO_DATA;
	#ifdef DEBUG_DETAILED
		MARK_PRINT("HL_DBG:get pIndexInfo->endpt=%d,pIndexInfo->fileTimename=%08x, [%s:%d]\n",
			pIndexInfo->endpt,
			pIndexInfo->fileTimeName,
			__FUNCTION__,__LINE__);
	#endif	
		l_fifoBufHead++;
		if (l_fifoBufHead == WAV_FIFO_SIZE)
		{
			l_fifoBufHead = 0;
		}

		if (l_fifoBufTail == l_fifoBufHead)
		{
			l_fifoBufState = WAV_FIFO_EMPTY;
		}

		return TRUE;
	}

	return FALSE;
}


/* 
 * fn		void usbvm_wavFIFODelIndexInfo( USBVM_WAV_FIFO_INFO pIndexInfo )
 * brief	Delete the specified WAV node from WAV FIFO
 * details	
 *
 * param[in]	IndexInfo  specified WAV node to delete
 * param[out]	
 *
 * return	
 * retval	
 *
 * note		
 */
void usbvm_wavFIFODelIndexInfo( USBVM_WAV_FIFO_INFO IndexInfo )
{
	int headTmp;
	BOOL bMatch = FALSE;

	/* WAV FIFO buffer is not empty */
	if (l_fifoBufState != WAV_FIFO_EMPTY)		
	{
		l_fifoBufState = WAV_FIFO_DATA;
		headTmp = l_fifoBufHead;

		/* Search the specified WAV node */
		while(headTmp != l_fifoBufTail)
		{
			if ((l_wavFIFOBuf[headTmp].fileTimeName == IndexInfo.fileTimeName))
			{
				bMatch = TRUE;
				break;
			}
				
			headTmp++;
			/* Start to search from 0 */
			if (headTmp == WAV_FIFO_SIZE)
			{
				headTmp = 0;
			}
		}
		
		if (!bMatch)
		{	
			/* Compare the tail node */
			if ((l_wavFIFOBuf[headTmp].fileTimeName == IndexInfo.fileTimeName))
			{
				bMatch = TRUE;
			}
		}
		/* Specified WAV node exist */
		if (bMatch)
		{
			/* If WAV node position is greater than or equal to FIFO head */
			if (headTmp >= l_fifoBufHead )
			{
				
				while(headTmp > l_fifoBufHead)
				{
					l_wavFIFOBuf[headTmp].fileTimeName = l_wavFIFOBuf[headTmp - 1].fileTimeName;						
					headTmp--;
				}
				l_fifoBufHead++;
			}
			/* If WAV node position is less than or equal to FIFO tail */
			else if (headTmp <= l_fifoBufTail)
			{
				while(headTmp < l_fifoBufTail)
				{
					l_wavFIFOBuf[headTmp].fileTimeName = l_wavFIFOBuf[headTmp + 1].fileTimeName;						
					headTmp++;
				}
				l_fifoBufTail--;					
				
			}
			
						
			if (l_fifoBufTail == WAV_FIFO_SIZE)
			{
				l_fifoBufTail = 0;
			}
			if (l_fifoBufHead == WAV_FIFO_SIZE)
			{
				l_fifoBufHead = 0;
			}
			
			if (l_fifoBufTail == l_fifoBufHead)
			{
				l_fifoBufState = WAV_FIFO_EMPTY;
			}
		}

	}

}


/* 
 * fn		void usbvm_wavNameFormat( USBVM_REC_FILE_NODE recNode, char *pwavStr )
 * brief	Transform wav file name format to xxx_xxx_xxxx-xx-xx xx.xx'xx''.wav
 * details	
 *
 * param[in]	recNode  record node information
 * param[out]	*pwavStr  the output wav file name format string after transformation
 *
 * return	
 * retval	
 *
 * note		
 */
void usbvm_wavNameFormat( USBVM_REC_FILE_NODE recNode, char *pwavStr )
{
	struct tm* ptm = NULL;

	/*<< BosaZhong@12Oct2013, modify, we use localtime according with web time. */
	#if 1
	ptm = gmtime((time_t *)&(recNode.fileTimName));
	#else
	ptm = localtime((time_t *)&(recNode.fileTimName));
	#endif /* 0 */
	/*>> endof BosaZhong@12Oct2013, modify, we use localtime according with web time. */
	
	/* Source and destination display name */
	memset(pwavStr, 0, 100);		
	/*  [Huang Lei] [29Nov12] "\'" should be used here. Because it is a pathname we'll use.
		Otherwise some system call such as "access()", will fail.*/
	sprintf(pwavStr, "%s_%s_%04d-%02d-%02d %02d.%02d\'%02d\'\'.wav",
		recNode.srcNum, recNode.dstNum,
		1900 + ptm->tm_year, 1 + ptm->tm_mon, ptm->tm_mday,
 		ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
}



/* 
 * fn		void usbvm_wavNameFormat2(unsigned int fileTimName, char *srcNum, char *dstNum, char *pwavStr )
 * brief	Transform wav file name format to xxx_xxx_xxxx-xx-xx xx.xx'xx''.wav
 * details	
 *
 * param[in]	recNode  record node information
 * param[out]	*pwavStr  the output wav file name format string after transformation
 *
 * return	
 * retval	
 *
 * note		
 */
void usbvm_wavNameFormat2(unsigned int fileTimName, char *srcNum, char *dstNum, char *pwavStr )
{
	struct tm* ptm = NULL;

	ptm = gmtime((time_t *)&(fileTimName));
	
	memset(pwavStr, 0, 100);		
	sprintf(pwavStr, "%s_%s_%04d-%02d-%02d %02d.%02d\'%02d\'\'.wav",
		srcNum, dstNum,
		1900 + ptm->tm_year, 1 + ptm->tm_mon, ptm->tm_mday,
 		ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

	return;
}

/* 
 * fn		void usbvm_wavPathFormat( USBVM_REC_FILE_NODE recNode, char *pwavPath)
 * brief	Transform wav file name format to xxx_xxx_xxxx-xx-xx xx.xx'xx''.wav
 * details	
 *
 * param[in]	recNode  record node information
 * param[out]	*pwavStr  the output wav file name format string after transformation
 *
 * return	
 * retval	
 *
 * note		
 */
void usbvm_wavPathFormat( USBVM_REC_FILE_NODE recNode, char *pwavPath)
{
	char wavName[100] = {0};

	usbvm_wavNameFormat(recNode, wavName);
	sprintf(pwavPath, "%s%s", USB_REC_FILE_DIR, wavName);

	return;
}

/* 
 * fn		BOOL usbvm_checkUSBDevExist( void )
 * brief	Check whether USB device exist 
 * details	
 *
 * param[in]	
 * param[out]	
 *
 * return	USB exist return TRUE, else return FALSE
 * retval	
 *
 * note		
 */
BOOL usbvm_checkUSBDevExist( void )
{
	BOOL ret = FALSE;

	if (access(USBDISK_DIR, F_OK) == 0)
	{
		 ret = TRUE;		 
	}
	return ret;
}

/* 
 * fn		int usbvm_openIndexFpAndLock(int endpt,FILE **pFpIndex,char* openMode,USBVM_LOCK_MODE lockMode)
 * brief	Open the fp of index file, and lock it with USBVM_LOCK_MODE.If we can not get lock now, 
 *			wait for WAIT_SEC seconds to get it until timeout.
 * details	
 *
 * param[in]	int endpt: endpoint number;
 *				char* openMode: same as fopen(),such as "r","w";
 *				USBVM_LOCK_MODE lockMode: 
 * param[out]	FILE **pFpIndex: pointer to fp, that will be got;
 *
 * return	int
 * retval	failed: -1, success: not -1,the same as fcntl()
 *
 * note		
 */
int usbvm_openIndexFpAndLock(int endpt,FILE **pFpIndex,char* openMode,USBVM_LOCK_MODE lockMode)
{
	int indexFd = -1;
	int lockRet = -1;

#ifdef DEBUG_DETAILED
	MARK_PRINT("HL_DBG:LOCK:  lockMode(%d), [%s:%d]\n",lockMode,__FUNCTION__,__LINE__);
#endif
	lockRet = pthread_mutex_lock((pthread_mutex_t *)&(l_mutexIndexFileOperation));
	if(lockRet!=0)
	{
		MARK_PRINT("HL_DBG:lock mutex FAILED: l_mutexIndexFileOperation[endpt(%d)]! [%s:%d]\n",
			endpt,__FUNCTION__,__LINE__);
	}
	*pFpIndex = usbvm_getRecIndexFileFp(endpt, openMode);
	if(*pFpIndex == NULL)
	{
		MARK_PRINT("HL_DBG: can't open indexfile! [%s:%d]\n",__FUNCTION__,__LINE__);
		lockRet = pthread_mutex_unlock((pthread_mutex_t *)&(l_mutexIndexFileOperation));
		USBVM_ASSERT(0==lockRet);
		return -1;
	}
	indexFd = fileno(*pFpIndex);
	fcntl(indexFd, F_SETFD, FD_CLOEXEC);
	struct sigaction act;

	act.sa_handler = sigAlrm;
    sigemptyset(&act.sa_mask);
	act.sa_flags = SA_INTERRUPT;
	if(0 > sigaction(SIGALRM, &act, NULL))
	{
		fclose(*pFpIndex);
		*pFpIndex = NULL;
		MARK_PRINT("lock file error, ret:SIG_ERR, errno:%d, errno==EINTR:%d\n", errno, errno==EINTR);
		lockRet = pthread_mutex_unlock((pthread_mutex_t *)&(l_mutexIndexFileOperation));
		USBVM_ASSERT(0==lockRet);
		return -1;
	}

	alarm(WAIT_SEC);
	do
	{
		switch(lockMode)
		{
			case WR_LOCK_WHOLE: 
				TT_PRINT(" WR_LOCK_WHOLE");
				lockRet = usbvm_writeWaitLock(indexFd, 0, SEEK_SET, 0);
				break;
			case RD_LOCK_HEADER:
				/* Lock byte 0-99 */
				TT_PRINT(" RD_LOCK_HEADER");
				lockRet = usbvm_readWaitLock(indexFd, 0, SEEK_SET, 100);
				break;
			case RD_LOCK_BODY:
				/* Lock byte 100-EOF */
				TT_PRINT(" RD_LOCK_BODY");
				lockRet = usbvm_readWaitLock(indexFd, 100, SEEK_SET, 0);
				break;
			case RD_LOCK_WHOLE:
				TT_PRINT(" RD_LOCK_WHOLE");
				lockRet = usbvm_readWaitLock(indexFd, 0, SEEK_SET, 0);
				break;
		}
	} while (-1 == lockRet && (EINTR == errno || EAGAIN == errno));
	if(-1 == lockRet)
	{
		fclose(*pFpIndex);
		*pFpIndex = NULL;
		int temp = -1;
		MARK_PRINT("lock file error %s\n", strerror(errno));
		temp = pthread_mutex_unlock((pthread_mutex_t *)&(l_mutexIndexFileOperation));
		USBVM_ASSERT(0==temp);
	}
	alarm(0);

	TT_PRINT("%s return %d, indexFd(%d)", __FUNCTION__, lockRet, indexFd);
	
	return lockRet;
}
/* 
 * fn		int usbvm_closeIndexFpAndUnlock(int endpt,FILE* fpIndex, USBVM_LOCK_MODE lockMode)
 * brief	Close the fp of index file, and unlock it with USBVM_LOCK_MODE.This func is only used to 
 *			release the fp and lock that opened by usbvm_openIndexFpAndLock().
 * details	
 *
 * param[in]	int endpt: endpt number;
 *				FILE* fpIndex: the fp that is got by usbvm_openIndexFpAndLock();
 *				USBVM_LOCK_MODE lockMode: MUST be the same as used by usbvm_openIndexFpAndLock();
 * param[out]	
 *
 * return	int
 * retval	failed: -1, success: not -1,the same as fcntl()
 *
 * note		
 */
int usbvm_closeIndexFpAndUnlock(int endpt,FILE* fpIndex, USBVM_LOCK_MODE lockMode)
{
	int indexFd = -1;
	int lockRet = -1;

	USBVM_ASSERT(fpIndex!=NULL);
	fflush(fpIndex);
	indexFd = fileno(fpIndex);
#ifdef DEBUG_DETAILED
	MARK_PRINT("HL_DBG: UNLOCK: lockMode(%d), [%s:%d]\n",lockMode,__FUNCTION__,__LINE__);
#endif
	do
	{
		switch(lockMode)
		{
			case WR_LOCK_WHOLE: 
			case RD_LOCK_WHOLE:
				lockRet = usbvm_unLock(indexFd, 0, SEEK_SET, 0);
				break;
			case RD_LOCK_HEADER:
				/* Lock byte 0-99 */
				lockRet = usbvm_unLock(indexFd, 0, SEEK_SET, 100);
				break;
			case RD_LOCK_BODY:
				/* Lock byte 100-EOF */
				lockRet = usbvm_unLock(indexFd, 100, SEEK_SET, 0);
				break;
		}
	} while (-1 == lockRet && (EINTR == errno || EAGAIN == errno));
	fclose(fpIndex);
	/* Should reset fpIndex to default */
	pthread_mutex_unlock((pthread_mutex_t *)&l_mutexIndexFileOperation);
	return lockRet;
}

/*  [Huang Lei] [28Nov12] when check the available space of usbvm, we do not scan the disk,
	and only read a file which saves the size.Everytime we record or delete files, we add or 
	subtract the size. When the usb disk is mounted, culculate the size */
/* 
 * fn		void usbvm_calculateUsedSize() 
 * brief	calculate the size of usbvm dir, save to file
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
void usbvm_calculateUsedSize()
{
	UINT64 sizeInallAvail = 0;
	int fileSize = 0;
	UINT64 sizeUsed = 0;
	int i = 0;
	char filepathname[128];
	DIR *dirp; 
	struct dirent *direntp; 
	struct statfs s;
	char cmd[256] = {0};
	
	if (access(USBDISK_DIR, F_OK) == 0)
	{		
		{	
			/*
			 * GET INDEX FILE SIZE	 
			 */
			snprintf(filepathname,sizeof(filepathname),"%s",USB_INDEX_FILE_DIR);
			if (access(filepathname, F_OK) == 0)
			{
				if ((dirp=opendir(filepathname)) == NULL) 
				{ 
/* 					MARK_DEBUGP("USB Voice MailBox--Open Directory %s Error\n", filepathname);  */
				}
				else
				{
					while ((direntp = readdir(dirp)) != NULL)
					{				
						snprintf(filepathname,sizeof(filepathname),"%s%s",
							USB_INDEX_FILE_DIR,direntp->d_name);
						if ((fileSize = getFileSize(filepathname)) > 0)
						{
							sizeUsed += fileSize;
						}
					}
					closedir(dirp); 
				}
			}


			/*
			 * GET RECORD FILE SIZE 
			 */		 
			snprintf(filepathname,sizeof(filepathname), "%s", USB_REC_FILE_DIR);
			if (access(filepathname,F_OK) == 0)
			{
				if ((dirp=opendir(filepathname)) == NULL) 
				{ 
					/* MARK_DEBUGP("USB Voice MailBox--Open Directory %s Error\n", filepathname);  */
				}
				else
				{
					while ((direntp = readdir(dirp)) != NULL)
					{
					
						snprintf(filepathname,sizeof(filepathname),"%s%s",
							USB_REC_FILE_DIR,direntp->d_name);
						if ((fileSize = getFileSize(filepathname)) > 0)
						{
							sizeUsed += fileSize;
						}
					}
					closedir(dirp); 
				}
			}
		}

		/*
		 * GET NOTIFY FILE SIZE 
		 */
		memset(filepathname,0,sizeof(filepathname));
		strncpy(filepathname, USB_NOT_FILE_CUSTOM_DIR,strlen(USB_NOT_FILE_CUSTOM_DIR));		
		if (access(filepathname, F_OK) == 0)
		{
			if ((dirp = opendir(filepathname)) == NULL) 
			{ 
/* 				MARK_DEBUGP("USB Voice MailBox--Open Directory %s Error\n", filepathname);  */
			}
			else
			{
				while ((direntp = readdir(dirp)) != NULL)
				{
					snprintf(filepathname, sizeof(filepathname),"%s%s",
						USB_NOT_FILE_CUSTOM_DIR,direntp->d_name);
					if ((fileSize = getFileSize(filepathname)) > 0)
					{
						sizeUsed += fileSize;
					}
				}
				closedir(dirp); 
			}
		}
		/* get size inall free */
		if (statfs(USBDISK_DIR, &s) == 0)
		{
			sizeInallAvail = s.f_bfree*s.f_bsize;
		}
		usbvm_writeUsedSize(sizeUsed,sizeInallAvail);
	}
}
/* 
 * fn		int usbvm_writeUsedSize(UINT32 size) 
 * brief	write the used size of usbvm to a file
 * details	
 *
 * param[in]	UINT32 size, size to be saved
 *				UINT32 sizeLeft, the size that is left in disk
 * param[out]	
 *
 * return	int
 * retval	0, success
 *			-1, fail
 * note 	
 */
int usbvm_writeUsedSize(UINT64 size, UINT64 sizeLeft)
{
	FILE* fp=NULL;
	char str[USBVM_USEDSIZE_STRLEN]={0};
	int fd = -1;
	fp = fopen(USBVM_USEDSIZE_FILE,"w+");
	if(fp != NULL)
	{
		fd = fileno(fp);
		fcntl(fd, F_SETFD, FD_CLOEXEC);
		snprintf(str,sizeof(str),"%llu %llu",size,sizeLeft);
		lockf(fd, F_LOCK,USBVM_USEDSIZE_STRLEN);
/*20130618_update*/
		fwrite(str,sizeof(str),1,fp);			/* digital string so we don't care byte order */
		lockf(fd, F_ULOCK,USBVM_USEDSIZE_STRLEN);
		fclose(fp);
		MARK_PRINT("FILE_USE: wirte size(%llu),sizeLeft(%llu)\n",size,sizeLeft);
	}
	else
	{
		MARK_PRINT("FILE_USE: open %s failed for write\n",USBVM_USEDSIZE_FILE);
		return -1;
	}
	return 0;
}
/* 
 * fn		int usbvm_readUsedSize(UINT32* size) 
 * brief	read the used size of usbvm from a file
 * details	
 *
 * param[in]	UINT32* size, pointer to size to be read
 * param[out]	UINT32* sizeLeft,pointer to the size that is left in disk
 *
 * return	int 
 * retval	0, success
 *			-1, fail
 * note		
 */
int usbvm_readUsedSize(UINT64* p_size, UINT64* p_sizeLeft)
{
	FILE* fp = NULL;
	char str[USBVM_USEDSIZE_STRLEN]={0};
	int fd = -1;
	fp = fopen(USBVM_USEDSIZE_FILE,"r");
	
	if(fp != NULL)
	{
		fd = fileno(fp);
		fcntl(fileno(fp), F_SETFD, FD_CLOEXEC);
		lockf(fd, F_LOCK,USBVM_USEDSIZE_STRLEN);
		fread(str,sizeof(str),1,fp);
		lockf(fd, F_ULOCK,USBVM_USEDSIZE_STRLEN);
		fclose(fp);
		sscanf(str,"%llu %llu",p_size,p_sizeLeft);
		MARK_PRINT("FILE_USE: read size(%llu),sizeLeft(%llu)\n",*p_size,*p_sizeLeft);
	}
	else
	{
		MARK_PRINT("FILE_USE: open %s failed for read\n",USBVM_USEDSIZE_FILE);
		return -1;
	}
	return 0;
}
/* 
 * fn		int usbvm_rewriteUsedSize(UINT32 delta,BOOL plus) 
 * brief	read size from file, add/subtract with delta, and then rewrite the result to file.
 * details	
 *
 * param[in]	UINT64 delta, the delta value to be calculated.
 *				BOOL plus, TRUE: add, FLASE: substruct
 * param[out]	
 *
 * return	int
 * retval	0, success
 *			-1, fail
 * note		
 */
int usbvm_rewriteUsedSize(UINT64 delta,BOOL plus)
{
	UINT64 size = 0;
	UINT64 sizeLeft = 0;
	UINT64 result = 0;
	int ret = 0;
	if(delta==0)
	{
		return -1;
	}
	MARK_PRINT("FILE_USE: delta(%llu),plus(%d) \n",delta,plus);
	ret = usbvm_readUsedSize(&size,&sizeLeft);
	if(ret!=0)
	{
		return ret;
	}
	if(plus == TRUE)
	{
		result = size + delta;
		if(result < size)
		{	/* the uint32 overflows.keep the value */
			MARK_PRINT("FILE_USE: size(%llu) is too big to add delta(%llu)\n",size,delta);
			return -1;
		}
		else
		{
			size = result;
		}
		result = sizeLeft - delta;
		if(result > sizeLeft)
		{
			/* the sizeLeft overflows ,but size is right, set sizeLeft to 0*/
			MARK_PRINT("FILE_USE: sizeLeft(%llu) is too small to subtract delta(%llu)\n",
				sizeLeft,delta);
			sizeLeft = 0;
		}
		else
		{
			sizeLeft = result;
		}
	}
	else
	{
		result = size - delta;
		if(result > size)
		{	/* the size overflows,keep the value */
			MARK_PRINT("FILE_USE: size(%llu) is too small to subtract delta(%llu)\n",size,delta);
			return -1;
		}
		else
		{
			size = result;
		}
		result = sizeLeft + delta;
		if(result < sizeLeft)
		{	/* the sizeLeft overflows ,but size is right, keep sizeLeft.*/
			MARK_PRINT("FILE_USE: sizeLeft(%llu) is too big to add delta(%llu)\n",sizeLeft,delta);
		}
		else
		{
			sizeLeft = result;
		}
	}
	ret =usbvm_writeUsedSize(size, sizeLeft);
	if(ret != 0)
	{
		return ret;
	}
	return 0;
}

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */


