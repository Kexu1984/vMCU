/**
 * @file hsm_driver.h
 * @brief HSM Driver Layer interface definitions
 *
 * This module provides high-level HSM driver functions with business logic
 * and configuration management. It builds on top of the HAL layer.
 */

#ifndef HSM_DRIVER_H
#define HSM_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief HSM driver status codes
 */
typedef enum {
    HSM_DRV_OK = 0,                 /**< Operation successful */
    HSM_DRV_ERROR,                  /**< General error */
    HSM_DRV_INVALID_PARAM,          /**< Invalid parameter */
    HSM_DRV_NOT_INITIALIZED,        /**< Driver not initialized */
    HSM_DRV_HARDWARE_ERROR,         /**< Hardware error */
    HSM_DRV_TIMEOUT,                /**< Operation timeout */
    HSM_DRV_BUSY                    /**< Device busy */
} hsm_drv_status_t;

/**
 * @brief HSM cipher modes
 */
typedef enum {
    HSM_MODE_ECB = 0,
    HSM_MODE_CBC = 1,
    HSM_MODE_CFB8 = 2,
    HSM_MODE_CFB128 = 3,
    HSM_MODE_OFB = 4,
    HSM_MODE_CTR = 5,
    HSM_MODE_CMAC = 6,
    HSM_MODE_AUTH = 7,
    HSM_MODE_RANDOM = 8
} hsm_mode_t;

/**
 * @brief HSM key types
 */
typedef enum {
    HSM_KEY_TYPE_OTP = 0,           /**< Key from OTP (key index 0-21) */
    HSM_KEY_TYPE_RAM = 22           /**< Key from RAM registers */
} hsm_key_type_t;

/**
 * @brief HSM operation configuration
 */
typedef struct {
    hsm_mode_t mode;                /**< Cipher mode */
    bool decrypt_enable;            /**< Decrypt flag (ECB/CBC only) */
    uint32_t key_index;             /**< Key index (0-22) */
    bool aes256_enable;             /**< AES-256 enable flag */
    uint32_t data_length;           /**< Data length in bytes */
    bool dma_enable;                /**< DMA enable flag */
    bool interrupt_enable;          /**< Interrupt enable flag */
    uint8_t iv[16];                 /**< Initialization vector */
    uint8_t key[32];                /**< RAM key (if key_index == 22) */
} hsm_config_t;

/**
 * @brief HSM context structure
 */
typedef struct {
    uint32_t context_id;            /**< Context identifier */
    hsm_config_t config;            /**< Current configuration */
    bool initialized;               /**< Context initialization flag */
    bool operation_pending;         /**< Operation in progress flag */
} hsm_context_t;

/**
 * @brief HSM authentication result
 */
typedef struct {
    bool challenge_valid;           /**< Challenge available flag */
    bool auth_success;              /**< Authentication success flag */
    bool auth_timeout;              /**< Authentication timeout flag */
    uint8_t challenge[16];          /**< Authentication challenge */
} hsm_auth_result_t;

/**
 * @brief Maximum number of contexts
 */
#define HSM_MAX_CONTEXTS    1

/**
 * @brief Default timeout in milliseconds
 */
#define HSM_DEFAULT_TIMEOUT_MS  5000

/**
 * @brief Initialize driver base address
 * @param base_address Register base address for HSM device
 * @return Driver status code
 */
hsm_drv_status_t hsm_driver_base_address_init(uint32_t base_address);

/**
 * @brief Initialize HSM driver (requires base address already set)
 * @return Driver status code
 */
hsm_drv_status_t hsm_driver_init(void);

/**
 * @brief Deinitialize HSM driver
 * @return Driver status code
 */
hsm_drv_status_t hsm_driver_deinit(void);

/**
 * @brief Get default configuration for HSM operation
 * @param mode Operating mode
 * @param config Pointer to configuration structure
 * @return Driver status code
 */
hsm_drv_status_t hsm_driver_get_default_config(hsm_mode_t mode, hsm_config_t *config);

/**
 * @brief Configure HSM context
 * @param context_id Context identifier (0 to HSM_MAX_CONTEXTS-1)
 * @param config Configuration to apply
 * @return Driver status code
 */
