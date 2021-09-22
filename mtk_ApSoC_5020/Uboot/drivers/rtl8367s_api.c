
/*  Copyright(c) 2009-2013 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		rtl8367s_api.c
 * brief		
 * details	
 *
 * author		Huang Qingjia
 * version	
 * date		2017.06.14
 *
 * history 	\arg	
 */
#include "rtl8367s_api.h"
#include <command.h>

/**************************************************************************************************/
/*                                           EXTERN_PROTOTYPES                                    */
/**************************************************************************************************/

extern int mii_mgr_read(u32 phy_addr, u32 phy_register, u32 *read_data);
extern int mii_mgr_write(u32 phy_addr, u32 phy_register, u32 write_data);

/**************************************************************************************************/
/*                                           LOCAL_PROTOTYPES                                     */
/**************************************************************************************************/


#define RTL8367_DEBUG(args...) do{printf("%s(%d):", __FUNCTION__, __LINE__);printf(args);}while(0);

#define MDC_MDIO_PHY_ID     29  /* MDIO used PHY 29, not 0 !!!! */

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


#define RTL8367C_REGBITLENGTH               16
#define RTL8367C_REGDATAMAX                 0xFFFF

#define RTL8367C_PORTNO                     11
#define RTL8367C_PORTIDMAX                  (RTL8367C_PORTNO-1)


#define RTL8367C_QOS_GRANULARTY_MAX         0x7FFFF
#define RTL8367C_QOS_GRANULARTY_LSB_MASK    0xFFFF
#define RTL8367C_QOS_GRANULARTY_LSB_OFFSET  0
#define RTL8367C_QOS_GRANULARTY_MSB_MASK    0x70000
#define RTL8367C_QOS_GRANULARTY_MSB_OFFSET  16

#define RTL8367C_QOS_RATE_INPUT_MAX         (0x1FFFF * 8)
#define RTL8367C_QOS_RATE_INPUT_MAX_HSG     (0x7FFFF * 8)
#define RTL8367C_QOS_RATE_INPUT_MIN         8

#define    RTL8367C_REG_GPHY_OCP_MSB_0    0x1d15
#define    RTL8367C_CFG_CPU_OCPADR_MSB_MASK    0xFC0

#define    RTL8367C_REG_PORT0_EEECFG    0x0018
#define    RTL8367C_PORT0_EEECFG_EEE_100M_OFFSET    11
#define    RTL8367C_PORT0_EEECFG_EEE_GIGA_500M_OFFSET    10
#define    RTL8367C_PORT0_EEECFG_EEE_TX_OFFSET    9
#define    RTL8367C_PORT0_EEECFG_EEE_RX_OFFSET    8

#define    RTL8367C_REG_UTP_FIB_DET    0x13eb

#define RTL8367C_PHY_REGNOMAX		    0x1F
#define	RTL8367C_PHY_BASE   	        0x2000
#define	RTL8367C_PHY_OFFSET	            5

#define    RTL8367C_REG_PHY_AD    0x130f
#define    RTL8367C_PDNPHY_OFFSET    5

#define    RTL8367C_REG_PORT0_EGRESSBW_CTRL0    0x038c
#define    RTL8367C_PORT_EGRESSBW_LSB_BASE                        RTL8367C_REG_PORT0_EGRESSBW_CTRL0
#define    RTL8367C_PORT_EGRESSBW_LSB_REG(port)                    (RTL8367C_PORT_EGRESSBW_LSB_BASE + (port << 1))

#define    RTL8367C_REG_PORT0_EGRESSBW_CTRL1    0x038d
#define    RTL8367C_PORT_EGRESSBW_MSB_BASE                        RTL8367C_REG_PORT0_EGRESSBW_CTRL1
#define    RTL8367C_PORT_EGRESSBW_MSB_REG(port)                    (RTL8367C_PORT_EGRESSBW_MSB_BASE + (port << 1))

#define    RTL8367C_PORT6_EGRESSBW_CTRL1_MASK    0x7

#define    RTL8367C_REG_INGRESSBW_PORT0_RATE_CTRL0    0x000f
#define    RTL8367C_INGRESSBW_PORT_RATE_LSB_BASE                RTL8367C_REG_INGRESSBW_PORT0_RATE_CTRL0
#define    RTL8367C_INGRESSBW_PORT_RATE_LSB_REG(port)            (RTL8367C_INGRESSBW_PORT_RATE_LSB_BASE + (port << 5))

#define    RTL8367C_REG_MAC0_FORCE_SELECT    0x1312

#define    RTL8367C_INGRESSBW_PORT0_RATE_CTRL1_INGRESSBW_RATE16_MASK    0x7
#define    RTL8367C_PORT0_MISC_CFG_INGRESSBW_IFG_OFFSET    10
#define    RTL8367C_PORT0_MISC_CFG_INGRESSBW_FLOWCTRL_OFFSET    11

#define    RTL8367C_REG_PORT0_MISC_CFG    0x000e
#define    RTL8367C_PORT_MISC_CFG_BASE                            RTL8367C_REG_PORT0_MISC_CFG
#define    RTL8367C_PORT_MISC_CFG_REG(port)                        (RTL8367C_PORT_MISC_CFG_BASE + (port << 5))

#define    RTL8367C_REG_SCHEDULE_WFQ_CTRL    0x0300
#define    RTL8367C_SCHEDULE_WFQ_CTRL_OFFSET    0
#define    RTL8367C_SCHEDULE_WFQ_CTRL_MASK    0x1

#define    RTL8367C_REG_LUT_CFG    0x0a30
#define    RTL8367C_LUT_IPMC_LOOKUP_OP_OFFSET    3

#define    RTL8367C_REG_MIRROR_CTRL2    0x09DA
#define    RTL8367C_MIRROR_TX_ISOLATION_LEAKY_OFFSET    2

#define RTL8367C_RMAMAX                     0x2F
#define    RTL8367C_REG_RMA_CTRL00    0x0800
#define    RTL8367C_TRAP_PRIORITY_MASK    0x38

#define    RTL8367C_REG_MAX_LENGTH_LIMINT_IPG    0x1200
#define    RTL8367C_MAX_LENTH_CTRL_MASK    0x6000

#define    RTL8367C_REG_MAX_LEN_RX_TX    0x0884
#define    RTL8367C_MAX_LEN_RX_TX_MASK    0x3

#define    RTL8367C_REG_ACL_ACCESS_MODE    0x06EB
#define    RTL8367C_ACL_ACCESS_MODE_MASK    0x1

#define    RTL8367C_REG_PORT_SECURITY_CTRL    0x08c8
#define    RTL8367C_PORT_SECURIT_CTRL_REG                        RTL8367C_REG_PORT_SECURITY_CTRL
#define    RTL8367C_UNKNOWN_UNICAST_DA_BEHAVE_MASK    0xC0

#define    RTL8367C_REG_IO_MISC_FUNC    0x1d32
#define    RTL8367C_INT_EN_OFFSET    1

#define    RTL8367C_REG_DIGITAL_INTERFACE_SELECT_1    0x13c3
#define    RTL8367C_REG_DIGITAL_INTERFACE2_FORCE    0x13c4
#define    RTL8367C_REG_BYPASS_LINE_RATE    0x03f7

#define PHY_CONTROL_REG                             0

#define RTK_CHK_INIT_STATE()                                \
    do                                                      \
    {                                                       \
        if(rtk_switch_initialState_get() != INIT_COMPLETED) \
        {                                                   \
            return RT_ERR_NOT_INIT;                         \
        }                                                   \
    }while(0)

