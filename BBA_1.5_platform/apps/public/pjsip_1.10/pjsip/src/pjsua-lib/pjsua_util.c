#include <string.h>
#include <pjsua-lib/pjsua.h>
#include <pjsua-lib/pjsua_internal.h>

#include "cmsip_assert.h"

#define THIS_FILE "pjsua_util.c"
static int isIp(char* uri) ;


/* 
 * brief	ycw-pjsip. uri may be 
 *		"display" <sip:number@ip:port;uri-param>;header-param
 *		"display" <sip:number@ip:port;uri-param>
 *		"display" <sip:number@ip:port>;header-param
 *		"display" <sip:number@ip:port>
 *		sip:number@ip:port
 *		......
 *		This function only get the part--number@ip:port
 *Return 1--it is IPv4
 *			0--it is not IP
 */
static int isIp(char* uri) 
{ 
	char* ptok = NULL; 
	char* saveptr = NULL; 
	int count = 0; 
	int field = 0; 
	int isfirst = 1; 
	char* p1 = NULL; 
	char* p2 = NULL;
	int isCont = 1; 
	char buf[256]; 
	char* ps = buf; 
 
	if (!uri) return 0; 
 
	strcpy(buf, uri); 
         
	while(*ps == ' ') 
	{ 
		++ps; 
	} 

	p2 = strchr(ps, ':');
	if (p2)
	{
		*p2 = 0;
	}
         
	/*ycw-pjsip:以后要增加IPv6的验证*/ 
	if (strchr(ps, '.')) 
	{ 
		ptok = strtok_r(ps, ".", &saveptr);      
		while (ptok) 
		{ 
			p1 = ptok; 
			while (*p1 != '\0') 
			{ 
				if (*p1 < '0' || *p1 > '9') 
				{ 
					isCont = 0; 
					break; 
				} 
				++p1; 
			} 
 
			if (isCont) 
			{ 
				field = atoi(ptok); 
				if (isfirst && 0 == field) 
				{ 
					isCont = 0; 
				} 
 
				if (isCont && field > 255) 
				{ 
					isCont = 0; 
				} 
 
				if (isCont) 
				{ 
					++count; 
					isfirst = 0; 
					ptok = strtok_r(NULL, ".", &saveptr); 
				}                                
 
			} 
 
			if (!isCont) 
			{ 
				break; 
			} 
       } 
 
		if (4 == count) 
		{ 
			int dotCnt = 0; 
			strcpy(buf, uri); 
			while ((ps=strchr(ps,'.'))!=NULL) 
			{ 
				++dotCnt; 
				++ps; 
			} 
                         
			if(3==dotCnt) 
			{ 
				return 1; 
			} 
		} 
 
	} 
	return 0; 
         
} 

/*only get-- username@IP:Port*/
int pjsua_util_getOnlyUri(char* uri)
{
	char* pleft = NULL;
	char* pright = NULL;
	int len = strlen(uri);

	CMSIP_ASSERT(uri && strcmp(uri, ""));

	if ((NULL == uri) || (0 == len))
	{
		return -1;
	}

	if ((pleft=pright=strchr(uri,'@')) != NULL)
	{
		while( pleft >= uri && *pleft != ':' )
		{
			pleft--;
		}
		++pleft;

		while ((*pright != ';') && (*pright != '>') && (*pright != '?') && (*pright != '\0'))
		{
			++pright;
		}
		
		len = pright - pleft;
		memmove(uri, pleft, len);
		uri[len] = 0;		

		CMSIP_PRINT("get uri [%s]", uri);
	}
	else if ((pleft=pright=strchr(uri,':')) != NULL)
	{
		++pleft;
		++pright;

		while ((*pright != '?') && (*pright != ':') && (*pright != ';') && (*pright != '>') && (*pright != '\0'))
		{
			++pright;
		}
		len = pright - pleft;
		memmove(uri, pleft, len);
		uri[len] = 0;
		
		CMSIP_PRINT("get uri [%s]", uri);
	}
	else if (!isIp(uri))
	{
		PJ_LOG(4, (THIS_FILE, "The uri format is wrong!"));		
		return -1;
	}

	return 0;
	
}

