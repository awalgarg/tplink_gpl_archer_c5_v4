/*  Copyright(c) 2009-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		rtl8367_api.c
 * brief		
 * details	
 *
 * author		Yuan Shang
 * version	
 * date		12Oct13
 *
 * history 	\arg	
 */
#include "rtl8367rb_api.h"
#include <command.h>
/**************************************************************************************************/
/*                                           DEFINES                                              */
/**************************************************************************************************/
#define RTL8367_DEBUG(args...) do{printf("%s(%d):", __FUNCTION__, __LINE__);printf(args);}while(0);

/* MDC_MDIO */
#define MDC_MDIO_DUMMY_ID           0x0
#define MDC_MDIO_CTRL0_REG          31
#define MDC_MDIO_START_REG          29
#define MDC_MDIO_CTRL1_REG          21
#define MDC_MDIO_ADDRESS_REG        23
#define MDC_MDIO_DATA_WRITE_REG     24
#define MDC_MDIO_DATA_READ_REG      25
#define MDC_MDIO_PREAMBLE_LEN       32
 
#define MDC_MDIO_START_OP          0xFFFF
#define MDC_MDIO_ADDR_OP           0x000E
#define MDC_MDIO_READ_OP           0x0001
#define MDC_MDIO_WRITE_OP          0x0003


/* EXT Interface */
#define RTL8367B_REG_DIGITAL_INTERFACE_SELECT	0x1305
#define RTL8367B_REG_DIGITAL_INTERFACE_SELECT_1 0x13c3

#define RTL8367B_REG_DIGITAL_INTERFACE1_FORCE	0x1311
#define RTL8367B_REG_DIGITAL_INTERFACE2_FORCE	0x13c4


#define    RTL8367B_SELECT_GMII_1_OFFSET    4



#define RTL8367B_REG_BYPASS_LINE_RATE 0x03f7


#define RTL8367_PORT0_STATUS_REG	0x1352

#define RTL8367B_REG_VLAN_MEMBER_CONFIGURATION0_CTRL0 0x0728
#define RTL8367B_REG_TABLE_WRITE_DATA0 0x0510
#define RTL8367B_REG_TABLE_ACCESS_ADDR 0x0501
#define RTL8367B_REG_TABLE_ACCESS_CTRL 0x0500
#define RTL8367B_REG_VLAN_PVID_CTRL0 0x0700
#define RTL8367B_REG_VLAN_PORTBASED_PRIORITY_CTRL0 0x0851

#define RTL8367B_REG_PORT0_MISC_CFG 0x000e
#define RTL8367B_REG_VLAN_CTRL 0x07a8




#define RTL8367B_VLAN_MEMBER_CONFIGURATION_BASE RTL8367B_REG_VLAN_MEMBER_CONFIGURATION0_CTRL0
#define RTL8367B_TABLE_ACCESS_WRDATA_BASE RTL8367B_REG_TABLE_WRITE_DATA0
#define RTL8367B_TABLE_ACCESS_ADDR_REG RTL8367B_REG_TABLE_ACCESS_ADDR
#define RTL8367B_TABLE_ACCESS_CTRL_REG RTL8367B_REG_TABLE_ACCESS_CTRL
#define RTL8367B_VLAN_PVID_CTRL_BASE RTL8367B_REG_VLAN_PVID_CTRL0
#define RTL8367B_VLAN_PORTBASED_PRIORITY_BASE RTL8367B_REG_VLAN_PORTBASED_PRIORITY_CTRL0

#define RTL8367B_PORT_MISC_CFG_BASE RTL8367B_REG_PORT0_MISC_CFG

#define RTL8367B_REG_MIB_COUNTER0 0x1000
#define RTL8367B_REG_MIB_COUNTER1 0x1001
#define RTL8367B_REG_MIB_COUNTER2 0x1002
#define RTL8367B_REG_MIB_COUNTER3 0x1003
#define RTL8367B_REG_MIB_ADDRESS  0x1004
#define RTL8367B_REG_MIB_CTRL0 0x1005
#define RTL8367B_MIB_CTRL_REG RTL8367B_REG_MIB_CTRL0




#define RTL8367B_CVIDXNO                    32
#define RTL8367B_CVIDXMAX                   (RTL8367B_CVIDXNO-1)


/**************************************************************************************************/
/*                                           TYPES                                                */
/**************************************************************************************************/
typedef enum rtk_mode_ext_e
{
    MODE_EXT_DISABLE = 0,
    MODE_EXT_RGMII,
    MODE_EXT_MII_MAC,
    MODE_EXT_MII_PHY,
    MODE_EXT_TMII_MAC,
    MODE_EXT_TMII_PHY,
    MODE_EXT_GMII,
    MODE_EXT_RMII_MAC,
    MODE_EXT_RMII_PHY,
    MODE_EXT_RGMII_33V,
    MODE_EXT_END
} rtk_mode_ext_t;

typedef enum rtk_ext_port_e
{
    EXT_PORT_0 = 0,
    EXT_PORT_1,
    EXT_PORT_2,
    EXT_PORT_END
}rtk_ext_port_t;

typedef struct  rtl8367b_port_ability_s{
#if 0//def _LITTLE_ENDIAN
    u16 speed:2;
    u16 duplex:1;
    u16 reserve1:1;
    u16 link:1;
    u16 rxpause:1;
    u16 txpause:1;
    u16 nway:1;
    u16 mstmode:1;
    u16 mstfault:1;
    u16 reserve2:2;
    u16 forcemode:1;
    u16 reserve3:3;
#else
    u16 reserve3:3;
    u16 forcemode:1;
    u16 reserve2:2;
    u16 mstfault:1;
    u16 mstmode:1;
    u16 nway:1;
    u16 txpause:1;
    u16 rxpause:1;
    u16 link:1;
    u16 reserve1:1;
    u16 duplex:1;
    u16 speed:2;
#endif
}rtl8367b_port_ability_t;

typedef enum RTL8367B_MIBCOUNTER_E{

    /* RX */
	ifInOctets = 0,

	dot3StatsFCSErrors,
	dot3StatsSymbolErrors,
	dot3InPauseFrames,
	dot3ControlInUnknownOpcodes,	
	
	etherStatsFragments,
	etherStatsJabbers,
	ifInUcastPkts,
	etherStatsDropEvents,

    ifInMulticastPkts,
    ifInBroadcastPkts,
    inMldChecksumError,
    inIgmpChecksumError,
    inMldSpecificQuery,
    inMldGeneralQuery,
    inIgmpSpecificQuery,
    inIgmpGeneralQuery,
    inMldLeaves,
    inIgmpLeaves,

    /* TX/RX */
	etherStatsOctets,

	etherStatsUnderSizePkts,
	etherOversizeStats,
	etherStatsPkts64Octets,
	etherStatsPkts65to127Octets,
	etherStatsPkts128to255Octets,
	etherStatsPkts256to511Octets,
	etherStatsPkts512to1023Octets,
	etherStatsPkts1024to1518Octets,

    /* TX */
	ifOutOctets,

	dot3StatsSingleCollisionFrames,
	dot3StatMultipleCollisionFrames,
	dot3sDeferredTransmissions,
	dot3StatsLateCollisions,
	etherStatsCollisions,
	dot3StatsExcessiveCollisions,
	dot3OutPauseFrames,
    ifOutDiscards,

    /* ALE */
	dot1dTpPortInDiscards,
	ifOutUcastPkts,
	ifOutMulticastPkts,
	ifOutBroadcastPkts,
	outOampduPkts,
	inOampduPkts,

    inIgmpJoinsSuccess,
    inIgmpJoinsFail,
    inMldJoinsSuccess,
    inMldJoinsFail,
    inReportSuppressionDrop,
    inLeaveSuppressionDrop,
    outIgmpReports,
    outIgmpLeaves,
    outIgmpGeneralQuery,
    outIgmpSpecificQuery,
    outMldReports,
    outMldLeaves,
    outMldGeneralQuery,
    outMldSpecificQuery,
    inKnownMulticastPkts,

	/*Device only */	
	dot1dTpLearnedEntryDiscards,
	RTL8367B_MIBS_NUMBER,
	
}RTL8367B_MIBCOUNTER;



/**************************************************************************************************/
/*                                           EXTERN_PROTOTYPES                                    */
/**************************************************************************************************/
extern int mii_mgr_read(u32 phy_addr, u32 phy_register, u32 *read_data);
extern int mii_mgr_write(u32 phy_addr, u32 phy_register, u32 write_data);

/**************************************************************************************************/
/*                                           LOCAL_PROTOTYPES                                     */
/**************************************************************************************************/
u32 rtl8367b_setAsicRegBit(u32 reg, u32 bit, u32 value);
u32 rtl8367b_setAsicRegBits(u32 reg, u32 bits, u32 value);
ret_t rtl8367b_setAsicReg(rtk_uint32 reg, rtk_uint32 value);
ret_t rtl8367b_setAsicReg(rtk_uint32 reg, rtk_uint32 value);
ret_t rtl8367b_setAsicPHYReg( rtk_uint32 phyNo, rtk_uint32 phyAddr, rtk_uint32 value);
ret_t rtl8367b_getAsicPHYReg( rtk_uint32 phyNo, rtk_uint32 phyAddr, rtk_uint32 *value);
rtk_api_ret_t rtk_port_phyTestModeAll_set(rtk_port_t port, rtk_port_phy_test_mode_t mode);
rtk_api_ret_t rtk_port_phyTestModeAll_get(rtk_port_t port, rtk_port_phy_test_mode_t *pMode);
static rtk_api_ret_t _rtk_switch_init_setreg(rtk_uint32 reg, rtk_uint32 data);

/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/
rtk_uint16	(*init_para)[2];
rtk_uint16      init_size;

