#if !defined(__DWC_CORE_IF_H__)
#define __DWC_CORE_IF_H__

/** @file
 * This file defines DWC_OTG Core API
 */

/**
 * The following parameters may be specified when starting the module. These
 * parameters define how the DWC_otg controller should be configured.
 */
typedef struct dwc_otg_core_params {
	int32 opt;

	/**
	 * Specifies the OTG capabilities. The driver will automatically
	 * detect the value for this parameter if none is specified.
	 * 0 - HNP and SRP capable (default)
	 * 1 - SRP Only capable
	 * 2 - No HNP/SRP capable
	 */
	int32 otg_cap;

	/**
	 * Specifies whether to use slave or DMA mode for accessing the data
	 * FIFOs. The driver will automatically detect the value for this
	 * parameter if none is specified.
	 * 0 - Slave
	 * 1 - DMA (default, if available)
	 */
	int32 dma_enable;

	/**
	 * When DMA mode is enabled specifies whether to use address DMA or DMA Descritor mode for accessing the data
	 * FIFOs in device mode. The driver will automatically detect the value for this
	 * parameter if none is specified.
	 * 0 - address DMA
	 * 1 - DMA Descriptor(default, if available)
	 */
	int32 dma_desc_enable;
	/** The DMA Burst size (applicable only for External DMA
	 * Mode). 1, 4, 8 16, 32, 64, 128, 256 (default 32)
	 */
	int32 dma_burst_size;	/* Translate this to GAHBCFG values */

	/**
	 * Specifies the maximum speed of operation in host and device mode.
	 * The actual speed depends on the speed of the attached device and
	 * the value of phy_type. The actual speed depends on the speed of the
	 * attached device.
	 * 0 - High Speed (default)
	 * 1 - Full Speed
	 */
	int32 speed;
	/** Specifies whether low power mode is supported when attached
	 *	to a Full Speed or Low Speed device in host mode.
	 * 0 - Don't support low power mode (default)
	 * 1 - Support low power mode
	 */
	int32 host_support_fs_ls_low_power;

	/** Specifies the PHY clock rate in low power mode when connected to a
	 * Low Speed device in host mode. This parameter is applicable only if
	 * HOST_SUPPORT_FS_LS_LOW_POWER is enabled.	 If PHY_TYPE is set to FS
	 * then defaults to 6 MHZ otherwise 48 MHZ.
	 *
	 * 0 - 48 MHz
	 * 1 - 6 MHz
	 */
	int32 host_ls_low_power_phy_clk;

	/**
	 * 0 - Use cC FIFO size parameters
	 * 1 - Allow dynamic FIFO sizing (default)
	 */
	int32 enable_dynamic_fifo;

	/** Total number of 4-byte words in the data FIFO memory. This
	 * memory includes the Rx FIFO, non-periodic Tx FIFO, and periodic
	 * Tx FIFOs.
	 * 32 to 32768 (default 8192)
	 * Note: The total FIFO memory depth in the FPGA configuration is 8192.
	 */
	int32 data_fifo_size;

	/** Number of 4-byte words in the Rx FIFO in device mode when dynamic
	 * FIFO sizing is enabled.
	 * 16 to 32768 (default 1064)
	 */
	int32 dev_rx_fifo_size;

	/** Number of 4-byte words in the non-periodic Tx FIFO in device mode
	 * when dynamic FIFO sizing is enabled.
	 * 16 to 32768 (default 1024)
	 */
	int32 dev_nperio_tx_fifo_size;

	/** Number of 4-byte words in each of the periodic Tx FIFOs in device
	 * mode when dynamic FIFO sizing is enabled.
	 * 4 to 768 (default 256)
	 */
	uint32 dev_perio_tx_fifo_size[MAX_PERIO_FIFOS];

	/** Number of 4-byte words in the Rx FIFO in host mode when dynamic
	 * FIFO sizing is enabled.
	 * 16 to 32768 (default 1024)
	 */
	int32 host_rx_fifo_size;

    /** Number of 4-byte words in the non-periodic Tx FIFO in host mode
	 * when Dynamic FIFO sizing is enabled in the core.
	 * 16 to 32768 (default 1024)
	 */
	int32 host_nperio_tx_fifo_size;

	/** Number of 4-byte words in the host periodic Tx FIFO when dynamic
	 * FIFO sizing is enabled.
	 * 16 to 32768 (default 1024)
	 */
	int32 host_perio_tx_fifo_size;

	/** The maximum transfer size supported in bytes.
	 * 2047 to 65, 535  (default 65,535)
	 */
	int32 max_transfer_size;

	/** The maximum number of packets in a transfer.
	 * 15 to 511  (default 511)
	 */
	int32 max_packet_count;

	/** The number of host channel registers to use.
	 * 1 to 16 (default 12)
	 * Note: The FPGA configuration supports a maximum of 12 host channels.
	 */
	int32 host_channels;

	/** The number of endpoints in addition to EP0 available for device
	 * mode operations.
	 * 1 to 15 (default 6 IN and OUT)
	 * Note: The FPGA configuration supports a maximum of 6 IN and OUT
	 * endpoints in addition to EP0.
	 */
	int32 dev_endpoints;

    /**
     * Specifies the type of PHY interface to use. By default, the driver
     * will automatically detect the phy_type.
     *
     * 0 - Full Speed PHY
     * 1 - UTMI+ (default)
     * 2 - ULPI
     */
	int32                                         phy_type;

	/**
	 * Specifies the UTMI+ Data Width.	This parameter is
	 * applicable for a PHY_TYPE of UTMI+ or ULPI. (For a ULPI
	 *    PHY_TYPE,                         this parameter indicates the data width between
	 * the MAC and the ULPI Wrapper.) Also, this parameter is
	 * applicable only if the OTG_HSPHY_WIDTH cC parameter was set
	 * to "8 and 16 bits",                  meaning that the core has been
	 * configured to work at either data path width.
	 *
	 * 8 or 16 bits (default 16)
	 */
	int32 phy_utmi_width;

	/**
	 * Specifies whether the ULPI operates at double or single
	 * data rate. This parameter is only applicable if PHY_TYPE is
	 * ULPI.
	 *
	 * 0 - single data rate ULPI interface with 8 bit wide data
	 * bus (default)
	 * 1 - double data rate ULPI interface with 4 bit wide data
	 * bus
	 */
	int32 phy_ulpi_ddr;

	/**
	 * Specifies whether to use the internal or external supply to
	 * drive the vbus with a ULPI phy.
	 */
	int32 phy_ulpi_ext_vbus;

	/**
	 * Specifies whether to use the I2Cinterface for full speed PHY. This
	 * parameter is only applicable if PHY_TYPE is FS.
	 * 0 - No (default)
	 * 1 - Yes
	 */
	int32 i2c_enable;

	int32 ulpi_fs_ls;

	int32 ts_dline;

	/**
	 * Specifies whether dedicated transmit FIFOs are
	 * enabled for non periodic IN endpoints in device mode
	 * 0 - No
	 * 1 - Yes
	 */
	int32 en_multiple_tx_fifo;

	/** Number of 4-byte words in each of the Tx FIFOs in device
	 * mode when dynamic FIFO sizing is enabled.
	 * 4 to 768 (default 256)
	 */
	uint32 dev_tx_fifo_size[MAX_TX_FIFOS];

	/** Thresholding enable flag-
	 * bit 0 - enable non-ISO Tx thresholding
	 * bit 1 - enable ISO Tx thresholding
	 * bit 2 - enable Rx thresholding
	 */
	uint32 thr_ctl;

	/** Thresholding length for Tx
	 *	FIFOs in 32 bit DWORDs
	 */
	uint32 tx_thr_length;

	/** Thresholding length for Rx
	 *	FIFOs in 32 bit DWORDs
	 */
	uint32 rx_thr_length;

	/**
	 * Specifies whether LPM (Link Power Management) support is enabled
	 */
	int32 lpm_enable;

	/** Per Transfer Interrupt
	 *	mode enable flag
	 * 1 - Enabled
	 * 0 - Disabled
	 */
	int32 pti_enable;

	/** Multi Processor Interrupt
	 *	mode enable flag
	 * 1 - Enabled
	 * 0 - Disabled
	 */
	int32 mpi_enable;

	/** IS_USB Capability
	 * 1 - Enabled
	 * 0 - Disabled
	 */
	int32 ic_usb_cap;

	/** AHB Threshold Ratio
	 * 2'b00 AHB Threshold = MAC Threshold
	 * 2'b01 AHB Threshold = 1/2 	MAC Threshold
	 * 2'b10 AHB Threshold = 1/4	MAC Threshold
	 * 2'b11 AHB Threshold = 1/8	MAC Threshold
	 */
	int32 ahb_thr_ratio;

} dwc_otg_core_params_t;

