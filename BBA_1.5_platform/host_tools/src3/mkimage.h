/*
 * Copyright (C) 2004  Manuel Novoa III  <mjn3@codepoet.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */


typedef struct _ROMFILE_STRUCT
{
	unsigned long signOffset;	/* flash UID signature */
	unsigned long macOffset;	/* MAC address */
	unsigned long pinOffset;	/* PIN code for wireless */
	unsigned long rfpiOffset;	/* RFPI code for dect */
	unsigned long deviceIdOffset;	/* Device ID for cloud */
}ROMFILE_STRUCT;

typedef struct _LINUX_FLASH_STRUCT
{
	unsigned long appSize;			/* kernel+rootfs size */
	unsigned long bootOffset;		/* boot loader	*/	
	unsigned long kernelOffset;		/* kernel */
	unsigned long rootfsOffset;		/* rootfsOffset is calculate in main() */
	unsigned long configOffset;		/* config */
}LINUX_FLASH_STRUCT;

#define MAC_MISC_OFFSET 0xF100
#define PIN_MISC_OFFSET 0xF200

/* 
 * brief Total image tag length	
 */
#define TAG_LEN         512

/* 
 * brief cloud ID length	
 */
#define CLOUD_ID_BYTE_LEN	16

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


#define BOOTLOADER_TAG_LEN TAG_LEN


/* 
 * brief Image tag struct,have different position in Linux and vxWorks(see TAG_OFFSET)	
 */
typedef struct _LINUX_FILE_TAG
{
	unsigned long tagVersion;			/* tag version number */
	
	unsigned char hardwareId[CLOUD_ID_BYTE_LEN];		/* HWID for cloud */
	unsigned char firmwareId[CLOUD_ID_BYTE_LEN];		/* FWID for cloud */
	unsigned char oemId[CLOUD_ID_BYTE_LEN];			/* OEMID for cloud */

	unsigned long productId;	/* product id */  
	unsigned long productVer;	/* product version */
	unsigned long addHver;		/* Addtional hardware version */
	
	unsigned char imageValidToken[TOKEN_LEN];	/* image validation token - md5 checksum */
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

	unsigned long swRevision;		/* software revision */
	unsigned long platformVer;		/* platform version */
	unsigned long specialVer;

	unsigned long binCrc32;			/* CRC32 for bin(kernel+rootfs) */

	unsigned long reserved1[13];	/* reserved for future */

	unsigned char sig[SIG_LEN];		/* signature for update */
	unsigned char resSig[SIG_LEN];	/* reserved for signature */

	unsigned long reserved2[12];	/* reserved for future */
}LINUX_FILE_TAG;


#define ROUND 1
#define MAX_FILENAME_LEN 256 
/* three files will input-bootloader, kernel, rootfs */
#define FILE_NUM 4

enum W8930G_FILE
{
	BOOT,
	KERNEL,
	FS,
	CONFIG
};

#define TAG_VERSION		0x03000003 
#define VERSION_INFO	"ver. 3.0"

unsigned char md5Key[16] = 
{	/* linux - wr841n */
	0xDC, 0xD7, 0x3A, 0xA5, 0xC3, 0x95, 0x98, 0xFB, 
	0xDC, 0xF9, 0xE7, 0xF4, 0x0E, 0xAE, 0x47, 0x37
};

unsigned char mk5Key_bootloader[16] =
{	/* linux bootloader - u-boot/redboot */
	0x8C, 0xEF, 0x33, 0x5F, 0xD5, 0xC5, 0xCE, 0xFA,
	0xAC, 0x9C, 0x28, 0xDA, 0xB2, 0xE9, 0x0F, 0x42
};


unsigned char def_mac[6] = {0x00, 0x0a, 0xeb, 0x13, 0x09, 0x69};

unsigned char magicNum[MAGIC_NUM_LEN] = {0x55, 0xAA, 0x55, 0xAA, 0xF1, 0xE2, 0xD3, 0xC4, 0xE5, 0xA6, 0x6A, 0x5E, 0x4C, 0x3D, 0x2E, 0x1F, 0xAA, 0x55, 0xAA, 0x55};

typedef struct _USRCONF_STRUCT
{
	unsigned long	length;			/* length in byte */
	unsigned long 	signature;		/* magic number */
	unsigned long	checksum;		/* checksum */
	unsigned long	isCompressed;	/* indicate config data is compressed */
	unsigned long	softwareRevision;	/* softwareRevision */
	unsigned char	version;
	unsigned char	cloudUserName[65];	/* cloud username */
	unsigned char	res[170];			/* reserved */
}USRCONF_STRUCT;

#define CONF_FLASH_SIGNATURE			0x98765432