rtk_uint16 ChipData30[][2]= {
/*Code of Func*/
{0x1B03, 0x0876}, {0x1200, 0x7FC4}, {0x0301, 0x0026}, {0x1722, 0x0E14},
{0x205F, 0x0002}, {0x2059, 0x1A00}, {0x205F, 0x0000}, {0x207F, 0x0002},
{0x2077, 0x0000}, {0x2078, 0x0000}, {0x2079, 0x0000}, {0x207A, 0x0000},
{0x207B, 0x0000}, {0x207F, 0x0000}, {0x205F, 0x0002}, {0x2053, 0x0000},
{0x2054, 0x0000}, {0x2055, 0x0000}, {0x2056, 0x0000}, {0x2057, 0x0000},
{0x205F, 0x0000}, {0x12A4, 0x110A}, {0x12A6, 0x150A}, {0x13F1, 0x0013},
{0x13F4, 0x0010}, {0x13F5, 0x0000}, {0x0018, 0x0F00}, {0x0038, 0x0F00},
{0x0058, 0x0F00}, {0x0078, 0x0F00}, {0x0098, 0x0F00}, {0x12B6, 0x0C02},
{0x12B7, 0x030F}, {0x12B8, 0x11FF}, {0x12BC, 0x0004}, {0x1362, 0x0115},
{0x1363, 0x0002}, {0x1363, 0x0000}, {0x133F, 0x0030}, {0x133E, 0x000E},
{0x221F, 0x0007}, {0x221E, 0x002D}, {0x2218, 0xF030}, {0x221F, 0x0007},
{0x221E, 0x0023}, {0x2216, 0x0005}, {0x2215, 0x00B9}, {0x2219, 0x0044},
{0x2215, 0x00BA}, {0x2219, 0x0020}, {0x2215, 0x00BB}, {0x2219, 0x00C1},
{0x2215, 0x0148}, {0x2219, 0x0096}, {0x2215, 0x016E}, {0x2219, 0x0026},
{0x2216, 0x0000}, {0x2216, 0x0000}, {0x221E, 0x002D}, {0x2218, 0xF010},
{0x221F, 0x0007}, {0x221E, 0x0020}, {0x2215, 0x0D00}, {0x221F, 0x0000},
{0x221F, 0x0000}, {0x2217, 0x2160}, {0x221F, 0x0001}, {0x2210, 0xF25E},
{0x221F, 0x0007}, {0x221E, 0x0042}, {0x2215, 0x0F00}, {0x2215, 0x0F00},
{0x2216, 0x7408}, {0x2215, 0x0E00}, {0x2215, 0x0F00}, {0x2215, 0x0F01},
{0x2216, 0x4000}, {0x2215, 0x0E01}, {0x2215, 0x0F01}, {0x2215, 0x0F02},
{0x2216, 0x9400}, {0x2215, 0x0E02}, {0x2215, 0x0F02}, {0x2215, 0x0F03},
{0x2216, 0x7408}, {0x2215, 0x0E03}, {0x2215, 0x0F03}, {0x2215, 0x0F04},
{0x2216, 0x4008}, {0x2215, 0x0E04}, {0x2215, 0x0F04}, {0x2215, 0x0F05},
{0x2216, 0x9400}, {0x2215, 0x0E05}, {0x2215, 0x0F05}, {0x2215, 0x0F06},
{0x2216, 0x0803}, {0x2215, 0x0E06}, {0x2215, 0x0F06}, {0x2215, 0x0D00},
{0x2215, 0x0100}, {0x221F, 0x0001}, {0x2210, 0xF05E}, {0x221F, 0x0000},
{0x2217, 0x2100}, {0x221F, 0x0000}, {0x220D, 0x0003}, {0x220E, 0x0015},
{0x220D, 0x4003}, {0x220E, 0x0006}, {0x221F, 0x0000}, {0x2200, 0x1340},
{0x133F, 0x0010}, {0x12A0, 0x0058}, {0x12A1, 0x0058}, {0x133E, 0x000E},
{0x133F, 0x0030}, {0x221F, 0x0000}, {0x2210, 0x0166}, {0x221F, 0x0000},
{0x133E, 0x000E}, {0x133F, 0x0010}, {0x133F, 0x0030}, {0x133E, 0x000E},
{0x221F, 0x0005}, {0x2205, 0xFFF6}, {0x2206, 0x0080}, {0x2205, 0x8B6E},
{0x2206, 0x0000}, {0x220F, 0x0100}, {0x2205, 0x8000}, {0x2206, 0x0280},
{0x2206, 0x28F7}, {0x2206, 0x00E0}, {0x2206, 0xFFF7}, {0x2206, 0xA080},
{0x2206, 0x02AE}, {0x2206, 0xF602}, {0x2206, 0x0153}, {0x2206, 0x0201},
{0x2206, 0x6602}, {0x2206, 0x80B9}, {0x2206, 0xE08B}, {0x2206, 0x8CE1},
{0x2206, 0x8B8D}, {0x2206, 0x1E01}, {0x2206, 0xE18B}, {0x2206, 0x8E1E},
{0x2206, 0x01A0}, {0x2206, 0x00E7}, {0x2206, 0xAEDB}, {0x2206, 0xEEE0},
{0x2206, 0x120E}, {0x2206, 0xEEE0}, {0x2206, 0x1300}, {0x2206, 0xEEE0},
{0x2206, 0x2001}, {0x2206, 0xEEE0}, {0x2206, 0x2166}, {0x2206, 0xEEE0},
{0x2206, 0xC463}, {0x2206, 0xEEE0}, {0x2206, 0xC5E8}, {0x2206, 0xEEE0},
{0x2206, 0xC699}, {0x2206, 0xEEE0}, {0x2206, 0xC7C2}, {0x2206, 0xEEE0},
{0x2206, 0xC801}, {0x2206, 0xEEE0}, {0x2206, 0xC913}, {0x2206, 0xEEE0},
{0x2206, 0xCA30}, {0x2206, 0xEEE0}, {0x2206, 0xCB3E}, {0x2206, 0xEEE0},
{0x2206, 0xDCE1}, {0x2206, 0xEEE0}, {0x2206, 0xDD00}, {0x2206, 0xEEE2},
{0x2206, 0x0001}, {0x2206, 0xEEE2}, {0x2206, 0x0100}, {0x2206, 0xEEE4},
{0x2206, 0x8860}, {0x2206, 0xEEE4}, {0x2206, 0x8902}, {0x2206, 0xEEE4},
{0x2206, 0x8C00}, {0x2206, 0xEEE4}, {0x2206, 0x8D30}, {0x2206, 0xEEEA},
{0x2206, 0x1480}, {0x2206, 0xEEEA}, {0x2206, 0x1503}, {0x2206, 0xEEEA},
{0x2206, 0xC600}, {0x2206, 0xEEEA}, {0x2206, 0xC706}, {0x2206, 0xEE85},
{0x2206, 0xEE00}, {0x2206, 0xEE85}, {0x2206, 0xEF00}, {0x2206, 0xEE8B},
{0x2206, 0x6750}, {0x2206, 0xEE8B}, {0x2206, 0x6632}, {0x2206, 0xEE8A},
{0x2206, 0xD448}, {0x2206, 0xEE8A}, {0x2206, 0xD548}, {0x2206, 0xEE8A},
{0x2206, 0xD649}, {0x2206, 0xEE8A}, {0x2206, 0xD7F8}, {0x2206, 0xEE8B},
{0x2206, 0x85E2}, {0x2206, 0xEE8B}, {0x2206, 0x8700}, {0x2206, 0xEEFF},
{0x2206, 0xF600}, {0x2206, 0xEEFF}, {0x2206, 0xF7FC}, {0x2206, 0x04F8},
{0x2206, 0xE08B}, {0x2206, 0x8EAD}, {0x2206, 0x2023}, {0x2206, 0xF620},
{0x2206, 0xE48B}, {0x2206, 0x8E02}, {0x2206, 0x2877}, {0x2206, 0x0225},
{0x2206, 0xC702}, {0x2206, 0x26A1}, {0x2206, 0x0281}, {0x2206, 0xB302},
{0x2206, 0x8496}, {0x2206, 0x0202}, {0x2206, 0xA102}, {0x2206, 0x27F1},
{0x2206, 0x0228}, {0x2206, 0xF902}, {0x2206, 0x2AA0}, {0x2206, 0x0282},
{0x2206, 0xB8E0}, {0x2206, 0x8B8E}, {0x2206, 0xAD21}, {0x2206, 0x08F6},
{0x2206, 0x21E4}, {0x2206, 0x8B8E}, {0x2206, 0x0202}, {0x2206, 0x80E0},
{0x2206, 0x8B8E}, {0x2206, 0xAD22}, {0x2206, 0x05F6}, {0x2206, 0x22E4},
{0x2206, 0x8B8E}, {0x2206, 0xE08B}, {0x2206, 0x8EAD}, {0x2206, 0x2305},
{0x2206, 0xF623}, {0x2206, 0xE48B}, {0x2206, 0x8EE0}, {0x2206, 0x8B8E},
{0x2206, 0xAD24}, {0x2206, 0x08F6}, {0x2206, 0x24E4}, {0x2206, 0x8B8E},
{0x2206, 0x0227}, {0x2206, 0x6AE0}, {0x2206, 0x8B8E}, {0x2206, 0xAD25},
{0x2206, 0x05F6}, {0x2206, 0x25E4}, {0x2206, 0x8B8E}, {0x2206, 0xE08B},
{0x2206, 0x8EAD}, {0x2206, 0x260B}, {0x2206, 0xF626}, {0x2206, 0xE48B},
{0x2206, 0x8E02}, {0x2206, 0x830D}, {0x2206, 0x021D}, {0x2206, 0x6BE0},
{0x2206, 0x8B8E}, {0x2206, 0xAD27}, {0x2206, 0x05F6}, {0x2206, 0x27E4},
{0x2206, 0x8B8E}, {0x2206, 0x0281}, {0x2206, 0x4402}, {0x2206, 0x045C},
{0x2206, 0xFC04}, {0x2206, 0xF8E0}, {0x2206, 0x8B83}, {0x2206, 0xAD23},
{0x2206, 0x30E0}, {0x2206, 0xE022}, {0x2206, 0xE1E0}, {0x2206, 0x2359},
{0x2206, 0x02E0}, {0x2206, 0x85EF}, {0x2206, 0xE585}, {0x2206, 0xEFAC},
{0x2206, 0x2907}, {0x2206, 0x1F01}, {0x2206, 0x9E51}, {0x2206, 0xAD29},
{0x2206, 0x20E0}, {0x2206, 0x8B83}, {0x2206, 0xAD21}, {0x2206, 0x06E1},
{0x2206, 0x8B84}, {0x2206, 0xAD28}, {0x2206, 0x42E0}, {0x2206, 0x8B85},
{0x2206, 0xAD21}, {0x2206, 0x06E1}, {0x2206, 0x8B84}, {0x2206, 0xAD29},
{0x2206, 0x36BF}, {0x2206, 0x34BF}, {0x2206, 0x022C}, {0x2206, 0x31AE},
{0x2206, 0x2EE0}, {0x2206, 0x8B83}, {0x2206, 0xAD21}, {0x2206, 0x10E0},
{0x2206, 0x8B84}, {0x2206, 0xF620}, {0x2206, 0xE48B}, {0x2206, 0x84EE},
{0x2206, 0x8ADA}, {0x2206, 0x00EE}, {0x2206, 0x8ADB}, {0x2206, 0x00E0},
{0x2206, 0x8B85}, {0x2206, 0xAD21}, {0x2206, 0x0CE0}, {0x2206, 0x8B84},
{0x2206, 0xF621}, {0x2206, 0xE48B}, {0x2206, 0x84EE}, {0x2206, 0x8B72},
{0x2206, 0xFFBF}, {0x2206, 0x34C2}, {0x2206, 0x022C}, {0x2206, 0x31FC},
{0x2206, 0x04F8}, {0x2206, 0xFAEF}, {0x2206, 0x69E0}, {0x2206, 0x8B85},
{0x2206, 0xAD21}, {0x2206, 0x42E0}, {0x2206, 0xE022}, {0x2206, 0xE1E0},
{0x2206, 0x2358}, {0x2206, 0xC059}, {0x2206, 0x021E}, {0x2206, 0x01E1},
{0x2206, 0x8B72}, {0x2206, 0x1F10}, {0x2206, 0x9E2F}, {0x2206, 0xE48B},
{0x2206, 0x72AD}, {0x2206, 0x2123}, {0x2206, 0xE18B}, {0x2206, 0x84F7},
{0x2206, 0x29E5}, {0x2206, 0x8B84}, {0x2206, 0xAC27}, {0x2206, 0x10AC},
{0x2206, 0x2605}, {0x2206, 0x0205}, {0x2206, 0x23AE}, {0x2206, 0x1602},
{0x2206, 0x0535}, {0x2206, 0x0282}, {0x2206, 0x30AE}, {0x2206, 0x0E02},
{0x2206, 0x056A}, {0x2206, 0x0282}, {0x2206, 0x75AE}, {0x2206, 0x0602},
{0x2206, 0x04DC}, {0x2206, 0x0282}, {0x2206, 0x04EF}, {0x2206, 0x96FE},
{0x2206, 0xFC04}, {0x2206, 0xF8F9}, {0x2206, 0xE08B}, {0x2206, 0x87AD},
{0x2206, 0x2321}, {0x2206, 0xE0EA}, {0x2206, 0x14E1}, {0x2206, 0xEA15},
{0x2206, 0xAD26}, {0x2206, 0x18F6}, {0x2206, 0x27E4}, {0x2206, 0xEA14},
{0x2206, 0xE5EA}, {0x2206, 0x15F6}, {0x2206, 0x26E4}, {0x2206, 0xEA14},
{0x2206, 0xE5EA}, {0x2206, 0x15F7}, {0x2206, 0x27E4}, {0x2206, 0xEA14},
{0x2206, 0xE5EA}, {0x2206, 0x15FD}, {0x2206, 0xFC04}, {0x2206, 0xF8F9},
{0x2206, 0xE08B}, {0x2206, 0x87AD}, {0x2206, 0x233A}, {0x2206, 0xAD22},
{0x2206, 0x37E0}, {0x2206, 0xE020}, {0x2206, 0xE1E0}, {0x2206, 0x21AC},
{0x2206, 0x212E}, {0x2206, 0xE0EA}, {0x2206, 0x14E1}, {0x2206, 0xEA15},
{0x2206, 0xF627}, {0x2206, 0xE4EA}, {0x2206, 0x14E5}, {0x2206, 0xEA15},
{0x2206, 0xE2EA}, {0x2206, 0x12E3}, {0x2206, 0xEA13}, {0x2206, 0x5A8F},
{0x2206, 0x6A20}, {0x2206, 0xE6EA}, {0x2206, 0x12E7}, {0x2206, 0xEA13},
{0x2206, 0xF726}, {0x2206, 0xE4EA}, {0x2206, 0x14E5}, {0x2206, 0xEA15},
{0x2206, 0xF727}, {0x2206, 0xE4EA}, {0x2206, 0x14E5}, {0x2206, 0xEA15},
{0x2206, 0xFDFC}, {0x2206, 0x04F8}, {0x2206, 0xF9E0}, {0x2206, 0x8B87},
{0x2206, 0xAD23}, {0x2206, 0x38AD}, {0x2206, 0x2135}, {0x2206, 0xE0E0},
{0x2206, 0x20E1}, {0x2206, 0xE021}, {0x2206, 0xAC21}, {0x2206, 0x2CE0},
{0x2206, 0xEA14}, {0x2206, 0xE1EA}, {0x2206, 0x15F6}, {0x2206, 0x27E4},
{0x2206, 0xEA14}, {0x2206, 0xE5EA}, {0x2206, 0x15E2}, {0x2206, 0xEA12},
{0x2206, 0xE3EA}, {0x2206, 0x135A}, {0x2206, 0x8FE6}, {0x2206, 0xEA12},
{0x2206, 0xE7EA}, {0x2206, 0x13F7}, {0x2206, 0x26E4}, {0x2206, 0xEA14},
{0x2206, 0xE5EA}, {0x2206, 0x15F7}, {0x2206, 0x27E4}, {0x2206, 0xEA14},
{0x2206, 0xE5EA}, {0x2206, 0x15FD}, {0x2206, 0xFC04}, {0x2206, 0xF8FA},
{0x2206, 0xEF69}, {0x2206, 0xE08B}, {0x2206, 0x86AD}, {0x2206, 0x2146},
{0x2206, 0xE0E0}, {0x2206, 0x22E1}, {0x2206, 0xE023}, {0x2206, 0x58C0},
{0x2206, 0x5902}, {0x2206, 0x1E01}, {0x2206, 0xE18B}, {0x2206, 0x651F},
{0x2206, 0x109E}, {0x2206, 0x33E4}, {0x2206, 0x8B65}, {0x2206, 0xAD21},
{0x2206, 0x22AD}, {0x2206, 0x272A}, {0x2206, 0xD400}, {0x2206, 0x01BF},
{0x2206, 0x34F2}, {0x2206, 0x022C}, {0x2206, 0xA2BF}, {0x2206, 0x34F5},
{0x2206, 0x022C}, {0x2206, 0xE0E0}, {0x2206, 0x8B67}, {0x2206, 0x1B10},
{0x2206, 0xAA14}, {0x2206, 0xE18B}, {0x2206, 0x660D}, {0x2206, 0x1459},
{0x2206, 0x0FAE}, {0x2206, 0x05E1}, {0x2206, 0x8B66}, {0x2206, 0x590F},
{0x2206, 0xBF85}, {0x2206, 0x6102}, {0x2206, 0x2CA2}, {0x2206, 0xEF96},
{0x2206, 0xFEFC}, {0x2206, 0x04F8}, {0x2206, 0xF9FA}, {0x2206, 0xFBEF},
{0x2206, 0x79E2}, {0x2206, 0x8AD2}, {0x2206, 0xAC19}, {0x2206, 0x2DE0},
{0x2206, 0xE036}, {0x2206, 0xE1E0}, {0x2206, 0x37EF}, {0x2206, 0x311F},
{0x2206, 0x325B}, {0x2206, 0x019E}, {0x2206, 0x1F7A}, {0x2206, 0x0159},
{0x2206, 0x019F}, {0x2206, 0x0ABF}, {0x2206, 0x348E}, {0x2206, 0x022C},
{0x2206, 0x31F6}, {0x2206, 0x06AE}, {0x2206, 0x0FF6}, {0x2206, 0x0302},
{0x2206, 0x0470}, {0x2206, 0xF703}, {0x2206, 0xF706}, {0x2206, 0xBF34},
{0x2206, 0x9302}, {0x2206, 0x2C31}, {0x2206, 0xAC1A}, {0x2206, 0x25E0},
{0x2206, 0xE022}, {0x2206, 0xE1E0}, {0x2206, 0x23EF}, {0x2206, 0x300D},
{0x2206, 0x311F}, {0x2206, 0x325B}, {0x2206, 0x029E}, {0x2206, 0x157A},
{0x2206, 0x0258}, {0x2206, 0xC4A0}, {0x2206, 0x0408}, {0x2206, 0xBF34},
{0x2206, 0x9E02}, {0x2206, 0x2C31}, {0x2206, 0xAE06}, {0x2206, 0xBF34},
{0x2206, 0x9C02}, {0x2206, 0x2C31}, {0x2206, 0xAC1B}, {0x2206, 0x4AE0},
{0x2206, 0xE012}, {0x2206, 0xE1E0}, {0x2206, 0x13EF}, {0x2206, 0x300D},
{0x2206, 0x331F}, {0x2206, 0x325B}, {0x2206, 0x1C9E}, {0x2206, 0x3AEF},
{0x2206, 0x325B}, {0x2206, 0x1C9F}, {0x2206, 0x09BF}, {0x2206, 0x3498},
{0x2206, 0x022C}, {0x2206, 0x3102}, {0x2206, 0x83C5}, {0x2206, 0x5A03},
{0x2206, 0x0D03}, {0x2206, 0x581C}, {0x2206, 0x1E20}, {0x2206, 0x0207},
{0x2206, 0xA0A0}, {0x2206, 0x000E}, {0x2206, 0x0284}, {0x2206, 0x17AD},
{0x2206, 0x1817}, {0x2206, 0xBF34}, {0x2206, 0x9A02}, {0x2206, 0x2C31},
{0x2206, 0xAE0F}, {0x2206, 0xBF34}, {0x2206, 0xC802}, {0x2206, 0x2C31},
{0x2206, 0xBF34}, {0x2206, 0xC502}, {0x2206, 0x2C31}, {0x2206, 0x0284},
{0x2206, 0x52E6}, {0x2206, 0x8AD2}, {0x2206, 0xEF97}, {0x2206, 0xFFFE},
{0x2206, 0xFDFC}, {0x2206, 0x04F8}, {0x2206, 0xBF34}, {0x2206, 0xDA02},
{0x2206, 0x2CE0}, {0x2206, 0xE58A}, {0x2206, 0xD3BF}, {0x2206, 0x34D4},
{0x2206, 0x022C}, {0x2206, 0xE00C}, {0x2206, 0x1159}, {0x2206, 0x02E0},
{0x2206, 0x8AD3}, {0x2206, 0x1E01}, {0x2206, 0xE48A}, {0x2206, 0xD3D1},
{0x2206, 0x00BF}, {0x2206, 0x34DA}, {0x2206, 0x022C}, {0x2206, 0xA2D1},
{0x2206, 0x01BF}, {0x2206, 0x34D4}, {0x2206, 0x022C}, {0x2206, 0xA2BF},
{0x2206, 0x34CB}, {0x2206, 0x022C}, {0x2206, 0xE0E5}, {0x2206, 0x8ACE},
{0x2206, 0xBF85}, {0x2206, 0x6702}, {0x2206, 0x2CE0}, {0x2206, 0xE58A},
{0x2206, 0xCFBF}, {0x2206, 0x8564}, {0x2206, 0x022C}, {0x2206, 0xE0E5},
{0x2206, 0x8AD0}, {0x2206, 0xBF85}, {0x2206, 0x6A02}, {0x2206, 0x2CE0},
{0x2206, 0xE58A}, {0x2206, 0xD1FC}, {0x2206, 0x04F8}, {0x2206, 0xE18A},
{0x2206, 0xD1BF}, {0x2206, 0x856A}, {0x2206, 0x022C}, {0x2206, 0xA2E1},
{0x2206, 0x8AD0}, {0x2206, 0xBF85}, {0x2206, 0x6402}, {0x2206, 0x2CA2},
{0x2206, 0xE18A}, {0x2206, 0xCFBF}, {0x2206, 0x8567}, {0x2206, 0x022C},
{0x2206, 0xA2E1}, {0x2206, 0x8ACE}, {0x2206, 0xBF34}, {0x2206, 0xCB02},
{0x2206, 0x2CA2}, {0x2206, 0xE18A}, {0x2206, 0xD3BF}, {0x2206, 0x34DA},
{0x2206, 0x022C}, {0x2206, 0xA2E1}, {0x2206, 0x8AD3}, {0x2206, 0x0D11},
{0x2206, 0xBF34}, {0x2206, 0xD402}, {0x2206, 0x2CA2}, {0x2206, 0xFC04},
{0x2206, 0xF9A0}, {0x2206, 0x0405}, {0x2206, 0xE38A}, {0x2206, 0xD4AE},
{0x2206, 0x13A0}, {0x2206, 0x0805}, {0x2206, 0xE38A}, {0x2206, 0xD5AE},
{0x2206, 0x0BA0}, {0x2206, 0x0C05}, {0x2206, 0xE38A}, {0x2206, 0xD6AE},
{0x2206, 0x03E3}, {0x2206, 0x8AD7}, {0x2206, 0xEF13}, {0x2206, 0xBF34},
{0x2206, 0xCB02}, {0x2206, 0x2CA2}, {0x2206, 0xEF13}, {0x2206, 0x0D11},
{0x2206, 0xBF85}, {0x2206, 0x6702}, {0x2206, 0x2CA2}, {0x2206, 0xEF13},
{0x2206, 0x0D14}, {0x2206, 0xBF85}, {0x2206, 0x6402}, {0x2206, 0x2CA2},
{0x2206, 0xEF13}, {0x2206, 0x0D17}, {0x2206, 0xBF85}, {0x2206, 0x6A02},
{0x2206, 0x2CA2}, {0x2206, 0xFD04}, {0x2206, 0xF8E0}, {0x2206, 0x8B85},
{0x2206, 0xAD27}, {0x2206, 0x2DE0}, {0x2206, 0xE036}, {0x2206, 0xE1E0},
{0x2206, 0x37E1}, {0x2206, 0x8B73}, {0x2206, 0x1F10}, {0x2206, 0x9E20},
{0x2206, 0xE48B}, {0x2206, 0x73AC}, {0x2206, 0x200B}, {0x2206, 0xAC21},
{0x2206, 0x0DAC}, {0x2206, 0x250F}, {0x2206, 0xAC27}, {0x2206, 0x0EAE},
{0x2206, 0x0F02}, {0x2206, 0x84CC}, {0x2206, 0xAE0A}, {0x2206, 0x0284},
{0x2206, 0xD1AE}, {0x2206, 0x05AE}, {0x2206, 0x0302}, {0x2206, 0x84D8},
{0x2206, 0xFC04}, {0x2206, 0xEE8B}, {0x2206, 0x6800}, {0x2206, 0x0402},
{0x2206, 0x84E5}, {0x2206, 0x0285}, {0x2206, 0x2804}, {0x2206, 0x0285},
{0x2206, 0x4904}, {0x2206, 0xEE8B}, {0x2206, 0x6800}, {0x2206, 0xEE8B},
{0x2206, 0x6902}, {0x2206, 0x04F8}, {0x2206, 0xF9E0}, {0x2206, 0x8B85},
{0x2206, 0xAD26}, {0x2206, 0x38D0}, {0x2206, 0x0B02}, {0x2206, 0x2B4D},
{0x2206, 0x5882}, {0x2206, 0x7882}, {0x2206, 0x9F2D}, {0x2206, 0xE08B},
{0x2206, 0x68E1}, {0x2206, 0x8B69}, {0x2206, 0x1F10}, {0x2206, 0x9EC8},
{0x2206, 0x10E4}, {0x2206, 0x8B68}, {0x2206, 0xE0E0}, {0x2206, 0x00E1},
{0x2206, 0xE001}, {0x2206, 0xF727}, {0x2206, 0xE4E0}, {0x2206, 0x00E5},
{0x2206, 0xE001}, {0x2206, 0xE2E0}, {0x2206, 0x20E3}, {0x2206, 0xE021},
{0x2206, 0xAD30}, {0x2206, 0xF7F6}, {0x2206, 0x27E4}, {0x2206, 0xE000},
{0x2206, 0xE5E0}, {0x2206, 0x01FD}, {0x2206, 0xFC04}, {0x2206, 0xF8FA},
{0x2206, 0xEF69}, {0x2206, 0xE08B}, {0x2206, 0x86AD}, {0x2206, 0x2212},
{0x2206, 0xE0E0}, {0x2206, 0x14E1}, {0x2206, 0xE015}, {0x2206, 0xAD26},
{0x2206, 0x9CE1}, {0x2206, 0x85E0}, {0x2206, 0xBF85}, {0x2206, 0x6D02},
{0x2206, 0x2CA2}, {0x2206, 0xEF96}, {0x2206, 0xFEFC}, {0x2206, 0x04F8},
{0x2206, 0xFAEF}, {0x2206, 0x69E0}, {0x2206, 0x8B86}, {0x2206, 0xAD22},
{0x2206, 0x09E1}, {0x2206, 0x85E1}, {0x2206, 0xBF85}, {0x2206, 0x6D02},
{0x2206, 0x2CA2}, {0x2206, 0xEF96}, {0x2206, 0xFEFC}, {0x2206, 0x0464},
{0x2206, 0xE48C}, {0x2206, 0xFDE4}, {0x2206, 0x80CA}, {0x2206, 0xE480},
{0x2206, 0x66E0}, {0x2206, 0x8E70}, {0x2206, 0xE076}, {0x2205, 0xE142},
{0x2206, 0x0701}, {0x2205, 0xE140}, {0x2206, 0x0405}, {0x220F, 0x0000},
{0x221F, 0x0000}, {0x2200, 0x1340}, {0x133E, 0x000E}, {0x133F, 0x0010},
{0x13EB, 0x11BB}, {0x13E0, 0x0010}
};
/*End of ChipData30[][2]*/

