/**
 * @file dmav2_driver.c
 * @brief DMAv2 Driver Layer implementation
 *
 * This module implements high-level API functions for the DMAv2 device,
 * providing business logic and configuration management.
 */

#include "dmav2_driver.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Driver initialization status
 */
static bool g_dmav2_driver_initialized = false;

/**
 * @brief Channel state tracking
 */
static dmav2_channel_state_t g_channel_states[DMAV2_MAX_CHANNELS];
static dmav2_channel_config_t g_channel_configs[DMAV2_MAX_CHANNELS];

/**
 * @brief Driver version string
 */
static const char *g_version_string = "DMAv2 Driver v2.0.0";

/**
 * @brief Error code to string mapping
 */
static const char* dmav2_error_strings[] = {
    "Success",
    "Invalid parameter",
    "Not initialized", 
    "Hardware fault",
    "Timeout",
    "Channel busy",
    "Invalid channel",
    "Invalid configuration",
    "Transfer failed"
};

/**
 * @brief Initialize DMAv2 driver
 */
dmav2_status_t dmav2_init(void)
{
    dmav2_hal_status_t hal_status;
    
    // Initialize HAL layer
    hal_status = dmav2_hal_init();
    if (hal_status != DMAV2_HAL_OK) {
        return DMAV2_ERROR_HARDWARE_FAULT;
    }
    
    // Initialize channel states
    for (int i = 0; i < DMAV2_MAX_CHANNELS; i++) {
        g_channel_states[i] = DMAV2_CHANNEL_IDLE;
        memset(&g_channel_configs[i], 0, sizeof(dmav2_channel_config_t));
    }
    
    g_dmav2_driver_initialized = true;
    return DMAV2_SUCCESS;
}

/**
 * @brief Deinitialize DMAv2 driver
 */
dmav2_status_t dmav2_deinit(void)
{
    dmav2_hal_status_t hal_status;
    
    if (!g_dmav2_driver_initialized) {
        return DMAV2_ERROR_NOT_INITIALIZED;
    }
    
    // Stop all active channels
    for (int i = 0; i < DMAV2_MAX_CHANNELS; i++) {
        if (g_channel_states[i] == DMAV2_CHANNEL_ACTIVE) {
            dmav2_stop_transfer(i);
        }
    }
    
    // Deinitialize HAL layer
    hal_status = dmav2_hal_deinit();
    if (hal_status != DMAV2_HAL_OK) {
        return DMAV2_ERROR_HARDWARE_FAULT;
    }
    
    g_dmav2_driver_initialized = false;
    return DMAV2_SUCCESS;
}

/**
 * @brief Reset DMA controller
 */
dmav2_status_t dmav2_reset(bool hard_reset)
{
    dmav2_hal_status_t hal_status;
    uint32_t reset_value;
    
    if (!g_dmav2_driver_initialized) {
        return DMAV2_ERROR_NOT_INITIALIZED;
    }
    
    // Prepare reset value
    if (hard_reset) {
        reset_value = DMAV2_TOP_RST_HARD_RST_MASK;
    } else {
        reset_value = DMAV2_TOP_RST_WARM_RST_MASK;
    }
    
    // Write reset register
    hal_status = dmav2_hal_write_top_rst(reset_value);
    if (hal_status != DMAV2_HAL_OK) {
        return DMAV2_ERROR_HARDWARE_FAULT;
    }
    
    // Clear reset for hard reset
    if (hard_reset) {
        hal_status = dmav2_hal_write_top_rst(0);
        if (hal_status != DMAV2_HAL_OK) {
            return DMAV2_ERROR_HARDWARE_FAULT;
        }
    }
    
    // Reset channel states
    for (int i = 0; i < DMAV2_MAX_CHANNELS; i++) {
        g_channel_states[i] = DMAV2_CHANNEL_IDLE;
    }
    
    return DMAV2_SUCCESS;
}

/**
 * @brief Get global DMA status
 */
