/**
 * @file libMoca.h
 * @author MStar Semiconductor, Inc.
 * @date 2 April 2014
 * @brief File containing functions of libmoca2.
 *
 */

/*! \mainpage libmoca2 documentation
 * \section sec_1 Funtionality
 * - Enable MoCA
 * - Disable MoCA
 * - Get parameter from MoCA
 * - Set parameter to MoCA
 * 
 * \section sec_2 Related files
 * - libmoca2.so --> moca2 shared library
 * - libMoca.h, libMoca_param.h, moca_Pragmas.h, moca_types.h --> header files for this library
 * - mocad2 --> moca2 daemon
 * - mocad2Init -->daemon init script
 *
 * \section sec_3 How to use this library?
 * - Please execute mocad2 before using this lib.
 * - Please execute mocad2 by using script mocad2Init. (./mocad2Init start)
 * - Include header files in your project and link this library (-lmoca2).
 * - Please use the compile option on a little endian system: -DMOCA_LITTLE_ENDIAN=1
 * - Please use the compile option on a big endian system: -DMOCA_LITTLE_ENDIAN=0
 *
 * \section sec_4 System reqirements
 *  - libpcap
 *  - libpthread
 *  - libstdc++
 *  - Kernel support "Unix domain sockets"
 */


#ifndef _LIB_MOCA_H_
#define _LIB_MOCA_H_

#include "moca_types.h"
#include "libMoca_param.h"

/**
 * @brief  Error code for lib moca.
 *
 * If you get some errors from functions in libmoca, 
 * you can know what kind of error that occurred.
 */
typedef enum
{
    LIBMOCA_RET_OK = 0, /**< No error */
    LIBMOCA_RET_INVALID_PARAMETER, /**< Invalid parameter */
    LIBMOCA_RET_ERR_SEM, /**< semaphore error*/
    LIBMOCA_RET_ERR_SOCK_CREATE,/**< create unix socket error*/    
    LIBMOCA_RET_ERR_SOCK_BIND,/**< bind unix socket error*/        
    LIBMOCA_RET_ERR_SOCK_CONNET, /**<  connect to mocad2 error*/    
    LIBMOCA_RET_ERR_SOCK_SEND, /**<  send error*/        
    LIBMOCA_RET_ERR_MOCAD_TIMEOUT,/**<  mocad timeout*/    
    LIBMOCA_RET_ERR_MOCAD_PARA,/**<  mocad parameter error*/        
    LIBMOCA_RET_ERR_MOCAD_FAIL,/**<  mocad return fail*/    
} libmocaRetCode_e;

/**
* @brief Init enable parameter (libmocaEnableReq_t) for band D
* @param[out] ptReq enable parameter
* @return Return code of this function.
* @note Suggest to use this function to init enable parameter before using structure libmocaEnableReq_t
* @note The following defaults are used:
* @note aMacAddress = 00:01:02:03:04:05
* @note lofFreqMhz = 1150
* @note scanFromFreqMhz = 1150
* @note scanToFreqMhz = 1600
* @note aPassword = 
* @note mode = LibMoca_PreferredNC_Auto
* @note mocaMode = LibMoca_mocaMode_Mixed
* @note priOffset = 0
* @note isPolicing = 0
* @note tlpMinTimeS = 0
* @note tlpMaxTimeS = 95
* @note isConstantPower = 0
* @note maxTxPower = 12
* @note beaconBackoff = 0          
*/
libmocaRetCode_e libmoca_enableInitParam(libmocaEnableReq_t *ptReq);

/**
* @brief Interpret error code of libmoca (libmocaRetCode_e)
* @param[in]  code The error code that need to be interpreted.
* @return The result string of interpreter.
*/
const char * libmoca_InterpretErrCode(libmocaRetCode_e code);


/**
* @brief Enable MoCA device.
*
* Host processor shall send enable operation request to enable both the MoCA MAC and PHY.
* @param[in]  ptReq enable parameter
* @return Return code of this function.
* @note Please use libmoca_disable() to disable MoCA device before change the enable paramter.
* @note Please use libmoca_enableInitParam() to init enable parameter bofore calling this function.
* @note Please set the frequency related parameters (scanFromFreqMhz, scanToFreqMhz and lofFreqMhz) if the operation band is not band D.
* example:
* @code
* libmocaEnableReq_t tReq;
* libmoca_disable(); 
* libmoca_enableInitParam(&tReq);
* tReq.scanFromFreqMhz = 1200;
* tReq.scanToFreqMhz = 1200;
* tReq.lofFreqMhz = 1200;
* libmoca_enable(&tReq);
* @endcode
* @see libmoca_enableInitParam()
*/
libmocaRetCode_e libmoca_enable(libmocaEnableReq_t *ptReq);

/**
* @brief Disable MoCA device.
*
* Host processor may send disable operation request to disable both the MoCA MAC and PHY. 
* After disabled is completed, there shall not be any RF signal generated from the PHY, and there should not be any control frame initiated by the MoCA processor.
* @return Return code of this function.
*/
libmocaRetCode_e libmoca_disable(void);