rtk_uint16 ChipData31[][2]= {
/*Code of Func*/
{0x1B03, 0x0876}, {0x1200, 0x7FC4}, {0x1305, 0xC000}, {0x121E, 0x03CA},
{0x1233, 0x0352}, {0x1234, 0x0064}, {0x1237, 0x0096}, {0x1238, 0x0078},
{0x1239, 0x0084}, {0x123A, 0x0030}, {0x205F, 0x0002}, {0x2059, 0x1A00},
{0x205F, 0x0000}, {0x207F, 0x0002}, {0x2077, 0x0000}, {0x2078, 0x0000},
{0x2079, 0x0000}, {0x207A, 0x0000}, {0x207B, 0x0000}, {0x207F, 0x0000},
{0x205F, 0x0002}, {0x2053, 0x0000}, {0x2054, 0x0000}, {0x2055, 0x0000},
{0x2056, 0x0000}, {0x2057, 0x0000}, {0x205F, 0x0000}, {0x133F, 0x0030},
{0x133E, 0x000E}, {0x221F, 0x0005}, {0x2205, 0x8B86}, {0x2206, 0x800E},
{0x221F, 0x0000}, {0x133F, 0x0010}, {0x12A3, 0x2200}, {0x6107, 0xE58B},
{0x6103, 0xA970}, {0x0018, 0x0F00}, {0x0038, 0x0F00}, {0x0058, 0x0F00},
{0x0078, 0x0F00}, {0x0098, 0x0F00}, {0x133F, 0x0030}, {0x133E, 0x000E},
{0x221F, 0x0005}, {0x2205, 0x8B6E}, {0x2206, 0x0000}, {0x220F, 0x0100},
{0x2205, 0xFFF6}, {0x2206, 0x0080}, {0x2205, 0x8000}, {0x2206, 0x0280},
{0x2206, 0x2BF7}, {0x2206, 0x00E0}, {0x2206, 0xFFF7}, {0x2206, 0xA080},
{0x2206, 0x02AE}, {0x2206, 0xF602}, {0x2206, 0x0153}, {0x2206, 0x0201},
{0x2206, 0x6602}, {0x2206, 0x8044}, {0x2206, 0x0201}, {0x2206, 0x7CE0},
{0x2206, 0x8B8C}, {0x2206, 0xE18B}, {0x2206, 0x8D1E}, {0x2206, 0x01E1},
{0x2206, 0x8B8E}, {0x2206, 0x1E01}, {0x2206, 0xA000}, {0x2206, 0xE4AE},
{0x2206, 0xD8EE}, {0x2206, 0x85C0}, {0x2206, 0x00EE}, {0x2206, 0x85C1},
{0x2206, 0x00EE}, {0x2206, 0x8AFC}, {0x2206, 0x07EE}, {0x2206, 0x8AFD},
{0x2206, 0x73EE}, {0x2206, 0xFFF6}, {0x2206, 0x00EE}, {0x2206, 0xFFF7},
{0x2206, 0xFC04}, {0x2206, 0xF8E0}, {0x2206, 0x8B8E}, {0x2206, 0xAD20},
{0x2206, 0x0302}, {0x2206, 0x8050}, {0x2206, 0xFC04}, {0x2206, 0xF8F9},
{0x2206, 0xE08B}, {0x2206, 0x85AD}, {0x2206, 0x2548}, {0x2206, 0xE08A},
{0x2206, 0xE4E1}, {0x2206, 0x8AE5}, {0x2206, 0x7C00}, {0x2206, 0x009E},
{0x2206, 0x35EE}, {0x2206, 0x8AE4}, {0x2206, 0x00EE}, {0x2206, 0x8AE5},
{0x2206, 0x00E0}, {0x2206, 0x8AFC}, {0x2206, 0xE18A}, {0x2206, 0xFDE2},
{0x2206, 0x85C0}, {0x2206, 0xE385}, {0x2206, 0xC102}, {0x2206, 0x2DAC},
{0x2206, 0xAD20}, {0x2206, 0x12EE}, {0x2206, 0x8AE4}, {0x2206, 0x03EE},
{0x2206, 0x8AE5}, {0x2206, 0xB7EE}, {0x2206, 0x85C0}, {0x2206, 0x00EE},
{0x2206, 0x85C1}, {0x2206, 0x00AE}, {0x2206, 0x1115}, {0x2206, 0xE685},
{0x2206, 0xC0E7}, {0x2206, 0x85C1}, {0x2206, 0xAE08}, {0x2206, 0xEE85},
{0x2206, 0xC000}, {0x2206, 0xEE85}, {0x2206, 0xC100}, {0x2206, 0xFDFC},
{0x2206, 0x0400}, {0x2205, 0xE142}, {0x2206, 0x0701}, {0x2205, 0xE140},
{0x2206, 0x0405}, {0x220F, 0x0000}, {0x221F, 0x0000}, {0x133E, 0x000E},
{0x133F, 0x0010}, {0x13E0, 0x0010}, {0x207F, 0x0002}, {0x2073, 0x1D22},
{0x207F, 0x0000}, {0x133F, 0x0030}, {0x133E, 0x000E}, {0x2200, 0x1340},
{0x133E, 0x000E}, {0x133F, 0x0010}, };
/*End of ChipData31[][2]*/

