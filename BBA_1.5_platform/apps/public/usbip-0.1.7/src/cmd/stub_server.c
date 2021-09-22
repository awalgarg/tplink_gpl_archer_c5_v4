/*
 * Copyright (C) 2005-2006 Takahiro Hirofuchi
 */

#include <unistd.h>
#include <netdb.h>
#include <strings.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/un.h>

#ifdef HAVE_LIBWRAP
#include <tcpd.h>
#endif
#define _GNU_SOURCE
#include <getopt.h>
#include <signal.h>
#include <time.h>

#include "usbip.h"
#include "usbip_network.h"

/* added by tf, 110507, for udp broadcasting thread */
#include <pthread.h>

#define MAX 2
//modify by chz for TD-W8968V1. 2011-12-14
//#define UNIX_PATH			 "/var/printSev.state"
//#define USBIP_STATE_PATH	"/tmp/usbip_serv.state"
#define USBIP_STATE_PATH	"/var/usbip_serv.state"
//end modify

#define PATTERN_MAX_LEN	33
#define SSID_MAX_LEN	33
#define BCAST_MTU	1500	/* can support up to 10 equipment informations, it's enough! considerd by tf, 110509  */
static u_char bcast_pkt_buf[BCAST_MTU]; /* buffer for broadcasting packet , added by tf, 110509 */
#if 0
static const char version[] = "$Id: stub_server.c 97 2006-03-31 16:08:40Z taka-hir $";
#else
static const char version[] = "$Id: stub_server.c 97 2011-05-09 08:40:40Z tf $";
#endif

#define LAN_INTF_NAME	"br0"

static struct usbip_stub_driver *StubDriver;
/* added by tf, 110510 */
static uint8_t Stub_online = 1;
static uint8_t isBusy_now = 0;
const int	bcast_period = 2000;	/* milliseconds */
struct dev_list	*devlist = NULL;
/* Modify by chz for IPv6 host name, 2013-05-30 */
//char host[NI_MAXHOST];	/* record ipaddr of Client */
char host[USBIP_MAXHOST];
/* end modify */
static char pattern_str[PATTERN_MAX_LEN];	/* for broadcasting pattern of router product */
static char ssid_str[SSID_MAX_LEN];	/* for broadcasting ssid */

static int send_reply_devlist(struct usbip_stub_driver *driver, int sockfd)
{
	int ret;
	struct usbip_exported_device *edev;
	struct op_devlist_reply reply;


	reply.ndev = 0;

	/* how many devices are exported ? */
	dlist_for_each_data(driver->edev_list, edev, struct usbip_exported_device) {
		reply.ndev += 1;
	}

	dbg("%d devices are exported", reply.ndev);

	ret = usbip_send_op_common(sockfd, OP_REP_DEVLIST,  ST_OK);
	if(ret < 0) {
		ps_err("send op_common");
		return ret;
	}

	PACK_OP_DEVLIST_REPLY(1, &reply);

	ret = usbip_send(sockfd, (void *) &reply, sizeof(reply));
	if(ret < 0) {
		ps_err("send op_devlist_reply");
		return ret;
	}

	dlist_for_each_data(driver->edev_list, edev, struct usbip_exported_device) {
		struct usb_device pdu_udev;

		dump_usb_device(&edev->udev);
		memcpy(&pdu_udev, &edev->udev, sizeof(pdu_udev));
		pack_usb_device(1, &pdu_udev);

		ret = usbip_send(sockfd, (void *) &pdu_udev, sizeof(pdu_udev));
		if(ret < 0) {
			ps_err("send pdu_udev");
			return ret;
		}

		for(int i=0; i < edev->udev.bNumInterfaces; i++) {
			struct usb_interface pdu_uinf;

			dump_usb_interface(&edev->uinf[i]);
			memcpy(&pdu_uinf, &edev->uinf[i], sizeof(pdu_uinf));
			pdu_uinf.padding = 0;
			pack_usb_interface(1, &pdu_uinf);

			ret = usbip_send(sockfd, (void *) &pdu_uinf, sizeof(pdu_uinf));
			if(ret < 0) {
				ps_err("send pdu_uinf");
				return ret;
			}
		}
	}

	return 0;
}

