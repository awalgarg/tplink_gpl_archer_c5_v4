/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		cmsip_transport.h
 * brief		This file implements the function which PJSIP communicates with CM.
 * details	
 *
 * author	Yu Chuwei
 * version	1.0.0
 * date		21Sept11
 *
 * warning	
 *
 * history \arg	
 */

#ifndef __CMSIP_TRANSPORT_H__
/* 
 * brief	prevent include again
 */
#define __CMSIP_TRANSPORT_H__

#include "cmsip_msgdef.h"
/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/

/* 
 * brief	The socket connected CM
 */
extern int g_cmsip_cliSockfd;
/**************************************************************************************************/
/*                                           FUNCTIONS                                            */
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
extern int cmsip_sockCreate(int *pSockfd, const char *strSrvAddress);

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
extern int cmsip_sockSend(int sockfd, char* buf, int size);

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
extern int cmsip_sockRecv(int sockfd, char* buf, int size);

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
extern int cmsip_sockClose(int* sockfd);

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
extern int cmsip_send_acc_regStatus(int accId, CMSIP_REG_STATE status
#	if defined(SUPPORT_ACCOUNT_RTT) && SUPPORT_ACCOUNT_RTT!=0
, int duration
#	endif
#ifdef INCLUDE_TFC_ES
, char* date /*By YuChuwei, For Telefonica*/
#endif
);

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
extern int cmsip_send_mwistatus(int accId, CMSIP_MWI_RESULT status);

/* 
 * fn		extern int cmsip_send_callStatus(CMSIP_CALL_TYPE type, int callIndex, int seq,
										int statusCode,	char contact[][MAX_URI_LEN], int cnt)
 * brief	Send the response of the outgoing INVITE request to CM
 * details	
 *
 *	param[in]	type		Call Type
 *  param[in]	callIndex	Index of the call which is related with the outgoing INVITE request.
 *	param[in]	seq			The data used by CM to look for related call.
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
extern int cmsip_send_callStatus(CMSIP_CALL_TYPE type, int callIndex, int seq,
										int statusCode,	char contact[][MAX_URI_LEN], int cnt);

/* 
 * fn		extern int cmsip_send_callRelease(int callIndex)
 * brief	Send Call Release request to CM.
 * details	
 *
 * param[in]	type		Call Type
 * param[in]	callIndex	Index of the call which will be released
 * param[out]	none
 *
 * return	int
 * retval	0	success
 *	retval	-1	failure
 *
 * note		
 */
extern int cmsip_send_callRelease(CMSIP_REQUEST type, unsigned callIndex);

/* 
 * fn		extern int cmsip_send_autocall_forward(int callIndex, int oldCallIndex, 
 *														char* reqUri, char* to)
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
extern int cmsip_send_autocall_forward(int callIndex, int oldCallIndex, 
																	char* reqUri, char* to);

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
extern int cmsip_send_incallack(int callIndex);

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
extern int cmsip_send_holdstatus(int callIndex, CMSIP_HOLD_RESULT result);

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
 *	param[in]	dlptime		decode Packetization Time of the code
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
 * return	
 * retval	
 *
 * note		DSP socket ---- related to this call, to send audio/video data which DSP generate.
 */
extern int cmsip_send_rtpcreate(int callIndex,
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
					);


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
 *	retval	-1	failure
 *
 * note		
 */
extern int cmsip_send_rtpdestroy(int callIndex);

/* 
 * fn		extern int cmsip_send_autocall_transfer(int callIndex, int oldCallIndex,
 *														char* reqUri, char* referBy)
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
 *				-1 failure
 *
 * note		
 */
extern int cmsip_send_autocall_transfer(int accId, int callIndex, int oldCallIndex,
													char* reqUri, char* referBy, int status);

/* 
 * fn		extern int cmsip_send_incall(CMSIP_REQUEST type, int accId, int callId, char* from, char* to,
 *											char* ipUri, int dstCallIndex)
 * brief	Send incoming INVITE request to CM.
 * details	
 *
 * param[in]	type	Type of this incoming call, my be a generic incoming call, or a forwarded call,
 *							or a transfered call.
 *	param[in]	accId	Index of the account which is this incoming call's destination.
 *	param[in]	from	From URI of this incoming call(INVITE request).
 *	param[in]	to		To URI of this incoming call(INVITE request).
 * param[in]	ipUri	Request URI if this incoming call is a IP call.
 *	param[in]	dstCallIndex	Index of the replaced dialog if this incoming call is a transfer attended
 * 										call.
 * param[out]	none
 *
 * return	int
 * retval	0	success
 *				-1	failure
 *
 * note		
 */
extern int cmsip_send_incall(CMSIP_REQUEST type, int accId, int callId, char* from,
									char* to, char* ipUri, int dstCallIndex
#	ifdef SUPPORT_NORMAL_CALL_WHEN_INVALID_CODEC_IN_OFFER
									,int supportLiteCodecs
#	endif
#ifdef INCLUDE_TFC_ES
									,char* date /*By YuChuwei, For Telefonica*/
									,char* alertInfo /*By YuChuwei, For Telefonica*/
									,char* assertedId /*By YuChuwei, For Telefonica*/
#endif
									);

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
 *				-1 failure
 *
 * note		
 */
extern int cmsip_send_inrefer(int accId, int callId, char* referto);

/* 
 * fn		extern int  cmsip_send_transfernotify(int callIndex,
 *								CMSIP_TRANSFERED_CALL_STATUS xferStatus, int statusCode)
 * brief	Send CMSIP_REQUEST_SIP_TRANSFERNOTIFY request to CM.
 * details	
 *
 * param[in]	callIndex	Index of the call which has been transfered by our IAD.
 *	param[in]	xferStatus	Status of the call which is transfered by our IAD.
 *	param[in]	statusCode	Status code of the call which is transfered by our IAD.This status
 *									code is carried in the NOTIFY request.
 * param[out]	none
 *
 * return	int
 * retval	0	success
 *				-1 failure
 *
 * note		
 */
extern int  cmsip_send_transfernotify(int callIndex,
							CMSIP_TRANSFERED_CALL_STATUS xferStatus, int statusCode);

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
 *				-1	failure
 *
 * note		
 */
extern int cmsip_send_mwinotify(int accId, char* body, int len);

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
 *				-1	failure
 *
 * note		
 */
extern int cmsip_send_outreferstatus(int callIndex, CMSIP_OUTREFER_RESULT result);

/* 
 * fn		extern int cmsip_send_systemlog(CMSIP_SYSTEMLOG_TYPE type, char* fmt,...)
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
extern int cmsip_send_systemlog(CMSIP_SYSTEMLOG_TYPE type, const char* fmt, ...);

extern int cmsip_send_genres(int accId, const char * method, int stat_code);

#endif	/* __CMSIP_TRANSPORT_H__ */
