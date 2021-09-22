/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		cmsip_msgdef.h
 * brief		This file defines the ENUMs and Data Structs about the Message packets transported between
 *				CM and PJSIP.
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
#ifndef __CMSIP_MSGDEF_H__
/* 
 * brief	prevent include again
 */
#define __CMSIP_MSGDEF_H__

#include "cmsip_assert.h"
/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/
#define PTHREAD_PRIO_FIFO_HIGH      40
#define PTHREAD_PRIO_FIFO_MED       50
#define PTHREAD_PRIO_FIFO_LOW       60
#define PTHREAD_PRIO_OTHER          100
/*
 * fax T.38 switch. If disable T.38, set it to 0.
 */
#ifndef SUPPORT_FAX_T38
#define SUPPORT_FAX_T38 1
#endif

#ifdef INCLUDE_USB_VOICEMAIL
/* 
 * brief If SDP offer codecs are not PCMA/U, there are two ways to process this 
 		 problem. One is responding 415(Unsupported Media Type), another is treating
 		 this call as a normal call. 
 		 If SUPPORT_NORMAL_CALL_WHEN_INVALID_CODEC_IN_OFFER is defined, 
 		 we take the 2nd way.
 */
#define SUPPORT_NORMAL_CALL_WHEN_INVALID_CODEC_IN_OFFER 1
#endif /* INCLUDE_USB_VOICEMAIL */

/* 
 * brief	Message type mask, low 30 bits
 */
#define CMSIP_MSGTYPE_MASK 0x3FFFFFFF

/* 
 * brief	Present request
 */
#define CMSIP_MESSAGE_REQUEST 0x80000000

/* 
 * brief	 The 30th bit of the Message Type field of a packet to present that if a packet is a 
 * 		 completed message or a part of a message.
 *			 If the 30th bit of the Message Type field is 0, this packet present a completed message.
 *			 If it is 1, this packet only is a part of a message, and the program must receive subsequent
 *			 packets to constitute the message.
 */
#define CMSIP_MESSAGE_COMPLETION 0x40000000

/* 
 * brief	Length of URI
 */
#define MAX_URI_LEN	256
/* 
 * brief	Length of method name
 */
#define MAX_METHOD_LEN 16
/* 
 * brief	Length of IP address
 */
#define MAX_IPADDR_LEN 16
/* 
 * brief	The maximum number of contact in the forwarded INVITE or a 302 response
 *			
 */
#define MAX_CONTACT_URI 1
/* 
 * brief	The size of extra data of Disable Stack Request.Now, it is 0.
 */
#define MAX_STACK_EXTDATA_LEN 0
/* 
 * brief	Path of CM Server Unix Socket
 */
#define CMSIP_SOCK_PATH       "/var/tmp/sock_CM_SIP"

/* 
 * brief	Constant.
 */
#define CMSIP_UNKNOWN  -1

/* 
 * brief	maximum number of codecs
 */
#define CMSIP_MAX_CODEC_NAME_LEN	32


/* 
 * brief	A const, value is 256
 */
#define CMSIP_STR_256 256

/* 
 * brief	A const, value is 128
 */
#define CMSIP_STR_128 128

/* 
 * brief	A const, value is 64
 */
#define CMSIP_STR_64	64

/* 
 * brief	A const, value is 40
 */
#define CMSIP_STR_40 40

/* 
 * brief	A const, value is 32
 */
#define CMSIP_STR_32 32

/* 
 * brief	A const, value is 16
 */
#define CMSIP_STR_16	16


/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/

/*************************SIP operation***************************************/

/***************Message type define*******************/
/* 
 * brief Type of Request Message
 */
