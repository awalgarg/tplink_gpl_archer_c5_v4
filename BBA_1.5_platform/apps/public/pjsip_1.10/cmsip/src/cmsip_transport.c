/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		cmsip_transport.c
 * brief		This file implements the function which PJSIP communicates with CM.
 * details	
 *
 * author	Yu Chuwei
 * version	1.0.0
 * date		22Sept11
 *
 * warning	
 *
 * history \arg	
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <string.h>
#include "cmsip_transport.h"
#include <stdio.h>
#include <stdarg.h>
#include "os_log.h"

/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/

/* 
 * brief	This macro is a string, the name of this file. When we call PJ_LOG function in the 
 *			functions of this file, we must pass this macro to PJ_LOG.
 */
#define THIS_FILE "cmsip_transport.c"


/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/

/* 
 * brief	The max connect times for local unix socket to connect CM's unix server sokcet.
 */
#define SOCK_MAX_CONNECT_TIMES 100

/* 
 * brief	When the local unix socket can not connect to CM's unix server socket, it will wait a
 *			moment and then try to connect again. 
 *			This macro's value is the time between the first time to connect and the second time to
 *			connect.
 */
#define INIT_WAIT_TIME 100

/**************************************************************************************************/
/*                                           EXTERN_PROTOTYPES                                    */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           LOCAL_PROTOTYPES                                     */
/**************************************************************************************************/
/* 
 * fn		static void getSipUri(char* dst, char* src, int size)
 * brief	Get the "user@host" part of a sip URI.
 * details	
 *
 * param[in]	src	the SIP URI
 *	param[in]	size	size of dst
 * param[out]	dst	Buffer to store the result
 *
 * return	void
 * retval	
 *
 * note		
 */
static void getSipUri(char* dst, char* src, int size);

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/

/* 
 * brief	The socket which connect to CM.
 */
int g_cmsip_cliSockfd = CMSIP_UNKNOWN;

/**************************************************************************************************/
/*                                           LOCAL_FUNCTIONS                                      */
/**************************************************************************************************/

/* 
 * fn		static void getSipUri(char* dst, char* src, int size)
 * brief	Get the "user@host" part of a sip URI.
 * details	
 *
 * param[in]	src	the SIP URI
 *	param[in]	size	size of dst
 * param[out]	dst	Buffer to store the result
 *
 * return	void
 * retval	
 *
 * note		
 */
static void getSipUri(char* dst, char* src, int size)
{
	char *p1 = NULL;
	char *p2 = NULL;

	CMSIP_ASSERT(dst!=NULL && src!=NULL);	

	/*here, I only process "sip" or "sips" uri*/
	/*ycw-pjsip-schema*/
	if (((p1 = strstr(src, "sip:"))!= NULL) ||
	 		((p1 = strstr(src, "sips:"))!= NULL))
	{
		/*may be sip:number@host, sip:host, ... and so on*/
		p2 = strchr(p1, ':');
		/*ycw-pjsip:has other end character???*/
		while (*p2 != ';' && *p2 != '?' && *p2 != '>' && *p2 != '\0')
		{
			p2++;
		}
		
		memcpy(dst, p1, p2 - p1);
		dst[p2 - p1] = 0;			
	}	 
	else
	{
		int len = strlen(src) + 1; 
		int min = (size > len) ? len : size;
		memcpy(dst, src, min);
		dst[min] = 0;
	}
}

/**************************************************************************************************/
/*                                           PUBLIC_FUNCTIONS                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           GLOBAL_FUNCTIONS                                     */
/**************************************************************************************************/

/* 
 * fn		extern int cmsip_sockCreate(int* pSockfd, const char* strSrvAddress)
 * brief	Create a TCP domain socket and connect to CM.
 * details	
 *
 * param[in]	srvAddr	A file path, is the server socket's address.
 * param[out]	pSockfd	the socket created in this function.
 *
 * return	int
 * retval	>=0	the socket created.
 *	retval	-1		error happen.
 * note		
 */
int cmsip_sockCreate(int *pSockfd, const char *strSrvAddress)
{
	int i;
	int ret;
	struct sockaddr_un srvAddr;
	int len;
	int sockfd = socket(PF_UNIX, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		return -1;
	}

	srvAddr.sun_family = PF_UNIX;
	strcpy(srvAddr.sun_path, strSrvAddress);
	len = sizeof(srvAddr);

	for (i = 0; i < SOCK_MAX_CONNECT_TIMES; ++i)
	{
		ret = connect(sockfd, (struct sockaddr*)&srvAddr, len);
		if (ret < 0)
		{
			CMSIP_PRINT("connect CM: counter[%d] fail, wait [%d]ms and retry", 
								i+1, (i+1) * INIT_WAIT_TIME);
			if (EINTR == errno)
			{
				continue;
			}
			
			if ( ETIMEDOUT==errno || EAGAIN==errno)
			{
				usleep(SOCK_MAX_CONNECT_TIMES * (i + 1));
				continue;
			}
		}
		
		break;		
	}

	if (ret < 0)
	{
		CMSIP_PRINT("try to connect CM for [%d] times,still fail, exit", SOCK_MAX_CONNECT_TIMES);
		return -1;
	}

	*pSockfd = sockfd;

	CMSIP_PRINT("connect CM successful!");

	return 0;
}


/* 
 * fn		extern int cmsip_sockRecv(int sockfd, char* buf, int size)
 * brief	Receive the data from remote party.
 * details	
 *
 * param[in]	sockfd	the socket to receive data.
 *	param[in]	buf		the buffer to stored the data received from remote party.
 *	param[in]	size		the size of the data received.
 * param[out]	none
 *
 * return	int
 * retval	0				receive successfully.
 *	retval	-1				receive failed.
 *
 * note		
 */
int cmsip_sockRecv(int sockfd, char* buf, int size)
{
	/*由于一定要接收到指定的字节数，所以假如有一个数据包发送不完整，后续数据包肯定都会出错。
	*	不过让人欣慰的是，根据linux关于unix socket的说明，发送的数据包是不会丢的。
	*/
	int sum = 0;	
	int ret;

	while (sum < size)
	{
		ret = recv(sockfd, &buf[sum], size - sum, 0);
		if (ret < 0)
		{
			if (EINTR == errno ) 
			{
				continue;
			}
			
			return -1;
		}
		else
		{
			sum += ret;
		}
	}

	return 0;
}

