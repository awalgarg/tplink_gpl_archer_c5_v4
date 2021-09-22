/*! Copyright(c) 2014 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 *\file     make_flash.c
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

#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <elf.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "make_flash.h"
#include "md5_interface.h"


static unsigned char md5Key[16] = 
{
	0x7a, 0x2b, 0x15, 0xed,  0x9b, 0x98, 0x59, 0x6d,
	0xe5, 0x04, 0xab, 0x44,  0xac, 0x2a, 0x9f, 0x4e
};

unsigned char tpHeaderMagicNumber[MAGIC_LEN] = 
{
	0xAA, 0x55, 0x9D, 0xD1, 0xA8, 0xC8, 0x83, 0x31, 0xC9, 0x69,
	0xFB, 0xBF, 0xBC, 0xF0, 0xD4, 0x32, 0x70, 0xC7, 0x55, 0xAA
 };

unsigned char upgradeHeaderMagicNumber[MAGIC_LEN] = 
{
	0xAA, 0x55, 0x4C, 0x5E, 0x83, 0x1F, 0x53, 0x4B, 0xA1, 0xF8,
	0xF7, 0xC9, 0x18, 0xDF, 0x8F, 0xBF, 0x7D, 0xA1, 0x55, 0xAA
};


partition_table *ptable;
partition_table ptn_table;

partition_table *g_upproptr;
partition_table upProStruct;

static DEV_INFO l_dev_info;

soft_ver ver;
int sw_ver1, sw_ver2, sw_ver3, sw_ver4;
char file_name[PTN_STRLEN] = {0};
char fw_ver[PTN_STRLEN] = {0};

char fw_id[STR_ID_LEN + 1] = {0};
char hw_id[STR_ID_LEN + 1] = {0};
char oem_id[STR_ID_LEN + 1] = {0};

int debug = FALSE;

int bootmode = 0; //0:single bootloader mode(default), 1:double bootloader mode
int prestore = 0; //0:flash and up file; 1:flash and special up with user-config; and another normal up file;

#define DEBUG(fmt, args...) \
	do{\
		if(debug)\
			printf(fmt, ##args);\
	}while(0)

/* hex to str, note : strBuf must be cleared to 0!!!! */
int hex2str(unsigned char *hexBuf, int hexLen, unsigned char* strBuf)
{
	int ret = -1;
	int i = 0;

	if (hexBuf == NULL || strBuf == NULL)
	{
		goto out;
	}

	for (i = 0; i < hexLen; i++)
	{
		sprintf(strBuf, "%s%02X", strBuf, hexBuf[i]);
	}

	ret = 0;
out:
	return ret;
}

int str2hex(unsigned char *hexBuf, unsigned char* strBuf)
{
	int ret = -1;
	int i = 0;
	unsigned char* strPtr;
	unsigned char strArr[3] = {'\0'};
	
	if (hexBuf == NULL || strBuf == NULL)
	{
		goto out;
	}

	strPtr = strBuf;
	while (*strPtr != '\0')
	{
		strArr[0] = *strPtr;
		strArr[1] = *(++strPtr);
		hexBuf[i] = (unsigned char)strtoul(strArr, NULL, 16);
		i++;
		strPtr++;
	}

	ret = 0;
out:
	return ret;
}

int strUpper(unsigned char strBuf[])
{
	int strLen = strlen(strBuf);
	int i = 0;

	for(i = 0; i < strLen; i++)
	{
		if(strBuf[i] >= 'a' && strBuf[i] <= 'z')
		{
			strBuf[i] -= 32;
		}
	}
	return 0;
}

int createFwID(char *id)
{
	if (0 == strlen(file_name))
	{
		printf("createFwID: file_name is empty.\n");
		return -1;
	}

	md5_make_digest(id, file_name, strlen(file_name));

	hex2str(id, 16, fw_id);
	fw_id[STR_ID_LEN] = '\0';
	
	DEBUG("id is %s\n", fw_id);

	return 0;
}

int createHwID(char *id)
{
	int i = 0;
	FILE *FP = NULL;
	int real_size = 0;
	char *data = NULL;

	for (i = 0; i < ptable->ptn_num; i++)
	{
		if (0 == strcmp(ptable->entries[i].name, PRODUCT_INFO_NAME))
		{
			FP = fopen(ptable->entries[i].file, "rb");
			if (NULL == FP)
			{
				printf("open file(%s) error.\n",ptable->entries[i].file);
				return -1;
			}

			fseek(FP, 0, SEEK_END);
			real_size = ftell(FP);

			data = (char *)malloc(real_size + 1);
			if (NULL == data)
			{
				printf("createHwID: malloc error.\n");
				fclose(FP);
				return -1;
			}
            fseek(FP, 0, SEEK_SET);
			fread(data, 1, real_size, FP);

			md5_make_digest(id, data, real_size);
			
			hex2str(id, 16, hw_id);
			hw_id[STR_ID_LEN] = '\0';
			
			DEBUG("hw_id is %s\n", hw_id);

			free(data);
			fclose(FP);

			break;
		}
	}
	if (i == ptable->ptn_num)
	{
		printf("createFwID: no product-info partition. Error!\n");
		return -1;
	}
	return 0;
}

