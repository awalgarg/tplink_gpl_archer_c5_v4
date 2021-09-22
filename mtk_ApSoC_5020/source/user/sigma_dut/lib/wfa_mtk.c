#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/types.h>
#include <linux/socket.h>
#include <linux/wireless.h>
#include <poll.h>

#include "wfa_portall.h"
#include "wfa_debug.h"
#include "wfa_main.h"
#include "wfa_types.h"
//#include "wfa_dut.h"
#include "wfa_sock.h"
#include "wfa_tg.h"
#include "wfa_stdincs.h"
#include "wfa_mtk.h"

#define printf(x...)    fprintf(stderr, x)
extern unsigned short wfa_defined_debug;
pwfa_driver_mtk_config pdrv_mtk_cfg = NULL;


#ifdef TGAC_DAEMON	
extern char* wireless_intf;
extern char* wireless_dut_ip;
extern char dut_mac[];
#define SERV_PORT 5134
#define MAXDATA   1024
#define MAXNAME 1024
#endif

#ifdef TGAC_DAEMON	
void wfa_driver_mtk_interface_name_send(void)
{
		 int fd;      /* fd into transport provider */
		 int i;      /* loops through user name */
		 int length;     /* length of message */
		 int fdesc;     /* file description */
		 int ndata;     /* the number of file data */
		 char data[MAXDATA]; /* read data form file */
		 char data1[MAXDATA];  /*server response a string */
		 char buf[BUFSIZ];     /* holds message from server */
		 struct hostent *hp;   /* holds IP address of server */
		 struct sockaddr_in myaddr;   /* address that client uses */
		 struct sockaddr_in servaddr; /* the server's full addr */
		 //char *tmpdata="OID_802_11_BSSID";
		 char tmpdata[64];


		if (wireless_dut_ip == NULL){
		  perror ("wireless_dut_ip is NULL!");
		  return;
		 }
		
		if (wireless_intf == NULL){
		  perror ("wireless_intf is NULL!");
		  return;
		 }


		 /*
		  *  Get a socket into TCP/IP
		  */
		 if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		  perror ("socket failed!");
		  return;
		 }
 
		 bzero((char *)&myaddr, sizeof(myaddr));
		 myaddr.sin_family = AF_INET;
		 myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		 myaddr.sin_port = htons(0);


		 if (bind(fd, (struct sockaddr *)&myaddr,
		   sizeof(myaddr)) <0) {
		  perror("bind failed!");
		  return;
		 }
		 /*
		  * Fill in the server's address and the data.
		  */


		 bzero((char *)&servaddr, sizeof(servaddr));
		 servaddr.sin_family = AF_INET;
		 servaddr.sin_port = htons(SERV_PORT);


		 hp = gethostbyname(wireless_dut_ip);
		 if (hp == 0) {
		  fprintf(stderr, 
		   "could not obtain address of %s\n",wireless_dut_ip);
		  return (-1);
		 } else
		 	printf("obtain address of %s\n",wireless_dut_ip);


		 bcopy(hp->h_addr_list[0], (caddr_t)&servaddr.sin_addr, 
		  hp->h_length);
		 /*
		  * Connect to the server連線.
		  */
		 if (connect(fd, (struct sockaddr *)&servaddr, 
		    sizeof(servaddr)) < 0) {
		  perror("connect failed!");
		  return;
		 } else
		 printf("connect %s success\n",wireless_dut_ip);

	 	memset(tmpdata, 0,64);

		sprintf(tmpdata,"INTERFACE_NAME=%s",wireless_intf);
		 if (write(fd, tmpdata, strlen(tmpdata)) == -1) {
			perror("write to server error !");
			return;
		}

		memset(data1, 0,MAXDATA);
		if (read(fd, data1, MAXDATA) == -1) {
			perror ("read from server error !");
			return;
		}

		printf("data1=%s\n", data1);
		sprintf(dut_mac, "%s", data1);
		printf("dut_mac=%s\n", dut_mac);
		printf("fd =%d\n",fd);

		close (fd);

}