/* 
 * fn		extern int cmsip_sockSend(int sockfd, char* buf, int size)
 * brief	Send the data to destination.
 * details	
 *
 * param[in]	sockfd	the socket to transfer data.
 * param[in]	buf		the buffer stored the data which will be sent.
 *	param[in]	size		the size of the data to be sent.
 * param[out]	none
 *
 * return	int
 * retval	0				send successfully
 *	retval	-1				send failed
 * note		
 */
int cmsip_sockSend(int sockfd, char* buf, int size)
{
	int sum = 0;
	int ret;
	while (sum < size)
	{
		ret = send(sockfd, &buf[sum], size - sum, 0);
		if (ret < 0)
		{
			/*This API is called in multi-threads, but you must remember that
			errno is thread-safe!!! yuchuwei*/
			if (EINTR == errno) 
			{
				continue;
			}

			if (ENOBUFS == errno || ENOMEM == errno)
			{
				usleep(100);
				continue;
			}
			
			return -1;
		}

		sum += ret;
	}

	return 0;
}

/* 
 * fn		extern int cmsip_sockClose(int sockfd)
 * brief	Close a socket really.
 * details	
 *
 * param[in]	sockfd	the socket to be close.
 * param[out]	none
 *
 * return	int 
 * retval	0	success
 *	retval	-1 failure
 * note		
 */
int cmsip_sockClose(int* sockfd)
{		
	int ret; 
	ret = shutdown(*sockfd, SHUT_RDWR);
	*sockfd = -1;
	return (ret);
}

/* 
 * fn		extern int cmsip_send_acc_regStatus(int accId, CMSIP_REG_STATE status, int duration)
 * brief	Send CMSIP_RESPONSE_SIP_REGSTATUS response to CM.
 * details	
 *
 * param[in]	accId		Index of the account.
 *	param[in]	status	Result of the account registration.
 *	param[in]	duration	The interval from PJSIP sended REGISTER request to PJSIP received the
 *					first response.
 * param[out]	none
 *
 * return	int
 * retval	0		success
 *	retval	-1		failure
 *
 * note		
 */
int cmsip_send_acc_regStatus(int accId, CMSIP_REG_STATE status
#	if defined(SUPPORT_ACCOUNT_RTT) && SUPPORT_ACCOUNT_RTT!=0
	, int duration
#	endif
#ifdef INCLUDE_TFC_ES
	, char *date /*By YuChuwei, For Telefonica*/
#endif
	)
{
	CMSIP_MSG cmSipMsg;
	CMSIP_SIP_REGSTATUS *pRegStat = NULL;
	int ret;

	cmSipMsg.type = CMSIP_RESPONSE_SIP_REGSTATUS;

	pRegStat = (CMSIP_SIP_REGSTATUS*)cmSipMsg.body;
	pRegStat->accId = accId;
	pRegStat->state = status;
#	if defined(SUPPORT_ACCOUNT_RTT) && SUPPORT_ACCOUNT_RTT!=0
	pRegStat->duration = duration;
#	endif
#ifdef INCLUDE_TFC_ES
	strcpy(pRegStat->date, date);
#endif

	#ifdef CMSIP_DEBUG
	printf("----%s,%d---send CMSIP_SIP_REGSTATUS---\n", __FUNCTION__, __LINE__);
	printf("acc Index %d\n", pRegStat->accId);
	printf("reg result %s\n", (pRegStat->state == CMSIP_REG_STATE_SUCCESS) ? "success" : "failure");
#	if defined(SUPPORT_ACCOUNT_RTT) && SUPPORT_ACCOUNT_RTT!=0
	printf("rtt %d\n", pRegStat->duration);
#	endif
#ifdef INCLUDE_TFC_ES
	printf(" date %s\n", pRegStat->date);
#endif
	#endif

	ret = cmsip_sockSend(g_cmsip_cliSockfd, (char *)&cmSipMsg, sizeof(cmSipMsg));
	if (ret < 0)
	{
		#ifdef CMSIP_DEBUG
		printf("%s,%d: can not send CMSIP_SIP_REGSTATUS\n", __FUNCTION__, __LINE__);
		#endif
		return -1;
	}

	return 0;
}

/* 
 * fn		extern int cmsip_send_mwistatus(int accId, CMSIP_MWI_RESULT status)
 * brief	Send a account's MWI status to CM
 * details	
 *
 * param[in]	accId	Index of the account
 *	param[in]	status	Status of the account MWI
 * param[out]	none
 *
 * return	int
 * retval	0		success
 *	retval	-1		failure
 *
 * note		
 */
int cmsip_send_mwistatus(int accId, CMSIP_MWI_RESULT status)
{
	CMSIP_MSG cmSipMsg;
	CMSIP_SIP_MWISTATUS *pmwiStatus = NULL;
	int ret;

	cmSipMsg.type = CMSIP_RESPONSE_SIP_MWISTATUS;

	pmwiStatus = (CMSIP_SIP_MWISTATUS*)cmSipMsg.body;
	pmwiStatus->accId = accId;
	pmwiStatus->result = status;

	#ifdef CMSIP_DEBUG
	printf("-------%s,%d----------------\n", __FUNCTION__, __LINE__);
	printf("account Index %d\n", pmwiStatus->accId);
	printf("result %s\n", (pmwiStatus->result == CMSIP_MWI_SUCCESS) ? "success" : "terminate");
	#endif

	ret = cmsip_sockSend(g_cmsip_cliSockfd, (char *)&cmSipMsg, sizeof(cmSipMsg));
	if (ret < 0)
	{
		#ifdef CMSIP_DEBUG
		printf("%s,%d: can not send CMSIP_SIP_MWISTATUS\n", __FUNCTION__, __LINE__);
		#endif

		return -1;
	}

	return 0;
	
}