/**
 * The <code>dwc_otg_core_if</code> structure contains information needed to manage
 * the DWC_otg controller acting in either host or device mode. It
 * represents the programming view of the controller as a whole.
 */
struct dwc_otg_core_if {
	/** Parameters that define how the core should be configured.*/
	dwc_otg_core_params_t *core_params;

	/** Core Global registers starting at offset 000h. */
	dwc_otg_core_global_regs_t *core_global_regs;

	/** Host-specific information */
	dwc_otg_host_if_t *host_if;

	/** Value from SNPSID register */
	uint32 snpsid;

	/*
	 * Set to 1 if the core PHY interface bits in USBCFG have been
	 * initialized.
	 */
	uint8 phy_init_done;

	/* Common configuration information */
	/** Power and Clock Gating Control Register */
	volatile uint32 *pcgcctl;
#define DWC_OTG_PCGCCTL_OFFSET 0xE00

	/** Push/pop addresses for endpoints or host channels.*/
	uint32 *data_fifo[MAX_EPS_CHANNELS];
#define DWC_OTG_DATA_FIFO_OFFSET 0x1000
#define DWC_OTG_DATA_FIFO_SIZE   0x1000

	/** Total RAM for FIFOs (Bytes) */
	uint16 total_fifo_size;
	/** Size of Rx FIFO (Bytes) */
	uint16 rx_fifo_size;
	/** Size of Non-periodic Tx FIFO (Bytes) */
	uint16 nperio_tx_fifo_size;