static int recv_request_reserve(int sockfd)
{
	int ret;
	struct op_import_request req;
	struct op_common reply;
	struct usbip_exported_device *edev;
	int found = 0;
	int error = 0;

	bzero(&req, sizeof(req));
	bzero(&reply, sizeof(reply));

	ret = usbip_recv(sockfd, (void *) &req, sizeof(req));
	if(ret < 0) {
		ps_err("recv import request");
		return -1;
	}

	PACK_OP_IMPORT_REQUEST(0, &req);

	dlist_for_each_data(StubDriver->edev_list, edev, struct usbip_exported_device) {
		if(!strncmp(req.busid, edev->udev.busid, SYSFS_BUS_ID_SIZE)) {
			dbg("found requested device %s", req.busid);
			found = 1;
			break;
		}
	}

	if(found) {
		ret = usbip_stub_reserve_device(edev);
		if(ret < 0) 
		{
			/*err("tf says: export_device err %d", ret);*/	//added by tf, 110512
			error = -ret;
		}
	} else {
		info("not found requested device %s", req.busid);
		error = 4;
	}

	/* error:
		1: device busy;		2: device error;
		3: not the first device	4: the device is absent
	*/
	ret = usbip_send_op_common(sockfd, OP_REP_RESERVE, (uint32_t)error);
	if(ret < 0) {
		err("send reserve reply");
		return -1;
	}
	
	return 0;
}

static int recv_request_devlist(int sockfd)
{
	int ret;
	struct op_devlist_request req;

	bzero(&req, sizeof(req));

	ret = usbip_recv(sockfd, (void *) &req, sizeof(req));
	if(ret < 0) {
		ps_err("recv devlist request");
		return -1;
	}

	ret = send_reply_devlist(StubDriver, sockfd);
	if(ret < 0) {
		err("send devlist reply");
		return -1;
	}

	return 0;
}


static int recv_request_import(int sockfd)
{
	int ret;
	struct op_import_request req;
	struct op_common reply;
	struct usbip_exported_device *edev;
	int found = 0;
	int error = 0;

	bzero(&req, sizeof(req));
	bzero(&reply, sizeof(reply));

	ret = usbip_recv(sockfd, (void *) &req, sizeof(req));
	if(ret < 0) {
		ps_err("recv import request");
		return -1;
	}

	PACK_OP_IMPORT_REQUEST(0, &req);

	dlist_for_each_data(StubDriver->edev_list, edev, struct usbip_exported_device) {
		if(!strncmp(req.busid, edev->udev.busid, SYSFS_BUS_ID_SIZE)) {
			dbg("found requested device %s", req.busid);
			found = 1;
			break;
		}
	}

	if(found) {
		usbip_set_nodelay(sockfd);
	} else {
		info("not found requested device %s", req.busid);
		error = 1;
	}

	ret = usbip_send_op_common(sockfd, OP_REP_IMPORT, (!error ? ST_OK : ST_NA));
	if(ret < 0) {
		err("send import reply");
		return -1;
	}
	
	if(!error) {
		struct usb_device pdu_udev;

		memcpy(&pdu_udev, &edev->udev, sizeof(pdu_udev));
		pack_usb_device(1, &pdu_udev);

		ret = usbip_send(sockfd, (void *) &pdu_udev, sizeof(pdu_udev));
		if(ret < 0) {
			ps_err("send devinfo");
			return -1;
		}
	}

	/* we write socket to threads in stub driver here. if we write it too early, kernel may release the socket at once,
	  * so usbip_send_op_common/usbip_send may causes  kernel panic.
	  * added by tf, 111203
	  */
	/* export_device needs a TCP/IP socket descriptor */
	ret = usbip_stub_export_device(edev, sockfd);
	if(ret < 0) 
	{
		ps_err("export device failure! err:%d\n", ret);
		return -2;
	}

	return 0;
}