/* 
 * fn		extern int cmsip_send_callStatus(int callIndex, int channelId, int statusCode,
 *																	char contact[MAX_CONTACT_URI][MAX_URI_LEN], int cnt)
 * brief	Send the response of the outgoing INVITE request to CM
 * details	
 *
 * param[in]	callIndex	Index of the call which is related with the outgoing INVITE request.
 *	param[in]	channelId	The data used by CM to look for related call.
 *	param[in]	statusCode	The status code of the response.
 *	param[in]	contact		The URI in the contact header,may be one or more.
 *	param[in]	cnt			The number of the contact URI.
 * param[out]	none
 *
 * return	int
 * retval	0		success
 *	retval	-1		failure
 *
 * note		
 */
int cmsip_send_callStatus(CMSIP_CALL_TYPE type, int callIndex, int seq, int statusCode,
														char contact[][MAX_URI_LEN], int cnt)
{
	CMSIP_MSG cmSipMsg;
	cmSipMsg.type = CMSIP_RESPONSE_SIP_CALLSTATUS;

	CMSIP_SIP_CALLSTATUS *pcallStatus = (CMSIP_SIP_CALLSTATUS*)cmSipMsg.body;
	pcallStatus->type = type;
	pcallStatus->callIndex = callIndex;
	pcallStatus->seq = seq;
	pcallStatus->statusCode = statusCode;
	memcpy(pcallStatus->contact, contact, cnt * MAX_URI_LEN);	

	#ifdef CMSIP_DEBUG
	printf("-------------%s,%s,%d-------\n", __FILE__, __FUNCTION__, __LINE__);
	printf("call index %d\n", pcallStatus->callIndex);
	printf("seq %d\n", pcallStatus->seq);
	printf("status code %d\n", pcallStatus->statusCode);
	printf("contact %s\n", pcallStatus->contact[0]);
	#endif

	int ret = cmsip_sockSend(g_cmsip_cliSockfd, (char*)&cmSipMsg, sizeof(cmSipMsg));
	if (ret < 0)
	{
		#ifdef CMSIP_DEBUG
		printf("%s,%d: can not send CMSIP_SIP_CALLSTATUS\n", __FUNCTION__, __LINE__);
		#endif
		return -1;
	}				

	return 0;

}

/* 
 * fn		extern int cmsip_send_callRelease(CMSIP_REQUEST type, unsigned callIndex)
 * brief	Send Call Release request to CM.
 * details	
 *
 * param[in]	type	Call Type
 *				callIndex	Index of the call which will be released
 * param[out]	none
 *
 * return	int
 * retval	0	success
 *	retval	-1	failure
 *
 * note		
 */
int cmsip_send_callRelease(CMSIP_REQUEST type, unsigned callIndex)
{
	CMSIP_MSG cmSipMsg;
	cmSipMsg.type = type;
	cmSipMsg.type |= CMSIP_MESSAGE_REQUEST;

	CMSIP_SIP_CALLRELEASE *pcallRelease = (CMSIP_SIP_CALLRELEASE*)cmSipMsg.body;
	pcallRelease->callIndex = callIndex;
	#ifdef CMSIP_DEBUG
	printf("-----------%s,%s,%d---------\n", __FILE__, __FUNCTION__, __LINE__);
	printf("call index %d\n", pcallRelease->callIndex);
	#endif

	int ret = cmsip_sockSend(g_cmsip_cliSockfd, (char*)&cmSipMsg, sizeof(cmSipMsg));
	if (ret < 0)
	{
		#ifdef CMSIP_DEBUG
		printf("%s,%d: can not send CMSIP_SIP_CALLRELEASE\n", __FUNCTION__, __LINE__);
		#endif
		return -1;
	}

	return 0;	

}

/* 
 * fn		extern int cmsip_send_autocall_forward(int callIndex, int oldCallIndex, 
 *																				char* reqUri, char* to)
 * brief	Report the call forward by the third party.
 * details	There are three IAD, we call them A, B and C. Our IAD is A. First, our IAD call
 *				B, then forwarded to C. After our IAD send INVITE request to C, it will call this
 *				function to send CMSIP_REQUEST_SIP_AUTOCALLFORWARD to CM.
 *
 * param[in]	callIndex	Index of the forwarded call.
 *	param[in]	oldCallIndex	Index of previous call which is forwarded.
 *	param[in]	reqUri		URI of the new call's destination.
 *	param[in]	to				URI of To header of previous call which is forwarded.
 * param[out]	none
 *
 * return	int
 * retval	0	success
 *	retval	-1 failure
 * note		
 */
int cmsip_send_autocall_forward(int callIndex, int oldCallIndex,
															char* reqUri, char* to)
{
	int min;
	int ret;
	int reqUriLen;
	int toLen;

	CMSIP_ASSERT(reqUri!=NULL);
	CMSIP_ASSERT(to!=NULL);
	
	reqUriLen = strlen(reqUri);
	toLen = strlen(to);
	
	CMSIP_MSG cmSipMsg;	
	cmSipMsg.type = CMSIP_REQUEST_SIP_AUTOCALLFORWARD;
	cmSipMsg.type |= CMSIP_MESSAGE_REQUEST;

	CMSIP_SIP_AUTOCALLINFO *pautoCall = (CMSIP_SIP_AUTOCALLINFO*)cmSipMsg.body;
	pautoCall->callIndex = callIndex;
	pautoCall->oldCallIndex = oldCallIndex;
	
	min = (reqUriLen < MAX_URI_LEN) ? reqUriLen : MAX_URI_LEN;
	memcpy(pautoCall->dst, reqUri, min);
	pautoCall->dst[min] = 0;
	
	min = (toLen < MAX_URI_LEN) ? toLen : MAX_URI_LEN;
	memcpy(pautoCall->to, to, min);
	pautoCall->to[min] = 0;
 	
	#ifdef CMSIP_DEBUG
	printf("----------send CMSIP_SIP_AUTOCALLINFO(forward)\n");
	printf("call index %d\n", pautoCall->callIndex);
	printf("channel Id %d\n", pautoCall->oldCallIndex);
	printf("req uri %s\n", pautoCall->dst);
	printf("to %s\n", pautoCall->to);
	#endif
	ret = cmsip_sockSend(g_cmsip_cliSockfd, (char *)&cmSipMsg, sizeof(cmSipMsg));
	if (ret < 0)
	{
		#ifdef CMSIP_DEBUG
		printf("%s,%d: can not send CMSIP_SIP_AUTOCALLINFO\n", __FUNCTION__, __LINE__);
		#endif
		return -1;
	}

	return 0;
}

