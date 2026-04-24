/**
 * @file crc_driver.h
 * @brief CRC Driver Layer Header
 *
 * This file provides the business logic layer for CRC calculations.
 * It implements CRC16-CCITT and CRC32 algorithms with support for
 * multiple contexts, byte/bit ordering, and result formatting.
 */

#ifndef CRC_DRIVER_H
#define CRC_DRIVER_H

#include "hal/crc_hal.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =============================================================================
 * Constants and Definitions
 * =============================================================================*/

/** Maximum data size for single CRC calculation */
#define CRC_MAX_DATA_SIZE       1024U

/** CRC16-CCITT polynomial: x^16 + x^12 + x^5 + 1 */
#define CRC16_CCITT_POLYNOMIAL  0x1021U

/** CRC32-IEEE polynomial: 0x04C11DB7 */
#define CRC32_IEEE_POLYNOMIAL   0x04C11DB7U

/** Default CRC16 initial value */
#define CRC16_INITIAL_VALUE     0xFFFFU

/** Default CRC32 initial value */
#define CRC32_INITIAL_VALUE     0xFFFFFFFFU

/* =============================================================================
 * Data Types
 * =============================================================================*/

/**
 * @brief CRC calculation mode
 */
typedef enum {
    CRC_MODE_CRC16_CCITT = 0U,  /**< CRC16-CCITT mode */
    CRC_MODE_CRC32_IEEE = 1U,   /**< CRC32-IEEE mode */
    CRC_MODE_MAX                /**< Maximum mode value */
} crc_calc_mode_t;

/**
 * @brief CRC context configuration
 */
typedef struct {
    crc_calc_mode_t mode;           /**< CRC calculation mode */
    uint32_t initial_value;         /**< Initial CRC value */
    bool acc_ms_bit_first;          /**< Accumulate MS bit first */
    bool acc_ms_byte_first;         /**< Accumulate MS byte first */
    bool out_ms_bit_first;          /**< Output MS bit first */
    bool out_ms_byte_first;         /**< Output MS byte first */
    bool output_inverted;           /**< Invert final result */
} crc_config_t;

/**
 * @brief CRC context state
 */
typedef struct {
    crc_config_t config;            /**< Context configuration */
    uint32_t current_value;         /**< Current CRC value */
    bool calculation_active;        /**< Calculation in progress */
    bool calculation_complete;      /**< Calculation completed */
    uint32_t data_count;           /**< Number of data bytes processed */
} crc_context_state_t;

/**
 * @brief CRC driver status
 */
typedef enum {
    CRC_DRV_OK = 0U,               /**< Operation successful */
    CRC_DRV_ERROR,                 /**< General error */
    CRC_DRV_INVALID_PARAM,         /**< Invalid parameter */
    CRC_DRV_CONTEXT_BUSY,          /**< Context busy */
    CRC_DRV_NOT_INITIALIZED,       /**< Driver not initialized */
    CRC_DRV_HARDWARE_ERROR         /**< Hardware error */
} crc_drv_status_t;

/* =============================================================================
 * Function Prototypes - Initialization
 * =============================================================================*/

/**
 * @brief Initialize CRC driver base address
 * @param base_address Base address of CRC hardware
 * @return CRC_DRV_OK on success, error code otherwise
 * @note This function must be called before crc_driver_init()
 */
crc_drv_status_t crc_driver_base_address_init(uint32_t base_address);

/**
 * @brief Initialize CRC driver
 * @return CRC_DRV_OK on success, error code otherwise
 * @note crc_driver_base_address_init() must be called first
 */
crc_drv_status_t crc_driver_init(void);

/**
 * @brief Deinitialize CRC driver
 * @return CRC_DRV_OK on success, error code otherwise
 */
crc_drv_status_t crc_driver_deinit(void);

/* =============================================================================
 * Function Prototypes - Context Configuration
 * =============================================================================*/

/**
 * @brief Configure CRC context
 * @param context CRC context to configure
 * @param config Pointer to configuration structure
 * @return CRC_DRV_OK on success, error code otherwise
 */
crc_drv_status_t crc_driver_configure_context(crc_context_t context, const crc_config_t *config);

/**
 * @brief Get default configuration for specified mode
 * @param mode CRC calculation mode
 * @param config Pointer to store default configuration
 * @return CRC_DRV_OK on success, error code otherwise
 */
crc_drv_status_t crc_driver_get_default_config(crc_calc_mode_t mode, crc_config_t *config);

/**
 * @brief Reset context to default state
 * @param context CRC context to reset
 * @return CRC_DRV_OK on success, error code otherwise
 */
crc_drv_status_t crc_driver_reset_context(crc_context_t context);