int parse_fw_ver(unsigned char *softverStr,unsigned char *relverStr,unsigned char *dateStr)
{
	int assigns;
	time_t now;
	struct tm* timenow = NULL;
	char strDate[12];
	char *chgTmpPtr;
	int relver=0;
	int year=0, month=0, day=0;
	
	assigns = sscanf(softverStr, "V%d.%d.%dP%d", &sw_ver1, &sw_ver2, &sw_ver3, &sw_ver4);
	if (4 != assigns)
	{
		printf("-s, --softver     Software version in format \"V1.0.0P1\"\n");
		return -1;
	}

	sw_ver4 = sw_ver4?sw_ver4:1;

	ver.sn = (0xff << 24 | (sw_ver1 & 0xff) << 16 | (sw_ver2 & 0xff) << 8 | (sw_ver3 & 0xff));
	ver.sn = htonl(ver.sn);
	
	time(&now);
	timenow = localtime(&now);

	ver.release = 3600 * timenow->tm_hour + 60 * timenow->tm_min + timenow->tm_sec;
	
	if (1 == sscanf(relverStr, "%d", &relver) && relver > 0 && relver < 86400) {
		ver.release = relver;
	}

	if (3 == sscanf(dateStr, "%d.%d.%d", &year, &month, &day) 
	  && year > 1900 && year < 2099 && month > 0 && month < 13 && day > 0 && day < 32) {
		timenow->tm_year = year - 1900;
		timenow->tm_mon = month - 1;
		timenow->tm_mday = day;
	}
	
	sprintf(strDate,"0x%04d%02d%02d",timenow->tm_year + 1900, timenow->tm_mon + 1, timenow->tm_mday);
	
	ver.date = strtoul(strDate,&chgTmpPtr,0);

	
	snprintf(file_name, PTN_STRLEN, "ver%d-%d-%d-P%d[%s-rel%s]",
			sw_ver1, sw_ver2, sw_ver3, sw_ver4, l_dev_info.build_date, l_dev_info.build_time);
	snprintf(fw_ver, PTN_STRLEN, "%d.%d.%d Build %8x rel.%s",
			sw_ver1, sw_ver2, sw_ver3, ver.date, l_dev_info.build_time);

	ver.date = htonl(ver.date);
	ver.release = htonl(ver.release);

	DEBUG("soft version string: %s\n", file_name);
	return 0;
}


static void jump_space(char *pStr, int size)
{
	int i = 0, num = 0;
	char tmp[PTN_STRLEN] = {0};
	char *pCur;

	if (NULL == pStr)
	{
		return;
	}

	for (num = 0, pCur = pStr; num < size && '\0' != *pCur; num ++, pCur ++)
	{
		if ((' ' != *pCur) && ('\t' != *pCur) && ('\r' != *pCur) && ('\n' != *pCur))
		{
			tmp[i++] = *pCur;
		}
	}
	tmp[i++] = '\0';

	memcpy(pStr, tmp, i);

	return;
}


int parse_ptn_table(partition_table *ptable)
{
	int ret = 0;
	int i = 0, noused = 0;
	int num_read = 0;
	int len = 0;
	char* line = 0;	
	FILE   *pfile = 0;
	int assigns;
	struct timespec curTime;
	struct tm* tm;
	

	if (NULL == ptable || 0 == ptable->ptn_file[0])
	{
		printf("-p, --partition=FILE      Partition file\n");
		return -1;
	}

	pfile = fopen(ptable->ptn_file, "r");
	
	if (!pfile)
	{
		printf("failed to open partition file.\n");
		return -1;
	}
	
	if ((num_read = getline(&line, &len, pfile)) != -1)
	{
		jump_space(line, num_read);
		assigns = sscanf(line, "total=%d,flash=%dM", &ptable->ptn_num, &ptable->flash_size);

		ptable->flash_size = ptable->flash_size * 1024 * 1024;

		if (0 == ptable->ptn_num || ENTRY_MAX_NUM < ptable->ptn_num)
		{
			printf("total=%d out of range\n", ptable->ptn_num);

			ret = -1;
			goto LEAVE;
		}
	}
	DEBUG("ptable->ptn_num is %d\n", ptable->ptn_num);

	for (i = 0; i < ptable->ptn_num; i++)
	{
		if((num_read = getline(&line, &len, pfile)) == -1)
			break;
		
		jump_space(line, num_read);
		DEBUG("%s\n", line);
		
		assigns = sscanf(line, "%d=%[^,],%x,%x,%d,%d,%s", 
			&noused, ptable->entries[i].name, 
			&ptable->entries[i].base, &ptable->entries[i].size, 
			&ptable->entries[i].type, &ptable->entries[i].flag, 
			ptable->entries[i].file);

		DEBUG("%d\n", assigns);

		if (7 != assigns || noused != i)
		{
			printf("partition table: i=%d error\n", i);

			ret = -1;
			goto LEAVE;
		}
	}
	DEBUG("i is %d\n", i);

LEAVE:
	if (line != NULL)  
		free(line);
    	
	fclose(pfile);

	return ret;
}

static void print_table(partition_table *ptable)
{
	int i = 0;
	
	DEBUG("================ partition table =========================\n");
	DEBUG("total=%d, flash=0x%08x\n", ptable->ptn_num, ptable->flash_size);

	for (i = 0; i < ptable->ptn_num; i ++)
	{
		DEBUG("%d= %16s, 0x%08x, 0x%08x, %d, %d, %s\n",
			i, ptable->entries[i].name, ptable->entries[i].base,
			ptable->entries[i].size, ptable->entries[i].type,
			ptable->entries[i].flag, ptable->entries[i].file);
	}

	DEBUG("================ partition table end ======================\n");
	
	return;
}



int check_table(partition_table *ptable)
{
	int i;
	FILE *FP;
	int real_size;
	unsigned long last_base = 0;
	unsigned long last_size = 0;
	for (i = 0; i < ptable->ptn_num; i ++)
	{
		if (ptable->entries[i].base < (last_base + last_size))
		{
			printf("patition table: i=%d error.\n", i);
			return -1;
		}
		else
		{
			last_base = ptable->entries[i].base;
			last_size = ptable->entries[i].size;
		}

		if (NULL == ptable->entries[i].file)
		{
			printf("patition table(i=%d): file is null.\n", i);
			return -1;
		}

		if (0 != strcmp(ptable->entries[i].file, FILE_EMPTY))
		{
			FP = fopen(ptable->entries[i].file, "rb");
			if (NULL == FP)
			{
				printf("open file(%s) error.\n",ptable->entries[i].file);
				return -1;
			}

			fseek(FP,0,SEEK_END);
			real_size = ftell(FP);
			if (real_size > last_size)
		    {
				printf("file(%s) is too large. file:0x%x, table:0x%x\n",ptable->entries[i].file, real_size, last_size);
		        return -1;
		    }
			
			fclose(FP);
		}
	}

	if ((last_base + last_size) > ptable->flash_size)
	{
		printf("patition table: i=%d error.\n", i);
		return -1;
	}
	
	return 0;
};


