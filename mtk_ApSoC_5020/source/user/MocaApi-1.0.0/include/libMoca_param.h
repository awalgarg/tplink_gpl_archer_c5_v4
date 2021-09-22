/**
 * @file libMoca_param.h
 * @author MStar Semiconductor, Inc.
 * @date 2 April 2014
 * @brief File containing structure and emuration used by libmoca function.
 *
 */

#ifndef _LIB_MOCA_PARAM_H_
#define _LIB_MOCA_PARAM_H_

#ifndef MOCA_LITTLE_ENDIAN
#define MOCA_LITTLE_ENDIAN 1
#endif

typedef enum eLibMoca_Mode {
    LibMoca_PreferredNC_Auto    = 3, /**< Compatible operation where a node listens at each frequency and if no beacons are detected, beacons are transmitted for a period of time.*/
    LibMoca_PreferredNC_AutoNc  = 4, /**< Compatible mode where the node identifies itself as a preferred NC */
} eLibMoca_Mode;

typedef enum eLibMoca_MocaMode {
    LibMoca_mocaMode_Mixed   = 1,  /**< The operation mode of a MoCA network consist a MoCA2.0 NC.*/
    LibMoca_mocaMode_Turbo  = 3, /**< When referring to a network, a MoCA network in MoCA 2.0 Mode 13 consisting of only two Nodes, both having their TURBO_MODE_EN parameter set to ENABLED. */
} eLibMoca_MocaMode;

typedef enum eLibMoca_Led {
    LibMoca_Led_Disable_PinUartRx    = 0, /**< Disable LED function. (for LED connected with pin "UartRx")*/    
    LibMoca_Led_Enable_PinUartRx  = 1, /**< Enable LED function. (for LED connected with pin "UartRx")*/    
    LibMoca_Led_Disable_PinIntOut    = 2, /**< Disable LED function. (for LED connected with pin "IntOut")*/    
    LibMoca_Led_Enable_PinIntOut  = 3, /**< Enable LED function. (for LED connected with pin "IntOut") */
} eLibMoca_Led;
/**
 * Enable request parameters
 */
typedef struct
{
    uint8 aMacAddress[18]; /**< set the MoCA node's MAC address */
    uint16 lofFreqMhz; /**< The last RF channel frequency on which a node was successfully in the steady state. */
    uint16 scanFromFreqMhz; /**< Start frequency of scan plan */
    uint16 scanToFreqMhz; /**< End frequency of scan plan*/
    uint8 aPassword[18]; /**< Set the privacy password  (12 to 17 decimal digits, default: NULL) */
    eLibMoca_Mode mode; /**< Set the prefer NC mode  (default: LibMoca_PreferredNC_Auto)*/
    eLibMoca_MocaMode mocaMode; /**< Set the MoCA mode  (default: LibMoca_mocaMode_Mixed) */
    int8 priOffset; /**< Relative RF channel number of the center frequency of the MoCA 2.0 Primary Channel to the channel number in CHANNEL_NUMBER field. (-25, 0, 25 Mhz; default: 0) */
    int8 isPolicing; /**< At runtime to enforce the 14 TSPEC of a PQoS Flow, detect TSPEC violations by the Flow, and prevent such violations from negatively 15 impacting other PQoS Flows in their usage of the network bandwidth. (0:disable 1:enable; default:0) */
    int tlpMinTimeS; /**< Minimum time to form a network (Valid value:0~215 s, , default:0)  */
    int tlpMaxTimeS; /**< Maximum time to form a network (Valid value:0~217 s, , default:95)  */
    uint8 isConstantPower; /**< Set constant power  (0:disable 1:enable; default:0)  */
    uint8 maxTxPower; /**< Set the maximum TX power(dB). (Valid value: 2~12, default:12) */
    uint8 beaconBackoff; /**< Backoff 0,3,6,9,12dB from max power (default:0) */
    eLibMoca_Led led; /**< Enable/Disable LED function on corresponding HW PIN. LED will be turned on when MoCA network is link up. (default: LibMoca_Led_Disable_PinUartRx)  */    
} libmocaEnableReq_t;

