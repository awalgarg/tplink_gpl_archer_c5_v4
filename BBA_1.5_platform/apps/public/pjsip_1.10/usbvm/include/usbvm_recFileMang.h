/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		usbvm_recFileMang.h
 * brief	Management of Record and Notify files in USB device	
 * details	
 *
 * author	Sirrain Zhang
 * version	
 * date		07Jun11
 *
 * history 	\arg	
 */

#ifndef __USBVM_RECFILEMANG_H__
#define __USBVM_RECFILEMANG_H__

#include "usbvm_types.h"
#include <usbvm_glbdef.h>
#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

#include <stdio.h>

/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/


#define MAX_DIGITS_NUM 32               /* max length of telehone number */
#define RECLIST_INDEX_AREA_SIZE 100     /* record index node start position */
#define RECNOTIF_DURATION_LIMIT 20      /* local record duration limit */
#define WAV_FIFO_SIZE 96                /* WAV FIFO buffer size */

#define USBVM_FILE_MNG_IDLE 0			/* is not operating file */
#define USBVM_FILE_MNG_BUSY 1			/* is writing or deleting file */

#define USBDISK_DIR "/var/usbdisk/usbvm"                           /* USB mount directory */
#define USB_VOICEMAIL_DIR USBDISK_DIR"/voicemail"                    /* voicemail directory */

#define USB_INDEX_FILE_DIR USB_VOICEMAIL_DIR"/index/"            /* record index file directory */
#define USB_REC_FILE_DIR USB_VOICEMAIL_DIR"/record/"             /* record file directory */
#define USB_WAV_FILE_DIR USB_VOICEMAIL_DIR"/wav/"                     /* wav file directory */
#define USB_NOT_FILE_CUSTOM_DIR USB_VOICEMAIL_DIR"/voiceNotify/" /* custom voice notify directory */
#define USB_NOT_FILE_DEF_DIR "/etc/voiceNotify/"         /* default voice notify directory */

/* Now we only support PCMU in local playing */

#define USB_NOT_NO_REC_FILE "noRecordNotify_g711u"
#define USB_NOT_NO_NEW_REC_FILE "noNewRecordNotify_g711u"
/* 
 * brief	the file path to save the size of usbvm files.
 */
#define USBVM_USEDSIZE_FILE	"/var/tmp/usbvm_usedSize"
#define USBVM_USEDSIZE_STRLEN 50
/* 
 * brief	If less than USBVM_MIN_MB_LEFT MB space is left, do not enter usbvm 
 */
#define USBVM_MIN_MB_LEFT	3	


#define WAIT_SEC 5  

/* get read lock */
#define usbvm_readLock(fd, offset, whence, len)\
	usbvm_lockHandle(fd, F_SETLK, F_RDLCK, (offset), (whence), (len))
/* get write lock */
#define usbvm_writeLock(fd, offset, whence, len)\
	usbvm_lockHandle(fd, F_SETLK, F_WRLCK, (offset), (whence), (len))	

/* get read lock and wait for timeout */
#define usbvm_readWaitLock(fd, offset, whence, len)\
	usbvm_lockHandle(fd, F_SETLKW, F_RDLCK, (offset), (whence), (len))
/* get write lock and wait for timeout */
#define usbvm_writeWaitLock(fd, offset, whence, len)\
	usbvm_lockHandle(fd, F_SETLKW, F_WRLCK, (offset), (whence), (len))
/* unlock, in pairs with lock  */
#define usbvm_unLock(fd, offset, whence, len)\
	usbvm_lockHandle(fd, F_SETLK, F_UNLCK, (offset), (whence), (len))
/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/
#define WAV_HEADER_LEN (16 + sizeof(long) + sizeof(WAVEAUDIOFORMAT) + 8)

/* 
 * brief	Record node
 */