/* 
 * fn		extern int cmsip_send_incallack(int callIndex)
 * brief	Send incoming ACK request to CM
 * details	
 *
 * param[in]	callIndex	Index of the call realted to ACK
 * param[out]	none
 *
 * return	int
 * retval	0	success
 *	retval	-1	failure
 *
 * note		
 */
int cmsip_send_incallack(int callIndex)
{
	CMSIP_MSG cmSipMsg;
	CMSIP_SIP_INCALL_ACK *pAck = NULL;
	int ret;

	cmSipMsg.type = CMSIP_REQUEST_SIP_INCOMINGCALL_ACK;
	cmSipMsg.type |= CMSIP_MESSAGE_REQUEST;

	pAck = (CMSIP_SIP_INCALL_ACK*)cmSipMsg.body;
	pAck->callIndex = callIndex;

	#ifdef CMSIP_DEBUG
	printf("-----%s,%d--send CMSIP_SIP_INCALL_ACK-\n", __FUNCTION__, __LINE__);
	printf("call index %d\n", pAck->callIndex);
	#endif

	ret = cmsip_sockSend(g_cmsip_cliSockfd, (char *)&cmSipMsg, sizeof(cmSipMsg));
	if (ret < 0)
	{
		#ifdef CMSIP_DEBUG
		printf("%s,%d: can not send CMSIP_SIP_INCALL_ACK\n", __FUNCTION__, __LINE__);
		#endif
		return -1;
	}

	return 0;
}

/* 
 * fn		extern int cmsip_send_holdstatus(int callIndex, CMSIP_HOLD_RESULT result)
 * brief	Send the response of the outgoing hold request to CM.
 * details	
 *
 * param[in]	callIndex	Index of the call be held.
 *	param[in]	result		Result of hold
 * param[out]	none
 *
 * return	int
 * retval	0	success
 *	retval	-1	failure
 *
 * note		
 */
int cmsip_send_holdstatus(int callIndex, CMSIP_HOLD_RESULT result)
{
	CMSIP_MSG cmSipMsg;
	CMSIP_SIP_HOLDSTATUS *pHoldStat = NULL;
	int ret;

	cmSipMsg.type = CMSIP_RESPONSE_SIP_HOLDSTATUS;

	pHoldStat = (CMSIP_SIP_HOLDSTATUS*)cmSipMsg.body;
	pHoldStat->callIndex = callIndex;
	pHoldStat->result = result;

	#ifdef CMSIP_DEBUG
	printf("------%s,%d-----send CMSIP_SIP_HOLDSTATUS-\n", __FUNCTION__, __LINE__);
	printf("call index %d\n", pHoldStat->callIndex);
	printf("hold %s\n", (pHoldStat->result == CMSIP_HOLD_SUCCESS) ? "success" : "failure");
	#endif

	ret = cmsip_sockSend(g_cmsip_cliSockfd, (char *)&cmSipMsg, sizeof(cmSipMsg));
	if (ret < 0)
	{
		#ifdef CMSIP_DEBUG
		printf("%s,%d: can not send CMSIP_SIP_HOLDSTATUS\n", __FUNCTION__, __LINE__);
		#endif
		return -1;
	}

	return 0;

}

/* 
 * fn		extern int cmsip_send_rtpcreate(int callIndex,
 *								CMSIP_MEDIA_TYPE	mediaType, 
 *								CMSIP_MEDIA_TPPROTO	mediaProtocol,
 *								CMSIP_MEDIA_DIR	dir, 
 *								char*	codecName, int codecNameLen,
 *								int clockRate, int channelCnt,
 *								int ulptime,
 *								int dlptime,
 *								int txVoicePayloadType,
 *								int rxVoicePayloadType,
 *								int txDTMFRelayPayloadType,
 *								int rxDTMFRelayPayloadType,
 *								unsigned faxMaxDatagram,
 *								unsigned faxMaxBitRate,
 *								int enableUdpEc,
 *								char* localIpAddr, int localIpLen,
 *								unsigned short localDataPort,
 *								char* remoteIpAddr, int remoteIpLen,
 *								unsigned short remoteDataPort,
 *								unsigned short remoteControlPort)
 * brief	Send RTP create request to CM.
 * details	
 *
 * param[in]	callIndex	Index of the call to create RTP/RTCP transport.
 *	param[in]	mediaType	Type of media(audio/video/image...)
 *	param[in]	mediaProtocol	Protocol of media(RTP/AVP,SRTP/AVP,...)
 *	param[in]	dir			Direction of the media stream.
 *	param[in]	codecName	Name of the codec used by DSP to encoder or decoder the media stream.
 *	param[in]	codecNameLen	Length of the codec name.
 *	param[in]	clockRate	Clock rate of the codec.
 *	param[in]	channelCnt	Number of the channel of the codec.
 *	param[in]	ulptime		encode Packetization Time of the codec.
 *	param[in]	dlptime		decode Packetization Time of the codec
 * param[in]	txVoicePayloadType	Payload type of out rtp packet.
 *	param[in]	rxVoicePayloadType	Payload type of in rtp packet.
 * param[in]	txDTMFRelayPayloadType Payload type of out DTMF rtp packet.
 *	param[in]	rxDTMFRelayPayloadType	Payload type of in DTMF rtp packet.
 *	param[in]	faxMaxDatagram	The length of the max FAX UDP packet.
 *	param[in]	faxMaxBitRate	The max bit rate of FAX.
 *	param[in]	enableUdpEc	Enable UDP Error Correct or not.
 *	param[in]	localIpAddr	The IP which DSP socket bind with.
 *	param[in]	localIpLen	The length of the string localIpAddr.
 *	param[in]	localDataPort	The port which DSP socket bind with.
 *	param[in]	remoteIpAddr	The destination Address of the DSP socket.
 * param[in]	remoteIpLen		The length of the string remoteIpAddr.
 *	param[in]	remoteDataPort	The destination Port of the DSP socket.
 *	param[in]	remoteControlPort	The destination RTCP Port of the DSP socket.
 * param[out]	none
 *
 * return	int
 * retval	0	success
 *			  -1	failure
 * note		DSP socket ---- related to this call, to send audio/video data which DSP generate.
 */