/**************************************************************************************************/
/*                                           LOCAL_FUNCTIONS                                      */
/**************************************************************************************************/
u32 rtl8367b_setAsicRegBit(u32 reg, u32 bit, u32 value)
{
	u32 regData = 0;
	u32 retVal;
	retVal = rtl_smi_read(reg, &regData);
	if (0 != retVal)
	{
		RTL8367_DEBUG("Read fail\n");
		return 1;
	}
	
	if(value)
		regData = regData | (1 << bit);
	else
		regData = regData & (~(1 << bit));

	retVal = rtl_smi_write(reg, regData);
	if (0 != retVal)
	{
		RTL8367_DEBUG("Write fail\n");
		return 1;
	}

	return 0;
}

u32 rtl8367b_setAsicRegBits(u32 reg, u32 bits, u32 value)
{
	u32 regData = 0;
	u32 retVal;
	u32 bitsShift;
	u32 valueShifted;

	bitsShift = 0;
	while(!(bits & (1 << bitsShift)))
	{
		bitsShift++;
	}
	valueShifted = value << bitsShift;


	retVal = rtl_smi_read(reg, &regData);
	if (0 != retVal)
	{
		RTL8367_DEBUG("Read fail\n");
		return 1;
	}

	regData = regData & (~bits);
	regData = regData | (valueShifted & bits);

	retVal = rtl_smi_write(reg, regData);
	if (0 != retVal)
	{
		RTL8367_DEBUG("Read fail\n");
		return 1;
	}

	return 0;
}