typedef enum _CMSIP_REQUEST
{
/*SIP operation*/
	CMSIP_REQUEST_SIP_REG = 1000, /*SIP REGISTER request*/
	CMSIP_REQUEST_SIP_UNREG = 1001, /*SIP un-REGISTER request*/
	CMSIP_REQUEST_SIP_RTPCREATE = 1002, /*Media info to create RTP stream*/
	CMSIP_REQUEST_SIP_RTPDESTROY = 1003, /*Media info to destroy RTP stream*/
	CMSIP_REQUEST_SIP_AUTOCALLFORWARD = 1004, /*If receive 3xx response, PJSIP
											call the target automatically	and then report
											the new call to CM*/
	CMSIP_REQUEST_SIP_AUTOCALLXFER = 1005, /*If receive REFER request, PJSIP call
											the target automatically and then report the 
											new call to CM*/
	CMSIP_REQUEST_SIP_CALL = 1006, /*SIP INVITE request*/
	CMSIP_REQUEST_SIP_CALL_T38 = 1007, /*SIP INVITE request for T.38*/
	CMSIP_REQUEST_SIP_INREFER = 1008, /*Incoming REFER request*/
	CMSIP_REQUEST_SIP_INCOMINGCALL = 1009, /*Generic incoming call*/
	CMSIP_REQUEST_SIP_INCOMINGCALL_FORWARDED = 1010, /*Incoming call is forwarded
											by the third patry*/
	CMSIP_REQUEST_SIP_INCOMINGCALL_TRANSFERED = 1011, /*Incoming call is transfered
											by the third party*/
	CMSIP_REQUEST_SIP_INCOMINGCALL_ACK = 1012, /*Acknowledge from the party where
											Incoming call comes from*/	
	CMSIP_REQUEST_SIP_TRANSFER_UNATTENDED = 1013, /*To unattended transfer a 
											call(incoming or outgoing)*/
	CMSIP_REQUEST_SIP_TRANSFER_ATTENDED = 1014, /*To attended transfer a 
											call(incoming or outgoing)*/
	CMSIP_REQUEST_SIP_TRANSFERNOTIFY = 1015, /*Notify from transferee to report
											the progress of the call which was transfered
											by our IAD*/
	CMSIP_REQUEST_SIP_HOLD = 1016, /*SIP Hold request*/
	CMSIP_REQUEST_SIP_UNHOLD = 1017, /*SIP un-Hold request*/
	CMSIP_REQUEST_SIP_OUTDTMF = 1018, /*Request to Send DTMF to other party*/
	CMSIP_REQUEST_SIP_INDTMF = 1019, /*Incoming Request carried DTMF */
	CMSIP_REQUEST_SIP_CALLRELEASE = 1020, /*Request to release call*/
	CMSIP_REQUEST_SIP_MWISUB = 1021, /*Request to subscribe MWI notify*/
	CMSIP_REQUEST_SIP_MWIUNSUB = 1022, /*Request to unsubscribe MWI notify*/
	CMSIP_REQUEST_SIP_MWINOTIFY = 1023, /*Notify request of MWI*/
	CMSIP_REQUEST_SIP_REINVITE = 1024, /*Re-INVITE request*/
	CMSIP_REQUEST_SIP_GENREQ = 1025, /*Generic request(mybe INFO/NOTIFY...)*/
	CMSIP_REQUEST_SIP_INGENREQ = 1026, /*Incoming Generic request(maybe 
											INFO/NOTIFY...)*/
	/*by yuchuwei*/
	/*For stranded CANCEL which arrives after local transaction has sent the
	final response, PJSIP will ignore it. But remote party has release the call,
	so PJSIP must ask CM to release this call on his own.*/
	CMSIP_REQUEST_SIP_REQUIRECALLRELEASE = 1028,

	/*Stack operation*/
	CMSIP_REQUEST_STACK_RESTART = 2000, /*SIP Stack Restart Request*/
	CMSIP_REQUEST_STACK_DISABLE = 2001, /*SIP Stack Close Request*/	

	/*Usb Voice Mail*/
	CMSIP_REQUEST_USBVM_CONNECTION = 4000, /* USBVM offhook, send 200 to remote */
	CMSIP_REQUEST_USBVM_ENTER = 4001, /* Enter the remote usb voicemail */
	CMSIP_REQUEST_USBVM_EXIT = 4002, /* Exit the remote usb voicemail */
	CMSIP_REQUEST_USBVM_CHANGE_TIMER = 4003, /*Change CM endpoint timer */
	CMSIP_REQUEST_USBVM_GENERATE_EVTONHOOK = 4004, /*Generate local ONHOOK event */
	CMSIP_REQUEST_DETECT_REMOTE_DTMF = 4005, /*Detect remote DTMF */	
/*Added by huanglei */
	/*DTMF receiver*/
	CMSIP_REQUEST_SIP_DTMF_RECEIVER_SWITCH = 4006,		/* Turn receiver on or off  */
	/* When enter and exit, directly send a msg to cm, cm now has no need to ask sip about it*/
	CMSIP_REQUEST_USBVM_REMOTE_STATUS_CHANGE = 4007, 	
	CMSIP_REQUEST_USBVM_OFFHOOK_INRECORD = 4008,   /* if offhook in recording, tell pjsip */
	CMSIP_REQUEST_USBVM_START = 4009, /*Build USBVM RTP connection*/
/*End huanglei */
}CMSIP_REQUEST;

/* 
 * brief	Response message
 */