/**
* @brief Get summary statistics (TX/Rx Ethernet packet counts only)
* @param[out]  ptRsp Structure pointer of this function's responce
* @return Return code of this function.
*/
libmocaRetCode_e libmoca_getMocaStats(getRes_moca_stats *ptRsp);


/**
* @brief Get summary network information.
* @param[out]  ptRsp Structure pointer of this function's responce
* @return Return code of this function.
*/
libmocaRetCode_e libmoca_getMacCtrl(getRes_mac_ctrl *ptRsp);

/**
* @brief Get some current PHY-related operating parameters.
* @param[out]  ptRsp Structure pointer of this function's responce
* @return Return code of this function.
*/
libmocaRetCode_e libmoca_getPhyCtrl(getRes_phy_ctrl *ptRsp);

/**
* @brief Get Vendor ID.
* @param[out] ptRsp Structure pointer of this function's responce
* @return Return code of this function.
*/
libmocaRetCode_e libmoca_getVendorId(getRes_vendor_id *ptRsp);

/**
* @brief Get current HW/SW version
* @param[out] ptRsp Structure pointer of this function's responce
* @return Return code of this function.
*/
libmocaRetCode_e libmoca_getVersion(getRes_version_info *ptRsp);

/**
* @brief Get SAPM (Subcarrier Added PHY Margin) table. (SAPM(EN), SAPM and ARPL_THLD)
* @param[out] ptRsp Structure pointer of this function's responce
* @return Return code of this function.
*/
libmocaRetCode_e libmoca_getSapm(getRes_sapm_table *ptRsp);

/**
* @brief Get RLAPM (Receive Level Added PHY Margin) talbe. (RLAPM(EN) and RLAPM(FUNC))
* @param[out] ptRsp Structure pointer of this function's responce
* @return Return code of this function.
*/
libmocaRetCode_e libmoca_getRlamp(getRes_rlapm_table *ptRsp);

/**
* @brief Get all power state of each node and M1 TX power variation.
* @param[out] ptRsp Structure pointer of this function's responce
* @return Return code of this function.
*/
libmocaRetCode_e libmoca_getPowerState(getRes_power_state *ptRsp);

/**
* @brief Get the freq. offset in Hz to another node
* @param[out] ptRsp Structure pointer of this function's responce
* @return Return code of this function.
*/
libmocaRetCode_e libmoca_getFreqOffset(int node, getRes_freq_offset *ptRsp);

/**
* @brief Get the moca daemon version number
* @param[out] ptRsp Structure pointer of this function's responce
* @return Return code of this function.
*/
libmocaRetCode_e libmoca_getMocadVersion(getRes_mocad_version *ptRsp);

/**
* @brief Get the phy rate (NPER/VLPER) of each node
* @param[out] ptRsp Structure pointer of this function's responce
* @return Return code of this function.
*/
libmocaRetCode_e libmoca_getPhyRate(getRes_phy_rate *ptRsp);

/**
* @brief Set SAPM(EN), SAPM and ARPL_THLD
* @param[in] pSapm A pointer that point to structure setReq_sapm_table
* @return Return code of this function.
* @see setReq_sapm_table
*/
libmocaRetCode_e libmoca_setSapm(setReq_sapm_table *pSapm);

/**
* @brief Set RLAPM(EN) and RLAPM(FUNC)
* @param[in] pRlapm A pointer that point to structure setReq_rlapm_table
* @return Return code of this function.
* @see setReq_rlapm_table
*/
libmocaRetCode_e libmoca_setRlapm(setReq_rlapm_table *pRlapm);

/**
* @brief RF do re-calibration.
* @return Return code of this function.
*/
libmocaRetCode_e libmoca_setRfRecalibration(void);

/**
* @brief Enable continuous TX
* @param[in] cont_mode TX mode
* @param[in] freqInMHz  RF channel frequency in MHz
* @param[in] Transmit power backoff from max power in dB (with 3dB resolution, Valid value: 0~36).
* @return Return code of this function.
* @note Please disable cont tx mode before change the parameter of cont tx mode.
* example:
* @code
* libmoca_setContTx(LibMoca_ContTx_M2Spectrum, 1200, 0);
* libmoca_setContTx(LibMoca_ContTx_Disable, 1200, 0);
* libmoca_setContTx(LibMoca_ContTx_M2Spectrum, 1250, 0);
* @endcode
*/
libmocaRetCode_e libmoca_setContTx(eLibMoca_ContTx cont_mode, uint32 freqInMHzm, uint16 backoffdB);

/**
* @brief Enable continuous RX
* @param[in] bEnable Enable or disable continuous RX mode
* @param[in] freqInMHz  RF channel frequency in MHz
* @return Return code of this function.
* @note Please disable cont rx mode before change the parameter of cont rx mode.
* example:
* @code
* libmoca_setContRx(1, 1200); //Enable continuous RX mode in 1200 MHz
* libmoca_setContRx(0, 1200); //Disable continuous RX mode
* libmoca_setContRx(1, 1250); //Enable continuous RX mode in 1250 MHz
* @endcode
*/
libmocaRetCode_e libmoca_setContRx(unsigned char bEnable, uint32 freqInMHz);

#endif // _LIB_MOCA_H_

