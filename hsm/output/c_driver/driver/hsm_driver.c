/**
 * @file hsm_driver.c
 * @brief HSM Driver Layer implementation
 *
 * This module provides high-level HSM driver functions with business logic
 * and configuration management. It builds on top of the HAL layer.
 */

#include "hsm_driver.h"
#include "../hal/hsm_hal.h"
#include <string.h>
#include <time.h>

/**
 * @brief Driver state
 */
static bool g_hsm_driver_initialized = false;
static hsm_context_t g_hsm_contexts[HSM_MAX_CONTEXTS];

/**
 * @brief Helper function to validate context ID
 */
static bool is_valid_context_id(uint32_t context_id)
{
    return (context_id < HSM_MAX_CONTEXTS);
}

/**
 * @brief Helper function to get context
 */
static hsm_context_t* get_context(uint32_t context_id)
{
    if (!is_valid_context_id(context_id)) {
        return NULL;
    }
    return &g_hsm_contexts[context_id];
}

/**
 * @brief Helper function to convert HAL status to driver status
 */
static hsm_drv_status_t hal_to_drv_status(hsm_hal_status_t hal_status)
{
    switch (hal_status) {
        case HSM_HAL_OK:
            return HSM_DRV_OK;
        case HSM_HAL_INVALID_PARAM:
            return HSM_DRV_INVALID_PARAM;
        case HSM_HAL_NOT_INITIALIZED:
            return HSM_DRV_NOT_INITIALIZED;
        case HSM_HAL_HARDWARE_ERROR:
            return HSM_DRV_HARDWARE_ERROR;
        default:
            return HSM_DRV_ERROR;
    }
}

/**
 * @brief Helper function to wait for operation completion
 */
static hsm_drv_status_t wait_for_operation(uint32_t timeout_ms)
{
    uint32_t status;
    uint32_t start_time = (uint32_t)time(NULL) * 1000; // Convert to ms
    uint32_t current_time;
    
    do {
        hsm_hal_status_t hal_status = hsm_hal_read_sys_status(&status);
        if (hal_status != HSM_HAL_OK) {
            return hal_to_drv_status(hal_status);
        }
        
        // Check if operation is complete
        if ((status & HSM_SYS_STATUS_HSM_CMD_DONE_MASK) != 0) {
            // Check if operation succeeded
            if ((status & HSM_SYS_STATUS_HSM_CMD_DONE_MASK) == 2) {
                return HSM_DRV_OK;
            } else {
                return HSM_DRV_ERROR;
            }
        }
        
        current_time = (uint32_t)time(NULL) * 1000;
    } while ((current_time - start_time) < timeout_ms);
    
    return HSM_DRV_TIMEOUT;
}

hsm_drv_status_t hsm_driver_base_address_init(uint32_t base_address)
{
    hsm_hal_status_t hal_status = hsm_hal_base_address_init(base_address);
    return hal_to_drv_status(hal_status);
}