void wfa_driver_mtk_sock_send( char *pCmdStr, int op, char* tmp)
{
		 int fd;      /* fd into transport provider */
		 int i;      /* loops through user name */
		 int length;     /* length of message */
		 int fdesc;     /* file description */
		 int ndata;     /* the number of file data */
		 char data[MAXDATA]; /* read data form file */
		 char data1[MAXDATA];  /*server response a string */
		 char buf[BUFSIZ];     /* holds message from server */
		 struct hostent *hp;   /* holds IP address of server */
		 struct sockaddr_in myaddr;   /* address that client uses */
		 struct sockaddr_in servaddr; /* the server's full addr */
		 //char *tmpdata="OID_802_11_BSSID";
		 char tmpdata[64];
		 

		if (wireless_dut_ip == NULL){
		  perror ("wireless_dut_ip is NULL!");
		  return;
		 }
		 /*
		  *  Get a socket into TCP/IP
		  */
		 if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		  perror ("socket failed!");
		  return;
		 }
 
		 bzero((char *)&myaddr, sizeof(myaddr));
		 myaddr.sin_family = AF_INET;
		 myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		 myaddr.sin_port = htons(0);


		 if (bind(fd, (struct sockaddr *)&myaddr,
		   sizeof(myaddr)) <0) {
		  perror("bind failed!");
		  return;
		 }
		 /*
		  * Fill in the server's address and the data.
		  */


		 bzero((char *)&servaddr, sizeof(servaddr));
		 servaddr.sin_family = AF_INET;
		 servaddr.sin_port = htons(SERV_PORT);


		 hp = gethostbyname(wireless_dut_ip);
		 if (hp == 0) {
		  fprintf(stderr, 
		   "could not obtain address of %s\n",wireless_dut_ip);
		  return (-1);
		 } else
		 	printf("obtain address of %s\n",wireless_dut_ip);


		 bcopy(hp->h_addr_list[0], (caddr_t)&servaddr.sin_addr, 
		  hp->h_length);
		 /*
		  * Connect to the server連線.
		  */
		 if (connect(fd, (struct sockaddr *)&servaddr, 
		    sizeof(servaddr)) < 0) {
		  perror("connect failed!");
		  return;
		 } else
		 printf("connect %s success\n",wireless_dut_ip);

	 	memset(tmpdata, 0,64);


		if (op==0)
		{
				sprintf(tmpdata,pCmdStr);
				 if (write(fd, tmpdata, strlen(tmpdata)) == -1) {
				  perror("write to server error !");
				  return;
				 }

				 memset(data1, 0,MAXDATA);
				 if (read(fd, data1, MAXDATA) == -1) {
				  perror ("read from server error !");
				  return;
				 }
				 printf("%s\n", data1);
				 printf("fd =%d\n",fd);
		} else if (op == 1){
				sprintf(tmpdata,pCmdStr);
				 if (write(fd, tmpdata, strlen(tmpdata)) == -1) {
				  perror("write to server error !");
				  return;
				 }

				 memset(tmp, 0,MAXDATA);
				 if (read(fd, tmp, MAXDATA) == -1) {
				  perror ("read from server error !");
				  return;
				 }
		}
		    close (fd);


}
#endif

void wfa_driver_mtk_sock_int()
{
	int s;
#if TGAC_DAEMON
		 int fd;      /* fd into transport provider */
		 int i;      /* loops through user name */
		 int length;     /* length of message */
		 int fdesc;     /* file description */
		 int ndata;     /* the number of file data */
		 char data[MAXDATA]; /* read data form file */
		 char data1[MAXDATA];  /*server response a string */
		 char buf[BUFSIZ];     /* holds message from server */
		 struct hostent *hp;   /* holds IP address of server */
		 struct sockaddr_in myaddr;   /* address that client uses */
		 struct sockaddr_in servaddr; /* the server's full addr */
		 char *tmpdata="OID_802_11_BSSID";
#endif

	DPRINT_INFO(WFA_OUT, "Entering wfa_driver_mtk_sock_int ...\n");

	pdrv_mtk_cfg = malloc(sizeof(wfa_driver_mtk_config));
	memset(pdrv_mtk_cfg, 0, sizeof(wfa_driver_mtk_config));
	/* open socket to kernel */
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s != -1)
	{
		pdrv_mtk_cfg->ioctl_sock = s;
		printf("%s(AF_INET)::  sock = %d.\n", __FUNCTION__, pdrv_mtk_cfg->ioctl_sock);
		return;
	}
	s = socket(AF_IPX, SOCK_DGRAM, 0);
	if (s != -1)
	{
		pdrv_mtk_cfg->ioctl_sock = s;
		printf("%s(AF_IPX)::  sock = %d.\n", __FUNCTION__, pdrv_mtk_cfg->ioctl_sock);
		return;
	}
	s = socket(AF_AX25, SOCK_DGRAM, 0);
	if (s != -1)
	{
		pdrv_mtk_cfg->ioctl_sock = s;
		printf("%s(AF_AX25)::  sock = %d.\n", __FUNCTION__, pdrv_mtk_cfg->ioctl_sock);
		return;
	}
	s = socket(AF_APPLETALK, SOCK_DGRAM, 0);
	if (s != -1)
	{
		pdrv_mtk_cfg->ioctl_sock = s;
		printf("%s(AF_APPLETALK)::  sock = %d.\n", __FUNCTION__, pdrv_mtk_cfg->ioctl_sock);
		return;
	}
}