/* Function Name:
 *      rtl8367b_setAsicReg
 * Description:
 *      Set content of asic register
 * Input:
 *      reg 	- register's address
 *      value 	- Value setting to register
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK 		- Success
 *      RT_ERR_SMI  	- SMI access error
 * Note:
 *      The value will be set to ASIC mapping address only and it is always return RT_ERR_OK while setting un-mapping address registers
 */
ret_t rtl8367b_setAsicReg(rtk_uint32 reg, rtk_uint32 value)
{
	ret_t retVal;

	retVal = rtl_smi_write(reg, value);
	if(retVal != RT_ERR_OK)
		return RT_ERR_SMI;
	
	return RT_ERR_OK;
}

/* Function Name:
 *      rtl8367b_getAsicReg
 * Description:
 *      Get content of asic register
 * Input:
 *      reg 	- register's address
 *      value 	- Value setting to register
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK 		- Success
 *      RT_ERR_SMI  	- SMI access error
 * Note:
 *      Value 0x0000 will be returned for ASIC un-mapping address
 */
ret_t rtl8367b_getAsicReg(rtk_uint32 reg, rtk_uint32 *pValue)
{
	rtk_uint32 regData;
	ret_t retVal;

	retVal = rtl_smi_read(reg, &regData);
	if(retVal != RT_ERR_OK)
		return RT_ERR_SMI;
	
	*pValue = regData;
	return RT_ERR_OK;
}

/* Function Name:
 *      rtl8367b_setAsicPHYReg
 * Description:
 *      Set PHY registers
 * Input:
 *      phyNo 	- Physical port number (0~4)
 *      phyAddr - PHY address (0~31)
 *      phyData - Writing data
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK 				- Success
 *      RT_ERR_SMI  			- SMI access error
 *      RT_ERR_PHY_REG_ID  		- invalid PHY address
 *      RT_ERR_PHY_ID  			- invalid PHY no
 *      RT_ERR_BUSYWAIT_TIMEOUT - PHY access busy
 * Note:
 *      None
 */
ret_t rtl8367b_setAsicPHYReg( rtk_uint32 phyNo, rtk_uint32 phyAddr, rtk_uint32 value)
{
	rtk_uint32 regAddr;

    if(phyNo > RTL8367B_PHY_INTERNALNOMAX)
        return RT_ERR_PORT_ID;

    if(phyAddr > RTL8367B_PHY_REGNOMAX)
        return RT_ERR_PHY_REG_ID;

    regAddr = 0x2000 + (phyNo << 5) + phyAddr;

    return rtl8367b_setAsicReg(regAddr, value);
}

/* Function Name:
 *      rtl8367b_getAsicPHYReg
 * Description:
 *      Get PHY registers
 * Input:
 *      phyNo 	- Physical port number (0~4)
 *      phyAddr - PHY address (0~31)
 *      pRegData - Writing data
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK 				- Success
 *      RT_ERR_SMI  			- SMI access error
 *      RT_ERR_PHY_REG_ID  		- invalid PHY address
 *      RT_ERR_PHY_ID  			- invalid PHY no
 *      RT_ERR_BUSYWAIT_TIMEOUT - PHY access busy
 * Note:
 *      None
 */
ret_t rtl8367b_getAsicPHYReg( rtk_uint32 phyNo, rtk_uint32 phyAddr, rtk_uint32 *value)
{
	rtk_uint32 regAddr;

    if(phyNo > RTL8367B_PHY_INTERNALNOMAX)
        return RT_ERR_PORT_ID;

    if(phyAddr > RTL8367B_PHY_REGNOMAX)
        return RT_ERR_PHY_REG_ID;

    regAddr = 0x2000 + (phyNo << 5) + phyAddr;

    return rtl8367b_getAsicReg(regAddr, value);
}

/* Function Name:
 *      rtk_port_phyTestModeAll_set
 * Description:
 *      Set PHY in test mode.
 * Input:
 *      port - port id.
 *      mode - PHY test mode 0:normal 1:test mode 1 2:test mode 2 3: test mode 3 4:test mode 4 5~7:reserved
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK              	- OK
 *      RT_ERR_FAILED          	- Failed
 *      RT_ERR_SMI             	- SMI access error
 *      RT_ERR_PORT_ID 			- Invalid port number.
 *      RT_ERR_BUSYWAIT_TIMEOUT - PHY access busy
 *      RT_ERR_NOT_ALLOWED      - The Setting is not allowed, caused by set more than 1 port in Test mode.
 * Note:
 *      Set PHY in test mode and only one PHY can be in test mode at the same time.
 *      It means API will return FAILED if other PHY is in test mode.
 *      This API only provide test mode 1 & 4 setup, and if users want other test modes,
 *      please contact realtek FAE.
 */