#define RTK_CHK_PORT_VALID(__port__)                            \
    do                                                          \
    {                                                           \
        if(rtk_switch_logicalPortCheck(__port__) != RT_ERR_OK)  \
        {                                                       \
            return RT_ERR_PORT_ID;                              \
        }                                                       \
    }while(0)

#define RTK_CHK_PORT_IS_UTP(__port__)                           \
    do                                                          \
    {                                                           \
        if(rtk_switch_isUtpPort(__port__) != RT_ERR_OK)         \
        {                                                       \
            return RT_ERR_PORT_ID;                              \
        }                                                       \
    }while(0)

    
#define UNDEFINE_PHY_PORT   (0xFF)
#define RTK_SWITCH_PORT_NUM (32)

#define MAXPKTLEN_CFG_ID_MAX (1)

#define RTK_SWITCH_MAX_PKTLEN (0x3FFF)

#define RTK_SCAN_ALL_LOG_PORT(__port__)     \
	for(__port__ = 0; __port__ < RTK_SWITCH_PORT_NUM; __port__++) \
		if( rtk_switch_logicalPortCheck(__port__) == RT_ERR_OK)

typedef enum init_state_e
{
    INIT_NOT_COMPLETED = 0,
    INIT_COMPLETED,
    INIT_STATE_END
} init_state_t;

typedef enum switch_chip_e
{
    CHIP_RTL8367C = 0,
    CHIP_RTL8370B,
    CHIP_RTL8364B,
    CHIP_END
}switch_chip_t;

typedef enum port_type_e
{
    UTP_PORT = 0,
    EXT_PORT,
    UNKNOWN_PORT = 0xFF,
    PORT_TYPE_END
}port_type_t;

typedef struct rtk_switch_halCtrl_s
{
    switch_chip_t   switch_type;
    rtk_uint32      l2p_port[RTK_SWITCH_PORT_NUM];
    rtk_uint32      p2l_port[RTK_SWITCH_PORT_NUM];
    port_type_t     log_port_type[RTK_SWITCH_PORT_NUM];
    rtk_uint32      ptp_port[RTK_SWITCH_PORT_NUM];
    rtk_uint32      valid_portmask;
    rtk_uint32      valid_utp_portmask;
    rtk_uint32      valid_ext_portmask;
    rtk_uint32      min_phy_port;
    rtk_uint32      max_phy_port;
    rtk_uint32      phy_portmask;
    rtk_uint32      combo_logical_port;
    rtk_uint32      hsg_logical_port;
    rtk_uint32      max_meter_id;
    rtk_uint32      max_lut_addr_num;
    rtk_uint32      max_trunk_id;

}rtk_switch_halCtrl_t;

typedef struct  rtl8367c_rma_s{

    rtk_uint16 operation;
    rtk_uint16 discard_storm_filter;
    rtk_uint16 trap_priority;
    rtk_uint16 keep_format;
    rtk_uint16 vlan_leaky;
    rtk_uint16 portiso_leaky;

}rtl8367c_rma_t;

typedef enum rtk_port_phy_reg_e
{
    PHY_REG_CONTROL             = 0,
    PHY_REG_STATUS,
    PHY_REG_IDENTIFIER_1,
    PHY_REG_IDENTIFIER_2,
    PHY_REG_AN_ADVERTISEMENT,
    PHY_REG_AN_LINKPARTNER,
    PHY_REG_1000_BASET_CONTROL  = 9,
    PHY_REG_1000_BASET_STATUS,
    PHY_REG_END                 = 32
} rtk_port_phy_reg_t;


typedef rtk_uint32  rtk_port_phy_data_t;     /* phy page  */


/**************************************************************************************************/
/*                                           VARIABLES                                            */
/**************************************************************************************************/

static init_state_t    init_state = INIT_NOT_COMPLETED;

static rtk_switch_halCtrl_t rtl8367c_hal_Ctrl =
{
    /* Switch Chip */
    CHIP_RTL8367C,

    /* Logical to Physical */
    {0, 1, 2, 3, 4, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     6, 7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },

    /* Physical to Logical */
    {UTP_PORT0, UTP_PORT1, UTP_PORT2, UTP_PORT3, UTP_PORT4, UNDEFINE_PORT, EXT_PORT0, EXT_PORT1,
     UNDEFINE_PORT, UNDEFINE_PORT, UNDEFINE_PORT, UNDEFINE_PORT, UNDEFINE_PORT, UNDEFINE_PORT, UNDEFINE_PORT, UNDEFINE_PORT,
     UNDEFINE_PORT, UNDEFINE_PORT, UNDEFINE_PORT, UNDEFINE_PORT, UNDEFINE_PORT, UNDEFINE_PORT, UNDEFINE_PORT, UNDEFINE_PORT,
     UNDEFINE_PORT, UNDEFINE_PORT, UNDEFINE_PORT, UNDEFINE_PORT, UNDEFINE_PORT, UNDEFINE_PORT, UNDEFINE_PORT, UNDEFINE_PORT},

    /* Port Type */
    {UTP_PORT, UTP_PORT, UTP_PORT, UTP_PORT, UTP_PORT, UNKNOWN_PORT, UNKNOWN_PORT, UNKNOWN_PORT,
     UNKNOWN_PORT, UNKNOWN_PORT, UNKNOWN_PORT, UNKNOWN_PORT, UNKNOWN_PORT, UNKNOWN_PORT, UNKNOWN_PORT, UNKNOWN_PORT,
     EXT_PORT, EXT_PORT, UNKNOWN_PORT, UNKNOWN_PORT, UNKNOWN_PORT, UNKNOWN_PORT, UNKNOWN_PORT, UNKNOWN_PORT,
     UNKNOWN_PORT, UNKNOWN_PORT, UNKNOWN_PORT, UNKNOWN_PORT, UNKNOWN_PORT, UNKNOWN_PORT, UNKNOWN_PORT, UNKNOWN_PORT},

    /* PTP port */
    {1, 1, 1, 1, 1, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0 },

    /* Valid port mask */
    ( (0x1 << UTP_PORT0) | (0x1 << UTP_PORT1) | (0x1 << UTP_PORT2) | (0x1 << UTP_PORT3) | (0x1 << UTP_PORT4) | (0x1 << EXT_PORT0) | (0x1 << EXT_PORT1) ),

    /* Valid UTP port mask */
    ( (0x1 << UTP_PORT0) | (0x1 << UTP_PORT1) | (0x1 << UTP_PORT2) | (0x1 << UTP_PORT3) | (0x1 << UTP_PORT4) ),

    /* Valid EXT port mask */
    ( (0x1 << EXT_PORT0) | (0x1 << EXT_PORT1) ),

    /* Minimum physical port number */
    0,

    /* Maxmum physical port number */
    7,

    /* Physical port mask */
    0xDF,

    /* Combo Logical port ID */
    4,

    /* HSG Logical port ID */
    EXT_PORT0,

    /* Max Meter ID */
    31,

    /* MAX LUT Address Number */
    2112,

    /* MAX TRUNK ID */
    1
};

static rtk_switch_halCtrl_t *halCtrl = NULL;


extern ret_t rtl8367c_setAsicReg(rtk_uint32 reg, rtk_uint32 value);
extern ret_t rtl8367c_getAsicReg(rtk_uint32 reg, rtk_uint32 *pValue);

/**************************************************************************************************/
/*                                           LOCAL_FUNCTIONS                                      */
/**************************************************************************************************/