typedef enum _CMSIP_RESPONSE
{
/*SIP operation*/
	CMSIP_RESPONSE_SIP_REGSTATUS = 1000, /*REG/UNREG*/
	CMSIP_RESPONSE_SIP_CALLSTATUS = 1001, /*Outgoing or incoming*/
	CMSIP_RESPONSE_SIP_OUTREFERSTATUS = 1002, /*Response of out REFER request*/
	CMSIP_RESPONSE_SIP_HOLDSTATUS = 1003, /*Response of Hold request*/
	CMSIP_RESPONSE_SIP_MWISTATUS = 1004, /*Response of MWI Subscribe request*/
	CMSIP_RESPONSE_SIP_GENRES = 1005, /*Response of Generic request*/
	CMSIP_RESPONSE_SIP_INGENRES = 1006, /*Response of out Generic request*/
	CMSIP_RESPONSE_SIP_CALLRELEASE_ACK = 1007, /*Response of Call Release request*/
}CMSIP_RESPONSE;


/***********Message packet data struct define**************/
/*//--1--Register*/
/* 
 * brief	Register/Unregister request
 *			Type:CMSIP_REQUEST_SIP_REG/CMSIP_REQUEST_SIP_UNREG
 */
typedef struct _CMSIP_SIP_REG
{
	int  accId; /*ID of the account to register/unregister*/
}CMSIP_SIP_REG;

/* 
 * brief	Result of Register/Unregister 
 */
typedef enum _CMSIP_REG_STATE
{
	CMSIP_REG_STATE_SUCCESS = 0, /*Register/unRegister success*/
	CMSIP_REG_STATE_FAILURE = 1 /*Register/unRegister failure*/
}CMSIP_REG_STATE;
/* 
 * brief	Status of an account to Register/Unregister
 *			Type:CMSIP_RESPONSE_SIP_REGSTATUS
 */
typedef struct  _CMSIP_SIP_REGSTATUS
{
	int	accId;
	CMSIP_REG_STATE  state;
#	if defined(SUPPORT_ACCOUNT_RTT) && SUPPORT_ACCOUNT_RTT!=0
	int	duration;	/*the time from Register start to Receive the first response. For Unregister, 
							it is 0.*/
#	endif
#ifdef INCLUDE_TFC_ES
	char date[CMSIP_STR_64]; /*Date Header Value, By YuChuwei, For Telefonica*/
#endif
}CMSIP_SIP_REGSTATUS;


/*//--2--Call*/
/* 
 * brief	Types of Media
 */
typedef enum _CMSIP_MEDIA_TYPE
{	
    CMSIP_MEDIA_TYPE_NONE = 0, /** No type. */
    
    CMSIP_MEDIA_TYPE_AUDIO = 1, /** The media is audio */
    
    CMSIP_MEDIA_TYPE_VIDEO = 2, /** The media is video. */

    CMSIP_MEDIA_TYPE_UNKNOWN = 3, /** Unknown media type, in this case the name
    										will be specified in encoding_name.*/
    
    CMSIP_MEDIA_TYPE_APPLICATION = 4, /** The media is application. */
	
	 CMSIP_MEDIA_TYPE_IMAGE = 5 /*The media is image(T.38)*/
}CMSIP_MEDIA_TYPE;

/* 
 * brief	Transport Protocol of Media
 */
typedef enum _CMSIP_MEDIA_TPPROTO
{
    CMSIP_MEDIA_TPPROTO_NONE = 0, /** No transport type */
    
    CMSIP_MEDIA_TPPROTO_RTP_AVP = 1, /** RTP using A/V profile */
    
    CMSIP_MEDIA_TPPROTO_RTP_SAVP = 2, /** Secure RTP */
	 
	 CMSIP_MEDIA_TPPROTO_UDPTL = 3, /**T38*/ 
    
    CMSIP_MEDIA_TPPROTO_UNKNOWN = 4 /** Unknown */
}CMSIP_MEDIA_TPPROTO;

/* 
 * brief	Direction of Media
 */
typedef enum _CMSIP_MEDIA_DIR
{	  
    CMSIP_MEDIA_DIR_NONE = 0, /** None */
    
    CMSIP_MEDIA_DIR_ENCODING = 1, /** Encoding (outgoing to network) stream */

    CMSIP_MEDIA_DIR_DECODING = 2, /** Decoding (incoming from network) stream. */
    
    CMSIP_MEDIA_DIR_ENCODING_DECODING = 3 /** Incoming and outgoing stream. */
}CMSIP_MEDIA_DIR;

/* 
 * brief Information about media transport.
 *			Local address and ports, remote address and port, network type,and so on.
 *			Type:CMSIP_REQUEST_SIP_RTPCREATE
 */