typedef struct RecNode
{
	unsigned int fileTimName;     /* record file name */
	unsigned int fileSize;        /* record file size */
	unsigned int fileDuration;    /* record file duration */
	char srcNum[MAX_DIGITS_NUM];  /* record source telephone number */
	char dstNum[MAX_DIGITS_NUM];  /* record destination telephone number */
	unsigned int payloadType;     /* rtp payload type */
	unsigned int readFlag;        /* read flag */
	unsigned int checkSum;        /* record node chek sum */
	

}USBVM_REC_FILE_NODE, *USBVM_REC_FILE_LIST;

/*20130618_update*/
#define  USBVM_REC_NODE_NUMBER_START	(3)
#define  USBVM_REC_NODE_NUMBER_END		(USBVM_REC_NODE_NUMBER_START - 1 + ((2 * MAX_DIGITS_NUM) / 4))


/* 
 * brief	WAV FIFO buffer state
 */
typedef enum
{
	WAV_FIFO_EMPTY,    /* FIFO is empty */
	WAV_FIFO_DATA,     /* FIFO is processing */
	WAV_FIFO_FULL      /* FIFO is full */
}WAV_FIFO_STATE;

/* 
 * brief	File type  
 */
typedef enum
{
	RECORD_TYPE,       /* record file */
	WAV_TYPE           /* wav file */
}FILE_TYPE;

/* 
 * brief	Index file locker's mode
 */
typedef enum 
{
	WR_LOCK_WHOLE = 0,	/* When write,lock whole file */
	RD_LOCK_HEADER = 1,	/* When read,lock header */
	RD_LOCK_BODY = 2,	/* When read,lock body */
	RD_LOCK_WHOLE = 3,	/* When read,lock whole file */
}USBVM_LOCK_MODE;
/* 
 * brief	WAV FIFO node
 */
typedef struct WavIndexInfo
{
	int endpt;        /* FXS endpoint number */
	unsigned int fileTimeName;      /* node position in record index file */
}USBVM_WAV_FIFO_INFO;

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/
/**************************************************************************************************/
/*                                           FUNCTIONS                                            */
/**************************************************************************************************/
/* 
 *	fn			int getFileSize(const char * filePathName)
 *
 *	brief		get file size
 *
 *	param[in]	filePathName		--	file path name
 *
 *	return		int
 */
int getFileSize(const char * filePathName);

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
FILE *usbvm_getRecIndexFileFp( int endpt,char* mode );

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
void usbvm_setRecCurNode( int endpt, int recCurNode );

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
int usbvm_getRecCurNode( int endpt );

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
void usbvm_setRecNumInAll( int endpt, int recNumInAll );

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
 * note		
 */
int usbvm_getRecNumInAll( int endpt,FILE* fpIndex );

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
void usbvm_setRecNotReadStartPos( int endpt, int recNotReadStartPos );

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
int usbvm_getRecNotReadStartPos( int endpt,FILE* fpIndex);

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
void usbvm_setRecNotReadNum( int endpt, int recNotReadNum );

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
int usbvm_getRecNotReadNum( int endpt,FILE* fpIndex );
/* 
 * fn		int usbvm_getStaticsFromIndexFile( int endpt , int *recInAll, int *notReadInAll, int *notReadStart ) 
 * brief	Get record statics from index file for other process, only used by httpd.
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
int usbvm_getStaticsFromIndexFile( int endpt , int *pRecInAll, int *pNotReadInAll, int *pNotReadStart );
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
 
int usbvm_getStaticVarsFromIndexFile( int endpt , int *pRecInAll,
											int *pNotReadInAll, int *pNotReadStart, FILE* fpIndex );

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
void usbvm_calChkSum( USBVM_REC_FILE_LIST precNode );

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
int usbvm_chkSum( USBVM_REC_FILE_LIST precNode );

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
void usbvm_createRecIndexList( void );

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
void usbvm_destoryRecIndexList( void );

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
void usbvm_recFileMangInit( void );

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
void usbvm_recFileMangDeInit( void );

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
 * note		
 */
int usbvm_insertRecIndexList( int endpt, USBVM_REC_FILE_LIST precNode,FILE* fpIndex );

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
 * note		
 */