	/** 1 if DMA is enabled, 0 otherwise. */
	uint8 dma_enable;

	/** 1 if DMA descriptor is enabled, 0 otherwise. */
	uint8 dma_desc_enable;

	/** 1 if PTI Enhancement mode is enabled, 0 otherwise. */
	uint8 pti_enh_enable;

	/** 1 if MPI Enhancement mode is enabled, 0 otherwise. */
	uint8 multiproc_int_enable;

	/** 1 if dedicated Tx FIFOs are enabled, 0 otherwise. */
	uint8 en_multiple_tx_fifo;

	/** Set to 1 if multiple packets of a high-bandwidth transfer is in
	 * process of being queued */
	uint8 queuing_high_bandwidth;

	/** Hardware Configuration -- stored here for convenience.*/
	hwcfg1_data_t hwcfg1;
	hwcfg2_data_t hwcfg2;
	hwcfg3_data_t hwcfg3;
	hwcfg4_data_t hwcfg4;

	/** Host and Device Configuration -- stored here for convenience.*/
	hcfg_data_t hcfg;

	/** The operational State, during transations
	 * (a_host>>a_peripherial and b_device=>b_host) this may not
	 * match the core but allows the software to determine
	 * transitions.
	 */
	uint8 op_state;

	/**
	 * Set to 1 if the HCD needs to be restarted on a session request
	 * interrupt. This is required if no connector ID status change has
	 * occurred since the HCD was last disconnected.
	 */
	uint8 restart_hcd_on_session_req;

	/** HCD callbacks */
	/** A-Device is a_host */
#define A_HOST		(1)
	/** A-Device is a_suspend */
#define A_SUSPEND	(2)
	/** A-Device is a_peripherial */
#define A_PERIPHERAL	(3)
	/** B-Device is operating as a Peripheral. */
#define B_PERIPHERAL	(4)
	/** B-Device is operating as a Host. */
#define B_HOST		(5)

	/** Device mode Periodic Tx FIFO Mask */
	uint32 p_tx_msk;
	/** Device mode Periodic Tx FIFO Mask */
	uint32 tx_msk;
};

typedef struct dwc_otg_core_if dwc_otg_core_if_t;

extern void dwc_otg_cil_init(dwc_otg_core_if_t *_core_if, const uint32 * _reg_base_addr);
extern void dwc_otg_core_init(dwc_otg_core_if_t * _core_if);
extern void dwc_otg_cil_remove(dwc_otg_core_if_t * _core_if);

extern void dwc_otg_enable_global_interrupts(dwc_otg_core_if_t * _core_if);
extern void dwc_otg_disable_global_interrupts(dwc_otg_core_if_t * _core_if);

extern uint8 dwc_otg_is_device_mode(dwc_otg_core_if_t * _core_if);
extern uint8 dwc_otg_is_host_mode(dwc_otg_core_if_t * _core_if);

extern uint8 dwc_otg_is_dma_enable(dwc_otg_core_if_t * core_if);

/** This function should be called on every hardware interrupt. */
extern int32 dwc_otg_handle_common_intr(dwc_otg_core_if_t * _core_if);

/* Register access functions */
void dwc_write_reg32(volatile uint32 *addr, uint32 value);
uint32 dwc_read_reg32(volatile uint32 *addr);

/** @name OTG Core Parameters */
/** @{ */

/**
 * Specifies the OTG capabilities. The driver will automatically
 * detect the value for this parameter if none is specified.
 * 0 - HNP and SRP capable (default)
 * 1 - SRP Only capable
 * 2 - No HNP/SRP capable
 */
extern int dwc_otg_set_param_otg_cap(dwc_otg_core_if_t * core_if, int32 val);
extern int32 dwc_otg_get_param_otg_cap(dwc_otg_core_if_t * core_if);
#define DWC_OTG_CAP_PARAM_HNP_SRP_CAPABLE    0
#define DWC_OTG_CAP_PARAM_SRP_ONLY_CAPABLE   1
#define DWC_OTG_CAP_PARAM_NO_HNP_SRP_CAPABLE 2
#define dwc_param_otg_cap_default            DWC_OTG_CAP_PARAM_HNP_SRP_CAPABLE

extern int dwc_otg_set_param_opt(dwc_otg_core_if_t * core_if, int32 val);
extern int32 dwc_otg_get_param_opt(dwc_otg_core_if_t * core_if);
#define dwc_param_opt_default 1

/**
 * Specifies whether to use slave or DMA mode for accessing the data
 * FIFOs. The driver will automatically detect the value for this
 * parameter if none is specified.
 * 0 - Slave
 * 1 - DMA (default, if                                     available)
 */
extern int dwc_otg_set_param_dma_enable(dwc_otg_core_if_t * core_if,
                                        int32               val);