typedef struct _CMSIP_SIP_RTPCREATE
{
	int 						callIndex;
	CMSIP_MEDIA_TYPE		mediaType;
	CMSIP_MEDIA_TPPROTO	mediaProtocol;
	CMSIP_MEDIA_DIR		dir;
	char 						codec[CMSIP_MAX_CODEC_NAME_LEN];
	int 						clockRate;
	int 						channelCnt;
	int 						ulptime;
	int						dlptime;
	int 						txVoicePayloadType;
	int 						rxVoicePayloadType;
	int 						txDTMFRelayPayloadType;
	int 						rxDTMFRelayPayloadType;
	unsigned					faxMaxDatagram;
	unsigned					faxMaxBitRate;
	int						faxEnableUdpEc;
#	if defined(PJ_HAS_IPV6) && PJ_HAS_IPV6!=0
	char 						remoteIpAddr[CMSIP_STR_16];
#	else
	char 						remoteIpAddr[CMSIP_STR_40];
#	endif
	unsigned short 		remoteDataPort;
	unsigned short 		remoteControlPort;
#	if defined(PJ_HAS_IPV6) && PJ_HAS_IPV6!=0
	char 						localIpAddr[CMSIP_STR_16]; /*DSP socket's IP*/
#	else
	char 						localIpAddr[CMSIP_STR_40]; /*DSP socket's IP*/
#	endif
	unsigned short 		localDataPort;/*DSP socket's port*/

	/*PJMEDIA_HAS_SRTP is not defined now.*/
	#if defined(PJMEDIA_HAS_SRTP) && (PJMEDIA_HAS_SRTP != 0)   
	unsigned short 		srtpSecurityType;
	char 						srtpSendKey[CMSIP_STR_128];
	char 						srtpRecvKey[CMSIP_STR_128];
	#endif
	
}CMSIP_SIP_RTPCREATE;

/* 
 * brief	Tell CM to destroy related DSP RTP Stream.
 *			Type:CMSIP_REQUEST_SIP_RTPDESTROY
 */
typedef struct _CMSIP_SIP_RTPDESTROY
{
	int callIndex;
}CMSIP_SIP_RTPDESTROY;

/* 
 * brief	If receive a 3xx response or a REFER request, PJSIP call the target
 * automatically,	and, report this to CM.
 *			Type:CMSIP_REQUEST_SIP_AUTOCALLFORWARD/CMSIP_REQUEST_SIP_AUTOCALLXFER
 */
typedef struct _CMSIP_SIP_AUTOCALLINFO
{
	int accId;
	int callIndex;
	int oldCallIndex; /*Call index of the forwarded/transfered call*/
	char dst[MAX_URI_LEN]; /*Request-URI*/
	char to[MAX_URI_LEN]; /*For transfer, this may be the Refer-By.*/
	int status; /*202--Accept; 488--Not Accept Here*/
}CMSIP_SIP_AUTOCALLINFO;


/* 
 * brief	Type of sip call. Generic sip call or t38
 */
typedef enum _CMSIP_CALL_TYPE
{
	CMSIP_CALL_GENERIC = 0, /*generic sip call*/
	CMSIP_CALL_ANONYMOUS = 1, /*anonymous call*/
#	if defined(INCLUDE_PSTN_GATEWAY) || (defined(SUPPORT_FAX_T38) && SUPPORT_FAX_T38!=0)
	CMSIP_CALL_PSTN = 2,	/*voip ---> pstn*/
#	endif
	CMSIP_CALL_T38 = 3,	/*t38*/
#	ifdef INCLUDE_USB_VOICEMAIL
	CMSIP_CALL_USBVM = 4,
#	endif /* SUPPORT_USV_VOICEMAIL */
	CMSIP_CALL_VOICEAPP = 5,
	CMSIP_CALL_VOICEAPP_ANONYMOUS = 6,
}CMSIP_CALL_TYPE;
/* 
 * brief	Call request
 *			Type:CMSIP_REQUEST_SIP_CALL
 */
typedef struct  _CMSIP_SIP_CALL
{
	CMSIP_CALL_TYPE type; /*type of this call*/
	int seq;	/*unique ID of this call between INVITE and 180*/
	int accId;	/*ID of the account to request call*/
	char destUri[MAX_URI_LEN]; /*URI of the call target*/
}CMSIP_SIP_CALL;

/* 
 * brief	send re-INVITE to negotiate again.
 *			Type:CMSIP_REQUEST_SIP_REINVITE
 */
typedef struct _CMSIP_SIP_REINVITE
{
	CMSIP_CALL_TYPE type;
	int callIndex;
}CMSIP_SIP_REINVITE;