int cmsip_send_rtpcreate(int callIndex,
					CMSIP_MEDIA_TYPE	mediaType, 
					CMSIP_MEDIA_TPPROTO	mediaProtocol,
					CMSIP_MEDIA_DIR	dir, 
					char*	codecName, int codecNameLen,
					int clockRate, int channelCnt,
					int ulptime,
					int dlptime,
					int txVoicePayloadType,
					int rxVoicePayloadType,
					int txDTMFRelayPayloadType,
					int rxDTMFRelayPayloadType,
					unsigned faxMaxDatagram,
					unsigned faxMaxBitRate,
					int enableUdpEc,
					char* localIpAddr, int localIpLen,
					unsigned short localDataPort,
					char* remoteIpAddr, int remoteIpLen,
					unsigned short remoteDataPort,
					unsigned short remoteControlPort
					)
{
	CMSIP_MSG	cmSipMsg;
	CMSIP_SIP_RTPCREATE *pRtpCreate = NULL;
	int ret;

#if !defined(INCLUDE_TFC_ES)
	CMSIP_ASSERT(codecName!=NULL);
	CMSIP_ASSERT(localIpAddr!=NULL);
	CMSIP_ASSERT(remoteIpAddr!=NULL);
	CMSIP_ASSERT(codecNameLen >= 0 && codecNameLen < CMSIP_MAX_CODEC_NAME_LEN);
	CMSIP_ASSERT(remoteIpLen >= 0 && remoteIpLen < CMSIP_STR_64);
	CMSIP_ASSERT(localIpLen >= 0 && localIpLen < CMSIP_STR_64);
#endif

	cmSipMsg.type = CMSIP_REQUEST_SIP_RTPCREATE;
	cmSipMsg.type |= CMSIP_MESSAGE_REQUEST;
	pRtpCreate = (CMSIP_SIP_RTPCREATE*)cmSipMsg.body;
	pRtpCreate->callIndex = callIndex;
	pRtpCreate->mediaType = mediaType;
	pRtpCreate->mediaProtocol = mediaProtocol;
	pRtpCreate->dir = dir;
	memcpy(pRtpCreate->codec, codecName, codecNameLen);
	pRtpCreate->codec[codecNameLen] = 0;
	pRtpCreate->clockRate = clockRate;
	pRtpCreate->channelCnt = channelCnt;
	pRtpCreate->ulptime = ulptime;
	pRtpCreate->dlptime = dlptime;
	pRtpCreate->txVoicePayloadType = txVoicePayloadType;
	pRtpCreate->rxVoicePayloadType = rxVoicePayloadType;
	pRtpCreate->txDTMFRelayPayloadType = txDTMFRelayPayloadType;
	pRtpCreate->rxDTMFRelayPayloadType = rxDTMFRelayPayloadType;
	pRtpCreate->faxMaxDatagram	= faxMaxDatagram;
	pRtpCreate->faxMaxBitRate	= faxMaxBitRate;
	pRtpCreate->faxEnableUdpEc	= enableUdpEc;
	memcpy(pRtpCreate->remoteIpAddr, remoteIpAddr, remoteIpLen);
	pRtpCreate->remoteIpAddr[remoteIpLen] = 0;
	pRtpCreate->remoteDataPort = remoteDataPort;
	pRtpCreate->remoteControlPort = remoteControlPort;
	memcpy(pRtpCreate->localIpAddr, localIpAddr, localIpLen);
	pRtpCreate->localIpAddr[localIpLen] = 0;
	pRtpCreate->localDataPort = localDataPort;

	#ifdef CMSIP_DEBUG
	printf("------------%s,%d-----------\n", __FUNCTION__, __LINE__);
	printf("call index %d\n", pRtpCreate->callIndex);
	printf("media type %d\n", pRtpCreate->mediaType);
	printf("media protocol %d\n", pRtpCreate->mediaProtocol);
	printf("media direction %d\n", pRtpCreate->dir);
	printf("codec name %s\n", pRtpCreate->codec);
	printf("clock rate %d\n", pRtpCreate->clockRate);
	printf("channel count %d\n", pRtpCreate->channelCnt);
	printf("ulptime %d\n", pRtpCreate->ulptime);
	printf("dlptime %d\n", pRtpCreate->dlptime);
	printf("voice pt(tx[%d],rx[%d])\n", pRtpCreate->txVoicePayloadType, pRtpCreate->rxVoicePayloadType);
	printf("DTMF pt:%d(tx), %d(rx)\n", pRtpCreate->txDTMFRelayPayloadType, 
					pRtpCreate->rxDTMFRelayPayloadType);
	printf("T.38 max datagram[%d], max BitRate[%d], enableUdpEc[%d]\n", 
					pRtpCreate->faxMaxDatagram, pRtpCreate->faxMaxBitRate, pRtpCreate->faxEnableUdpEc);
	printf("local IP %s, port %d\n", pRtpCreate->localIpAddr, pRtpCreate->localDataPort);
	printf("remote IP %s, port %d, rtcp port %d\n", pRtpCreate->remoteIpAddr,
					pRtpCreate->remoteDataPort, pRtpCreate->remoteControlPort);
	#endif

	ret = cmsip_sockSend(g_cmsip_cliSockfd, (char *)&cmSipMsg, sizeof(cmSipMsg));
	if (ret < 0)
	{
		#ifdef CMSIP_DEBUG
		printf("%s,%d: can not send CMSIP_SIP_RTPCREATE\n", __FUNCTION__, __LINE__);
		#endif
		return -1;
	}

	return 0;
	
}

/* 
 * fn		extern int cmsip_send_rtpdestroy(int callIndex)
 * brief	Send RTP Destroy request to CM.
 * details	
 *
 * param[in]	callIndex	Index of the call whose media transport will be destroyed.
 * param[out]	none
 *
 * return	int
 * retval	0	success
 *	retval  -1	failure
 *
 * note		
 */