int usbvm_addRecIndexList( int endpt, USBVM_REC_FILE_LIST precNode,FILE* fpIndex );

#if 0
/* 
 * fn		void usbvm_delRecIndexListAll( void )
 * brief	Delete all the record nodes and their relevant record files
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
void usbvm_delRecIndexListAll( void );
#endif

#if 0
/* 
 * fn		int usbvm_delRecIndexListByName( int endpt, unsigned int fileTimName )
 * brief	Delete the record node by name and its relevant record file
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
int usbvm_delRecIndexListByName( int endpt, unsigned int fileTimName );
#endif

int usbvm_delRecIndexListByName2_Action(int endpt, unsigned int fileTimeName, int notify);

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
int usbvm_delRecIndexListByName2( int endpt, unsigned int fileTimName );

/* 
 * fn		int usbvm_delRecIndexListByPos( int endpt, int nodePos )
 * brief	Delete the record node by position and its relevant record file
 * details	
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
int usbvm_delRecIndexListByPos( int endpt, int nodePos ,FILE* fpIndex);

/* 
 * fn		int usbvm_delRecIndexListOnly( int endpt, int nodePos )
 * brief	Delete the record index node only by position
 * details	
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
int usbvm_delRecIndexListOnly( int endpt, int nodePos, FILE* fpIndex);

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
int usbvm_delRecIndexListBackward( int endpt, int nodePos,FILE* fpIndex );

/* 
 * fn		CMEP_STATE usbvm_delAllRecIndexList( int endpt)
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
int usbvm_delAllRecIndexList(int endpt);

/* 
 * fn		int usbvm_delPcmRecByName( int endpt, unsigned int fileTimName)
 * brief	Delete pcm record by name, implement after record has transfer to wav format.
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
int usbvm_delPcmRecByName(unsigned int fileTimName);

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
 * note		
 */
int usbvm_schRecIndexListLatestExpired( int endpt, unsigned int expiredDays ,FILE* fpIndex);

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
 * note		
 */
int usbvm_schNotReadNodeFromCurPos( int endpt, int nodePos, FILE* fpIndex );

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
int usbvm_schNotReadNodeFromCurPos2( int endpt, int nodePos, FILE *fp );

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
int usbvm_schNotReadNodeFromHead( int endpt, FILE *fp );

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
int usbvm_updRecIndexListInfoAfterR( int endpt, USBVM_REC_FILE_LIST precNode,FILE* fpIndex );

/* 
 * fn		int usbvm_getRecIndexListNodeInfo( int endpt, int nodePos, USBVM_REC_FILE_LIST precNode )
 * brief	Get the specified record file info
 * details	
 *
 * param[in]	endpt  FXS endpoit number 
 * param[in]	nodePos  specified node position to get info
 * param[out]	precNode  pointer of record index node
 *
 * return	success return 0, else return < 0
 * retval	
 *
 * note		
 */
int usbvm_getRecIndexListNodeInfo( int endpt, int nodePos, USBVM_REC_FILE_LIST precNode );

/* 
 * fn		int usbvm_getRecIndexListNodeInfoAndFd( int endpt, int nodePos, USBVM_REC_FILE_LIST precNode, FILE **fppRecord )
 * brief	Get the specified record file info
 * details	
 *
 * param[in]	endpt  FXS endpoit number 
 * param[in]	nodePos  specified node position to get info
 * param[in]	**fppRecord  pointer of record index file
 * param[out]	precNode  pointer of record index node
 *
 * return	success return 0, else return < 0
 * retval	
 *
 * note		
 */
int usbvm_getRecIndexListNodeInfoAndFd( int endpt, int nodePos,
									USBVM_REC_FILE_LIST precNode, FILE **fppRecord, FILE* fpIndex);

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
unsigned int usbvm_getFileSize( const char *pfilePathName );

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
void usbvm_wavFIFOPutIndexInfo( USBVM_WAV_FIFO_INFO indexInfo );

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
int usbvm_wavFIFOGetIndexInfo( USBVM_WAV_FIFO_INFO *pIndexInfo );

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
void usbvm_wavFIFODelIndexInfo( USBVM_WAV_FIFO_INFO IndexInfo );