/* 
 * brief	Status of the outgoing/incoming call,include 100.
 *			Note:PJSIP call the forward target directly, not controled by CM,
 *			only report it.
 *			Type:CMSIP_RESPONSE_SIP_CALLSTATUS
 */
typedef struct _CMSIP_SIP_CALLSTATUS
{
	CMSIP_CALL_TYPE type; /*the incoming call will go to pstn or not*/
	int seq;	/*unique ID of this call between INVITE and 180*/
	int callIndex;
	int statusCode;
	char contact[MAX_CONTACT_URI][MAX_URI_LEN];/*the statusCode is 3xx.
															Default:NULL*/
}CMSIP_SIP_CALLSTATUS;
/* 
 * brief	We receive a incoming Refer request, as current call is transfered.
 *			Note:The response of the incoming REFER request is generated by PJSIP
 *					stack. So, we need not define it's data struct.
 *			Note:PJSIP call the transfer target directly, not controlled by CM,
 *			only report this to CM.
 *			Type:CMSIP_REQUEST_SIP_INREFER
 */
typedef struct _CMSIP_SIP_INREFER
{
	int accId;
	int callIndex;
	char referto[MAX_URI_LEN];
}CMSIP_SIP_INREFER;
/* 
 * brief Request of Incoming call.
 *			This call may be a genric call, or a forwarded call, or a transfered
 * 		call.
 *			If it is a generic call or a forward call, dstCallIndex equals 
 *			-1(UNKNOWN).
 *			If it is a unattended transfered call, dstCallIndex equals -1(UNKNOWN),
 *			and I will place the value of Refer-by header field into 'to'field.
 *			If it is a attended transfered call, dstCallIndex is the ID of the call
 *			which will be replaced.
 *			Type: CMSIP_REQUEST_SIP_INCOMINGCALL
 *					CMSIP_REQUEST_SIP_INCOMINGCALL_FORWARDED
 *					CMSIP_REQUEST_SIP_INCOMINGCALL_TRANSFERED 
 */
typedef struct  _CMSIP_SIP_INCALL
{
	int accId; 
	int callIndex;
	char from[MAX_URI_LEN];/*URI of caller. Serveral functions, such as Call 
									Return/Call Back/Caller ID, will need it.*/
	char to[MAX_URI_LEN]; /*For transfered incoming call, this may be the value
									of Refer-By header field*/
	char ipUri[CMSIP_STR_32]; /*For IP Call*/
	int dstCallIndex; /*Default:CMSIP_UNKNOWN*/
#	ifdef SUPPORT_NORMAL_CALL_WHEN_INVALID_CODEC_IN_OFFER
	int supportLiteCodecs;
#	endif

	char alertInfo[MAX_URI_LEN]; /*Alert-Info Header value, By YuChuwei, For Telefonica*/
#ifdef INCLUDE_TFC_ES
	char date[CMSIP_STR_64]; /*Date Header value, By YuChuwei, For Telefonica*/
	char assertedId[CMSIP_STR_64]; /*P-Asserted-Identity Header value, By YuChuwei, For Telefonica*/
#endif	
}CMSIP_SIP_INCALL;
/* 
 * brief	ACK request to incoming call
 *			Type:CMSIP_REQUEST_SIP_INCOMINGCALL_ACK
 */
typedef struct _CMSIP_SIP_INCALL_ACK
{
	int callIndex;
}CMSIP_SIP_INCALL_ACK;
/* 
 * brief	Unattended/Attended transfer request. We received a REFER request.
 *			PJSIP as transfer. This request will result in PJSIP to send REFER
 *			request.
 *			Type:CMSIP_REQUEST_SIP_TRANSFER_UNATTENDED
 *				CMSIP_REQUEST_SIP_TRANSFER_ATTENDED
 */
typedef struct _CMSIP_SIP_TRANSFER /*Out REFER*/
{
	int callIndex; /*ID of the call will be transfered*/
	char dstUri[MAX_URI_LEN]; /*URI of transfer target.Default:NULL*/
	int dstCallIndex;	/*Default:CMSIP_UNKNOWN*/
}CMSIP_SIP_TRANSFER;
/* 
 * brief	Result of outgoing refer request
 */
typedef enum _CMSIP_OUTREFER_RESULT
{
	CMSIP_OUTREFER_RESULT_SUCCESS = 1, /*out REFER succeed*/
	CMSIP_OUTREFER_RESULT_TERMINATED = 2 /*out REFER fail or terminate*/
}CMSIP_OUTREFER_RESULT;

/* 
 * brief	Status of out refer quest
 *			Type:CMSIP_RESPONSE_SIP_OUTREFERSTATUS
 */