int cmsip_send_rtpdestroy(int callIndex)
{
	CMSIP_MSG cmSipMsg;
	CMSIP_SIP_RTPDESTROY *pRtpDestroy = NULL;
	int ret;

	cmSipMsg.type = CMSIP_REQUEST_SIP_RTPDESTROY;
	cmSipMsg.type |= CMSIP_MESSAGE_REQUEST;

	pRtpDestroy = (CMSIP_SIP_RTPDESTROY*)cmSipMsg.body;
	pRtpDestroy->callIndex = callIndex;

	#ifdef CMSIP_DEBUG
	printf("-------------%s,%d--------------------\n", __FUNCTION__, __LINE__);
	printf("rtp destroy: call index %d\n", pRtpDestroy->callIndex);
	#endif

	ret = cmsip_sockSend(g_cmsip_cliSockfd, (char *)&cmSipMsg, sizeof(cmSipMsg));
	if (ret < 0)
	{
		#ifdef CMSIP_DEBUG
		printf("%s,%d: can not send CMSIP_SIP_RTPDESTROY\n", __FUNCTION__, __LINE__);
		#endif

		return -1;
	}

	return 0;
}

/* 
 * fn		extern int cmsip_send_autocall_transfer(int callIndex, int oldCallIndex,
 *																		char* reqUri, char* referBy)
 * brief	Send CMSIP_REQUEST_SIP_AUTOCALLXFER request to CM.
 * details	
 *
 * param[in]	callIndex	Index of the new call to the target.
 * param[in]	oldCallIndex	Index of the old call, which is transfered by transferer.
 *	param[in]	reqUri	URI to call the target.
 * param[in]	referBy	URI of transferer.
 * param[out]	none
 *
 * return	int
 * retval	0	success
 *			  -1 failure
 *
 * note		
 */
int cmsip_send_autocall_transfer(int accId, int callIndex, int oldCallIndex,
											char* reqUri, char* referBy, int status)
{
	 CMSIP_MSG cmSipMsg;
	 CMSIP_SIP_AUTOCALLINFO *pautoCall = NULL;	
	 int ret;

	 
	 cmSipMsg.type = CMSIP_REQUEST_SIP_AUTOCALLXFER;
	 cmSipMsg.type |= CMSIP_MESSAGE_REQUEST;

	 pautoCall = (CMSIP_SIP_AUTOCALLINFO*)cmSipMsg.body;
	 pautoCall->accId = accId;
	 pautoCall->callIndex = callIndex;
	 pautoCall->oldCallIndex = oldCallIndex;
	 pautoCall->status = status;

	 if (reqUri)
	 	strcpy(pautoCall->dst, reqUri);
	 else
	 	pautoCall->dst[0] = 0;

	 if (referBy)
	 	strcpy(pautoCall->to, referBy);
	 else
	 	pautoCall->to[0] = 0;
	 
	 #ifdef CMSIP_DEBUG
	 printf("---------%s,%d----\n", __FUNCTION__, __LINE__);
	 printf("full req uri:%s\n", reqUri);
	 printf("full refer by: %s\n", referBy);
	 printf("call index %d\n", pautoCall->callIndex);
	 printf("old call index %d\n", pautoCall->oldCallIndex);
	 printf("req-uri %s\n", pautoCall->dst);
	 printf("to uri %s\n", pautoCall->to);
	 printf("status %d\n", pautoCall->status);
	 #endif

	 ret = cmsip_sockSend(g_cmsip_cliSockfd, (char*)&cmSipMsg, sizeof(cmSipMsg));
	 if (ret < 0)
	 {
		#ifdef CMSIP_DEBUG
		printf("%s,%d: can not send CMSIP_SIP_AUTOCALLINFO\n", __FUNCTION__, __LINE__);
		#endif
		return -1;
	 }

	 return 0;	
	 
}

/* 
 * fn		extern int cmsip_send_incall(CMSIP_REQUEST type, int accId, int callId, char* from,
 * 													char* to, char* ipUri, int dstCallIndex)
 * brief	Send incoming INVITE request to CM.
 * details	
 *
 * param[in]	type	Type of this incoming call, my be a generic incoming call, or a forwarded
 *							call,	or a transfered call.
 *	param[in]	accId	Index of the account which is this incoming call's destination.
 *	param[in]	from	From URI of this incoming call(INVITE request).
 *	param[in]	to		To URI of this incoming call(INVITE request).
 * param[in]	ipUri	Request URI if this incoming call is a IP call.
 *	param[in]	dstCallIndex	Index of the replaced dialog if this incoming call is a transfer
 *										attended	call.
 * param[out]	none
 *
 * return	int
 * retval	0	success
 *			  -1	failure
 *
 * note		
 */
