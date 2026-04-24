/**
 * @file dma_hal.h
 * @brief DMA Hardware Abstraction Layer (HAL) interface definitions
 *
 * This module provides low-level register access functions for the DMA device.
 * All register operations go through the interface layer for communication
 * with the Python device model.
 */

#ifndef DMA_HAL_H
#define DMA_HAL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DMA HAL status codes
 */
typedef enum {
    DMA_HAL_OK = 0,                 /**< Operation successful */
    DMA_HAL_ERROR,                  /**< General error */
    DMA_HAL_INVALID_PARAM,          /**< Invalid parameter */
    DMA_HAL_NOT_INITIALIZED,        /**< HAL not initialized */
    DMA_HAL_HARDWARE_ERROR          /**< Hardware error */
} dma_hal_status_t;

/**
 * @brief DMA channel numbers (16 channels: 0-15)
 */
#define DMA_MAX_CHANNELS            16

/**
 * @brief DMA register offsets
 */
#define DMA_REG_TOP_RST             0x000

/* Channel status registers (read-only) */
#define DMA_REG_CHANNEL0_STATUS     0x040
#define DMA_REG_CHANNEL1_STATUS     0x080
#define DMA_REG_CHANNEL2_STATUS     0x0C0
#define DMA_REG_CHANNEL3_STATUS     0x100
#define DMA_REG_CHANNEL4_STATUS     0x140
#define DMA_REG_CHANNEL5_STATUS     0x180
#define DMA_REG_CHANNEL6_STATUS     0x1C0
#define DMA_REG_CHANNEL7_STATUS     0x200
#define DMA_REG_CHANNEL8_STATUS     0x240
#define DMA_REG_CHANNEL9_STATUS     0x280
#define DMA_REG_CHANNEL10_STATUS    0x2C0
#define DMA_REG_CHANNEL11_STATUS    0x300
#define DMA_REG_CHANNEL12_STATUS    0x340
#define DMA_REG_CHANNEL13_STATUS    0x380
#define DMA_REG_CHANNEL14_STATUS    0x3C0
#define DMA_REG_CHANNEL15_STATUS    0x400

/* Channel configuration registers */
#define DMA_REG_CHANNEL0_CONFIG     0x050
#define DMA_REG_CHANNEL1_CONFIG     0x090
#define DMA_REG_CHANNEL2_CONFIG     0x0D0
#define DMA_REG_CHANNEL3_CONFIG     0x110
#define DMA_REG_CHANNEL4_CONFIG     0x150
#define DMA_REG_CHANNEL5_CONFIG     0x190
#define DMA_REG_CHANNEL6_CONFIG     0x1D0
#define DMA_REG_CHANNEL7_CONFIG     0x210
#define DMA_REG_CHANNEL8_CONFIG     0x250
#define DMA_REG_CHANNEL9_CONFIG     0x290
#define DMA_REG_CHANNEL10_CONFIG    0x2D0
#define DMA_REG_CHANNEL11_CONFIG    0x310
#define DMA_REG_CHANNEL12_CONFIG    0x350
#define DMA_REG_CHANNEL13_CONFIG    0x390
#define DMA_REG_CHANNEL14_CONFIG    0x3D0
#define DMA_REG_CHANNEL15_CONFIG    0x410

/* Channel length registers */
#define DMA_REG_CHANNEL0_LENGTH     0x054
#define DMA_REG_CHANNEL1_LENGTH     0x094
#define DMA_REG_CHANNEL2_LENGTH     0x0D4
#define DMA_REG_CHANNEL3_LENGTH     0x114
#define DMA_REG_CHANNEL4_LENGTH     0x154
#define DMA_REG_CHANNEL5_LENGTH     0x194
#define DMA_REG_CHANNEL6_LENGTH     0x1D4
#define DMA_REG_CHANNEL7_LENGTH     0x214
#define DMA_REG_CHANNEL8_LENGTH     0x254
#define DMA_REG_CHANNEL9_LENGTH     0x294
#define DMA_REG_CHANNEL10_LENGTH    0x2D4
#define DMA_REG_CHANNEL11_LENGTH    0x314
#define DMA_REG_CHANNEL12_LENGTH    0x354
#define DMA_REG_CHANNEL13_LENGTH    0x394
#define DMA_REG_CHANNEL14_LENGTH    0x3D4
#define DMA_REG_CHANNEL15_LENGTH    0x414

