/**
 * @file dma_hal.c
 * @brief DMA Hardware Abstraction Layer (HAL) Implementation
 *
 * This file implements the hardware abstraction layer for DMA device register access.
 * It provides low-level register read/write functions and basic hardware operations.
 */

#include "dma_hal.h"
#include <string.h>
#include <stdio.h>

#ifndef DMA_HAL_DEBUG
#define DMA_HAL_DEBUG 1
#endif
#if DMA_HAL_DEBUG
#define HAL_LOG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define HAL_LOG(fmt, ...) (void)0
#endif

#ifndef REAL_CHIP
/* Function declarations are in interface_layer.h */
#endif

/* =============================================================================
 * Private Variables
 * =============================================================================*/

/** Base address of DMA device */
static uint32_t g_dma_base_address = 0U;

/** HAL base address initialization status */
static bool g_dma_base_address_initialized = false;

/** HAL initialization status */
static bool g_dma_hal_initialized = false;

/* =============================================================================
 * Private Function Prototypes
 * =============================================================================*/

/**
 * @brief Validate channel parameter
 * @param channel Channel to validate
 * @return true if valid, false otherwise
 */
static bool dma_hal_validate_channel(uint32_t channel);

/**
 * @brief Get channel register offset
 * @param channel Channel number (0-15)
 * @param reg_type Register type offset base
 * @return Register offset for the channel
 */
static uint32_t dma_hal_get_channel_offset(uint32_t channel, uint32_t reg_type);

/* =============================================================================
 * Private Function Implementations
 * =============================================================================*/

static bool dma_hal_validate_channel(uint32_t channel)
{
    return (channel < DMA_MAX_CHANNELS);
}

static uint32_t dma_hal_get_channel_offset(uint32_t channel, uint32_t reg_type)
{
    if (!dma_hal_validate_channel(channel)) {
        return 0xFFFFFFFFU;
    }
    
    /* Each channel has a 0x40 byte spacing */
    return reg_type + (channel * 0x40);
}

/* =============================================================================
 * Public Function Implementations
 * =============================================================================*/

dma_hal_status_t dma_hal_base_address_init(uint32_t base_address)
{
    g_dma_base_address = base_address;
    g_dma_base_address_initialized = true;
    
    HAL_LOG("DMA HAL: Base address initialized to 0x%08X\n", base_address);
    return DMA_HAL_OK;
}

dma_hal_status_t dma_hal_init(void)
{
    if (!g_dma_base_address_initialized) {
        return DMA_HAL_ERROR;
    }
    
    g_dma_hal_initialized = true;
    HAL_LOG("DMA HAL: Initialized successfully\n");
    
    return DMA_HAL_OK;
}

dma_hal_status_t dma_hal_deinit(void)
{
    g_dma_hal_initialized = false;
    g_dma_base_address_initialized = false;
    g_dma_base_address = 0U;
    
    HAL_LOG("DMA HAL: Deinitialized\n");
    return DMA_HAL_OK;
}

dma_hal_status_t dma_hal_read_raw(uint32_t offset, uint32_t *value)
{
    if (!g_dma_hal_initialized) {
        return DMA_HAL_NOT_INITIALIZED;
    }
    
    if (value == NULL) {
        return DMA_HAL_INVALID_PARAM;
    }
    
    uint32_t address = g_dma_base_address + offset;
    HAL_LOG("DMA HAL READ: base=0x%08X offset=0x%08X addr=0x%08X\n", 
            g_dma_base_address, offset, address);
    *value = READ_REGISTER(address);
    
    return DMA_HAL_OK;
}

dma_hal_status_t dma_hal_write_raw(uint32_t offset, uint32_t value)
{
    if (!g_dma_hal_initialized) {
        return DMA_HAL_NOT_INITIALIZED;
    }
    
    uint32_t address = g_dma_base_address + offset;
    HAL_LOG("DMA HAL WRITE: base=0x%08X offset=0x%08X addr=0x%08X value=0x%08X\n", 
            g_dma_base_address, offset, address, value);
    WRITE_REGISTER(address, value);
    
    return DMA_HAL_OK;
}