int cmsip_send_incall(CMSIP_REQUEST type, int accId, int callId, char* from, char* to,
										char* ipUri, int dstCallIndex
#	ifdef SUPPORT_NORMAL_CALL_WHEN_INVALID_CODEC_IN_OFFER
										,int supportLiteCodecs
#	endif
#ifdef INCLUDE_TFC_ES
									,char* date /*By YuChuwei, For Telefonica*/
									,char* alertInfo /*By YuChuwei, For Telefonica*/
									,char* assertedId /*By YuChuwei, For Telefonica*/
#endif
										)
{
	int ret;
	CMSIP_MSG cmSipMsg;		
	CMSIP_SIP_INCALL *pinCall = (CMSIP_SIP_INCALL*)cmSipMsg.body;
#ifdef INCLUDE_TFC_ES
	int min;
	int len;
#endif

	CMSIP_ASSERT(from!=NULL);
	CMSIP_ASSERT(to!=NULL);
	CMSIP_ASSERT(ipUri!=NULL);

	cmSipMsg.type = type;
	cmSipMsg.type |= CMSIP_MESSAGE_REQUEST;
	
	pinCall->accId = accId;
	pinCall->callIndex = callId;
	strcpy(pinCall->from, from);
	strcpy(pinCall->to, to);
	strcpy(pinCall->ipUri, ipUri);
	pinCall->dstCallIndex = dstCallIndex;
#	ifdef SUPPORT_NORMAL_CALL_WHEN_INVALID_CODEC_IN_OFFER
	pinCall->supportLiteCodecs = supportLiteCodecs;
#	endif
#ifdef INCLUDE_TFC_ES
	/*Date Header value, By YuChuwei, For Telefonica*/
	len = strlen(date);
	min = (sizeof(pinCall->date) > len)? len : sizeof(pinCall->date)-1;
	strncpy(pinCall->date, date, min);
	pinCall->date[min] = 0;	
	
	strcpy(pinCall->alertInfo, alertInfo);
	
	len = strlen(assertedId);
	min = (sizeof(pinCall->assertedId) > len)? len : sizeof(pinCall->assertedId)-1;
	strncpy(pinCall->assertedId, assertedId, min);
	pinCall->assertedId[min] = 0;
#endif

	#ifdef CMSIP_DEBUG
	printf("--%s,%d--acc[%d], \ncall[%d], \nfrom[%s], \nto[%s],\nipUri[%s],"
	"\ndstCall[%d]\n"
#	ifdef SUPPORT_NORMAL_CALL_WHEN_INVALID_CODEC_IN_OFFER
			"supportLiteCodec [%d]\n"
#	endif
#ifdef INCLUDE_TFC_ES

			" Date[%s]\n"
			" Alert-Info[%s]\n"
			" P-Asserted-Identity[%s]\n"
#	endif
			,
			__FUNCTION__, __LINE__,
				pinCall->accId, pinCall->callIndex, pinCall->from,
				pinCall->to, pinCall->ipUri, pinCall->dstCallIndex
#	ifdef SUPPORT_NORMAL_CALL_WHEN_INVALID_CODEC_IN_OFFER
				,pinCall->supportLiteCodecs
#	endif
#ifdef INCLUDE_TFC_ES
				,pinCall->date
				,pinCall->alertInfo
				,pinCall->assertedId
#endif
				);
	#endif

	ret = cmsip_sockSend(g_cmsip_cliSockfd, (char*)&cmSipMsg, sizeof(cmSipMsg));
	if (ret < 0)
	{
		#ifdef CMSIP_DEBUG
		printf("%s,%d: can not send CMSIP_SIP_INCALL\n", __FUNCTION__, __LINE__);
		#endif
		return -1;
	}

	return 0;

}

/* 
 * fn		extern int cmsip_send_inrefer(int accId, int callId,	char* referto)
 * brief	Send CMSIP_REQUEST_SIP_INREFER request to CM.
 * details	
 *
 * param[in]	accId	Index of the destination account of the incoming REFER request.
 *	param[in]	callId	Index of the call related to the incoming REFER request.
 *	param[in]	referto	URI of REFER-TO header, which will be as the target of the new call.
 * param[out]	none
 *
 * return	int
 * retval	0	success
 *			  -1 failure
 *
 * note		
 */
int cmsip_send_inrefer(int accId, int callId, char *referto)
{
	 CMSIP_MSG cmSipMsg;	 
	 CMSIP_SIP_INREFER *pinRefer = NULL;
	 int ret;

	 CMSIP_ASSERT(referto!=NULL);
	 
	 cmSipMsg.type = CMSIP_REQUEST_SIP_INREFER;
	 cmSipMsg.type |= CMSIP_MESSAGE_REQUEST;

	 pinRefer = (CMSIP_SIP_INREFER*)cmSipMsg.body;
	 pinRefer->accId = accId;
	 pinRefer->callIndex = callId;

	 getSipUri(pinRefer->referto, referto, sizeof(pinRefer->referto));
	 
	 #ifdef CMSIP_DEBUG
	 printf("----------%s---%d----send CMSIP_SIP_INREFER\n", __FUNCTION__, __LINE__);
	 printf("accId %d\n", pinRefer->accId);
	 printf("call index %d\n", pinRefer->callIndex);
	 printf("refer to %s\n", pinRefer->referto);
	 #endif

	 ret = cmsip_sockSend(g_cmsip_cliSockfd, (char*)&cmSipMsg, sizeof(cmSipMsg));
	 if (ret < 0)
	 {
		#ifdef CMSIP_DEBUG
		printf("%s,%d: can not send CMSIP_SIP_INREFER\n", __FUNCTION__, __LINE__);
		#endif
		return -1;
	 }

	 return 0;
}

/* 
 * fn		extern int  cmsip_send_transfernotify(int callIndex,
 *							CMSIP_TRANSFERED_CALL_STATUS xferStatus, int statusCode)
 * brief	Send CMSIP_REQUEST_SIP_TRANSFERNOTIFY request to CM.
 * details	
 *
 * param[in]	callIndex	Index of the call which has been transfered by our IAD.
 *	param[in]	xferStatus	Status of the call which is transfered by our IAD.
 *	param[in]	statusCode	Status code of the call which is transfered by our IAD.
 *									This status code is carried in the NOTIFY request.
 * param[out]	none
 *
 * return	int
 * retval	0	success
 *			  -1 failure
 *
 * note		
 */
int  cmsip_send_transfernotify(int callIndex, CMSIP_TRANSFERED_CALL_STATUS xferStatus,
														int statusCode)
 {
 	CMSIP_MSG cmSipMsg;
	CMSIP_SIP_TRANSFERNOTIRY *pstat = NULL;
	int ret;

	cmSipMsg.type = CMSIP_REQUEST_SIP_TRANSFERNOTIFY;
	cmSipMsg.type |= CMSIP_MESSAGE_REQUEST;

	pstat = (CMSIP_SIP_TRANSFERNOTIRY*)cmSipMsg.body;
	pstat->callIndex = callIndex;
	pstat->notifyStatus = xferStatus;
	pstat->statusCode = statusCode;

	#ifdef CMSIP_DEBUG
	printf("----%s,%d---send CMSIP_SIP_TRANSFERSTATUS--\n", __FUNCTION__, __LINE__);
	printf("call index %d\n", pstat->callIndex);
	printf("status code %d\n", pstat->statusCode);
	printf("xfer status %d\n", pstat->notifyStatus);
	#endif

	ret = cmsip_sockSend(g_cmsip_cliSockfd, (char *)&cmSipMsg, sizeof(cmSipMsg));
	if (ret < 0)
	{
		#ifdef CMSIP_DEBUG
		printf("%s,%d: can not send CMSIP_SIP_TRANSFERSTATUS\n", __FUNCTION__, __LINE__);
		#endif

		return -1;
	}

	return 0;
 }