dmav2_status_t dmav2_get_global_status(dmav2_global_status_t *status)
{
    dmav2_hal_status_t hal_status;
    uint32_t reg_value;
    
    if (!g_dmav2_driver_initialized) {
        return DMAV2_ERROR_NOT_INITIALIZED;
    }
    
    if (status == NULL) {
        return DMAV2_ERROR_INVALID_PARAM;
    }
    
    // Read DMA_TOP_RST register for idle status
    hal_status = dmav2_hal_read_top_rst(&reg_value);
    if (hal_status != DMAV2_HAL_OK) {
        return DMAV2_ERROR_HARDWARE_FAULT;
    }
    
    status->dma_idle = (reg_value & DMAV2_TOP_RST_DMA_IDLE_MASK) ? true : false;
    
    // Build active channels mask from local state
    status->active_channels = 0;
    for (int i = 0; i < DMAV2_MAX_CHANNELS; i++) {
        if (g_channel_states[i] == DMAV2_CHANNEL_ACTIVE) {
            status->active_channels |= (1U << i);
        }
    }
    
    return DMAV2_SUCCESS;
}

/**
 * @brief Validate channel number
 */
dmav2_status_t dmav2_validate_channel(uint8_t channel)
{
    if (channel >= DMAV2_MAX_CHANNELS) {
        return DMAV2_ERROR_CHANNEL_INVALID;
    }
    return DMAV2_SUCCESS;
}

/**
 * @brief Initialize specific channel
 */
dmav2_status_t dmav2_channel_init(uint8_t channel)
{
    dmav2_status_t status;
    
    if (!g_dmav2_driver_initialized) {
        return DMAV2_ERROR_NOT_INITIALIZED;
    }
    
    status = dmav2_validate_channel(channel);
    if (status != DMAV2_SUCCESS) {
        return status;
    }
    
    // Reset channel state
    g_channel_states[channel] = DMAV2_CHANNEL_IDLE;
    memset(&g_channel_configs[channel], 0, sizeof(dmav2_channel_config_t));
    
    // Set default configuration
    g_channel_configs[channel].mode = DMAV2_MODE_MEM2MEM;
    g_channel_configs[channel].src_width = DMAV2_DATA_WIDTH_32BIT;
    g_channel_configs[channel].dst_width = DMAV2_DATA_WIDTH_32BIT;
    g_channel_configs[channel].priority = DMAV2_PRIORITY_LOW;
    g_channel_configs[channel].src_addr_mode = DMAV2_ADDR_INCREMENT;
    g_channel_configs[channel].dst_addr_mode = DMAV2_ADDR_INCREMENT;
    
    return DMAV2_SUCCESS;
}

/**
 * @brief Deinitialize specific channel
 */
dmav2_status_t dmav2_channel_deinit(uint8_t channel)
{
    dmav2_status_t status;
    
    if (!g_dmav2_driver_initialized) {
        return DMAV2_ERROR_NOT_INITIALIZED;
    }
    
    status = dmav2_validate_channel(channel);
    if (status != DMAV2_SUCCESS) {
        return status;
    }
    
    // Stop channel if active
    if (g_channel_states[channel] == DMAV2_CHANNEL_ACTIVE) {
        dmav2_stop_transfer(channel);
    }
    
    g_channel_states[channel] = DMAV2_CHANNEL_IDLE;
    memset(&g_channel_configs[channel], 0, sizeof(dmav2_channel_config_t));
    
    return DMAV2_SUCCESS;
}

/**
 * @brief Validate channel configuration
 */
dmav2_status_t dmav2_validate_config(const dmav2_channel_config_t *config)
{
    if (config == NULL) {
        return DMAV2_ERROR_INVALID_PARAM;
    }
    
    if (config->mode > DMAV2_MODE_PERI2PERI) {
        return DMAV2_ERROR_CONFIG_INVALID;
    }
    
    if (config->src_width > DMAV2_DATA_WIDTH_32BIT || 
        config->dst_width > DMAV2_DATA_WIDTH_32BIT) {
        return DMAV2_ERROR_CONFIG_INVALID;
    }
    
    if (config->priority > DMAV2_PRIORITY_VERY_HIGH) {
        return DMAV2_ERROR_CONFIG_INVALID;
    }
    
    if (config->src_addr_mode > DMAV2_ADDR_INCREMENT ||
        config->dst_addr_mode > DMAV2_ADDR_INCREMENT) {
        return DMAV2_ERROR_CONFIG_INVALID;
    }
    
    if (config->dmamux_req_id > 127) {  // 7-bit field
        return DMAV2_ERROR_CONFIG_INVALID;
    }
    
    return DMAV2_SUCCESS;
}