extern int32 dwc_otg_get_param_dma_enable(dwc_otg_core_if_t * core_if);
#define dwc_param_dma_enable_default 1

/**
 * When DMA mode is enabled specifies whether to use
 * address DMA or DMA Descritor mode for accessing the data
 * FIFOs in device mode. The driver will automatically detect
 * the value for this parameter if none is specified.
 * 0 - address DMA
 * 1 - DMA Descriptor(default, if                                available)
 */
extern int dwc_otg_set_param_dma_desc_enable(dwc_otg_core_if_t * core_if,
                                             int32               val);
extern int32                                                     dwc_otg_get_param_dma_desc_enable(dwc_otg_core_if_t * core_if);
//#define dwc_param_dma_desc_enable_default 1
#define dwc_param_dma_desc_enable_default 0 // Broadcom BCM2708

/** The DMA Burst size (applicable only for External DMA
 * Mode). 1, 4, 8 16, 32, 64, 128, 256 (default 32)
 */
extern int dwc_otg_set_param_dma_burst_size(dwc_otg_core_if_t * core_if,
                                            int32               val);
extern int32                                                    dwc_otg_get_param_dma_burst_size(dwc_otg_core_if_t * core_if);
#define dwc_param_dma_burst_size_default 32

/**
 * Specifies the maximum speed of operation in host and device mode.
 * The actual speed depends on the speed of the attached device and
 * the value of phy_type. The actual speed depends on the speed of the
 * attached device.
 * 0 - High Speed (default)
 * 1 - Full Speed
 */
extern int   dwc_otg_set_param_speed(dwc_otg_core_if_t * core_if, int32 val);
extern int32 dwc_otg_get_param_speed(dwc_otg_core_if_t * core_if);
#define dwc_param_speed_default 0
#define DWC_SPEED_PARAM_HIGH    0
#define DWC_SPEED_PARAM_FULL    1

/** Specifies whether low power mode is supported when attached
 *	to a Full Speed or Low Speed device in host mode.
 * 0 - Don't support low power mode (default)
 * 1 - Support low power mode
 */
extern int dwc_otg_set_param_host_support_fs_ls_low_power(dwc_otg_core_if_t *
                                                          core_if, int32 val);
extern int32 dwc_otg_get_param_host_support_fs_ls_low_power(dwc_otg_core_if_t
                                                            *            core_if);
#define dwc_param_host_support_fs_ls_low_power_default 0

/** Specifies the PHY clock rate in low power mode when connected to a
 * Low Speed device in host mode. This parameter is applicable only if
 * HOST_SUPPORT_FS_LS_LOW_POWER is enabled. If PHY_TYPE is set to FS
 * then defaults to 6 MHZ otherwise 48 MHZ.
 *
 * 0 - 48 MHz
 * 1 - 6 MHz
 */
extern int dwc_otg_set_param_host_ls_low_power_phy_clk(dwc_otg_core_if_t *
                                                       core_if, int32 val);
extern int32 dwc_otg_get_param_host_ls_low_power_phy_clk(dwc_otg_core_if_t *
                                                         core_if);
#define dwc_param_host_ls_low_power_phy_clk_default 0
#define DWC_HOST_LS_LOW_POWER_PHY_CLK_PARAM_48MHZ   0
#define DWC_HOST_LS_LOW_POWER_PHY_CLK_PARAM_6MHZ    1

/**
 * 0 - Use cC FIFO size parameters
 * 1 - Allow dynamic FIFO sizing (default)
 */
extern int dwc_otg_set_param_enable_dynamic_fifo(dwc_otg_core_if_t * core_if,
                                                 int32               val);
extern int32 dwc_otg_get_param_enable_dynamic_fifo(dwc_otg_core_if_t *
                                                   core_if);
#define dwc_param_enable_dynamic_fifo_default 1

/** Total number of 4-byte words in the data FIFO memory. This
 * memory includes the Rx FIFO, non-periodic Tx FIFO, and periodic
 * Tx FIFOs.
 * 32 to 32768 (default 8192)
 * Note: The total FIFO memory depth in the FPGA configuration is 8192.
 */
extern int dwc_otg_set_param_data_fifo_size(dwc_otg_core_if_t * core_if,
                                            int32               val);
extern int32                                                    dwc_otg_get_param_data_fifo_size(dwc_otg_core_if_t * core_if);
//#define dwc_param_data_fifo_size_default 8192
#define dwc_param_data_fifo_size_default 0xFF0 // Broadcom BCM2708

/** Number of 4-byte words in the Rx FIFO in device mode when dynamic
 * FIFO sizing is enabled.
 * 16 to 32768 (default 1064)
 */
extern int dwc_otg_set_param_dev_rx_fifo_size(dwc_otg_core_if_t * core_if,
                                              int32               val);
extern int32                                                      dwc_otg_get_param_dev_rx_fifo_size(dwc_otg_core_if_t * core_if);
#define dwc_param_dev_rx_fifo_size_default 1064

/** Number of 4-byte words in the non-periodic Tx FIFO in device mode
 * when dynamic FIFO sizing is enabled.
 * 16 to 32768 (default 1024)
 */