/* 
 * fn		extern int cmsip_send_mwinotify(int accId, char* body, int len)
 * brief	Send CMSIP_REQUEST_SIP_MWINOTIFY request to CM.
 * details	
 *
 * param[in]	accId	Index of the account which subscribes MWI.
 *	param[in]	body	String of the NOTIFY request's body, such as
 *							Messages-Waiting: yes \r\n
 *							Voice-Message: 1/3 (0/1) \r\n
 *	param[in]	len	Size of body
 * param[out]	none
 *
 * return	int
 * retval	0	success
 *			  -1	failure
 *
 * note		
 */
int cmsip_send_mwinotify(int accId, char* body, int len)
{
	CMSIP_MSG cmSipMsg;
	CMSIP_SIP_MWINOTIFY *pnotify = NULL;
	int ret;

	cmSipMsg.type = CMSIP_REQUEST_SIP_MWINOTIFY;
	cmSipMsg.type |= CMSIP_MESSAGE_REQUEST;

	pnotify = (CMSIP_SIP_MWINOTIFY*)cmSipMsg.body;
	pnotify->accId = accId;
	if (sizeof(pnotify->data) <= len)
	{
		/*Note: if body is too large, we will seprate it into two or more packets.		
		*/
	}
	else
	{
		memcpy(pnotify->data, body, len);
		pnotify->data[len] = 0;
	}

	#ifdef CMSIP_DEBUG
	printf("-----------%s, %d-------------\n", __FUNCTION__, __LINE__);
	printf("account %d\n", pnotify->accId);
	printf("mwi info: %s\n", pnotify->data);
	#endif

	ret = cmsip_sockSend(g_cmsip_cliSockfd, (char *)&cmSipMsg, sizeof(cmSipMsg));
	if (ret < 0)
	{
		#ifdef CMSIP_DEBUG
		printf("%s,%d: can not send CMSIP_SIP_MWINOTIFY\n", __FUNCTION__, __LINE__);
		#endif

		return -1;
	}
	return 0;
}

/* 
 * fn		extern int cmsip_send_outreferstatus(int callIndex, CMSIP_OUTREFER_RESULT result)
 * brief	Send CMSIP_RESPONSE_SIP_OUTREFERSTATUS response to CM.
 * details	
 *
 * param[in]	callIndex	Index of the call realted to the out REFER request sent by our IAD.
 *	param[in]	result		Result of the response to the out REFER request.
 * param[out]	none
 *
 * return	int
 * retval	0	success
 *			  -1	failure
 *
 * note		
 */
int cmsip_send_outreferstatus(int callIndex, CMSIP_OUTREFER_RESULT result)
{
	CMSIP_MSG cmSipMsg;
	CMSIP_SIP_OUTREFERSTATUS *pOutReferStat = NULL;
	int ret;
	
	cmSipMsg.type = CMSIP_RESPONSE_SIP_OUTREFERSTATUS;

	pOutReferStat = (CMSIP_SIP_OUTREFERSTATUS*)cmSipMsg.body;
	pOutReferStat->callIndex = callIndex;
	pOutReferStat->result = result;

	#ifdef CMSIP_DEBUG
	printf("----------%s,%d---------\n", __FUNCTION__, __LINE__);
	printf("call index %d\n", pOutReferStat->callIndex);
	printf("result %s\n", 
		(pOutReferStat->result == CMSIP_OUTREFER_RESULT_SUCCESS) ? "success" : "fail");
	#endif

	ret = cmsip_sockSend(g_cmsip_cliSockfd, (char *)&cmSipMsg, sizeof(cmSipMsg));
	if (ret < 0)
	{
		#ifdef CMSIP_DEBUG
		printf("%s,%d: can not send CMSIP_SIP_OUTREFERSTATUS\n", __FUNCTION__, __LINE__);
		#endif
		return -1;
	}

	 return 0;	

}

/* 
 * fn		int cmsip_send_systemlog(CMSIP_SYSTEMLOG_TYPE type, char* fmt,...)
 * brief	Send system log to CM.
 * details	
 *
 * param[in]	type	The type of system log
 * param[in]	fmt		The format of system log
 * param[in]	...		extra parameters of system log
 *
 * return	int
 * retval	0	success
 *		-1	failure
 *
 */
int cmsip_send_systemlog(CMSIP_SYSTEMLOG_TYPE type, const char* fmt, ...)
{
	LOG_SEVERITY severity;
	va_list vList;
	char log[LOG_CONTENT_LEN] = { 0 };
	
	switch (type)
	{
		case CMSIP_SYSTEMLOG_WARN:
			severity = LOG_WARN;
			break;
			
		case CMSIP_SYSTEMLOG_INFO:
			severity = LOG_INFORM;
			break;

		default:
			severity = LOG_DEBUG;
			break;
	}

	va_start (vList, fmt);
	vsnprintf(log, sizeof(log) - 1, fmt, vList);
	va_end (vList);
	
	cmmlog(severity, LOG_VOIP, log);

	#ifdef CMSIP_DEBUG
	printf("%s,%d: send CMSIP_SIP_SYSTEMLOG(type %d, log: %s)\n", __FUNCTION__, __LINE__, 
			severity, log);
	#endif

	return 0;	
}

int cmsip_send_genres(int accId, const char* method, int stat_code)
{
	CMSIP_MSG cmSipMsg;
	CMSIP_SIP_GENRES* pGenRes = NULL;
	int ret;

	cmSipMsg.type = CMSIP_RESPONSE_SIP_GENRES;

	pGenRes = (CMSIP_SIP_GENRES*)cmSipMsg.body;
	pGenRes->accId = accId;
	pGenRes->statusCode = stat_code;
	strcpy(pGenRes->data, method);
	
	#ifdef CMSIP_DEBUG
	printf("----------%s,%d---------\n", __FUNCTION__, __LINE__);
	printf("accId %d\n", pGenRes->accId);
	printf("status %d\n", pGenRes->statusCode);
	printf("method %s\n", pGenRes->data);
	#endif

	ret = cmsip_sockSend(g_cmsip_cliSockfd, (char *)&cmSipMsg, sizeof(cmSipMsg));
	if (ret < 0)
	{
		#ifdef CMSIP_DEBUG
		printf("%s,%d: can not send CMSIP_SIP_OUTREFERSTATUS\n", __FUNCTION__, __LINE__);
		#endif
		return -1;
	}

	 return 0;	

}