static void usbip_refresh_waiting_list(void)
{
	if (devlist == NULL)
		return;
	
	struct dev_list *tmp, *tmp2, *head;
	struct waiting_list *cli, *cli_tmp;
	struct usbip_exported_device *edev;
	int status = SDEV_ST_USED;
	int absent = 1;	
	head = devlist;
	for(tmp = devlist; tmp != NULL; )		/* delete absent devices */
	{
		absent = 1;		/* Is one dev in devlist absent ? */
		dlist_for_each_data(StubDriver->edev_list, edev, struct usbip_exported_device)
		{
			if (!strcmp(tmp->busid, edev->udev.busid))
			{
				/*ps_info("busid %s is alive!\n", tmp->busid);*/
				absent = 0;
				status = edev->status;
				goto out;
			}
		}
		out:
			if (absent)
			{
				/*
				ps_info("busid %s is absent now!\n", tmp->busid);
				*/
				for (cli = tmp->client; cli != NULL; )
				{
					cli_tmp = cli->next;
					free(cli);
					cli = cli_tmp;
				}
				if (devlist == tmp)
				{
					devlist = tmp->next;
					free(tmp);
					tmp = devlist;
					head = devlist;
				}
				else
				{
					tmp2 = tmp->next;
					head->next = tmp2;
					free(tmp);
					tmp = tmp2;
				}
			}
			else
			{
				tmp->status = status;
				head = tmp;
				tmp = tmp->next;	
			}
	}

	for(tmp = devlist; tmp != NULL; tmp = tmp->next)
	{
		if (tmp->status != SDEV_ST_AVAILABLE)
		{
			/*ps_info("busid %s is busy now!\n", tmp->busid);*/
			continue;
		}
		cli = tmp->client;
		/*dbg("cli->hostname = %s ", cli->hostname);*/
		if (cli == NULL)
			return;
		if (cli->status == 0)	/* first time! mark the first client! */
		{
			/*
			ps_info("busid %s is available! Mark 1st client: %s\n", tmp->busid, cli->hostname);
			*/
			cli->status = 1;
		}
		else if (cli->status == 1) /* second time ! delete the first client and mark the next client!*/
		{
			/*ps_info("Delete current 1st client: %s\n", cli->hostname);*/
			tmp->client = cli->next;
			free(cli);
			if (tmp->client)
			{
				/*ps_info("The next client: %s\n", tmp->client->hostname);*/
				tmp->client->status = 1;
			}
		}
		else
		{
			ps_info("The 1st client's illegal status: %d\n", cli->status);
		}
	}

	return;
}

#if 0
static void usbip_send_info(int state)
{
	int sockfd;
	int ret;
	struct sockaddr_un servaddr;
	char buf[MAX] = {0};

	memset(buf, 0, MAX);
	buf[0] = (int8_t)state;

	sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		/*dbg("socket:%s", strerror(errno));*/
		return;
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strncpy(servaddr.sun_path, UNIX_PATH, strlen(UNIX_PATH));	 

	ret =  connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if (ret < 0)
	{
		close(sockfd);
		return;
	}
	
	ret = send(sockfd, buf, 2, 0);
	if (ret < 0)
	{
		close(sockfd);
		return;
	}

	close(sockfd);
	/*dbg("Send %d bytes!", ret);*/
	return;
}
#endif

/***************************************************************************
 * FUNCTION       :       usbip_device_state()
 * AUTHOR          :        TengFei
 * OCCASION      :        
 * DESCRIPTION  :      sent states of usbip devices to webserver.
 *
 * INPUT             :       N/A
 * OUTPUT          :       N/A
 * RETURN          :       N/A
 ***************************************************************************/
static void usbip_device_state(void)
{
	int state = SDEV_ST_AVAILABLE;
	static int counter = 0;
	char cmd_str[64];
	
#define	BUSY		1
#define	IDLE		0
#define	OFFLINE		0xf

	if (!Stub_online)
	{
		if (++counter < 30 && isBusy_now == OFFLINE)	
		{
			return;
		}
		isBusy_now = OFFLINE;
		goto next;	
	}
	
	struct usbip_exported_device *edev;
	dlist_for_each_data(StubDriver->edev_list, edev, struct usbip_exported_device)
	{
		if (edev->status != SDEV_ST_AVAILABLE)
			state = edev->status;
	}

	if ( BUSY != isBusy_now && state != SDEV_ST_AVAILABLE)
	{
		isBusy_now = BUSY;
	}
	else if (isBusy_now != IDLE && state == SDEV_ST_AVAILABLE)
	{
		isBusy_now = IDLE;
	}
	/*
	 * 由于同步问题而导致的web 页面打印状态不准确的情况，
	 * 所以这里等待1 minute (30*2s)    后，无论如何都会向USBIP_STATE_PATH
	 * 打印机目前的状态。
	 * by tf, 110815
	 */
	else if (++counter < 30)	
	{
		return;
	}

	/* now we deliver the status of Print Serv using file instead of unix socket. 111130, tf */
#if 0
	usbip_send_info(state);
#endif

next:

	memset(cmd_str, 0, sizeof(cmd_str));
	if (isBusy_now == BUSY)
		sprintf(cmd_str, "echo busy > "USBIP_STATE_PATH);
	else if (isBusy_now == IDLE)
		sprintf(cmd_str, "echo idle > "USBIP_STATE_PATH);
	else if (isBusy_now == OFFLINE)
		sprintf(cmd_str, "echo offline > "USBIP_STATE_PATH);

	system(cmd_str);

	counter = 0;
	return ;
}