typedef struct _CMSIP_SIP_OUTREFERSTATUS
{
	int  callIndex;
	CMSIP_OUTREFER_RESULT result;
}CMSIP_SIP_OUTREFERSTATUS;

/* 
 * brief	Result of the transfered call
 */
typedef enum  _CMSIP_TRANSFERRED_CALL_STATUS
{
	CMSIP_TRANSFERED_CALL_TRYING = 0, /*the transfered call is trying*/
	CMSIP_TRANSFERED_CALL_SUCCESS = 1, /*the transfered call succeed*/
	CMSIP_TRANSFERED_CALL_FAIL = 2 /*the transfered call fail*/
}CMSIP_TRANSFERED_CALL_STATUS;
/* 
 * brief	Receive Notify request which report the status of the transfered call
 *			Type:CMSIP_REQUEST_SIP_TRANSFERNOTIFY
 */
typedef struct _CMSIP_SIP_TRANSFERNOTIFY
{
	int    callIndex;
	CMSIP_TRANSFERED_CALL_STATUS  notifyStatus;
	int statusCode;
}CMSIP_SIP_TRANSFERNOTIRY;

/* 
 * brief	Hold request(local or remote)
 *			Type:CMSIP_REQUEST_SIP_HOLD/CMSIP_REQUEST_SIP_UNHOLD
 */
typedef struct _CMSIP_SIP_HOLD
{
	int callIndex; /*ID of the call which will be hold*/
}CMSIP_SIP_HOLD;

/* 
 * brief	Result of Hold/Unhold request
 */
typedef enum _CMSIP_HOLD_RESULT
{
	CMSIP_HOLD_SUCCESS = 0, /*Hold request succeed*/
	CMSIP_HOLD_FAIL = 1 /*Hold request fail*/
}CMSIP_HOLD_RESULT;
/* 
 * brief	Status of Hold/Unhold request
 *			Type:CMSIP_RESPONSE_SIP_HOLDSTATUS
 */
typedef struct _CMSIP_SIP_HOLDSTATUS
{
	int  callIndex;
	CMSIP_HOLD_RESULT  result;
}CMSIP_SIP_HOLDSTATUS;

/* 
 * brief	 There are three specification to send DTMF
 */
typedef enum _CMSIP_DTMF_SEND_TYPE
{
	CMSIP_DTMF_INBAND = 0, /*The method of DTMF Relay is Inband*/
	CMSIP_DTMF_RFC2833 = 1, /*The method of DTMF Relay is RFC2833*/
	CMSIP_DTMF_SIPINFO = 2, /*The method of DTMF Relay is SipINFO*/
}CMSIP_DTMF_SEND_TYPE;

/* 
 * brief	Request to send DTMF/Receive a SIP INFO request which carries DTMF
 *		 	Type:CMSIP_REQUEST_SIP_OUTDTMF/CMSIP_REQUEST_SIP_INDTMF
 */
typedef struct  _CMSIP_SIP_DTMF
{
	int 	callIndex;
	CMSIP_DTMF_SEND_TYPE	type; /*In band or rfc2833 or sip info*/
	char	digit;		
}CMSIP_SIP_DTMF;

/* 
 * brief	Release a call
 *			Type:CMSIP_REQUEST_SIP_CALLRELEASE/CMSIP_RESPONSE_SIP_CALLRELEASE_ACK/
 *				CMSIP_REQUEST_SIP_REQUIRECALLRELEASE
 */
typedef struct _CMSIP_SIP_CALLRELEASE
{
	unsigned callIndex;
	int seq;
}CMSIP_SIP_CALLRELEASE;

/*//--3--MWI*/
/* 
 * brief	MWI subscribe request
 *			Type:CMSIP_REQUEST_SIP_MWISUB/CMSIP_REQUEST_SIP_MWIUNSUB
 */
typedef struct  _CMSIP_SIP_MWI
{	
	int accId;
}CMSIP_SIP_MWI;

/* 
 * brief	Result of MWI Subscribe
 */
typedef enum _CMSIP_MWI_RESULT
{
	CMSIP_MWI_SUCCESS = 0, /*MWI request succeed*/
	CMSIP_MWI_TERMINATE = 1 /*MWI request fail or terminate*/
}CMSIP_MWI_RESULT;
/* 
 * brief	Status of MWI Subscribe/unSubscribe request
 *			Type:CMSIP_RESPONSE_SIP_MWISTATUS
 */
typedef struct  _CMSIP_SIP_MWISTATUS
{
	int accId;
	CMSIP_MWI_RESULT result;
}CMSIP_SIP_MWISTATUS;

/* 
 * brief	The types of the logs sent to cm
 */