static char* skipSipUriSchema(char * strSipUri, char *schema)
{
	pj_assert(strSipUri != NULL && schema != NULL);

	if (!strSipUri || !schema) return strSipUri;

	while(*strSipUri == ' ') ++strSipUri;

	if (!strncmp(strSipUri, schema, strlen(schema)))
	{
		strSipUri += strlen(schema);
		while (*strSipUri == ' ') ++strSipUri;
		if (*strSipUri == ':')
		{
			++strSipUri;
			while (*strSipUri == ' ') ++strSipUri;
		}
	}

	return strSipUri;

}

void pjsua_stripDelForTelUri(char* strTelUri)
{
	CMSIP_ASSERT(strTelUri!=NULL);
	CMSIP_PRINT("%s", strTelUri);

	char dels[] = {'+', '-', '.', '(', ')'};
	char tmpBuf[MAX_URI_LEN] = {0};
	char *p = strTelUri;
	char *pBuf = tmpBuf;
	int i;
	int copy = 1;

	while (*p != '\0')
	{
		copy = 1;
		
		for (i = 0; i < sizeof(dels)/sizeof(dels[0]); ++i)
		{
			if (*p == dels[i])
			{
				copy = 0;
				break;
			}
		}

		if (copy)
		{
			*pBuf = *p;
			++pBuf;
		}

		++p;
	}

	*pBuf = 0;

	strcpy(strTelUri, tmpBuf);
	CMSIP_PRINT("%s", strTelUri);

}

void pjsua_parseStrSipUri(char* strSipUri, PJSUA_STR_SIP_URI* pParsedUri )
{
	char* p = NULL;
	char* p1 = NULL;
	CMSIP_ASSERT(strSipUri!=NULL);
	CMSIP_ASSERT(pParsedUri!=NULL);
	
	strSipUri = skipSipUriSchema(strSipUri, "sips");
	strSipUri = skipSipUriSchema(strSipUri, "sip");	
	strSipUri = skipSipUriSchema(strSipUri, "tel");

	memset(pParsedUri, 0, sizeof(PJSUA_STR_SIP_URI));

	p = strchr(strSipUri, '@');
	if (p != NULL)
	{
		pParsedUri->type = PJSUA_STR_SIP_URI_NOMAL;
		memcpy(pParsedUri->username, strSipUri, p-strSipUri);
		pParsedUri->username[p-strSipUri] = 0;
		++p;
		p1 = strchr(p, ':');
		if (p1 != NULL)
		{
			memcpy(pParsedUri->host, p, p1-p);
			pParsedUri->host[p1-p] = 0;
			++p1;
			strcpy(pParsedUri->port, p1);
		}
		else
		{
			strcpy(pParsedUri->host, p);
			strcpy(pParsedUri->port, "5060");
		}
	}
	else
	{
		if (isIp(strSipUri))
		{
			pParsedUri->type = PJSUA_STR_SIP_URI_IP;
			p1 = strchr(strSipUri, ':');
			if (p1 != NULL)
			{
				memcpy(pParsedUri->host, strSipUri, p1-strSipUri);
				pParsedUri->host[p1-strSipUri] = 0;
				++p1;
				strcpy(pParsedUri->port, p1);
			}
			else
			{
				strcpy(pParsedUri->host, strSipUri);
				strcpy(pParsedUri->port, "5060");
			}
		}
		else
		{
			pParsedUri->type = PJSUA_STR_SIP_URI_UNKNOWN;
			strcpy(pParsedUri->username, strSipUri);
		}
	}
}

/* 
 * brief	check that if one ip in account host ip list is the same as the matched ip
 */
pj_bool_t pjsua_matchInAccHostIpList(pjsua_acc_id acc_id, char* matchedIp)
{
	CMSIP_PRINT("----acc(%d), matchedIp(%s)------------", acc_id, matchedIp);
	pjsua_acc *acc = &pjsua_var.acc[acc_id];
	host_ip_node *pnext = NULL;
	
	for (pnext = acc->host_ip_list; pnext!=NULL; pnext = pnext->next)
	{
		CMSIP_PRINT("host ip(%s)", pnext->host_ip);
		if (!strcmp(pnext->host_ip, matchedIp))
		{
			CMSIP_PRINT("matched");
			return PJ_TRUE;
		}
	}

	CMSIP_PRINT("not matched");
	return PJ_FALSE;
}