hsm_drv_status_t hsm_driver_init(void)
{
    if (g_hsm_driver_initialized) {
        return HSM_DRV_OK;
    }
    
    // Initialize HAL
    hsm_hal_status_t hal_status = hsm_hal_init();
    if (hal_status != HSM_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    // Initialize contexts
    memset(g_hsm_contexts, 0, sizeof(g_hsm_contexts));
    for (uint32_t i = 0; i < HSM_MAX_CONTEXTS; i++) {
        g_hsm_contexts[i].context_id = i;
        g_hsm_contexts[i].initialized = false;
        g_hsm_contexts[i].operation_pending = false;
    }
    
    g_hsm_driver_initialized = true;
    return HSM_DRV_OK;
}

hsm_drv_status_t hsm_driver_deinit(void)
{
    if (!g_hsm_driver_initialized) {
        return HSM_DRV_NOT_INITIALIZED;
    }
    
    // Deinitialize HAL
    hsm_hal_status_t hal_status = hsm_hal_deinit();
    if (hal_status != HSM_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    g_hsm_driver_initialized = false;
    return HSM_DRV_OK;
}

hsm_drv_status_t hsm_driver_get_default_config(hsm_mode_t mode, hsm_config_t *config)
{
    if (config == NULL) {
        return HSM_DRV_INVALID_PARAM;
    }
    
    // Clear configuration
    memset(config, 0, sizeof(hsm_config_t));
    
    // Set default values
    config->mode = mode;
    config->decrypt_enable = false;
    config->key_index = 0;
    config->aes256_enable = false;
    config->data_length = 16; // Default block size
    config->dma_enable = false;
    config->interrupt_enable = false;
    
    // Set default IV (all zeros)
    memset(config->iv, 0, sizeof(config->iv));
    
    // Set default key (all zeros)
    memset(config->key, 0, sizeof(config->key));
    
    return HSM_DRV_OK;
}

hsm_drv_status_t hsm_driver_configure_context(uint32_t context_id, const hsm_config_t *config)
{
    if (!g_hsm_driver_initialized) {
        return HSM_DRV_NOT_INITIALIZED;
    }
    
    if (config == NULL) {
        return HSM_DRV_INVALID_PARAM;
    }
    
    hsm_context_t *context = get_context(context_id);
    if (context == NULL) {
        return HSM_DRV_INVALID_PARAM;
    }
    
    // Validate configuration parameters
    if (config->mode > HSM_MODE_RANDOM) {
        return HSM_DRV_INVALID_PARAM;
    }
    
    if (config->key_index > 22) {
        return HSM_DRV_INVALID_PARAM;
    }
    
    if (config->data_length > 0xFFFFF) {
        return HSM_DRV_INVALID_PARAM;
    }
    
    // Copy configuration
    memcpy(&context->config, config, sizeof(hsm_config_t));
    context->initialized = true;
    
    return HSM_DRV_OK;
}

hsm_drv_status_t hsm_driver_get_context_config(uint32_t context_id, hsm_config_t *config)
{
    if (!g_hsm_driver_initialized) {
        return HSM_DRV_NOT_INITIALIZED;
    }
    
    if (config == NULL) {
        return HSM_DRV_INVALID_PARAM;
    }
    
    hsm_context_t *context = get_context(context_id);
    if (context == NULL || !context->initialized) {
        return HSM_DRV_INVALID_PARAM;
    }
    
    memcpy(config, &context->config, sizeof(hsm_config_t));
    return HSM_DRV_OK;
}

hsm_drv_status_t hsm_driver_aes_operation(uint32_t context_id,
                                          const uint8_t *input_data, uint32_t input_len,
                                          uint8_t *output_data, uint32_t *output_len)
{
    if (!g_hsm_driver_initialized) {
        return HSM_DRV_NOT_INITIALIZED;
    }
    
    if (input_data == NULL || output_data == NULL || output_len == NULL) {
        return HSM_DRV_INVALID_PARAM;
    }
    
    hsm_context_t *context = get_context(context_id);
    if (context == NULL || !context->initialized) {
        return HSM_DRV_INVALID_PARAM;
    }
    
    if (context->operation_pending) {
        return HSM_DRV_BUSY;
    }
    
    hsm_config_t *config = &context->config;
    
    // Validate mode for AES operation
    if (config->mode > HSM_MODE_CTR) {
        return HSM_DRV_INVALID_PARAM;
    }
    
    // Configure system control
    hsm_hal_status_t hal_status = hsm_hal_write_sys_ctrl(config->interrupt_enable, config->dma_enable);
    if (hal_status != HSM_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    // Configure IV if needed
    if (config->mode != HSM_MODE_ECB) {
        for (int i = 0; i < 4; i++) {
            uint32_t iv_word = (config->iv[i*4 + 3] << 24) |
                              (config->iv[i*4 + 2] << 16) |
                              (config->iv[i*4 + 1] << 8) |
                              (config->iv[i*4 + 0]);
            hal_status = hsm_hal_write_iv(i, iv_word);
            if (hal_status != HSM_HAL_OK) {
                return hal_to_drv_status(hal_status);
            }
        }
    }
    
    // Configure RAM key if needed
    if (config->key_index == HSM_KEY_TYPE_RAM) {
        uint32_t key_len = config->aes256_enable ? 8 : 4;
        for (uint32_t i = 0; i < key_len; i++) {
            uint32_t key_word = (config->key[i*4 + 3] << 24) |
                               (config->key[i*4 + 2] << 16) |
                               (config->key[i*4 + 1] << 8) |
                               (config->key[i*4 + 0]);
            hal_status = hsm_hal_write_ram_key(i, key_word);
            if (hal_status != HSM_HAL_OK) {
                return hal_to_drv_status(hal_status);
            }
        }
    }
    
    // Configure command
    hal_status = hsm_hal_write_cmd(config->mode, config->decrypt_enable,
                                   config->key_index, config->aes256_enable, input_len);
    if (hal_status != HSM_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    context->operation_pending = true;
    
    // Write input data
    uint32_t words_to_write = (input_len + 3) / 4;
    for (uint32_t i = 0; i < words_to_write; i++) {
        uint32_t data_word = 0;
        for (int j = 0; j < 4 && (i*4 + j) < input_len; j++) {
            data_word |= ((uint32_t)input_data[i*4 + j]) << (j * 8);
        }
        
        hal_status = hsm_hal_write_cipher_in(data_word);
        if (hal_status != HSM_HAL_OK) {
            context->operation_pending = false;
            return hal_to_drv_status(hal_status);
        }
    }
    
    // Start operation
    hal_status = hsm_hal_start_operation();
    if (hal_status != HSM_HAL_OK) {
        context->operation_pending = false;
        return hal_to_drv_status(hal_status);
    }
    
    // Wait for completion
    hsm_drv_status_t drv_status = wait_for_operation(HSM_DEFAULT_TIMEOUT_MS);
    if (drv_status != HSM_DRV_OK) {
        context->operation_pending = false;
        return drv_status;
    }
    
    // Read output data
    uint32_t output_words = (input_len + 3) / 4;
    uint32_t bytes_read = 0;
    
    for (uint32_t i = 0; i < output_words && bytes_read < *output_len; i++) {
        uint32_t data_word;
        hal_status = hsm_hal_read_cipher_out(&data_word);
        if (hal_status != HSM_HAL_OK) {
            context->operation_pending = false;
            return hal_to_drv_status(hal_status);
        }
        
        for (int j = 0; j < 4 && bytes_read < *output_len && bytes_read < input_len; j++) {
            output_data[bytes_read] = (data_word >> (j * 8)) & 0xFF;
            bytes_read++;
        }
    }
    
    *output_len = bytes_read;
    context->operation_pending = false;
    
    return HSM_DRV_OK;
}

hsm_drv_status_t hsm_driver_compute_cmac(uint32_t context_id,
                                         const uint8_t *input_data, uint32_t input_len,
                                         uint8_t *mac_output)
{
    if (!g_hsm_driver_initialized) {
        return HSM_DRV_NOT_INITIALIZED;
    }
    
    if (input_data == NULL || mac_output == NULL) {
        return HSM_DRV_INVALID_PARAM;
    }
    
    hsm_context_t *context = get_context(context_id);
    if (context == NULL || !context->initialized) {
        return HSM_DRV_INVALID_PARAM;
    }
    
    if (context->operation_pending) {
        return HSM_DRV_BUSY;
    }
    
    hsm_config_t *config = &context->config;
    
    // Configure system control
    hsm_hal_status_t hal_status = hsm_hal_write_sys_ctrl(config->interrupt_enable, config->dma_enable);
    if (hal_status != HSM_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    // Configure RAM key if needed
    if (config->key_index == HSM_KEY_TYPE_RAM) {
        uint32_t key_len = config->aes256_enable ? 8 : 4;
        for (uint32_t i = 0; i < key_len; i++) {
            uint32_t key_word = (config->key[i*4 + 3] << 24) |
                               (config->key[i*4 + 2] << 16) |
                               (config->key[i*4 + 1] << 8) |
                               (config->key[i*4 + 0]);
            hal_status = hsm_hal_write_ram_key(i, key_word);
            if (hal_status != HSM_HAL_OK) {
                return hal_to_drv_status(hal_status);
            }
        }
    }
    
    // Configure command for CMAC
    hal_status = hsm_hal_write_cmd(HSM_MODE_CMAC, false,
                                   config->key_index, config->aes256_enable, input_len);
    if (hal_status != HSM_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    context->operation_pending = true;
    
    // Write input data
    uint32_t words_to_write = (input_len + 3) / 4;
    for (uint32_t i = 0; i < words_to_write; i++) {
        uint32_t data_word = 0;
        for (int j = 0; j < 4 && (i*4 + j) < input_len; j++) {
            data_word |= ((uint32_t)input_data[i*4 + j]) << (j * 8);
        }
        
        hal_status = hsm_hal_write_cipher_in(data_word);
        if (hal_status != HSM_HAL_OK) {
            context->operation_pending = false;
            return hal_to_drv_status(hal_status);
        }
    }
    
    // Start operation
    hal_status = hsm_hal_start_operation();
    if (hal_status != HSM_HAL_OK) {
        context->operation_pending = false;
        return hal_to_drv_status(hal_status);
    }
    
    // Wait for completion
    hsm_drv_status_t drv_status = wait_for_operation(HSM_DEFAULT_TIMEOUT_MS);
    if (drv_status != HSM_DRV_OK) {
        context->operation_pending = false;
        return drv_status;
    }
    
    // Read MAC output (16 bytes)
    for (uint32_t i = 0; i < 4; i++) {
        uint32_t data_word;
        hal_status = hsm_hal_read_cipher_out(&data_word);
        if (hal_status != HSM_HAL_OK) {
            context->operation_pending = false;
            return hal_to_drv_status(hal_status);
        }
        
        mac_output[i*4 + 0] = (data_word >> 0) & 0xFF;
        mac_output[i*4 + 1] = (data_word >> 8) & 0xFF;
        mac_output[i*4 + 2] = (data_word >> 16) & 0xFF;
        mac_output[i*4 + 3] = (data_word >> 24) & 0xFF;
    }
    
    context->operation_pending = false;
    return HSM_DRV_OK;
}

hsm_drv_status_t hsm_driver_verify_cmac(uint32_t context_id,
                                        const uint8_t *input_data, uint32_t input_len,
                                        const uint8_t *expected_mac, bool *verify_result)
{
    if (!g_hsm_driver_initialized) {
        return HSM_DRV_NOT_INITIALIZED;
    }
    
    if (input_data == NULL || expected_mac == NULL || verify_result == NULL) {
        return HSM_DRV_INVALID_PARAM;
    }
    
    hsm_context_t *context = get_context(context_id);
    if (context == NULL || !context->initialized) {
        return HSM_DRV_INVALID_PARAM;
    }
    
    // Set expected MAC in golden MAC registers
    for (uint32_t i = 0; i < 4; i++) {
        uint32_t mac_word = (expected_mac[i*4 + 3] << 24) |
                           (expected_mac[i*4 + 2] << 16) |
                           (expected_mac[i*4 + 1] << 8) |
                           (expected_mac[i*4 + 0]);
        hsm_hal_status_t hal_status = hsm_hal_write_golden_mac(i, mac_word);
        if (hal_status != HSM_HAL_OK) {
            return hal_to_drv_status(hal_status);
        }
    }
    
    // Compute CMAC and compare automatically (when key_idx = 1)
    uint8_t computed_mac[16];
    hsm_drv_status_t drv_status = hsm_driver_compute_cmac(context_id, input_data, input_len, computed_mac);
    if (drv_status != HSM_DRV_OK) {
        return drv_status;
    }
    
    // Compare MACs
    *verify_result = (memcmp(computed_mac, expected_mac, 16) == 0);
    
    return HSM_DRV_OK;
}

hsm_drv_status_t hsm_driver_generate_random(uint8_t *output_data, uint32_t output_len)
{
    if (!g_hsm_driver_initialized) {
        return HSM_DRV_NOT_INITIALIZED;
    }
    
    if (output_data == NULL || output_len == 0) {
        return HSM_DRV_INVALID_PARAM;
    }
    
    // Configure command for random number generation
    hsm_hal_status_t hal_status = hsm_hal_write_cmd(HSM_MODE_RANDOM, false, 0, false, output_len);
    if (hal_status != HSM_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    // Start operation
    hal_status = hsm_hal_start_operation();
    if (hal_status != HSM_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    // Wait for completion
    hsm_drv_status_t drv_status = wait_for_operation(HSM_DEFAULT_TIMEOUT_MS);
    if (drv_status != HSM_DRV_OK) {
        return drv_status;
    }
    
    // Read random data
    uint32_t words_to_read = (output_len + 3) / 4;
    uint32_t bytes_read = 0;
    
    for (uint32_t i = 0; i < words_to_read && bytes_read < output_len; i++) {
        uint32_t data_word;
        hal_status = hsm_hal_read_cipher_out(&data_word);
        if (hal_status != HSM_HAL_OK) {
            return hal_to_drv_status(hal_status);
        }
        
        for (int j = 0; j < 4 && bytes_read < output_len; j++) {
            output_data[bytes_read] = (data_word >> (j * 8)) & 0xFF;
            bytes_read++;
        }
    }
    
    return HSM_DRV_OK;
}

hsm_drv_status_t hsm_driver_start_authentication(hsm_auth_result_t *auth_result)
{
    if (!g_hsm_driver_initialized) {
        return HSM_DRV_NOT_INITIALIZED;
    }
    
    if (auth_result == NULL) {
        return HSM_DRV_INVALID_PARAM;
    }
    
    // Clear result structure
    memset(auth_result, 0, sizeof(hsm_auth_result_t));
    
    // Configure command for authentication
    hsm_hal_status_t hal_status = hsm_hal_write_cmd(HSM_MODE_AUTH, false, 0, false, 0);
    if (hal_status != HSM_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    // Start operation
    hal_status = hsm_hal_start_operation();
    if (hal_status != HSM_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    // Wait for completion
    hsm_drv_status_t drv_status = wait_for_operation(HSM_DEFAULT_TIMEOUT_MS);
    if (drv_status != HSM_DRV_OK) {
        return drv_status;
    }
    
    // Read authentication status
    uint32_t auth_status;
    hal_status = hsm_hal_read_auth_status(&auth_status);
    if (hal_status != HSM_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    auth_result->challenge_valid = (auth_status & HSM_AUTH_STATUS_AUTH_CHALLENGE_VALID) != 0;
    auth_result->auth_timeout = (auth_status & HSM_AUTH_STATUS_DEBUG_UNLOCK_AUTH_TMO_FAIL) != 0;
    
    // Read challenge if valid
    if (auth_result->challenge_valid) {
        for (uint32_t i = 0; i < 4; i++) {
            uint32_t challenge_word;
            hal_status = hsm_hal_read_auth_challenge(i, &challenge_word);
            if (hal_status != HSM_HAL_OK) {
                return hal_to_drv_status(hal_status);
            }
            
            auth_result->challenge[i*4 + 0] = (challenge_word >> 0) & 0xFF;
            auth_result->challenge[i*4 + 1] = (challenge_word >> 8) & 0xFF;
            auth_result->challenge[i*4 + 2] = (challenge_word >> 16) & 0xFF;
            auth_result->challenge[i*4 + 3] = (challenge_word >> 24) & 0xFF;
        }
    }
    
    return HSM_DRV_OK;
}

hsm_drv_status_t hsm_driver_submit_auth_response(const uint8_t *response,
                                                 hsm_auth_result_t *auth_result)
{
    if (!g_hsm_driver_initialized) {
        return HSM_DRV_NOT_INITIALIZED;
    }
    
    if (response == NULL || auth_result == NULL) {
        return HSM_DRV_INVALID_PARAM;
    }
    
    // Write authentication response
    for (uint32_t i = 0; i < 4; i++) {
        uint32_t resp_word = (response[i*4 + 3] << 24) |
                            (response[i*4 + 2] << 16) |
                            (response[i*4 + 1] << 8) |
                            (response[i*4 + 0]);
        hsm_hal_status_t hal_status = hsm_hal_write_auth_resp(i, resp_word);
        if (hal_status != HSM_HAL_OK) {
            return hal_to_drv_status(hal_status);
        }
    }
    
    // Trigger response validation
    hsm_hal_status_t hal_status = hsm_hal_write_raw(HSM_REG_AUTH_CTRL, 1);
    if (hal_status != HSM_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    // Read authentication status
    uint32_t auth_status;
    hal_status = hsm_hal_read_auth_status(&auth_status);
    if (hal_status != HSM_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    auth_result->auth_success = (auth_status & HSM_AUTH_STATUS_DEBUG_UNLOCK_AUTH_PASS) != 0;
    auth_result->auth_timeout = (auth_status & HSM_AUTH_STATUS_DEBUG_UNLOCK_AUTH_TMO_FAIL) != 0;
    
    return HSM_DRV_OK;
}

hsm_drv_status_t hsm_driver_is_operation_complete(uint32_t context_id, bool *completed)
{
    if (!g_hsm_driver_initialized) {
        return HSM_DRV_NOT_INITIALIZED;
    }
    
    if (completed == NULL) {
        return HSM_DRV_INVALID_PARAM;
    }
    
    hsm_context_t *context = get_context(context_id);
    if (context == NULL) {
        return HSM_DRV_INVALID_PARAM;
    }
    
    if (!context->operation_pending) {
        *completed = true;
        return HSM_DRV_OK;
    }
    
    uint32_t status;
    hsm_hal_status_t hal_status = hsm_hal_read_sys_status(&status);
    if (hal_status != HSM_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    *completed = (status & HSM_SYS_STATUS_HSM_CMD_DONE_MASK) != 0;
    
    if (*completed) {
        context->operation_pending = false;
    }
    
    return HSM_DRV_OK;
}

hsm_drv_status_t hsm_driver_wait_for_completion(uint32_t context_id, uint32_t timeout_ms)
{
    if (!g_hsm_driver_initialized) {
        return HSM_DRV_NOT_INITIALIZED;
    }
    
    hsm_context_t *context = get_context(context_id);
    if (context == NULL) {
        return HSM_DRV_INVALID_PARAM;
    }
    
    if (!context->operation_pending) {
        return HSM_DRV_OK;
    }
    
    return wait_for_operation(timeout_ms);
}

hsm_drv_status_t hsm_driver_get_status(bool *tx_fifo_empty, bool *rx_fifo_full, bool *operation_busy)
{
    if (!g_hsm_driver_initialized) {
        return HSM_DRV_NOT_INITIALIZED;
    }
    
    uint32_t status;
    hsm_hal_status_t hal_status = hsm_hal_read_sys_status(&status);
    if (hal_status != HSM_HAL_OK) {
        return hal_to_drv_status(hal_status);
    }
    
    if (tx_fifo_empty != NULL) {
        *tx_fifo_empty = (status & HSM_SYS_STATUS_TX_FIFO_EMPTY) != 0;
    }
    
    if (rx_fifo_full != NULL) {
        *rx_fifo_full = (status & HSM_SYS_STATUS_RX_FIFO_FULL) != 0;
    }
    
    if (operation_busy != NULL) {
        *operation_busy = (status & HSM_SYS_STATUS_CPU_CMD_ON_GOING) != 0;
    }
    
    return HSM_DRV_OK;
}

hsm_drv_status_t hsm_driver_get_uid(uint8_t *uid_buffer)
{
    if (!g_hsm_driver_initialized) {
        return HSM_DRV_NOT_INITIALIZED;
    }
    
    if (uid_buffer == NULL) {
        return HSM_DRV_INVALID_PARAM;
    }
    
    // Read 4 UID registers (16 bytes total)
    for (uint32_t i = 0; i < 4; i++) {
        uint32_t uid_word;
        hsm_hal_status_t hal_status = hsm_hal_read_uid(i, &uid_word);
        if (hal_status != HSM_HAL_OK) {
            return hal_to_drv_status(hal_status);
        }
        
        uid_buffer[i*4 + 0] = (uid_word >> 0) & 0xFF;
        uid_buffer[i*4 + 1] = (uid_word >> 8) & 0xFF;
        uid_buffer[i*4 + 2] = (uid_word >> 16) & 0xFF;
        uid_buffer[i*4 + 3] = (uid_word >> 24) & 0xFF;
    }
    
    return HSM_DRV_OK;
}

hsm_drv_status_t hsm_driver_clear_interrupt(void)
{
    if (!g_hsm_driver_initialized) {
        return HSM_DRV_NOT_INITIALIZED;
    }
    
    hsm_hal_status_t hal_status = hsm_hal_clear_irq();
    return hal_to_drv_status(hal_status);
}

hsm_drv_status_t hsm_driver_get_interrupt_status(uint32_t *irq_status)
{
    if (!g_hsm_driver_initialized) {
        return HSM_DRV_NOT_INITIALIZED;
    }
    
    if (irq_status == NULL) {
        return HSM_DRV_INVALID_PARAM;
    }
    
    hsm_hal_status_t hal_status = hsm_hal_read_irq_status(irq_status);
    return hal_to_drv_status(hal_status);
}

hsm_drv_status_t hsm_driver_reset(void)
{
    if (!g_hsm_driver_initialized) {
        return HSM_DRV_NOT_INITIALIZED;
    }
    
    // Reset all contexts
    for (uint32_t i = 0; i < HSM_MAX_CONTEXTS; i++) {
        g_hsm_contexts[i].initialized = false;
        g_hsm_contexts[i].operation_pending = false;
    }
    
    return HSM_DRV_OK;
}