/**
 * @brief Configure channel
 */
dmav2_status_t dmav2_channel_configure(uint8_t channel, const dmav2_channel_config_t *config)
{
    dmav2_status_t status;
    dmav2_hal_status_t hal_status;
    uint32_t dmamux_value;
    
    if (!g_dmav2_driver_initialized) {
        return DMAV2_ERROR_NOT_INITIALIZED;
    }
    
    status = dmav2_validate_channel(channel);
    if (status != DMAV2_SUCCESS) {
        return status;
    }
    
    status = dmav2_validate_config(config);
    if (status != DMAV2_SUCCESS) {
        return status;
    }
    
    if (g_channel_states[channel] == DMAV2_CHANNEL_ACTIVE) {
        return DMAV2_ERROR_BUSY;
    }
    
    // Store configuration
    memcpy(&g_channel_configs[channel], config, sizeof(dmav2_channel_config_t));
    
    // Configure DMAMUX
    dmamux_value = config->dmamux_req_id & DMAV2_DMAMUX_REQ_ID_MASK;
    if (config->dmamux_trigger_enable) {
        dmamux_value |= DMAV2_DMAMUX_TRIG_EN_MASK;
    }
    
    hal_status = dmav2_hal_write_channel_dmamux_cfg(channel, dmamux_value);
    if (hal_status != DMAV2_HAL_OK) {
        return DMAV2_ERROR_HARDWARE_FAULT;
    }
    
    return DMAV2_SUCCESS;
}

/**
 * @brief Get channel configuration
 */
dmav2_status_t dmav2_channel_get_config(uint8_t channel, dmav2_channel_config_t *config)
{
    dmav2_status_t status;
    
    if (!g_dmav2_driver_initialized) {
        return DMAV2_ERROR_NOT_INITIALIZED;
    }
    
    status = dmav2_validate_channel(channel);
    if (status != DMAV2_SUCCESS) {
        return status;
    }
    
    if (config == NULL) {
        return DMAV2_ERROR_INVALID_PARAM;
    }
    
    memcpy(config, &g_channel_configs[channel], sizeof(dmav2_channel_config_t));
    
    return DMAV2_SUCCESS;
}

/**
 * @brief Validate transfer configuration
 */
dmav2_status_t dmav2_validate_transfer_config(const dmav2_transfer_config_t *transfer_config)
{
    if (transfer_config == NULL) {
        return DMAV2_ERROR_INVALID_PARAM;
    }
    
    if (transfer_config->transfer_count == 0) {
        return DMAV2_ERROR_CONFIG_INVALID;
    }
    
    if (transfer_config->src_addr == 0 || transfer_config->dst_addr == 0) {
        return DMAV2_ERROR_CONFIG_INVALID;
    }
    
    return DMAV2_SUCCESS;
}

/**
 * @brief Start DMA transfer
 */
