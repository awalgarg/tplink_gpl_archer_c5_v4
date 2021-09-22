
/**************************************************************************************
* File Name  : tpLinuxTag.h
*
* Description: add tag with validation system to the firmware image file to be uploaded
*              via http
*
* Created    : 16Sep07,	Liang Qiming
**************************************************************************************/

#ifndef _TP_LINUX_TAG_H_
#define _TP_LINUX_TAG_H_


#define TAG_LEN         512
#define MAGIC_LEN		24
#define CHIP_ID_LEN		8
#define BOARD_ID_LEN    16
#define TOKEN_LEN       20
#define SIG_LEN		    128

typedef struct
{	
	unsigned long tagVersion;			/* tag version number */   
	char 		  signiture[MAGIC_LEN];	/* magic number */
	char 		  chipId[CHIP_ID_LEN];		/* chip id */  
	char 		  boardId[BOARD_ID_LEN];	/* board id */  
	unsigned long productId;	/* product id */  
	unsigned long productVer;	/* product version */
	unsigned long addHver;		/* Addtional hardware version */
	
	unsigned char imageValidToken[TOKEN_LEN];	/* image validation token - md5 checksum */
	unsigned char rcSingature[TOKEN_LEN];	 	/* RC singature(only for vxWorks) - RSA */
	
	unsigned long kernelTextAddr; 	/* text section address of kernel */
	unsigned long kernelEntryPoint; /* entry point address of kernel */
	
	unsigned long totalImageLen;	/* the sum of kernelLen+rootfsLen+tagLen */
	
	unsigned long kernelAddress;	/* starting address (offset from the beginning of FILE_TAG) 
									 * of kernel image 
									 */
	unsigned long kernelLen;		/* length of kernel image */
	
	unsigned long rootfsAddress;	/* starting address (offset) of filesystem image */
	unsigned long rootfsLen;		/* length of filesystem image */

	unsigned long bootAddress;		/* starting address (offset) of bootloader image */
	unsigned long bootLen;			/* length of bootloader image */

	/*dbs: added */
	unsigned long swRevision;		/* software revision */
	unsigned long platformVer;		/* platform version */
	/*dbs: add end */

	unsigned long binCrc32;			/* CRC32 for bin(kernel+rootfs) */

	unsigned long reserved[14];		/* reserved for future */

	unsigned char sig[SIG_LEN];		/* signature for update */
	unsigned char resSig[SIG_LEN];	/* reserved for signature */
}LINUX_FILE_TAG;


#define BOOTLOADER_SIZE         0x20000
#define FILE_TAG_SIZE			512

#define LINUX_KERNEL_TAG_OFFSET		(BOOTLOADER_SIZE)



#endif // _TP_LINUX_TAG_H_

