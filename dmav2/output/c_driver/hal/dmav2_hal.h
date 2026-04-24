/**
 * @file dmav2_hal.h
 * @brief DMAv2 Hardware Abstraction Layer (HAL) interface definitions
 *
 * This module provides low-level register access functions for the DMAv2 device.
 * All register operations go through the interface layer for communication
 * with the Python device model.
 * 
 * Based on dmav2/input/dma_register_map.yaml specifications
 */

#ifndef DMAV2_HAL_H
#define DMAV2_HAL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DMAv2 HAL status codes
 */
typedef enum {
    DMAV2_HAL_OK = 0,               /**< Operation successful */
    DMAV2_HAL_ERROR,                /**< General error */
    DMAV2_HAL_INVALID_PARAM,        /**< Invalid parameter */
    DMAV2_HAL_NOT_INITIALIZED,      /**< HAL not initialized */
    DMAV2_HAL_HARDWARE_ERROR        /**< Hardware error */
} dmav2_hal_status_t;

/**
 * @brief DMAv2 channel numbers (16 channels: 0-15)
 */
#define DMAV2_MAX_CHANNELS          16

/**
 * @brief DMAv2 register offsets - Global registers
 */
#define DMAV2_REG_DMA_TOP_RST       0x000

/**
 * @brief DMAv2 channel register offsets
 * Each channel occupies 0x40 bytes address space
 */
#define DMAV2_CHANNEL_OFFSET(ch)    ((ch) * 0x40)

/* Channel Status registers - Read Only */
#define DMAV2_REG_CHANNEL0_STATUS   0x040
#define DMAV2_REG_CHANNEL1_STATUS   0x080
#define DMAV2_REG_CHANNEL2_STATUS   0x0C0
#define DMAV2_REG_CHANNEL3_STATUS   0x100
#define DMAV2_REG_CHANNEL4_STATUS   0x140
#define DMAV2_REG_CHANNEL5_STATUS   0x180
#define DMAV2_REG_CHANNEL6_STATUS   0x1C0
#define DMAV2_REG_CHANNEL7_STATUS   0x200
#define DMAV2_REG_CHANNEL8_STATUS   0x240
#define DMAV2_REG_CHANNEL9_STATUS   0x280
#define DMAV2_REG_CHANNEL10_STATUS  0x2C0
#define DMAV2_REG_CHANNEL11_STATUS  0x300
#define DMAV2_REG_CHANNEL12_STATUS  0x340
#define DMAV2_REG_CHANNEL13_STATUS  0x380
#define DMAV2_REG_CHANNEL14_STATUS  0x3C0
#define DMAV2_REG_CHANNEL15_STATUS  0x400

/* Channel Enable registers */
#define DMAV2_REG_CHANNEL0_CHAN_ENABLE      0x064
#define DMAV2_REG_CHANNEL1_CHAN_ENABLE      0x0A4
#define DMAV2_REG_CHANNEL2_CHAN_ENABLE      0x0E4
#define DMAV2_REG_CHANNEL3_CHAN_ENABLE      0x124
#define DMAV2_REG_CHANNEL4_CHAN_ENABLE      0x164
#define DMAV2_REG_CHANNEL5_CHAN_ENABLE      0x1A4
#define DMAV2_REG_CHANNEL6_CHAN_ENABLE      0x1E4
#define DMAV2_REG_CHANNEL7_CHAN_ENABLE      0x224
#define DMAV2_REG_CHANNEL8_CHAN_ENABLE      0x264
#define DMAV2_REG_CHANNEL9_CHAN_ENABLE      0x2A4
#define DMAV2_REG_CHANNEL10_CHAN_ENABLE     0x2E4
#define DMAV2_REG_CHANNEL11_CHAN_ENABLE     0x324
#define DMAV2_REG_CHANNEL12_CHAN_ENABLE     0x364
#define DMAV2_REG_CHANNEL13_CHAN_ENABLE     0x3A4
#define DMAV2_REG_CHANNEL14_CHAN_ENABLE     0x3E4
#define DMAV2_REG_CHANNEL15_CHAN_ENABLE     0x424

