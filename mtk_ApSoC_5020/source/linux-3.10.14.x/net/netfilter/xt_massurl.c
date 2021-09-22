/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		xt_massurl.c
 * brief		
 * details	
 *
 * author	wangwenhao
 * version	
 * date		24Oct11
 *
 * history \arg	1.0, 24Oct11, wangwenhao, create file
 *         \arg 1.1, 25Nov11, zhulu, modified some function interfaces for kernel 2.6.30.9
 		   \arg 1.2, 6Jul17, zhangwenkai, adapt to IPv6 for kernel 3.10.14
 *
 * note		we use hash methed to do string match between "DNS domain/HTTP Host" and mass strings
 * 			configured by user
 *			if we use hash method CPU cost is:	l * (2 + n / x * (2 + c / (c - 1)))
 *			if we use mormal methoed then:		l * n * (c / (c - 1) + 1)
 *
 *			'l' means length of the HTTP host
 *			'n' means count of mass strings
 *			'c' means charactors used in HTTP host
 *			'x' means member count of hash array
 *
 *			so if 'n' gets large and 'x' gets big enough, hash method is much better than normal
 */
 
#include <linux/module.h>
#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter/xt_massurl.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <asm/uaccess.h>
#include <net/netfilter/nf_conntrack_ecache.h>
#include <net/ipv6.h>
#include <linux/version.h>

#ifndef CONFIG_NF_CONNTRACK_MARK
# error "MASSURL need CONNTRACK_MARK selected"
#endif

/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/
#define	HOST_STR	"\r\nHost: "

#define URL_HASH_EXP 5
#define URL_HASH_SIZE (1 << URL_HASH_EXP)
#define URL_HASH_MASK (URL_HASH_SIZE - 1)

#define DNS_HEADER_LEN 12

#define HASH(a) ((unsigned char)(a) & URL_HASH_MASK)

#define U32_BIT_MASK 0x1f
#define BIT_ISSET(index, set) (set[index >> 5] & (1 << (index & U32_BIT_MASK)))

#define HANDSHAKE 22 /*ssl: content type.SSL_TYPE*/
#define CLIENT_HELLO 1 /*handshake: content type.HANDSHAKE_TYPE*/
#define SERVER_NAME 0 /*extension type in client hello(can only appear once in client hello).EXTENSION_TYPE*/
#define HOST_NAME 0 /*content type in SNI(in server_name extension).SERVER_NAME_TYPE*/

#define NET_3BYTES_TO_HOST_UINT32(pNet) \
((((uint32_t)*(pNet)) << 16) + (((uint32_t)*((pNet) + 1))<< 8) + ((uint32_t)*((pNet) + 2)))

#define CHECK_MSG_POINTER(curPtr,endPtr) \
((curPtr) <= (endPtr) ? true : false)


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35))
#define CHECK_OK 0
#define CHECK_FAIL(err) (err)
#else
#define CHECK_OK 1
#define CHECK_FAIL(err) 0
#endif

/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/
struct hash_url {
	struct hlist_node node;
	char url[MASSURL_URL_LEN];
	unsigned char offset;
	unsigned short index;
};

typedef int (*CMP_FUNC)(const void *, const void *, size_t);

/* Start adding by fang ruilong*/
#define DNS_PORT 53
#define HTTP_PORT 80
#define HTTPS_PORT 443

typedef struct _PROTOCOL_VERSION
{
	uint8_t majorVersion;
	uint8_t minorVersion;
}PROTOCOL_VERSION;

typedef struct _SSL_MSG{
	uint8_t type; /*len:1 byte*/
	PROTOCOL_VERSION version; /*len:2 bytes*/
	uint16_t length; /* The length (in bytes) of the following TLSPlaintext.fragment.*/
	uint8_t *pContent; /*  The application data,type is specified by the type field.*/
}SSL_MSG;

typedef uint32_t uint24_t;
typedef struct{
	uint16_t length;
	uint8_t *pData;
}CIPHER_SUITE,CH_EXTENSIONS;

typedef struct{
	uint8_t length;
	uint8_t *pData;
}SESSION_ID,COMPRESSION_METHOD;