#if 0
/* added by tf, 110531, please modify here when placing usbip in a new pattern of our router product. */
enum 
{
	WR1043N,
	WR842N,
	WR2543N
} tp_pro = WR1043N;

/* pattern list of our company router product */
static const char* pattern[] = {
	"TL-WR1043ND",
	"TL-WR842ND",
	"TL-WR2543ND",
	""
};
#endif

/***************************************************************************
 * FUNCTION       :       	usbip_fill_bcastpkt()
 * AUTHOR          :        TengFei
 * OCCASION      :         
 * DESCRIPTION  :      	fill up broadcasting packet
 *
 * INPUT             :       N/A
 * OUTPUT          :       N/A
 * RETURN          :       N/A
 ***************************************************************************/
 static uint32_t usbip_fill_bcastpkt(void)
{
#define PUTCHAR(c, cp, len) { \
	*(cp)++ = (u_char) (c); \
	(len) += 1;				\
}
#define PUTSHORT(s, cp, len) { \
	*(cp)++ = (u_char) ((s) >> 8); \
	*(cp)++ = (u_char) (s); \
	(len) += 2;				\
}
#define PUTLONG(l, cp, len) { \
	*(cp)++ = (u_char) ((l) >> 24); \
	*(cp)++ = (u_char) ((l) >> 16); \
	*(cp)++ = (u_char) ((l) >> 8); \
	*(cp)++ = (u_char) (l); \
	(len) += 4;				\
}
#define INCPTR(n, cp)	((cp) += (n))
#define DECPTR(n, cp)	((cp) -= (n))

	u_char *outp = bcast_pkt_buf;
	uint32_t pkt_len = 0;
	uint8_t dev_count = 0;
	uint8_t dev_status = 0;
	uint8_t  devnum = 0, intf_count = 0;
	uint16_t path_len, busid_len, devinfo_len;

#if CLIENT > 2
	uint16_t product_len, manufacturer_len, serial_len;	
