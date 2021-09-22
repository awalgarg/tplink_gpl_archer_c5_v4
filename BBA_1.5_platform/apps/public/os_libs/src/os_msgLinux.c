/*  Copyright(c) 2010-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file	os_msgLinux.c
 * brief	Linux general IPC method. 
 *
 * author	Yang Xv
 * version	1.0.0
 * date	28Apr11
 *
 * history 	\arg 1.0.0, 28Apr11, Yang Xv, Create the file.
 */
#ifdef __LINUX_OS_FC__

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>


#include <os_lib.h>
#include <os_msg.h>
#include <os_log.h>

/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/

/* 
 * brief Different OS have different path prefix
 */
//#define OS_PATH_PREFIX "/var/run/"
#define OS_PATH_PREFIX "/var/tmp/"


/* 
 * brief Max path length, should smaller than sockaddr_un.sun_path length	
 */
#define MAX_PATH_LEN	64


/* 
 * brief For VOIP	
 */
#ifdef INCLUDE_VOIP
#define CM_IS_STARTING         "/var/tmp/voip_starting"
#endif /* INCLUDE_VOIP */

/**************************************************************************************************/
/*                                           GLOBAL_FUNCTIONS                                     */
/**************************************************************************************************/

/* 
 * fn		int msg_init(CMSG_FD *pMsgFd)
 * brief	Create an endpoint for msg
 *	
 * param[out]	pMsgFd - return msg descriptor that has been create	
 *
 * return	-1 is returned if an error occurs, otherwise is 0
 *
 * note 	Need call msg_cleanup() when you no longer use this msg which is created by msg_init()
 */
int msg_init(CMSG_FD *pMsgFd)
{
	assert(pMsgFd != NULL);

	if (-1 == (pMsgFd->fd = socket(AF_LOCAL, SOCK_DGRAM, 0))) 	
	{		
		perror("socket");		
		return -1;	
	}
	
	/* set FD_CLOEXEC */
	if (-1 == fcntl(pMsgFd->fd, F_SETFD, 1))
	{
		perror("msg fcntl");
		close(pMsgFd->fd);
		return -1;
	}
	
	return 0;
}


/* 
 * fn		int msg_srvInit(CMSG_ID msgId, CMSG_FD *pMsgFd)
 * brief	Init an endpoint as a server and bind a name to this endpoint msg	
 *
 * param[in]	msgId - server name	
 * param[in]	pMsgFd - server endpoint msg fd
 *
 * return	-1 is returned if an error occurs, otherwise is 0	
 */
int msg_srvInit(CMSG_ID msgId, CMSG_FD *pMsgFd)
/* ����Ӧ���Ǵ���һ��name��֮��ƴ�ӳ�path����������ɿ����ߴ���name����Ϊ�������������ܻ���ֳ�ͻ
 * �������ֱ�Ӵ���ID����ID���ļ���
 */
{
	char path[MAX_PATH_LEN + 1] = {0};

	assert(pMsgFd != NULL);

	/* prepare domain path */
	snprintf(path, MAX_PATH_LEN, "%s%d", OS_PATH_PREFIX, msgId);
	DEBUG_PRINT("Server path is %s\n", path);
	unlink(path);
	
	/* bind socket to specify path */
	memset(&(pMsgFd->_localAddr), 0, sizeof(pMsgFd->_localAddr));
	pMsgFd->_localAddr.sun_family = AF_LOCAL;	
	strncpy(pMsgFd->_localAddr.sun_path, path, sizeof(pMsgFd->_localAddr.sun_path));	

	if (-1 == bind(pMsgFd->fd, (struct sockaddr *)(&(pMsgFd->_localAddr)), 
		SUN_LEN(&(pMsgFd->_localAddr))))
	{		
		perror("bind");		
		return -1;	
	}
	
	return 0;
}



