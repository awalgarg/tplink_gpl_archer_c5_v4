ifneq ($(INCLUDE_CUSTOMIZED_SP), "")
export CSP=$(subst ",,$(INCLUDE_CUSTOMIZED_SP))
DFLAGS += -DINCLUDE_$(CSP)
DFLAGS += -DINCLUDE_$(MODEL)_$(SPEC)_$(CSP)
endif


ifeq ($(INCLUDE_CPU_MT7620A), y) 
DFLAGS += -DINCLUDE_CPU_MT7620A
endif 

ifeq ($(INCLUDE_CPU_MT7628), y) 
DFLAGS += -DINCLUDE_CPU_MT7628
endif 

ifeq ($(INCLUDE_CPU_MT7621), y) 
DFLAGS += -DINCLUDE_CPU_MT7621
endif 

ifeq ($(INCLUDE_CPU_BIG_ENDIAN), y) 
DFLAGS += -DINCLUDE_CPU_BIG_ENDIAN
CPU_ENDIAN_TYPE = 0
endif 

ifeq ($(INCLUDE_CPU_LITTLE_ENDIAN), y) 
DFLAGS += -DINCLUDE_CPU_LITTLE_ENDIAN
CPU_ENDIAN_TYPE = 1
endif 

ifeq ($(INCLUDE_NAND_FLASH), y)
DFLAGS += -DINCLUDE_NAND_FLASH
endif

ifeq ($(INCLUDE_NOR_FLASH), y)
DFLAGS += -DINCLUDE_NOR_FLASH
endif

ifeq ($(INCLUDE_UBOOT_ENV), y)
DFLAGS += -DINCLUDE_UBOOT_ENV
endif

ifeq ($(INCLUDE_FACTORY_UBOOT), y)
DFLAGS += -DINCLUDE_FACTORY_UBOOT
endif

ifeq ($(INCLUDE_DUAL_IMAGE_SUPPORT), y)
DFLAGS += -DINCLUDE_DUAL_IMAGE_SUPPORT
endif

ifeq ($(strip $(INCLUDE_MODEL_GW)), y)
DFLAGS += -DINCLUDE_MODEL_GW
export INCLUDE_MODEL_GW=y
endif

ifeq ($(strip $(INCLUDE_MODEL_MODEM)), y)
DFLAGS += -DINCLUDE_MODEL_MODEM
export INCLUDE_MODEL_MODEM=y
endif

ifeq ($(strip $(INCLUDE_LANGUAGE_CN)), y)
DFLAGS += -DINCLUDE_LANGUAGE_CN
export INCLUDE_LANGUAGE_CN=y
endif

ifeq ($(strip $(INCLUDE_LANGUAGE_EN)), y)
DFLAGS += -DINCLUDE_LANGUAGE_EN
export INCLUDE_LANGUAGE_EN=y
endif

ifeq ($(strip $(INCLUDE_SOCKET_LOCK)), y)
DFLAGS += -DINCLUDE_SOCKET_LOCK
export INCLUDE_SOCKET_LOCK=y
endif

ifeq ($(INCLUDE_SERBIA), y) 
DFLAGS += -DINCLUDE_SERBIA
endif 

ifeq ($(INCLUDE_WATCHDOG), y) 
DFLAGS += -DINCLUDE_WATCHDOG
endif 

ifeq ($(INCLUDE_PPA_ACCELERATE), y)
DFLAGS += -DINCLUDE_PPA_ACCELERATE
endif

ifeq ($(INCLUDE_MTD_TYPE1), y)
DFLAGS += -DINCLUDE_MTD_TYPE1
MTD_PART_TYPE = 1
endif

ifeq ($(INCLUDE_MTD_TYPE2), y)
DFLAGS += -DINCLUDE_MTD_TYPE2
MTD_PART_TYPE = 2
endif

ifeq ($(INCLUDE_MTD_TYPE3), y)
DFLAGS += -DINCLUDE_MTD_TYPE3
MTD_PART_TYPE = 3
endif

ifeq ($(INCLUDE_MTD_TYPE4), y)
DFLAGS += -DINCLUDE_MTD_TYPE4
MTD_PART_TYPE = 4
endif

ifneq ($(strip $(BOOT_MAX_SIZE)),)
DFLAGS += -DMTD_BOOT_SIZE=$(subst ",,$(BOOT_MAX_SIZE))
endif

ifneq ($(strip $(FLASH_SIZE)),)
DFLAGS += -DFLASH_SIZE=$(subst ",,$(FLASH_SIZE))
endif

ifeq ($(INCLUDE_REDUCE_FLASH_SIZE), y)
DFLAGS += -DINCLUDE_REDUCE_FLASH_SIZE
endif

ifeq ($(INCLUDE_DROP_CACHE), y)
DFLAGS += -DINCLUDE_DROP_CACHE
endif

ifeq ($(INCLUDE_KERNEL_NETLINK), y)
DFLAGS += -DINCLUDE_KERNEL_NETLINK
endif

ifeq ($(INCLUDE_GPIO_CONTROL), y)
DFLAGS += -DINCLUDE_GPIO_CONTROL
endif

ifeq ($(INCLUDE_CLI_FULL_FEATURE), y)
DFLAGS += -DINCLUDE_CLI_FULL_FEATURE
endif

ifeq ($(INCLUDE_USER_ACCOUNT), y) 
DFLAGS += -DINCLUDE_USER_ACCOUNT
endif

ifeq ($(INCLUDE_NEWUI), y) 
DFLAGS += -DINCLUDE_NEWUI
endif

ifeq ($(INCLUDE_CHGINIT_PWD), y) 
DFLAGS += -DINCLUDE_CHGINIT_PWD
WEBFLAGS += INCLUDE_CHGINIT_PWD=1
else
WEBFLAGS += INCLUDE_CHGINIT_PWD=0
endif

ifeq ($(INCLUDE_CHGPWD_LOGIN), y) 
WEBFLAGS += INCLUDE_CHGPWD_LOGIN=1
else
WEBFLAGS += INCLUDE_CHGPWD_LOGIN=0
endif

ifeq ($(INCLUDE_BANDSTEERING), y)
WEBFLAGS += INCLUDE_BANDSTEERING
DFLAGS += -DINCLUDE_BANDSTEERING
else
WEBFLAGS += INCLUDE_BANDSTEERING=0
endif

ifeq ($(INCLUDE_TXBF), y)
WEBFLAGS += INCLUDE_TXBF=1
DFLAGS += -DINCLUDE_TXBF
else
WEBFLAGS += INCLUDE_TXBF=0
endif

ifeq ($(INCLUDE_MU_MIMO), y)
WEBFLAGS += INCLUDE_MU_MIMO=1
DFLAGS += -DINCLUDE_MU_MIMO
else
WEBFLAGS += INCLUDE_MU_MIMO=0
endif

ifeq ($(INCLUDE_AIRTIME_FAIRNESS), y)
WEBFLAGS += INCLUDE_AIRTIME_FAIRNESS=1
DFLAGS += -DINCLUDE_AIRTIME_FAIRNESS
else
WEBFLAGS += INCLUDE_AIRTIME_FAIRNESS=0
endif


ifeq ($(INCLUDE_AUTH_PASSWORD), y) 
DFLAGS += -DINCLUDE_AUTH_PASSWORD
endif

ifeq ($(INCLUDE_NEWAUTH_CLI), y) 
DFLAGS += -DINCLUDE_NEWAUTH_CLI
endif

ifeq ($(INCLUDE_TPLOGIN_REDIRECT), y)
DFLAGS += -DINCLUDE_TPLOGIN_REDIRECT
endif

ifeq ($(INCLUDE_CO_FW), y)
DFLAGS += -DINCLUDE_CO_FW
endif

ifeq ($(INCLUDE_OPTION66), y)
DFLAGS += -DINCLUDE_OPTION66
ifeq ($(INCLUDE_OPTION66_NEED_TAG), y)
DFLAGS += -DINCLUDE_OPTION66_NEED_TAG
endif
endif

ifeq ($(INCLUDE_DEVICE_CONFIG), y)
DFLAGS += -DINCLUDE_DEVICE_CONFIG
endif

ifeq ($(INCLUDE_ISP_NTC), y)
DFLAGS += -DINCLUDE_ISP_NTC
endif

ifeq ($(INCLUDE_MEXICO_SPEC), y)
DFLAGS += -DINCLUDE_MEXICO_SPEC
endif

ifeq ($(INCLUDE_MALAYSIA_SPEC), y)
DFLAGS += -DINCLUDE_MALAYSIA_SPEC
endif

ifeq ($(INCLUDE_RUSSIA_SPEC), y)
DFLAGS += -DINCLUDE_RUSSIA_SPEC
endif

ifeq ($(INCLUDE_TW_SPEC), y)
DFLAGS += -DINCLUDE_TW_SPEC
endif

ifeq ($(INCLUDE_KOREA_SPEC), y)
DFLAGS += -DINCLUDE_KOREA_SPEC
endif

ifeq ($(INCLUDE_CANADA_SPEC), y)
DFLAGS += -DINCLUDE_CANADA_SPEC
endif

ifeq ($(INCLUDE_BR_SPEC), y)
DFLAGS += -DINCLUDE_BR_SPEC
endif

ifeq ($(strip $(INCLUDE_BR_HUGHES_SPEC)), y)
DFLAGS += -DINCLUDE_BR_HUGHES_SPEC
endif

ifeq ($(INCLUDE_ES_SPEC), y)
DFLAGS += -DINCLUDE_ES_SPEC
endif

ifeq ($(INCLUDE_GERMANY_SPEC), y)
DFLAGS += -DINCLUDE_GERMANY_SPEC
endif

ifeq ($(INCLUDE_VIETNAM_FPT), y)
DFLAGS += -DINCLUDE_VIETNAM_FPT
endif

ifeq ($(INCLUDE_US_SPEC), y)
DFLAGS += -DINCLUDE_US_SPEC
endif

ifeq ($(DEFAULT_NO_SPEC), y)
DFLAGS += -DDEFAULT_NO_SPEC
endif

ifeq ($(INCLUDE_JAPAN_SPEC),y)
DFLAGS +=-DINCLUDE_JAPAN_SPEC
endif

ifeq ($(INCLUDE_PL_SPEC), y)
DFLAGS +=-DINCLUDE_PL_SPEC
endif

ifeq ($(strip $(INCLUDE_TELNET_CONTROL)), y)
DFLAGS += -DINCLUDE_TELNET_CONTROL
endif

ifeq ($(INCLUDE_ATED_IWPRIV), y)
DFLAGS += -DINCLUDE_ATED_IWPRIV
endif

ifeq ($(INCLUDE_VN_SPEC),y)
DFLAGS +=-DINCLUDE_VN_SPEC
DFLAGS +=-DINCLUDE_AUTOREBOOT
endif

ifeq ($(INCLUDE_ROMANIA_SPEC),y)
DFLAGS +=-DINCLUDE_ROMANIA_SPEC
DFLAGS +=-DINCLUDE_CPUMEM_INFO
endif

ifeq ($(INCLUDE_FCC_BORDER_CHANNEL_AVOID),y)
DFLAGS +=-DINCLUDE_FCC_BORDER_CHANNEL_AVOID
endif
#################### ETH Type ######################
ifeq ($(INCLUDE_ETH_LTQ), y) 
DFLAGS += -DINCLUDE_ETH_LTQ
endif 

ifeq ($(INCLUDE_ETH_RA), y) 
DFLAGS += -DINCLUDE_ETH_RA
endif 

ifeq ($(INCLUDE_ETH_ATH), y) 
DFLAGS += -DINCLUDE_ETH_ATH
endif 

ifeq ($(INCLUDE_GMAC1_ONLY), y)
DFLAGS += -DINCLUDE_GMAC1_ONLY
endif

###################### WIFI  ########################
ifeq ($(INCLUDE_LAN_WLAN), y) 
DFLAGS += -DINCLUDE_LAN_WLAN 
endif 

ifeq ($(INCLUDE_WLAN_RA), y) 
DFLAGS += -DINCLUDE_WLAN_RA
endif 

ifeq ($(INCLUDE_WLAN_RTL), y) 
DFLAGS += -DINCLUDE_WLAN_RTL
endif 

ifeq ($(INCLUDE_WLAN_ATH), y) 
DFLAGS += -DINCLUDE_WLAN_ATH
endif 

#added by wangbing 2013-10-22
ifeq ($(INCLUDE_WLAN_MTK_AC),y)
DFLAGS += -DINCLUDE_WLAN_MTK_AC
endif 

ifeq ($(ATH_A_BAND_MIMO_NUM_3_3), y)
DFLAGS += -DWLAN_ATH_A_BAND_MIMO_NUM=3
endif

ifeq ($(ATH_G_BAND_MIMO_NUM_2_2), y)
DFLAGS += -DWLAN_ATH_G_BAND_MIMO_NUM=2
endif

ifeq ($(INCLUDE_WLAN_BCM), y) 
DFLAGS += -DINCLUDE_WLAN_BCM
endif 

ifeq ($(BCM_A_BAND_MIMO_NUM_1_1), y)
DFLAGS += -DWLAN_BCM_A_BAND_MIMO_NUM=1
endif

ifeq ($(BCM_A_BAND_MIMO_NUM_2_2), y)
DFLAGS += -DWLAN_BCM_A_BAND_MIMO_NUM=2
endif

ifeq ($(BCM_A_BAND_MIMO_NUM_3_3), y)
DFLAGS += -DWLAN_BCM_A_BAND_MIMO_NUM=3
endif

ifeq ($(BCM_B_BAND_MIMO_NUM_1_1), y)
DFLAGS += -DWLAN_BCM_B_BAND_MIMO_NUM=1
endif

ifeq ($(BCM_B_BAND_MIMO_NUM_2_2), y)
DFLAGS += -DWLAN_BCM_B_BAND_MIMO_NUM=2
endif

ifeq ($(BCM_B_BAND_MIMO_NUM_3_3), y)
DFLAGS += -DWLAN_BCM_B_BAND_MIMO_NUM=3
endif
ifeq ($(INCLUDE_LAN_WLAN_MSSID), y) 
DFLAGS += -DINCLUDE_LAN_WLAN_MSSID 
endif 

ifdef WLAN_VAP_NUM
DFLAGS += -DWLAN_VAP_NUM=$(WLAN_VAP_NUM)
endif

ifeq ($(INCLUDE_LAN_WLAN_GUESTNETWORK), y) 
DFLAGS += -DINCLUDE_LAN_WLAN_GUESTNETWORK 
endif 

ifeq ($(INCLUDE_HIDE_GUESTNETWORK), y) 
DFLAGS += -DINCLUDE_HIDE_GUESTNETWORK 
endif 

ifeq ($(INCLUDE_5G_DFS), y) 
DFLAGS += -DINCLUDE_5G_DFS
endif

ifeq ($(INCLUDE_LAN_WLAN_WDS), y) 
DFLAGS += -DINCLUDE_LAN_WLAN_WDS
endif 

ifeq ($(INCLUDE_LAN_WLAN_WDS_DETECT), y)
DFLAGS += -DINCLUDE_LAN_WLAN_WDS_DETECT
endif

ifeq ($(INCLUDE_LAN_WLAN_8021X), y) 
DFLAGS += -DINCLUDE_LAN_WLAN_802_1X
endif 

ifeq ($(INCLUDE_LAN_WLAN_DUALBAND),y)
DFLAGS+= -DINCLUDE_LAN_WLAN_DUALBAND
endif

ifeq ($(INCLUDE_LAN_WLAN_AC),y)
DFLAGS+= -DINCLUDE_LAN_WLAN_AC
endif

ifeq ($(INCLUDE_LAN_WLAN_SCHEDULE), y)
DFLAGS += -DINCLUDE_LAN_WLAN_SCHEDULE
endif

ifeq ($(INCLUDE_LAN_WLAN_DUALBAND_DIF_MAC),y)
DFLAGS+= -DINCLUDE_LAN_WLAN_DUALBAND_DIF_MAC
endif

ifeq ($(INCLUDE_WIFI_5G), y)
DFLAGS += -DINCLUDE_WIFI_5G
endif

# Added by KangYi 2017-09-26
ifeq ($(INCLUDE_CE_REGION), y)
DFLAGS += -DINCLUDE_CE_REGION
endif

ifeq ($(INCLUDE_FCC_REGION), y)
DFLAGS += -DINCLUDE_FCC_REGION
endif

ifeq ($(INCLUDE_OTHER_REGION), y)
DFLAGS += -DINCLUDE_OTHER_REGION
endif

ifeq ($(INCLUDE_LAN_WLAN_SHOWSTARATE), y)
DFLAGS += -DINCLUDE_LAN_WLAN_SHOWSTARATE
endif

# Added end

# added by yanglianxiang 2014-7-23
ifeq ($(INCLUDE_WIFI_5G_CHIP_MT7610E), y)
DFLAGS += -DINCLUDE_WIFI_5G_CHIP_MT7610E
else ifeq ($(INCLUDE_WIFI_5G_CHIP_MT7612E), y)
DFLAGS += -DINCLUDE_WIFI_5G_CHIP_MT7612E
# for ated_tp
export WT_FLAGS += -DINCLUDE_WIFI_5G_CHIP_MT7612E
else ifeq ($(INCLUDE_WIFI_5G_CHIP_MT7615N), y)
DFLAGS += -DINCLUDE_WIFI_5G_CHIP_MT7615N
endif 

#################### Function ######################

ifeq ($(INCLUDE_BRIDGING), y)
DFLAGS += -DINCLUDE_BRIDGING
endif

ifeq ($(INCLUDE_ADSLWAN), y)
DFLAGS += -DINCLUDE_ADSLWAN
endif

ifeq ($(INCLUDE_VDSLWAN), y)
DFLAGS += -DINCLUDE_VDSLWAN
endif

ifeq ($(INCLUDE_ADSL_RA), y)
DFLAGS += -DINCLUDE_ADSL_RA
endif

ifeq ($(INCLUDE_ADSL_RTL), y)
DFLAGS += -DINCLUDE_ADSL_RTL
endif

ifeq ($(INCLUDE_ADSL_LTQ), y)
DFLAGS += -DINCLUDE_ADSL_LTQ
endif

ifeq ($(INCLUDE_ETHERNET_WAN), y)
DFLAGS += -DINCLUDE_ETHERNET_WAN
endif

ifeq ($(INCLUDE_IGMP), y)
DFLAGS += -DINCLUDE_IGMP
endif

#add by wanghao
ifeq ($(INCLUDE_IGMP_FORCEVERSION), y)
DFLAGS += -DINCLUDE_IGMP_FORCEVERSION
endif

ifeq ($(INCLUDE_IGMP_SRC_CARE), y)
DFLAGS += -DINCLUDE_IGMP_SRC_CARE
endif
#add end

ifeq ($(INCLUDE_IGMP_ETHPROXY), y)
DFLAGS += -DINCLUDE_IGMP_ETHPROXY
endif

ifeq ($(INCLUDE_IGMP_CONFLICT_DETECT), y)
DFLAGS += -DINCLUDE_IGMP_CONFLICT_DETECT
endif


ifeq ($(INCLUDE_IPTV), y)
DFLAGS += -DINCLUDE_IPTV
endif

ifeq ($(INCLUDE_EWAN_IPTV), y)
DFLAGS += -DINCLUDE_EWAN_IPTV
WEBFLAGS += INCLUDE_EWAN_IPTV=1
else
WEBFLAGS += INCLUDE_EWAN_IPTV=0
endif

ifeq ($(INCLUDE_AUTO_PVC), y)
DFLAGS += -DINCLUDE_AUTO_PVC
endif

ifeq ($(INCLUDE_RIP), y)
DFLAGS += -DINCLUDE_RIP
endif

ifeq ($(INCLUDE_RIPNG), y)
DFLAGS += -DINCLUDE_RIPNG
endif

ifeq ($(INCLUDE_SNMP), y)
DFLAGS += -DINCLUDE_SNMP
endif

ifeq ($(INCLUDE_SSL), y)
DFLAGS += -DINCLUDE_SSL
endif

ifeq ($(INCLUDE_CWMP), y)
DFLAGS += -DINCLUDE_CWMP
endif

ifeq ($(INCLUDE_TR069_ACSURL_FROM_DHCP), y)
DFLAGS += -DINCLUDE_TR069_ACSURL_FROM_DHCP
endif

ifeq ($(INCLUDE_TR111_PART1), y)
DFLAGS += -DINCLUDE_TR111_PART1
endif

ifeq ($(INCLUDE_TR111_PART2), y)
DFLAGS += -DINCLUDE_TR111_PART2
endif

ifeq ($(INCLUDE_CWMP_SSL), y)
DFLAGS += -DINCLUDE_CWMP_SSL
export INCLUDE_CWMP_SSL
ifeq ($(INCLUDE_CWMP_SSL_VERIFY_PEER), y)
DFLAGS += -DINCLUDE_CWMP_SSL_VERIFY_PEER
export INCLUDE_CWMP_SSL_VERIFY_PEER
endif
endif

ifeq ($(INCLUDE_CWMP_USE_FLIE), y)
DFLAGS += -DINCLUDE_CWMP_USE_FLIE
export INCLUDE_CWMP_USE_FLIE
ifeq ($(INCLUDE_CWMP_302), y)
DFLAGS += -DINCLUDE_CWMP_302
export INCLUDE_CWMP_302
endif
endif

ifeq ($(INCLUDE_HTTP_SSL), y)
DFLAGS += -DINCLUDE_HTTP_SSL
endif

ifeq ($(INCLUDE_WRITE_SN), y)
DFLAGS += -DINCLUDE_WRITE_SN
endif

ifeq ($(INCLUDE_TR143), y)
DFLAGS += -DINCLUDE_TR143
endif

ifeq ($(INCLUDE_OPTION60_FOR_NTC), y)
DFLAGS += -DINCLUDE_OPTION60_FOR_NTC
endif

ifeq ($(INCLUDE_IPPING_DIAG), y)
DFLAGS += -DINCLUDE_IPPING_DIAG
endif

ifeq ($(INCLUDE_TRACEROUTE_DIAG), y)
DFLAGS += -DINCLUDE_TRACEROUTE_DIAG
endif

ifeq ($(INCLUDE_DOWNLOAD_DIAG), y)
DFLAGS += -DINCLUDE_DOWNLOAD_DIAG
endif

ifeq ($(INCLUDE_UPLOAD_DIAG), y)
DFLAGS += -DINCLUDE_UPLOAD_DIAG
endif

ifeq ($(INCLUDE_FLASH_SPI), y)
DFLAGS += -DINCLUDE_FLASH_SPI
endif

ifeq ($(INCLUDE_FLASH_NAND), y)
DFLAGS += -DINCLUDE_FLASH_NAND
endif

ifeq ($(INCLUDE_FS_UBI), y)
DFLAGS += -DINCLUDE_FS_UBI
endif

ifeq ($(INCLUDE_DUAL_IMAGE), y)
DFLAGS += -DINCLUDE_DUAL_IMAGE
endif

ifeq ($(INCLUDE_LZMA), y)
export INCLUDE_LZMA=y
endif

ifeq ($(INCLUDE_REDUCED_FS), y)
export INCLUDE_REDUCED_FS=y
endif

ifeq ($(INCLUDE_CONFIG_FORMAT_COMPRESSED), y)
DFLAGS += -DINCLUDE_CONFIG_FORMAT_COMPRESSED
endif

ifeq ($(INCLUDE_CONFIG_FORMAT_XML), y)
DFLAGS += -DINCLUDE_CONFIG_FORMAT_XML
endif

ifeq ($(INCLUDE_CONFIG_FORMAT_BASE64), y)
DFLAGS += -DINCLUDE_CONFIG_FORMAT_BASE64
endif

ifeq ($(INCLUDE_DIGITAL_SIGNATURE), y)
DFLAGS += -DINCLUDE_DIGITAL_SIGNATURE
endif

ifeq ($(INCLUDE_DDNS), y)
DFLAGS += -DINCLUDE_DDNS
endif

ifeq ($(INCLUDE_DDNS_PH), y)
DFLAGS += -DINCLUDE_DDNS_PH
endif

ifeq ($(INCLUDE_DYNDNS), y)
DFLAGS += -DINCLUDE_DYNDNS
endif

ifeq ($(INCLUDE_NOIPDNS), y)
DFLAGS += -DINCLUDE_NOIPDNS
endif

ifeq ($(INCLUDE_DIAGTOOL), y)
DFLAGS += -DINCLUDE_DIAGTOOL
endif

ifeq ($(INCLUDE_ALG), y)
DFLAGS += -DINCLUDE_ALG
endif

ifeq ($(INCLUDE_ALG_PPTP), y)
DFLAGS += -DINCLUDE_ALG_PPTP
endif

ifeq ($(INCLUDE_ALG_L2TP), y)
DFLAGS += -DINCLUDE_ALG_L2TP
endif

ifeq ($(INCLUDE_ALG_H323), y)
DFLAGS += -DINCLUDE_ALG_H323
endif

ifeq ($(INCLUDE_ALG_SIP), y)
DFLAGS += -DINCLUDE_ALG_SIP
endif

ifeq ($(INCLUDE_WAN_DETECT), y)
DFLAGS += -DINCLUDE_WAN_DETECT
endif

ifeq ($(INCLUDE_HOSTNAME_EDITABLE), y)
DFLAGS += -DINCLUDE_HOSTNAME_EDITABLE
endif

ifeq ($(INCLUDE_REBOOT_SCHEDULE), y)
DFLAGS += -DINCLUDE_REBOOT_SCHEDULE
endif

ifeq ($(INCLUDE_YANDEX_DNS), y)
DFLAGS += -DINCLUDE_YANDEX_DNS
endif

ifeq ($(INCLUDE_DHCPC_REQUEST_PREV_IP), y)
DFLAGS += -DINCLUDE_DHCPC_REQUEST_PREV_IP
endif

ifeq ($(INCLUDE_DHCPC_FILTER_MULTICAST_PACKET), y)
DFLAGS += -DINCLUDE_DHCPC_FILTER_MULTICAST_PACKET
endif

ifeq ($(INCLUDE_IGMPPROXY_IGNORE_PPP), y)
DFLAGS += -DINCLUDE_IGMPPROXY_IGNORE_PPP
endif

ifeq ($(INCLUDE_SYNC_SECOND_CONN_FORWARD_RULES), y)
DFLAGS += -DINCLUDE_SYNC_SECOND_CONN_FORWARD_RULES
endif

#NOTE:INCLUDE_VOIP in voip.mak
ifeq ($(INCLUDE_VOIP), y)
DFLAGS += $(VOIP_DFLAGS)
DF_FLAGS += $(VOIP_CFLAGS)

ifeq ($(INCLUDE_VOIP_WITHOUT_CMM),y)
DFLAGS += -DINCLUDE_VOIP_WITHOUT_CMM
endif
endif


ifeq ($(INCLUDE_IPV6), y)

DFLAGS += -DINCLUDE_IPV6

ifeq ($(INCLUDE_IPV6_MLD), y)
DFLAGS += -DINCLUDE_IPV6_MLD
endif

ifeq ($(INCLUDE_IPV6_SLAAC), y)
DFLAGS += -DINCLUDE_IPV6_SLAAC
endif

ifeq ($(INCLUDE_IPV6_HTTP), y)
DFLAGS += -DINCLUDE_IPV6_HTTP
endif

ifeq ($(INCLUDE_IP6_WAN_NOT_ASSIGN_ADDR), y)
DFLAGS += -DINCLUDE_IP6_WAN_NOT_ASSIGN_ADDR
endif 

ifeq ($(INCLUDE_IPV6_PASS_THROUGH), y)
DFLAGS += -DINCLUDE_IPV6_PASS_THROUGH
endif

endif

ifeq ($(INCLUDE_USB), y)
DFLAGS += -DINCLUDE_USB
endif

ifeq ($(INCLUDE_USB_OVER_IP), y)
DFLAGS += -DINCLUDE_USB_OVER_IP
endif

ifeq ($(INCLUDE_USB_OVER_IP_TPLINK), y)
DFLAGS += -DINCLUDE_USB_OVER_IP_TPLINK
endif

ifeq ($(INCLUDE_USB_OVER_IP_KCODES), y)
DFLAGS += -DINCLUDE_USB_OVER_IP_KCODES
endif

ifeq ($(INCLUDE_USB_STORAGE), y)
DFLAGS += -DINCLUDE_USB_STORAGE
endif

ifeq ($(INCLUDE_USB_MEDIA_SERVER), y)
DFLAGS += -DINCLUDE_USB_MEDIA_SERVER
endif

ifeq ($(INCLUDE_USHARE), y)
DFLAGS += -DINCLUDE_USHARE
endif

ifeq ($(INCLUDE_MINIDLNA), y)
DFLAGS += -DINCLUDE_MINIDLNA
endif

ifeq ($(INCLUDE_LITE_MINIDLNA), y)
DFLAGS += -DINCLUDE_LITE_MINIDLNA
endif


ifeq ($(INCLUDE_USB_SAMBA_SERVER), y)
DFLAGS += -DINCLUDE_USB_SAMBA_SERVER
endif

ifeq ($(INCLUDE_USB_FTP_SERVER), y)
DFLAGS += -DINCLUDE_USB_FTP_SERVER
endif

ifeq ($(INCLUDE_USB_3G_DONGLE), y)
DFLAGS += -DINCLUDE_USB_3G_DONGLE
endif

ifeq ($(INCLUDE_USB_SYSFS), y)
DFLAGS += -DINCLUDE_USB_SYSFS
endif

ifeq ($(INCLUDE_VPN), y)
DFLAGS += -DINCLUDE_VPN
endif

ifeq ($(INCLUDE_IPSEC), y)
DFLAGS += -DINCLUDE_IPSEC
endif

ifeq ($(INCLUDE_OPENVPN_SERVER), y)
DFLAGS += -DINCLUDE_OPENVPN_SERVER
endif

ifeq ($(INCLUDE_PPTPVPN_SERVER), y)
DFLAGS += -DINCLUDE_PPTPVPN_SERVER
endif

ifeq ($(INCLUDE_WAN_MODE), y)
DFLAGS += -DINCLUDE_WAN_MODE
endif

ifeq ($(INCLUDE_FIREWALL), y)
DFLAGS += -DINCLUDE_FIREWALL
endif

ifeq ($(INCLUDE_FORWARD), y)
DFLAGS += -DINCLUDE_FORWARD
endif

ifeq ($(INCLUDE_VS), y)
DFLAGS += -DINCLUDE_VS
endif

ifeq ($(INCLUDE_PT), y)
DFLAGS += -DINCLUDE_PT
endif

ifeq ($(INCLUDE_DMZ), y)
DFLAGS += -DINCLUDE_DMZ
endif

ifeq ($(INCLUDE_UPNP), y)
DFLAGS += -DINCLUDE_UPNP
endif

ifeq ($(INCLUDE_ROUTE), y)
DFLAGS += -DINCLUDE_ROUTE
endif

ifeq ($(INCLUDE_STAT), y)
DFLAGS += -DINCLUDE_STAT
endif

ifeq ($(INCLUDE_DDOS), y)
DFLAGS += -DINCLUDE_DDOS
endif

ifeq ($(INCLUDE_QOS), y)
DFLAGS += -DINCLUDE_QOS
endif

#<< wuchao@2016-03-01, added
ifeq ($(INCLUDE_HW_QOS), y)
DFLAGS += -DINCLUDE_HW_QOS
endif

ifeq ($(INCLUDE_PORTABLE_APP), y)
DFLAGS += -DINCLUDE_PORTABLE_APP
endif

ifeq ($(INCLUDE_TC), y)
DFLAGS += -DINCLUDE_TC
endif
#>> endof added, wuchao@2016-03-01

ifeq ($(INCLUDE_NO_QOS), y)
DFLAGS += -DINCLUDE_NO_QOS
endif

ifeq ($(INCLUDE_E8_APP), y)
DFLAGS += -DINCLUDE_E8_APP
endif

ifeq ($(INCLUDE_ANNEXB), y)
DFLAGS += -DINCLUDE_ANNEXB
endif

ifeq ($(INCLUDE_ROUTE_BINDING), y)
DFLAGS += -DINCLUDE_ROUTE_BINDING
endif

ifeq ($(INCLUDE_POLICY_ROUTE), y)
DFLAGS += -DINCLUDE_POLICY_ROUTE
endif

ifeq ($(INCLUDE_DUAL_ACCESS), y)
DFLAGS += -DINCLUDE_DUAL_ACCESS
endif

ifeq ($(INCLUDE_L2TP), y)
DFLAGS += -DINCLUDE_L2TP
endif

ifeq ($(INCLUDE_PPTP), y)
DFLAGS += -DINCLUDE_PPTP
endif

ifeq ($(INCLUDE_ACL), y)
DFLAGS += -DINCLUDE_ACL
ifeq ($(INCLUDE_ACL_ADVANCE), y)
DFLAGS += -DINCLUDE_ACL_ADVANCE
endif
endif

ifeq ($(INCLUDE_CT_BASE), y)
DFLAGS += -DINCLUDE_CT_BASE
endif

ifeq ($(INCLUDE_MIC), y)
DFLAGS += -DINCLUDE_MIC
endif

#################### Cloud ######################
# added by Mei Zaihong, 2016-01-21
ifeq ($(INCLUDE_CLOUD), y)
DFLAGS += -DINCLUDE_CLOUD
endif
ifeq ($(INCLUDE_WAN_BLOCK), y)
DFLAGS += -DINCLUDE_WAN_BLOCK
endif
ifeq ($(INCLUDE_WAN_BLOCK_WAN_ERROR), y)
DFLAGS += -DINCLUDE_WAN_BLOCK_WAN_ERROR
endif
ifeq ($(INCLUDE_WAN_BLOCK_FW_UP_INFO), y)
DFLAGS += -DINCLUDE_WAN_BLOCK_FW_UP_INFO
endif
# end add, Mei Zaihong, 2016-01-21

ifeq ($(INCLUDE_X_TP_VLAN), y)
DFLAGS += -DINCLUDE_X_TP_VLAN
endif

ifeq ($(INCLUDE_FORBID_WAN_PING), y)
DFLAGS += -DINCLUDE_FORBID_WAN_PING
endif

###################### PON  ########################
ifeq ($(INCLUDE_PON), y)
DFLAGS += -DINCLUDE_PON
DFLAGS += -DINCLUDE_PON_ETH_WAN
endif

ifeq ($(INCLUDE_PON_MARVELL), y)
DFLAGS += -DNCLUDE_PON_MARVELL
endif

ifeq ($(INCLUDE_PON_BCM), y)
DFLAGS += -DINCLUDE_PON_BCM
endif

ifeq ($(INCLUDE_PON_MTK), y)
DFLAGS += -DINCLUDE_PON_MTK
endif

ifeq ($(INCLUDE_PON_EPON), y)
DFLAGS += -DINCLUDE_PON_EPON
DFLAGS += -DINCLUDE_EPON_INFO
export INCLUDE_PON_EPON=y
endif

ifeq ($(INCLUDE_PON_GPON), y)
DFLAGS += -DINCLUDE_PON_GPON
DFLAGS += -DINCLUDE_GPON_INFO
#added by 2015-6-8 Tang Wenhan
DFLAGS += -DPON_TYPE
export PON_TYPE=GPON
#added end.
export INCLUDE_PON_GPON=y
endif

ifeq ($(INCLUDE_CT_EPON), y)
DFLAGS += -DINCLUDE_CT_EPON
endif
ifeq ($(INCLUDE_CT_GPON), y)
DFLAGS += -DINCLUDE_CT_GPON
endif

#added by 2014-08-12 10:46:32  Chen Yingtao start
ifeq ($(INCLUDE_GPON_OMCI), y)
DFLAGS += -DINCLUDE_GPON_OMCI
endif

ifeq ($(INCLUDE_GPON_OMCI_VOICE), y)
DFLAGS += -DINCLUDE_GPON_OMCI_VOICE
endif

ifeq ($(INCLUDE_GPON_OMCI_MOCA), y)
DFLAGS += -DINCLUDE_GPON_OMCI_MOCA
endif

ifeq ($(INCLUDE_GPON_OMCI_CT), y)
DFLAGS += -DINCLUDE_GPON_OMCI_CT
export INCLUDE_GPON_OMCI_CT=y
endif

ifeq ($(INCLUDE_GPON_OMCI_IGNORE_PBIT_MATCH), y)
DFLAGS += -DINCLUDE_GPON_OMCI_IGNORE_PBIT_MATCH
endif

#added by 2014-08-12 10:46:32  Chen Yingtao end

ifeq ($(INCLUDE_GPON_CTC), y)
DFLAGS += -DINCLUDE_GPON_CTC
endif

ifeq ($(INCLUDE_GPON_FWDRULE), y)
DFLAGS += -DINCLUDE_GPON_FWDRULE
endif

ifeq ($(INCLUDE_GPON_IOT), y)
DFLAGS += -DINCLUDE_GPON_IOT
endif

ifeq ($(INCLUDE_WAN_PORT_BINDING), y)
DFLAGS += -DINCLUDE_WAN_PORT_BINDING
export INCLUDE_WAN_PORT_BINDING=y
endif

ifeq ($(INCLUDE_ISOLATE_LAN), y)
DFLAGS += -DINCLUDE_ISOLATE_LAN
endif

ifeq ($(INCLUDE_PON_IPTV), y)
DFLAGS += -DINCLUDE_PON_IPTV
endif

ifeq ($(INCLUDE_PON_IPTV_PUBLIC), y)
DFLAGS += -DINCLUDE_PON_IPTV_PUBLIC
endif

ifeq ($(INCLUDE_VLAN_SEARCH), y)
DFLAGS += -DINCLUDE_VLAN_SEARCH
endif

ifeq ($(INCLUDE_MULTI_BRIDGE), y)
DFLAGS += -DINCLUDE_MULTI_BRIDGE
endif

ifeq ($(INCLUDE_MULTI_PRIO), y)
DFLAGS += -DINCLUDE_MULTI_PRIO
WEBFLAGS += INCLUDE_MULTI_PRIO=1
else
WEBFLAGS += INCLUDE_MULTI_PRIO=0
endif

ifeq ($(INCLUDE_SEARCH_PRIO), y)
DFLAGS += -DINCLUDE_SEARCH_PRIO
endif

ifeq ($(INCLUDE_KEEP_AUTHINFO), y)
DFLAGS += -DINCLUDE_KEEP_AUTHINFO
endif

ifeq ($(INCLUDE_TP_EBTABLES), y)
DFLAGS += -DINCLUDE_TP_EBTABLES
endif

ifeq ($(INCLUDE_CT_LOID), y)
DFLAGS += -DINCLUDE_CT_LOID
endif

ifeq ($(strip $(INCLUDE_TR069_ACSURL_FROM_DHCP)), y)
DFLAGS += -DINCLUDE_TR069_ACSURL_FROM_DHCP
endif

ifeq ($(INCLUDE_PPPOE_RELAY), y)
DFLAGS += -DINCLUDE_PPPOE_RELAY
endif

ifeq ($(INCLUDE_PADT_BEFORE_DIAL), y)
WEBFLAGS += INCLUDE_PADT_BEFORE_DIAL=1
DFLAGS += -DINCLUDE_PADT_BEFORE_DIAL
export INCLUDE_PADT_BEFORE_DIAL=y
else
WEBFLAGS += INCLUDE_PADT_BEFORE_DIAL=0
endif

ifeq ($(INCLUDE_LOGIN_GDPR_ENCRYPT),y)
WEBFLAGS += INCLUDE_LOGIN_GDPR_ENCRYPT=1
DFLAGS += -DINCLUDE_LOGIN_GDPR_ENCRYPT
else
WEBFLAGS += INCLUDE_LOGIN_GDPR_ENCRYPT=0
endif

ifeq ($(strip $(INCLUDE_CPU_MEMORY_DISPLAY)), y)
DFLAGS += -DINCLUDE_CPU_MEMORY_DISPLAY
endif

###################### WEB ########################
ifneq ($(strip $(WEB_REBOOT_TIME)),)
WEBFLAGS += WEB_REBOOT_TIME=$(WEB_REBOOT_TIME)
else
WEBFLAGS += WEB_REBOOT_TIME=90
endif

ifneq ($(strip $(WEB_UPGRADE_TIME)),)
WEBFLAGS += WEB_UPGRADE_TIME=$(WEB_UPGRADE_TIME)
else
WEBFLAGS += WEB_UPGRADE_TIME=180
endif

ifeq ($(WEB_INCLUDE_DST), y)
WEBFLAGS += WEB_INCLUDE_DST=1
else
WEBFLAGS += WEB_INCLUDE_DST=0
endif

ifeq ($(WEB_INCLUDE_HELP), y)
WEBFLAGS += WEB_INCLUDE_HELP=1
else
WEBFLAGS += WEB_INCLUDE_HELP=0
endif

ifeq ($(INCLUDE_OPTION66), y)
WEBFLAGS += INCLUDE_OPTION66=1
else
WEBFLAGS += INCLUDE_OPTION66=0
endif

ifeq ($(WEB_INCLUDE_MOBILE_UI), y)
WEBFLAGS += WEB_INCLUDE_MOBILE_UI=1
DFLAGS += -DWEB_INCLUDE_MOBILE_UI
else
WEBFLAGS += WEB_INCLUDE_MOBILE_UI=0
endif

ifeq ($(INCLUDE_WEB_LOCAL), y)
WEBFLAGS += INCLUDE_WEB_LOCAL=1
else
WEBFLAGS += INCLUDE_WEB_LOCAL=0
endif

ifeq ($(INCLUDE_WEB_REGION), y)
WEBFLAGS += INCLUDE_WEB_REGION=1
else
WEBFLAGS += INCLUDE_WEB_REGION=0
endif

ifeq ($(strip $(INCLUDE_ONLY_ONE_LOGIN)),y)
DFLAGS += -DINCLUDE_ONLY_ONE_LOGIN
endif

ifeq ($(strip $(INCLUDE_HTTPD_REMOTE_NETWORK)),y)
DFLAGS += -DINCLUDE_HTTPD_REMOTE_NETWORK
endif
ifeq ($(strip $(INCLUDE_ROOT_ACCOUNT)),y)
DFLAGS += -DINCLUDE_ROOT_ACCOUNT
endif

ifeq ($(strip $(INCLUDE_SWITCH_MT7628)), y)
DFLAGS += -DINCLUDE_SWITCH_MT7628
endif

ifeq ($(strip $(INCLUDE_SWITCH_RTL8367S)), y)
DFLAGS += -DINCLUDE_SWITCH_RTL8367S
endif

ifeq ($(strip $(INCLUDE_SWITCH_MT7621)), y)
DFLAGS += -DINCLUDE_SWITCH_MT7621
endif

ifeq ($(strip $(INCLUDE_ETH_PORT_STATUS)), y)
DFLAGS += -DINCLUDE_ETH_PORT_STATUS
endif

ifeq ($(strip $(INCLUDE_LAN_INTERFACE_STATS)), y)
DFLAGS += -DINCLUDE_LAN_INTERFACE_STATS
endif

ifeq ($(strip $(INCLUDE_WAN_INTERFACE_STATS)), y)
DFLAGS += -DINCLUDE_WAN_INTERFACE_STATS
endif

ifeq ($(strip $(INCLUDE_MULTI_LANGUAGE)), y)
DFLAGS += -DINCLUDE_MULTI_LANGUAGE
endif

ifeq ($(INCLUDE_DAY_MONTH_YEAR), y)
WEBFLAGS += INCLUDE_DAY_MONTH_YEAR=1
else
WEBFLAGS += INCLUDE_DAY_MONTH_YEAR=0
endif

ifeq ($(INCLUDE_GIGABIT_WAN), y)
WEBFLAGS += INCLUDE_GIGABIT_WAN=1
else
WEBFLAGS += INCLUDE_GIGABIT_WAN=0
endif

ifeq ($(INCLUDE_MULTI_EWAN), y)
DFLAGS += -DINCLUDE_MULTI_EWAN
WEBFLAGS += INCLUDE_MULTI_EWAN=1
else
WEBFLAGS += INCLUDE_MULTI_EWAN=0
endif

ifeq ($(INCLUDE_MULTI_EWAN_MACVLAN), y)
DFLAGS += -DINCLUDE_MULTI_EWAN_MACVLAN
WEBFLAGS += INCLUDE_MULTI_EWAN_MACVLAN=1
else
WEBFLAGS += INCLUDE_MULTI_EWAN_MACVLAN=0
endif

ifeq ($(INCLUDE_MULTI_EWAN_8021P), y)
DFLAGS += -DINCLUDE_MULTI_EWAN_8021P
WEBFLAGS += INCLUDE_MULTI_EWAN_8021P=1
else
WEBFLAGS += INCLUDE_MULTI_EWAN_8021P=0
endif


ifeq ($(INCLUDE_ANTI_INTERFERENCE), y)
WEBFLAGS += INCLUDE_ANTI_INTERFERENCE=1
else
WEBFLAGS += INCLUDE_ANTI_INTERFERENCE=0
endif

ifeq ($(INCLUDE_5G_DFS), y)
WEBFLAGS += INCLUDE_5G_DFS=1
else
WEBFLAGS += INCLUDE_5G_DFS=0
endif

ifeq ($(INCLUDE_HIDE_GUESTNETWORK), y)
WEBFLAGS += INCLUDE_HIDE_GUESTNETWORK=1
else
WEBFLAGS += INCLUDE_HIDE_GUESTNETWORK=0
endif

ifeq ($(INCLUDE_SSL), y)
WEBFLAGS += INCLUDE_SSL=1
else
WEBFLAGS += INCLUDE_SSL=0
endif

ifeq ($(INCLUDE_CPU_MEMORY_DISPLAY), y)
WEBFLAGS += INCLUDE_CPU_MEMORY_DISPLAY=1
else
WEBFLAGS += INCLUDE_CPU_MEMORY_DISPLAY=0
endif

ifeq ($(INCLUDE_LED_NIGHTMODE), y)
WEBFLAGS += INCLUDE_LED_NIGHTMODE
DFLAGS += -DINCLUDE_LED_NIGHTMODE
else
WEBFLAGS += INCLUDE_LED_NIGHTMODE=0
endif

#################### Multi-language ######################
# added by Mei Zaihong, 2016-01-21
ifeq ($(INCLUDE_MULTI_LANGUAGE), y)
WEBFLAGS += INCLUDE_MULTI_LANGUAGE=1
else
WEBFLAGS += INCLUDE_MULTI_LANGUAGE=0
endif
ifeq ($(INCLUDE_LANGUAGE_EN_US), y)
WEBFLAGS += INCLUDE_LANGUAGE_EN_US=1
else
WEBFLAGS += INCLUDE_LANGUAGE_EN_US=0
endif
ifeq ($(INCLUDE_LANGUAGE_DE_DE), y)
WEBFLAGS += INCLUDE_LANGUAGE_DE_DE=1
else
WEBFLAGS += INCLUDE_LANGUAGE_DE_DE=0
endif
ifeq ($(INCLUDE_LANGUAGE_IT_IT), y)
WEBFLAGS += INCLUDE_LANGUAGE_IT_IT=1
else
WEBFLAGS += INCLUDE_LANGUAGE_IT_IT=0
endif
ifeq ($(INCLUDE_LANGUAGE_RU_RU), y)
WEBFLAGS += INCLUDE_LANGUAGE_RU_RU=1
else
WEBFLAGS += INCLUDE_LANGUAGE_RU_RU=0
endif
ifeq ($(INCLUDE_LANGUAGE_TR_TR), y)
WEBFLAGS += INCLUDE_LANGUAGE_TR_TR=1
else
WEBFLAGS += INCLUDE_LANGUAGE_TR_TR=0
endif
ifeq ($(INCLUDE_LANGUAGE_PL_PL), y)
WEBFLAGS += INCLUDE_LANGUAGE_PL_PL=1
else
WEBFLAGS += INCLUDE_LANGUAGE_PL_PL=0
endif
ifeq ($(INCLUDE_LANGUAGE_FR_FR), y)
WEBFLAGS += INCLUDE_LANGUAGE_FR_FR=1
else
WEBFLAGS += INCLUDE_LANGUAGE_FR_FR=0
endif
ifeq ($(INCLUDE_LANGUAGE_ES_ES), y)
WEBFLAGS += INCLUDE_LANGUAGE_ES_ES=1
else
WEBFLAGS += INCLUDE_LANGUAGE_ES_ES=0
endif
ifeq ($(INCLUDE_LANGUAGE_ES_LA), y)
WEBFLAGS += INCLUDE_LANGUAGE_ES_LA=1
else
WEBFLAGS += INCLUDE_LANGUAGE_ES_LA=0
endif
ifeq ($(INCLUDE_LANGUAGE_PT_PT), y)
WEBFLAGS += INCLUDE_LANGUAGE_PT_PT=1
else
WEBFLAGS += INCLUDE_LANGUAGE_PT_PT=0
endif
ifeq ($(INCLUDE_LANGUAGE_PT_BR), y)
WEBFLAGS += INCLUDE_LANGUAGE_PT_BR=1
else
WEBFLAGS += INCLUDE_LANGUAGE_PT_BR=0
endif
ifeq ($(INCLUDE_LANGUAGE_EL_GR), y)
WEBFLAGS += INCLUDE_LANGUAGE_EL_GR=1
else
WEBFLAGS += INCLUDE_LANGUAGE_EL_GR=0
endif
ifeq ($(INCLUDE_LANGUAGE_TH_TH), y)
WEBFLAGS += INCLUDE_LANGUAGE_TH_TH=1
else
WEBFLAGS += INCLUDE_LANGUAGE_TH_TH=0
endif
# end add, Mei Zaihong, 2016-01-21


####################### OEM/Case Option ########################
ifeq ($(INCLUDE_NO_INTERNET_LED), y)
DFLAGS += -DINCLUDE_NO_INTERNET_LED
export INCLUDE_NO_INTERNET_LED=y
endif

ifeq ($(INCLUDE_MTK_INTERNET_LED), y)
DFLAGS += -DINCLUDE_MTK_INTERNET_LED
export INCLUDE_MTK_INTERNET_LED=y
endif

ifeq ($(INCLUDE_TP_INTERNET_LED), y)
DFLAGS += -DINCLUDE_TP_INTERNET_LED
export INCLUDE_TP_INTERNET_LED=y
endif

ifeq ($(INCLUDE_REMOTE_UPGRADE), y)
DFLAGS += -DINCLUDE_REMOTE_UPGRADE
WEBFLAGS += INCLUDE_REMOTE_UPGRADE=1
else
WEBFLAGS += INCLUDE_REMOTE_UPGRADE=0
endif

ifeq ($(INCLUDE_VIETTEL), y)
DFLAGS += -DINCLUDE_VIETTEL
export INCLUDE_VIETTEL=y
endif

ifeq ($(INCLUDE_VIETTEL_HANOI), y)
DFLAGS += -DINCLUDE_VIETTEL_HANOI
export INCLUDE_VIETTEL_HANOI=y
endif

ifeq ($(INCLUDE_SIMAT), y)
DFLAGS += -DINCLUDE_SIMAT
export INCLUDE_SIMAT=y
WEBFLAGS += INCLUDE_SIMAT=1
else
WEBFLAGS += INCLUDE_SIMAT=0
endif

ifeq ($(INCLUDE_TEKCOM), y)
DFLAGS += -DINCLUDE_TEKCOM
export INCLUDE_TEKCOM=y
endif

DFLAGS += -DINCLUDE_$(MODEL)
ifneq ($(SPEC),)
DFLAGS += -DINCLUDE_$(MODEL)_$(SPEC)
endif