/* Channel source address registers */
#define DMA_REG_CHANNEL0_SADDR      0x058
#define DMA_REG_CHANNEL1_SADDR      0x098
#define DMA_REG_CHANNEL2_SADDR      0x0D8
#define DMA_REG_CHANNEL3_SADDR      0x118
#define DMA_REG_CHANNEL4_SADDR      0x158
#define DMA_REG_CHANNEL5_SADDR      0x198
#define DMA_REG_CHANNEL6_SADDR      0x1D8
#define DMA_REG_CHANNEL7_SADDR      0x218
#define DMA_REG_CHANNEL8_SADDR      0x258
#define DMA_REG_CHANNEL9_SADDR      0x298
#define DMA_REG_CHANNEL10_SADDR     0x2D8
#define DMA_REG_CHANNEL11_SADDR     0x318
#define DMA_REG_CHANNEL12_SADDR     0x358
#define DMA_REG_CHANNEL13_SADDR     0x398
#define DMA_REG_CHANNEL14_SADDR     0x3D8
#define DMA_REG_CHANNEL15_SADDR     0x418

/* Channel destination address registers */
#define DMA_REG_CHANNEL0_DADDR      0x060
#define DMA_REG_CHANNEL1_DADDR      0x0A0
#define DMA_REG_CHANNEL2_DADDR      0x0E0
#define DMA_REG_CHANNEL3_DADDR      0x120
#define DMA_REG_CHANNEL4_DADDR      0x160
#define DMA_REG_CHANNEL5_DADDR      0x1A0
#define DMA_REG_CHANNEL6_DADDR      0x1E0
#define DMA_REG_CHANNEL7_DADDR      0x220
#define DMA_REG_CHANNEL8_DADDR      0x260
#define DMA_REG_CHANNEL9_DADDR      0x2A0
#define DMA_REG_CHANNEL10_DADDR     0x2E0
#define DMA_REG_CHANNEL11_DADDR     0x320
#define DMA_REG_CHANNEL12_DADDR     0x360
#define DMA_REG_CHANNEL13_DADDR     0x3A0
#define DMA_REG_CHANNEL14_DADDR     0x3E0
#define DMA_REG_CHANNEL15_DADDR     0x420

/* Channel enable registers */
#define DMA_REG_CHANNEL0_ENABLE     0x064
#define DMA_REG_CHANNEL1_ENABLE     0x0A4
#define DMA_REG_CHANNEL2_ENABLE     0x0E4
#define DMA_REG_CHANNEL3_ENABLE     0x124
#define DMA_REG_CHANNEL4_ENABLE     0x164
#define DMA_REG_CHANNEL5_ENABLE     0x1A4
#define DMA_REG_CHANNEL6_ENABLE     0x1E4
#define DMA_REG_CHANNEL7_ENABLE     0x224
#define DMA_REG_CHANNEL8_ENABLE     0x264
#define DMA_REG_CHANNEL9_ENABLE     0x2A4
#define DMA_REG_CHANNEL10_ENABLE    0x2E4
#define DMA_REG_CHANNEL11_ENABLE    0x324
#define DMA_REG_CHANNEL12_ENABLE    0x364
#define DMA_REG_CHANNEL13_ENABLE    0x3A4
#define DMA_REG_CHANNEL14_ENABLE    0x3E4
#define DMA_REG_CHANNEL15_ENABLE    0x424

/* Channel data transfer number registers */
#define DMA_REG_CHANNEL0_DATA_TRANS_NUM     0x068
#define DMA_REG_CHANNEL1_DATA_TRANS_NUM     0x0A8
#define DMA_REG_CHANNEL2_DATA_TRANS_NUM     0x0E8
#define DMA_REG_CHANNEL3_DATA_TRANS_NUM     0x128
#define DMA_REG_CHANNEL4_DATA_TRANS_NUM     0x168
#define DMA_REG_CHANNEL5_DATA_TRANS_NUM     0x1A8
#define DMA_REG_CHANNEL6_DATA_TRANS_NUM     0x1E8
#define DMA_REG_CHANNEL7_DATA_TRANS_NUM     0x228
#define DMA_REG_CHANNEL8_DATA_TRANS_NUM     0x268
#define DMA_REG_CHANNEL9_DATA_TRANS_NUM     0x2A8
#define DMA_REG_CHANNEL10_DATA_TRANS_NUM    0x2E8
#define DMA_REG_CHANNEL11_DATA_TRANS_NUM    0x328
#define DMA_REG_CHANNEL12_DATA_TRANS_NUM    0x368
#define DMA_REG_CHANNEL13_DATA_TRANS_NUM    0x3A8
#define DMA_REG_CHANNEL14_DATA_TRANS_NUM    0x3E8
#define DMA_REG_CHANNEL15_DATA_TRANS_NUM    0x428