u32 rtl_smi_write(u32 mAddrs, u32 rData)
{
    if(mAddrs > 0xFFFF)
        return RT_ERR_INPUT;

    if(rData > 0xFFFF)
        return RT_ERR_INPUT;
	
    /* Write address control code to register 31 */
    mii_mgr_write(MDC_MDIO_PHY_ID, MDC_MDIO_CTRL0_REG, MDC_MDIO_ADDR_OP);

    /* Write address to register 23 */
    mii_mgr_write(MDC_MDIO_PHY_ID, MDC_MDIO_ADDRESS_REG, mAddrs);

    /* Write data to register 24 */
    mii_mgr_write(MDC_MDIO_PHY_ID, MDC_MDIO_DATA_WRITE_REG, rData);

    /* Write data control code to register 21 */
    mii_mgr_write(MDC_MDIO_PHY_ID, MDC_MDIO_CTRL1_REG, MDC_MDIO_WRITE_OP);
	
	return 0;
}

u32 rtl_smi_read(u32 mAddrs, u32* rData)
{
    if(mAddrs > 0xFFFF)
        return RT_ERR_INPUT;

    if(rData == NULL)
        return RT_ERR_NULL_POINTER;
	
    /* Write address control code to register 31 */
    mii_mgr_write(MDC_MDIO_PHY_ID, MDC_MDIO_CTRL0_REG, MDC_MDIO_ADDR_OP);

    /* Write address to register 23 */
    mii_mgr_write(MDC_MDIO_PHY_ID, MDC_MDIO_ADDRESS_REG, mAddrs);

    /* Write read control code to register 21 */
    mii_mgr_write(MDC_MDIO_PHY_ID, MDC_MDIO_CTRL1_REG, MDC_MDIO_READ_OP);

    /* Read data from register 25 */
    mii_mgr_read(MDC_MDIO_PHY_ID, MDC_MDIO_DATA_READ_REG, rData);
	
	return 0;
}




/* Function Name:
 *      rtl8367c_setAsicRegBit
 * Description:
 *      Set a bit value of a specified register
 * Input:
 *      reg 	- register's address
 *      bit 	- bit location
 *      value 	- value to set. It can be value 0 or 1.
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK 		- Success
 *      RT_ERR_SMI  	- SMI access error
 *      RT_ERR_INPUT  	- Invalid input parameter
 * Note:
 *      Set a bit of a specified register to 1 or 0.
 */
ret_t rtl8367c_setAsicRegBit(rtk_uint32 reg, rtk_uint32 bit, rtk_uint32 value)
{
	rtk_uint32 regData;
	ret_t retVal;

	if(bit >= RTL8367C_REGBITLENGTH)
		return RT_ERR_INPUT;

	retVal = rtl_smi_read(reg, &regData);
	if(retVal != RT_ERR_OK)
		return RT_ERR_SMI;

	if(value)
		regData = regData | (1 << bit);
	else
		regData = regData & (~(1 << bit));

	retVal = rtl_smi_write(reg, regData);
	if(retVal != RT_ERR_OK)
		return RT_ERR_SMI;

	return RT_ERR_OK;
}
/* Function Name:
 *      rtl8367c_getAsicRegBit
 * Description:
 *      Get a bit value of a specified register
 * Input:
 *      reg 	- register's address
 *      bit 	- bit location
 *      value 	- value to get.
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK 		- Success
 *      RT_ERR_SMI  	- SMI access error
 *      RT_ERR_INPUT  	- Invalid input parameter
 * Note:
 *      None
 */
ret_t rtl8367c_getAsicRegBit(rtk_uint32 reg, rtk_uint32 bit, rtk_uint32 *pValue)
{
	rtk_uint32 regData;
	ret_t retVal;

	retVal = rtl_smi_read(reg, &regData);
	if(retVal != RT_ERR_OK)
		return RT_ERR_SMI;

	*pValue = (regData & (0x1 << bit)) >> bit;

	return RT_ERR_OK;
}
/* Function Name:
 *      rtl8367c_setAsicRegBits
 * Description:
 *      Set bits value of a specified register
 * Input:
 *      reg 	- register's address
 *      bits 	- bits mask for setting
 *      value 	- bits value for setting
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK 		- Success
 *      RT_ERR_SMI  	- SMI access error
 *      RT_ERR_INPUT  	- Invalid input parameter
 * Note:
 *      Set bits of a specified register to value. Both bits and value are be treated as bit-mask
 */
ret_t rtl8367c_setAsicRegBits(rtk_uint32 reg, rtk_uint32 bits, rtk_uint32 value)
{
	rtk_uint32 regData;
	ret_t retVal;
	rtk_uint32 bitsShift;
	rtk_uint32 valueShifted;

	if(bits >= (1 << RTL8367C_REGBITLENGTH) )
		return RT_ERR_INPUT;

	bitsShift = 0;
	while(!(bits & (1 << bitsShift)))
	{
		bitsShift++;
		if(bitsShift >= RTL8367C_REGBITLENGTH)
			return RT_ERR_INPUT;
	}
	valueShifted = value << bitsShift;

	if(valueShifted > RTL8367C_REGDATAMAX)
		return RT_ERR_INPUT;

	retVal = rtl_smi_read(reg, &regData);
	if(retVal != RT_ERR_OK)
		return RT_ERR_SMI;

	regData = regData & (~bits);
	regData = regData | (valueShifted & bits);

	retVal = rtl_smi_write(reg, regData);
	if(retVal != RT_ERR_OK)
		return RT_ERR_SMI;


	return RT_ERR_OK;
}

/* Function Name:
 *      rtl8367c_getAsicRegBits
 * Description:
 *      Get bits value of a specified register
 * Input:
 *      reg 	- register's address
 *      bits 	- bits mask for setting
 *      value 	- bits value for setting
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK 		- Success
 *      RT_ERR_SMI  	- SMI access error
 *      RT_ERR_INPUT  	- Invalid input parameter
 * Note:
 *      None
 */
ret_t rtl8367c_getAsicRegBits(rtk_uint32 reg, rtk_uint32 bits, rtk_uint32 *pValue)
{
	rtk_uint32 regData;
	ret_t retVal;
	rtk_uint32 bitsShift;

	if(bits>= (1<<RTL8367C_REGBITLENGTH) )
		return RT_ERR_INPUT;

	bitsShift = 0;
	while(!(bits & (1 << bitsShift)))
	{
		bitsShift++;
		if(bitsShift >= RTL8367C_REGBITLENGTH)
			return RT_ERR_INPUT;
	}

	retVal = rtl_smi_read(reg, &regData);
	if(retVal != RT_ERR_OK) return RT_ERR_SMI;

	*pValue = (regData & bits) >> bitsShift;

	return RT_ERR_OK;
}

/* Function Name:
 *      rtl8367c_setAsicReg
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
ret_t rtl8367c_setAsicReg(rtk_uint32 reg, rtk_uint32 value)
{
	ret_t retVal;

	retVal = rtl_smi_write(reg, value);
	if(retVal != RT_ERR_OK)
		return RT_ERR_SMI;

	return RT_ERR_OK;
}
/* Function Name:
 *      rtl8367c_getAsicReg
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
ret_t rtl8367c_getAsicReg(rtk_uint32 reg, rtk_uint32 *pValue)
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
 *      rtl8367c_setAsicPHYOCPReg
 * Description:
 *      Set PHY OCP registers
 * Input:
 *      phyNo 	- Physical port number (0~7)
 *      ocpAddr - OCP address
 *      ocpData - Writing data
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
ret_t rtl8367c_setAsicPHYOCPReg(rtk_uint32 phyNo, rtk_uint32 ocpAddr, rtk_uint32 ocpData )
{
    ret_t retVal;
	rtk_uint32 regAddr;
    rtk_uint32 ocpAddrPrefix, ocpAddr9_6, ocpAddr5_1;

    /* OCP prefix */
    ocpAddrPrefix = ((ocpAddr & 0xFC00) >> 10);
    if((retVal = rtl8367c_setAsicRegBits(RTL8367C_REG_GPHY_OCP_MSB_0, RTL8367C_CFG_CPU_OCPADR_MSB_MASK, ocpAddrPrefix)) != RT_ERR_OK)
        return retVal;

    /*prepare access address*/
    ocpAddr9_6 = ((ocpAddr >> 6) & 0x000F);
    ocpAddr5_1 = ((ocpAddr >> 1) & 0x001F);
    regAddr = RTL8367C_PHY_BASE | (ocpAddr9_6 << 8) | (phyNo << RTL8367C_PHY_OFFSET) | ocpAddr5_1;
    if((retVal = rtl8367c_setAsicReg(regAddr, ocpData)) != RT_ERR_OK)
        return retVal;

    return RT_ERR_OK;
}