/**
 * @brief Get context state information
 * @param context CRC context
 * @param state Pointer to store context state
 * @return CRC_DRV_OK on success, error code otherwise
 */
crc_drv_status_t crc_driver_get_context_state(crc_context_t context, crc_context_state_t *state);

/* =============================================================================
 * Function Prototypes - CRC Calculation
 * =============================================================================*/

/**
 * @brief Start CRC calculation for context
 * @param context CRC context
 * @param initial_value Initial CRC value (0 to use context default)
 * @return CRC_DRV_OK on success, error code otherwise
 */
crc_drv_status_t crc_driver_start_calculation(crc_context_t context, uint32_t initial_value);

/**
 * @brief Process data through CRC calculation
 * @param context CRC context
 * @param data Pointer to data buffer
 * @param length Data length in bytes
 * @return CRC_DRV_OK on success, error code otherwise
 */
crc_drv_status_t crc_driver_process_data(crc_context_t context, const uint8_t *data, uint32_t length);

/**
 * @brief Process single 32-bit word through CRC calculation
 * @param context CRC context
 * @param word 32-bit word to process
 * @return CRC_DRV_OK on success, error code otherwise
 */
crc_drv_status_t crc_driver_process_word(crc_context_t context, uint32_t word);

/**
 * @brief Finalize CRC calculation and get result
 * @param context CRC context
 * @param result Pointer to store CRC result
 * @return CRC_DRV_OK on success, error code otherwise
 */
crc_drv_status_t crc_driver_finalize_calculation(crc_context_t context, uint32_t *result);

/**
 * @brief Get current CRC value without finalizing
 * @param context CRC context
 * @param current_value Pointer to store current CRC value
 * @return CRC_DRV_OK on success, error code otherwise
 */
crc_drv_status_t crc_driver_get_current_value(crc_context_t context, uint32_t *current_value);

/* =============================================================================
 * Function Prototypes - High-level Interface
 * =============================================================================*/

/**
 * @brief Calculate CRC for buffer using specified context
 * @param context CRC context to use
 * @param data Pointer to data buffer
 * @param length Data length in bytes
 * @param result Pointer to store CRC result
 * @return CRC_DRV_OK on success, error code otherwise
 */
crc_drv_status_t crc_driver_calculate_buffer(crc_context_t context, const uint8_t *data,
                                             uint32_t length, uint32_t *result);

/**
 * @brief Calculate CRC16-CCITT for buffer (direct calculation)
 * @param data Pointer to data buffer
 * @param length Data length in bytes
 * @param initial_value Initial CRC value
 * @param result Pointer to store CRC result
 * @return CRC_DRV_OK on success, error code otherwise
 */
crc_drv_status_t crc_driver_calculate_crc16_direct(const uint8_t *data, uint32_t length,
                                                   uint16_t initial_value, uint16_t *result);

/**
 * @brief Calculate CRC32-IEEE for buffer (direct calculation)
 * @param data Pointer to data buffer
 * @param length Data length in bytes
 * @param initial_value Initial CRC value
 * @param result Pointer to store CRC result
 * @return CRC_DRV_OK on success, error code otherwise
 */
crc_drv_status_t crc_driver_calculate_crc32_direct(const uint8_t *data, uint32_t length,
                                                   uint32_t initial_value, uint32_t *result);

/* =============================================================================
 * Function Prototypes - Utilities
 * =============================================================================*/

/**
 * @brief Check if context is busy
 * @param context CRC context
 * @param is_busy Pointer to store busy status
 * @return CRC_DRV_OK on success, error code otherwise
 */
crc_drv_status_t crc_driver_is_context_busy(crc_context_t context, bool *is_busy);

/**
 * @brief Get driver version information
 * @param major Pointer to store major version
 * @param minor Pointer to store minor version
 * @param patch Pointer to store patch version
 * @return CRC_DRV_OK on success, error code otherwise
 */
crc_drv_status_t crc_driver_get_version(uint8_t *major, uint8_t *minor, uint8_t *patch);

/**
 * @brief Convert driver status to string
 * @param status Driver status code
 * @return Pointer to status string
 */
const char* crc_driver_status_to_string(crc_drv_status_t status);

/* =============================================================================
 * Utility Macros
 * =============================================================================*/

/**
 * @brief Check if CRC mode is valid
 */
#define CRC_DRV_IS_VALID_MODE(mode)     ((mode) < CRC_MODE_MAX)

/**
 * @brief Get CRC result mask based on mode
 */
#define CRC_DRV_GET_RESULT_MASK(mode)   \
    ((mode) == CRC_MODE_CRC16_CCITT ? 0xFFFFU : 0xFFFFFFFFU)

#ifdef __cplusplus
}
#endif

#endif /* CRC_DRIVER_H */