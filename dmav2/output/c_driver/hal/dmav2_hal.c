/**
 * @file dmav2_hal.c
 * @brief DMAv2 Hardware Abstraction Layer (HAL) implementation
 *
 * This module implements low-level register access functions for the DMAv2 device.
 * All register operations go through the interface layer for communication
 * with the Python device model.
 */

#include "dmav2_hal.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Global base address storage
 */
uint32_t g_dmav2_base_address = 0;
bool g_dmav2_base_address_initialized = false;

/**
 * @brief HAL initialization status
 */
static bool g_dmav2_hal_initialized = false;

/**
 * @brief Channel register offset lookup table
 */
static const uint32_t channel_status_offsets[DMAV2_MAX_CHANNELS] = {
    DMAV2_REG_CHANNEL0_STATUS, DMAV2_REG_CHANNEL1_STATUS, DMAV2_REG_CHANNEL2_STATUS, DMAV2_REG_CHANNEL3_STATUS,
    DMAV2_REG_CHANNEL4_STATUS, DMAV2_REG_CHANNEL5_STATUS, DMAV2_REG_CHANNEL6_STATUS, DMAV2_REG_CHANNEL7_STATUS,
    DMAV2_REG_CHANNEL8_STATUS, DMAV2_REG_CHANNEL9_STATUS, DMAV2_REG_CHANNEL10_STATUS, DMAV2_REG_CHANNEL11_STATUS,
    DMAV2_REG_CHANNEL12_STATUS, DMAV2_REG_CHANNEL13_STATUS, DMAV2_REG_CHANNEL14_STATUS, DMAV2_REG_CHANNEL15_STATUS
};

static const uint32_t channel_enable_offsets[DMAV2_MAX_CHANNELS] = {
    DMAV2_REG_CHANNEL0_CHAN_ENABLE, DMAV2_REG_CHANNEL1_CHAN_ENABLE, DMAV2_REG_CHANNEL2_CHAN_ENABLE, DMAV2_REG_CHANNEL3_CHAN_ENABLE,
    DMAV2_REG_CHANNEL4_CHAN_ENABLE, DMAV2_REG_CHANNEL5_CHAN_ENABLE, DMAV2_REG_CHANNEL6_CHAN_ENABLE, DMAV2_REG_CHANNEL7_CHAN_ENABLE,
    DMAV2_REG_CHANNEL8_CHAN_ENABLE, DMAV2_REG_CHANNEL9_CHAN_ENABLE, DMAV2_REG_CHANNEL10_CHAN_ENABLE, DMAV2_REG_CHANNEL11_CHAN_ENABLE,
    DMAV2_REG_CHANNEL12_CHAN_ENABLE, DMAV2_REG_CHANNEL13_CHAN_ENABLE, DMAV2_REG_CHANNEL14_CHAN_ENABLE, DMAV2_REG_CHANNEL15_CHAN_ENABLE
};

static const uint32_t channel_data_trans_num_offsets[DMAV2_MAX_CHANNELS] = {
    DMAV2_REG_CHANNEL0_DATA_TRANS_NUM, DMAV2_REG_CHANNEL1_DATA_TRANS_NUM, DMAV2_REG_CHANNEL2_DATA_TRANS_NUM, DMAV2_REG_CHANNEL3_DATA_TRANS_NUM,
    DMAV2_REG_CHANNEL4_DATA_TRANS_NUM, DMAV2_REG_CHANNEL5_DATA_TRANS_NUM, DMAV2_REG_CHANNEL6_DATA_TRANS_NUM, DMAV2_REG_CHANNEL7_DATA_TRANS_NUM,
    DMAV2_REG_CHANNEL8_DATA_TRANS_NUM, DMAV2_REG_CHANNEL9_DATA_TRANS_NUM, DMAV2_REG_CHANNEL10_DATA_TRANS_NUM, DMAV2_REG_CHANNEL11_DATA_TRANS_NUM,
    DMAV2_REG_CHANNEL12_DATA_TRANS_NUM, DMAV2_REG_CHANNEL13_DATA_TRANS_NUM, DMAV2_REG_CHANNEL14_DATA_TRANS_NUM, DMAV2_REG_CHANNEL15_DATA_TRANS_NUM
};