rtk_api_ret_t rtk_port_phyTestModeAll_set(rtk_port_t port, rtk_port_phy_test_mode_t mode)
{
    rtk_uint32          data, i, index, phy, reg;
    rtk_api_ret_t       retVal;
    CONST_T rtk_uint16 ParaTM_1[][2] = { {0x205F,0x0002}, {0x2053,0xAA00}, {0x2054,0xAA00}, {0x2055,0xAA00},
                                         {0x2056,0xAA00}, {0x2057,0xAA00}, {0x205F,0x0002} };

    if (port > RTK_PHY_ID_MAX)
        return RT_ERR_PORT_ID;

    if(mode >= PHY_TEST_MODE_END)
        return RT_ERR_INPUT;

    if (PHY_TEST_MODE_NORMAL != mode)
    {
        /* Other port should be Normal mode */
        for(i = 0; i <= RTK_PHY_ID_MAX; i++)
        {
            if(i != port)
            {
                if ((retVal = rtl8367b_setAsicPHYReg(i, 31, 0)) != RT_ERR_OK)
                    return retVal;

                if ((retVal = rtl8367b_getAsicPHYReg(i, 9, &data)) != RT_ERR_OK)
                    return retVal;

                if((data & 0xE000) != 0)
                    return RT_ERR_NOT_ALLOWED;
            }
        }
    }

    if (PHY_TEST_MODE_1 == mode)
    {
        for (index = 0; index < (sizeof(ParaTM_1) / ((sizeof(rtk_uint16))*2)); index++)
        {
            phy = (ParaTM_1[index][0] - 0x2000) / 0x0020;
            reg = (ParaTM_1[index][0] - 0x2000) % 0x0020;
            if ((retVal = rtl8367b_setAsicPHYReg(phy, reg, ParaTM_1[index][1])) != RT_ERR_OK)
                return retVal;
        }
    }

    if ((retVal = rtl8367b_setAsicPHYReg(port, 31, 0)) != RT_ERR_OK)
        return retVal;

    if ((retVal = rtl8367b_getAsicPHYReg(port, 9, &data)) != RT_ERR_OK)
        return retVal;

    data &= ~0xE000;
    data |= (mode << 13);
    if ((retVal = rtl8367b_setAsicPHYReg(port, 9, data)) != RT_ERR_OK)
        return retVal;

    if (PHY_TEST_MODE_3 == mode)
    {
        if ((retVal = rtl8367b_setAsicPHYReg(port, 31, 2)) != RT_ERR_OK)
            return retVal;

        if ((retVal = rtl8367b_setAsicPHYReg(port, 1, 0x065A)) != RT_ERR_OK)
            return retVal;
    }

    return RT_ERR_OK;
}

/* Function Name:
 *      rtk_port_phyTestModeAll_get
 * Description:
 *      Get PHY in which test mode.
 * Input:
 *      port - Port id.
 * Output:
 *      mode - PHY test mode 0:normal 1:test mode 1 2:test mode 2 3: test mode 3 4:test mode 4 5~7:reserved
 * Return:
 *      RT_ERR_OK              	- OK
 *      RT_ERR_FAILED          	- Failed
 *      RT_ERR_SMI             	- SMI access error
 *      RT_ERR_PORT_ID 			- Invalid port number.
 *      RT_ERR_INPUT 			- Invalid input parameters.
 *      RT_ERR_BUSYWAIT_TIMEOUT - PHY access busy
 * Note:
 *      Get test mode of PHY from register setting 9.15 to 9.13.
 */
rtk_api_ret_t rtk_port_phyTestModeAll_get(rtk_port_t port, rtk_port_phy_test_mode_t *pMode)
{
    rtk_uint32      data;
    rtk_api_ret_t   retVal;

    if (port > RTK_PHY_ID_MAX)
        return RT_ERR_PORT_ID;

    if ((retVal = rtl8367b_setAsicPHYReg(port, 31, 0)) != RT_ERR_OK)
        return retVal;

    if ((retVal = rtl8367b_getAsicPHYReg(port, 9, &data)) != RT_ERR_OK)
        return retVal;

    *pMode = (data & 0xE000) >> 13;

    return RT_ERR_OK;
}

static rtk_api_ret_t _rtk_switch_init_setreg(rtk_uint32 reg, rtk_uint32 data)
{
    rtk_api_ret_t   retVal;

    if((retVal = rtl8367b_setAsicReg(reg, data) != RT_ERR_OK))
            return retVal;

    return RT_ERR_OK;
}

void regDebug(int reg)
{
	int data;
	rw_rf_reg(0, reg, &data);
	printf("rf reg <%d> = 0x%x\n", reg, data);
}

/**************************************************************************************************/
/*                                           PUBLIC_FUNCTIONS                                     */
/**************************************************************************************************/
/* Port from Realtek */
u32 rtl_smi_write(u32 mAddrs, u32 rData)
{
	//RTL8367_DEBUG("About write 0x%x to 0x%x\n", rData, mAddrs);
	/* Write Start command to register 29 */
	mii_mgr_write(MDC_MDIO_DUMMY_ID, MDC_MDIO_START_REG, MDC_MDIO_START_OP);

	/* Write address control code to register 31 */
	mii_mgr_write(MDC_MDIO_DUMMY_ID, MDC_MDIO_CTRL0_REG, MDC_MDIO_ADDR_OP);

	/* Write Start command to register 29 */
	mii_mgr_write(MDC_MDIO_DUMMY_ID, MDC_MDIO_START_REG, MDC_MDIO_START_OP);
	
	/* Write address to register 23 */
	mii_mgr_write(MDC_MDIO_DUMMY_ID, MDC_MDIO_ADDRESS_REG, mAddrs);

	/* Write Start command to register 29 */
	mii_mgr_write(MDC_MDIO_DUMMY_ID, MDC_MDIO_START_REG, MDC_MDIO_START_OP);

	/* Write data to register 24 */
	mii_mgr_write(MDC_MDIO_DUMMY_ID, MDC_MDIO_DATA_WRITE_REG, rData);

	/* Write Start command to register 29 */
	mii_mgr_write(MDC_MDIO_DUMMY_ID, MDC_MDIO_START_REG, MDC_MDIO_START_OP);

	/* Write Start control code to register 21 */
	mii_mgr_write(MDC_MDIO_DUMMY_ID, MDC_MDIO_CTRL1_REG, MDC_MDIO_WRITE_OP);

	return 0;
}

u32 rtl_smi_read(u32 mAddrs, u32* rData)
{
	//RTL8367_DEBUG("Try to read at 0x%x\n", mAddrs);
	/* Write Start command to register 29 */
	mii_mgr_write(MDC_MDIO_DUMMY_ID, MDC_MDIO_START_REG, MDC_MDIO_START_OP);

	/* Write address control code to register 31 */
	mii_mgr_write(MDC_MDIO_DUMMY_ID, MDC_MDIO_CTRL0_REG, MDC_MDIO_ADDR_OP);


	/* Write Start command to register 29 */
	mii_mgr_write(MDC_MDIO_DUMMY_ID, MDC_MDIO_START_REG, MDC_MDIO_START_OP);

	/* Write address to register 23 */
	mii_mgr_write(MDC_MDIO_DUMMY_ID, MDC_MDIO_ADDRESS_REG, mAddrs);

#if 0
{
	u32 data;
	mii_mgr_read(MDC_MDIO_DUMMY_ID, MDC_MDIO_ADDRESS_REG, &data);
	printf("mii_mgr_read write=%x, read=%x\r\n", mAddrs, data);
}
#endif

	/* Write Start command to register 29 */
	mii_mgr_write(MDC_MDIO_DUMMY_ID, MDC_MDIO_START_REG, MDC_MDIO_START_OP);

	/* Write read control code to register 21 */
	mii_mgr_write(MDC_MDIO_DUMMY_ID, MDC_MDIO_CTRL1_REG, MDC_MDIO_READ_OP);

	/* Write Start command to register 29 */
	mii_mgr_write(MDC_MDIO_DUMMY_ID, MDC_MDIO_START_REG, MDC_MDIO_START_OP);

	/* Read data from register 25 */
	if (1 != mii_mgr_read(MDC_MDIO_DUMMY_ID, MDC_MDIO_DATA_READ_REG, rData))
	{
		RTL8367_DEBUG("mii_mgr_read error\n");
		return 1;
	}
	//RTL8367_DEBUG("Reg 0x%x with data 0x%x\n", mAddrs, *rData);
	return 0;
}

void rt_rtl8367_phy_status(void)
{
#if 1
#else
	u32 regData = 0;
	int count = 0;
	rtl_smi_read(RTL8367_EXTINTF_CTRL_REG, &regData);
	printf("RTL8367_EXTINTF_CTRL_REG is 0x%x\n", regData);

	for (count = 0; count <= 7; count++)
	{
		regData = 0;
		rtl_smi_read(RTL8367_PORT0_STATUS_REG + count, &regData);
		printf("RTL8367_PORT_STATUS_REG(%d) is 0x%x\n", count, regData);
	}
#endif
	
}
/* 
 * According to <RTL8367RB_Switch_ProgrammingGuide> 4.14 Force External Interface 
 * For RTL8367RB, MAC6 <---> RG1(Port1) and MAC7 <---> RG2(Port2)
 */
