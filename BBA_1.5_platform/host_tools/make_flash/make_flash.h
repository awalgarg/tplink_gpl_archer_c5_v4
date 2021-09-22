/*! Copyright(c) 2014 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     make_flash.h
 *\brief    Tool for generating firmware
 *
 *\author   Huang Qinglou
 *\version  1.0.0
 *\date     18/Aug/2014
 *
 *\history \arg 
 *\01   18/Aug/2014  HQL     Create.
 *\02	05/Feb/2015	 Liwei   add CloudInfo.
 */



#ifndef _MAKE_FLASH_H_
#define _MAKE_FLASH_H_



#define VERSION			"mtk A2300 v1.0"
#define EXTRA_PARA_NAME	"extra-para" 
#define OS_IMAGE_NAME	"os-image"
#define PRODUCT_INFO_NAME	"product-info"
#define HISTORY_NAME	"history"

#define NM_PTN_TABLE_SIZE	(ENTRY_MAX_NUM * PTN_ENTRY_LEN) /* 0x500 */
#define ENTRY_MAX_NUM		(32)
#define ENTRY_FILE_PATH_LEN	(512)
#define ENTRY_NAME_LEN 		(32)
#define UP_PTNTBL_SIZE      (0x800)
#define PTN_STRLEN			(512)
#define NUMBER_SIZE			(4)

#define BUF_LEN_2M			(2 * 1024 * 1024)
#define BUF_LEN_4M			(4 * 1024 * 1024)
#define BUF_LEN_8M			(8 * 1024 * 1024)
#define BUF_LEN_16M			(16 * 1024 * 1024)
#define BUF_LEN_32M			(32 * 1024 * 1024)
#define BUF_LEN_64M			(64 * 1024 * 1024)
#define BUF_LEN_128M		(128 * 1024 * 1024)


#define NM_BYTE_LEN             4
#define FILE_EMPTY				"#"
#define NM_FILE_NOOUTPUT        "*"

#define TRUE	(1)
#define FALSE	(0)

#define CLOUD_HEAD_OFFSET 	252
#define HW_ID_LEN			16
#define FW_ID_LEN			16
#define OEM_ID_LEN			16
#define STR_ID_LEN			32
#define HW_LIST_MAX		128
#define INFO_LEN			20

#define CLOUD			"Cloud"
#define CLOUD_HWID		"hw_id"
#define CLOUD_FWID		"fw_id"
#define CLOUD_OEMID		"oem_id"
#define CLOUD_HWLIST	"hw_list"
#define FWTYPE			"fw-type"
#define SOFTVERSION		"soft_ver"

#define HEADER_VERSION 	0x00000100
#define MAGIC_LEN 		20
#define CRC_LEN 			16
#define RSA_SIGN_LEN 		128
#define FWID_FL_MASK_LEN 	12
#define FW_DESC_LEN	 	12

typedef unsigned int uint32;

typedef enum flash_em_size{
	FLASH_EM_2M  = 2,
	FLASH_EM_4M  = 4,
	FLASH_EM_8M  = 8,
	FLASH_EM_16M = 16,
	FLASH_EM_32M = 32,
	FLASH_EM_64M = 64
}FLASH_EM_SIZE;


typedef enum partition_entry_type
{
	NM_PTNCONTENT_OUTPUT = 0,
	NM_PTNCONTENT_PTNTBL,
	NM_PTNCONTENT_PDINFO,
	NM_PTNCONTENT_SOFTVER,
	NM_PTNCONTENT_PROFILE,
	NM_PTNCONTENT_SIGNATURE,
	NM_PTNCONTENT_EXTRAPARA,
	NM_PTNCONTENT_CLOUDINFO,
	NM_PTNCONTENT_END
}entry_type;

typedef enum nm_type
{
	NM_TYPE_BOTH = 0,
	NM_TYPE_FLASH,
	NM_TYPE_UP,
	NM_TYPE_NOT,
	NM_TYPE_FLASH_AND_MUP,  /*for manufactory: user-config in flash and special up file*/
	NM_TYPE_END
}NM_TYPE;