static const uint32_t channel_inter_fifo_data_left_num_offsets[DMAV2_MAX_CHANNELS] = {
    DMAV2_REG_CHANNEL0_INTER_FIFO_DATA_LEFT_NUM, DMAV2_REG_CHANNEL1_INTER_FIFO_DATA_LEFT_NUM, DMAV2_REG_CHANNEL2_INTER_FIFO_DATA_LEFT_NUM, DMAV2_REG_CHANNEL3_INTER_FIFO_DATA_LEFT_NUM,
    DMAV2_REG_CHANNEL4_INTER_FIFO_DATA_LEFT_NUM, DMAV2_REG_CHANNEL5_INTER_FIFO_DATA_LEFT_NUM, DMAV2_REG_CHANNEL6_INTER_FIFO_DATA_LEFT_NUM, DMAV2_REG_CHANNEL7_INTER_FIFO_DATA_LEFT_NUM,
    DMAV2_REG_CHANNEL8_INTER_FIFO_DATA_LEFT_NUM, DMAV2_REG_CHANNEL9_INTER_FIFO_DATA_LEFT_NUM, DMAV2_REG_CHANNEL10_INTER_FIFO_DATA_LEFT_NUM, DMAV2_REG_CHANNEL11_INTER_FIFO_DATA_LEFT_NUM,
    DMAV2_REG_CHANNEL12_INTER_FIFO_DATA_LEFT_NUM, DMAV2_REG_CHANNEL13_INTER_FIFO_DATA_LEFT_NUM, DMAV2_REG_CHANNEL14_INTER_FIFO_DATA_LEFT_NUM, DMAV2_REG_CHANNEL15_INTER_FIFO_DATA_LEFT_NUM
};

static const uint32_t channel_dend_addr_offsets[DMAV2_MAX_CHANNELS] = {
    DMAV2_REG_CHANNEL0_DEND_ADDR, DMAV2_REG_CHANNEL1_DEND_ADDR, DMAV2_REG_CHANNEL2_DEND_ADDR, DMAV2_REG_CHANNEL3_DEND_ADDR,
    DMAV2_REG_CHANNEL4_DEND_ADDR, DMAV2_REG_CHANNEL5_DEND_ADDR, DMAV2_REG_CHANNEL6_DEND_ADDR, DMAV2_REG_CHANNEL7_DEND_ADDR,
    DMAV2_REG_CHANNEL8_DEND_ADDR, DMAV2_REG_CHANNEL9_DEND_ADDR, DMAV2_REG_CHANNEL10_DEND_ADDR, DMAV2_REG_CHANNEL11_DEND_ADDR,
    DMAV2_REG_CHANNEL12_DEND_ADDR, DMAV2_REG_CHANNEL13_DEND_ADDR, DMAV2_REG_CHANNEL14_DEND_ADDR, DMAV2_REG_CHANNEL15_DEND_ADDR
};

static const uint32_t channel_addr_offset_offsets[DMAV2_MAX_CHANNELS] = {
    DMAV2_REG_CHANNEL0_ADDR_OFFSET, DMAV2_REG_CHANNEL1_ADDR_OFFSET, DMAV2_REG_CHANNEL2_ADDR_OFFSET, DMAV2_REG_CHANNEL3_ADDR_OFFSET,
    DMAV2_REG_CHANNEL4_ADDR_OFFSET, DMAV2_REG_CHANNEL5_ADDR_OFFSET, DMAV2_REG_CHANNEL6_ADDR_OFFSET, DMAV2_REG_CHANNEL7_ADDR_OFFSET,
    DMAV2_REG_CHANNEL8_ADDR_OFFSET, DMAV2_REG_CHANNEL9_ADDR_OFFSET, DMAV2_REG_CHANNEL10_ADDR_OFFSET, DMAV2_REG_CHANNEL11_ADDR_OFFSET,
    DMAV2_REG_CHANNEL12_ADDR_OFFSET, DMAV2_REG_CHANNEL13_ADDR_OFFSET, DMAV2_REG_CHANNEL14_ADDR_OFFSET, DMAV2_REG_CHANNEL15_ADDR_OFFSET
};

static const uint32_t channel_dmamux_cfg_offsets[DMAV2_MAX_CHANNELS] = {
    DMAV2_REG_CHANNEL0_DMAMUX_CFG, DMAV2_REG_CHANNEL1_DMAMUX_CFG, DMAV2_REG_CHANNEL2_DMAMUX_CFG, DMAV2_REG_CHANNEL3_DMAMUX_CFG,
    DMAV2_REG_CHANNEL4_DMAMUX_CFG, DMAV2_REG_CHANNEL5_DMAMUX_CFG, DMAV2_REG_CHANNEL6_DMAMUX_CFG, DMAV2_REG_CHANNEL7_DMAMUX_CFG,
    DMAV2_REG_CHANNEL8_DMAMUX_CFG, DMAV2_REG_CHANNEL9_DMAMUX_CFG, DMAV2_REG_CHANNEL10_DMAMUX_CFG, DMAV2_REG_CHANNEL11_DMAMUX_CFG,
    DMAV2_REG_CHANNEL12_DMAMUX_CFG, DMAV2_REG_CHANNEL13_DMAMUX_CFG, DMAV2_REG_CHANNEL14_DMAMUX_CFG, DMAV2_REG_CHANNEL15_DMAMUX_CFG
};