/* 
 * fn		int msg_connSrv(CMSG_ID msgId, CMSG_FD *pMsgFd)
 * brief	Init an endpoint as a client and specify a server name	
 *
 * param[in]		msgId - server name that we want to connect	
 * param[in/out]	pMsgFd - client endpoint msg fd	
 *
 * return	-1 is returned if an error occurs, otherwise is 0
 */
int msg_connSrv(CMSG_ID msgId, CMSG_FD *pMsgFd)
{
	char path[MAX_PATH_LEN + 1] = {0};

	assert(pMsgFd != NULL);

	/* prepare domain path */
	snprintf(path, MAX_PATH_LEN, "%s%d", OS_PATH_PREFIX, msgId);
	DEBUG_PRINT("Client connect server path is %s\n", path);
	
	/* bind socket to specify path */
	memset(&(pMsgFd->_remoteAddr), 0, sizeof(pMsgFd->_remoteAddr));
	pMsgFd->_remoteAddr.sun_family = AF_LOCAL;	
	strncpy(pMsgFd->_remoteAddr.sun_path, path, sizeof(pMsgFd->_remoteAddr.sun_path));

	return 0;
}


/* 
 * fn		int msg_recv(const CMSG_FD *pMsgFd, CMSG_BUFF *pMsgBuff)
 * brief	Receive a message form a msg	
 *
 * param[in]	pMsgFd - msg fd that we want to receive message
 * param[out]	pMsgBuff - return recived message
 *
 * return	-1 is returned if an error occurs, otherwise is 0
 *
 * note		we will clear msg buffer before recv
 */
int msg_recv(const CMSG_FD *pMsgFd, CMSG_BUFF *pMsgBuff)
/* ����ʹ��_remoteAddr��Ϊ�˺�������ʹ��msg_send�����(msg_sendAndGetReplyWithTimeout()) */
{
	int len = sizeof(pMsgFd->_remoteAddr);
	
	assert((pMsgFd != NULL) && (pMsgBuff != NULL));

	memset(pMsgBuff, 0, MSG_SIZE);
	
	if (-1 == recvfrom(pMsgFd->fd, pMsgBuff, MSG_SIZE, 0,
			(struct sockaddr *)(&(pMsgFd->_remoteAddr)), (socklen_t *)&len))
	{
		perror("recvfrom");
		return -1;
	}

	return 0;
}


/* 
 * fn		int msg_send(const CMSG_FD *pMsgFd, const CMSG_BUFF *pMsgBuff)
 * brief	Send a message from a msg	
 *
 * param[in]	pMsgFd - msg fd that we want to send message	
 * param[in]	pMsgBuff - msg that we wnat to send
 *
 * return	-1 is returned if an error occurs, otherwise is 0
 *
 * note 	This function will while call sendto() if sendto() return ENOENT error
 */