static int getFileContentToBuf_bytes(char* buf, unsigned int BUFLEN, unsigned int *real_size, char* fileName)
{
    int retVal = 0;
	FILE *FP;
	*real_size = 0;
		
    if (fileName == NULL)
    {
		retVal = -1;
		return retVal;
    }	
	
	if((FP = fopen(fileName,"rb")) == NULL)
	{
		printf("open file(%s) error.\n",fileName);
		return -1;
	}
	fseek(FP,0,SEEK_END);
	*real_size = ftell(FP);

    if (*real_size > BUFLEN)
    {
		printf("file(%s) is too large.file: 0x%x, buf:0x%x\n",fileName, *real_size, BUFLEN);
        retVal = -1;
		goto ERROR_LEAVE;
    } 
	
	fseek(FP,0,SEEK_SET);
	fread(buf,1,*real_size,FP);
ERROR_LEAVE:
	fclose(FP);	
	return retVal;
}


static int makePtnTableFileByStruct(char *buf, int *len)
{
	int i;
	char ptnTable[NM_PTN_TABLE_SIZE];
	char tempCmd[256] = {0};
	char* ptnStart = ptnTable;

	memset(ptnTable,0xFF,NM_PTN_TABLE_SIZE);

	for (i = 0; i < g_upproptr->ptn_num; i ++)
	{
		printf("================%d	%s\n", i, g_upproptr->entries[i].name);
		memcpy(ptnStart, g_upproptr->entries[i].name, PTN_ENTRY_LEN);
		ptnStart = ptnStart + PTN_ENTRY_LEN;
		*len += PTN_ENTRY_LEN;
	}

	printf("len is %d\n", *len);
	memcpy(buf, ptnTable, *len);
	
	return 0;
}

static int makeUpSetExtraPara(char *buf, unsigned int buflen, int *real_size)
{
	EXTRA_PARA_STRUCT extraPara;
	EXTRA_PARA_STRUCT *p;
	
	*real_size = sizeof(EXTRA_PARA_STRUCT);
	memset(&extraPara, 0, *real_size);

	//set the flash image boot mode
	if (bootmode == 1)
	{
		extraPara.dbootFlag = 1;
	}
	else
	{
		extraPara.dbootFlag = 0;
	}

	//set the flash image is not integer when up file
	extraPara.integerFlag = 0;

	if (*real_size > buflen)
	{
		DEBUG("make up extra para partition length is not enough.\n");
		return -1;
	}

	memcpy(buf, (char *)&extraPara, *real_size);
	p = (EXTRA_PARA_STRUCT *)buf;
	printf("dbootflag %d integerflag %d\n", p->dbootFlag, p->integerFlag);
	return 0;
}

static int makeUpCompletePtnTableFileByStruct(char *buf, int *len)
{
	int i;
	char ptnTable[NM_PTN_TABLE_SIZE] = {0};
	char tempCmd[1024] = {0};
	char* ptnStart = ptnTable;

	*len = 0;

	for (i = 0; i < g_upproptr->ptn_num; i ++)
	{
		memcpy(ptnStart, g_upproptr->entries[i].name, PTN_ENTRY_LEN);
		ptnStart = ptnStart + PTN_ENTRY_LEN;
		*len += PTN_ENTRY_LEN;
	}
	
	memcpy(buf, ptnTable,*len);/*生成索引文件*/

	return 0;
}

static int makeUpPtnTableFileByStruct(char *buf, int *len)
{
	int i;
	char ptnTable[NM_PTN_TABLE_SIZE] = {0};
	char tempCmd[1024] = {0};
	char* ptnStart = ptnTable;

	*len = 0;

	for (i = 0; i < g_upproptr->ptn_num; i ++)
	{
		if (g_upproptr->entries[i].size != 0
		&& (g_upproptr->entries[i].flag == NM_TYPE_BOTH
		||  g_upproptr->entries[i].flag == NM_TYPE_UP))	
		{
			printf("%d	%s\n", i, g_upproptr->entries[i].name);
			memcpy(ptnStart, g_upproptr->entries[i].name, PTN_ENTRY_LEN);
			ptnStart = ptnStart + PTN_ENTRY_LEN;
			*len += PTN_ENTRY_LEN;
		}
	}

	printf("len is %d\n", *len);
	
	memcpy(buf, ptnTable,*len);/*生成索引文件*/

	return 0;
}

static int makeMUpPtnTableFileByStruct(char *buf, int *len)
{
	int i;
	char ptnTable[NM_PTN_TABLE_SIZE] = {0};
	char tempCmd[256] = {0};
	char* ptnStart = ptnTable;

	*len = 0;

	for (i = 0; i < g_upproptr->ptn_num; i ++)
	{
		if (g_upproptr->entries[i].size != 0
		&& (g_upproptr->entries[i].flag == NM_TYPE_BOTH
		||  g_upproptr->entries[i].flag == NM_TYPE_UP
		||  g_upproptr->entries[i].flag == NM_TYPE_FLASH_AND_MUP))	
		{
			sprintf(tempCmd,
				"fwup-ptn %s base 0x%05x size 0x%05x\t\r\n",
				g_upproptr->entries[i].name, 
				g_upproptr->entries[i].base,
				g_upproptr->entries[i].size);
			memcpy(ptnStart,tempCmd,strlen(tempCmd));
			ptnStart = ptnStart + (strlen(tempCmd));
			*len += strlen(tempCmd);
		}
	}
	*(ptnStart+1) = '\0';
	memcpy(buf,ptnTable,*len+1);/*生成索引文件*/

	return 0;
}