/* Data Transfer Number registers - Read Only */
#define DMAV2_REG_CHANNEL0_DATA_TRANS_NUM   0x068
#define DMAV2_REG_CHANNEL1_DATA_TRANS_NUM   0x0A8
#define DMAV2_REG_CHANNEL2_DATA_TRANS_NUM   0x0E8
#define DMAV2_REG_CHANNEL3_DATA_TRANS_NUM   0x128
#define DMAV2_REG_CHANNEL4_DATA_TRANS_NUM   0x168
#define DMAV2_REG_CHANNEL5_DATA_TRANS_NUM   0x1A8
#define DMAV2_REG_CHANNEL6_DATA_TRANS_NUM   0x1E8
#define DMAV2_REG_CHANNEL7_DATA_TRANS_NUM   0x228
#define DMAV2_REG_CHANNEL8_DATA_TRANS_NUM   0x268
#define DMAV2_REG_CHANNEL9_DATA_TRANS_NUM   0x2A8
#define DMAV2_REG_CHANNEL10_DATA_TRANS_NUM  0x2E8
#define DMAV2_REG_CHANNEL11_DATA_TRANS_NUM  0x328
#define DMAV2_REG_CHANNEL12_DATA_TRANS_NUM  0x368
#define DMAV2_REG_CHANNEL13_DATA_TRANS_NUM  0x3A8
#define DMAV2_REG_CHANNEL14_DATA_TRANS_NUM  0x3E8
#define DMAV2_REG_CHANNEL15_DATA_TRANS_NUM  0x428

/* Inter FIFO Data Left Number registers - Read Only */
#define DMAV2_REG_CHANNEL0_INTER_FIFO_DATA_LEFT_NUM   0x06C
#define DMAV2_REG_CHANNEL1_INTER_FIFO_DATA_LEFT_NUM   0x0AC
#define DMAV2_REG_CHANNEL2_INTER_FIFO_DATA_LEFT_NUM   0x0EC
#define DMAV2_REG_CHANNEL3_INTER_FIFO_DATA_LEFT_NUM   0x12C
#define DMAV2_REG_CHANNEL4_INTER_FIFO_DATA_LEFT_NUM   0x16C
#define DMAV2_REG_CHANNEL5_INTER_FIFO_DATA_LEFT_NUM   0x1AC
#define DMAV2_REG_CHANNEL6_INTER_FIFO_DATA_LEFT_NUM   0x1EC
#define DMAV2_REG_CHANNEL7_INTER_FIFO_DATA_LEFT_NUM   0x22C
#define DMAV2_REG_CHANNEL8_INTER_FIFO_DATA_LEFT_NUM   0x26C
#define DMAV2_REG_CHANNEL9_INTER_FIFO_DATA_LEFT_NUM   0x2AC
#define DMAV2_REG_CHANNEL10_INTER_FIFO_DATA_LEFT_NUM  0x2EC
#define DMAV2_REG_CHANNEL11_INTER_FIFO_DATA_LEFT_NUM  0x32C
#define DMAV2_REG_CHANNEL12_INTER_FIFO_DATA_LEFT_NUM  0x36C
#define DMAV2_REG_CHANNEL13_INTER_FIFO_DATA_LEFT_NUM  0x3AC
#define DMAV2_REG_CHANNEL14_INTER_FIFO_DATA_LEFT_NUM  0x3EC
#define DMAV2_REG_CHANNEL15_INTER_FIFO_DATA_LEFT_NUM  0x42C

