/***************************************************************
 *
 * Copyright(c) 2005-2007 Shenzhen TP-Link Technologies Co. Ltd.
 * All right reserved.
 *
 * Filename		:	md5_interface.c
 * Version		:	1.0
 * Abstract		:	md5 make and verify response interface
 * Author		:	LI SHAOZHANG (lishaozhang@tp-link.net)
 * Created Date	:	07/11/2007
 *
 * Modified History:
 ***************************************************************/
#include "string.h"
#include "md5.h"
#include "md5_interface.h"

/*
 * md5_make_digest:make md5 digest for 'input' and save in 'digest'
 */
void md5_make_digest(unsigned char* digest, const unsigned char* input, int len)
{
	MD5_CTX ctx;
	
	MD5_Init(&ctx);
	MD5_Update(&ctx, input, len);
	MD5_Final(digest, &ctx);
}

/* verify the 'digest' for 'input'*/
int md5_verify_digest(unsigned char* digest, unsigned char* input, int len)
{
	unsigned char digst[MD5_HASH_SIZE];
	
	md5_make_digest(digst, input, len);

	if (memcmp(digst, digest, MD5_HASH_SIZE) == 0)
		return 1;
	
	return 0;
}

void hmac_md5(
			unsigned char* text, /* pointer to data stream */
			int text_len, /* length of data stream */
			unsigned char* key, /* pointer to authentication key */
			int key_len, /* length of authentication key */
			unsigned char * digest) /* caller digest to be filled in */
{
	MD5_CTX context;
	unsigned char k_ipad[65]; /* inner padding -key XORd with ipad */
	unsigned char k_opad[65]; /* outer padding key XORd with opad*/

	unsigned char tk[16];
	int i;

	/* if key is longer than 64 bytes reset it to key=MD5(key) */

	if (key_len > 64) 
	{
		MD5_CTX tctx;
		MD5_Init(&tctx);
		MD5_Update(&tctx, key, key_len);
		MD5_Final(tk, &tctx);
		key = tk;
		key_len = 16;
	}


	/*
	the HMAC_MD5 transform looks like:
	MD5(K XOR opad, MD5(K XOR ipad, text))
	where K is an n byte key
	ipad is the byte 0x36 repeated 64 times
	opad is the byte 0x5c repeated 64 times
	and text is the data being protected
	*/

	/* start out by storing key in pads */
	bzero( k_ipad, sizeof k_ipad);
	bzero( k_opad, sizeof k_opad);
	bcopy( key, k_ipad, key_len);
	bcopy( key, k_opad, key_len);
	/* XOR key with ipad and opad values */
	for (i=0; i<64; i++) 
	{
		k_ipad[i] ^= 0x36;
		k_opad[i] ^= 0x5c;
	}

	/*perform inner MD5	*/

	MD5_Init(&context); /* init context for 1st pass */
	MD5_Update(&context, k_ipad, 64); /* start with inner pad */
	MD5_Update(&context, text, text_len); /* then text of datagram */
	MD5_Final((unsigned char*)digest, &context); /* finish up 1st pass */

	/* perform outer MD5 */

	MD5_Init(&context); /* init context for 2nd pass */
	MD5_Update(&context, k_opad, 64); /* start with outer pad */
	MD5_Update(&context, digest, 16); /* then results of 1st hash */
	MD5_Final((unsigned char*)digest, &context); /* finish up 2nd pass */
}