#endif

	struct usbip_exported_device *edev;
	PUTSHORT(USBIP_TP_VERSION, outp, pkt_len);
	PUTSHORT(USBIP_TP_MAGIC, outp, pkt_len);
	/* how many devices are exported ? */	
	dlist_for_each_data(StubDriver->edev_list, edev, struct usbip_exported_device) 
	{		
		devnum += 1;	
	}
	PUTCHAR(Stub_online, outp, pkt_len);
	PUTCHAR(devnum, outp, pkt_len);

	/* add pattern of router product and wireless ssid to broadcasting packet */
	uint16_t len = strlen(pattern_str) + 3;
	PUTSHORT(len, outp, pkt_len);
	strcpy((char*)outp, pattern_str);
	pkt_len += (len - 2);
	INCPTR(len - 2, outp);
	len = strlen(ssid_str) + 3;
	PUTSHORT(len, outp, pkt_len);
	strcpy((char*)outp, ssid_str);
	pkt_len += (len - 2);
	INCPTR(len - 2, outp);
	
	dlist_for_each_data(StubDriver->edev_list, edev, struct usbip_exported_device) {
		#if 0
		dump_usb_device(&edev->udev);
		#endif
		path_len = strlen(edev->udev.path) + 1 + 2;
		busid_len = strlen(edev->udev.busid) + 1 + 2;
		devinfo_len = 4 + path_len + busid_len + 24 + 4 * (edev->udev.bNumInterfaces);
		
#if CLIENT > 2
		product_len = strlen(edev->udev.product) + 1 + 2;
		manufacturer_len = strlen(edev->udev.manufacturer) + 1 + 2;
		serial_len = strlen(edev->udev.serial) + 1 + 2;
		devinfo_len += (product_len + manufacturer_len + serial_len);
#endif
		
		dev_count++;
		PUTCHAR(dev_count, outp, pkt_len);	/* device id */

		dev_status = (uint8_t)edev->status;
		PUTCHAR(dev_status, outp, pkt_len);	/* the status of this device */
		
		PUTSHORT(devinfo_len, outp, pkt_len);
		
		PUTSHORT(path_len, outp, pkt_len);
		strcpy((char*)outp, edev->udev.path);
		pkt_len += (path_len - 2);
		INCPTR(path_len - 2, outp);
		PUTSHORT(busid_len, outp, pkt_len);
		strcpy((char*)outp, edev->udev.busid);
		pkt_len += (busid_len - 2);
		INCPTR(busid_len - 2, outp);

#if CLIENT > 2
		PUTSHORT(product_len, outp, pkt_len);
		strcpy((char*)outp, edev->udev.product);
		pkt_len += (product_len - 2);
		INCPTR(product_len - 2, outp);
		PUTSHORT(manufacturer_len, outp, pkt_len);
		strcpy((char*)outp, edev->udev.manufacturer);
		pkt_len += (manufacturer_len - 2);
		INCPTR(manufacturer_len - 2, outp);
		PUTSHORT(serial_len, outp, pkt_len);
		strcpy((char*)outp, edev->udev.serial);
		pkt_len += (serial_len - 2);
		INCPTR(serial_len - 2, outp);
#endif
		
		PUTLONG(edev->udev.busnum, outp, pkt_len);
		PUTLONG(edev->udev.devnum, outp, pkt_len);
		PUTLONG(edev->udev.speed, outp, pkt_len);
		
		PUTSHORT(edev->udev.idVendor, outp, pkt_len);
		PUTSHORT(edev->udev.idProduct, outp, pkt_len);
		PUTSHORT(edev->udev.bcdDevice, outp, pkt_len);
		
		PUTCHAR(edev->udev.bDeviceClass, outp, pkt_len);
		PUTCHAR(edev->udev.bDeviceSubClass, outp, pkt_len);
		PUTCHAR(edev->udev.bDeviceProtocol, outp, pkt_len);
		PUTCHAR(edev->udev.bConfigurationValue, outp, pkt_len);
		PUTCHAR(edev->udev.bNumConfigurations, outp, pkt_len);
		PUTCHAR(edev->udev.bNumInterfaces, outp, pkt_len);

		for(intf_count = 1; intf_count <= edev->udev.bNumInterfaces; intf_count++)
		{
			#if 0
			dump_usb_interface(&edev->uinf[intf_count-1]);
			#endif
			PUTCHAR(intf_count, outp, pkt_len);
			PUTCHAR(edev->uinf[intf_count-1].bInterfaceClass, outp, pkt_len);
			PUTCHAR(edev->uinf[intf_count-1].bInterfaceSubClass, outp, pkt_len);
			PUTCHAR(edev->uinf[intf_count-1].bInterfaceProtocol, outp, pkt_len);
		}
	}
	return pkt_len;
}

/***************************************************************************
 * FUNCTION		:		usbip_broadcast_devlist()
 * AUTHOR		:		TengFei
 * OCCASION		:		always running for broadcasting info of devlist at moment 
 * DESCRIPTION  :        
 *
 * INPUT			:       	sockfd
 * OUTPUT		:       	N/A
 * RETURN		:       	0 - SUCCESS, others - ERROR
 ***************************************************************************/
int usbip_broadcast_devlist(int sockfd)
{
	int pkt_len = usbip_fill_bcastpkt();

#if 0
	printf("The length of broadcasting pkt is: %d\n", pkt_len);
	for(int i=0; i <pkt_len; i++ )
	{
		if (i%16 == 0)
			printf("\n");
		printf("%02x\t", bcast_pkt_buf[i]);
	}
#endif
	int ret = usbip_send(sockfd, bcast_pkt_buf, pkt_len);
	/*printf("\n sent %d bytes.\n", ret);*/
	return ret;
}

/***************************************************************************
 * FUNCTION       :       broadcast_start()
 * AUTHOR          :        TengFei
 * OCCASION      :      aways running 
 * DESCRIPTION  :      broadcast the infomations of devices attached to USB slot of Router
 *
 * INPUT             :       N/A
 * OUTPUT          :       N/A
 * RETURN          :       N/A
 ***************************************************************************/
static void broadcast_start(int sockfd)
{
	int ret;

	Stub_online = 1;

	ret = usbip_stub_refresh_device_list(StubDriver);
	if(ret < 0) 
	{
		info("usbip_stub_refresh_device_list error %d", ret);
	}
	else if(ret == STUB_OFFLINE)
	{
		Stub_online = 0;
	}

	usbip_device_state();
	if (Stub_online)
	{
		usbip_refresh_waiting_list();
	}
	else
	{
		/*ps_info("%s: Stub_online = %d\n", __func__, Stub_online);*/
	}

	usbip_broadcast_devlist(sockfd);
	return ;
}