typedef struct _TLS_EXTENSION{
	uint16_t type;
	uint16_t length;
	uint8_t *pData;
}TLS_EXTENSION;/*TLS(client hello) extension*/
typedef struct _HANDSHAKE_CLIENT_HELLO{
	uint8_t type; /*len:1 byte*/
	uint24_t length;
	PROTOCOL_VERSION clientVersion;
    uint8_t *random;/*the length is 32,but we don't need this field.So only give pointer to start position*/
    SESSION_ID sessionID;
    CIPHER_SUITE cipherSuites;
    COMPRESSION_METHOD compression_methods;
    uint8_t *pExtensions /*pointer to extensions length field*/;
}HANDSHAKE_CLIENT_HELLO;

typedef enum _HTTPS_MATCH_RESULT
{
	HTTPS_UNMATCH = 0,
	HTTPS_MATCH_SPEED = 1,
	HTTPS_MATCH_NO_SPEED = 2, /*used to distinguish HTTPS_MATCH_SPEED.*/
	HTTPS_UNIDENTIFY = 3/*current message shouldn't be filtered by SNI in client_hello.*/
}HTTPS_MATCH_RESULT;
/**************************************************************************************************/
/*                                           EXTERN_PROTOTYPES                                    */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           LOCAL_PROTOTYPES                                     */
/**************************************************************************************************/
/* Start adding by fang ruilong*/
static bool extractSNIFromExtensions(const uint8_t *pExtensions,uint8_t **ppSNIExt);
static bool extractSNIFromClientHello(const uint8_t *pClientHello, uint8_t **ppSNIExt);
static HTTPS_MATCH_RESULT matchUrlBasedOnSNI(const uint8_t *pSNIExt,
													const uint8_t *pExtEnd,
													const struct xt_massurl_info *info);
static bool extractHandshakeFromSSL(const uint8_t *pSSL, uint8_t **ppHandshake);
static HTTPS_MATCH_RESULT match_https_clientHello(const struct sk_buff *skb,
	                                                   const struct xt_massurl_info *info);
static int url_strhash(const unsigned char *start,
				const unsigned char *end,
				unsigned int *pIndexBits,
                                unsigned int type);
static bool match(const struct sk_buff *skb, struct xt_match_param *par);

static int checkentry(const struct xt_mtchk_param *par);

/*add by frl 2015-11-18*/
static int isLenByte(const char *pStartDns, const char *pCurDns);
/*add end*/
/*add by wuzeyu 2015.03.06, to compare http url*/
static int cmpdns(const void *url, const void *dns, const void *dnsStart, size_t len);
static int cmpHttpUrl(const char *configUrl, const char *httpUrl, size_t len);
/*end add*/

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/
static DEFINE_RWLOCK(massurl_lock);

static struct xt_match xt_massurl_match[] __read_mostly = { 
	{
	.family			= AF_INET,
    .name           = "massurl",
    .match          = match,
    .matchsize		= sizeof(struct xt_massurl_info),
    .checkentry     = checkentry,
    .me             = THIS_MODULE,
	},
	{
	.family			= AF_INET6,
    .name           = "massurl",
    .match          = match,
    .matchsize		= sizeof(struct xt_massurl_info),
    .checkentry     = checkentry,
    .me             = THIS_MODULE,
	},
};

static struct hash_url urls[MASSURL_MAX_ENTRY];

static struct hlist_head urlHash[URL_HASH_SIZE];

unsigned short urlHashCount[URL_HASH_SIZE];