extern int dwc_otg_set_param_dev_nperio_tx_fifo_size(dwc_otg_core_if_t *
                                                     core_if, int32 val);
extern int32 dwc_otg_get_param_dev_nperio_tx_fifo_size(dwc_otg_core_if_t *
                                                       core_if);
#define dwc_param_dev_nperio_tx_fifo_size_default 1024

/** Number of 4-byte words in each of the periodic Tx FIFOs in device
 * mode when dynamic FIFO sizing is enabled.
 * 4 to 768 (default 256)
 */
extern int dwc_otg_set_param_dev_perio_tx_fifo_size(dwc_otg_core_if_t * core_if,
                                                    int32          val, int fifo_num);
extern int32 dwc_otg_get_param_dev_perio_tx_fifo_size(dwc_otg_core_if_t *
                                                      core_if,          int fifo_num);
#define dwc_param_dev_perio_tx_fifo_size_default 256

/** Number of 4-byte words in the Rx FIFO in host mode when dynamic
 * FIFO sizing is enabled.
 * 16 to 32768 (default 1024)
 */
extern int dwc_otg_set_param_host_rx_fifo_size(dwc_otg_core_if_t * core_if,
                                               int32               val);
extern int32                                                       dwc_otg_get_param_host_rx_fifo_size(dwc_otg_core_if_t * core_if);
//#define dwc_param_host_rx_fifo_size_default 1024
#define dwc_param_host_rx_fifo_size_default 774 // Broadcom BCM2708

/** Number of 4-byte words in the non-periodic Tx FIFO in host mode
 * when Dynamic FIFO sizing is enabled in the core.
 * 16 to 32768 (default 1024)
 */
extern int dwc_otg_set_param_host_nperio_tx_fifo_size(dwc_otg_core_if_t *
                                                      core_if, int32 val);
extern int32 dwc_otg_get_param_host_nperio_tx_fifo_size(dwc_otg_core_if_t *
                                                        core_if);
//#define dwc_param_host_nperio_tx_fifo_size_default 1024
#define dwc_param_host_nperio_tx_fifo_size_default 0x100 // Broadcom BCM2708

/** Number of 4-byte words in the host periodic Tx FIFO when dynamic
 * FIFO sizing is enabled.
 * 16 to 32768 (default 1024)
 */
extern int dwc_otg_set_param_host_perio_tx_fifo_size(dwc_otg_core_if_t *
                                                     core_if, int32 val);
extern int32 dwc_otg_get_param_host_perio_tx_fifo_size(dwc_otg_core_if_t *
                                                       core_if);
//#define dwc_param_host_perio_tx_fifo_size_default 1024
#define dwc_param_host_perio_tx_fifo_size_default 0x200 // Broadcom BCM2708

/** The maximum transfer size supported in bytes.
 * 2047 to 65, 535  (default 65, 535)
 */
extern int dwc_otg_set_param_max_transfer_size(dwc_otg_core_if_t * core_if,
                                               int32               val);
extern int32                                                       dwc_otg_get_param_max_transfer_size(dwc_otg_core_if_t * core_if);
#define dwc_param_max_transfer_size_default 65535

/** The maximum number of packets in a transfer.
 * 15 to 511  (default 511)
 */
extern int dwc_otg_set_param_max_packet_count(dwc_otg_core_if_t * core_if,
                                              int32               val);
extern int32                                                      dwc_otg_get_param_max_packet_count(dwc_otg_core_if_t * core_if);
#define dwc_param_max_packet_count_default 511

/** The number of host channel registers to use.
 * 1 to 16 (default 12)
 * Note: The FPGA configuration supports a maximum of 12 host channels.
 */
extern int dwc_otg_set_param_host_channels(dwc_otg_core_if_t * core_if,
                                           int32               val);
extern int32                                                   dwc_otg_get_param_host_channels(dwc_otg_core_if_t * core_if);
#define dwc_param_host_channels_default 12

/** The number of endpoints in addition to EP0 available for device
 * mode operations.
 * 1 to 15 (default 6 IN and                                   OUT)
 * Note: The FPGA configuration supports a maximum of 6 IN and OUT
 * endpoints in addition to EP0.
 */
extern int dwc_otg_set_param_dev_endpoints(dwc_otg_core_if_t * core_if,
                                           int32               val);
extern int32                                                   dwc_otg_get_param_dev_endpoints(dwc_otg_core_if_t * core_if);
#define dwc_param_dev_endpoints_default 6

/**
 * Specifies the type of PHY interface to use. By default, the driver
 * will automatically detect the phy_type.
 *
 * 0 - Full Speed PHY
 * 1 - UTMI+ (default)
 * 2 - ULPI
 */
extern int   dwc_otg_set_param_phy_type(dwc_otg_core_if_t * core_if, int32 val);
extern int32 dwc_otg_get_param_phy_type(dwc_otg_core_if_t * core_if);
#define DWC_PHY_TYPE_PARAM_FS      0
#define DWC_PHY_TYPE_PARAM_UTMI    1
#define DWC_PHY_TYPE_PARAM_ULPI    2
#define dwc_param_phy_type_default DWC_PHY_TYPE_PARAM_UTMI