/* Function Name:
 *      rtl8367c_getAsicPHYOCPReg
 * Description:
 *      Get PHY OCP registers
 * Input:
 *      phyNo 	- Physical port number (0~7)
 *      ocpAddr - PHY address
 *      pRegData - read data
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
ret_t rtl8367c_getAsicPHYOCPReg(rtk_uint32 phyNo, rtk_uint32 ocpAddr, rtk_uint32 *pRegData )
{
    ret_t retVal;
	rtk_uint32 regAddr;
    rtk_uint32 ocpAddrPrefix, ocpAddr9_6, ocpAddr5_1;

    /* OCP prefix */
    ocpAddrPrefix = ((ocpAddr & 0xFC00) >> 10);
    if((retVal = rtl8367c_setAsicRegBits(RTL8367C_REG_GPHY_OCP_MSB_0, RTL8367C_CFG_CPU_OCPADR_MSB_MASK, ocpAddrPrefix)) != RT_ERR_OK)
        return retVal;

    /*prepare access address*/
    ocpAddr9_6 = ((ocpAddr >> 6) & 0x000F);
    ocpAddr5_1 = ((ocpAddr >> 1) & 0x001F);
    regAddr = RTL8367C_PHY_BASE | (ocpAddr9_6 << 8) | (phyNo << RTL8367C_PHY_OFFSET) | ocpAddr5_1;
    if((retVal = rtl8367c_getAsicReg(regAddr, pRegData)) != RT_ERR_OK)
        return retVal;

    return RT_ERR_OK;
}


/* Function Name:
 *      rtl8367c_setAsicPHYReg
 * Description:
 *      Set PHY registers
 * Input:
 *      phyNo 	- Physical port number (0~7)
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
ret_t rtl8367c_setAsicPHYReg(rtk_uint32 phyNo, rtk_uint32 phyAddr, rtk_uint32 phyData )
{
    rtk_uint32 ocp_addr;

    if(phyAddr > RTL8367C_PHY_REGNOMAX)
        return RT_ERR_PHY_REG_ID;

    ocp_addr = 0xa400 + phyAddr*2;

    return rtl8367c_setAsicPHYOCPReg(phyNo, ocp_addr, phyData);
}
/* Function Name:
 *      rtl8367c_getAsicPHYReg
 * Description:
 *      Get PHY registers
 * Input:
 *      phyNo 	- Physical port number (0~7)
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
ret_t rtl8367c_getAsicPHYReg(rtk_uint32 phyNo, rtk_uint32 phyAddr, rtk_uint32 *pRegData )
{
	rtk_uint32 ocp_addr;

    if(phyAddr > RTL8367C_PHY_REGNOMAX)
        return RT_ERR_PHY_REG_ID;

    ocp_addr = 0xa400 + phyAddr*2;

    return rtl8367c_getAsicPHYOCPReg(phyNo, ocp_addr, pRegData);
}

/* Function Name:
 *      rtl8370_setAsicPortEnableAll
 * Description:
 *      Set ALL ports enable.
 * Input:
 *      enable - enable all ports.
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK 			- Success
 *      RT_ERR_SMI  		- SMI access error
 * Note:
 *      None
 */
ret_t rtl8367c_setAsicPortEnableAll(rtk_uint32 enable)
{
    if(enable >= 2)
        return RT_ERR_INPUT;

    return rtl8367c_setAsicRegBit(RTL8367C_REG_PHY_AD, RTL8367C_PDNPHY_OFFSET, !enable);
}

/* Function Name:
 *      rtl8367c_setAsicPortEgressRate
 * Description:
 *      Set per-port egress rate
 * Input:
 *      port 		- Physical port number (0~10)
 *      rate 		- Egress rate
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK 			- Success
 *      RT_ERR_SMI  		- SMI access error
 *      RT_ERR_PORT_ID  	- Invalid port number
 *      RT_ERR_QOS_EBW_RATE - Invalid bandwidth/rate
 * Note:
 *      None
 */
ret_t rtl8367c_setAsicPortEgressRate(rtk_uint32 port, rtk_uint32 rate)
{
    ret_t retVal;
    rtk_uint32 regAddr, regData;

    if(port > RTL8367C_PORTIDMAX)
        return RT_ERR_PORT_ID;

    if(rate > RTL8367C_QOS_GRANULARTY_MAX)
        return RT_ERR_QOS_EBW_RATE;

    regAddr = RTL8367C_PORT_EGRESSBW_LSB_REG(port);
    regData = RTL8367C_QOS_GRANULARTY_LSB_MASK & rate;

    retVal = rtl8367c_setAsicReg(regAddr, regData);

    if(retVal != RT_ERR_OK)
        return retVal;

    regAddr = RTL8367C_PORT_EGRESSBW_MSB_REG(port);
    regData = (RTL8367C_QOS_GRANULARTY_MSB_MASK & rate) >> RTL8367C_QOS_GRANULARTY_MSB_OFFSET;

    retVal = rtl8367c_setAsicRegBits(regAddr, RTL8367C_PORT6_EGRESSBW_CTRL1_MASK, regData);

	return retVal;
}

/* Function Name:
 *      rtl8367c_setAsicPortEgressRateIfg
 * Description:
 *      Set per-port egress rate calculate include/exclude IFG
 * Input:
 *      ifg 	- 1:include IFG 0:exclude IFG
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK 	- Success
 *      RT_ERR_SMI  - SMI access error
 * Note:
 *      None
 */
ret_t rtl8367c_setAsicPortEgressRateIfg(rtk_uint32 ifg)
{
    ret_t retVal;

    retVal = rtl8367c_setAsicRegBit(RTL8367C_REG_SCHEDULE_WFQ_CTRL, RTL8367C_SCHEDULE_WFQ_CTRL_OFFSET, ifg);

	return retVal;
}

/* Function Name:
 *      rtl8367c_setAsicPortIngressBandwidth
 * Description:
 *      Set per-port total ingress bandwidth
 * Input:
 *      port 		- Physical port number (0~7)
 *      bandwidth 	- The total ingress bandwidth (unit: 8Kbps), 0x1FFFF:disable
 *      preifg 		- Include preamble and IFG, 0:Exclude, 1:Include
 *      enableFC 	- Action when input rate exceeds. 0: Drop	1: Flow Control
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK 			- Success
 *      RT_ERR_SMI  		- SMI access error
 *      RT_ERR_PORT_ID  	- Invalid port number
 *      RT_ERR_OUT_OF_RANGE - input parameter out of range
 * Note:
 *      None
 */