static int makeUpContent(int i,char**fileAddr,int *filesize,char* baseAddr)
{
	char *fileContentPtr;
	char *fileHeadPtr;
	char *fileEndPtr;
	char *setBase;

	char *strBuf = NULL;

	unsigned int ptnSize;
	unsigned int real_size;
	unsigned int bigEnLen;

	partition_entry *pEntry = (partition_entry*)(&g_upproptr->entries[i]);
	DEBUG("%d= %16s, 0x%08x, 0x%08x, %d, %d, %s\n",
			i, pEntry->name, pEntry->base,
			pEntry->size, pEntry->type,
			pEntry->flag, pEntry->file);

	switch(pEntry->type)
	{
	case NM_PTNCONTENT_PROFILE:
	case NM_PTNCONTENT_OUTPUT:
	case NM_PTNCONTENT_SIGNATURE:
	case NM_PTNCONTENT_PDINFO:
		ptnSize = pEntry->size;
		fileHeadPtr = *fileAddr;
		fileContentPtr = *fileAddr;

		 if(0 != strcmp(pEntry->file, FILE_EMPTY))
		 {
			if(getFileContentToBuf_bytes(fileContentPtr,ptnSize,&real_size,pEntry->file) == -1)
			{
				printf("failed to read file\n");
				pEntry->base = *fileAddr - baseAddr;
				pEntry->size = 0;
				return -1;
			}
			else
			{
				pEntry->base = *fileAddr - baseAddr;
				pEntry->size = real_size;
				fileEndPtr = fileContentPtr + real_size;
				*fileAddr += pEntry->size;
				//memset(fileEndPtr,0,1);/*分区结束set 00*/
			}
		 }
		 else
		 {
			pEntry->base = *fileAddr - baseAddr;
			pEntry->size = 0;
			fileEndPtr = fileHeadPtr;
		 }
		 *filesize += pEntry->size;
		break;
		
	case NM_PTNCONTENT_PTNTBL:
		real_size = 0;
		ptnSize = pEntry->size;
		fileHeadPtr = baseAddr + UP_PTNTBL_SIZE;
		fileContentPtr = fileHeadPtr;
		makePtnTableFileByStruct(fileContentPtr, &real_size);
		pEntry->base = UP_PTNTBL_SIZE;
		pEntry->size = UP_PTNTBL_SIZE;
		break;
		
	case NM_PTNCONTENT_SOFTVER:
	 	ptnSize = pEntry->size;
		fileHeadPtr = *fileAddr;
		fileContentPtr = *fileAddr;

		snprintf(fileContentPtr, ptnSize, "%s:%s\n", SOFTVERSION, fw_ver);
		real_size = strlen(fileContentPtr) + 1;
		
		pEntry->base = (*fileAddr - baseAddr);/*返回实际写入的文件大小*/
		pEntry->size = real_size + 1;
		*fileAddr += real_size + 1;

		fileEndPtr = fileContentPtr + real_size;
		memset(fileEndPtr,0,1);/*分区结束set 00*/
		*filesize = (*fileAddr - baseAddr);
		break;
	case NM_PTNCONTENT_CLOUDINFO:
		
		break;
	case NM_PTNCONTENT_EXTRAPARA:
		ptnSize = pEntry->size;
		fileHeadPtr = *fileAddr;
		fileContentPtr = *fileAddr;
		
		if (makeUpSetExtraPara(fileContentPtr, ptnSize, (int *)&real_size) < 0)
		{
			return -1;
		}
		
		pEntry->base = (*fileAddr - baseAddr);
		pEntry->size = real_size + 1;
		*fileAddr += real_size + 1;
		fileEndPtr = fileContentPtr + real_size;
		memset(fileEndPtr,0,1);/*pattition end with 00*/
		*filesize = (*fileAddr - baseAddr);		
		break;
	default:
		break;
	}	
	return 1;	
}


int package_up_fw()
{
	char *basicAddr;
	char *tmpFileBuf;
	char *fileAddr;
	unsigned char md5_digest[FWUP_HDR_MD5_LEN] = {'\0'};
	FWUP_FILE_HEAD fileHead = {0};
	memset(&fileHead, 0xFF, sizeof(FWUP_FILE_HEAD));

	int upFileSize = 0;
	int i;
	int len = 0;

	int out_fd = 0;
	char up_filename[PTN_STRLEN] = {0};

	g_upproptr = (partition_table *)(&upProStruct);
	memset(g_upproptr,0,sizeof(partition_table));
	memcpy(g_upproptr,ptable,sizeof(partition_table));

	basicAddr = (char *)malloc(g_upproptr->flash_size + sizeof(FWUP_FILE_HEAD));
	if (NULL == basicAddr)
	{
		printf("malloc error\n");
		return -1;
	}
	memset(basicAddr, 0xFF, g_upproptr->flash_size + sizeof(FWUP_FILE_HEAD));
	
	tmpFileBuf = basicAddr + sizeof(FWUP_FILE_HEAD);
	
	fileAddr = tmpFileBuf+ 2 * UP_PTNTBL_SIZE;/*写入升级软件自索引*/
	upFileSize = 2 * UP_PTNTBL_SIZE;

	DEBUG("makeupcontent start\n");

	/* Construct target partition. */
	{
		len = 0;
		char buf[NM_PTN_TABLE_SIZE];
		makeUpCompletePtnTableFileByStruct(buf, &len);
		memcpy((void *)(tmpFileBuf + NM_PTN_TABLE_SIZE),(void *)buf,len);
		fileHead.flash_ptn_entry_num = g_upproptr->ptn_num;
	}

	for(i = 0; i < g_upproptr->ptn_num; i ++)
	{
		if (g_upproptr->entries[i].flag == NM_TYPE_BOTH 
		 || g_upproptr->entries[i].flag == NM_TYPE_UP)
		{
			if (-1 == makeUpContent(i,&fileAddr,&upFileSize,tmpFileBuf)/*MAX-3M-size*/)
			{
				printf("makeupcontent error\n");
				free(basicAddr);
				return -1;
			}
		}	
	}
	
	DEBUG("makeupcontent done\n");

	/* Construct up partition. */
	{
		len = 0;
		char buf[NM_PTN_TABLE_SIZE];
		memset(buf,0xFF,NM_PTN_TABLE_SIZE);
		makeUpPtnTableFileByStruct(buf,&len);
		memcpy((void *)tmpFileBuf,(void *)buf,len);
		fileHead.up_ptn_entry_num = (len/PTN_ENTRY_LEN);
	}
	
	upFileSize += sizeof(FWUP_FILE_HEAD);
	
	DEBUG("up file len: 0x%x\n", upFileSize);
	
	/*==文件长度==*/
	fileHead.fileSize = upFileSize;
	
	memcpy(&fileHead.product_id, &l_dev_info.product_id, 3*sizeof(unsigned int));
	memcpy(fileHead.hw_id, l_dev_info.hw_id, CLOUD_ID_BYTE_LEN);
	memcpy(fileHead.oem_id, l_dev_info.oem_id, CLOUD_ID_BYTE_LEN);
	fileHead.sw_revision = l_dev_info.sw_revision;

/* Add by Zhao Mengqing, 20180607 */
	fileHead.tagVersion = TAG_VERSION;
	memcpy(fileHead.magicNum, magicNum, MAGIC_NUM_LEN);

	memset(fileHead.fileMd5, 0, TOKEN_LEN);

	fileHead.bootAddress = 0x0;
	fileHead.bootLen = 0x0;

	fileHead.kernelAddress = TAG_LEN;
	fileHead.kernelLen = 0;
	
	memset(fileHead.sig, 0, SIG_LEN);
	memset(fileHead.resSig, 0, SIG_LEN);
/* End add */
	memcpy(basicAddr, &fileHead, sizeof(FWUP_FILE_HEAD));
	
	/*==MD5头部==*/
	/* By Zhao Mengqing, 20180607, do md5 checksum caculate when signature firmware*/
#if 0 
	memcpy(basicAddr+4, md5Key, FWUP_HDR_MD5_LEN);
	md5_make_digest(md5_digest, (unsigned char *)(basicAddr+4), (upFileSize-4));
    
	memcpy(fileHead.fileMd5, md5_digest, FWUP_HDR_MD5_LEN);	
	memcpy(basicAddr, &fileHead, sizeof(FWUP_FILE_HEAD));
#endif
	/*======生成文件=====*/
	snprintf(up_filename, PTN_STRLEN, "%s-up-%s.bin", ptable->fw_prefix, file_name);
	DEBUG("up file name: %s\n", up_filename);
	
	out_fd = open(up_filename, O_RDWR | O_CREAT | O_TRUNC, 00777);
	if (0 > out_fd)
	{
		printf("failed to open file.\n");
		free(basicAddr);
		return -1;
	}
	
	write(out_fd, basicAddr, upFileSize);

	close(out_fd);
	
	free(basicAddr);

	return 0;
}