/* 
 * fn		void usbvm_wavNameFormat( USBVM_REC_FILE_NODE recNode, char *pwavStr )
 * brief	Transform wav file name format to xxxx_xxxx_xxxx-xx-xx xx.xx'xx''.wav
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
void usbvm_wavNameFormat( USBVM_REC_FILE_NODE recNode, char *pwavStr );

/* 
 * fn		void usbvm_wavNameFormat2(unsigned int fileTimName, char *srcNum, char *dstNum, char *pwavStr )
 * brief	Transform wav file name format to xxx_xxx_xxxx-xx-xx xx.xx'xx''.wav
 * details	
 *
 * param[in]	fileTimName, srcNum, dstNum
 * param[out]	*pwavStr  the output wav file name format string after transformation
 *
 * return	
 * retval	
 *
 * note		
 */
void usbvm_wavNameFormat2(unsigned int fileTimName, char *srcNum, char *dstNum, char *pwavStr );

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
void usbvm_wavPathFormat( USBVM_REC_FILE_NODE recNode, char *pwavPath);

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
void usbvm_escapeCharHandle( char *pstr );

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
BOOL usbvm_checkUSBDevExist( void );
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
int usbvm_lockHandle(int fd,int cmd,int type, off_t offset,int whence, off_t len);
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
int usbvm_openIndexFpAndLock(int endpt,FILE **pFpIndex,char* openMode,USBVM_LOCK_MODE lockMode);
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
int usbvm_closeIndexFpAndUnlock(int endpt,FILE* fpIndex, USBVM_LOCK_MODE lockMode);


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
void usbvm_calculateUsedSize();

/* 
 * fn		int usbvm_writeUsedSize(UINT32 size) 
 * brief	write the used size of usbvm to a file
 * details	
 *
 * param[in]	UINT64 size, size to be saved
 *				UINT64 sizeLeft, the size that is left in disk
 * param[out]	
 *
 * return	int
 * retval	0, success
 *			-1, fail
 * note 	
 */
int usbvm_writeUsedSize(UINT64 size, UINT64 sizeLeft);
/* 
 * fn		int usbvm_readUsedSize(UINT32* size) 
 * brief	read the used size of usbvm from a file
 * details	
 *
 * param[in]	UINT64* size, pointer to size to be read
 * param[out]	UINT64* sizeLeft,pointer to the size that is left in disk
 *
 * return	int 
 * retval	0, success
 *			-1, fail
 * note		
 */
int usbvm_readUsedSize(UINT64* size, UINT64* sizeLeft);
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
int usbvm_rewriteUsedSize(UINT64 delta,BOOL plus);
/*20130618_update*/

/*
 * brief		read a integer of net side byte order and transfer it to host side byte order.
 *
 * note		BosaZhong@25Apr2013, add, for compatible.
 */
size_t fread_HostIntFromNetByteOrder(void *ptr, size_t size, size_t nmemb, FILE *stream);

/*
 * brief		transfer a integer of host side byte order to net side byte order, then write
 *			it to a file.
 *
 * note		BosaZhong@25Apr2013, add, for compatible.
 */
size_t fwrite_HostIntToNetByteOrder(const void *ptr, size_t size, size_t nmemb, FILE *stream);


/*
 * brief		read USBVM_REC_FILE_NODE from a file.
 *
 * note		BosaZhong@25Apr2013, add, for compatible.
 */
size_t fread_USBVM_REC_FILE_NODE(void *ptr, size_t size, size_t nmemb, FILE *stream);


/*
 * brief		write USBVM_REC_FILE_NODE from a file.
 *
 * note		BosaZhong@25Apr2013, add, for compatible.
 */
size_t fwrite_USBVM_REC_FILE_NODE(const void *ptr, size_t size, size_t nmemb, FILE *stream);


#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif	/* __USBVM_RECFILEMANG_H__ */