typedef union LibMoca_ProtocolSupport {
    struct {
        uint8               versionIndicator;
        uint8               _reserved_0x00;
#if MOCA_LITTLE_ENDIAN==1
        union {
            uint8       aggLevel;
            struct {
                uint8   _reservedType1_1        :1;
                uint8   bondingCapable          :1;
                uint8   concatCapable           :1;
                uint8   _reservedTypeIII_b      :5;
            };
        };

        uint8           _reserved_0x7           :4; 
        uint8           txQam256                :1;
        uint8           _reserved_0x0           :1;
        uint8           preferredNc             :1;
        uint8           proTem                  :1;
#else
        union {
            uint8       aggLevel;
            struct {
                uint8   _reservedTypeIII_b      :5;
                uint8   concatCapable           :1;
                uint8   bondingCapable          :1;
                uint8   _reservedType1_1        :1;
            };
        };
        uint8           proTem                  :1;
        uint8           preferredNc             :1;
        uint8           _reserved_0x0           :1;
        uint8           txQam256                :1;
        uint8           _reserved_0x7           :4;
#endif
    };
    uint32              all;
} _moca_packed_attribute_ LibMoca_ProtocolSupport;


typedef struct LibMoca_NodeInfo {
    uint8                   _reserved1_TypeI_00;
    uint8                   nodeId;
    union {
        uint16              _reserved2_TypeII;
        struct {
            uint8           _reserved4_TypeIII;
            union {
                uint8           nodePsByte;
                struct {
#if MOCA_LITTLE_ENDIAN==1
                    uint8       m1TxPowerVariation  : 3;
                    uint8       nodePowerState      : 2;
                    uint8       _reserved3_TypeIII  : 3;
#else
                    uint8       _reserved3_TypeIII  : 3;
                    uint8       nodePowerState      : 2;
                    uint8       m1TxPowerVariation  : 3;
#endif
                };
            };
        };
    };
    LibMoca_ProtocolSupport    nodeProtocolSupport;
    guid64                  guid;
} _moca_packed_attribute_ LibMoca_NodeInfo;

typedef struct LibMoca_rlapm_entry {
   int16  rlapm_threshold; /**< GARPL: 0~-65,  0~-65dBm */
   uint16 rlapm_margin;  /**< RLAPM: 0~60, 0~30dB in 0.5 dB*/
} LibMoca_rlapm_entry;

typedef struct LibMoca_stats_entry {
    uint32 node_id;
    uint32 ingress_packets;
    uint32 egress_packets;
    uint32 egress_errors;
} LibMoca_stats_entry;

typedef struct getRes_moca_stats {
    uint32 tx_packets;
    uint32 rx_packets;
    uint32 n_nodes;
    LibMoca_stats_entry entry[16];
} getRes_moca_stats;

typedef struct LibMoca_node_info_entry {
    LibMoca_NodeInfo moca_node_info;
    uint32 tx_phy_rate;
    uint32 rx_phy_rate;
    uint32 rx_gcd_rate;
    int16  tx_pwr_dbmv;
    int16  rx_pwr_dbmv;
    int16  rx_gcd_dbmv;
    int8   rx_beacon_dbmv;
    int8   num_probes;
    int32  snr;
} _moca_packed_attribute_ LibMoca_node_info_entry;


typedef struct getRes_mac_ctrl {
    uint32 mac_version;
    uint32 link_status;
    uint32 enable_privacy;
    uint8  node_id;
    uint8  nc_node;
    uint8  backup_nc_node;
    uint8  lmo_node;
    uint32 n_nodes;
    LibMoca_node_info_entry node_info[16];
} _moca_packed_attribute_ getRes_mac_ctrl;

typedef struct getRes_phy_ctrl {
    uint32 rf_channel;
    uint32 last_operational_freq;
    uint32 qam256_capable;
    uint32 taboo_start_freq;/**< Bit value 1 = unusable channel, MSB = lowest frequency in the range (corresponding to the taboo_start_freq frequency).  Each successive bit after the MSB represents a frequency 25 MHz higher than the previous bit.*/
    uint32 taboo_channel_mask;/**< RF channel number of the lowest frequency channel covered by the taboo_channel_mask. */
} getRes_phy_ctrl;