hsm_drv_status_t hsm_driver_configure_context(uint32_t context_id, const hsm_config_t *config);

/**
 * @brief Get HSM context configuration
 * @param context_id Context identifier
 * @param config Pointer to store current configuration
 * @return Driver status code
 */
hsm_drv_status_t hsm_driver_get_context_config(uint32_t context_id, hsm_config_t *config);

/**
 * @brief Perform AES encryption/decryption operation
 * @param context_id Context identifier
 * @param input_data Input data buffer
 * @param input_len Input data length in bytes
 * @param output_data Output data buffer
 * @param output_len Pointer to output length (input: buffer size, output: actual length)
 * @return Driver status code
 */
hsm_drv_status_t hsm_driver_aes_operation(uint32_t context_id,
                                          const uint8_t *input_data, uint32_t input_len,
                                          uint8_t *output_data, uint32_t *output_len);

/**
 * @brief Compute CMAC
 * @param context_id Context identifier
 * @param input_data Input data buffer
 * @param input_len Input data length in bytes
 * @param mac_output MAC output buffer (16 bytes)
 * @return Driver status code
 */
hsm_drv_status_t hsm_driver_compute_cmac(uint32_t context_id,
                                         const uint8_t *input_data, uint32_t input_len,
                                         uint8_t *mac_output);

/**
 * @brief Verify CMAC
 * @param context_id Context identifier
 * @param input_data Input data buffer
 * @param input_len Input data length in bytes
 * @param expected_mac Expected MAC value (16 bytes)
 * @param verify_result Pointer to store verification result
 * @return Driver status code
 */
hsm_drv_status_t hsm_driver_verify_cmac(uint32_t context_id,
                                        const uint8_t *input_data, uint32_t input_len,
                                        const uint8_t *expected_mac, bool *verify_result);

/**
 * @brief Generate random data
 * @param output_data Output buffer for random data
 * @param output_len Length of random data to generate
 * @return Driver status code
 */
hsm_drv_status_t hsm_driver_generate_random(uint8_t *output_data, uint32_t output_len);

/**
 * @brief Start authentication challenge
 * @param auth_result Pointer to store authentication result
 * @return Driver status code
 */
hsm_drv_status_t hsm_driver_start_authentication(hsm_auth_result_t *auth_result);

/**
 * @brief Submit authentication response
 * @param response Authentication response (16 bytes)
 * @param auth_result Pointer to store authentication result
 * @return Driver status code
 */
hsm_drv_status_t hsm_driver_submit_auth_response(const uint8_t *response,
                                                 hsm_auth_result_t *auth_result);

/**
 * @brief Check if HSM operation is complete
 * @param context_id Context identifier
 * @param completed Pointer to store completion status
 * @return Driver status code
 */
hsm_drv_status_t hsm_driver_is_operation_complete(uint32_t context_id, bool *completed);

/**
 * @brief Wait for HSM operation completion with timeout
 * @param context_id Context identifier
 * @param timeout_ms Timeout in milliseconds
 * @return Driver status code
 */
hsm_drv_status_t hsm_driver_wait_for_completion(uint32_t context_id, uint32_t timeout_ms);

/**
 * @brief Get HSM device status
 * @param tx_fifo_empty TX FIFO empty flag
 * @param rx_fifo_full RX FIFO full flag
 * @param operation_busy Operation busy flag
 * @return Driver status code
 */
hsm_drv_status_t hsm_driver_get_status(bool *tx_fifo_empty, bool *rx_fifo_full, bool *operation_busy);

/**
 * @brief Get HSM device UID
 * @param uid_buffer Buffer to store 16-byte UID
 * @return Driver status code
 */
hsm_drv_status_t hsm_driver_get_uid(uint8_t *uid_buffer);

/**
 * @brief Clear HSM interrupt status
 * @return Driver status code
 */
hsm_drv_status_t hsm_driver_clear_interrupt(void);

/**
 * @brief Get HSM interrupt status
 * @param irq_status Pointer to store interrupt status
 * @return Driver status code
 */
hsm_drv_status_t hsm_driver_get_interrupt_status(uint32_t *irq_status);

/**
 * @brief Reset HSM device
 * @return Driver status code
 */
hsm_drv_status_t hsm_driver_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* HSM_DRIVER_H */