int msg_send(const CMSG_FD *pMsgFd, const CMSG_BUFF *pMsgBuff)
/* ���sendto()���ص���ENOENT����ô���ǽ���������sendto��ֱ���ɹ����߷�����������Ϊֹ
 * ��ҪĿ�������������������δ���������ǿͻ����Ѿ����������(��Ҫ��ϵͳ��ʼ����ʱ��)
 * ������һ�ֿ��ܣ�����������׽��ֵ�·���Ǵ��ڵģ����Ƿ������Ѿ��رգ���ʱ������
 */
{
	int ret = 0;
	int flag = 0;
	
	assert((pMsgFd != NULL) && (pMsgBuff != NULL));

#ifdef INCLUDE_VOIP	
	char cmd[MAX_PATH_LEN];
	/* 
	 * brief	NOTE !! : These messages will lead to the restart of CM, so CMM MUST remember it
	 *					and MUST NOT try to get the CM status if any flash writing is to be issued.
	 *					Otherwise, CMM will lock the DM, send request to CM and wait reply from CM.
	 *					unfortunately, the CM is starting and it want to access the DM but the DM
	 *					is locked, so CMM can not get the replay from CM and it will delete the
	 *					socket and return after timeout. After the DM is unlocked, CM will enter
	 *					into dead loop to send replay to CMM because of the socket error.
	 */
	if (pMsgBuff->type >= CMSG_VOIP_WAN_STS_CHANGED && pMsgBuff->type <= CMSG_VOIP_RESTART_CALLMGR)
	{
		sprintf(cmd, "echo 1 > %s", CM_IS_STARTING);
		do
		{
			system(cmd);
			usleep(1 * 1000);
		} while (access(CM_IS_STARTING, F_OK) == -1);
	}
#endif /* INCLUDE_VOIP */	

	/* Don't wait log message when queue is full 
	 * added by yangxv,2012.9.29
	 */
	if (pMsgBuff->type == CMSG_LOG)
	{
		flag = MSG_DONTWAIT;
	}
	
	do
	{
		if (pMsgBuff->type == CMSG_LOG)
		{
			ret = sendto(pMsgFd->fd, pMsgBuff, LOG_MSG_SIZE, flag, 
						(struct sockaddr *)(&(pMsgFd->_remoteAddr)), SUN_LEN(&(pMsgFd->_remoteAddr)));
		}
		else
		{
			ret = sendto(pMsgFd->fd, pMsgBuff, MSG_SIZE, flag, 
					(struct sockaddr *)(&(pMsgFd->_remoteAddr)), SUN_LEN(&(pMsgFd->_remoteAddr)));
		}

		if ((-1 == ret) && (ENOENT == errno))
		{
			usleep(10000);
			perror("sendto");
			printf("pid %d send %d error\n", getpid(), pMsgBuff->type);
		}
	}while((-1 == ret) && (ENOENT == errno || EINTR == errno || EAGAIN == errno));

	if (-1 == ret)
	{
#ifdef INCLUDE_VOIP	
		if (pMsgBuff->type >= CMSG_VOIP_WAN_STS_CHANGED && pMsgBuff->type <= CMSG_VOIP_RESTART_CALLMGR)
		{
			sprintf(cmd, "rm -f %s", CM_IS_STARTING);
			do
			{
				system(cmd);
				usleep(1 * 1000);
			} while(access(CM_IS_STARTING, F_OK) == 0);
		}
#endif /* INCLUDE_VOIP */	
		perror("sendto");
		printf("pid %d send %d error, errno is %d\n", getpid(), pMsgBuff->type, errno);
		return -1;
	}
#ifdef INCLUDE_VOIP	
	if (pMsgBuff->type >= CMSG_VOIP_WAN_STS_CHANGED && pMsgBuff->type <= CMSG_VOIP_RESTART_CALLMGR)
	{
		/* 
		 * brief	let CM delete the temporary file in time, so CMM can send msg to CM as normal
		 */
		usleep(500 * 1000);
	}
#endif /* INCLUDE_VOIP */	
	
	return 0;
}


/* 
 * fn		int msg_cleanup(CMSG_FD *pMsgFd)
 * brief	Close a message fd
 * details	
 *
 * param[in]	pMsgFd - message fd that we want to close		
 *
 * return	-1 is returned if an error occurs, otherwise is 0		
 */
int msg_cleanup(CMSG_FD *pMsgFd)
{
	assert(pMsgFd != NULL);

	if (-1 == close(pMsgFd->fd))
	{
		perror("close");		
		return -1;		
	}

	/* TODO:
	 * ������Կ�������ɾ�����׽��ֵ�·�����������ڷ�������������֮��Ͳ����˳���
	 * �����ʱ���Բ�����
	 */

	return 0;
}


/* 
 * fn		int msg_connCliAndSend(CMSG_ID msgId, CMSG_FD *pMsgFd, CMSG_BUFF *pMsgBuff)
 * brief	init a client msg and send msg to server which is specified by msgId	
 *
 * param[in]	msgId -	server ID that we want to send
 * param[in]	pMsgFd - message fd that we want to send
 * param[in]	pMsgBuff - msg that we wnat to send
 *
 * return	-1 is returned if an error occurs, otherwise is 0	
 */