/**************************************************************************************************/
/*                                           LOCAL_FUNCTIONS                                      */
/**************************************************************************************************/
#if ( LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,14) )
static inline struct tcphdr *tp_tcp_hdr(const struct sk_buff *skb)
{
	if (ntohs(skb->protocol) == ETH_P_IP && ip_hdr(skb)->protocol== IPPROTO_TCP)
	{
		return tcp_hdr(skb);
	}
	else if (ntohs(skb->protocol) == ETH_P_IPV6 && ipv6_hdr(skb)->version == 6)
	{
		__u8 nexthdr = ipv6_hdr(skb)->nexthdr;
		int tcphoff;
		__be16 fragoff;

		tcphoff = ipv6_skip_exthdr(skb, sizeof(struct ipv6hdr), &nexthdr, &fragoff);
		if (tcphoff < 0 || nexthdr != IPPROTO_TCP)
			return NULL;

		return (struct tcphdr *)((unsigned char *)ipv6_hdr(skb) + tcphoff);
	}

	return NULL;
}
static inline struct udphdr *tp_udp_hdr(const struct sk_buff *skb)
{
	if (ntohs(skb->protocol) == ETH_P_IP && ip_hdr(skb)->protocol == IPPROTO_UDP)
	{
		return udp_hdr(skb);
	}
	else if (ntohs(skb->protocol) == ETH_P_IPV6 && ipv6_hdr(skb)->version == 6)
	{
		__u8 nexthdr = ipv6_hdr(skb)->nexthdr;
		int udphoff;
		__be16 fragoff;

		udphoff = ipv6_skip_exthdr(skb, sizeof(struct ipv6hdr), &nexthdr, &fragoff);
		if (udphoff < 0 || nexthdr != IPPROTO_UDP)
			return NULL;

		return (struct udphdr *)((unsigned char *)ipv6_hdr(skb) + udphoff);
	}

	return NULL;
}
#endif

/* 
 * fn		static bool extractSNIFromExtensions(const uint8_t *pExtensions, uint8_t *ppSNIExt) 
 * brief	extract SNI extension form extensions.
 * details	get pointer to start position of SNI extension that exists in server name extension.
 *
 * param[in]	pExtensions - pointer to start of extensionList.
 * param[out]	ppSNIExt      - address of pointer to SNI extension.
 *
 * return	bool
 * retval	true - extract SNI extension successfully.
 *          false - extract SNI extension unsuccessfully.
 * note		
 */