dmav2_status_t dmav2_start_transfer(uint8_t channel, const dmav2_transfer_config_t *transfer_config)
{
    dmav2_status_t status;
    dmav2_hal_status_t hal_status;
    uint32_t addr_offset_value;
    
    if (!g_dmav2_driver_initialized) {
        return DMAV2_ERROR_NOT_INITIALIZED;
    }
    
    status = dmav2_validate_channel(channel);
    if (status != DMAV2_SUCCESS) {
        return status;
    }
    
    status = dmav2_validate_transfer_config(transfer_config);
    if (status != DMAV2_SUCCESS) {
        return status;
    }
    
    if (g_channel_states[channel] == DMAV2_CHANNEL_ACTIVE) {
        return DMAV2_ERROR_BUSY;
    }
    
    // Configure destination addresses
    if (transfer_config->dst_start_addr != 0) {
        hal_status = dmav2_hal_write_channel_dstart_addr(channel, transfer_config->dst_start_addr);
        if (hal_status != DMAV2_HAL_OK) {
            return DMAV2_ERROR_HARDWARE_FAULT;
        }
    }
    
    if (transfer_config->dst_end_addr != 0) {
        hal_status = dmav2_hal_write_channel_dend_addr(channel, transfer_config->dst_end_addr);
        if (hal_status != DMAV2_HAL_OK) {
            return DMAV2_ERROR_HARDWARE_FAULT;
        }
    }
    
    // Configure address offsets
    addr_offset_value = ((uint32_t)transfer_config->dst_offset << DMAV2_ADDR_OFFSET_DOFFSET_SHIFT) |
                        (transfer_config->src_offset & DMAV2_ADDR_OFFSET_SOFFSET_MASK);
    
    hal_status = dmav2_hal_write_channel_addr_offset(channel, addr_offset_value);
    if (hal_status != DMAV2_HAL_OK) {
        return DMAV2_ERROR_HARDWARE_FAULT;
    }
    
    // Enable channel
    hal_status = dmav2_hal_write_channel_enable(channel, DMAV2_ENABLE_CHAN_EN_MASK);
    if (hal_status != DMAV2_HAL_OK) {
        return DMAV2_ERROR_HARDWARE_FAULT;
    }
    
    g_channel_states[channel] = DMAV2_CHANNEL_ACTIVE;
    
    return DMAV2_SUCCESS;
}

/**
 * @brief Stop DMA transfer
 */
dmav2_status_t dmav2_stop_transfer(uint8_t channel)
{
    dmav2_status_t status;
    dmav2_hal_status_t hal_status;
    
    if (!g_dmav2_driver_initialized) {
        return DMAV2_ERROR_NOT_INITIALIZED;
    }
    
    status = dmav2_validate_channel(channel);
    if (status != DMAV2_SUCCESS) {
        return status;
    }
    
    // Disable channel
    hal_status = dmav2_hal_write_channel_enable(channel, 0);
    if (hal_status != DMAV2_HAL_OK) {
        return DMAV2_ERROR_HARDWARE_FAULT;
    }
    
    g_channel_states[channel] = DMAV2_CHANNEL_IDLE;
    
    return DMAV2_SUCCESS;
}

/**
 * @brief Get channel status
 */