static const uint32_t channel_dstart_addr_offsets[DMAV2_MAX_CHANNELS] = {
    DMAV2_REG_CHANNEL0_DSTART_ADDR, DMAV2_REG_CHANNEL1_DSTART_ADDR, DMAV2_REG_CHANNEL2_DSTART_ADDR, DMAV2_REG_CHANNEL3_DSTART_ADDR,
    DMAV2_REG_CHANNEL4_DSTART_ADDR, DMAV2_REG_CHANNEL5_DSTART_ADDR, DMAV2_REG_CHANNEL6_DSTART_ADDR, DMAV2_REG_CHANNEL7_DSTART_ADDR,
    DMAV2_REG_CHANNEL8_DSTART_ADDR, DMAV2_REG_CHANNEL9_DSTART_ADDR, DMAV2_REG_CHANNEL10_DSTART_ADDR, DMAV2_REG_CHANNEL11_DSTART_ADDR,
    DMAV2_REG_CHANNEL12_DSTART_ADDR, DMAV2_REG_CHANNEL13_DSTART_ADDR, DMAV2_REG_CHANNEL14_DSTART_ADDR, DMAV2_REG_CHANNEL15_DSTART_ADDR
};

/**
 * @brief Initialize HAL base address
 */
void dmav2_hal_base_address_init(uint32_t base_addr)
{
    g_dmav2_base_address = base_addr;
    g_dmav2_base_address_initialized = true;
}

/**
 * @brief Initialize DMAv2 HAL
 */
dmav2_hal_status_t dmav2_hal_init(void)
{
    if (!g_dmav2_base_address_initialized) {
        return DMAV2_HAL_NOT_INITIALIZED;
    }
    
    g_dmav2_hal_initialized = true;
    return DMAV2_HAL_OK;
}

/**
 * @brief Deinitialize DMAv2 HAL
 */
dmav2_hal_status_t dmav2_hal_deinit(void)
{
    g_dmav2_hal_initialized = false;
    return DMAV2_HAL_OK;
}

/**
 * @brief Raw register write operation
 */
dmav2_hal_status_t dmav2_hal_write_raw(uint32_t offset, uint32_t value)
{
    if (!g_dmav2_hal_initialized || !g_dmav2_base_address_initialized) {
        return DMAV2_HAL_NOT_INITIALIZED;
    }
    
    uint32_t address = g_dmav2_base_address + offset;
    DMAV2_WRITE_REGISTER(address, value);
    
    return DMAV2_HAL_OK;
}

/**
 * @brief Raw register read operation
 */
dmav2_hal_status_t dmav2_hal_read_raw(uint32_t offset, uint32_t *value)
{
    if (!g_dmav2_hal_initialized || !g_dmav2_base_address_initialized) {
        return DMAV2_HAL_NOT_INITIALIZED;
    }
    
    if (value == NULL) {
        return DMAV2_HAL_INVALID_PARAM;
    }
    
    uint32_t address = g_dmav2_base_address + offset;
    *value = DMAV2_READ_REGISTER(address);
    
    return DMAV2_HAL_OK;
}

/**
 * @brief Write to DMA_TOP_RST register
 */
dmav2_hal_status_t dmav2_hal_write_top_rst(uint32_t value)
{
    return dmav2_hal_write_raw(DMAV2_REG_DMA_TOP_RST, value);
}

/**
 * @brief Read from DMA_TOP_RST register
 */
dmav2_hal_status_t dmav2_hal_read_top_rst(uint32_t *value)
{
    return dmav2_hal_read_raw(DMAV2_REG_DMA_TOP_RST, value);
}

/**
 * @brief Check if channel number is valid
 */
bool dmav2_hal_is_channel_valid(uint8_t channel)
{
    return (channel < DMAV2_MAX_CHANNELS);
}

/**
 * @brief Write to channel enable register
 */
dmav2_hal_status_t dmav2_hal_write_channel_enable(uint8_t channel, uint32_t value)
{
    if (!dmav2_hal_is_channel_valid(channel)) {
        return DMAV2_HAL_INVALID_PARAM;
    }
    
    return dmav2_hal_write_raw(channel_enable_offsets[channel], value);
}

/**
 * @brief Read from channel enable register
 */
dmav2_hal_status_t dmav2_hal_read_channel_enable(uint8_t channel, uint32_t *value)
{
    if (!dmav2_hal_is_channel_valid(channel)) {
        return DMAV2_HAL_INVALID_PARAM;
    }
    
    return dmav2_hal_read_raw(channel_enable_offsets[channel], value);
}

/**
 * @brief Read from channel status register
 */