int recv_pdu(int sockfd)
{
	int ret;
	uint16_t code = OP_UNSPEC;


	ret = usbip_recv_op_common(sockfd, &code);
	if(ret < 0) {
		err("recv op_common, %d", ret);
		return ret;
	}


	ret = usbip_stub_refresh_device_list(StubDriver);
	if(ret < 0) 
		return -1;

	switch(code) {
		case OP_REQ_RESERVE:
			ret = recv_request_reserve(sockfd);		/* one client reserve device */
			break;

		case OP_REQ_IMPORT:
			ret = recv_request_import(sockfd);
			break;
			
		case OP_REQ_DEVLIST:
			ret = recv_request_devlist(sockfd);
			break;
		
		case OP_REQ_DEVINFO:
		case OP_REQ_CRYPKEY:

		default:
			err("unknown op_code, %d", code);
			ret = -1;
	}

	return ret;
}




static void log_addrinfo(struct addrinfo *ai)
{
	int ret;
	char hbuf[NI_MAXHOST];
	char sbuf[NI_MAXSERV];

	ret = getnameinfo(ai->ai_addr, ai->ai_addrlen, hbuf, sizeof(hbuf),
			sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
	if(ret)
		ps_err("getnameinfo, %s", gai_strerror(ret));

	dbg("listen at [%s]:%s", hbuf, sbuf);
}

static struct addrinfo *my_getaddrinfo(char *host, int ai_family)
{
	int ret;
	struct addrinfo hints, *ai_head;

	bzero(&hints, sizeof(hints));

	hints.ai_family   = ai_family;
	hints.ai_flags    = AI_PASSIVE;

	/* modified by tf, 110505, make ai_head contains two members, one for tcp, the other for udp. */
#if 1
	hints.ai_socktype = SOCK_STREAM;

	ret = getaddrinfo(host, USBIP_PORT_STRING, &hints, &ai_head);
#else
	ret = getaddrinfo(host, "usbip", &hints, &ai_head);	/* "usbip" defined in /etc/servers, tf. */
#endif
	if (ret){
		ps_err("%s: %s", USBIP_PORT_STRING, gai_strerror(ret) );
		return NULL;
	}

	return ai_head;
}

#define MAXSOCK 20
static int listen_all_addrinfo(struct addrinfo *ai_head, int lsock[], int *maxfd)
{
	struct addrinfo *ai;
	int n = 0;		/* number of sockets */
	*maxfd = -1;		/* maximum descriptor number */

	for(ai = ai_head; ai && n < MAXSOCK; ai = ai->ai_next){
		int ret;

		lsock[n] = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if(lsock[n] < 0)
			continue;

		usbip_set_reuseaddr(lsock[n]);
		usbip_set_nodelay(lsock[n]);
		
		if(lsock[n] >= FD_SETSIZE){
			close(lsock[n]);
			lsock[n] = -1;
			continue;
		}

		ret = bind(lsock[n], ai->ai_addr, ai->ai_addrlen);
		if(ret < 0){
			close(lsock[n]);
			lsock[n] = -1;
			continue;
		}

		
		ret = listen(lsock[n], SOMAXCONN);
		if(ret < 0){
			close(lsock[n]);
			lsock[n] = -1;
			continue;
		}

		log_addrinfo(ai);

		if(lsock[n] > *maxfd)
			*maxfd = lsock[n];

		n++;
	}

	if(n == 0){
		ps_err("no socket to listen to");
		return -1;
	}

	dbg("listen %d address%s", n, (n==1)?"":"es");

	return n;
}

#ifdef HAVE_LIBWRAP
static int tcpd_auth(int csock)
{
	int ret;
	struct request_info request;

	request_init(&request, RQ_DAEMON, "usbipd", RQ_FILE, csock, 0);

	fromhost(&request);

	ret = hosts_access(&request);
	if(!ret)
		return -1;

	return 0;	
}
#endif

static int my_accept(int lsock)
{
	int csock;
	struct sockaddr_storage ss;
	socklen_t len = sizeof(ss);
	char  port[NI_MAXSERV];
	int ret;

	bzero(&ss, sizeof(ss));

	csock = accept(lsock, (struct sockaddr *) &ss, &len);
	if(csock < 0) {
		ps_err("accept");
		return -1;
	}

	ret = getnameinfo((struct sockaddr *)&ss, len,
			host, sizeof(host), port, sizeof(port),
			NI_NUMERICHOST | NI_NUMERICSERV);
	if(ret)
		ps_err("getnameinfo, %s", gai_strerror(ret));

#ifdef HAVE_LIBWRAP
	ret = tcpd_auth(csock);
	if(ret < 0) {
		info("deny access from %s", host);
		close(csock);
		return -1;
	}
#endif

	info("connected from %s:%s", host, port);

	return csock;
}

static int loop_standalone = 1;
static void signal_handler(int i)
{
	dbg("signal catched %d", i);
	loop_standalone = 0;
}

static void daemon_init(void)
{
	pid_t pid;

	usbip_use_stderr = 0;
	usbip_use_syslog = 1;

	pid = fork();
	if(pid < 0) {
		err("fork");
		exit(EXIT_FAILURE);
	} else if(pid > 0)
		/* bye parent */
		exit(EXIT_SUCCESS);

	setsid();

	signal(SIGHUP, SIG_IGN);

	pid = fork();
	if(pid < 0) {
		err("fork");
		exit(EXIT_FAILURE);
	} else if(pid > 0)
		/* bye parent */
		exit(EXIT_SUCCESS);

	chdir("/");
	umask(0);

	for(int i=0; i < 3; i++)
		close(i);

}

static void set_signal(void)
{
	struct sigaction act;

	bzero(&act, sizeof(act));
	act.sa_handler = signal_handler;
	sigemptyset(&act.sa_mask);
	sigaction(SIGTERM, &act, NULL);
	sigaction(SIGINT, &act, NULL);
}

static void do_standalone_mode(void)
{
	int ret;
	int lsock[MAXSOCK];
	struct addrinfo *ai_head;
	int n;
	int maxfd;

	if(!usbip_use_debug)
		daemon_init();

	set_signal();

	/* At the begining of usbipd, we echo idle to notice webserver that usbipd is up. tf, 111201 */
	system("echo idle > "USBIP_STATE_PATH);
	
#if 1
	ai_head = my_getaddrinfo(NULL, PF_UNSPEC);
#else
	ai_head = my_getaddrinfo(NULL, AF_INET);
#endif
	if(!ai_head)
		goto err_driver_open;

	n = listen_all_addrinfo(ai_head, lsock, &maxfd);
	if(n <= 0) {
		ps_err("no socket to listen to");
		goto err_driver_open;
	}

	StubDriver = usbip_stub_driver_open();
	if(!StubDriver) 
		goto err_driver_open;
	
	int sockfd;
	const int on = 1;
	struct sockaddr_in servaddr;
	struct ifreq ifr;
	
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(USBIP_PORT);
	
	/* for broadcast */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		ps_err("create broadcast socket failure!");
		goto err;
	}
	
	memset(&ifr, 0, sizeof(ifr));	
	strncpy(ifr.ifr_name, LAN_INTF_NAME, IFNAMSIZ);
	ret = ioctl(sockfd, SIOCGIFBRDADDR, &ifr);
	if (ret < 0)
	{
		ps_info("ioctl to get ifbrdaddr failed, err %d\n", errno);
	}
	
	struct sockaddr_in *pSin = (struct sockaddr_in*)&(ifr.ifr_addr);
	memcpy(&servaddr.sin_addr, &pSin->sin_addr, sizeof(servaddr.sin_addr));
	close(sockfd);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		ps_err("create broadcast socket failure!");
		goto err;
	}