dmav2_status_t dmav2_get_channel_status(uint8_t channel, dmav2_channel_status_t *status)
{
    dmav2_status_t ret_status;
    dmav2_hal_status_t hal_status;
    uint32_t status_reg, data_trans_reg, fifo_left_reg;
    
    if (!g_dmav2_driver_initialized) {
        return DMAV2_ERROR_NOT_INITIALIZED;
    }
    
    ret_status = dmav2_validate_channel(channel);
    if (ret_status != DMAV2_SUCCESS) {
        return ret_status;
    }
    
    if (status == NULL) {
        return DMAV2_ERROR_INVALID_PARAM;
    }
    
    // Read hardware status
    hal_status = dmav2_hal_read_channel_status(channel, &status_reg);
    if (hal_status != DMAV2_HAL_OK) {
        return DMAV2_ERROR_HARDWARE_FAULT;
    }
    
    hal_status = dmav2_hal_read_channel_data_trans_num(channel, &data_trans_reg);
    if (hal_status != DMAV2_HAL_OK) {
        return DMAV2_ERROR_HARDWARE_FAULT;
    }
    
    hal_status = dmav2_hal_read_channel_inter_fifo_data_left_num(channel, &fifo_left_reg);
    if (hal_status != DMAV2_HAL_OK) {
        return DMAV2_ERROR_HARDWARE_FAULT;
    }
    
    // Fill status structure
    status->state = g_channel_states[channel];
    status->transfer_complete = (status_reg & DMAV2_STATUS_FINISH_MASK) ? true : false;
    status->half_complete = (status_reg & DMAV2_STATUS_HALF_FINISH_MASK) ? true : false;
    status->error_occurred = (status_reg & (DMAV2_STATUS_CLE_MASK | DMAV2_STATUS_SAE_MASK | 
                                           DMAV2_STATUS_DAE_MASK | DMAV2_STATUS_SOE_MASK |
                                           DMAV2_STATUS_DOE_MASK | DMAV2_STATUS_SBE_MASK |
                                           DMAV2_STATUS_DBE_MASK)) ? true : false;
    
    // Extract error flags
    status->error_flags = 0;
    if (status_reg & DMAV2_STATUS_CLE_MASK) status->error_flags |= DMAV2_ERROR_CHANNEL_LENGTH;
    if (status_reg & DMAV2_STATUS_SAE_MASK) status->error_flags |= DMAV2_ERROR_SOURCE_ADDRESS;
    if (status_reg & DMAV2_STATUS_DAE_MASK) status->error_flags |= DMAV2_ERROR_DEST_ADDRESS;
    if (status_reg & DMAV2_STATUS_SOE_MASK) status->error_flags |= DMAV2_ERROR_SOURCE_OFFSET;
    if (status_reg & DMAV2_STATUS_DOE_MASK) status->error_flags |= DMAV2_ERROR_DEST_OFFSET;
    if (status_reg & DMAV2_STATUS_SBE_MASK) status->error_flags |= DMAV2_ERROR_SOURCE_BUS;
    if (status_reg & DMAV2_STATUS_DBE_MASK) status->error_flags |= DMAV2_ERROR_DEST_BUS;
    
    status->transferred_count = data_trans_reg & 0xFFFF;
    status->fifo_data_left = fifo_left_reg & 0x3F;
    
    return DMAV2_SUCCESS;
}

/**
 * @brief Memory-to-Memory transfer convenience function
 */
dmav2_status_t dmav2_mem_to_mem_transfer(uint8_t channel, 
                                        uint32_t src_addr, 
                                        uint32_t dst_addr, 
                                        uint16_t count,
                                        dmav2_data_width_t data_width)
{
    dmav2_status_t status;
    dmav2_channel_config_t config;
    dmav2_transfer_config_t transfer_config;
    
    // Setup channel configuration
    memset(&config, 0, sizeof(config));
    config.mode = DMAV2_MODE_MEM2MEM;
    config.src_width = data_width;
    config.dst_width = data_width;
    config.priority = DMAV2_PRIORITY_MEDIUM;
    config.src_addr_mode = DMAV2_ADDR_INCREMENT;
    config.dst_addr_mode = DMAV2_ADDR_INCREMENT;
    config.dmamux_req_id = 0;  // Always enabled for mem2mem
    
    status = dmav2_channel_configure(channel, &config);
    if (status != DMAV2_SUCCESS) {
        return status;
    }
    
    // Setup transfer configuration
    memset(&transfer_config, 0, sizeof(transfer_config));
    transfer_config.src_addr = src_addr;
    transfer_config.dst_addr = dst_addr;
    transfer_config.transfer_count = count;
    
    // Calculate appropriate offsets based on data width
    uint16_t offset = (data_width == DMAV2_DATA_WIDTH_8BIT) ? 1 :
                     (data_width == DMAV2_DATA_WIDTH_16BIT) ? 2 : 4;
    transfer_config.src_offset = offset;
    transfer_config.dst_offset = offset;
    
    return dmav2_start_transfer(channel, &transfer_config);
}

/**
 * @brief Get error string
 */
const char* dmav2_get_error_string(dmav2_status_t status)
{
    if (status < sizeof(dmav2_error_strings) / sizeof(dmav2_error_strings[0])) {
        return dmav2_error_strings[status];
    }
    return "Unknown error";
}

/**
 * @brief Get version string
 */
const char* dmav2_get_version_string(void)
{
    return g_version_string;
}

/**
 * @brief Set channel priority
 */