int makeMUpFile()
{
	char *basicAddr;
	char *tmpFileBuf;
	char *fileAddr;
	unsigned char md5_digest[FWUP_HDR_MD5_LEN] = {'\0'};
	FWUP_FILE_HEAD fileHead = {0};

	int upFileSize = 0;
	int i;

	int out_fd = 0;
	char up_filename[PTN_STRLEN] = {0};

	g_upproptr = (partition_table *)(&upProStruct);
	memset(g_upproptr,0,sizeof(partition_table));
	memcpy(g_upproptr,ptable,sizeof(partition_table));

	basicAddr = (char *)malloc(g_upproptr->flash_size + sizeof(FWUP_FILE_HEAD));
	if (NULL == basicAddr)
	{
		printf("malloc error\n");
		return -1;
	}
	memset(basicAddr,0xFF,g_upproptr->flash_size + sizeof(FWUP_FILE_HEAD));
	
	tmpFileBuf = basicAddr + sizeof(FWUP_FILE_HEAD);
	
	fileAddr = tmpFileBuf+ 2 * UP_PTNTBL_SIZE;/*写入升级软件自索引*/
	upFileSize = 2 * UP_PTNTBL_SIZE;

	DEBUG("makeupcontent start\n");

	/*=====写入分区内容=====*/
	for(i = 0; i < g_upproptr->ptn_num; i ++)
	{
		/*加入是否可写的判断*/
		if (g_upproptr->entries[i].flag == NM_TYPE_BOTH 
		 || g_upproptr->entries[i].flag == NM_TYPE_UP
		 || g_upproptr->entries[i].flag == NM_TYPE_FLASH_AND_MUP)
		{
			if (-1 == makeUpContent(i,&fileAddr,&upFileSize,tmpFileBuf)/*MAX-3M-size*/)
			{
				printf("makeupcontent error\n");
				free(basicAddr);
				return -1;
			}
		}	
	}
	DEBUG("makeupcontent done\n");
	
	/*=====写入自索引====*/
	{
		char buf[NM_PTN_TABLE_SIZE];
		int len;
		memset(buf,0xFF,NM_PTN_TABLE_SIZE);
		makeMUpPtnTableFileByStruct(buf,&len);
		memcpy((void *)tmpFileBuf,(void *)buf,len+1);
	}
	
	upFileSize += sizeof(FWUP_FILE_HEAD);
	DEBUG("up file size: %d\n", upFileSize);
	
	/*==文件长度==*/
	fileHead.fileSize = htonl(upFileSize);
	memset(fileHead.product_id, 0xFF, 3*sizeof(unsigned int));
	memcpy(&fileHead.product_id, &l_dev_info.product_id, 3*sizeof(unsigned int));
	memcpy(fileHead.hw_id, l_dev_info.hw_id, CLOUD_ID_BYTE_LEN);
	memcpy(fileHead.oem_id, l_dev_info.oem_id, CLOUD_ID_BYTE_LEN);
	fileHead.sw_revision = l_dev_info.sw_revision;
	
	memcpy(basicAddr, &fileHead, sizeof(FWUP_FILE_HEAD));
	
	/*==MD5头部==*/
	memcpy(basicAddr+4, md5Key, FWUP_HDR_MD5_LEN);
	md5_make_digest(md5_digest, (unsigned char *)(basicAddr+4), (upFileSize-4));
    
	memcpy(fileHead.fileMd5, md5_digest, FWUP_HDR_MD5_LEN);	
	memcpy(basicAddr, &fileHead, sizeof(FWUP_FILE_HEAD));

	/*======生成文件=====*/
	snprintf(up_filename, PTN_STRLEN, "%s-mf-up-%s.bin", ptable->fw_prefix, file_name);
	DEBUG("up file name: %s\n", up_filename);
	
	out_fd = open(up_filename, O_RDWR | O_CREAT | O_TRUNC, 00777);
	if (0 > out_fd)
	{
		printf("failed to open file.\n");
		free(basicAddr);
		return -1;
	}
	
	write(out_fd, basicAddr,upFileSize);
	close(out_fd);
	
	free(basicAddr);

	return 0;
}