#if 1	/* for broadcast */
	setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
#endif

	ret = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	if (ret < 0)
	{
		ps_err("connect ifbrdaddr err");
		close(sockfd);
		goto err;
	}
	#if 0	/* using select func instead of thread */
	/* create a thread to process udp broadcast. 110507, added by tf */
	pthread_t p_broadcast;  
	pthread_attr_t p_broadcast_attr;
	ret = pthread_attr_init(&p_broadcast_attr);
	if(ret) 
	      	err("Thread attribute creation failed");
	 ret = pthread_attr_setdetachstate(&p_broadcast_attr, PTHREAD_CREATE_DETACHED);
	 if(ret) 
	    	 err("Set detached attribute failed");

	 ret = pthread_create(&p_broadcast, &p_broadcast_attr, &broadcast_thread, NULL);
	 if(ret) 
	        err("Create agent main thread failed.\n");
	#endif
	/* remove it by tf, 110505, it's not necessary to parse usb.ids file. */
	#if 0
 	ret = names_init(USBIDS_FILE);
 	if(ret) 
 		err("open usb.ids");
	#endif

	info("start");

	/* setup fd_set */
	fd_set rfd0;
	FD_ZERO(&rfd0);
	struct timeval timeout;	/* using select instead of thread */

	int	seconds_mark = time((time_t *)NULL);
	int	seconds_current = time((time_t *)NULL);
		
	for(int i = 0; i < n; i++)
		FD_SET(lsock[i], &rfd0);

	while(loop_standalone)
	{
		fd_set rfd = rfd0;
		timeout.tv_sec = bcast_period / 1000;
		timeout.tv_usec = (bcast_period % 1000) * 1000;
		ret = select(maxfd + 1, &rfd, NULL, NULL, &timeout);
		if(ret < 0)
		{
			if(!(errno == EINTR))
				ps_err("select");
			continue;
		}

		if( ret == 0 )	/* timeout */
		{
			broadcast_start(sockfd);
			seconds_mark =  time((time_t *)NULL);
			continue;
		}
			
		for(int i = 0; i < n; i++)
		{
			if(FD_ISSET(lsock[i], &rfd))
			{
				int csock;
				csock = my_accept(lsock[i]);
				if(csock < 0)
				{
					continue;
				}
				ret = recv_pdu(csock);
				if(ret < 0)
					ps_err("process recieved pdu");
				close(csock);
			}
		}

		seconds_current = time((time_t *)NULL);
		if (seconds_current - seconds_mark >= 2)
		{
			broadcast_start(sockfd);
			seconds_mark = seconds_current;
		}
	}