/* Channel FIFO data left number registers */
#define DMA_REG_CHANNEL0_FIFO_LEFT_NUM      0x06C
#define DMA_REG_CHANNEL1_FIFO_LEFT_NUM      0x0AC
#define DMA_REG_CHANNEL2_FIFO_LEFT_NUM      0x0EC
#define DMA_REG_CHANNEL3_FIFO_LEFT_NUM      0x12C
#define DMA_REG_CHANNEL4_FIFO_LEFT_NUM      0x16C
#define DMA_REG_CHANNEL5_FIFO_LEFT_NUM      0x1AC
#define DMA_REG_CHANNEL6_FIFO_LEFT_NUM      0x1EC
#define DMA_REG_CHANNEL7_FIFO_LEFT_NUM      0x22C
#define DMA_REG_CHANNEL8_FIFO_LEFT_NUM      0x26C
#define DMA_REG_CHANNEL9_FIFO_LEFT_NUM      0x2AC
#define DMA_REG_CHANNEL10_FIFO_LEFT_NUM     0x2EC
#define DMA_REG_CHANNEL11_FIFO_LEFT_NUM     0x32C
#define DMA_REG_CHANNEL12_FIFO_LEFT_NUM     0x36C
#define DMA_REG_CHANNEL13_FIFO_LEFT_NUM     0x3AC
#define DMA_REG_CHANNEL14_FIFO_LEFT_NUM     0x3EC
#define DMA_REG_CHANNEL15_FIFO_LEFT_NUM     0x42C

/* Channel destination end address registers */
#define DMA_REG_CHANNEL0_DEND_ADDR      0x070
#define DMA_REG_CHANNEL1_DEND_ADDR      0x0B0
#define DMA_REG_CHANNEL2_DEND_ADDR      0x0F0
#define DMA_REG_CHANNEL3_DEND_ADDR      0x130
#define DMA_REG_CHANNEL4_DEND_ADDR      0x170
#define DMA_REG_CHANNEL5_DEND_ADDR      0x1B0
#define DMA_REG_CHANNEL6_DEND_ADDR      0x1F0
#define DMA_REG_CHANNEL7_DEND_ADDR      0x230
#define DMA_REG_CHANNEL8_DEND_ADDR      0x270
#define DMA_REG_CHANNEL9_DEND_ADDR      0x2B0
#define DMA_REG_CHANNEL10_DEND_ADDR     0x2F0
#define DMA_REG_CHANNEL11_DEND_ADDR     0x330
#define DMA_REG_CHANNEL12_DEND_ADDR     0x370
#define DMA_REG_CHANNEL13_DEND_ADDR     0x3B0
#define DMA_REG_CHANNEL14_DEND_ADDR     0x3F0
#define DMA_REG_CHANNEL15_DEND_ADDR     0x430

/* Channel address offset registers */
#define DMA_REG_CHANNEL0_ADDR_OFFSET    0x074
#define DMA_REG_CHANNEL1_ADDR_OFFSET    0x0B4
#define DMA_REG_CHANNEL2_ADDR_OFFSET    0x0F4
#define DMA_REG_CHANNEL3_ADDR_OFFSET    0x134
#define DMA_REG_CHANNEL4_ADDR_OFFSET    0x174
#define DMA_REG_CHANNEL5_ADDR_OFFSET    0x1B4
#define DMA_REG_CHANNEL6_ADDR_OFFSET    0x1F4
#define DMA_REG_CHANNEL7_ADDR_OFFSET    0x234
#define DMA_REG_CHANNEL8_ADDR_OFFSET    0x274
#define DMA_REG_CHANNEL9_ADDR_OFFSET    0x2B4
#define DMA_REG_CHANNEL10_ADDR_OFFSET   0x2F4
#define DMA_REG_CHANNEL11_ADDR_OFFSET   0x334
#define DMA_REG_CHANNEL12_ADDR_OFFSET   0x374
#define DMA_REG_CHANNEL13_ADDR_OFFSET   0x3B4
#define DMA_REG_CHANNEL14_ADDR_OFFSET   0x3F4
#define DMA_REG_CHANNEL15_ADDR_OFFSET   0x434