typedef enum nm_offset
{
	NM_OFFSET_NO = 0,
	NM_OFFSET_4B,
	NM_OFFSET_8B,
	NM_OFFSET_12B,
	NM_OFFSET_END
}NM_OFFSET;

typedef struct _soft_ver
{
	unsigned long int sn;
	unsigned long int date;
	unsigned long int release;
}soft_ver;

typedef struct extra_para_struct
{
	unsigned char dbootFlag;	
	unsigned char integerFlag;	
}EXTRA_PARA_STRUCT;

typedef struct _partition_entry
{
	char 				name[ENTRY_NAME_LEN];
	uint32 				size;
	uint32 				base;
	entry_type 			type;
	char 				file[ENTRY_FILE_PATH_LEN];/*input file path*/
	uint32 				flag;
} partition_entry;

typedef struct _mtd_partition {
	char	name[ENTRY_NAME_LEN];
	uint32 	size;
	uint32 	offset;
}mtd_partition;


typedef struct _partition_table
{
	partition_entry 	entries[ENTRY_MAX_NUM];
	char 				ptn_file[ENTRY_FILE_PATH_LEN];
	char 				fw_prefix[PTN_STRLEN];
	char 				product_info[PTN_STRLEN];
	soft_ver			ver;
	int 				ptn_num;
	int 				flash_size;
} partition_table;

#define PTN_ENTRY_LEN	sizeof(mtd_partition)

/* fwup-file header */
#define FWUP_HDR_MD5_LEN 		16
#define CLOUD_ID_BYTE_LEN		16

#define FWUP_HDR_RESERVED		(512 - 28 - FWUP_HDR_MD5_LEN - 2 * CLOUD_ID_BYTE_LEN)
//#define FWUP_HDR_PRODUCT_ID_LEN 16

#if 0//修改升级软件头部结构，支持签名机制
typedef struct fwup_file_head
{
	uint32 fileSize;
	char fileMd5[FWUP_HDR_MD5_LEN];
	uint32 up_ptn_entry_num;
	uint32 flash_ptn_entry_num;
	uint32 product_id;
	uint32 product_ver;
	uint32 add_hver;	
	uint32 sw_revision;
	unsigned char hw_id[CLOUD_ID_BYTE_LEN];
	unsigned char oem_id[CLOUD_ID_BYTE_LEN];
	char reserved[FWUP_HDR_RESERVED];

} FWUP_FILE_HEAD;
#endif
#define TAG_VERSION		0x03000003 

/* 
 * brief Total image tag length	
 */
#define TAG_LEN         512

/* 
 * brief Token length	
 */
#define TOKEN_LEN       20
/* 
 * brief magic number length	
 */
#define MAGIC_NUM_LEN	20
/* 
 * brief signature length	
 */
#define SIG_LEN		128
unsigned char magicNum[MAGIC_NUM_LEN] = {0x55, 0xAA, 0x55, 0xAA, 0xF1, 0xE2, 0xD3, 0xC4, 0xE5, 0xA6, 0x6A, 0x5E, 0x4C, 0x3D, 0x2E, 0x1F, 0xAA, 0x55, 0xAA, 0x55};

typedef struct fwup_file_head
{
	unsigned long tagVersion;			/* tag version number */  
	unsigned char hw_id[CLOUD_ID_BYTE_LEN];		/* HWID for cloud */
	unsigned char oem_id[CLOUD_ID_BYTE_LEN];		/* FWID for cloud */
	unsigned char oemId[CLOUD_ID_BYTE_LEN];			/* OEMID for cloud */
	unsigned long product_id;	/* product id */  
	unsigned long product_ver;	/* product version */
	unsigned long add_hver;		/* Addtional hardware version */
	
	unsigned char fileMd5[TOKEN_LEN];	/* image validation token - md5 checksum */
	unsigned char magicNum[MAGIC_NUM_LEN];	 	/* magic number */
	
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

	unsigned long sw_revision;		/* software revision */
	unsigned long platformVer;		/* platform version */
	unsigned long specialVer;

	unsigned long binCrc32;			/* CRC32 for bin(kernel+rootfs) */

	unsigned long fileSize;

	unsigned long up_ptn_entry_num;
	
	unsigned long flash_ptn_entry_num;

	unsigned long reserved1[10];		/* reserved for future */

	unsigned char sig[SIG_LEN];		/* signature for update */
	unsigned char resSig[SIG_LEN];	/* reserved for signature */

	unsigned long reserved2[12];	/* reserved for future */


} FWUP_FILE_HEAD;


