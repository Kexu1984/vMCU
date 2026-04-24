/**
 * @file crc_hal.h
 * @brief CRC Hardware Abstraction Layer (HAL) Header
 *
 * This file provides the hardware abstraction layer for CRC device register access.
 * It defines register addresses, bit field definitions, and basic read/write operations.
 *
 * Based on register.yaml specification with 3 CRC contexts.
 */

#ifndef CRC_HAL_H
#define CRC_HAL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =============================================================================
 * Register Address Definitions
 * =============================================================================*/

/* CRC Context 0 Registers */
#define CRC_CTX0_MODE_REG       0x000U  /**< Context 0 Mode register */
#define CRC_CTX0_IVAL_REG       0x004U  /**< Context 0 Initial Value register */
#define CRC_CTX0_DATA_REG       0x008U  /**< Context 0 Data register */

/* CRC Context 1 Registers */
#define CRC_CTX1_MODE_REG       0x020U  /**< Context 1 Mode register */
#define CRC_CTX1_IVAL_REG       0x024U  /**< Context 1 Initial Value register */
#define CRC_CTX1_DATA_REG       0x028U  /**< Context 1 Data register */

/* CRC Context 2 Registers */
#define CRC_CTX2_MODE_REG       0x040U  /**< Context 2 Mode register */
#define CRC_CTX2_IVAL_REG       0x044U  /**< Context 2 Initial Value register */
#define CRC_CTX2_DATA_REG       0x048U  /**< Context 2 Data register */

/* Context Size and Offsets */
#define CRC_CONTEXT_SIZE        0x020U  /**< Size between contexts */
#define CRC_NUM_CONTEXTS        3U      /**< Number of CRC contexts */

/* Register offsets within context */
#define CRC_CTX_MODE_OFFSET     0x00U   /**< Mode register offset */
#define CRC_CTX_IVAL_OFFSET     0x04U   /**< Initial value offset */
#define CRC_CTX_DATA_OFFSET     0x08U   /**< Data register offset */

/* =============================================================================
 * Bit Field Definitions for CtxMode Register
 * =============================================================================*/

/* CRC_MODE field (bit 0) */
#define CRC_MODE_BIT            0U
#define CRC_MODE_MASK           (1U << CRC_MODE_BIT)
#define CRC_MODE_CRC16          0U      /**< CRC16-CCITT mode */
#define CRC_MODE_CRC32          1U      /**< CRC32 mode */

/* ACC_MS_BIT field (bit 1) */
#define CRC_ACC_MS_BIT_BIT      1U
#define CRC_ACC_MS_BIT_MASK     (1U << CRC_ACC_MS_BIT_BIT)
#define CRC_ACC_MS_BIT_LAST     0U      /**< MS bit last */
#define CRC_ACC_MS_BIT_FIRST    1U      /**< MS bit first */

/* ACC_MS_BYTE field (bit 2) */
#define CRC_ACC_MS_BYTE_BIT     2U
#define CRC_ACC_MS_BYTE_MASK    (1U << CRC_ACC_MS_BYTE_BIT)
#define CRC_ACC_MS_BYTE_LAST    0U      /**< MS byte last */
#define CRC_ACC_MS_BYTE_FIRST   1U      /**< MS byte first */

/* OUT_MS_BIT field (bit 3) */
#define CRC_OUT_MS_BIT_BIT      3U
#define CRC_OUT_MS_BIT_MASK     (1U << CRC_OUT_MS_BIT_BIT)
#define CRC_OUT_MS_BIT_LAST     0U      /**< Output MS bit last */
#define CRC_OUT_MS_BIT_FIRST    1U      /**< Output MS bit first */

/* OUT_MS_BYTE field (bit 4) */
#define CRC_OUT_MS_BYTE_BIT     4U
#define CRC_OUT_MS_BYTE_MASK    (1U << CRC_OUT_MS_BYTE_BIT)
#define CRC_OUT_MS_BYTE_LAST    0U      /**< Output MS byte last */
#define CRC_OUT_MS_BYTE_FIRST   1U      /**< Output MS byte first */

/* OUT_INV field (bit 5) */
#define CRC_OUT_INV_BIT         5U
#define CRC_OUT_INV_MASK        (1U << CRC_OUT_INV_BIT)
#define CRC_OUT_INV_NORMAL      0U      /**< Output not inverted */
#define CRC_OUT_INV_INVERTED    1U      /**< Output inverted */

/* =============================================================================
 * Data Types
 * =============================================================================*/

/**
 * @brief CRC context enumeration
 */
