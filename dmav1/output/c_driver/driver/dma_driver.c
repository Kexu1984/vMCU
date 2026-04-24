/**
 * @file dma_driver.c
 * @brief DMA Driver Layer implementation
 *
 * This module provides high-level DMA driver functions with business logic
 * and configuration management. It builds on top of the HAL layer.
 */

#include "dma_driver.h"
#include "../hal/dma_hal.h"
#include <string.h>
#include <stdio.h>

#ifndef DMA_DRV_DEBUG
#define DMA_DRV_DEBUG 1
#endif
#if DMA_DRV_DEBUG
#define DRV_LOG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define DRV_LOG(fmt, ...) (void)0
#endif

/**
 * @brief Driver version information
 */
#define DMA_DRIVER_VERSION_MAJOR    1
#define DMA_DRIVER_VERSION_MINOR    0
#define DMA_DRIVER_VERSION_PATCH    0

/**
 * @brief Driver state
 */
static bool g_dma_driver_initialized = false;

/**
 * @brief Channel state tracking
 */
static dma_channel_state_t g_channel_states[DMA_MAX_CHANNELS];

/**
 * @brief Helper function to validate channel ID
 */
static bool is_valid_channel_id(uint32_t channel)
{
    return (channel < DMA_MAX_CHANNELS);
}

/**
 * @brief Helper function to convert HAL status to driver status
 */
static dma_drv_status_t hal_to_drv_status(dma_hal_status_t hal_status)
{
    switch (hal_status) {
        case DMA_HAL_OK:
            return DMA_DRV_OK;
        case DMA_HAL_INVALID_PARAM:
            return DMA_DRV_INVALID_PARAM;
        case DMA_HAL_NOT_INITIALIZED:
            return DMA_DRV_NOT_INITIALIZED;
        case DMA_HAL_HARDWARE_ERROR:
        case DMA_HAL_ERROR:
        default:
            return DMA_DRV_ERROR;
    }
}

/**
 * @brief Update channel state based on status flags
 */
static void update_channel_state(uint32_t channel, uint32_t status_flags)
{
    if (!is_valid_channel_id(channel)) {
        return;
    }

    if (status_flags & (DMA_STATUS_DBE | DMA_STATUS_SBE | DMA_STATUS_DOE | 
                        DMA_STATUS_SOE | DMA_STATUS_DAE | DMA_STATUS_SAE | DMA_STATUS_CLE)) {
        g_channel_states[channel] = DMA_CHANNEL_STATE_ERROR;
    } else if (status_flags & DMA_STATUS_FINISH) {
        g_channel_states[channel] = DMA_CHANNEL_STATE_COMPLETE;
    } else if (status_flags & DMA_STATUS_HALF_FINISH) {
        g_channel_states[channel] = DMA_CHANNEL_STATE_HALF_COMPLETE;
    }
}