int pjsua_isForwardCall(pjsip_msg* msg, pj_bool_t* isIpCall)
{
	char requestUri[MAX_URI_LEN] = {0};
	char toUri[MAX_URI_LEN] = {0};
	pjsip_to_hdr* to;
	char bufAcct[MAX_URI_LEN] = {0};
	int i;
	pjsua_acc* acc = NULL;
	char boundIp[40] = {0};
	PJSUA_STR_SIP_URI strReqUri = {0};
	PJSUA_STR_SIP_URI strToUri = {0};
	PJSUA_STR_SIP_URI strAOR = {0};

	CMSIP_ASSERT(msg!=NULL);
	CMSIP_ASSERT(isIpCall!=NULL);

	*isIpCall = PJ_FALSE;

	memcpy(boundIp, pjsua_var.BoundIp.ptr, pjsua_var.BoundIp.slen);
	boundIp[pjsua_var.BoundIp.slen] = 0;

	pjsip_uri_print(PJSIP_URI_IN_REQ_URI, msg->line.req.uri, requestUri, MAX_URI_LEN);
	if (pjsua_util_getOnlyUri(requestUri) < 0)
	{
		PJ_LOG(4, (THIS_FILE, "wrong request uri"));
		return -1;
	}
	CMSIP_PRINT("---requestUri[%s]-------------", requestUri);
	pjsua_parseStrSipUri(requestUri, &strReqUri);
	CMSIP_PRINT("type[%d], user[%s], host[%s], port[%s]", strReqUri.type, strReqUri.username,
		strReqUri.host, strReqUri.port);
	if (PJSUA_STR_SIP_URI_UNKNOWN==strReqUri.type && PJSIP_URI_SCHEME_IS_TEL(msg->line.req.uri))
	{
		strReqUri.type = PJSUA_STR_SIP_URI_TEL;
		pjsua_stripDelForTelUri(strReqUri.username);
	}
	CMSIP_PRINT("type[%d], user[%s], host[%s], port[%s]", strReqUri.type, strReqUri.username,
		strReqUri.host, strReqUri.port);

	
	to = (pjsip_to_hdr*)pjsip_msg_find_hdr(msg, PJSIP_H_TO, NULL);
	if (NULL == to)
	{
		PJ_LOG(4, (THIS_FILE, "Can not find To header!"));
		return -1;
	}	
	pjsip_uri_print(PJSIP_URI_IN_FROMTO_HDR, to->uri, toUri, MAX_URI_LEN);
	if (pjsua_util_getOnlyUri(toUri) < 0)
	{
		PJ_LOG(4, (THIS_FILE, "wrong To uri"));
		return -1;
	}
	CMSIP_PRINT("----toUri[%s]------------", toUri);
	pjsua_parseStrSipUri(toUri, &strToUri);
	CMSIP_PRINT("type[%d], user[%s], host[%s], port[%s]", strToUri.type, strToUri.username,
		strToUri.host, strToUri.port);
	if (PJSUA_STR_SIP_URI_UNKNOWN==strToUri.type && PJSIP_URI_SCHEME_IS_TEL(to->uri))
	{
		strToUri.type = PJSUA_STR_SIP_URI_TEL;
		pjsua_stripDelForTelUri(strToUri.username);
	}
	CMSIP_PRINT("type[%d], user[%s], host[%s], port[%s]", strToUri.type, strToUri.username,
		strToUri.host, strToUri.port);


	if (PJSUA_STR_SIP_URI_NOMAL == strReqUri.type)
	{
		if (PJSUA_STR_SIP_URI_NOMAL == strToUri.type)
		{
			if (strcmp(strReqUri.username, strToUri.username) ||
					(strcmp(strReqUri.host, strToUri.host) && 
						strcmp(strReqUri.host, boundIp) &&
						strcmp(strToUri.host, boundIp))				||
					strcmp(strReqUri.port, strToUri.port)
				)
			{
				CMSIP_PRINT("------It is forwarded call-----------");
				return 1;
			}
		}
		else if (PJSUA_STR_SIP_URI_TEL == strToUri.type)
		{
			int reqlen = strlen(strReqUri.username);
			int tolen = strlen(strToUri.username);

			if (reqlen <= tolen)
			{
				if (strcmp(strReqUri.username, &strToUri.username[tolen-reqlen]))
				{
					CMSIP_PRINT("----It is forward call------------");
					return 1;
				}
			}
			else
			{
				if (strcmp(&strReqUri.username[reqlen-tolen], strToUri.username))
				{
					CMSIP_PRINT("-----It is forward call------------");
					return 1;
				}
			}
			
		}
		else
		{
			/*ycw-pjsip. May be the forward destination account is in the same IAD*/
			#if 0 
			if (strcmp(strReqUri.host, boundIp))
			{
				CMSIP_PRINT("------It is forwarded call-----------");
				return 1;
			}
			#else
			CMSIP_PRINT("------It is forwarded call-----------");
			return 1;
			#endif
		}
	}
	else if (PJSUA_STR_SIP_URI_IP == strReqUri.type)
	{
		*isIpCall = PJ_TRUE;
		
		if (PJSUA_STR_SIP_URI_NOMAL == strToUri.type)
		{
			for (i = 0; i < pjsua_var.acc_cnt; ++i)
			{
				acc = &pjsua_var.acc[i];
				if (acc->valid
#	if defined(SUPPORT_ACCOUNT_RTT) && SUPPORT_ACCOUNT_RTT!=0
					&& acc->regDuration > 0
#	else
					&& acc->regOK
#	endif
					)
				{
					memcpy(bufAcct, acc->cfg.id.ptr, acc->cfg.id.slen);
					bufAcct[acc->cfg.id.slen] = 0;
					if (pjsua_util_getOnlyUri(bufAcct) < 0)
					{
						PJ_LOG(4, (THIS_FILE, "Wrong account(%d) id", i));
						return -1;
					}
					pjsua_parseStrSipUri(bufAcct, &strAOR);

					CMSIP_ASSERT(strAOR.type==PJSUA_STR_SIP_URI_NOMAL);

					if (0==strcmp(strAOR.username, strToUri.username) &&
							(0==strcmp(strAOR.host, strToUri.host) ||
								pjsua_matchInAccHostIpList(i, strToUri.host) ||
								0==strcmp(strToUri.host, boundIp)) &&
						 0==strcmp(strAOR.port, strToUri.port))
					{
						break;
					}
				}
			}

			if (i >= pjsua_var.acc_cnt)
			{
				return 1;
			}
		}
		else if (PJSUA_STR_SIP_URI_TEL == strToUri.type)
		{
			int idlen, tolen;
			for (i = 0; i < pjsua_var.acc_cnt; ++i)
			{
				acc = &pjsua_var.acc[i];
				if (acc->valid
#	if defined(SUPPORT_ACCOUNT_RTT) && SUPPORT_ACCOUNT_RTT!=0
					&& acc->regDuration > 0
#	else
					&& acc->regOK
#	endif
					)
				{
					memcpy(bufAcct, acc->cfg.id.ptr, acc->cfg.id.slen);
					bufAcct[acc->cfg.id.slen] = 0;
					if (pjsua_util_getOnlyUri(bufAcct) < 0)
					{
						PJ_LOG(4, (THIS_FILE, "Wrong account(%d) id", i));
						return -1;
					}
					pjsua_parseStrSipUri(bufAcct, &strAOR);

					CMSIP_ASSERT(strAOR.type==PJSUA_STR_SIP_URI_NOMAL);

					idlen = strlen(strAOR.username);
					tolen = strlen(strToUri.username);
					if (idlen <= tolen)
					{
						if (0 == strcmp(strAOR.username, &strToUri.username[tolen-idlen]))
						{
							break;
						}
					}
				}
			}
			
			if (i >= pjsua_var.acc_cnt)
			{
				return 1;
			}

		}
		else if (PJSUA_STR_SIP_URI_IP == strToUri.type)
		{
			if (strcmp(strReqUri.host, strToUri.host) || strcmp(strReqUri.port, strToUri.port))
			{
				return 1;
			}
		}
		else
		{
			return 1;
		}
	}
	else if (PJSUA_STR_SIP_URI_TEL == strReqUri.type)
	{
		int reqlen, tolen;
		if (PJSUA_STR_SIP_URI_NOMAL == strToUri.type)
		{
			reqlen = strlen(strReqUri.username);
			tolen = strlen(strToUri.username);

			if (tolen > reqlen || strcmp(&strReqUri.username[reqlen-tolen], strToUri.username))
			{
				return 1;
			}
		}
		else if (PJSUA_STR_SIP_URI_TEL == strToUri.type)
		{
			if (strcmp(strReqUri.username, strToUri.username))
			{
				return 1;
			}
		}
		else
		{
			return 1;
		}
			
	}
	else
	{
		if (PJSUA_STR_SIP_URI_UNKNOWN == strToUri.type)
		{
			if (strcmp(strReqUri.username, strToUri.username))
			{
				return 1;
			}
		}
		else
		{
			return 1;
		}
	}
	
	return 0;
}