/* Destination End Address registers */
#define DMAV2_REG_CHANNEL0_DEND_ADDR        0x070
#define DMAV2_REG_CHANNEL1_DEND_ADDR        0x0B0
#define DMAV2_REG_CHANNEL2_DEND_ADDR        0x0F0
#define DMAV2_REG_CHANNEL3_DEND_ADDR        0x130
#define DMAV2_REG_CHANNEL4_DEND_ADDR        0x170
#define DMAV2_REG_CHANNEL5_DEND_ADDR        0x1B0
#define DMAV2_REG_CHANNEL6_DEND_ADDR        0x1F0
#define DMAV2_REG_CHANNEL7_DEND_ADDR        0x230
#define DMAV2_REG_CHANNEL8_DEND_ADDR        0x270
#define DMAV2_REG_CHANNEL9_DEND_ADDR        0x2B0
#define DMAV2_REG_CHANNEL10_DEND_ADDR       0x2F0
#define DMAV2_REG_CHANNEL11_DEND_ADDR       0x330
#define DMAV2_REG_CHANNEL12_DEND_ADDR       0x370
#define DMAV2_REG_CHANNEL13_DEND_ADDR       0x3B0
#define DMAV2_REG_CHANNEL14_DEND_ADDR       0x3F0
#define DMAV2_REG_CHANNEL15_DEND_ADDR       0x430

/* Address Offset registers */
#define DMAV2_REG_CHANNEL0_ADDR_OFFSET      0x074
#define DMAV2_REG_CHANNEL1_ADDR_OFFSET      0x0B4
#define DMAV2_REG_CHANNEL2_ADDR_OFFSET      0x0F4
#define DMAV2_REG_CHANNEL3_ADDR_OFFSET      0x134
#define DMAV2_REG_CHANNEL4_ADDR_OFFSET      0x174
#define DMAV2_REG_CHANNEL5_ADDR_OFFSET      0x1B4
#define DMAV2_REG_CHANNEL6_ADDR_OFFSET      0x1F4
#define DMAV2_REG_CHANNEL7_ADDR_OFFSET      0x234
#define DMAV2_REG_CHANNEL8_ADDR_OFFSET      0x274
#define DMAV2_REG_CHANNEL9_ADDR_OFFSET      0x2B4
#define DMAV2_REG_CHANNEL10_ADDR_OFFSET     0x2F4
#define DMAV2_REG_CHANNEL11_ADDR_OFFSET     0x334
#define DMAV2_REG_CHANNEL12_ADDR_OFFSET     0x374
#define DMAV2_REG_CHANNEL13_ADDR_OFFSET     0x3B4
#define DMAV2_REG_CHANNEL14_ADDR_OFFSET     0x3F4
#define DMAV2_REG_CHANNEL15_ADDR_OFFSET     0x434

/* DMAMUX Configuration registers */
#define DMAV2_REG_CHANNEL0_DMAMUX_CFG       0x078
#define DMAV2_REG_CHANNEL1_DMAMUX_CFG       0x0B8
#define DMAV2_REG_CHANNEL2_DMAMUX_CFG       0x0F8
#define DMAV2_REG_CHANNEL3_DMAMUX_CFG       0x138
#define DMAV2_REG_CHANNEL4_DMAMUX_CFG       0x178
#define DMAV2_REG_CHANNEL5_DMAMUX_CFG       0x1B8
#define DMAV2_REG_CHANNEL6_DMAMUX_CFG       0x1F8
#define DMAV2_REG_CHANNEL7_DMAMUX_CFG       0x238
#define DMAV2_REG_CHANNEL8_DMAMUX_CFG       0x278
#define DMAV2_REG_CHANNEL9_DMAMUX_CFG       0x2B8
#define DMAV2_REG_CHANNEL10_DMAMUX_CFG      0x2F8
#define DMAV2_REG_CHANNEL11_DMAMUX_CFG      0x338
#define DMAV2_REG_CHANNEL12_DMAMUX_CFG      0x378
#define DMAV2_REG_CHANNEL13_DMAMUX_CFG      0x3B8
#define DMAV2_REG_CHANNEL14_DMAMUX_CFG      0x3F8
#define DMAV2_REG_CHANNEL15_DMAMUX_CFG      0x438