ret_t rtl8367c_setAsicPortIngressBandwidth(rtk_uint32 port, rtk_uint32 bandwidth, rtk_uint32 preifg, rtk_uint32 enableFC)
{
	ret_t retVal;
	rtk_uint32 regData;
	rtk_uint32 regAddr;

	/* Invalid input parameter */
	if(port >= RTL8367C_PORTNO)
		return RT_ERR_PORT_ID;

	if(bandwidth > RTL8367C_QOS_GRANULARTY_MAX)
		return RT_ERR_OUT_OF_RANGE;

	regAddr = RTL8367C_INGRESSBW_PORT_RATE_LSB_REG(port);
	regData = bandwidth & RTL8367C_QOS_GRANULARTY_LSB_MASK;
	retVal = rtl8367c_setAsicReg(regAddr, regData);
	if(retVal != RT_ERR_OK)
		return retVal;

	regAddr += 1;
	regData = (bandwidth & RTL8367C_QOS_GRANULARTY_MSB_MASK) >> RTL8367C_QOS_GRANULARTY_MSB_OFFSET;
	retVal = rtl8367c_setAsicRegBits(regAddr, RTL8367C_INGRESSBW_PORT0_RATE_CTRL1_INGRESSBW_RATE16_MASK, regData);
	if(retVal != RT_ERR_OK)
		return retVal;

	regAddr = RTL8367C_PORT_MISC_CFG_REG(port);
	retVal = rtl8367c_setAsicRegBit(regAddr, RTL8367C_PORT0_MISC_CFG_INGRESSBW_IFG_OFFSET, preifg);
	if(retVal != RT_ERR_OK)
		return retVal;

	regAddr = RTL8367C_PORT_MISC_CFG_REG(port);
	retVal = rtl8367c_setAsicRegBit(regAddr, RTL8367C_PORT0_MISC_CFG_INGRESSBW_FLOWCTRL_OFFSET, enableFC);
	if(retVal != RT_ERR_OK)
		return retVal;

	return RT_ERR_OK;
}


/* Function Name:
 *      rtl8367c_setAsicLutIpLookupMethod
 * Description:
 *      Set Lut IP lookup hash with DIP or {DIP,SIP} pair
 * Input:
 *      type - 1: When DIP can be found in IPMC_GROUP_TABLE, use DIP+SIP Hash, otherwise, use DIP+(SIP=0.0.0.0) Hash.
 *             0: When DIP can be found in IPMC_GROUP_TABLE, use DIP+(SIP=0.0.0.0) Hash, otherwise use DIP+SIP Hash.
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK 	- Success
 *      RT_ERR_SMI  - SMI access error
 * Note:
 *      None
 */
ret_t rtl8367c_setAsicLutIpLookupMethod(rtk_uint32 type)
{
	return rtl8367c_setAsicRegBit(RTL8367C_REG_LUT_CFG, RTL8367C_LUT_IPMC_LOOKUP_OP_OFFSET, type);
}

/* Function Name:
 *      rtl8367c_setAsicPortMirrorIsolationTxLeaky
 * Description:
 *      Set the mirror function of Isolation TX leaky
 * Input:
 *      enabled 	- 1: enabled, 0: disabled
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK 		- Success
 *      RT_ERR_SMI  	- SMI access error
 * Note:
 *      None
 */
ret_t rtl8367c_setAsicPortMirrorIsolationTxLeaky(rtk_uint32 enabled)
{
	return rtl8367c_setAsicRegBit(RTL8367C_REG_MIRROR_CTRL2, RTL8367C_MIRROR_TX_ISOLATION_LEAKY_OFFSET, enabled);
}

/* Function Name:
 *      rtl8367c_setAsicRma
 * Description:
 *      Set reserved multicast address for CPU trapping
 * Input:
 *      index     - reserved multicast LSB byte, 0x00~0x2F is available value
 *      pRmacfg     - type of RMA for trapping frame type setting
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK         - Success
 *      RT_ERR_SMI      - SMI access error
 *      RT_ERR_RMA_ADDR - Invalid RMA address index
 * Note:
 *      None
 */
ret_t rtl8367c_setAsicRma(rtk_uint32 index, rtl8367c_rma_t* pRmacfg)
{
    rtk_uint32 regData = 0;
    ret_t retVal;

    if(index > RTL8367C_RMAMAX)
        return RT_ERR_RMA_ADDR;

    regData |= (pRmacfg->portiso_leaky & 0x0001);
    regData |= ((pRmacfg->vlan_leaky & 0x0001) << 1);
    regData |= ((pRmacfg->keep_format & 0x0001) << 2);
    regData |= ((pRmacfg->trap_priority & 0x0007) << 3);
    regData |= ((pRmacfg->discard_storm_filter & 0x0001) << 6);
    regData |= ((pRmacfg->operation & 0x0003) << 7);

    if( (index >= 0x4 && index <= 0x7) || (index >= 0x9 && index <= 0x0C) || (0x0F == index))
        index = 0x04;
    else if((index >= 0x13 && index <= 0x17) || (0x19 == index) || (index >= 0x1B && index <= 0x1f))
        index = 0x13;
    else if(index >= 0x22 && index <= 0x2F)
        index = 0x22;

    retVal = rtl8367c_setAsicRegBits(RTL8367C_REG_RMA_CTRL00, RTL8367C_TRAP_PRIORITY_MASK, pRmacfg->trap_priority);
    if(retVal != RT_ERR_OK)
        return retVal;

    return rtl8367c_setAsicReg(RTL8367C_REG_RMA_CTRL00+index, regData);
}



/* Function Name:
 *      rtk_switch_probe
 * Description:
 *      Probe switch
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK       - Switch probed
 *      RT_ERR_FAILED   - Switch Unprobed.
 * Note:
 *
 */