/**
 * Specifies the UTMI+ Data Width.	This parameter is
 * applicable for a PHY_TYPE of UTMI+ or ULPI. (For a ULPI
 *                                                              PHY_TYPE, this parameter indicates the data width between
 * the MAC and the ULPI Wrapper.) Also,                                   this parameter is
 * applicable only if the OTG_HSPHY_WIDTH cC parameter was set
 * to "8 and 16 bits",                                                    meaning that the core has been
 * configured to work at either data path width.
 *
 * 8 or 16 bits (default 16)
 */
extern int dwc_otg_set_param_phy_utmi_width(dwc_otg_core_if_t * core_if,
                                            int32               val);
extern int32                                                    dwc_otg_get_param_phy_utmi_width(dwc_otg_core_if_t * core_if);
//#define dwc_param_phy_utmi_width_default 16
#define dwc_param_phy_utmi_width_default 8 // Broadcom BCM2708

/**
 * Specifies whether the ULPI operates at double or single
 * data rate. This parameter is only applicable if PHY_TYPE is
 * ULPI.
 *
 * 0 - single data rate ULPI interface with 8 bit wide data
 * bus (default)
 * 1 - double data rate ULPI interface with 4 bit wide data
 * bus
 */
extern int dwc_otg_set_param_phy_ulpi_ddr(dwc_otg_core_if_t * core_if,
                                          int32               val);
extern int32 dwc_otg_get_param_phy_ulpi_ddr(dwc_otg_core_if_t * core_if);
#define dwc_param_phy_ulpi_ddr_default 0

/**
 * Specifies whether to use the internal or external supply to
 * drive the vbus with a ULPI phy.
 */
extern int dwc_otg_set_param_phy_ulpi_ext_vbus(dwc_otg_core_if_t * core_if,
                                               int32 val);
extern int32 dwc_otg_get_param_phy_ulpi_ext_vbus(dwc_otg_core_if_t * core_if);
#define DWC_PHY_ULPI_INTERNAL_VBUS          0
#define DWC_PHY_ULPI_EXTERNAL_VBUS          1
#define dwc_param_phy_ulpi_ext_vbus_default DWC_PHY_ULPI_INTERNAL_VBUS

/**
 * Specifies whether to use the I2Cinterface for full speed PHY. This
 * parameter is only applicable if PHY_TYPE is FS.
 * 0 - No (default)
 * 1 - Yes
 */
extern int dwc_otg_set_param_i2c_enable(dwc_otg_core_if_t * core_if,
                                        int32               val);
extern int32                                                dwc_otg_get_param_i2c_enable(dwc_otg_core_if_t * core_if);
#define dwc_param_i2c_enable_default 0

extern int dwc_otg_set_param_ulpi_fs_ls(dwc_otg_core_if_t * core_if,
                                        int32 val);
extern int32                                  dwc_otg_get_param_ulpi_fs_ls(dwc_otg_core_if_t * core_if);
#define dwc_param_ulpi_fs_ls_default 0

extern int dwc_otg_set_param_ts_dline(dwc_otg_core_if_t * core_if, int32 val);
extern int32 dwc_otg_get_param_ts_dline(dwc_otg_core_if_t * core_if);
#define dwc_param_ts_dline_default 0

/**
 * Specifies whether dedicated transmit FIFOs are
 * enabled for non periodic IN endpoints in device mode
 * 0 - No
 * 1 - Yes
 */
extern int dwc_otg_set_param_en_multiple_tx_fifo(dwc_otg_core_if_t * core_if,
                                                 int32               val);
extern int32 dwc_otg_get_param_en_multiple_tx_fifo(dwc_otg_core_if_t *
                                                   core_if);
#define dwc_param_en_multiple_tx_fifo_default 1

/** Number of 4-byte words in each of the Tx FIFOs in device
 * mode when dynamic FIFO sizing is enabled.
 * 4 to 768 (default 256)
 */
extern int dwc_otg_set_param_dev_tx_fifo_size(dwc_otg_core_if_t *   core_if,
                                              int                   fifo_num, int32 val);
extern int32 dwc_otg_get_param_dev_tx_fifo_size(dwc_otg_core_if_t * core_if,
                                                int fifo_num);
#define dwc_param_dev_tx_fifo_size_default 256

/** Thresholding enable flag-
 * bit 0 - enable non-ISO Tx thresholding
 * bit 1 - enable ISO Tx thresholding
 * bit 2 - enable Rx thresholding
 */
extern int   dwc_otg_set_param_thr_ctl(dwc_otg_core_if_t * core_if, int32 val);
extern int32 dwc_otg_get_thr_ctl(dwc_otg_core_if_t * core_if,       int fifo_num);
#define dwc_param_thr_ctl_default 0

/** Thresholding length for Tx
 * FIFOs in 32 bit DWORDs
 */
extern int dwc_otg_set_param_tx_thr_length(dwc_otg_core_if_t * core_if,
                                           int32               val);