err:
	usbip_stub_driver_close(StubDriver);
err_driver_open:
	ps_info("ps shutdown");
	freeaddrinfo(ai_head);
	/* At the end of usbipd, we unlink USBIP_STATE_PATH  to notice webserver that usbipd is down. tf, 111201 */
	unlink(USBIP_STATE_PATH);
#if 0
	names_deinit();
#endif
	return;
}

static inline char char_to_digit(char ch)
{
	if (ch >= '0' && ch <= '9')
		return (ch - '0');
	if (ch >= 'a' && ch <= 'f')
		return (ch + 10 - 'a');
	if (ch >= 'A' && ch <= 'F')
		return (ch + 10 - 'A');
	return 0;
}

static void hex_to_ascll(const char * hex_str)
{
	int i, hexstr_len = strlen(hex_str);
	char hex_val = 0;
	for (i = 0; i * 2 < hexstr_len; i++)
	{
		hex_val = (char_to_digit(hex_str[2*i]) << 4) + char_to_digit(hex_str[2*i+1]);
		ssid_str[i] = hex_val;
	}
	ssid_str[i] = 0;
}

const char help_message[] = "\
Usage: usbipd [options]				\n\
	-D, --debug				\n\
		Print debugging information.	\n\
						\n\
	-v, --version				\n\
		Show version.			\n\
						\n\
	-h, --help 				\n\
		Print this help.		\n";

static void show_help(void)
{
	printf("%s", help_message);
}

static const struct option longopts[] = {
	{"version",	no_argument,	NULL, 'v'},
	{"help",	no_argument,	NULL, 'h'},
	{"debug",	no_argument,	NULL, 'D'},
	/* added by tf, 110531, for broadcasting pattern and ssid */
	{"pattern",required_argument,	NULL, 'p'},
	{"ssid",	required_argument,	NULL, 's'},
	{NULL,		0,		NULL,  0}
};

int main(int argc, char *argv[])
{
	enum {
		cmd_standalone_mode = 1,
		cmd_help,
		cmd_version
	} cmd = cmd_standalone_mode;

	usbip_use_stderr = 1;
	usbip_use_syslog = 0;

	while(1)
	{
		int c;
		int index = 0;

		c = getopt_long(argc, argv, "vhDp:s:", longopts, &index);

		if(c == -1)
			break;

		switch(c)
		{
			case 'p':
				strcpy(pattern_str, optarg);	/* added by tf, 110531, for pattern broadcasting */
				break;
			case 's':
				hex_to_ascll(optarg);	/* added by tf, 110531, for ssid broadcasting */
				break;
			case 'v':
				if(cmd)
					cmd = cmd_help;
				else
					cmd = cmd_version;
				break;
			case 'h':
				cmd = cmd_help;
			case 'D':
				usbip_use_debug = 1;
				break;
			case '?':
				break;
			default:
				err("getopt");
		}
	}

	switch(cmd) 
	{
		case cmd_standalone_mode:
			do_standalone_mode();
			break;
		case cmd_version:
			printf("%s\n", version);
			break;
		case cmd_help:
			show_help();
			break;
		default:
			info("unknown cmd");
			show_help();
	}

	return 0;
}