void wfa_driver_mtk_sock_exit()
{
   DPRINT_INFO(WFA_OUT, "Entering wfa_driver_mtk_sock_exit ...\n");	
   close(pdrv_mtk_cfg->ioctl_sock);
#ifdef TGAC_DAEMON	 
   //close (pdrv_mtk_cfg->fd);
#endif
}

int wfa_driver_mtk_set_oid(char *ifname, int oid, char *data, int len)
{
	struct ifreq ifr;
	struct iwreq iwr;
	int err;

	memset (&iwr, 0, sizeof(iwr));
	strncpy(iwr.ifr_name, ifname, strlen(ifname));
	iwr.u.data.flags = OID_GET_SET_TOGGLE | oid;
	iwr.u.data.pointer = data;
	iwr.u.data.length = len;
	
	if (err = ioctl(pdrv_mtk_cfg->ioctl_sock, RT_PRIV_IOCTL, &iwr) < 0)
	{
		printf("ERR(%d)::  %s in 0x%04x.\n", err, __FUNCTION__, oid);
		return 0;
	}

	return 1;
}

int wfa_driver_mtk_get_oid(char *intf, int oid, unsigned char *data, int len)
{

	int s = 0;
	struct ifreq ifr;
	struct iwreq iwr;
	int err, Len, i;
#ifdef TGAC_DAEMON	
	char data1[MAXDATA];
	char *pCmdStr;
#endif
	memset (&iwr, 0, sizeof(iwr));
	strncpy(iwr.ifr_name, intf, strlen(intf));
	iwr.u.data.flags = oid;

	iwr.u.data.pointer = data;
	iwr.u.data.length = len;
	printf("wfa_driver_mtk_get_oid::  inf:%s in 0x%04x.\n", intf, oid);
	printf(stderr, "wfa_driver_mtk_get_oid::  inf:%s in 0x%04x.\n", intf, oid);
#ifdef TGAC_DAEMON

		if(oid ==OID_GEN_MEDIA_CONNECT_STATUS)
			pCmdStr="OID_GEN_MEDIA_CONNECT_STATUS";
		else if(oid ==OID_802_11_BSSID)
			pCmdStr="OID_802_11_BSSID";
		else
		{
			data=NULL;
			Len=0;
			return 0;

		}

		bzero(data1, MAXDATA);
		
		 if(oid ==OID_GEN_MEDIA_CONNECT_STATUS)
		 {
		 	int connected=1;
		 	memcpy(data,&connected, sizeof(int));
		 } else if(oid ==OID_802_11_BSSID) {
		 	wfa_driver_mtk_sock_send(pCmdStr, 1,data1);
		 	memcpy(data,data1, 8);
		 }
		 //printf("%s\n", data1);	
#else
	if (err = ioctl(pdrv_mtk_cfg->ioctl_sock, RT_PRIV_IOCTL, &iwr) < 0)
	{
		printf("ERR(%d)::  %s in 0x%04x.\n", err, __FUNCTION__, oid);
		return -1;
	} else 
		printf("SUCCESS(%d)::  %s in 0x%04x.\n", err, __FUNCTION__, oid);

	Len = iwr.u.data.length;
#endif	
	return 0;
}

int wfa_driver_mtk_set_cmd(char *pIntfName, char *pCmdStr)
{
	
#ifdef TGAC_DAEMON
		char data1[MAXDATA];
		int rv = 0;

		printf("-->wfa_driver_mtk_set_cmd (%s)\n",pCmdStr);
		wfa_driver_mtk_sock_send(pCmdStr,0 ,NULL);
	 
#else
	struct iwreq lwreq;	
	char *pBuffer = 0;
	int rv = 0;
	
	pBuffer = (char*) malloc (512*sizeof(char));
	if (pBuffer)
	{
		
		memset(pBuffer, 0, 512*sizeof(char));
		memcpy(pBuffer, pCmdStr, strlen(pCmdStr));
		sprintf(lwreq.ifr_ifrn.ifrn_name, pIntfName, strlen(pIntfName));
		lwreq.u.data.length = strlen(pBuffer) + 1;
		lwreq.u.data.pointer = (caddr_t) pBuffer;
		//printf("wfa_driver_mtk_set_cmd::  inf:%s \n", pIntfName);
		fprintf(stderr, "wfa_driver_mtk_set_cmd::  inf:%s \n", pIntfName);
		if(ioctl(pdrv_mtk_cfg->ioctl_sock, RTPRIV_IOCTL_SET, &lwreq) < 0)	
		{
			fprintf(stderr, "wfa_driver_mtk_set_cmd:: Interface doesn't accept private ioctl...(%s)\n", pBuffer);
			rv = -1;
		} else
			fprintf(stderr, "wfa_driver_mtk_set_cmd:: Interface accept private ioctl...(%s)\n", pBuffer);
	}
	
	if (pBuffer)
		free(pBuffer);
#endif	
	return rv;
}