void rt_rtl8367_enableRgmii(void)
{
#if 1
	RTL8367_DEBUG("Begin\n");
	/* 
	 * 1. rtl8367b_setAsicPortExtMode
	 * (EXT_PORT_1, MODE_EXT_RGMII)
	 * (EXT_PORT_2, MODE_EXT_RGMII)
	 */
	rtl8367b_setAsicRegBit(RTL8367B_REG_BYPASS_LINE_RATE, EXT_PORT_1, 0);
	rtl8367b_setAsicRegBits(RTL8367B_REG_DIGITAL_INTERFACE_SELECT, 0xF << (EXT_PORT_1 * RTL8367B_SELECT_GMII_1_OFFSET), MODE_EXT_RGMII);

	/*rtl8367b_setAsicRegBit(RTL8367B_REG_BYPASS_LINE_RATE, EXT_PORT_2, 0);
	rtl8367b_setAsicRegBits(RTL8367B_REG_DIGITAL_INTERFACE_SELECT_1, 0xF, MODE_EXT_RGMII);*/


	/* 2. rtl8367b_getAsicPortForceLinkExt */
	/* 3. rtl8367b_setAsicPortForceLinkExt */
	{
		u32 reg_data;
		//rtl8367b_port_ability_t *pExtPort1 = (u16*)&reg_data;

		rtl_smi_read(RTL8367B_REG_DIGITAL_INTERFACE1_FORCE, &reg_data);

		reg_data &= ~0x10f7;
		reg_data |= ((1<<12) | (2 << 0) | (1 << 2) | (7 << 4));
		
		/*pExtPort1->forcemode = 1;
		pExtPort1->speed = 2;
		pExtPort1->duplex = 1;
		pExtPort1->link = 1;
		pExtPort1->nway = 0;
		pExtPort1->txpause = 1;
		pExtPort1->rxpause = 1;*/

		rtl_smi_write(RTL8367B_REG_DIGITAL_INTERFACE1_FORCE, reg_data);
	}
	#if 0
	{
		u32 reg_data;
		//rtl8367b_port_ability_t *pExtPort1 = (u16*)&reg_data;

		rtl_smi_read(RTL8367B_REG_DIGITAL_INTERFACE2_FORCE, &reg_data);

		reg_data &= ~0x10f7;
		reg_data |= ((1<<12) | (2 << 0) | (1 << 2) | (7 << 4));
		
		/*pExtPort1->forcemode = 1;
		pExtPort1->speed = 2;
		pExtPort1->duplex = 1;
		pExtPort1->link = 1;
		pExtPort1->nway = 0;
		pExtPort1->txpause = 1;
		pExtPort1->rxpause = 1;*/
		
		rtl_smi_write(RTL8367B_REG_DIGITAL_INTERFACE2_FORCE, reg_data);
	}
	#endif
	
#else
	u32 regData = 0;
	regData = (0x1 << 0) | (0x1 << 4);
	rtl_smi_write(RTL8367_EXTINTF_CTRL_REG, regData);
	printf("%s, %d, 0x%x\n", __FUNCTION__, __LINE__, regData);
	rtl_smi_read(RTL8367_EXTINTF_CTRL_REG, &regData);
	printf("%s, %d, 0x%x\n", __FUNCTION__, __LINE__, regData);
#endif
}

/* Function Name:
 *      rtk_switch_init
 * Description:
 *      Set chip to default configuration enviroment
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 *      RT_ERR_SMI          - SMI access error
 * Note:
 *      The API can set chip registers to default configuration for different release chip model.
 */
rtk_api_ret_t rtk_switch_init(void)
{
    rtk_uint16      i;
    rtk_uint32      data;
    rtk_api_ret_t   retVal;
    rtk_uint32      phy;

    if((retVal = rtl8367b_setAsicReg(0x13C2, 0x0249)) != RT_ERR_OK)
        return retVal;

    if((retVal = rtl8367b_getAsicReg(0x1301, &data)) != RT_ERR_OK)
        return retVal;

    if(data & 0xF000)
    {
        init_para = ChipData31;
        init_size = (sizeof(ChipData31) / ((sizeof(rtk_uint16))*2));
    }
    else
    {
        init_para = ChipData30;
        init_size = (sizeof(ChipData30) / ((sizeof(rtk_uint16))*2));
    }

    if(init_para == NULL)
        return RT_ERR_CHIP_NOT_SUPPORTED;

    /* Analog parameter update. ID:0001 */
    for(phy = 0; phy <= RTK_PHY_ID_MAX; phy++)
    {
        if((retVal = rtl8367b_setAsicPHYReg(phy, 31, 0x7)) != RT_ERR_OK)
            return retVal;

        if((retVal = rtl8367b_setAsicPHYReg(phy, 30, 0x2c)) != RT_ERR_OK)
            return retVal;

        if((retVal = rtl8367b_setAsicPHYReg(phy, 25, 0x0504)) != RT_ERR_OK)
            return retVal;

        if((retVal = rtl8367b_setAsicPHYReg(phy, 31, 0x0)) != RT_ERR_OK)
            return retVal;
    }

    for(i = 0; i < init_size; i++)
    {
        if((retVal = _rtk_switch_init_setreg((rtk_uint32)init_para[i][0], (rtk_uint32)init_para[i][1])) != RT_ERR_OK)
            return retVal;
    }

    /* Analog parameter update. ID:0002 */
    if((retVal = rtl8367b_setAsicPHYReg(1, 31, 0x2)) != RT_ERR_OK)
        return retVal;

    if((retVal = rtl8367b_getAsicPHYReg(1, 17, &data)) != RT_ERR_OK)
        return retVal;

    data |= 0x01E0;

    if((retVal = rtl8367b_setAsicPHYReg(1, 17, data)) != RT_ERR_OK)
        return retVal;

    if((retVal = rtl8367b_setAsicPHYReg(1, 31, 0x0)) != RT_ERR_OK)
        return retVal;


    if((retVal = rtl8367b_setAsicRegBit(0x18e0, 0, 0)) != RT_ERR_OK)
        return retVal;

    if((retVal = rtl8367b_setAsicReg(0x1303, 0x0778)) != RT_ERR_OK)
        return retVal;
    if((retVal = rtl8367b_setAsicReg(0x1304, 0x7777)) != RT_ERR_OK)
        return retVal;
    if((retVal = rtl8367b_setAsicReg(0x13E2, 0x01FE)) != RT_ERR_OK)
        return retVal;

    return RT_ERR_OK;
}

void rt_rtl8367_initVlan()
{
	u32 index = 0;
	u32 page_index = 0;
	/* clean 32 VLAN member configuration */
	for (index = 0; index <= RTL8367B_CVIDXMAX; index++)
	{
		for (page_index = 0; page_index < 4; page_index++)
		{
			rtl_smi_write(RTL8367B_VLAN_MEMBER_CONFIGURATION_BASE + (index * 4) + page_index, 0x0);
		}
	}

	/* Set a default VLAN with vid 1 to 4K table for all ports */
	/* 1. Prepare Data */
	rtl_smi_write(RTL8367B_TABLE_ACCESS_WRDATA_BASE, 0xffff);
	rtl_smi_write(RTL8367B_TABLE_ACCESS_WRDATA_BASE + 1, 0x0);
	/* 2. Write Address (VLAN_ID) */
	rtl_smi_write(RTL8367B_TABLE_ACCESS_ADDR_REG, 0x1);/* vid=1 */
	/* 3. Write Command */
	rtl_smi_write(RTL8367B_TABLE_ACCESS_CTRL_REG, (1 << 3) | (3 << 0));

	/* Also set the default VLAN to 32 member configuration index 0 */
	rtl_smi_write(RTL8367B_VLAN_MEMBER_CONFIGURATION_BASE, 0xff);
	rtl_smi_write(RTL8367B_VLAN_MEMBER_CONFIGURATION_BASE + 1, 0x0);
	rtl_smi_write(RTL8367B_VLAN_MEMBER_CONFIGURATION_BASE + 2, 0x0);
	rtl_smi_write(RTL8367B_VLAN_MEMBER_CONFIGURATION_BASE + 3, 0x1);

	/* Set all ports PVID to default VLAN and tag-mode to original */
	/* 1. Port base vid */
	rtl_smi_write(RTL8367B_VLAN_PVID_CTRL_BASE, 0x0);
	rtl_smi_write(RTL8367B_VLAN_PVID_CTRL_BASE + 1, 0x0);
	rtl_smi_write(RTL8367B_VLAN_PVID_CTRL_BASE + 2, 0x0);
	rtl_smi_write(RTL8367B_VLAN_PVID_CTRL_BASE + 3, 0x0);
	rtl_smi_write(RTL8367B_VLAN_PORTBASED_PRIORITY_BASE, 0x0);
	rtl_smi_write(RTL8367B_VLAN_PORTBASED_PRIORITY_BASE + 1, 0x0);
	/* 2. Egress Tag Mode */
	for (index = 0; index < 8; index++)
	{
		rtl8367b_setAsicRegBits(RTL8367B_PORT_MISC_CFG_BASE + (index << 5), 0x30, 0x0);
	}

	/* Enable VLAN */
	rtl_smi_write(RTL8367B_REG_VLAN_CTRL, 0x1);

	
}

void disableEthForward()/* add for BUG[58884]  */
{
	u32 index = 0;
	u32 portIsolationCtrlReg = 0x08a2;
	u32 portMatrixCtrlReg = 0x2004;

#if defined(TP_MODEL_C2V1) || defined(TP_MODEL_C5V1)
	for (index = 0; index < 8; index++)
	{
		rtl_smi_write(portIsolationCtrlReg + index, 0x0);
	}
#endif
	
#if defined(TP_MODEL_C20iV1) || defined(TP_MODEL_C20V1) || defined(TP_MODEL_C50V1)
	for (index = 0; index < 8; index++)
	{
		*(unsigned long *)(0xB0110000 + portMatrixCtrlReg + index * 256) = 0x0;
	}
#endif

	RTL8367_DEBUG("disable switch forward...\n");

}

#define RTL8367B_MIB_PORT_OFFSET (0x7C)