typedef enum _CMSIP_SYSTEMLOG_TYPE
{
	CMSIP_SYSTEMLOG_INFO, /*generic*/
	CMSIP_SYSTEMLOG_WARN, /*warning*/
	CMSIP_SYSTEMLOG_DEBUG /*debug*/
}CMSIP_SYSTEMLOG_TYPE;

/*********************Application Stack Operation****************/
/* 
 * brief	The request to ask PJSIP to disable
 *			Type:CMSIP_REQUEST_STACK_DISABLE
 */
typedef struct _CMSIP_STACK_DISABLESTACK
{
	char data[MAX_STACK_EXTDATA_LEN];
}CMSIP_STACK_DISABLESTACK;

/* 
 * brief	The request to ask PJSIP to restart, not exit the progress.
 *			Type:CMSIP_REQUEST_STACK_RESTART
 */
typedef struct _CMSIP_STACK_RESTARTSTACK
{
	char data[MAX_STACK_EXTDATA_LEN];
}CMSIP_STACK_RESTARTSTACK;


/************************USB Voice Mail Operation*********************/
/* 
 * brief	The request from CM to PJSIP for building RTP connection for USB voicemail
 *			Type:CMSIP_REQUEST_USBVM_CONNECTION
 */
typedef struct _CMSIP_USBVM_CONNECTION
{
	int callIndex;   /* call ID */
	int recordId;    /* record ID */
	int endpt;       /* endpoint ID */
	int duration; /* duration of record  */
	int globalWav; /* automatic transform to wav  */
	unsigned int ssrc;	/* add by yuanjp
						ssrc is allocated by voip_client, keep answer machine and voip server same. */
} CMSIP_USBVM_CONNECTION;

/* 
 * brief	The request from CM to PJSIP for entering the USB voicemail
 *			Type:CMSIP_USBVM_ENTER
 */
typedef struct _CMSIP_USBVM_ENTER
{
	int recordId; /* record ID */
	int callIndex;   /* call ID */
	int endpt;    /* endpoint ID */
	int duration; /* duration of record  */
	int globalWav; 
} CMSIP_USBVM_ENTER;

/* 
 * brief	The request from CM to PJSIP for exiting the USB voicemail
 *			Type:CMSIP_USBVM_EXIT
 */
typedef struct _CMSIP_USBVM_EXIT
{
	int recordId; /* record ID */
	int callIndex; /*call id*/
} CMSIP_USBVM_EXIT;


/* 
 * brief	Define USBVM remote state, in a msg sent to cm
 */
typedef enum _CMSIP_USBVM_REMOTE_STATE
{
	CMSIP_USBVM_REMOTE_STATE_IDLE = 0, /* USBVM remote is idle */
	CMSIP_USBVM_REMOTE_STATE_BUSY = 1, /* USBVM remote is busy */
	CMSIP_USBVM_REMOTE_STATE_EXITED = 2, /* USBVM remote has been exited*/
} CMSIP_USBVM_REMOTE_STATE;

/* 
 * brief	the msg struct of dtmf switch
 */
typedef struct _CMSIP_DTMF_RECEIVER_SWITCH
{
	int cmd;
	int callId;
}CMSIP_DTMF_RECEIVER_SWITCH;

/* 
 * brief	the cmd of dtmf switch
 */
typedef enum _DTMF_SWITCH
{
	DTMF_RECEIVER_OFF = 0,
	DTMF_RECEIVER_ON = 1
}DTMF_SWITCH;

/* 
 * brief	the status of remote usbvm, is used in cmEndpt.remoteUsbvmStatus
 */
typedef enum _USBVM_REMOTE_STATUS
{
	USBVM_REMOTE_STATUS_IDLE = 0,	/* has not started */
	USBVM_REMOTE_STATUS_BUSY = 1,	/* has started */
	USBVM_REMOTE_STATUS_START = 2, /* starting */
}USBVM_REMOTE_STATUS;
/* 
 * brief	Tell cm to change remoteUsbvmStatus in g_cmEndpt[endpt]
 */
typedef struct _CMSIP_USBVM_REMOTE_STATUS_CHANGE
{
	int status;		/* a value in CMSIP_USBVM_REMOTE_STATE */
	int endpt;		/* The endpt num  */
	unsigned int globalWavTimeName;	/* The file name that will be transformed to wav */
}CMSIP_USBVM_REMOTE_STATUS_CHANGE;

/* 
 * brief	The request from PJSIP to CM for changing USBVM endpoint timer
 *			Type:CMSIP_REQUEST_USBVM_CHANGE_TIMER
 */