static int getFileContentToBuf_flash(char* buf, unsigned int BUFLEN, unsigned int *real_size,char* fileName)
{
	int retVal = 0;
	FILE *FP;
	*real_size = 0;
		
    if (fileName == NULL)
    {
		retVal = -1;
		return retVal;
    }	
	
	if((FP = fopen(fileName,"rb")) == NULL)
	{
		printf("open file(%s) error.\n",fileName);
		return -1;
	}
	fseek(FP,0,SEEK_END);
	*real_size = ftell(FP);

    if (*real_size > BUFLEN)
    {
		printf("file(%s) is too large. file:0x%x, buffer:0x%x\n",fileName, *real_size, BUFLEN);
        retVal = -1;
		goto ERROR_LEAVE;
    } 
	
	fseek(FP,0,SEEK_SET);
	fread(buf,1,*real_size,FP);
ERROR_LEAVE:
	fclose(FP);	
	return retVal;
}

static int makeFlashSetExtraPara(char *buf, unsigned int buflen, int *real_size)
{
	EXTRA_PARA_STRUCT extraPara;
	EXTRA_PARA_STRUCT *p;

	*real_size = sizeof(EXTRA_PARA_STRUCT);
	memset(&extraPara, 0, *real_size);

	//set the flash image boot mode
	if (bootmode == 1)
	{
		extraPara.dbootFlag = 1;
	}
	else
	{
		extraPara.dbootFlag = 0;
	}

	//set the flash image is integer
	extraPara.integerFlag = 1;

	if (*real_size > buflen)
	{
		DEBUG("make up extra para partition length is not enough.\n");
		return -1;
	}

	memcpy(buf, (char *)&extraPara, *real_size);
	p = (EXTRA_PARA_STRUCT *)buf;
	printf("size %d dbootflag %d integerflag %d\n",*real_size, p->dbootFlag, p->integerFlag);

	return 0;
}

static int makeFlashContent(partition_entry *pEntry,char*baseAddr)
{
	char *fileContentPtr;
	char *fileHeadPtr;
	char *fileEndPtr;
	char *setBase;

	char *strBuf = NULL;

	unsigned int ptnBaseAddr;
	unsigned int ptnSize;
	unsigned int real_size=0;
	unsigned int bigEnLen;
	
	ptnBaseAddr = pEntry->base;
	ptnSize = pEntry->size;


	fileHeadPtr = baseAddr + ptnBaseAddr;
	fileContentPtr = fileHeadPtr;

	DEBUG("%16s, 0x%08x, 0x%08x, %d, %d, %s\n",
			pEntry->name, pEntry->base,
			pEntry->size, pEntry->type,
			pEntry->flag, pEntry->file);
	
	switch(pEntry->type)
	{
	case NM_PTNCONTENT_PROFILE:
	case NM_PTNCONTENT_OUTPUT:
	case NM_PTNCONTENT_SIGNATURE:
	case NM_PTNCONTENT_PDINFO:
		if(0 != strcmp(pEntry->file, FILE_EMPTY))
		{
			if(getFileContentToBuf_flash(fileContentPtr,ptnSize,&real_size,pEntry->file) == -1)
			{
				printf("failed to read file");
				return -1;
			}
		}
		break;
	case NM_PTNCONTENT_PTNTBL:
		break;
	case NM_PTNCONTENT_SOFTVER:
		{
			memcpy((void *)fileContentPtr,(void *)&ver, sizeof(soft_ver));
			real_size = sizeof(soft_ver);
		}
		break;
	case NM_PTNCONTENT_EXTRAPARA:
		if (makeFlashSetExtraPara(fileContentPtr, pEntry->size, (int *)&real_size) < 0)
		{
			return -1;
		}
		break;
	default:
		break;
	}
	fileEndPtr = fileContentPtr + real_size;
	memset(fileEndPtr,0,1);/*分区结束set 00*/
	
	if (NM_PTNCONTENT_PTNTBL == pEntry->type)
	{
		real_size += 5;
	}

	return 0;
}



int package_flash_fw()
{
	FILE *fpFlash;
	char *tmpFileBuf;
	int flashFileSize = 0;
	int i;

	int out_fd = 0;
	char flash_filename[PTN_STRLEN] = {0};
	
	tmpFileBuf = malloc(ptable->flash_size);
	flashFileSize = ptable->flash_size;
	memset(tmpFileBuf,0xFF,flashFileSize);
	

	if (NULL == tmpFileBuf)
	{
		printf("malloc error\n");
		return -1;
	}

	DEBUG("makeFlashContent start\n");
	/*=====写入内容=====*/
	for(i = 0; i < ptable->ptn_num; i ++)
	{
		/*加入是否可写的判断*/
		if (ptable->entries[i].flag == NM_TYPE_BOTH 
		 || ptable->entries[i].flag == NM_TYPE_FLASH
		 || ptable->entries[i].flag == NM_TYPE_FLASH_AND_MUP)
		{
			if (-1 == makeFlashContent((partition_entry *)&(ptable->entries[i]),tmpFileBuf))
			{
				printf("makeFlashContent error\n");
				free(tmpFileBuf);
				return -1;
			}
		}
	}
	DEBUG("makeFlashContent done\n");

	/*======生成文件=====*/
	snprintf(flash_filename, PTN_STRLEN, "%s-flash-%s.bin", ptable->fw_prefix, file_name);
	DEBUG("flash file name: %s\n", flash_filename);
	
	out_fd = open(flash_filename, O_RDWR | O_CREAT | O_TRUNC, 00777);
	if (0 > out_fd)
	{
		printf("failed to open file.\n");
		free(tmpFileBuf);
		return -1;
	}
	
	write(out_fd, tmpFileBuf, flashFileSize);
	close(out_fd);
	free(tmpFileBuf);
	
	return 0;
}


int saveInHistory(void)
{
	FILE *FP;
	char stradd[PTN_STRLEN];
	int i = 0;
	char strBuf[STR_ID_LEN + 1];
	char strhwList[PTN_STRLEN] = {0};

	FP = fopen(HISTORY_NAME, "a+");
	if (NULL == FP)
	{
		printf("saveInDatabase: can't open %s.\n", HISTORY_NAME);
		return -1;
	}

	snprintf(stradd, PTN_STRLEN * 2, "{\n	%s:%s,\n	%s:%s,\n	%s:%s,\n	%s:%s,\n    %s:%s\n}\n",
	    SOFTVERSION, file_name, CLOUD_HWID, hw_id, CLOUD_FWID, fw_id, CLOUD_OEMID, oem_id, CLOUD_HWLIST, strhwList);

	fwrite(stradd, 1, strlen(stradd), FP);

	fclose(FP);

	return 0;
}