dmav2_status_t dmav2_set_channel_priority(uint8_t channel, dmav2_priority_t priority)
{
    dmav2_status_t status;
    
    status = dmav2_validate_channel(channel);
    if (status != DMAV2_SUCCESS) {
        return status;
    }
    
    if (priority > DMAV2_PRIORITY_VERY_HIGH) {
        return DMAV2_ERROR_INVALID_PARAM;
    }
    
    g_channel_configs[channel].priority = priority;
    
    return DMAV2_SUCCESS;
}

/**
 * @brief Test memory-to-memory mode
 */
dmav2_status_t dmav2_test_mem2mem_mode(uint8_t channel)
{
    uint32_t src_addr = 0x20000000;   // Example source address
    uint32_t dst_addr = 0x20001000;   // Example destination address
    uint16_t count = 256;             // Transfer 256 words
    
    return dmav2_mem_to_mem_transfer(channel, src_addr, dst_addr, count, DMAV2_DATA_WIDTH_32BIT);
}

/**
 * @brief Test memory-to-peripheral mode
 */
dmav2_status_t dmav2_test_mem2peri_mode(uint8_t channel)
{
    dmav2_channel_config_t config;
    dmav2_transfer_config_t transfer_config;
    dmav2_status_t status;
    
    // Setup peripheral transfer configuration
    memset(&config, 0, sizeof(config));
    config.mode = DMAV2_MODE_MEM2PERI;
    config.src_width = DMAV2_DATA_WIDTH_32BIT;
    config.dst_width = DMAV2_DATA_WIDTH_32BIT;
    config.src_addr_mode = DMAV2_ADDR_INCREMENT;
    config.dst_addr_mode = DMAV2_ADDR_FIXED;  // Fixed peripheral address
    config.dmamux_req_id = 2;  // Example: UART TX request
    
    status = dmav2_channel_configure(channel, &config);
    if (status != DMAV2_SUCCESS) {
        return status;
    }
    
    memset(&transfer_config, 0, sizeof(transfer_config));
    transfer_config.src_addr = 0x20000000;    // Memory source
    transfer_config.dst_addr = 0x40000000;    // Peripheral register
    transfer_config.transfer_count = 128;
    transfer_config.src_offset = 4;           // Increment source
    transfer_config.dst_offset = 0;           // Fixed destination
    
    return dmav2_start_transfer(channel, &transfer_config);
}

/**
 * @brief Test peripheral-to-memory mode
 */
dmav2_status_t dmav2_test_peri2mem_mode(uint8_t channel)
{
    dmav2_channel_config_t config;
    dmav2_transfer_config_t transfer_config;
    dmav2_status_t status;
    
    // Setup peripheral transfer configuration
    memset(&config, 0, sizeof(config));
    config.mode = DMAV2_MODE_PERI2MEM;
    config.src_width = DMAV2_DATA_WIDTH_32BIT;
    config.dst_width = DMAV2_DATA_WIDTH_32BIT;
    config.src_addr_mode = DMAV2_ADDR_FIXED;     // Fixed peripheral address
    config.dst_addr_mode = DMAV2_ADDR_INCREMENT; // Increment memory address
    config.dmamux_req_id = 1;  // Example: UART RX request
    
    status = dmav2_channel_configure(channel, &config);
    if (status != DMAV2_SUCCESS) {
        return status;
    }
    
    memset(&transfer_config, 0, sizeof(transfer_config));
    transfer_config.src_addr = 0x40000004;    // Peripheral register
    transfer_config.dst_addr = 0x20001000;    // Memory destination
    transfer_config.transfer_count = 128;
    transfer_config.src_offset = 0;           // Fixed source
    transfer_config.dst_offset = 4;           // Increment destination
    
    return dmav2_start_transfer(channel, &transfer_config);
}

/**
 * @brief Test address fixed mode
 */