/* Destination Start Address registers */
#define DMAV2_REG_CHANNEL0_DSTART_ADDR      0x020
#define DMAV2_REG_CHANNEL1_DSTART_ADDR      0x060
#define DMAV2_REG_CHANNEL2_DSTART_ADDR      0x0A0
#define DMAV2_REG_CHANNEL3_DSTART_ADDR      0x0E0
#define DMAV2_REG_CHANNEL4_DSTART_ADDR      0x120
#define DMAV2_REG_CHANNEL5_DSTART_ADDR      0x160
#define DMAV2_REG_CHANNEL6_DSTART_ADDR      0x1A0
#define DMAV2_REG_CHANNEL7_DSTART_ADDR      0x1E0
#define DMAV2_REG_CHANNEL8_DSTART_ADDR      0x220
#define DMAV2_REG_CHANNEL9_DSTART_ADDR      0x260
#define DMAV2_REG_CHANNEL10_DSTART_ADDR     0x2A0
#define DMAV2_REG_CHANNEL11_DSTART_ADDR     0x2E0
#define DMAV2_REG_CHANNEL12_DSTART_ADDR     0x320
#define DMAV2_REG_CHANNEL13_DSTART_ADDR     0x360
#define DMAV2_REG_CHANNEL14_DSTART_ADDR     0x3A0
#define DMAV2_REG_CHANNEL15_DSTART_ADDR     0x3E0

/**
 * @brief DMAv2 register bit field definitions
 */

/* DMA_TOP_RST register bits */
#define DMAV2_TOP_RST_DMA_IDLE_MASK     (1U << 2)
#define DMAV2_TOP_RST_HARD_RST_MASK     (1U << 1)  
#define DMAV2_TOP_RST_WARM_RST_MASK     (1U << 0)

/* Channel STATUS register bits */
#define DMAV2_STATUS_CLE_MASK           (1U << 8)   /* Channel Length Error */
#define DMAV2_STATUS_SAE_MASK           (1U << 7)   /* Source Address Error */
#define DMAV2_STATUS_DAE_MASK           (1U << 6)   /* Destination Address Error */
#define DMAV2_STATUS_SOE_MASK           (1U << 5)   /* Source Offset Error */
#define DMAV2_STATUS_DOE_MASK           (1U << 4)   /* Destination Offset Error */
#define DMAV2_STATUS_SBE_MASK           (1U << 3)   /* Source Bus Error */
#define DMAV2_STATUS_DBE_MASK           (1U << 2)   /* Destination Bus Error */
#define DMAV2_STATUS_HALF_FINISH_MASK   (1U << 1)   /* Half Finish Flag */
#define DMAV2_STATUS_FINISH_MASK        (1U << 0)   /* Finish Flag */

/* Channel ENABLE register bits */
#define DMAV2_ENABLE_EDBG_MASK          (1U << 1)   /* Enable Debug */
#define DMAV2_ENABLE_CHAN_EN_MASK       (1U << 0)   /* Channel Enable */

/* DMAMUX_CFG register bits */
#define DMAV2_DMAMUX_TRIG_EN_MASK       (1U << 7)   /* Trigger Enable */
#define DMAV2_DMAMUX_REQ_ID_MASK        (0x7FU)     /* Request ID [6:0] */

/* ADDR_OFFSET register bit fields */
#define DMAV2_ADDR_OFFSET_DOFFSET_SHIFT  16
#define DMAV2_ADDR_OFFSET_DOFFSET_MASK   (0xFFFFU << 16)  /* Destination Offset [31:16] */
#define DMAV2_ADDR_OFFSET_SOFFSET_MASK   (0xFFFFU)        /* Source Offset [15:0] */

/**
 * @brief DMAv2 Register Structure Definition
 * Complete register map for DMAv2 device
 */
struct DMAv2Register {
    uint32_t DMA_TOP_RST;                          /* 0x000 */
    uint32_t reserved_0x004[15];                   /* 0x004-0x03C */
    