int idstrToByte(const char *pIdstr, unsigned char *pByte)
{
	int i = 0, c = 0;
	
	for (i = 0; i < 16; i++)
	{	
		if (*pIdstr >= '0' && *pIdstr <= '9')
		{
			c  = (unsigned char) (*pIdstr++ - '0'); 
		}
		else if (*pIdstr >= 'a' && *pIdstr <= 'f')
		{
			c  = (unsigned char) (*pIdstr++ - 'a') + 10;
		}
		else if (*pIdstr >= 'A' && *pIdstr <= 'F')
		{
			c  = (unsigned char) (*pIdstr++ - 'A') + 10; 
		}
		else
		{
			return -1;
		}

		c <<= 4;

		if (*pIdstr >= '0' && *pIdstr <= '9')
		{
			c |= (unsigned char) (*pIdstr++ - '0');
		}
		else if (*pIdstr >= 'a' && *pIdstr <= 'f')
		{
			c |= (unsigned char) (*pIdstr++ - 'a') + 10;
		}
		else if (*pIdstr >= 'A' && *pIdstr <= 'F')
		{
			c |= (unsigned char) (*pIdstr++ - 'A') + 10;
		}
		else
		{
			return -1;
		}

		pByte[i] = (unsigned char) c;

	}

	return 0;
}


int getStrAttrVal(char *buf, char *attr, char *value, int maxLen)
{
	char *p = NULL; 
	char *q = NULL; 
	char *valStart = NULL; 
	char *valEnd = NULL;

	if ((buf == NULL) || (attr == NULL) || (value == NULL))
	{
		fprintf(stderr, "%s(): get attr \"%s\" parameter error\n", __FUNCTION__, attr);
		return -1;
	}
	
	p = strstr(buf, attr);		
	if (p == NULL)
	{
		fprintf(stderr, "%s(): cannot find attr %s\n", __FUNCTION__, attr);
		return -1;
	}

	q = strstr(p , "d=");
	p = strchr(q, '>');
	if ((q == NULL) || (p == NULL))
	{
		fprintf(stderr, "%s(): \"%s\" cannot find \"d=\" Or \">\"\n", __FUNCTION__, attr);
		return -1;
	}

	q += 2;
	
	while (isspace(*q))
	{
		q++;
	}

	/*string value with quote */
	if ((*q == '\"') || (*q == '\''))
	{
		valStart = q + 1;
		valEnd = strchr(valStart, *q);

		if ((valEnd > p) || (valEnd - valStart > maxLen))
		{
			fprintf(stderr, "%s(): \"%s\" can't find right quote Or exceed max len\n", __FUNCTION__, attr);
			return -1;
		}
	}
	else
	{
		valStart = q;

		while(!isspace(*q) && (*q != '>'))
		{
			if ((*q) == '/' && (*(q + 1) == '>'))
			{
				break;
			}
			q++;
		}
		valEnd = q;

		if (valEnd - valStart > maxLen)
		{
			fprintf(stderr, "%s(): value of attr \"%s\" exceed max length\n", __FUNCTION__, attr);
			return -1;
		}
	}
	
	memcpy(value, valStart, valEnd - valStart);

	return 0;
}

int getVersion(const char *reduced_xml_name)
{
	int fd;
	char tmp[12] = {0};
	char tmpId[33] = {0};

	#define MAX_LEN	(80 * 1024)
	char buf[MAX_LEN] = {0};

	fd = open(reduced_xml_name, O_RDONLY);

	if (fd < 0)
	{
		perror("open profile error\n");
		return -1;
	}

	while (read(fd, buf, MAX_LEN - 1) > 0)
	{
		buf[MAX_LEN - 1] = 0;

		/* Get software revision */
		char *q = strstr(buf, "X_TP_SoftwareRevision");
		char *p = strstr(q, "d=");
		char *pEnd = NULL;
		int strLen = 0;
		
		if (p != 0)
		{
			p += 2;
			strncpy(tmp, p, 10);			
			l_dev_info.sw_revision = strtoul(tmp, NULL, 16);
		}

		/* Get platform version */
		p = strstr(buf, "X_TP_PlatformVersion");
		q = strstr(p, "d=");
		if (q != 0)
		{
			q += 2;
			strncpy(tmp, q, 10);			
			l_dev_info.platform_ver = strtoul(tmp, NULL, 16);  
		}

		/* Get product Id */		
		p = strstr(buf, "X_TP_ProductID");
		q = strstr(p, "d=");

		if (q != 0)
		{
			q += 2;
			strncpy(tmp, q, 10);			
			l_dev_info.product_id = strtoul(tmp, NULL, 16);  
		}

		/* Get product version */		
		p = strstr(buf, "X_TP_ProductVersion");
		q = strstr(p, "d=");

		if (q != 0)
		{
			q += 2;
			strncpy(tmp, q, 10);			
			l_dev_info.product_ver = strtoul(tmp, NULL, 16);  
		}

		/* Get special version */		
		p = strstr(buf, "X_TP_SpecialVersion");
		q = strstr(p, "d=");

		if (q != 0)
		{
			q += 2;
			strncpy(tmp, q, 10);			
			l_dev_info.special_ver = strtoul(tmp, NULL, 16);  
		}

		
		/* Get Additional HardwareVersion */		
		memset(tmp, 0, 12);	
		if (getStrAttrVal(buf, "AdditionalHardwareVersion", tmp, 10) != 0)
		{
			close(fd);
			return -1;
		}
		l_dev_info.add_hver = strtoul(tmp, NULL, 16);

		/* Get build date */		
		if (getStrAttrVal(buf, "X_TP_BuildDate", l_dev_info.build_date, BUFFER_LEN64) != 0)
		{
			close(fd);
			return -1;
		}		
		
		/* Get build time */		
		if (getStrAttrVal(buf, "X_TP_BuildTime", l_dev_info.build_time, BUFFER_LEN64) != 0)
		{
			close(fd);
			return -1;
		}	
		
		/* Get device model version */		
		if (getStrAttrVal(buf, "X_TP_DevModelVersion", l_dev_info.dev_ver, BUFFER_LEN64) != 0)
		{
			close(fd);
			return -1;
		}

		/* Get model name */		
		if (getStrAttrVal(buf, "ModelName", l_dev_info.model_name, BUFFER_LEN64) != 0)
		{
			close(fd);
			return -1;
		}

		/* Get HardwareID */		
		if (getStrAttrVal(buf, "X_TP_HardwareID", tmpId, CLOUD_ID_STR_LEN) != 0)
		{
			close(fd);
			return -1;
		}

		printf("hwid is %s\n", tmpId);

		if (idstrToByte(tmpId, l_dev_info.hw_id) != 0)
		{
			close(fd);
			return -1;
		}

		/* Get FirmwareID */	
		if (getStrAttrVal(buf, "X_TP_FirmwareID", tmpId, CLOUD_ID_STR_LEN) != 0)
		{
			close(fd);
			return -1;
		}

		printf("fwid is %s\n", tmpId);

		if (idstrToByte(tmpId, l_dev_info.fw_id) != 0)
		{
			close(fd);
			return -1;
		}		


		/* Get OemID */	
		if (getStrAttrVal(buf, "X_TP_OemID", tmpId, CLOUD_ID_STR_LEN) != 0)
		{
			close(fd);
			return -1;
		}

		printf("oemid is %s\n", tmpId);

		if (idstrToByte(tmpId, l_dev_info.oem_id) != 0)
		{
			close(fd);
			return -1;
		}

		break;
	}

	close(fd);

	return 0;
}