static bool extractSNIFromExtensions(const uint8_t *pExtensions, uint8_t **ppSNIExt)
{
	int extensionsLen; /*length of all extensions.*/
	int handledExtLen;/*length of handled extensions.*/
	TLS_EXTENSION ext;

	extensionsLen = ntohs(*((uint16_t *)pExtensions));
	pExtensions += 2;
	
	for (handledExtLen = 0; handledExtLen < extensionsLen; )
	{
		ext.type = ntohs(*((uint16_t *)pExtensions));
		pExtensions += 2;
		ext.length = ntohs(*((uint16_t *)pExtensions));
		pExtensions += 2;
		ext.pData = (ext.length ? (uint8_t *)pExtensions : NULL);
		if (SERVER_NAME == ext.type)
		{
			*ppSNIExt = ext.pData;
			if (ext.length)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		pExtensions += ext.length;
		handledExtLen += (2 + 2 + ext.length);
	}

	return false;
}
/* 
 * fn		static  bool extractSNIFromClientHello(const uint8_t *pClientHello, uint8_t **ppSNIExt) 
 * brief	extract SNI extension(Server_name)represents host_name from client_hello.
 * details	get pointer to start position of SNI extension from client_hello message.
 *
 * param[in]	pClientHello - pointer to start position of client_hello message.
 * param[out]	ppSNIExt - address of pointer to the start position of SNI extension in client_hello.
 *
 * return	bool
 * retval	true -get the SNI represents host_name.
 *			false - doesn't get the right SNI.
 * note		
 */
 static bool extractSNIFromClientHello(const uint8_t *pClientHello, uint8_t **ppSNIExt)
{
	HANDSHAKE_CLIENT_HELLO clientHello;
	/*
	clientHello.type = *pClientHello++;
	clientHello.length = NET_3BYTES_TO_HOST_UINT32(pClientHello);
	pClientHello += 3;
	Ignore type and length of client_hello.
	*/
	pClientHello += 4;
	
	clientHello.clientVersion.majorVersion = *pClientHello++;
	clientHello.clientVersion.minorVersion = *pClientHello++;
	/*SNI extension is not supported until TLS 1.0(version 0x0301)*/
	if (clientHello.clientVersion.majorVersion < 3
	 || (3 == clientHello.clientVersion.majorVersion && 0 == clientHello.clientVersion.minorVersion))
	{
		return false;
	}
	/*clientHello.random = pClientHello;*/
	pClientHello += 32;/*length of random is fixed.*/
	clientHello.sessionID.length = *pClientHello++;
	/*clientHello.sessionID.pData = pClientHello;*/
	pClientHello += clientHello.sessionID.length;
	clientHello.cipherSuites.length = ntohs(*((uint16_t *)pClientHello));
	pClientHello += 2;
	/*clientHello.cipherSuites.pData = pClientHello;*/
	pClientHello += clientHello.cipherSuites.length;
	clientHello.compression_methods.length = *pClientHello++;
	/*clientHello.compression_methods.pData = pClientHello;*/
	
	pClientHello += clientHello.compression_methods.length;
	clientHello.pExtensions = (uint8_t *)pClientHello;

	return extractSNIFromExtensions(clientHello.pExtensions, ppSNIExt);
}
/* 
 * fn		static HTTPS_MATCH_RESULT matchUrlBasedOnSNI(const uint8_t *pSNIExt,
 *														 const uint8_t *pExtEnd,
 *														 const struct xt_massurl_info *info) 
 * brief	match the match(massurl) based on host_name in SNI extension.
 * details	pSNIExt shouldn't be greater than pExtEnd.
 *
 * param[in]	pSNIExt - pointer to start of SNI extension.
 * param[in]	pExtEnd - end position of actually accepted SSL packet(or extensions).
 * param[in]	info  - pointer to the massurl to be matched.
 * param[out]	
 *
 * return	HTTPS_MATCH_RESULT
 * retval	HTTPS_MATCH - match successfully.
 *          HTTPS_UNMATCH - match successfully.
 *          HTTPS_UNIDENTIFY - current message shouldn't be filtered(to match the rule).
 * note		
 */
static HTTPS_MATCH_RESULT matchUrlBasedOnSNI(const uint8_t *pSNIExt, 
													const uint8_t *pExtEnd,
												    const struct xt_massurl_info *info)
{
	TLS_EXTENSION SNIExt;/*format is similar with server name extension*/
	int SNIListLen;
	int handledSNILen; 

	SNIListLen = ntohs(*((uint16_t *)pSNIExt));
	pSNIExt += 2;

	for (handledSNILen = 0; handledSNILen < SNIListLen; )
	{
		SNIExt.type = *pSNIExt++;
		SNIExt.length = ntohs(*((uint16_t *)pSNIExt));
		pSNIExt += 2;
		SNIExt.pData = (uint8_t *)pSNIExt;
		pSNIExt += SNIExt.length;
		/*Does CLENT HELLO  fragment have impact on SNI?*/
		if (!CHECK_MSG_POINTER(pSNIExt, pExtEnd))
		{
			return HTTPS_UNIDENTIFY;
		}
		handledSNILen += (1 + 2 + SNIExt.length);
		if (HOST_NAME == SNIExt.type
		 && url_strhash(SNIExt.pData, pSNIExt, (uint32_t *)(info->urlIndexBits), MASSURL_TYPE_HTTP))
		{
			return (info->type == MASSURL_TYPE_HTTP) ? HTTPS_MATCH_SPEED : HTTPS_MATCH_NO_SPEED;
		}
		
	}
	
	return HTTPS_UNMATCH;	
}

/* 
 * fn		static bool extractHandshakeFromSSL(const uint8_t *pSSLBuff, uint8_t **ppHandshake) 
 * brief	extract the handshake From SSL packet.
 * details	only get address of the pointer to handshake.
 *
 * param[in]	pSSL - pointer to the start of SSL packet in skb_buff.
 * param[out]	ppHandshake - address of pointer to the start of handshake message wrapped with SSLv3/TLS.
 *
 * return	bool
 * retval	true  succeed to extract handshake 
 *			false fail to extract handshake  
 * note		
 */
static bool extractHandshakeFromSSL(const uint8_t *pSSL, uint8_t **ppHandshake)
{
	SSL_MSG ssl;
	
	if ((ssl.type = *pSSL++) != HANDSHAKE)
	{
		return false;
	}
	/*
	ssl.version.majorVersion = *pSSL++;
	ssl.version.minorVersion = *pSSL++;
	*/
	pSSL += 2;
	
	ssl.length = ntohs(*((uint16_t *)pSSL));
	pSSL += 2;
	
	if(0 == ssl.length)
	{
		return false;
	}
	/*ssl.pContent = pSSL;*/
	*ppHandshake = (uint8_t *)pSSL;

	
	return true;
}

/* 
 * fn		static HTTPS_MATCH_RESULT match_https_clientHello(const struct sk_buff *skb,
 *                                                            const struct xt_massurl_info *info)
 * brief	match the match(massurl) based on( SNI in )client_hello.
 * details	
 *
 * param[in]	skb - pointer to sk_buff.
 * param[in]	info  - pointer to the massurl to be matched.
 *
 * return	HTTPS_MATCH_RESULT
 * retval	HTTPS_UNMATCH - match successfully.
 *			HTTPS_MATCH_SPEED - (conn)match successfully.
 *			HTTPS_MATCH_NO_SPEED - allow TCP handsheke message to pass through.
 *          HTTPS_UNIDENTIFY - current message shouldn't be filtered(to match the rule).
 * note		
 */
static HTTPS_MATCH_RESULT match_https_clientHello(const struct sk_buff *skb,
                                                       const struct xt_massurl_info *info)
{
	const struct iphdr *iph = ip_hdr(skb);
#if ( LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,14) )
	const struct ipv6hdr *ip6h = ipv6_hdr(skb);
	struct tcphdr *tcph;
#else
	struct tcphdr *tcph = (void *)iph + iph->ihl * 4;
#endif
	unsigned char *sslStart;
	unsigned char *sslEnd;
	uint8_t *pHandshake;
	uint8_t * pSNIExt;
		
#if ( LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,14) )
	/* fail only with broken skb, for bound iptables rule has specified protocol as TCP. */
	if (unlikely(NULL == (tcph = tp_tcp_hdr(skb))))
	{
		return 0;
	}
	sslStart = (unsigned char *)tcph + tcph->doff * 4;
	
	if (ntohs(skb->protocol) == ETH_P_IP && iph->version == 4)
	{
		sslEnd = (unsigned char *)iph + ntohs(iph->tot_len);
	}
	else if (ntohs(skb->protocol) == ETH_P_IPV6 && ip6h->version == 6)
	{
		sslEnd = (unsigned char *)ip6h + sizeof(struct ipv6hdr) + ntohs(ip6h->payload_len);
	}
	else
	{
		return 0;
	}
#else
	sslStart = (unsigned char *)tcph + tcph->doff * 4;
	sslEnd = sslStart + (ntohs(iph->tot_len) - iph->ihl * 4 - tcph->doff * 4);
#endif
	if (sslStart >= sslEnd)
	{
		if (MASSURL_TYPE_HTTP == info->type)
		{
			return HTTPS_MATCH_NO_SPEED;
		}
		else
		{
			return HTTPS_UNMATCH;
		}
	}

	if ((!extractHandshakeFromSSL(sslStart, &pHandshake))
	 || (*pHandshake != CLIENT_HELLO)
	 || (!extractSNIFromClientHello(pHandshake, &pSNIExt)))
	{
		return HTTPS_UNIDENTIFY;
	}

	return matchUrlBasedOnSNI(pSNIExt, sslEnd, info);
}

static unsigned char *url_strstr(const unsigned char *start,
								const unsigned char *end,
								const unsigned char *strCharSet)
{
	unsigned char *s_temp = (unsigned char *)start;        /*the s_temp point to the s*/

	int l1, l2;

	l2 = strlen(strCharSet);
	if (!l2)
		return (unsigned char *)start;

	l1 = end - s_temp;

	while (l1 >= l2)
	{
		l1--;
		if (!memcmp(s_temp, strCharSet, l2))
			return (unsigned char *)s_temp;
		s_temp++;
	}

	return NULL;
}

/* 
 * fn		static int isLenByte(const char *startDns, const char *startDns)
 * brief	judge whether curDns represents len in startDns.
 * details	
 *
 * param[in]	pStartDns
 * param[in]	pCurDns
 *
 * return	
 * retval	
 *
 * note		
 */
static int isLenByte(const char *startDns, const char *curDns)
	{
	const char *dnsLenBytePos = startDns;

	while (dnsLenBytePos <= curDns)
			{
		if (dnsLenBytePos == curDns)
			{
			return 1;
			}
		if (*dnsLenBytePos != 0)
			{
			dnsLenBytePos += ((int)*dnsLenBytePos + 1);
		}
		else
		{
			return 0;
}
	}

	return 0;
}

static int cmpdns(const void *url, const void *dns, const void *dnsStart, size_t len)
{
	const unsigned char *uUrl, *uDns;
	unsigned char tmpCount = 0;
	unsigned char count = 0;

	for (uUrl = url, uDns = dns; len > 0; len--, uUrl++, uDns++)
	{
		if (*uUrl == '.')
		{
			if (tmpCount != count && tmpCount != 0)
			{
				return 1;
			}

			/*value of uDns doesn't represent len of continus domain name.*/
			if (!isLenByte(dnsStart, uDns))
			{
				return 1;
			}
			tmpCount = *uDns;
			count = 0;
		}
		else
		{
			/*add by wuzeyu for capital form*/
			if (*uDns >= 'A' && *uDns <= 'Z') 
			{
				if (*uUrl != (*uDns + ('a' - 'A')))
				{
					return 1;
				}
			}
			/*end add*/
			else if (*uUrl != *uDns)
			{
				return 1;
			}
			count++;
		}
	}

	return 0;
}

/*add by wuzeyu 2015.03.06, to compare char do not distinguish capital form or lower case */
static int cmpHttpUrl(const char *configUrl, const char *httpUrl, size_t len)
{
	const unsigned char *uUrl = NULL;
	const unsigned char *uHttp = NULL;
	if (configUrl == NULL || httpUrl == NULL)
	{
		printk("the str is NULL\n");
		return 1;
	}
	for (uUrl = configUrl, uHttp = httpUrl; len > 0; len--, uUrl++, uHttp++)
	{
		if (*uHttp >= 'A' && *uHttp <= 'Z') /*for capital form*/
		{
			if (*uUrl != (*uHttp + ('a' - 'A')))
			{
				return 1;
			}
		}
		else if (*uUrl != *uHttp)
		{
			return 1;
		}
	}
	return 0;
}
/*end add****************/

static int url_strhash(const unsigned char *start,
				const unsigned char *end,
				unsigned int *pIndexBits,
				unsigned int type)
{
	const unsigned char *pIndex = start;
	const unsigned char *offStart;
	struct hash_url *pUrl;
	size_t len;

	read_lock(&massurl_lock);
	while (pIndex < end)
	{
		hlist_for_each_entry(pUrl, &urlHash[HASH(*pIndex)], node)
		{
			if (!BIT_ISSET(pUrl->index, pIndexBits))
			{
				continue;
			}
			
			offStart = pIndex - pUrl->offset;
			if (offStart < start)
			{
				continue;
			}

			len = strlen(pUrl->url);
			if (end - offStart < len)
			{
				continue;
			}
			
			if (MASSURL_TYPE_HTTP == type && 0 == cmpHttpUrl((unsigned char *)pUrl->url, offStart, len))/*change by wuzeyu*/
			{
				read_unlock(&massurl_lock);
				/* printk("!!!!!!!!!!http packet caught!!!!!!!!!\n"); */
				return 1;
			}
			
			if (MASSURL_TYPE_DNS == type && 0 == cmpdns((unsigned char *)pUrl->url, offStart, start, len))/*change by wuzeyu*/
			{
				read_unlock(&massurl_lock);
				/* printk("!!!!!!!!!!dns packet caught!!!!!!!!!\n"); */
				
				return 2;
			}
		}

		pIndex++;
	}
	
	read_unlock(&massurl_lock);
	return 0;
}

static int setUrl(void __user *user, unsigned int userLen)
{
	struct massurl_url_info urlInfo;
	size_t len;
	struct hash_url *pUrl;
	char *pIndex;
	int hash;
	int minCount;
	int minHash;

	if (copy_from_user(&urlInfo, user, sizeof(urlInfo)) != 0)
	{
		return -EFAULT;
	}

	if (urlInfo.index >= MASSURL_MAX_ENTRY)
	{
		printk(KERN_WARNING "url index overflow\n");
		return -EINVAL;
	}

	pUrl = &urls[urlInfo.index];

	len = strlen(urlInfo.url);
	write_lock_bh(&massurl_lock);
	if (len == 0)
	{
/*
		if (pUrl->url[0] == '\0')
		{
			write_unlock_bh(&massurl_lock);
			printk(KERN_WARNING "can not del index empty\n");
			return -EINVAL;
		}
*/
		if (pUrl->url[0] != '\0')
		{
			urlHashCount[HASH(pUrl->url[pUrl->offset])]--;
			hlist_del(&pUrl->node);
			pUrl->url[0] = '\0';
			pUrl->offset = 0;
		}
	}
	else
	{
		if (pUrl->url[0] != '\0')
		{
			write_unlock_bh(&massurl_lock);
			printk(KERN_WARNING "can not add to already exist index\n");
			return -EINVAL;
		}
		
		strncpy(pUrl->url, urlInfo.url, MASSURL_URL_LEN);
		pUrl->url[MASSURL_URL_LEN - 1] = '\0';

		minCount = MASSURL_MAX_ENTRY + 1;
		minHash = HASH('.');	/* defult . for all . string */
		for (pIndex = pUrl->url; *pIndex != '\0'; pIndex++)
		{
			if (*pIndex == '.')
			{
				continue;
			}
			
			hash = HASH(*pIndex);

			if (urlHashCount[hash] < minCount)
			{
				minHash = hash;
				minCount = urlHashCount[hash];
				pUrl->offset = (unsigned char)(pIndex - pUrl->url);
			}
		}
		hlist_add_head(&pUrl->node, &urlHash[minHash]);
		urlHashCount[minHash]++;
		/*printk("###add url %s to hash %d\n", pUrl->url, minHash);*/
	}
	write_unlock_bh(&massurl_lock);

	return 0;
}

static int match_http(const struct sk_buff *skb, struct xt_massurl_info *info)
{
	const struct iphdr *iph = ip_hdr(skb);
#if ( LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,14) )
	const struct ipv6hdr *ip6h = ipv6_hdr(skb);
	struct tcphdr *tcph;
#else
	struct tcphdr *tcph = (void *)iph + iph->ihl * 4;
#endif

	unsigned char *start;
	unsigned char *end;

#if ( LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,14) )
	/* fail only with broken skb, for bound iptables rule has specified protocol as TCP. */
	if (unlikely(NULL == (tcph = tp_tcp_hdr(skb))))
	{
		return 0;
	}
	start = (unsigned char *)tcph + tcph->doff * 4;
	
	if (ntohs(skb->protocol) == ETH_P_IP && iph->version == 4)
	{
		end = (unsigned char *)iph + ntohs(iph->tot_len);
	}
	else if (ntohs(skb->protocol) == ETH_P_IPV6 && ip6h->version == 6)
	{
		end = (unsigned char *)ip6h + sizeof(struct ipv6hdr) + ntohs(ip6h->payload_len);
	}
	else
	{
		return 0;
	}
#else
	start = (unsigned char *)tcph + tcph->doff * 4;
	end = start + (ntohs(iph->tot_len) - iph->ihl * 4 - tcph->doff * 4);
#endif

	if (start >= end)
	{
		if (info->type == MASSURL_TYPE_HTTP)
		{
			return 2;
		}
		else
		{
			return 0;
		}
	}

	start = url_strstr(start, end, HOST_STR);
	if (start == NULL)
	{
		return 0;
	}

	start += 8;
	end = url_strstr(start, end, "\r\n");
	
	if (end == NULL)
	{
		return 0;
	}

	if (url_strhash(start, end, info->urlIndexBits, MASSURL_TYPE_HTTP))
	{
		return (info->type == MASSURL_TYPE_HTTP) ? 1 : 2;
	}
	return 0;
}

static int match_dns(const struct sk_buff *skb, struct xt_massurl_info *info)
{
#if ( LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,14) )
	const struct udphdr *udph;
	/* fail only with broken skb, for bound iptables rule has specified protocol as UDP. */
	if (unlikely(NULL == (udph = tp_udp_hdr(skb))))
	{
		printk("fail to get udp header\n");
		return 0;
	}
#else
	const struct iphdr *iph = ip_hdr(skb);
	const struct udphdr *udph = (void *)iph + iph->ihl * 4;
#endif
	
	return url_strhash((unsigned char *)udph + sizeof(struct udphdr) + DNS_HEADER_LEN,
		(unsigned char *)udph + ntohs(udph->len),
		info->urlIndexBits,
		MASSURL_TYPE_DNS);
}

static bool match(const struct sk_buff *skb, struct xt_match_param *par)
{
	struct xt_massurl_info *info = (struct xt_massurl_info *)(par->matchinfo);
	enum ip_conntrack_info ctinfo;
	struct nf_conn *ct;
	int ret = 0;
	const struct tcphdr *tcph;

	ct = nf_ct_get(skb, &ctinfo);
	if (ct == NULL)
	{
		printk("MASSURL: get conntrak failed\n");
		return false;
	}
	
	ct->mark |= 0x40000000;

	if (info->type == MASSURL_TYPE_HTTP || info->type == MASSURL_TYPE_URL)
	{
#if ( LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,14) )
		/* fail only with broken skb, 
		 * for bound iptables rule has specified protocol as TCP when type= HTTP or URL.
		 */
		if (unlikely(NULL == (tcph = tp_tcp_hdr(skb))))
		{
			return false;
		}
#else
		tcph = (void *)ip_hdr(skb) + ip_hdr(skb)->ihl * 4;
#endif
		if (HTTP_PORT == ntohs(tcph->dest))
		{
			ret = match_http(skb, info);
		}
		/*add by fang ruilong*/
		else if (HTTPS_PORT == ntohs(tcph->dest))
		{
			ret = match_https_clientHello(skb, info);
			if (HTTPS_UNIDENTIFY == ret)
			{	
				/* 
				 * brief ret = HTTPS_UNMATCH
				 *		 HTTPS communication that matches url in blacklist may appear not to be blocked
				 *		 in blcaklist mode util DNS query occurs from client.
				 */			
				ret = HTTPS_UNMATCH;
				if (MASSURL_TYPE_HTTP == info->type) /*whitelist mode*/
				{
					ret = HTTPS_MATCH_SPEED;
				}
			}
		}
		/*add end*/
	}
	else if (info->type == MASSURL_TYPE_DNS)
	{
		ret = match_dns(skb, info);
	}

	if (1 == ret) 
	{
		/* printk("set connmark!!!!!!\n"); */
		ct->mark |= 0x80000000;
	}
	if (ret)
	{
		return true;
	}

	return false;
}


static int checkentry(const struct xt_mtchk_param *par)
{
	struct xt_massurl_info *info = (struct xt_massurl_info *)(par->matchinfo);

	if (info->type < MASSURL_TYPE_HTTP || info->type > MASSURL_TYPE_DNS)
	{
		printk(KERN_WARNING "massurl: type can only be 'http' or 'dns'\n");
		return CHECK_FAIL(-EINVAL);
	}

	return CHECK_OK;
}

static ssize_t url_list_show(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	int n = 0;
	int index;
	int first = 0;
	struct hash_url *pUrl;

	n += sprintf(buf + n, "hash    count   index   offset  urls\n");
	for (index = 0; index < URL_HASH_SIZE; index++)
	{
		first = 1;

 		hlist_for_each_entry(pUrl, &urlHash[index], node)
 		{
			if (first)
			{
				n += sprintf(buf + n, "%-6d  %-6d  ", index, urlHashCount[index]);
				first = 0;
			}
			else
			{
				n += sprintf(buf + n, "                ");
			}
			n += sprintf(buf + n, "%-6d  %-6d  %s\n", pUrl->index, pUrl->offset, pUrl->url);
 		}
	}
	return n;
}

/* Added by chz to fit the new kernel 3.10.14. 2017-03-31 */
static struct file_operations url_list_fops = {
	.owner 		= THIS_MODULE,
	.read	 	= url_list_show,
};
/* end add */

static int __init init(void)
{
	int index;
	for (index = 0; index < MASSURL_MAX_ENTRY; index++)
	{
		urls[index].index = index;
	}
	
	ipt_ctl_hook_url = setUrl;
	proc_create("url_list", 0, NULL, &url_list_fops);

	return xt_register_matches(xt_massurl_match, ARRAY_SIZE(xt_massurl_match));
}

static void __exit fini(void)
{
	ipt_ctl_hook_url = NULL;
	xt_unregister_matches(xt_massurl_match, ARRAY_SIZE(xt_massurl_match));
}

/**************************************************************************************************/
/*                                           PUBLIC_FUNCTIONS                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           GLOBAL_FUNCTIONS                                     */
/**************************************************************************************************/

module_init(init);
module_exit(fini);

MODULE_AUTHOR("Wang Wenhao <wangwenhao@tp-link.net>");
MODULE_DESCRIPTION("netfilter massurl match");
MODULE_LICENSE("GPL");