/* Channel DMAMUX configuration registers */
#define DMA_REG_CHANNEL0_DMAMUX_CFG     0x078
#define DMA_REG_CHANNEL1_DMAMUX_CFG     0x0B8
#define DMA_REG_CHANNEL2_DMAMUX_CFG     0x0F8
#define DMA_REG_CHANNEL3_DMAMUX_CFG     0x138
#define DMA_REG_CHANNEL4_DMAMUX_CFG     0x178
#define DMA_REG_CHANNEL5_DMAMUX_CFG     0x1B8
#define DMA_REG_CHANNEL6_DMAMUX_CFG     0x1F8
#define DMA_REG_CHANNEL7_DMAMUX_CFG     0x238
#define DMA_REG_CHANNEL8_DMAMUX_CFG     0x278
#define DMA_REG_CHANNEL9_DMAMUX_CFG     0x2B8
#define DMA_REG_CHANNEL10_DMAMUX_CFG    0x2F8
#define DMA_REG_CHANNEL11_DMAMUX_CFG    0x338
#define DMA_REG_CHANNEL12_DMAMUX_CFG    0x378
#define DMA_REG_CHANNEL13_DMAMUX_CFG    0x3B8
#define DMA_REG_CHANNEL14_DMAMUX_CFG    0x3F8
#define DMA_REG_CHANNEL15_DMAMUX_CFG    0x438

/* Channel destination start address registers */
#define DMA_REG_CHANNEL0_DSTART_ADDR    0x05C
#define DMA_REG_CHANNEL1_DSTART_ADDR    0x09C
#define DMA_REG_CHANNEL2_DSTART_ADDR    0x0DC
#define DMA_REG_CHANNEL3_DSTART_ADDR    0x11C
#define DMA_REG_CHANNEL4_DSTART_ADDR    0x15C
#define DMA_REG_CHANNEL5_DSTART_ADDR    0x19C
#define DMA_REG_CHANNEL6_DSTART_ADDR    0x1DC
#define DMA_REG_CHANNEL7_DSTART_ADDR    0x21C
#define DMA_REG_CHANNEL8_DSTART_ADDR    0x25C
#define DMA_REG_CHANNEL9_DSTART_ADDR    0x29C
#define DMA_REG_CHANNEL10_DSTART_ADDR   0x2DC
#define DMA_REG_CHANNEL11_DSTART_ADDR   0x31C
#define DMA_REG_CHANNEL12_DSTART_ADDR   0x35C
#define DMA_REG_CHANNEL13_DSTART_ADDR   0x39C
#define DMA_REG_CHANNEL14_DSTART_ADDR   0x3DC
#define DMA_REG_CHANNEL15_DSTART_ADDR   0x41C

/**
 * @brief DMA register structure definition
 */
struct DMARegister {
    uint32_t DMA_TOP_RST;                   /**< Global DMA top reset control */
    uint32_t CHANNEL_STATUS[16];            /**< Channel status registers */
    uint32_t CHANNEL_CONFIG[16];            /**< Channel configuration registers */
    uint32_t CHANNEL_LENGTH[16];            /**< Channel transfer length registers */
    uint32_t CHANNEL_SADDR[16];             /**< Channel source address registers */
    uint32_t CHANNEL_DADDR[16];             /**< Channel destination address registers */
    uint32_t CHANNEL_ENABLE[16];            /**< Channel enable registers */
    uint32_t CHANNEL_DATA_TRANS_NUM[16];    /**< Channel data transfer number registers */
    uint32_t CHANNEL_FIFO_LEFT_NUM[16];     /**< Channel FIFO data left number registers */
    uint32_t CHANNEL_DEND_ADDR[16];         /**< Channel destination end address registers */
    uint32_t CHANNEL_ADDR_OFFSET[16];       /**< Channel address offset registers */
    uint32_t CHANNEL_DMAMUX_CFG[16];        /**< Channel DMAMUX configuration registers */
    uint32_t CHANNEL_DSTART_ADDR[16];       /**< Channel destination start address registers */
};