static void show_usage(void)
{
	printf(
"Usage: make_flash [OPTIONS]\n\n"
"  -p, --partition=FILE             Partition file\n"
"  -o, --output=FILE                Output Filename prefix\n"
"  -c, --cloud=FILE                 Cloud Info file\n"
"  -s, --softver                    Software version in format \"V1.0.0P1\"\n"
"  -r, --relver                     release version\n"
"  -t, --time                       in format 2015.01.15\n"
"  -m, --mode-double                Set image in double bootloader mode.\n"
"  -d, --debug                      Display debug information\n"
"  -h, --help                       Display this message\n"
"  -v, --version                    Display version\n"
"  -x, --version                    Data model file\n"
"  -P, --prestore                   pre install the userconfig in flash and special mf up file\n"
	);
}


int main(int argc, char* argv[])
{
	int c = 0;
	unsigned char softver[16] = {"V1.0.0P1"};
	unsigned char relver[8] = {0};
	unsigned char date[16] = {0};
	char reduced_xml_name[PTN_STRLEN] = {0};
	memset(reduced_xml_name, 0, PTN_STRLEN);
	
	static const struct option arg_options[] = {
		{"partition",		required_argument,	0, 'p'},
		{"output",			required_argument,	0, 'o'},
		{"softver",			required_argument,	0, 's'},
		{"relver",			required_argument,	0, 'r'},
		{"date",			required_argument,	0, 't'},
		{"cloud",			required_argument,	0, 'c'},
		{"datamodel",		required_argument,	0, 'x'},
		{"mode",			no_argument,		0, 'm'},
		{"debug",			no_argument,		0, 'd'},
		{"help",			no_argument,		0, 'h'},
		{"version",			no_argument,		0, 'v'},
		{"prestore",        no_argument,        0, 'P'},
		{0, 0, 0, 0}
	};

	memset(&ptn_table, 0, sizeof(partition_table));
	ptable= &ptn_table;
	
	while (1) {
		int option_index = 0;
		c = getopt_long(argc, argv, "p:o:s:r:t:c:x:mdhvP", arg_options, &option_index);
		if (c == -1) break;
		
		switch (c) {
		case 'p':
			strncpy(ptable->ptn_file, optarg, 
				strlen(optarg) > PTN_STRLEN?PTN_STRLEN:strlen(optarg));
			break;
		case 'o':
			strncpy(ptable->fw_prefix, optarg, 
				strlen(optarg) > PTN_STRLEN?PTN_STRLEN:strlen(optarg));
			break;
		case 's':
			strncpy(softver, optarg, 
				strlen(optarg) > 16?16:strlen(optarg));
			break;
		case 'r':
			strncpy(relver, optarg, 
				strlen(optarg) > 5?5:strlen(optarg));
			break;
		case 't':
			strncpy(date, optarg, 
				strlen(optarg) > 16?16:strlen(optarg));
			break;
		case 'm':
			bootmode = 1;
			printf("make_flash in double bootloader mode\n");
			break;
		case 'd':
			debug = TRUE;
			break;
		case 'P':
			prestore = 1;
			break;
		case 'h':
			show_usage();
			exit(0);
			break;
		case 'v':
			printf("make_flash, version %s\n\n", VERSION);
			exit(0);
			break;
		case 'x':
			strncpy(reduced_xml_name, optarg,
				strlen(optarg) > PTN_STRLEN?PTN_STRLEN:strlen(optarg));
			break;
		}
	}
	DEBUG("make_flash: start.\n");
	
	if (getVersion(reduced_xml_name) < 0)
	{
		fprintf(stderr, "get version error!\n");
		return -1;
	}
	DEBUG("make_flash: getVersion done.\n");

	if (parse_fw_ver(softver, relver, date) < 0)
	{
		printf("parse firmware version info error.\n");
		return 0;
	}
	DEBUG("make_flash: parse firmware version done.\n");	
	
	if (parse_ptn_table(ptable) < 0)
	{
		printf("parse partition file error.\n");
		return 0;
	}
	DEBUG("make_flash: parse partition table file done.\n");

	print_table(ptable);

	if (check_table(ptable) < 0)
	{
		printf("check partition table error.\n");
		return 0;
	}
	DEBUG("make_flash: check partition table done.\n");
	
	package_up_fw();
	DEBUG("make_flash: make up file done.\n");
	
	package_flash_fw();
	DEBUG("make_flash: make flash file done.\n");

	return 0;
}


