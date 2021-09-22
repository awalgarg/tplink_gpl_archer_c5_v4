/***************************************************************
 *
 * Copyright(c) 2005-2007 Shenzhen TP-Link Technologies Co. Ltd.
 * All right reserved.
 *
 * Filename		:	md5_interface.h
 * Version		:	1.0
 * Abstract		:	md5 make and verify response interface
 * Author		:	LI SHAOZHANG (lishaozhang@tp-link.net)
 * Created Date	:	07/11/2007
 *
 * Modified History:
 ***************************************************************/


#ifndef _MD5_INTERFACE_H_
#define _MD5_INTERFACE_H_

#define MD5_HASH_SIZE		16

void md5_make_digest(unsigned char* digest, unsigned char* input, int len);

int md5_verify_digest(unsigned char* digest, unsigned char* input, int len);

void hmac_md5(unsigned char* text, int text_len, unsigned char* key, 
			int key_len, unsigned char * digest);

unsigned long calc_crc32(const char *buf, size_t len);

#endif /* _MD5_INTERFACE_H_ */