dma_drv_status_t dma_driver_init(uint32_t base_address)
{
    dma_hal_status_t hal_status;
    
    // Initialize HAL with base address
    hal_status = dma_hal_base_address_init(base_address);
    if (hal_status != DMA_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    // Initialize HAL
    hal_status = dma_hal_init();
    if (hal_status != DMA_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    // Initialize channel states
    for (uint32_t i = 0; i < DMA_MAX_CHANNELS; i++) {
        g_channel_states[i] = DMA_CHANNEL_STATE_IDLE;
    }
    
    g_dma_driver_initialized = true;
    DRV_LOG("DMA Driver: Initialized successfully with base address 0x%08X\n", base_address);
    
    return DMA_DRV_OK;
}

dma_drv_status_t dma_driver_deinit(void)
{
    if (!g_dma_driver_initialized) {
        return DMA_DRV_NOT_INITIALIZED;
    }
    
    // Stop all channels
    for (uint32_t i = 0; i < DMA_MAX_CHANNELS; i++) {
        dma_channel_stop(i);
    }
    
    // Deinitialize HAL
    dma_hal_status_t hal_status = dma_hal_deinit();
    if (hal_status != DMA_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    g_dma_driver_initialized = false;
    DRV_LOG("DMA Driver: Deinitialized\n");
    
    return DMA_DRV_OK;
}

dma_drv_status_t dma_driver_get_version(dma_driver_version_t *version)
{
    if (version == NULL) {
        return DMA_DRV_INVALID_PARAM;
    }
    
    version->major = DMA_DRIVER_VERSION_MAJOR;
    version->minor = DMA_DRIVER_VERSION_MINOR;
    version->patch = DMA_DRIVER_VERSION_PATCH;
    
    return DMA_DRV_OK;
}

dma_drv_status_t dma_channel_get_default_config(dma_channel_config_t *config)
{
    if (config == NULL) {
        return DMA_DRV_INVALID_PARAM;
    }
    
    memset(config, 0, sizeof(dma_channel_config_t));
    
    config->direction = DMA_TRANSFER_MEM2MEM;
    config->priority = DMA_DEFAULT_PRIORITY;
    config->src_data_size = DMA_DEFAULT_DATA_SIZE;
    config->dst_data_size = DMA_DEFAULT_DATA_SIZE;
    config->src_addr_mode = DMA_DEFAULT_ADDR_MODE;
    config->dst_addr_mode = DMA_DEFAULT_ADDR_MODE;
    config->req_id = DMA_REQ_ALWAYS_ENABLED;
    config->trig_enable = false;
    config->circular_mode = false;
    config->half_complete_interrupt = false;
    config->complete_interrupt = true;
    config->error_interrupt = true;
    config->debug_enable = false;
    
    return DMA_DRV_OK;
}

dma_drv_status_t dma_channel_configure(uint32_t channel, const dma_channel_config_t *config)
{
    if (!g_dma_driver_initialized) {
        return DMA_DRV_NOT_INITIALIZED;
    }
    
    if (!is_valid_channel_id(channel) || config == NULL) {
        return DMA_DRV_INVALID_PARAM;
    }
    
    dma_hal_status_t hal_status;
    uint32_t config_reg = 0;
    uint32_t dmamux_reg = 0;
    uint32_t addr_offset_reg = 0;
    
    DRV_LOG("DMA Driver: Configuring channel %u\n", channel);
    
    // Configure priority
    config_reg |= ((config->priority & 0x3) << DMA_CONFIG_PRIORITY_SHIFT);
    
    // Configure source data size
    config_reg |= ((config->src_data_size & 0x3) << DMA_CONFIG_SSIZE_SHIFT);
    
    // Configure destination data size
    config_reg |= ((config->dst_data_size & 0x3) << DMA_CONFIG_DSIZE_SHIFT);
    
    // Configure address modes
    if (config->src_addr_mode == DMA_ADDR_MODE_INCREMENT) {
        config_reg |= DMA_CONFIG_SINC;
    }
    if (config->dst_addr_mode == DMA_ADDR_MODE_INCREMENT) {
        config_reg |= DMA_CONFIG_DINC;
    }
    
    // Configure transfer direction
    config_reg |= ((config->direction & 0x3) << DMA_CONFIG_DIRECTION_SHIFT);
    
    // Configure circular mode
    if (config->circular_mode) {
        config_reg |= DMA_CONFIG_CIRCULAR;
    }
    
    // Configure interrupts
    if (config->half_complete_interrupt) {
        config_reg |= DMA_CONFIG_HALF_INT_EN;
    }
    if (config->complete_interrupt) {
        config_reg |= DMA_CONFIG_COMPLETE_INT_EN;
    }
    if (config->error_interrupt) {
        config_reg |= DMA_CONFIG_ERROR_INT_EN;
    }
    
    // Configure DMAMUX
    dmamux_reg |= ((config->req_id & 0x7F) << DMA_DMAMUX_REQ_ID_SHIFT);
    if (config->trig_enable) {
        dmamux_reg |= DMA_DMAMUX_TRIG_EN;
    }
    
    // Configure address offsets
    addr_offset_reg = ((config->dst_offset & 0xFFFF) << 16) | (config->src_offset & 0xFFFF);
    
    // Write configuration registers
    hal_status = dma_hal_write_channel_config(channel, config_reg);
    if (hal_status != DMA_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    hal_status = dma_hal_write_channel_length(channel, config->data_length);
    if (hal_status != DMA_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    hal_status = dma_hal_write_channel_saddr(channel, config->src_address);
    if (hal_status != DMA_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    hal_status = dma_hal_write_channel_daddr(channel, config->dst_address);
    if (hal_status != DMA_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    hal_status = dma_hal_write_channel_dmamux_cfg(channel, dmamux_reg);
    if (hal_status != DMA_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    hal_status = dma_hal_write_channel_addr_offset(channel, addr_offset_reg);
    if (hal_status != DMA_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    // Configure circular mode addresses if enabled
    if (config->circular_mode) {
        hal_status = dma_hal_write_channel_dstart_addr(channel, config->dst_start_addr);
        if (hal_status != DMA_HAL_OK) {
            return hal_to_drv_status(hal_status);
        }
        
        hal_status = dma_hal_write_channel_dend_addr(channel, config->dst_end_addr);
        if (hal_status != DMA_HAL_OK) {
            return hal_to_drv_status(hal_status);
        }
    }
    
    g_channel_states[channel] = DMA_CHANNEL_STATE_READY;
    DRV_LOG("DMA Driver: Channel %u configured successfully\n", channel);
    
    return DMA_DRV_OK;
}

dma_drv_status_t dma_channel_start(uint32_t channel)
{
    if (!g_dma_driver_initialized) {
        return DMA_DRV_NOT_INITIALIZED;
    }
    
    if (!is_valid_channel_id(channel)) {
        return DMA_DRV_INVALID_PARAM;
    }
    
    dma_hal_status_t hal_status;
    uint32_t enable_reg = DMA_ENABLE_CHAN_EN;
    
    DRV_LOG("DMA Driver: Starting channel %u\n", channel);
    
    hal_status = dma_hal_write_channel_enable(channel, enable_reg);
    if (hal_status != DMA_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    g_channel_states[channel] = DMA_CHANNEL_STATE_BUSY;
    
    return DMA_DRV_OK;
}

dma_drv_status_t dma_channel_stop(uint32_t channel)
{
    if (!g_dma_driver_initialized) {
        return DMA_DRV_NOT_INITIALIZED;
    }
    
    if (!is_valid_channel_id(channel)) {
        return DMA_DRV_INVALID_PARAM;
    }
    
    dma_hal_status_t hal_status;
    
    DRV_LOG("DMA Driver: Stopping channel %u\n", channel);
    
    hal_status = dma_hal_write_channel_enable(channel, 0);
    if (hal_status != DMA_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    g_channel_states[channel] = DMA_CHANNEL_STATE_IDLE;
    
    return DMA_DRV_OK;
}

dma_drv_status_t dma_channel_pause(uint32_t channel)
{
    // For this implementation, pause is equivalent to stop
    // In a real implementation, this might use a STOP bit to pause temporarily
    return dma_channel_stop(channel);
}

dma_drv_status_t dma_channel_resume(uint32_t channel)
{
    // For this implementation, resume is equivalent to start
    return dma_channel_start(channel);
}

dma_drv_status_t dma_channel_get_status(uint32_t channel, dma_channel_status_t *status)
{
    if (!g_dma_driver_initialized) {
        return DMA_DRV_NOT_INITIALIZED;
    }
    
    if (!is_valid_channel_id(channel) || status == NULL) {
        return DMA_DRV_INVALID_PARAM;
    }
    
    dma_hal_status_t hal_status;
    uint32_t status_reg;
    uint32_t trans_num;
    uint32_t fifo_left;
    
    hal_status = dma_hal_read_channel_status(channel, &status_reg);
    if (hal_status != DMA_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    hal_status = dma_hal_read_channel_data_trans_num(channel, &trans_num);
    if (hal_status != DMA_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    hal_status = dma_hal_read_channel_fifo_left_num(channel, &fifo_left);
    if (hal_status != DMA_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    // Update channel state based on status flags
    update_channel_state(channel, status_reg);
    
    // Fill status structure
    memset(status, 0, sizeof(dma_channel_status_t));
    status->state = g_channel_states[channel];
    status->status_flags = status_reg;
    status->transferred_count = trans_num & 0xFFFF;
    status->fifo_left_count = fifo_left & 0x3F;
    status->finish_flag = (status_reg & DMA_STATUS_FINISH) != 0;
    status->half_finish_flag = (status_reg & DMA_STATUS_HALF_FINISH) != 0;
    status->dst_bus_error = (status_reg & DMA_STATUS_DBE) != 0;
    status->src_bus_error = (status_reg & DMA_STATUS_SBE) != 0;
    status->dst_offset_error = (status_reg & DMA_STATUS_DOE) != 0;
    status->src_offset_error = (status_reg & DMA_STATUS_SOE) != 0;
    status->dst_addr_error = (status_reg & DMA_STATUS_DAE) != 0;
    status->src_addr_error = (status_reg & DMA_STATUS_SAE) != 0;
    status->channel_length_error = (status_reg & DMA_STATUS_CLE) != 0;
    
    return DMA_DRV_OK;
}

dma_drv_status_t dma_channel_clear_status(uint32_t channel, uint32_t flags)
{
    if (!g_dma_driver_initialized) {
        return DMA_DRV_NOT_INITIALIZED;
    }
    
    if (!is_valid_channel_id(channel)) {
        return DMA_DRV_INVALID_PARAM;
    }
    
    // Note: In the actual hardware, writing 0 to status bits clears them
    // This is a simplified implementation
    DRV_LOG("DMA Driver: Clearing status flags 0x%08X for channel %u\n", flags, channel);
    
    return DMA_DRV_OK;
}

dma_drv_status_t dma_channel_is_busy(uint32_t channel, bool *is_busy)
{
    if (!g_dma_driver_initialized) {
        return DMA_DRV_NOT_INITIALIZED;
    }
    
    if (!is_valid_channel_id(channel) || is_busy == NULL) {
        return DMA_DRV_INVALID_PARAM;
    }
    
    dma_channel_status_t status;
    dma_drv_status_t drv_status = dma_channel_get_status(channel, &status);
    if (drv_status != DMA_DRV_OK) {
        return drv_status;
    }
    
    *is_busy = (status.state == DMA_CHANNEL_STATE_BUSY);
    
    return DMA_DRV_OK;
}

dma_drv_status_t dma_channel_wait_for_completion(uint32_t channel, uint32_t timeout_ms)
{
    if (!g_dma_driver_initialized) {
        return DMA_DRV_NOT_INITIALIZED;
    }
    
    if (!is_valid_channel_id(channel)) {
        return DMA_DRV_INVALID_PARAM;
    }
    
    // Simple polling implementation
    // In a real implementation, this would use proper timing mechanisms
    dma_channel_status_t status;
    uint32_t attempts = timeout_ms > 0 ? timeout_ms / 10 : 1000; // 10ms intervals
    
    for (uint32_t i = 0; i < attempts; i++) {
        dma_drv_status_t drv_status = dma_channel_get_status(channel, &status);
        if (drv_status != DMA_DRV_OK) {
            return drv_status;
        }
        
        if (status.state == DMA_CHANNEL_STATE_COMPLETE) {
            return DMA_DRV_TRANSFER_COMPLETE;
        }
        
        if (status.state == DMA_CHANNEL_STATE_ERROR) {
            return DMA_DRV_CHANNEL_ERROR;
        }
        
        // Simple delay simulation
        for (volatile int j = 0; j < 10000; j++);
        
        if (timeout_ms == 0) {
            break; // No timeout, return immediately
        }
    }
    
    return DMA_DRV_TIMEOUT;
}

dma_drv_status_t dma_mem2mem_transfer(uint32_t channel, uint32_t src_addr, uint32_t dst_addr, uint32_t size)
{
    dma_channel_config_t config;
    dma_drv_status_t drv_status;
    
    drv_status = dma_channel_get_default_config(&config);
    if (drv_status != DMA_DRV_OK) {
        return drv_status;
    }
    
    config.direction = DMA_TRANSFER_MEM2MEM;
    config.src_address = src_addr;
    config.dst_address = dst_addr;
    config.data_length = size;
    config.req_id = DMA_REQ_ALWAYS_ENABLED;
    
    drv_status = dma_channel_configure(channel, &config);
    if (drv_status != DMA_DRV_OK) {
        return drv_status;
    }
    
    return dma_channel_start(channel);
}

dma_drv_status_t dma_mem2peri_transfer(uint32_t channel, uint32_t src_addr, uint32_t peri_addr, uint32_t size, uint8_t req_id)
{
    dma_channel_config_t config;
    dma_drv_status_t drv_status;
    
    drv_status = dma_channel_get_default_config(&config);
    if (drv_status != DMA_DRV_OK) {
        return drv_status;
    }
    
    config.direction = DMA_TRANSFER_MEM2PERI;
    config.src_address = src_addr;
    config.dst_address = peri_addr;
    config.data_length = size;
    config.req_id = req_id;
    config.dst_addr_mode = DMA_ADDR_MODE_FIXED; // Peripheral address typically fixed
    
    drv_status = dma_channel_configure(channel, &config);
    if (drv_status != DMA_DRV_OK) {
        return drv_status;
    }
    
    return dma_channel_start(channel);
}

dma_drv_status_t dma_peri2mem_transfer(uint32_t channel, uint32_t peri_addr, uint32_t dst_addr, uint32_t size, uint8_t req_id)
{
    dma_channel_config_t config;
    dma_drv_status_t drv_status;
    
    drv_status = dma_channel_get_default_config(&config);
    if (drv_status != DMA_DRV_OK) {
        return drv_status;
    }
    
    config.direction = DMA_TRANSFER_PERI2MEM;
    config.src_address = peri_addr;
    config.dst_address = dst_addr;
    config.data_length = size;
    config.req_id = req_id;
    config.src_addr_mode = DMA_ADDR_MODE_FIXED; // Peripheral address typically fixed
    
    drv_status = dma_channel_configure(channel, &config);
    if (drv_status != DMA_DRV_OK) {
        return drv_status;
    }
    
    return dma_channel_start(channel);
}

dma_drv_status_t dma_setup_circular_mode(uint32_t channel, uint32_t src_addr, uint32_t dst_start_addr, uint32_t dst_end_addr, dma_data_size_t data_size)
{
    dma_channel_config_t config;
    dma_drv_status_t drv_status;
    
    drv_status = dma_channel_get_default_config(&config);
    if (drv_status != DMA_DRV_OK) {
        return drv_status;
    }
    
    config.circular_mode = true;
    config.src_address = src_addr;
    config.dst_address = dst_start_addr;
    config.dst_start_addr = dst_start_addr;
    config.dst_end_addr = dst_end_addr;
    config.src_data_size = data_size;
    config.dst_data_size = data_size;
    config.data_length = dst_end_addr - dst_start_addr;
    
    return dma_channel_configure(channel, &config);
}

dma_drv_status_t dma_global_warm_reset(void)
{
    if (!g_dma_driver_initialized) {
        return DMA_DRV_NOT_INITIALIZED;
    }
    
    dma_hal_status_t hal_status;
    uint32_t reset_reg = 0x1; // WARM_RST bit
    
    DRV_LOG("DMA Driver: Performing global warm reset\n");
    
    hal_status = dma_hal_write_top_reset(reset_reg);
    if (hal_status != DMA_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    // Reset channel states
    for (uint32_t i = 0; i < DMA_MAX_CHANNELS; i++) {
        g_channel_states[i] = DMA_CHANNEL_STATE_IDLE;
    }
    
    return DMA_DRV_OK;
}

dma_drv_status_t dma_global_hard_reset(void)
{
    if (!g_dma_driver_initialized) {
        return DMA_DRV_NOT_INITIALIZED;
    }
    
    dma_hal_status_t hal_status;
    uint32_t reset_reg = 0x2; // HARD_RST bit
    
    DRV_LOG("DMA Driver: Performing global hard reset\n");
    
    hal_status = dma_hal_write_top_reset(reset_reg);
    if (hal_status != DMA_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    // Reset channel states
    for (uint32_t i = 0; i < DMA_MAX_CHANNELS; i++) {
        g_channel_states[i] = DMA_CHANNEL_STATE_IDLE;
    }
    
    return DMA_DRV_OK;
}

dma_drv_status_t dma_is_idle(bool *is_idle)
{
    if (!g_dma_driver_initialized) {
        return DMA_DRV_NOT_INITIALIZED;
    }
    
    if (is_idle == NULL) {
        return DMA_DRV_INVALID_PARAM;
    }
    
    dma_hal_status_t hal_status;
    uint32_t reset_reg;
    
    hal_status = dma_hal_read_top_reset(&reset_reg);
    if (hal_status != DMA_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    *is_idle = (reset_reg & 0x4) != 0; // DMA_IDLE bit
    
    return DMA_DRV_OK;
}

dma_drv_status_t dma_channel_get_transferred_count(uint32_t channel, uint32_t *transferred_count)
{
    if (!g_dma_driver_initialized) {
        return DMA_DRV_NOT_INITIALIZED;
    }
    
    if (!is_valid_channel_id(channel) || transferred_count == NULL) {
        return DMA_DRV_INVALID_PARAM;
    }
    
    dma_hal_status_t hal_status;
    uint32_t trans_num;
    
    hal_status = dma_hal_read_channel_data_trans_num(channel, &trans_num);
    if (hal_status != DMA_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    *transferred_count = trans_num & 0xFFFF;
    
    return DMA_DRV_OK;
}

dma_drv_status_t dma_channel_get_fifo_left_count(uint32_t channel, uint32_t *fifo_left_count)
{
    if (!g_dma_driver_initialized) {
        return DMA_DRV_NOT_INITIALIZED;
    }
    
    if (!is_valid_channel_id(channel) || fifo_left_count == NULL) {
        return DMA_DRV_INVALID_PARAM;
    }
    
    dma_hal_status_t hal_status;
    uint32_t fifo_left;
    
    hal_status = dma_hal_read_channel_fifo_left_num(channel, &fifo_left);
    if (hal_status != DMA_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    *fifo_left_count = fifo_left & 0x3F;
    
    return DMA_DRV_OK;
}

dma_drv_status_t dma_channel_flush_fifo(uint32_t channel)
{
    if (!g_dma_driver_initialized) {
        return DMA_DRV_NOT_INITIALIZED;
    }
    
    if (!is_valid_channel_id(channel)) {
        return DMA_DRV_INVALID_PARAM;
    }
    
    // FIFO flush implementation would depend on hardware specifics
    // This is a placeholder implementation
    DRV_LOG("DMA Driver: Flushing FIFO for channel %u\n", channel);
    
    return DMA_DRV_OK;
}