typedef struct _CMSIP_USBVM_CHANGE_TIMER
{
	int callIndex;     /* record ID */
	int endpt;      /* endpoint ID */
	int timerValue; /* value of endpoint timer */
} CMSIP_USBVM_CHANGE_TIMER;

/* 
 * brief	The request from PJSIP to CM for generating local ONHOOK event
 *			Type:CMSIP_REQUEST_USBVM_GENERATE_EVTONHOOK
 */
typedef struct _CMSIP_USBVM_GENERATE_EVTONHOOK
{
	int callIndex; /* call ID */
	int endpt;  /* endpoint ID */
} CMSIP_USBVM_GENERATE_EVTONHOOK;

/* 
 * brief	The request from PJSIP to CM for processing detected DTMF event
 *			Type:CMSIP_REQUEST_DETECT_REMOTE_DTMF
 */
typedef struct _CMSIP_DETECT_REMOTE_DTMF
{
	int callIndex;  /* call ID */
	CMSIP_DTMF_SEND_TYPE	type; /*In band or rfc2833 or sip info*/
	int dtmfNum; /* DTMF number from 0 to 11 */
} CMSIP_DETECT_REMOTE_DTMF;

/* 
 * brief	Size of a Message packet, the size of CMSIP_SIP_INCALL is the biggest
 */
#define MAX_MSG_LEN	sizeof(CMSIP_SIP_INCALL)
/* 
 * brief	Length of message body, compute according the size of the maximum message packet define
 */
#define MAX_MSGBODY_LEN (MAX_MSG_LEN - sizeof(int))
/* 
 * brief	Only present in the data struct 'CMSIP_SIP_MWINOTIRY'.
 *			The value of this macro must be computed according to MAX_MSG_LEN and 
 *			the size of the other data members of 'CMSIP_SIP_MWINOTIFY'
 */
#define MAX_MSG_SUM_LEN (MAX_MSG_LEN - sizeof(int) * 2)
/* 
 * brief	Only present in the data struct 'CMSIP_SIP_INGENREQ'.
 *			The value of this macro must be computed according to MAX_MSG_LEN and 
 *			the size of the other data members of 'CMSIP_SIP_INGENREQ'
 */
#define MAX_GENREQ_LEN (MAX_MSG_LEN - sizeof(int) * 2 - MAX_URI_LEN - MAX_METHOD_LEN)
/* 
 * brief	Only present in the data struct 'CMSIP_SIP_INGENRES'.
 *			The value of this macro must be computed according to MAX_MSG_LEN and 
 *			the size of the other data members of 'CMSIP_SIP_INGENRES'
 */
#define MAX_GENRES_LEN 	(MAX_MSG_LEN - sizeof(int) * 3)

/*************************Common Message Type define**************************/
/*************Note:A Message may be consisted by serval pakcets***************/
/* 
 * brief	Type of a Message pakcet to show what a packet is
 */
typedef unsigned int CMSIP_MSGTYPE;
/* 
 * brief	Common define of a Message packet
 */
typedef struct _CMSIP_MSG
{
	CMSIP_MSGTYPE type;
	char body[MAX_MSGBODY_LEN];
}CMSIP_MSG;

/* 
 * brief	Receive a Notify of MWI Subscribe
 *			Type:CMSIP_REQUEST_SIP_MWINOTIFY
 */
typedef struct  _CMSIP_SIP_MWINOTIFY
{
	int accId;
	int notifyType;  /*MWI / PRESENCE/бнбн*/
	char data[MAX_MSG_SUM_LEN];
}CMSIP_SIP_MWINOTIFY;

/*//--4--Generic*/
/* 
 * brief	Generic request,for the requests not defined here.
 *			Type:CMSIP_REQUEST_SIP_GENREQ/CMSIP_REQUEST_SIP_INGENREQ
 */
typedef struct  _CMSIP_SIP_GENREQ
{
	int	accId; /*default is -2*/
	int callId; /*default is -1*/
	char	destUri[MAX_URI_LEN];
	char	method[MAX_METHOD_LEN]; /*letters in the Method must be capital*/
	char	data[MAX_GENREQ_LEN];
}CMSIP_SIP_GENREQ;
/* 
 * brief	Receive incoming Generic Response
 *			Type:CMSIP_RESPONSE_SIP_GENRES/CMSIP_RESPONSE_SIP_INGENRES
 */
typedef struct _CMSIP_SIP_GENRES
{
	int accId;
	int callId;
	int statusCode;
	char  data[MAX_GENRES_LEN];
}CMSIP_SIP_GENRES;

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                           FUNCTIONS                                            */
/**************************************************************************************************/

 

#endif	/* __CMSIP_MSGDEF_H__ */