dma_hal_status_t dma_hal_read_register_direct(uint32_t address, uint32_t *value)
{
    if (!g_dma_hal_initialized) {
        return DMA_HAL_NOT_INITIALIZED;
    }

    if (value == NULL) {
        return DMA_HAL_INVALID_PARAM;
    }

    /* Direct access expects absolute address */
    HAL_LOG("DMA HAL READ DIRECT: abs=0x%08X\n", address);
    *value = READ_REGISTER(address);

    return DMA_HAL_OK;
}

dma_hal_status_t dma_hal_write_register_direct(uint32_t address, uint32_t value)
{
    if (!g_dma_hal_initialized) {
        return DMA_HAL_NOT_INITIALIZED;
    }

    /* Direct access expects absolute address */
    HAL_LOG("DMA HAL WRITE DIRECT: abs=0x%08X value=0x%08X\n", address, value);
    WRITE_REGISTER(address, value);

    return DMA_HAL_OK;
}

dma_hal_status_t dma_hal_read_top_reset(uint32_t *value)
{
    return dma_hal_read_raw(DMA_REG_TOP_RST, value);
}

dma_hal_status_t dma_hal_write_top_reset(uint32_t value)
{
    return dma_hal_write_raw(DMA_REG_TOP_RST, value);
}

dma_hal_status_t dma_hal_read_channel_status(uint32_t channel, uint32_t *status)
{
    if (!dma_hal_validate_channel(channel)) {
        return DMA_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = dma_hal_get_channel_offset(channel, DMA_REG_CHANNEL0_STATUS);
    return dma_hal_read_raw(offset, status);
}

dma_hal_status_t dma_hal_read_channel_config(uint32_t channel, uint32_t *config)
{
    if (!dma_hal_validate_channel(channel)) {
        return DMA_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = dma_hal_get_channel_offset(channel, DMA_REG_CHANNEL0_CONFIG);
    return dma_hal_read_raw(offset, config);
}

dma_hal_status_t dma_hal_write_channel_config(uint32_t channel, uint32_t config)
{
    if (!dma_hal_validate_channel(channel)) {
        return DMA_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = dma_hal_get_channel_offset(channel, DMA_REG_CHANNEL0_CONFIG);
    return dma_hal_write_raw(offset, config);
}

dma_hal_status_t dma_hal_read_channel_length(uint32_t channel, uint32_t *length)
{
    if (!dma_hal_validate_channel(channel)) {
        return DMA_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = dma_hal_get_channel_offset(channel, DMA_REG_CHANNEL0_LENGTH);
    return dma_hal_read_raw(offset, length);
}

dma_hal_status_t dma_hal_write_channel_length(uint32_t channel, uint32_t length)
{
    if (!dma_hal_validate_channel(channel)) {
        return DMA_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = dma_hal_get_channel_offset(channel, DMA_REG_CHANNEL0_LENGTH);
    return dma_hal_write_raw(offset, length);
}

dma_hal_status_t dma_hal_read_channel_saddr(uint32_t channel, uint32_t *saddr)
{
    if (!dma_hal_validate_channel(channel)) {
        return DMA_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = dma_hal_get_channel_offset(channel, DMA_REG_CHANNEL0_SADDR);
    return dma_hal_read_raw(offset, saddr);
}

dma_hal_status_t dma_hal_write_channel_saddr(uint32_t channel, uint32_t saddr)
{
    if (!dma_hal_validate_channel(channel)) {
        return DMA_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = dma_hal_get_channel_offset(channel, DMA_REG_CHANNEL0_SADDR);
    return dma_hal_write_raw(offset, saddr);
}

dma_hal_status_t dma_hal_read_channel_daddr(uint32_t channel, uint32_t *daddr)
{
    if (!dma_hal_validate_channel(channel)) {
        return DMA_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = dma_hal_get_channel_offset(channel, DMA_REG_CHANNEL0_DADDR);
    return dma_hal_read_raw(offset, daddr);
}

dma_hal_status_t dma_hal_write_channel_daddr(uint32_t channel, uint32_t daddr)
{
    if (!dma_hal_validate_channel(channel)) {
        return DMA_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = dma_hal_get_channel_offset(channel, DMA_REG_CHANNEL0_DADDR);
    return dma_hal_write_raw(offset, daddr);
}

dma_hal_status_t dma_hal_read_channel_enable(uint32_t channel, uint32_t *enable)
{
    if (!dma_hal_validate_channel(channel)) {
        return DMA_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = dma_hal_get_channel_offset(channel, DMA_REG_CHANNEL0_ENABLE);
    return dma_hal_read_raw(offset, enable);
}

dma_hal_status_t dma_hal_write_channel_enable(uint32_t channel, uint32_t enable)
{
    if (!dma_hal_validate_channel(channel)) {
        return DMA_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = dma_hal_get_channel_offset(channel, DMA_REG_CHANNEL0_ENABLE);
    return dma_hal_write_raw(offset, enable);
}

dma_hal_status_t dma_hal_read_channel_data_trans_num(uint32_t channel, uint32_t *trans_num)
{
    if (!dma_hal_validate_channel(channel)) {
        return DMA_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = dma_hal_get_channel_offset(channel, DMA_REG_CHANNEL0_DATA_TRANS_NUM);
    return dma_hal_read_raw(offset, trans_num);
}

dma_hal_status_t dma_hal_read_channel_fifo_left_num(uint32_t channel, uint32_t *fifo_left)
{
    if (!dma_hal_validate_channel(channel)) {
        return DMA_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = dma_hal_get_channel_offset(channel, DMA_REG_CHANNEL0_FIFO_LEFT_NUM);
    return dma_hal_read_raw(offset, fifo_left);
}

dma_hal_status_t dma_hal_read_channel_dend_addr(uint32_t channel, uint32_t *dend_addr)
{
    if (!dma_hal_validate_channel(channel)) {
        return DMA_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = dma_hal_get_channel_offset(channel, DMA_REG_CHANNEL0_DEND_ADDR);
    return dma_hal_read_raw(offset, dend_addr);
}

dma_hal_status_t dma_hal_write_channel_dend_addr(uint32_t channel, uint32_t dend_addr)
{
    if (!dma_hal_validate_channel(channel)) {
        return DMA_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = dma_hal_get_channel_offset(channel, DMA_REG_CHANNEL0_DEND_ADDR);
    return dma_hal_write_raw(offset, dend_addr);
}

dma_hal_status_t dma_hal_read_channel_addr_offset(uint32_t channel, uint32_t *addr_offset)
{
    if (!dma_hal_validate_channel(channel)) {
        return DMA_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = dma_hal_get_channel_offset(channel, DMA_REG_CHANNEL0_ADDR_OFFSET);
    return dma_hal_read_raw(offset, addr_offset);
}

dma_hal_status_t dma_hal_write_channel_addr_offset(uint32_t channel, uint32_t addr_offset)
{
    if (!dma_hal_validate_channel(channel)) {
        return DMA_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = dma_hal_get_channel_offset(channel, DMA_REG_CHANNEL0_ADDR_OFFSET);
    return dma_hal_write_raw(offset, addr_offset);
}

dma_hal_status_t dma_hal_read_channel_dmamux_cfg(uint32_t channel, uint32_t *dmamux_cfg)
{
    if (!dma_hal_validate_channel(channel)) {
        return DMA_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = dma_hal_get_channel_offset(channel, DMA_REG_CHANNEL0_DMAMUX_CFG);
    return dma_hal_read_raw(offset, dmamux_cfg);
}

dma_hal_status_t dma_hal_write_channel_dmamux_cfg(uint32_t channel, uint32_t dmamux_cfg)
{
    if (!dma_hal_validate_channel(channel)) {
        return DMA_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = dma_hal_get_channel_offset(channel, DMA_REG_CHANNEL0_DMAMUX_CFG);
    return dma_hal_write_raw(offset, dmamux_cfg);
}

dma_hal_status_t dma_hal_read_channel_dstart_addr(uint32_t channel, uint32_t *dstart_addr)
{
    if (!dma_hal_validate_channel(channel)) {
        return DMA_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = dma_hal_get_channel_offset(channel, DMA_REG_CHANNEL0_DSTART_ADDR);
    return dma_hal_read_raw(offset, dstart_addr);
}

dma_hal_status_t dma_hal_write_channel_dstart_addr(uint32_t channel, uint32_t dstart_addr)
{
    if (!dma_hal_validate_channel(channel)) {
        return DMA_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = dma_hal_get_channel_offset(channel, DMA_REG_CHANNEL0_DSTART_ADDR);
    return dma_hal_write_raw(offset, dstart_addr);
}