int msg_connCliAndSend(CMSG_ID msgId, CMSG_FD *pMsgFd, CMSG_BUFF *pMsgBuff)
/* ����server������Ϣ���Ĳ�������װ����������ʹ�� */
{
	assert((pMsgFd != NULL) && (pMsgBuff != NULL));
	
	if (msg_init(pMsgFd) < 0)
	{
		printf("Init %d msg client error\n", msgId);
		return -1;
	}
	
	if (msg_connSrv(msgId, pMsgFd) < 0)
	{
		printf("Connect %d msg client error\n", msgId);
		msg_cleanup(pMsgFd);
		
		return -1;
	}

	msg_send(pMsgFd, pMsgBuff);

	msg_cleanup(pMsgFd);

	return 0;
}


/* 
 * fn		int msg_sendAndGetReply(CMSG_FD *pMsgFd, CMSG_BUFF *pMsgBuff)
 * brief	
 *
 * param[in]	pMsgFd - msg fd that we want to use
 * param[in/out]pMsgBuff - send msg and get reply
 * param[in]	timeSeconds - timeout in second
 *
 * return	-1 is returned if an error occurs, otherwise is 0	
 */
int msg_sendAndGetReplyWithTimeout(CMSG_FD *pMsgFd, CMSG_BUFF *pMsgBuff, int timeSeconds)
{
	int fd = 0;
	int ret = 0;
	struct sockaddr_un tmpAddr;
	char tempFile[]= "/var/tmp_XXXXXX";
	struct timeval tv;
	int errHappen = 0;

	assert((pMsgFd != NULL) && (pMsgBuff != NULL));

	if (-1 == (fd = mkstemp(tempFile)))
	{
		perror("mkstemp");
		return -1;
	}
	
	memset(&tmpAddr, 0, sizeof(tmpAddr));
	tmpAddr.sun_family = AF_LOCAL;
	strncpy(tmpAddr.sun_path, tempFile, sizeof(tmpAddr.sun_path));
	unlink(tmpAddr.sun_path);

	/* bind socket to a temp path for recive */
	if (-1 == bind(pMsgFd->fd, (struct sockaddr *)&tmpAddr, SUN_LEN(&tmpAddr))) 	
	{		
		perror("bind");		
		return -1;	
	}

	do
	{
		ret = sendto(pMsgFd->fd, pMsgBuff, MSG_SIZE, 0, 
				(struct sockaddr *)(&(pMsgFd->_remoteAddr)), SUN_LEN(&(pMsgFd->_remoteAddr)));

		if ((-1 == ret) && (ENOENT == errno))
		{
			usleep(10000);
			perror("sendto");
			printf("send %d error %d, %d\n", pMsgBuff->type, getpid(), ret);
		}
	}while((-1 == ret) && (ENOENT == errno));

	if (-1 == ret)
	{
		perror("sendto");
		errHappen = 1;
		goto error;		
	}

	/* set recvfrom timeout */
	tv.tv_sec = timeSeconds;
	tv.tv_usec = 0;
	
	if (-1 == setsockopt(pMsgFd->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)))
	{
		perror("setsockopt");
		errHappen = 1;
		goto error;
	}

	if (-1 == recvfrom(pMsgFd->fd, pMsgBuff, MSG_SIZE, 0, NULL, NULL))
	{
		perror("recvfrom");
		errHappen = 1;
		goto error;
	}

error:

	/* set recvfrom no timeout */
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	
	if (-1 == setsockopt(pMsgFd->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)))
	{
		perror("setsockopt");
	}

	/* delete file after get reply */
	close(fd);
	unlink(tmpAddr.sun_path);

	/* rebind method not found,so close and create socket again, ugly I know... */
	close(pMsgFd->fd);

	if (-1 == (pMsgFd->fd = socket(AF_LOCAL, SOCK_DGRAM, 0))) 	
	{		
		perror("new socket");		
		return -1;	
	}

	if (1 == errHappen)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

#endif /* __LINUX_OS_FC__ */