/**
 * @brief Register access macros
 */
#ifdef REAL_CHIP
/* read / write memory directly */
#define WRITE_REGISTER(addr, value)  *(volatile uint32_t *)(addr) = (value)
#define READ_REGISTER(addr)          *(volatile uint32_t *)(addr)
#else
/* read / write memory by libdrvintf.a which could help you to communicate with python model */
#include <interface_layer.h>
#define WRITE_REGISTER(addr, value)  (void)write_register(addr, value, 4)
#define READ_REGISTER(addr)          read_register(addr, 4)
#endif

/**
 * @brief Initialize HAL with base address
 * @param base_address Register base address for DMA device
 * @return HAL status code
 */
dma_hal_status_t dma_hal_base_address_init(uint32_t base_address);

/**
 * @brief Initialize DMA HAL (requires base address already set)
 * @return HAL status code
 */
dma_hal_status_t dma_hal_init(void);

/**
 * @brief Deinitialize DMA HAL
 * @return HAL status code
 */
dma_hal_status_t dma_hal_deinit(void);

/**
 * @brief Raw register read using base + offset
 * @param offset Register offset from base address
 * @param value Pointer to store read value
 * @return HAL status code
 */
dma_hal_status_t dma_hal_read_raw(uint32_t offset, uint32_t *value);

/**
 * @brief Raw register write using base + offset
 * @param offset Register offset from base address
 * @param value Value to write
 * @return HAL status code
 */
dma_hal_status_t dma_hal_write_raw(uint32_t offset, uint32_t value);

/**
 * @brief Direct register read with absolute address
 * @param address Absolute register address
 * @param value Pointer to store read value
 * @return HAL status code
 */
dma_hal_status_t dma_hal_read_register_direct(uint32_t address, uint32_t *value);

/**
 * @brief Direct register write with absolute address
 * @param address Absolute register address
 * @param value Value to write
 * @return HAL status code
 */
dma_hal_status_t dma_hal_write_register_direct(uint32_t address, uint32_t value);

/**
 * @brief Read DMA top reset register
 * @param value Pointer to store reset value
 * @return HAL status code
 */
dma_hal_status_t dma_hal_read_top_reset(uint32_t *value);

/**
 * @brief Write DMA top reset register
 * @param value Reset value to write
 * @return HAL status code
 */
dma_hal_status_t dma_hal_write_top_reset(uint32_t value);

/**
 * @brief Read channel status register
 * @param channel Channel number (0-15)
 * @param status Pointer to store status value
 * @return HAL status code
 */
dma_hal_status_t dma_hal_read_channel_status(uint32_t channel, uint32_t *status);

/**
 * @brief Read channel configuration register
 * @param channel Channel number (0-15)
 * @param config Pointer to store configuration value
 * @return HAL status code
 */
dma_hal_status_t dma_hal_read_channel_config(uint32_t channel, uint32_t *config);

/**
 * @brief Write channel configuration register
 * @param channel Channel number (0-15)
 * @param config Configuration value to write
 * @return HAL status code
 */
dma_hal_status_t dma_hal_write_channel_config(uint32_t channel, uint32_t config);

/**
 * @brief Read channel length register
 * @param channel Channel number (0-15)
 * @param length Pointer to store length value
 * @return HAL status code
 */
dma_hal_status_t dma_hal_read_channel_length(uint32_t channel, uint32_t *length);

/**
 * @brief Write channel length register
 * @param channel Channel number (0-15)
 * @param length Length value to write
 * @return HAL status code
 */
dma_hal_status_t dma_hal_write_channel_length(uint32_t channel, uint32_t length);

/**
 * @brief Read channel source address register
 * @param channel Channel number (0-15)
 * @param saddr Pointer to store source address value
 * @return HAL status code
 */
dma_hal_status_t dma_hal_read_channel_saddr(uint32_t channel, uint32_t *saddr);

/**
 * @brief Write channel source address register
 * @param channel Channel number (0-15)
 * @param saddr Source address value to write
 * @return HAL status code
 */