extern int32                                                   dwc_otg_get_tx_thr_length(dwc_otg_core_if_t * core_if);
#define dwc_param_tx_thr_length_default 64

/** Thresholding length for Rx
 *	FIFOs in 32 bit DWORDs
 */
extern int dwc_otg_set_param_rx_thr_length(dwc_otg_core_if_t * core_if,
                                           int32               val);
extern int32                                                   dwc_otg_get_rx_thr_length(dwc_otg_core_if_t * core_if);
#define dwc_param_rx_thr_length_default 64

/**
 * Specifies whether LPM (Link Power Management) support is enabled
 */
extern int dwc_otg_set_param_lpm_enable(dwc_otg_core_if_t * core_if,
                                        int32 val);
extern int32 dwc_otg_get_param_lpm_enable(dwc_otg_core_if_t * core_if);
#define dwc_param_lpm_enable_default 0

/**
 * Specifies whether PTI enhancement is enabled
 */
extern int dwc_otg_set_param_pti_enable(dwc_otg_core_if_t * core_if,
                                        int32 val);
extern int32                                                dwc_otg_get_param_pti_enable(dwc_otg_core_if_t * core_if);
#define dwc_param_pti_enable_default 0

/**
 * Specifies whether MPI enhancement is enabled
 */
extern int dwc_otg_set_param_mpi_enable(dwc_otg_core_if_t * core_if,
                                        int32 val);
extern int32                                  dwc_otg_get_param_mpi_enable(dwc_otg_core_if_t * core_if);
#define dwc_param_mpi_enable_default 0

/**
 * Specifies whether IC_USB capability is enabled
 */
extern int dwc_otg_set_param_ic_usb_cap(dwc_otg_core_if_t * core_if,
                                        int32 val);
extern int32                                                dwc_otg_get_param_ic_usb_cap(dwc_otg_core_if_t * core_if);
#define dwc_param_ic_usb_cap_default 0

extern int dwc_otg_set_param_ahb_thr_ratio(dwc_otg_core_if_t * core_if, int32 val);
extern int32 dwc_otg_get_param_ahb_thr_ratio(dwc_otg_core_if_t * core_if);
#define dwc_param_ahb_thr_ratio_default 0

/** @} */

/** @name Access to registers and bit-fields */

/**
 * Dump core registers and SPRAM
 */
extern void dwc_otg_dump_dev_registers(dwc_otg_core_if_t * _core_if);
extern void dwc_otg_dump_spram(dwc_otg_core_if_t * _core_if);
extern void dwc_otg_dump_host_registers(dwc_otg_core_if_t * _core_if);
extern void dwc_otg_dump_global_registers(dwc_otg_core_if_t * _core_if);

/**
 * Get host negotiation status.
 */
extern uint32 dwc_otg_get_hnpstatus(dwc_otg_core_if_t * core_if);

/**
 * Get srp status
 */
extern uint32 dwc_otg_get_srpstatus(dwc_otg_core_if_t * core_if);

/**
 * Set hnpreq bit in the GOTGCTL register.
 */
extern void dwc_otg_set_hnpreq(dwc_otg_core_if_t * core_if, uint32 val);

/**
 * Get Content of SNPSID register.
 */
extern uint32 dwc_otg_get_gsnpsid(dwc_otg_core_if_t * core_if);

/**
 * Get current mode.
 * Returns 0 if in device mode, and 1 if in host mode.
 */
extern uint32             dwc_otg_get_mode(dwc_otg_core_if_t * core_if);

/**
 * Get value of hnpcapable field in the GUSBCFG register
 */
extern uint32 dwc_otg_get_hnpcapable(dwc_otg_core_if_t * core_if);
/**
 * Set value of hnpcapable field in the GUSBCFG register
 */
extern void   dwc_otg_set_hnpcapable(dwc_otg_core_if_t * core_if, uint32 val);

/**
 * Get value of srpcapable field in the GUSBCFG register
 */
extern uint32 dwc_otg_get_srpcapable(dwc_otg_core_if_t * core_if);
/**
 * Set value of srpcapable field in the GUSBCFG register
 */
extern void   dwc_otg_set_srpcapable(dwc_otg_core_if_t * core_if, uint32 val);

/**
 * Get value of devspeed field in the DCFG register
 */
extern uint32 dwc_otg_get_devspeed(dwc_otg_core_if_t * core_if);
/**
 * Set value of devspeed field in the DCFG register
 */
extern void   dwc_otg_set_devspeed(dwc_otg_core_if_t * core_if, uint32 val);

/**
 * Get the value of busconnected field from the HPRT0 register
 */
extern uint32 dwc_otg_get_busconnected(dwc_otg_core_if_t * core_if);

/**
 * Gets the device enumeration Speed.
 */
extern uint32 dwc_otg_get_enumspeed(dwc_otg_core_if_t * core_if);

/**
 * Get value of prtpwr field from the HPRT0 register
 */
extern uint32 dwc_otg_get_prtpower(dwc_otg_core_if_t * core_if);
/**
 * Set value of prtpwr field from the HPRT0 register
 */