#if INCLUDE_CLOUD
typedef struct fw_cloud_info
{
	char filePath[NM_PTN_FILE_PATH];
	//char hwId[HW_ID_LEN];
	char fwId[FW_ID_LEN];
	//char oemId[OEM_ID_LEN];
	int	hwListNum;
	char hwList[HW_LIST_MAX][HW_ID_LEN];
}FW_CLOUD_INFO;
#endif


typedef struct _UPGRADE_HEADER
{
	unsigned int headerVersion;
	unsigned char magicNumber[MAGIC_LEN];
	unsigned short tagLength;
	unsigned short vendorId;
	unsigned short zoneCode;
	unsigned short contentTypes;
	unsigned char rsaSignature[RSA_SIGN_LEN];
	unsigned short hwIdNum;
	unsigned short fwIdFLNum;
	unsigned char fwIdFLMask[FWID_FL_MASK_LEN];
	unsigned char hwIdList[0][HW_ID_LEN];
//	unsigned char fwIdFL[0][FW_ID_LEN];
} UPGRADE_HEADER;

typedef struct _TP_HEADER
{
	unsigned int headerVersion;
	unsigned char magicNumber[MAGIC_LEN];

	unsigned int kernelLoadAddress;
	unsigned int kernelEntryPoint;

	unsigned short vendorId;
	unsigned short zoneCode;

	unsigned int partitionNum;
	unsigned int factoryBootOffset;
	unsigned int factoryBootLen;
	unsigned int factoryInfoOffset;
	unsigned int factoryInfoLen;
	unsigned int radioOffset;
	unsigned int radioLen;
	unsigned int ucOffset;
	unsigned int ucLen;
	unsigned int bootloaderOffset;
	unsigned int bootloaderLen;
	unsigned int tpHeaderOffset;
	unsigned int tpHeaderLen;
	unsigned int kernelOffset;
	unsigned int kernelLen;
	unsigned int romFsOffset;
	unsigned int romFsLen;
	unsigned int jffs2FsOffset;
	unsigned int jffs2FsLen;

	unsigned char factoryInfoCRC[CRC_LEN];
	unsigned char radioCRC[CRC_LEN];
	unsigned char ubootCRC[CRC_LEN];
	unsigned char kernelAndRomfsCRC[CRC_LEN];
	
	unsigned char fwId[FW_ID_LEN];
	unsigned char fwDescription[FW_DESC_LEN];
	unsigned int fwIdBLNum;
	unsigned char fwIdBL[0][FW_ID_LEN];
} TP_HEADER;

#define CLOUD_ID_STR_LEN    33
#define BUFFER_LEN64	64




typedef struct _DEV_INFO
{
	unsigned int sw_revision;
	unsigned int platform_ver;
	unsigned int special_ver;
	unsigned int product_id;
	unsigned int product_ver;
	unsigned int add_hver;
	char model_name[BUFFER_LEN64];
	char build_date[BUFFER_LEN64];
	char build_time[BUFFER_LEN64];
	char dev_ver[BUFFER_LEN64];
	unsigned char hw_id[CLOUD_ID_BYTE_LEN];
	unsigned char fw_id[CLOUD_ID_BYTE_LEN];
	unsigned char oem_id[CLOUD_ID_BYTE_LEN];
}DEV_INFO;

#endif