rtk_api_ret_t rtk_switch_probe(switch_chip_t *pSwitchChip)
{
    rtk_uint32 retVal;
    rtk_uint32 data;

    if((retVal = rtl8367c_setAsicReg(0x13C2, 0x0249)) != RT_ERR_OK)
        return retVal;

    if((retVal = rtl8367c_getAsicReg(0x1300, &data)) != RT_ERR_OK)
        return retVal;

    if((retVal = rtl8367c_setAsicReg(0x13C2, 0x0000)) != RT_ERR_OK)
        return retVal;

    switch (data)
    {
        case 0x0276:
        case 0x0597:
        case 0x6367:
            *pSwitchChip = CHIP_RTL8367C;
            halCtrl = &rtl8367c_hal_Ctrl;
            break;
        default:
		printf("rtk_switch_probe error: chipdata = %x\r\n", data);
            return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
}

/* Function Name:
 *      rtk_switch_initialState_set
 * Description:
 *      Set initial status
 * Input:
 *      state   - Initial state;
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK       - Initialized
 *      RT_ERR_FAILED   - Uninitialized
 * Note:
 *
 */
rtk_api_ret_t rtk_switch_initialState_set(init_state_t state)
{
    if(state >= INIT_STATE_END)
        return RT_ERR_FAILED;

    init_state = state;
    return RT_ERR_OK;
}

/* Function Name:
 *      rtk_switch_initialState_get
 * Description:
 *      Get initial status
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      INIT_COMPLETED     - Initialized
 *      INIT_NOT_COMPLETED - Uninitialized
 * Note:
 *
 */
init_state_t rtk_switch_initialState_get(void)
{
    return init_state;
}

/* Function Name:
 *      rtk_switch_logicalPortCheck
 * Description:
 *      Check logical port ID.
 * Input:
 *      logicalPort     - logical port ID
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK       - Port ID is correct
 *      RT_ERR_FAILED   - Port ID is not correct
 *      RT_ERR_NOT_INIT - Not Initialize
 * Note:
 *
 */
rtk_api_ret_t rtk_switch_logicalPortCheck(rtk_port_t logicalPort)
{
    if(init_state != INIT_COMPLETED)
        return RT_ERR_NOT_INIT;

    if(logicalPort >= RTK_SWITCH_PORT_NUM)
        return RT_ERR_FAILED;

    if(halCtrl->l2p_port[logicalPort] == 0xFF)
        return RT_ERR_FAILED;

    return RT_ERR_OK;
}

/* Function Name:
 *      rtk_switch_isUtpPort
 * Description:
 *      Check is logical port a UTP port
 * Input:
 *      logicalPort     - logical port ID
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK       - Port ID is a UTP port
 *      RT_ERR_FAILED   - Port ID is not a UTP port
 *      RT_ERR_NOT_INIT - Not Initialize
 * Note:
 *
 */
rtk_api_ret_t rtk_switch_isUtpPort(rtk_port_t logicalPort)
{
    if(init_state != INIT_COMPLETED)
        return RT_ERR_NOT_INIT;

    if(logicalPort >= RTK_SWITCH_PORT_NUM)
        return RT_ERR_FAILED;

    if(halCtrl->log_port_type[logicalPort] == UTP_PORT)
        return RT_ERR_OK;
    else
        return RT_ERR_FAILED;
}


/* Function Name:
 *      rtk_switch_isHsgPort
 * Description:
 *      Check is logical port a HSG port
 * Input:
 *      logicalPort     - logical port ID
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK       - Port ID is a HSG port
 *      RT_ERR_FAILED   - Port ID is not a HSG port
 *      RT_ERR_NOT_INIT - Not Initialize
 * Note:
 *
 */
rtk_api_ret_t rtk_switch_isHsgPort(rtk_port_t logicalPort)
{
    if(init_state != INIT_COMPLETED)
        return RT_ERR_NOT_INIT;

    if(logicalPort >= RTK_SWITCH_PORT_NUM)
        return RT_ERR_FAILED;

    if(logicalPort == halCtrl->hsg_logical_port)
        return RT_ERR_OK;
    else
        return RT_ERR_FAILED;
}

/* Function Name:
 *      rtk_switch_port_L2P_get
 * Description:
 *      Get physical port ID
 * Input:
 *      logicalPort       - logical port ID
 * Output:
 *      None
 * Return:
 *      Physical port ID
 * Note:
 *
 */
rtk_uint32 rtk_switch_port_L2P_get(rtk_port_t logicalPort)
{
    if(init_state != INIT_COMPLETED)
        return UNDEFINE_PHY_PORT;

    if(logicalPort >= RTK_SWITCH_PORT_NUM)
        return UNDEFINE_PHY_PORT;

    return (halCtrl->l2p_port[logicalPort]);
}



static rtk_api_ret_t _rtk_switch_init_8367c(void)
{
    rtk_port_t port;
    rtk_uint32 retVal;
    rtk_uint32 regData;
    rtk_uint32 regValue;

    if( (retVal = rtl8367c_setAsicReg(0x13c2, 0x0249)) != RT_ERR_OK)
        return retVal;

    if( (retVal = rtl8367c_getAsicReg(0x1301, &regValue)) != RT_ERR_OK)
        return retVal;

    if( (retVal = rtl8367c_setAsicReg(0x13c2, 0x0000)) != RT_ERR_OK)
        return retVal;

    RTK_SCAN_ALL_LOG_PORT(port)
    {
         if(rtk_switch_isUtpPort(port) == RT_ERR_OK)
         {
             if((retVal = rtl8367c_setAsicRegBit(RTL8367C_REG_PORT0_EEECFG + (0x20 * port), RTL8367C_PORT0_EEECFG_EEE_100M_OFFSET, 1)) != RT_ERR_OK)
                 return retVal;

             if((retVal = rtl8367c_setAsicRegBit(RTL8367C_REG_PORT0_EEECFG + (0x20 * port), RTL8367C_PORT0_EEECFG_EEE_GIGA_500M_OFFSET, 1)) != RT_ERR_OK)
                 return retVal;

             if((retVal = rtl8367c_setAsicRegBit(RTL8367C_REG_PORT0_EEECFG + (0x20 * port), RTL8367C_PORT0_EEECFG_EEE_TX_OFFSET, 1)) != RT_ERR_OK)
                 return retVal;

             if((retVal = rtl8367c_setAsicRegBit(RTL8367C_REG_PORT0_EEECFG + (0x20 * port), RTL8367C_PORT0_EEECFG_EEE_RX_OFFSET, 1)) != RT_ERR_OK)
                 return retVal;

             if((retVal = rtl8367c_getAsicPHYOCPReg(port, 0xA428, &regData)) != RT_ERR_OK)
                return retVal;

             regData &= ~(0x0200);
             if((retVal = rtl8367c_setAsicPHYOCPReg(port, 0xA428, regData)) != RT_ERR_OK)
                 return retVal;

             if((regValue & 0x00F0) == 0x00A0)
             {
                 if((retVal = rtl8367c_getAsicPHYOCPReg(port, 0xA5D0, &regData)) != RT_ERR_OK)
                     return retVal;

                 regData |= 0x0006;
                 if((retVal = rtl8367c_setAsicPHYOCPReg(port, 0xA5D0, regData)) != RT_ERR_OK)
                     return retVal;
             }
         }
    }

    if((retVal = rtl8367c_setAsicReg(RTL8367C_REG_UTP_FIB_DET, 0x15BB)) != RT_ERR_OK)
        return retVal;

    if((retVal = rtl8367c_setAsicReg(0x1303, 0x06D6)) != RT_ERR_OK)
        return retVal;

    if((retVal = rtl8367c_setAsicReg(0x1304, 0x0700)) != RT_ERR_OK)
        return retVal;

    if((retVal = rtl8367c_setAsicReg(0x13E2, 0x003F)) != RT_ERR_OK)
        return retVal;

    if((retVal = rtl8367c_setAsicReg(0x13F9, 0x0090)) != RT_ERR_OK)
        return retVal;

    if((retVal = rtl8367c_setAsicReg(0x121e, 0x03CA)) != RT_ERR_OK)
        return retVal;

    if((retVal = rtl8367c_setAsicReg(0x1233, 0x0352)) != RT_ERR_OK)
        return retVal;

    if((retVal = rtl8367c_setAsicReg(0x1237, 0x00a0)) != RT_ERR_OK)
        return retVal;

    if((retVal = rtl8367c_setAsicReg(0x123a, 0x0030)) != RT_ERR_OK)
        return retVal;

    if((retVal = rtl8367c_setAsicReg(0x1239, 0x0084)) != RT_ERR_OK)
        return retVal;

    if((retVal = rtl8367c_setAsicReg(0x0301, 0x1000)) != RT_ERR_OK)
        return retVal;

    if((retVal = rtl8367c_setAsicReg(0x1349, 0x001F)) != RT_ERR_OK)
        return retVal;

    if((retVal = rtl8367c_setAsicRegBit(0x18e0, 0, 0)) != RT_ERR_OK)
        return retVal;

    if((retVal = rtl8367c_setAsicRegBit(0x122b, 14, 1)) != RT_ERR_OK)
        return retVal;

    if((retVal = rtl8367c_setAsicRegBits(0x1305, 0xC000, 3)) != RT_ERR_OK)
        return retVal;

    return RT_ERR_OK;
}

/* Function Name:
 *      rtk_rate_igrBandwidthCtrlRate_set
 * Description:
 *      Set port ingress bandwidth control
 * Input:
 *      port        - Port id
 *      rate        - Rate of share meter
 *      ifg_include - include IFG or not, ENABLE:include DISABLE:exclude
 *      fc_enable   - enable flow control or not, ENABLE:use flow control DISABLE:drop
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 *      RT_ERR_SMI          - SMI access error
 *      RT_ERR_PORT_ID 		- Invalid port number.
 *      RT_ERR_ENABLE 		- Invalid IFG parameter.
 *      RT_ERR_INBW_RATE 	- Invalid ingress rate parameter.
 * Note:
 *      The rate unit is 1 kbps and the range is from 8k to 1048568k. The granularity of rate is 8 kbps.
 *      The ifg_include parameter is used for rate calculation with/without inter-frame-gap and preamble.
 */
rtk_api_ret_t rtk_rate_igrBandwidthCtrlRate_set(rtk_port_t port, rtk_rate_t rate, rtk_enable_t ifg_include, rtk_enable_t fc_enable)
{
    rtk_api_ret_t retVal;

    /* Check initialization state */
    RTK_CHK_INIT_STATE();

    /* Check Port Valid */
    RTK_CHK_PORT_VALID(port);

    if(ifg_include >= RTK_ENABLE_END)
        return RT_ERR_INPUT;

    if(fc_enable >= RTK_ENABLE_END)
        return RT_ERR_INPUT;

    if(rtk_switch_isHsgPort(port) == RT_ERR_OK)
    {
        if ((rate > RTL8367C_QOS_RATE_INPUT_MAX_HSG) || (rate < RTL8367C_QOS_RATE_INPUT_MIN))
            return RT_ERR_QOS_EBW_RATE ;
    }
    else
    {
        if ((rate > RTL8367C_QOS_RATE_INPUT_MAX) || (rate < RTL8367C_QOS_RATE_INPUT_MIN))
            return RT_ERR_QOS_EBW_RATE ;
    }

    if (ifg_include >= RTK_ENABLE_END)
        return RT_ERR_ENABLE;

    if ((retVal = rtl8367c_setAsicPortIngressBandwidth(rtk_switch_port_L2P_get(port), rate>>3, ifg_include,fc_enable)) != RT_ERR_OK)
        return retVal;

    return RT_ERR_OK;
}


/* Function Name:
 *      rtk_rate_egrBandwidthCtrlRate_set
 * Description:
 *      Set port egress bandwidth control
 * Input:
 *      port        - Port id
 *      rate        - Rate of egress bandwidth
 *      ifg_include - include IFG or not, ENABLE:include DISABLE:exclude
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 *      RT_ERR_SMI          - SMI access error
 *      RT_ERR_PORT_ID 		- Invalid port number.
 *      RT_ERR_INPUT 		- Invalid input parameters.
 *      RT_ERR_QOS_EBW_RATE - Invalid egress bandwidth/rate
 * Note:
 *     The rate unit is 1 kbps and the range is from 8k to 1048568k. The granularity of rate is 8 kbps.
 *     The ifg_include parameter is used for rate calculation with/without inter-frame-gap and preamble.
 */
rtk_api_ret_t rtk_rate_egrBandwidthCtrlRate_set( rtk_port_t port, rtk_rate_t rate,  rtk_enable_t ifg_include)
{
    rtk_api_ret_t retVal;

    /* Check initialization state */
    RTK_CHK_INIT_STATE();

    /* Check Port Valid */
    RTK_CHK_PORT_VALID(port);

    if(rtk_switch_isHsgPort(port) == RT_ERR_OK)
    {
        if ((rate > RTL8367C_QOS_RATE_INPUT_MAX_HSG) || (rate < RTL8367C_QOS_RATE_INPUT_MIN))
            return RT_ERR_QOS_EBW_RATE ;
    }
    else
    {
        if ((rate > RTL8367C_QOS_RATE_INPUT_MAX) || (rate < RTL8367C_QOS_RATE_INPUT_MIN))
            return RT_ERR_QOS_EBW_RATE ;
    }

    if (ifg_include >= RTK_ENABLE_END)
        return RT_ERR_ENABLE;

    if ((retVal = rtl8367c_setAsicPortEgressRate(rtk_switch_port_L2P_get(port), rate>>3)) != RT_ERR_OK)
        return retVal;

    if ((retVal = rtl8367c_setAsicPortEgressRateIfg(ifg_include)) != RT_ERR_OK)
        return retVal;

    return RT_ERR_OK;
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
    rtk_uint32  retVal;
    rtl8367c_rma_t rmaCfg;
    switch_chip_t   switchChip;

    /* probe switch */
    if((retVal = rtk_switch_probe(&switchChip)) != RT_ERR_OK)
        return retVal;

    /* Set initial state */

    if((retVal = rtk_switch_initialState_set(INIT_COMPLETED)) != RT_ERR_OK)
        return retVal;

    /* Initial */
    switch(switchChip)
    {
        case CHIP_RTL8367C:
            if((retVal = _rtk_switch_init_8367c()) != RT_ERR_OK)
                return retVal;
            break;
        default:
            return RT_ERR_CHIP_NOT_FOUND;
    }

    /* Set Old max packet length to 16K */
    if((retVal = rtl8367c_setAsicRegBits(RTL8367C_REG_MAX_LENGTH_LIMINT_IPG, RTL8367C_MAX_LENTH_CTRL_MASK, 3)) != RT_ERR_OK)
        return retVal;

    if((retVal = rtl8367c_setAsicRegBits(RTL8367C_REG_MAX_LEN_RX_TX, RTL8367C_MAX_LEN_RX_TX_MASK, 3)) != RT_ERR_OK)
        return retVal;

    /* ACL Mode */
    if((retVal = rtl8367c_setAsicRegBits(RTL8367C_REG_ACL_ACCESS_MODE, RTL8367C_ACL_ACCESS_MODE_MASK, 1)) != RT_ERR_OK)
        return retVal;

    /* Max rate */
    if((retVal = rtk_rate_igrBandwidthCtrlRate_set(halCtrl->hsg_logical_port, RTL8367C_QOS_RATE_INPUT_MAX_HSG, DISABLED, ENABLED)) != RT_ERR_OK)
        return retVal;

    if((retVal = rtk_rate_egrBandwidthCtrlRate_set(halCtrl->hsg_logical_port, RTL8367C_QOS_RATE_INPUT_MAX_HSG, ENABLED)) != RT_ERR_OK)
        return retVal;

    if((retVal = rtl8367c_setAsicReg(0x03fa, 0x0007)) != RT_ERR_OK)
        return retVal;

    /* Change unknown DA to per port setting */
    if((retVal = rtl8367c_setAsicRegBits(RTL8367C_PORT_SECURIT_CTRL_REG, RTL8367C_UNKNOWN_UNICAST_DA_BEHAVE_MASK, 3)) != RT_ERR_OK)
        return retVal;

    /* LUT lookup OP = 1 */
    if ((retVal = rtl8367c_setAsicLutIpLookupMethod(1))!=RT_ERR_OK)
        return retVal;

    /* Set RMA */
    rmaCfg.portiso_leaky = 0;
    rmaCfg.vlan_leaky = 0;
    rmaCfg.keep_format = 0;
    rmaCfg.trap_priority = 0;
    rmaCfg.discard_storm_filter = 0;
    rmaCfg.operation = 0;
    if ((retVal = rtl8367c_setAsicRma(2, &rmaCfg))!=RT_ERR_OK)
        return retVal;

    /* Enable TX Mirror isolation leaky */
    if ((retVal = rtl8367c_setAsicPortMirrorIsolationTxLeaky(ENABLED)) != RT_ERR_OK)
        return retVal;

    /* INT EN */
    if((retVal = rtl8367c_setAsicRegBit(RTL8367C_REG_IO_MISC_FUNC, RTL8367C_INT_EN_OFFSET, 1)) != RT_ERR_OK)
        return retVal;

    return RT_ERR_OK;
}



/* Function Name:
 *      rtk_port_phyReg_set
 * Description:
 *      Set PHY register data of the specific port.
 * Input:
 *      port    - port id.
 *      reg     - Register id
 *      regData - Register data
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK               - OK
 *      RT_ERR_FAILED           - Failed
 *      RT_ERR_SMI              - SMI access error
 *      RT_ERR_PORT_ID          - Invalid port number.
 *      RT_ERR_PHY_REG_ID       - Invalid PHY address
 *      RT_ERR_BUSYWAIT_TIMEOUT - PHY access busy
 * Note:
 *      This API can set PHY register data of the specific port.
 */
rtk_api_ret_t rtk_port_phyReg_set(rtk_port_t port, rtk_port_phy_reg_t reg, rtk_port_phy_data_t regData)
{
    rtk_api_ret_t retVal;

    /* Check initialization state */
    RTK_CHK_INIT_STATE();

    /* Check Port Valid */
    RTK_CHK_PORT_IS_UTP(port);

    if ((retVal = rtl8367c_setAsicPHYReg(rtk_switch_port_L2P_get(port), reg, regData)) != RT_ERR_OK)
        return retVal;

    return RT_ERR_OK;
}

/* Function Name:
 *      rtk_port_phyReg_get
 * Description:
 *      Get PHY register data of the specific port.
 * Input:
 *      port    - Port id.
 *      reg     - Register id
 * Output:
 *      pData   - Register data
 * Return:
 *      RT_ERR_OK               - OK
 *      RT_ERR_FAILED           - Failed
 *      RT_ERR_SMI              - SMI access error
 *      RT_ERR_PORT_ID          - Invalid port number.
 *      RT_ERR_PHY_REG_ID       - Invalid PHY address
 *      RT_ERR_BUSYWAIT_TIMEOUT - PHY access busy
 * Note:
 *      This API can get PHY register data of the specific port.
 */
rtk_api_ret_t rtk_port_phyReg_get(rtk_port_t port, rtk_port_phy_reg_t reg, rtk_port_phy_data_t *pData)
{
    rtk_api_ret_t retVal;

    /* Check initialization state */
    RTK_CHK_INIT_STATE();

    /* Check Port Valid */
    RTK_CHK_PORT_IS_UTP(port);

    if ((retVal = rtl8367c_getAsicPHYReg(rtk_switch_port_L2P_get(port), reg, pData)) != RT_ERR_OK)
        return retVal;

    return RT_ERR_OK;
}

/* Function Name:
 *      rtk_port_phyEnableAll_set
 * Description:
 *      Set all PHY enable status.
 * Input:
 *      enable - PHY Enable State.
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK           - OK
 *      RT_ERR_FAILED       - Failed
 *      RT_ERR_SMI          - SMI access error
 *      RT_ERR_ENABLE       - Invalid enable input.
 * Note:
 *      This API can set all PHY status.
 *      The configuration of all PHY is as following:
 *      - DISABLE
 *      - ENABLE
 */
rtk_api_ret_t rtk_port_phyEnableAll_set(rtk_enable_t enable)
{
    rtk_api_ret_t retVal;
    rtk_uint32 data;
    rtk_uint32 port;

    /* Check initialization state */
    RTK_CHK_INIT_STATE();

    if (enable >= RTK_ENABLE_END)
        return RT_ERR_ENABLE;

    if ((retVal = rtl8367c_setAsicPortEnableAll(enable)) != RT_ERR_OK)
        return retVal;

    RTK_SCAN_ALL_LOG_PORT(port)
    {
        if(rtk_switch_isUtpPort(port) == RT_ERR_OK)
        {
            if ((retVal = rtk_port_phyReg_get(port, PHY_CONTROL_REG, &data)) != RT_ERR_OK)
                return retVal;

            if (ENABLED == enable)
            {
                data &= 0xF7FF;
                data |= 0x0200;
            }
            else
            {
                data |= 0x0800;
            }

            if ((retVal = rtk_port_phyReg_set(port, PHY_CONTROL_REG, data)) != RT_ERR_OK)
                return retVal;
        }
    }

    return RT_ERR_OK;

}

/**************************************************************************************************/
/*                                           PUBLIC_FUNCTIONS                                     */
/**************************************************************************************************/


void rt_rtl8367_initVlan()
{
#if 0
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
#endif
}



/* 
 * According to <RTL8367S_Switch_ProgrammingGuide> 4.14 Force External Interface 
 * For RTL8367S, MAC7 <---> RG2(Port2)
 */
void rt_rtl8367_enableRgmii(void)
{
	/* 
	 * 1. rtl8367c_setAsicPortExtMode
	 * (EXT_PORT_1, MODE_EXT_RGMII)
	 */

	rtl8367c_setAsicRegBit(RTL8367C_REG_BYPASS_LINE_RATE, 2, 0);
	rtl8367c_setAsicRegBits(RTL8367C_REG_DIGITAL_INTERFACE_SELECT_1, 0xF, 1);

	/* 2. rtl8367c_getAsicPortForceLinkExt */
	/* 3. rtl8367c_setAsicPortForceLinkExt */
	{
		u32 reg_data;
		//rtl8367c_port_ability_t *pExtPort1 = (u16*)&reg_data;

		rtl_smi_read(RTL8367C_REG_DIGITAL_INTERFACE2_FORCE, &reg_data);

		reg_data &= ~0x10f7;
		reg_data |= ((1<<12) | (2 << 0) | (1 << 2) | (7 << 4));
		
		/*pExtPort1->forcemode = 1;
		pExtPort1->speed = 2;
		pExtPort1->duplex = 1;
		pExtPort1->link = 1;
		pExtPort1->nway = 0;
		pExtPort1->txpause = 1;
		pExtPort1->rxpause = 1;*/
		rtl_smi_write(RTL8367C_REG_DIGITAL_INTERFACE2_FORCE, reg_data);
	}

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
			printf("Wait for RTL8367C Ready\n");
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
	printf("\nRTL8367C is ready now!\n");
#endif
	{
		int ret;
		if ((ret = rtk_switch_init()) != 0)
		{
			printf("init switch error: 0x%08x...\n", ret);
		}
	}

	rtk_port_phyEnableAll_set(ENABLED);

	rtl_smi_write(0x13c5, 0xc);
	RTL8367_DEBUG("Call Func rt_rtl8367_enableRgmii()\n");
	rt_rtl8367_enableRgmii();

	/*RTL8367_DEBUG("Call Func rt_rtl8367_initVlan()\n");*/
	/*rt_rtl8367_initVlan();*/

}

void disableEthForward()
{
	u32 index = 0;
	u32 portIsolationCtrlReg = 0x08a2;

	for (index = 0; index < 8; index++)
	{
		rtl_smi_write(portIsolationCtrlReg + index, 0x0);
	}

	printf("disable switch forward...\n");
	
}
#ifndef CONFIG_REDUCE_FLASH_SIZE /* disable to reduce flash size*/
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
#if 0
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
#endif
		else if (!strncmp(argv[1], "init", 5) && 2 == argc)
		{
			rt_rtl8367_init();
		}
#if 0
		else if (!strncmp(argv[1], "stat", 5) && 2 == argc)
		{
			rt_rtl8367_stat(8);
		}
#endif
		else if (!strncmp(argv[1], "inner", 6) && 2 == argc)
		{
			#define RALINK_ETH_SW_BASE 0xB0110000
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
#endif /*CONFIG_REDUCE_FLASH_SIZE*/