extern void   dwc_otg_set_prtpower(dwc_otg_core_if_t * core_if, uint32 val);

/**
 * Get value of prtsusp field from the HPRT0 regsiter
 */
extern uint32 dwc_otg_get_prtsuspend(dwc_otg_core_if_t * core_if);
/**
 * Set value of prtpwr field from the HPRT0 register
 */
extern void dwc_otg_set_prtsuspend(dwc_otg_core_if_t * core_if, uint32 val);

/**
 * Set value of prtres field from the HPRT0 register
 *FIXME Remove?
 */
extern void dwc_otg_set_prtresume(dwc_otg_core_if_t * core_if, uint32 val);

/**
 * Get value of rmtwkupsig bit in DCTL register
 */
extern uint32 dwc_otg_get_remotewakesig(dwc_otg_core_if_t * core_if);

/**
 * Get value of prt_sleep_sts field from the GLPMCFG register
 */
extern uint32 dwc_otg_get_lpm_portsleepstatus(dwc_otg_core_if_t * core_if);

/**
 * Get value of rem_wkup_en field from the GLPMCFG register
 */
extern uint32 dwc_otg_get_lpm_remotewakeenabled(dwc_otg_core_if_t * core_if);

/**
 * Get value of appl_resp field from the GLPMCFG register
 */
extern uint32 dwc_otg_get_lpmresponse(dwc_otg_core_if_t * core_if);
/**
 * Set value of appl_resp field from the GLPMCFG register
 */
extern void dwc_otg_set_lpmresponse(dwc_otg_core_if_t * core_if, uint32 val);

/**
 * Get value of hsic_connect field from the GLPMCFG register
 */
extern uint32 dwc_otg_get_hsic_connect(dwc_otg_core_if_t * core_if);
/**
 * Set value of hsic_connect field from the GLPMCFG register
 */
extern void dwc_otg_set_hsic_connect(dwc_otg_core_if_t * core_if, uint32 val);

/**
 * Get value of inv_sel_hsic field from the GLPMCFG register.
 */
extern uint32 dwc_otg_get_inv_sel_hsic(dwc_otg_core_if_t * core_if);
/**
 * Set value of inv_sel_hsic field from the GLPMFG register.
 */
extern void dwc_otg_set_inv_sel_hsic(dwc_otg_core_if_t * core_if, uint32 val);

/*
 * Some functions for accessing registers
 */

/**
 *  GOTGCTL register
 */
extern uint32 dwc_otg_get_gotgctl(dwc_otg_core_if_t * core_if);
extern void dwc_otg_set_gotgctl(dwc_otg_core_if_t * core_if, uint32 val);

/**
 * GUSBCFG register
 */
extern uint32 dwc_otg_get_gusbcfg(dwc_otg_core_if_t * core_if);
extern void   dwc_otg_set_gusbcfg(dwc_otg_core_if_t * core_if, uint32 val);

/**
 * GRXFSIZ register
 */
extern uint32 dwc_otg_get_grxfsiz(dwc_otg_core_if_t * core_if);
extern void   dwc_otg_set_grxfsiz(dwc_otg_core_if_t * core_if, uint32 val);

/**
 * GNPTXFSIZ register
 */
extern uint32 dwc_otg_get_gnptxfsiz(dwc_otg_core_if_t * core_if);
extern void   dwc_otg_set_gnptxfsiz(dwc_otg_core_if_t * core_if, uint32 val);

extern uint32 dwc_otg_get_gpvndctl(dwc_otg_core_if_t * core_if);
extern void   dwc_otg_set_gpvndctl(dwc_otg_core_if_t * core_if, uint32 val);

/**
 * GGPIO register
 */
extern uint32 dwc_otg_get_ggpio(dwc_otg_core_if_t * core_if);
extern void   dwc_otg_set_ggpio(dwc_otg_core_if_t * core_if, uint32 val);

/**
 * GUID register
 */
extern uint32 dwc_otg_get_guid(dwc_otg_core_if_t * core_if);
extern void   dwc_otg_set_guid(dwc_otg_core_if_t * core_if, uint32 val);

/**
 * HPRT0 register
 */
extern uint32 dwc_otg_get_hprt0(dwc_otg_core_if_t * core_if);
extern void   dwc_otg_set_hprt0(dwc_otg_core_if_t * core_if, uint32 val);
extern uint32 dwc_otg_read_hprt0(dwc_otg_core_if_t * _core_if);

/**
 * GHPTXFSIZE
 */
extern uint32 dwc_otg_get_hptxfsiz(dwc_otg_core_if_t * core_if);

/**
 * Init channel @hc_num with provided parameters
 */
extern void dwc_otg_hc_init(dwc_otg_core_if_t * core_if,  uint8 hc_num,
                            uint8               dev_addr, uint8 ep_num, uint8 ep_is_in,
                            uint8               ep_type,  uint16 max_packet);


/**
 * Host controller initialization part
 */
extern void dwc_otg_core_host_init(dwc_otg_core_if_t * core_if);

/** @} */

#endif				/* __DWC_CORE_IF_H__ */
