
#if WIRELESS_EXT <= 11
#ifndef SIOCDEVPRIVATE
#define SIOCDEVPRIVATE			0x8BE0
#endif
#define SIOCIWFIRSTPRIV			SIOCDEVPRIVATE
#endif


#define RT_PRIV_IOCTL			(SIOCIWFIRSTPRIV + 0x01)
#define RTPRIV_IOCTL_SET		(SIOCIWFIRSTPRIV + 0x02)
#define OID_GET_SET_TOGGLE		0x8000

#define OID_802_DOT1X_IDLE_TIMEOUT			0x0545
#define RT_OID_802_DOT1X_IDLE_TIMEOUT		(OID_GET_SET_TOGGLE | OID_802_DOT1X_IDLE_TIMEOUT)

#define	OID_802_11_SSID						0x0509
#define	OID_802_11_BSSID					0x050A
#define	OID_GEN_MEDIA_CONNECT_STATUS		0x060B

typedef struct _wfa_driver_mtk_config {
	int ioctl_sock;
#ifdef TGAC_DAEMON	
	int fd;
#endif
	char ifname[32];
	unsigned char dev_addr[6];
	unsigned char addr[18];
	unsigned char ssid[32];
	unsigned char conn_stat;
} wfa_driver_mtk_config, *pwfa_driver_mtk_config;

void wfa_driver_mtk_sock_int();
void wfa_driver_mtk_sock_exit();
int wfa_driver_mtk_set_oid(char *ifname, int oid, char *data, int len);
int wfa_driver_mtk_get_oid(char *intf, int oid, unsigned char *data, int len);
int wfa_driver_mtk_set_cmd(char *pIntfName, char *pCmdStr);
int wfa_driver_mtk_check_ssid(char *pIntfName, char *pSsid);
void wfa_driver_mtk_interface_name_send(void);