void rt_rtl8367_stat_port_save(u32 port)
{
	
	/* address offset to MIBs counter */
	const u16 mibLength[RTL8367B_MIBS_NUMBER]= {
		4,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
		4,2,2,2,2,2,2,2,2,
		4,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
		2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2};

	u32 mibAddr;
	u32 mibOff=0;
	u32 index = 0;
	u32 regData = 0;
	u32 mibCounterIn = 0;
	u32 mibCounterOut = 0;
	u32 regAddr = 0;

	if (port > 7)
	{
		return;
	}

	/* ifInOctets */

	index = 0;
	mibOff = RTL8367B_MIB_PORT_OFFSET * port;

	while(index < ifInOctets)
	{
		mibOff += mibLength[index];
		index++;
	}
	/*RTL8367_DEBUG("mibOff is 0x%x\n", mibOff);*/
	
	mibAddr = mibOff;

	/*RTL8367_DEBUG("Write 0x%x to 0x%x\n", (mibAddr >> 2), RTL8367B_REG_MIB_ADDRESS);*/

	rtl_smi_write(RTL8367B_REG_MIB_ADDRESS, (mibAddr >> 2));

	 /* polling busy flag */
    index = 100;
    while (index > 0)
    {
        /*read MIB control register*/
        rtl_smi_read(RTL8367B_MIB_CTRL_REG,&regData);
    
        if ((regData & 0x1) == 0)
        {
            break;
        }
    
        index--;
    }
	if (regData & 0x1)
	{
		RTL8367_DEBUG("MIB BUSYWAIT_TIMEOUT\n");
		return ;
	}
	if (regData & 0x2)
	{
		RTL8367_DEBUG("MIB STAT_CNTR_FAIL\n");
		return ;
	}

	
#if 0
	index = mibLength[ifInOctets];
	if(4 == index)
		regAddr = RTL8367B_REG_MIB_COUNTER0 + 3;
	else
		regAddr = RTL8367B_REG_MIB_COUNTER0 + ((mibOff + 1) % 4);

	regData = 0;
	mibCounterIn = 0;
	while(index)
	{
		rtl_smi_read(regAddr, &regData);
		RTL8367_DEBUG("Read from 0x%x is 0x%x \n", regAddr, regData);
		mibCounterIn = (mibCounterIn << 16) | (regData & 0xFFFF);
		/*RTL8367_DEBUG("mibCounterIn 0x%x \n", mibCounterIn);*/

		regAddr--;
		index--;
	}
#else
	mibCounterIn = 0;

	rtl_smi_read(RTL8367B_REG_MIB_COUNTER0 + 1, &regData);
	mibCounterIn = (regData & 0xFFFF);

	rtl_smi_read(RTL8367B_REG_MIB_COUNTER0, &regData);
	mibCounterIn = (mibCounterIn << 16) | (regData & 0xFFFF);

#endif
	/* ifOutOctets */

	index = 0;
	mibOff = RTL8367B_MIB_PORT_OFFSET * port;

	while(index < ifOutOctets)/*ifInOctets*/
	{
		mibOff += mibLength[index];
		index++;
	}		
	/*RTL8367_DEBUG("mibOff is 0x%x\n", mibOff);*/
	
	mibAddr = mibOff;

	/*RTL8367_DEBUG("Write 0x%x to 0x%x\n", (mibAddr >> 2), RTL8367B_REG_MIB_ADDRESS);*/

	rtl_smi_write(RTL8367B_REG_MIB_ADDRESS, (mibAddr >> 2));

	 /* polling busy flag */
    index = 100;
    while (index > 0)
    {
        /*read MIB control register*/
        rtl_smi_read(RTL8367B_MIB_CTRL_REG, &regData);
    
        if ((regData & 0x1) == 0)
        {
            break;
        }
    
        index--;
    }
	if (regData & 0x1)
	{
		RTL8367_DEBUG("MIB BUSYWAIT_TIMEOUT\n");
		return ;
	}
	if (regData & 0x2)
	{
		RTL8367_DEBUG("MIB STAT_CNTR_FAIL\n");
		return ;
	}

	
#if 0
	index = mibLength[ifOutOctets];
	if(4 == index)
		regAddr = RTL8367B_REG_MIB_COUNTER0 + 3;
	else
		regAddr = RTL8367B_REG_MIB_COUNTER0 + ((mibOff + 1) % 4);

	regData = 0;
	mibCounterOut = 0;
	while(index)
	{
		rtl_smi_read(regAddr, &regData);
		RTL8367_DEBUG("Read from 0x%x is 0x%x \n", regAddr, regData);

		mibCounterOut = (mibCounterOut << 16) | (regData & 0xFFFF);
		/*RTL8367_DEBUG("mibCounterOut 0x%x \n", mibCounterOut);*/

		regAddr--;
		index--;
	}
#else
		mibCounterOut = 0;
	
		rtl_smi_read(RTL8367B_REG_MIB_COUNTER0 + 1, &regData);
		mibCounterOut = (regData & 0xFFFF);
	
		rtl_smi_read(RTL8367B_REG_MIB_COUNTER0, &regData);
		mibCounterOut = (mibCounterOut << 16) | (regData & 0xFFFF);
	
#endif

	printf("Port %02d\t", port);
	printf("IN:0x%08x\tOUT:0x%08x\n", mibCounterIn, mibCounterOut);
	
}
void rt_rtl8367_stat(u32 port)
{
	u32 index;
	printf("==============================\n");
	printf("Port Stat:\n");
	if (port <= 7)
	{
		rt_rtl8367_stat_port_save(port);
		printf("==============================\n");
		return;
	}

	for (index = 0; index <= 7; index++)
	{
		rt_rtl8367_stat_port_save(index);
	}
	printf("==============================\n");
	return;

	
}

void rt_rtl8367_init()
{
	RTL8367_DEBUG("Begin\n");
	u32 data;
	u32 counter = 0;
	
#if 1
	while(1)
	{
		rtl_smi_read(0x2002, &data);
		printf("rtl_smi_read, data=%x\r\n", data);
		if (0x1c == data)
		{
			break;
		}
		else if (0x70 == data)
		{
			RTL8367_DEBUG("MT7620 SMI Init ERROR\n");
			return;
		}
		
		if (counter == 0)
		{
			printf("Wait for RTL8367RB Ready\n");
		}
		else if (counter >= 100)
		{
			/* about 10s */
			printf("\nTimeout\n");
			return;
		}
		udelay (10000 * 10);
		printf(".");
		counter++;
	};
	printf("\nRTL8367RB is ready now!\n");
	return;
#endif
	{
		int ret;
		if ((ret = rtk_switch_init()) != 0)
		{
			printf("init switch error: 0x%08x...\n", ret);
		}
	}
	rtl_smi_write(0x1307, 0xc);
	RTL8367_DEBUG("Call Func rt_rtl8367_enableRgmii()\n");
	rt_rtl8367_enableRgmii();

	/*RTL8367_DEBUG("Call Func rt_rtl8367_initVlan()\n");*/
	/*rt_rtl8367_initVlan();*/

}

int rtl8367_reg_command(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u32 swReg = 0, data = 1, status = 0;
	if (!strncmp(argv[0], "rtl8367", 8))
	{
		if (!strncmp(argv[1], "read", 5) && 3 == argc)
		{
			swReg = simple_strtoul(argv[2], NULL, 16);
			rtl_smi_read(swReg, &data);
			printf("Read phy_reg 0x%x : 0x%x\n", swReg, data);
		}
		else if (!strncmp(argv[1], "write", 6) && 4 == argc)
		{
			swReg = simple_strtoul(argv[2], NULL, 16);
			data = simple_strtoul(argv[3], NULL, 16);
			rtl_smi_write(swReg, data);
			printf("Write phy_reg 0x%x : 0x%x\n", swReg, data);
		}
		else if (!strncmp(argv[1], "set", 4) && 4 == argc)
		{
			swReg = simple_strtoul(argv[2], NULL, 16);
			data = simple_strtoul(argv[3], NULL, 16);
			status = rtk_port_phyTestModeAll_set(swReg, data);
			printf("set port %x to mode %x ------ status is %x\n", swReg, data, status);
		}
		else if (!strncmp(argv[1], "get", 4) && 3 == argc)
		{
			swReg = simple_strtoul(argv[2], NULL, 16);
			status = rtk_port_phyTestModeAll_get(swReg, &data);
			printf("port %x is at mode %x ------ status is %x\n", swReg, data, status);
		}
		else if (!strncmp(argv[1], "init", 5) && 2 == argc)
		{
			rt_rtl8367_init();
		}
		else if (!strncmp(argv[1], "stat", 5) && 2 == argc)
		{
			rt_rtl8367_stat(8);
		}
		else if (!strncmp(argv[1], "inner", 6) && 2 == argc)
		{
			#define RALINK_ETH_SW_BASE 0xB0110000
			printf("inner switch port 4: TX packet counter is 0x%08x		RX packet counter is 0x%08x\n", RALINK_REG(RALINK_ETH_SW_BASE+0x4410), RALINK_REG(RALINK_ETH_SW_BASE+0x4420));
			printf("inner switch port 5: TX packet counter is 0x%08x		RX packet counter is 0x%08x\n", RALINK_REG(RALINK_ETH_SW_BASE+0x4510), RALINK_REG(RALINK_ETH_SW_BASE+0x4520));
			printf("inner switch port 6: TX packet counter is 0x%08x		RX packet counter is 0x%08x\n", RALINK_REG(RALINK_ETH_SW_BASE+0x4610), RALINK_REG(RALINK_ETH_SW_BASE+0x4620));
		}
		else
		{
			printf ("Usage:\n%s\n", cmdtp->usage);
			return -1;
		}
	}
	else
	{
		printf ("Usage:\n%s\n", cmdtp->usage);
		return -1;
	}
	return 0;
}


U_BOOT_CMD(
	rtl8367,	4,	1, 	rtl8367_reg_command,
	"rtl8367	- rtl8367 switch command\n",
	"\nread\n   - read\n"
);


/**************************************************************************************************/
/*                                           GLOBAL_FUNCTIONS                                     */
/**************************************************************************************************/