typedef struct getRes_vendor_id {
    uint32 vendor_id;
    uint32 sw_version;
    uint32 hw_version;
} getRes_vendor_id;

typedef struct getRes_version_info {
    uint32 mac_hw_version; /**< A version number of MAC HW. */
    uint32 phy_hw_version; /**< A version number of PHY HW. */ 
    char sw_version_string[32]; /**< A string array contains MoCA software version */
    uint32 sw_build_date; /**< The build date of this SW. */ 
    uint32 rf_version; /**< A version number of RF. */ 
} getRes_version_info;

typedef struct libMoca_power_state_entry {
    uint8 node_id;
    uint8 power_state;
    uint8 m1_tx_power_var;
    uint8 rfu8;
} libMoca_power_state_entry;

typedef struct getRes_power_state {
    uint32            number;
    libMoca_power_state_entry entry[16];
} getRes_power_state;

typedef struct getRes_freq_offset {            
    int32 freq_offset;  /**< freq. offset in Hz to another node*/
} _moca_packed_attribute_  getRes_freq_offset;

typedef struct getRes_mocad_version {
    uint8              mocad_major; /**< A major version number of mocad. */
    uint8              mocad_minor; /**< A minor version number of mocad. */    
    uint8              mocad_sublevel; /**< A sub-level number of mocad. */    
    uint8              libmoca_major;  /**< A major version number of libmoca. */
    uint8              libmoca_minor; /**< A major version number of libmoca. */    
    uint8              libmoca_sublevel;  /**< A major version number of libmoca. */
    uint32            mocaItf_ver; /**< This is the version number of sub-module of mocad. */    
} getRes_mocad_version;

typedef struct libmoca_phy_rate {
    uint8   src_node;
    uint8   dest_node;
    float   rate_nper;  
    float   rate_vlper;    
} libmoca_phy_rate;

typedef struct getRes_phy_rate {
    uint32            number;    
    libmoca_phy_rate rate[16*16]; 
} getRes_phy_rate;

/**
 * Enumerations for continue TX mode
 */
typedef enum eLibMoca_ContTx { 
    LibMoca_ContTx_Disable    = 0x00000000, /**< Disable continuous transmission */
    LibMoca_ContTx_Cw         = 0x00000001, /**< Enable carrier wave continuous transmission*/
    LibMoca_ContTx_Spectrum   = 0x00000002, /**< Enable spectrum continuous transmission*/
    LibMoca_ContTx_M2Spectrum = 0x00000005, /**< Enable 100 MHz spectrum continuous transmission*/
} eLibMoca_ContTx;

#define LibMoca_Apm_MaxGarplPairs 61
typedef struct setReq_sapm_table {
    uint32 enable;/**< Enable or disable the SAPM function (SAPM(EN)). 1:enable 0:dsiable */
    int32  sapm_threshold;/**< The ARPL threshold to be used by the node when SAPM(EN)=ENABLED. (Valid ARPL_THLD: 0 to -65, 0 to -65 dBm in steps of 1 dB) */     
    uint8  sapm_margin[512];/**< An array contains 512 SAPM values. (Valid SAPM value: 0 to 120, 0 dB to 60.0 dB in 0.5 dB steps) */
} setReq_sapm_table;

typedef struct setReq_rlapm_table {
    uint32      enable;/**< Enable or disable the RLAPM function (RLAPM(EN)). 1:enable 0:dsiable*/       
    uint32      number;/**< Numbers of pairs of GAPRL and RLAPM*/   
    LibMoca_rlapm_entry entry[LibMoca_Apm_MaxGarplPairs];/**< An array contains the pairs of GAPRL and RLAPM (RLAPM(FUNC)).*/   
} setReq_rlapm_table;

typedef setReq_sapm_table  getRes_sapm_table;
typedef setReq_rlapm_table getRes_rlapm_table;

#endif  // _LIB_MOCA_PARAM_H_