    /* Channel 0 registers */
    uint32_t CHANNEL0_STATUS;                      /* 0x040 */
    uint32_t reserved_0x044[8];                    /* 0x044-0x060 */
    uint32_t CHANNEL0_DSTART_ADDR;                 /* 0x020 */ 
    uint32_t CHANNEL0_CHAN_ENABLE;                 /* 0x064 */
    uint32_t CHANNEL0_DATA_TRANS_NUM;              /* 0x068 */
    uint32_t CHANNEL0_INTER_FIFO_DATA_LEFT_NUM;    /* 0x06C */
    uint32_t CHANNEL0_DEND_ADDR;                   /* 0x070 */
    uint32_t CHANNEL0_ADDR_OFFSET;                 /* 0x074 */
    uint32_t CHANNEL0_DMAMUX_CFG;                  /* 0x078 */
    uint32_t reserved_0x07C[1];                    /* 0x07C */
    
    /* Additional channels follow similar pattern... */
    /* Note: This is a simplified structure for reference */
};

/**
 * @brief Register access macros
 * Supports both real chip and simulator modes
 */
#ifdef REAL_CHIP
#define DMAV2_WRITE_REGISTER(addr, value)  *(volatile uint32_t *)(addr) = (value)
#define DMAV2_READ_REGISTER(addr)          *(volatile uint32_t *)(addr)
#else
#include "interface_layer.h"
extern uint32_t read_register(uint32_t address, uint32_t size);
extern int write_register(uint32_t address, uint32_t data, uint32_t size);
#define DMAV2_WRITE_REGISTER(addr, value)  (void)write_register(addr, value, 4)
#define DMAV2_READ_REGISTER(addr)          read_register(addr, 4)
#endif

/**
 * @brief Base address management
 */
extern uint32_t g_dmav2_base_address;
extern bool g_dmav2_base_address_initialized;

/**
 * @brief HAL initialization functions
 */
void dmav2_hal_base_address_init(uint32_t base_addr);
dmav2_hal_status_t dmav2_hal_init(void);
dmav2_hal_status_t dmav2_hal_deinit(void);

/**
 * @brief Raw register access functions
 */
dmav2_hal_status_t dmav2_hal_write_raw(uint32_t offset, uint32_t value);
dmav2_hal_status_t dmav2_hal_read_raw(uint32_t offset, uint32_t *value);

/**
 * @brief Global register access functions
 */
dmav2_hal_status_t dmav2_hal_write_top_rst(uint32_t value);
dmav2_hal_status_t dmav2_hal_read_top_rst(uint32_t *value);

/**
 * @brief Channel-specific register access functions
 */
dmav2_hal_status_t dmav2_hal_write_channel_enable(uint8_t channel, uint32_t value);
dmav2_hal_status_t dmav2_hal_read_channel_enable(uint8_t channel, uint32_t *value);
dmav2_hal_status_t dmav2_hal_read_channel_status(uint8_t channel, uint32_t *value);
dmav2_hal_status_t dmav2_hal_read_channel_data_trans_num(uint8_t channel, uint32_t *value);
dmav2_hal_status_t dmav2_hal_read_channel_inter_fifo_data_left_num(uint8_t channel, uint32_t *value);
dmav2_hal_status_t dmav2_hal_write_channel_dend_addr(uint8_t channel, uint32_t value);
dmav2_hal_status_t dmav2_hal_read_channel_dend_addr(uint8_t channel, uint32_t *value);
dmav2_hal_status_t dmav2_hal_write_channel_addr_offset(uint8_t channel, uint32_t value);
dmav2_hal_status_t dmav2_hal_read_channel_addr_offset(uint8_t channel, uint32_t *value);
dmav2_hal_status_t dmav2_hal_write_channel_dmamux_cfg(uint8_t channel, uint32_t value);
dmav2_hal_status_t dmav2_hal_read_channel_dmamux_cfg(uint8_t channel, uint32_t *value);
dmav2_hal_status_t dmav2_hal_write_channel_dstart_addr(uint8_t channel, uint32_t value);
dmav2_hal_status_t dmav2_hal_read_channel_dstart_addr(uint8_t channel, uint32_t *value);

/**
 * @brief Utility functions
 */
dmav2_hal_status_t dmav2_hal_get_channel_register_offset(uint8_t channel, uint32_t base_offset, uint32_t *offset);
bool dmav2_hal_is_channel_valid(uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif /* DMAV2_HAL_H */