typedef enum {
    CRC_CONTEXT_0 = 0U,     /**< CRC Context 0 */
    CRC_CONTEXT_1 = 1U,     /**< CRC Context 1 */
    CRC_CONTEXT_2 = 2U,     /**< CRC Context 2 */
    CRC_CONTEXT_MAX         /**< Maximum context value (for validation) */
} crc_context_t;

/**
 * @brief CRC register enumeration within a context
 */
typedef enum {
    CRC_REG_MODE = 0U,      /**< Mode register */
    CRC_REG_IVAL = 1U,      /**< Initial value register */
    CRC_REG_DATA = 2U,      /**< Data register */
    CRC_REG_MAX             /**< Maximum register value (for validation) */
} crc_register_t;

/**
 * @brief CRC HAL status enumeration
 */
typedef enum {
    CRC_HAL_OK = 0U,        /**< Operation successful */
    CRC_HAL_ERROR,          /**< General error */
    CRC_HAL_INVALID_PARAM,  /**< Invalid parameter */
    CRC_HAL_TIMEOUT,        /**< Operation timeout */
    CRC_HAL_BUSY            /**< Device busy */
} crc_hal_status_t;

/* =============================================================================
 * Function Prototypes
 * =============================================================================*/

/**
 * @brief Initialize CRC HAL base address
 * @param base_address Base address of CRC device
 * @return CRC_HAL_OK on success, error code otherwise
 * @note This function must be called before any other HAL operations
 */
crc_hal_status_t crc_hal_base_address_init(uint32_t base_address);

/**
 * @brief Initialize CRC HAL
 * @return CRC_HAL_OK on success, error code otherwise
 * @note crc_hal_base_address_init() must be called first
 */
crc_hal_status_t crc_hal_init(void);

/**
 * @brief Deinitialize CRC HAL
 * @return CRC_HAL_OK on success, error code otherwise
 */
crc_hal_status_t crc_hal_deinit(void);

/**
 * @brief Read from CRC register
 * @param context CRC context (0-2)
 * @param reg Register type
 * @param value Pointer to store read value
 * @return CRC_HAL_OK on success, error code otherwise
 */
crc_hal_status_t crc_hal_read_register(crc_context_t context, crc_register_t reg, uint32_t *value);

/**
 * @brief Write to CRC register
 * @param context CRC context (0-2)
 * @param reg Register type
 * @param value Value to write
 * @return CRC_HAL_OK on success, error code otherwise
 */
crc_hal_status_t crc_hal_write_register(crc_context_t context, crc_register_t reg, uint32_t value);

/**
 * @brief Read from CRC register using direct address
 * @param address Register address offset
 * @param value Pointer to store read value
 * @return CRC_HAL_OK on success, error code otherwise
 */
crc_hal_status_t crc_hal_read_register_direct(uint32_t address, uint32_t *value);

/**
 * @brief Write to CRC register using direct address
 * @param address Register address offset
 * @param value Value to write
 * @return CRC_HAL_OK on success, error code otherwise
 */
crc_hal_status_t crc_hal_write_register_direct(uint32_t address, uint32_t value);

/**
 * @brief Get register address for context and register type
 * @param context CRC context (0-2)
 * @param reg Register type
 * @return Register address offset, or 0xFFFFFFFF if invalid
 */
uint32_t crc_hal_get_register_address(crc_context_t context, crc_register_t reg);

/**
 * @brief Reset all CRC contexts to default values
 * @return CRC_HAL_OK on success, error code otherwise
 */
crc_hal_status_t crc_hal_reset_all_contexts(void);

/**
 * @brief Reset specific CRC context to default values
 * @param context CRC context to reset
 * @return CRC_HAL_OK on success, error code otherwise
 */
crc_hal_status_t crc_hal_reset_context(crc_context_t context);

/* =============================================================================
 * Utility Macros
 * =============================================================================*/

/**
 * @brief Check if context is valid
 */
#define CRC_HAL_IS_VALID_CONTEXT(ctx)   ((ctx) < CRC_CONTEXT_MAX)

/**
 * @brief Check if register type is valid
 */
#define CRC_HAL_IS_VALID_REGISTER(reg)  ((reg) < CRC_REG_MAX)

/**
 * @brief Extract bit field from register value
 */
#define CRC_HAL_GET_FIELD(value, mask, shift)  (((value) & (mask)) >> (shift))

/**
 * @brief Set bit field in register value
 */
#define CRC_HAL_SET_FIELD(value, field_val, mask, shift) \
    (((value) & ~(mask)) | (((field_val) << (shift)) & (mask)))

#ifdef __cplusplus
}
#endif

#endif /* CRC_HAL_H */