dmav2_status_t dmav2_test_address_fixed_mode(uint8_t channel)
{
    dmav2_channel_config_t config;
    dmav2_transfer_config_t transfer_config;
    dmav2_status_t status;
    
    // Configure for fixed address mode
    memset(&config, 0, sizeof(config));
    config.mode = DMAV2_MODE_MEM2MEM;
    config.src_width = DMAV2_DATA_WIDTH_32BIT;
    config.dst_width = DMAV2_DATA_WIDTH_32BIT;
    config.src_addr_mode = DMAV2_ADDR_FIXED;  // Source fixed
    config.dst_addr_mode = DMAV2_ADDR_FIXED;  // Destination fixed
    
    status = dmav2_channel_configure(channel, &config);
    if (status != DMAV2_SUCCESS) {
        return status;
    }
    
    memset(&transfer_config, 0, sizeof(transfer_config));
    transfer_config.src_addr = 0x20000000;
    transfer_config.dst_addr = 0x20001000;
    transfer_config.transfer_count = 10;
    transfer_config.src_offset = 0;           // No increment
    transfer_config.dst_offset = 0;           // No increment
    
    return dmav2_start_transfer(channel, &transfer_config);
}

/**
 * @brief Test address increment mode
 */
dmav2_status_t dmav2_test_address_increment_mode(uint8_t channel)
{
    dmav2_channel_config_t config;
    dmav2_transfer_config_t transfer_config;
    dmav2_status_t status;
    
    // Configure for increment address mode
    memset(&config, 0, sizeof(config));
    config.mode = DMAV2_MODE_MEM2MEM;
    config.src_width = DMAV2_DATA_WIDTH_32BIT;
    config.dst_width = DMAV2_DATA_WIDTH_32BIT;
    config.src_addr_mode = DMAV2_ADDR_INCREMENT;
    config.dst_addr_mode = DMAV2_ADDR_INCREMENT;
    
    status = dmav2_channel_configure(channel, &config);
    if (status != DMAV2_SUCCESS) {
        return status;
    }
    
    memset(&transfer_config, 0, sizeof(transfer_config));
    transfer_config.src_addr = 0x20000000;
    transfer_config.dst_addr = 0x20001000;
    transfer_config.transfer_count = 256;
    transfer_config.src_offset = 4;           // 32-bit increment
    transfer_config.dst_offset = 4;           // 32-bit increment
    
    return dmav2_start_transfer(channel, &transfer_config);
}

/**
 * @brief Test circular mode
 */
dmav2_status_t dmav2_test_circular_mode(uint8_t channel)
{
    dmav2_channel_config_t config;
    dmav2_transfer_config_t transfer_config;
    dmav2_status_t status;
    
    // Configure for circular mode
    memset(&config, 0, sizeof(config));
    config.mode = DMAV2_MODE_MEM2MEM;
    config.src_width = DMAV2_DATA_WIDTH_32BIT;
    config.dst_width = DMAV2_DATA_WIDTH_32BIT;
    config.src_addr_mode = DMAV2_ADDR_INCREMENT;
    config.dst_addr_mode = DMAV2_ADDR_INCREMENT;
    config.circular_mode = true;              // Enable circular mode
    
    status = dmav2_channel_configure(channel, &config);
    if (status != DMAV2_SUCCESS) {
        return status;
    }
    
    memset(&transfer_config, 0, sizeof(transfer_config));
    transfer_config.src_addr = 0x20000000;
    transfer_config.dst_addr = 0x20001000;
    transfer_config.dst_start_addr = 0x20001000;  // Circular start
    transfer_config.dst_end_addr = 0x20001400;    // Circular end (1KB buffer)
    transfer_config.transfer_count = 500;         // More than buffer size to test wrap
    transfer_config.src_offset = 4;
    transfer_config.dst_offset = 4;
    
    return dmav2_start_transfer(channel, &transfer_config);
}

/**
 * @brief Self-test function
 */
dmav2_status_t dmav2_self_test(void)
{
    dmav2_status_t status;
    dmav2_global_status_t global_status;
    
    if (!g_dmav2_driver_initialized) {
        return DMAV2_ERROR_NOT_INITIALIZED;
    }
    
    // Test global status read
    status = dmav2_get_global_status(&global_status);
    if (status != DMAV2_SUCCESS) {
        return status;
    }
    
    // Test channel initialization
    status = dmav2_channel_init(0);
    if (status != DMAV2_SUCCESS) {
        return status;
    }
    
    printf("DMAv2 self-test passed\n");
    return DMAV2_SUCCESS;
}