dmav2_hal_status_t dmav2_hal_read_channel_status(uint8_t channel, uint32_t *value)
{
    if (!dmav2_hal_is_channel_valid(channel)) {
        return DMAV2_HAL_INVALID_PARAM;
    }
    
    return dmav2_hal_read_raw(channel_status_offsets[channel], value);
}

/**
 * @brief Read from channel data transfer number register
 */
dmav2_hal_status_t dmav2_hal_read_channel_data_trans_num(uint8_t channel, uint32_t *value)
{
    if (!dmav2_hal_is_channel_valid(channel)) {
        return DMAV2_HAL_INVALID_PARAM;
    }
    
    return dmav2_hal_read_raw(channel_data_trans_num_offsets[channel], value);
}

/**
 * @brief Read from channel inter FIFO data left number register
 */
dmav2_hal_status_t dmav2_hal_read_channel_inter_fifo_data_left_num(uint8_t channel, uint32_t *value)
{
    if (!dmav2_hal_is_channel_valid(channel)) {
        return DMAV2_HAL_INVALID_PARAM;
    }
    
    return dmav2_hal_read_raw(channel_inter_fifo_data_left_num_offsets[channel], value);
}

/**
 * @brief Write to channel destination end address register
 */
dmav2_hal_status_t dmav2_hal_write_channel_dend_addr(uint8_t channel, uint32_t value)
{
    if (!dmav2_hal_is_channel_valid(channel)) {
        return DMAV2_HAL_INVALID_PARAM;
    }
    
    return dmav2_hal_write_raw(channel_dend_addr_offsets[channel], value);
}

/**
 * @brief Read from channel destination end address register
 */
dmav2_hal_status_t dmav2_hal_read_channel_dend_addr(uint8_t channel, uint32_t *value)
{
    if (!dmav2_hal_is_channel_valid(channel)) {
        return DMAV2_HAL_INVALID_PARAM;
    }
    
    return dmav2_hal_read_raw(channel_dend_addr_offsets[channel], value);
}

/**
 * @brief Write to channel address offset register
 */
dmav2_hal_status_t dmav2_hal_write_channel_addr_offset(uint8_t channel, uint32_t value)
{
    if (!dmav2_hal_is_channel_valid(channel)) {
        return DMAV2_HAL_INVALID_PARAM;
    }
    
    return dmav2_hal_write_raw(channel_addr_offset_offsets[channel], value);
}

/**
 * @brief Read from channel address offset register
 */
dmav2_hal_status_t dmav2_hal_read_channel_addr_offset(uint8_t channel, uint32_t *value)
{
    if (!dmav2_hal_is_channel_valid(channel)) {
        return DMAV2_HAL_INVALID_PARAM;
    }
    
    return dmav2_hal_read_raw(channel_addr_offset_offsets[channel], value);
}

/**
 * @brief Write to channel DMAMUX configuration register
 */
dmav2_hal_status_t dmav2_hal_write_channel_dmamux_cfg(uint8_t channel, uint32_t value)
{
    if (!dmav2_hal_is_channel_valid(channel)) {
        return DMAV2_HAL_INVALID_PARAM;
    }
    
    return dmav2_hal_write_raw(channel_dmamux_cfg_offsets[channel], value);
}

/**
 * @brief Read from channel DMAMUX configuration register
 */
dmav2_hal_status_t dmav2_hal_read_channel_dmamux_cfg(uint8_t channel, uint32_t *value)
{
    if (!dmav2_hal_is_channel_valid(channel)) {
        return DMAV2_HAL_INVALID_PARAM;
    }
    
    return dmav2_hal_read_raw(channel_dmamux_cfg_offsets[channel], value);
}

/**
 * @brief Write to channel destination start address register
 */
dmav2_hal_status_t dmav2_hal_write_channel_dstart_addr(uint8_t channel, uint32_t value)
{
    if (!dmav2_hal_is_channel_valid(channel)) {
        return DMAV2_HAL_INVALID_PARAM;
    }
    
    return dmav2_hal_write_raw(channel_dstart_addr_offsets[channel], value);
}

/**
 * @brief Read from channel destination start address register
 */
dmav2_hal_status_t dmav2_hal_read_channel_dstart_addr(uint8_t channel, uint32_t *value)
{
    if (!dmav2_hal_is_channel_valid(channel)) {
        return DMAV2_HAL_INVALID_PARAM;
    }
    
    return dmav2_hal_read_raw(channel_dstart_addr_offsets[channel], value);
}

/**
 * @brief Get channel register offset with base offset
 */
dmav2_hal_status_t dmav2_hal_get_channel_register_offset(uint8_t channel, uint32_t base_offset, uint32_t *offset)
{
    if (!dmav2_hal_is_channel_valid(channel) || offset == NULL) {
        return DMAV2_HAL_INVALID_PARAM;
    }
    
    *offset = base_offset + (channel * 0x40);
    return DMAV2_HAL_OK;
}