dma_hal_status_t dma_hal_write_channel_saddr(uint32_t channel, uint32_t saddr);

/**
 * @brief Read channel destination address register
 * @param channel Channel number (0-15)
 * @param daddr Pointer to store destination address value
 * @return HAL status code
 */
dma_hal_status_t dma_hal_read_channel_daddr(uint32_t channel, uint32_t *daddr);

/**
 * @brief Write channel destination address register
 * @param channel Channel number (0-15)
 * @param daddr Destination address value to write
 * @return HAL status code
 */
dma_hal_status_t dma_hal_write_channel_daddr(uint32_t channel, uint32_t daddr);

/**
 * @brief Read channel enable register
 * @param channel Channel number (0-15)
 * @param enable Pointer to store enable value
 * @return HAL status code
 */
dma_hal_status_t dma_hal_read_channel_enable(uint32_t channel, uint32_t *enable);

/**
 * @brief Write channel enable register
 * @param channel Channel number (0-15)
 * @param enable Enable value to write
 * @return HAL status code
 */
dma_hal_status_t dma_hal_write_channel_enable(uint32_t channel, uint32_t enable);

/**
 * @brief Read channel data transfer number register
 * @param channel Channel number (0-15)
 * @param trans_num Pointer to store data transfer number value
 * @return HAL status code
 */
dma_hal_status_t dma_hal_read_channel_data_trans_num(uint32_t channel, uint32_t *trans_num);

/**
 * @brief Read channel FIFO data left number register
 * @param channel Channel number (0-15)
 * @param fifo_left Pointer to store FIFO data left number value
 * @return HAL status code
 */
dma_hal_status_t dma_hal_read_channel_fifo_left_num(uint32_t channel, uint32_t *fifo_left);

/**
 * @brief Read channel destination end address register
 * @param channel Channel number (0-15)
 * @param dend_addr Pointer to store destination end address value
 * @return HAL status code
 */
dma_hal_status_t dma_hal_read_channel_dend_addr(uint32_t channel, uint32_t *dend_addr);

/**
 * @brief Write channel destination end address register
 * @param channel Channel number (0-15)
 * @param dend_addr Destination end address value to write
 * @return HAL status code
 */
dma_hal_status_t dma_hal_write_channel_dend_addr(uint32_t channel, uint32_t dend_addr);

/**
 * @brief Read channel address offset register
 * @param channel Channel number (0-15)
 * @param addr_offset Pointer to store address offset value
 * @return HAL status code
 */
dma_hal_status_t dma_hal_read_channel_addr_offset(uint32_t channel, uint32_t *addr_offset);

/**
 * @brief Write channel address offset register
 * @param channel Channel number (0-15)
 * @param addr_offset Address offset value to write
 * @return HAL status code
 */
dma_hal_status_t dma_hal_write_channel_addr_offset(uint32_t channel, uint32_t addr_offset);

/**
 * @brief Read channel DMAMUX configuration register
 * @param channel Channel number (0-15)
 * @param dmamux_cfg Pointer to store DMAMUX configuration value
 * @return HAL status code
 */
dma_hal_status_t dma_hal_read_channel_dmamux_cfg(uint32_t channel, uint32_t *dmamux_cfg);

/**
 * @brief Write channel DMAMUX configuration register
 * @param channel Channel number (0-15)
 * @param dmamux_cfg DMAMUX configuration value to write
 * @return HAL status code
 */
dma_hal_status_t dma_hal_write_channel_dmamux_cfg(uint32_t channel, uint32_t dmamux_cfg);

/**
 * @brief Read channel destination start address register
 * @param channel Channel number (0-15)
 * @param dstart_addr Pointer to store destination start address value
 * @return HAL status code
 */
dma_hal_status_t dma_hal_read_channel_dstart_addr(uint32_t channel, uint32_t *dstart_addr);

/**
 * @brief Write channel destination start address register
 * @param channel Channel number (0-15)
 * @param dstart_addr Destination start address value to write
 * @return HAL status code
 */
dma_hal_status_t dma_hal_write_channel_dstart_addr(uint32_t channel, uint32_t dstart_addr);

#ifdef __cplusplus
}
#endif

#endif /* DMA_HAL_H */