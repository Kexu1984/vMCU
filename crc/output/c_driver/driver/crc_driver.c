/**
 * @file crc_driver.c
 * @brief CRC Driver Layer Implementation
 *
 * This file implements the business logic layer for CRC calculations.
 * It provides high-level CRC calculation functions using the HAL layer.
 */

#include "crc_driver.h"
#include <string.h>

/* =============================================================================
 * Constants
 * =============================================================================*/

/** Driver version */
#define CRC_DRIVER_VERSION_MAJOR    1U
#define CRC_DRIVER_VERSION_MINOR    0U
#define CRC_DRIVER_VERSION_PATCH    0U

/* =============================================================================
 * Private Variables
 * =============================================================================*/

/** Driver initialization status */
static bool g_crc_driver_initialized = false;

/** Context states */
static crc_context_state_t g_context_states[CRC_NUM_CONTEXTS];

/* =============================================================================
 * Private Function Prototypes
 * =============================================================================*/

/**
 * @brief Validate context parameter
 */
static bool crc_drv_validate_context(crc_context_t context);

/**
 * @brief Update hardware registers from context configuration
 */
static crc_drv_status_t crc_drv_update_hardware_config(crc_context_t context);

/**
 * @brief Read hardware registers to context configuration
 */
static crc_drv_status_t crc_drv_read_hardware_config(crc_context_t context);

/**
 * @brief Calculate CRC16-CCITT for data
 */
static uint16_t crc_drv_calc_crc16_ccitt(const uint8_t *data, uint32_t length,
                                         uint16_t initial, bool msb_bit_first);

/**
 * @brief Calculate CRC32-IEEE for data
 */
static uint32_t crc_drv_calc_crc32_ieee(const uint8_t *data, uint32_t length,
                                        uint32_t initial, bool msb_bit_first);

/**
 * @brief Apply byte ordering to data
 */
static void crc_drv_apply_byte_ordering(uint8_t *data, uint32_t length, bool msb_first);

/**
 * @brief Apply output formatting to result
 */
static uint32_t crc_drv_apply_output_formatting(uint32_t value, const crc_config_t *config);

/* =============================================================================
 * Public Function Implementations - Initialization
 * =============================================================================*/

crc_drv_status_t crc_driver_base_address_init(uint32_t base_address)
{
    /* Initialize HAL base address */
    crc_hal_status_t hal_status = crc_hal_base_address_init(base_address);
    if (hal_status != CRC_HAL_OK) {
        return CRC_DRV_HARDWARE_ERROR;
    }

    return CRC_DRV_OK;
}

crc_drv_status_t crc_driver_init(void)
{
    if (g_crc_driver_initialized) {
        return CRC_DRV_OK; /* Already initialized */
    }

    /* Initialize HAL layer */
    crc_hal_status_t hal_status = crc_hal_init();
    if (hal_status != CRC_HAL_OK) {
        return CRC_DRV_HARDWARE_ERROR;
    }

    /* Initialize context states */
    memset(g_context_states, 0, sizeof(g_context_states));

    /* Set default configurations for all contexts */
    for (crc_context_t ctx = CRC_CONTEXT_0; ctx < CRC_CONTEXT_MAX; ctx++) {
        crc_config_t default_config;
        crc_driver_get_default_config(CRC_MODE_CRC16_CCITT, &default_config);
        g_context_states[ctx].config = default_config;
        g_context_states[ctx].current_value = default_config.initial_value;
        g_context_states[ctx].calculation_active = false;
        g_context_states[ctx].calculation_complete = false;
        g_context_states[ctx].data_count = 0U;
    }

    /* Reset hardware to known state */
    hal_status = crc_hal_reset_all_contexts();
    if (hal_status != CRC_HAL_OK) {
        return CRC_DRV_HARDWARE_ERROR;
    }

    g_crc_driver_initialized = true;
    return CRC_DRV_OK;
}

crc_drv_status_t crc_driver_deinit(void)
{
    if (!g_crc_driver_initialized) {
        return CRC_DRV_NOT_INITIALIZED;
    }

    /* Deinitialize HAL layer */
    crc_hal_status_t hal_status = crc_hal_deinit();
    if (hal_status != CRC_HAL_OK) {
        return CRC_DRV_HARDWARE_ERROR;
    }

    /* Clear context states */
    memset(g_context_states, 0, sizeof(g_context_states));

    g_crc_driver_initialized = false;
    return CRC_DRV_OK;
}

/* =============================================================================
 * Public Function Implementations - Context Configuration
 * =============================================================================*/

crc_drv_status_t crc_driver_configure_context(crc_context_t context, const crc_config_t *config)
{
    if (!g_crc_driver_initialized) {
        return CRC_DRV_NOT_INITIALIZED;
    }

    if (!crc_drv_validate_context(context) || (config == NULL)) {
        return CRC_DRV_INVALID_PARAM;
    }

    if (!CRC_DRV_IS_VALID_MODE(config->mode)) {
        return CRC_DRV_INVALID_PARAM;
    }

    /* Check if context is busy */
    if (g_context_states[context].calculation_active) {
        return CRC_DRV_CONTEXT_BUSY;
    }

    /* Update context configuration */
    g_context_states[context].config = *config;
    g_context_states[context].current_value = config->initial_value;
    g_context_states[context].calculation_complete = false;
    g_context_states[context].data_count = 0U;

    /* Update hardware registers */
    return crc_drv_update_hardware_config(context);
}

crc_drv_status_t crc_driver_get_default_config(crc_calc_mode_t mode, crc_config_t *config)
{
    if (config == NULL) {
        return CRC_DRV_INVALID_PARAM;
    }

    if (!CRC_DRV_IS_VALID_MODE(mode)) {
        return CRC_DRV_INVALID_PARAM;
    }

    memset(config, 0, sizeof(crc_config_t));

    config->mode = mode;
    config->acc_ms_bit_first = false;
    config->acc_ms_byte_first = false;
    config->out_ms_bit_first = false;
    config->out_ms_byte_first = false;
    config->output_inverted = false;

    if (mode == CRC_MODE_CRC16_CCITT) {
        config->initial_value = CRC16_INITIAL_VALUE;
    } else {
        config->initial_value = CRC32_INITIAL_VALUE;
    }

    return CRC_DRV_OK;
}

crc_drv_status_t crc_driver_reset_context(crc_context_t context)
{
    if (!g_crc_driver_initialized) {
        return CRC_DRV_NOT_INITIALIZED;
    }

    if (!crc_drv_validate_context(context)) {
        return CRC_DRV_INVALID_PARAM;
    }

    /* Reset context state */
    crc_config_t *config = &g_context_states[context].config;
    g_context_states[context].current_value = config->initial_value;
    g_context_states[context].calculation_active = false;
    g_context_states[context].calculation_complete = false;
    g_context_states[context].data_count = 0U;

    /* Reset hardware registers */
    crc_hal_status_t hal_status = crc_hal_reset_context(context);
    if (hal_status != CRC_HAL_OK) {
        return CRC_DRV_HARDWARE_ERROR;
    }

    return crc_drv_update_hardware_config(context);
}

crc_drv_status_t crc_driver_get_context_state(crc_context_t context, crc_context_state_t *state)
{
    if (!g_crc_driver_initialized) {
        return CRC_DRV_NOT_INITIALIZED;
    }

    if (!crc_drv_validate_context(context) || (state == NULL)) {
        return CRC_DRV_INVALID_PARAM;
    }

    *state = g_context_states[context];
    return CRC_DRV_OK;
}

/* =============================================================================
 * Public Function Implementations - CRC Calculation
 * =============================================================================*/

crc_drv_status_t crc_driver_start_calculation(crc_context_t context, uint32_t initial_value)
{
    if (!g_crc_driver_initialized) {
        return CRC_DRV_NOT_INITIALIZED;
    }

    if (!crc_drv_validate_context(context)) {
        return CRC_DRV_INVALID_PARAM;
    }

    crc_context_state_t *state = &g_context_states[context];

    /* Set initial value */
    if (initial_value != 0U) {
        state->current_value = initial_value;
    } else {
        state->current_value = state->config.initial_value;
    }

    /* Update hardware initial value register */
    crc_hal_status_t hal_status = crc_hal_write_register(context, CRC_REG_IVAL, state->current_value);
    if (hal_status != CRC_HAL_OK) {
        return CRC_DRV_HARDWARE_ERROR;
    }

    /* Mark calculation as active */
    state->calculation_active = true;
    state->calculation_complete = false;
    state->data_count = 0U;

    return CRC_DRV_OK;
}

crc_drv_status_t crc_driver_process_data(crc_context_t context, const uint8_t *data, uint32_t length)
{
    if (!g_crc_driver_initialized) {
        return CRC_DRV_NOT_INITIALIZED;
    }

    if (!crc_drv_validate_context(context) || (data == NULL) || (length == 0U)) {
        return CRC_DRV_INVALID_PARAM;
    }

    if (length > CRC_MAX_DATA_SIZE) {
        return CRC_DRV_INVALID_PARAM;
    }

    crc_context_state_t *state = &g_context_states[context];

    if (!state->calculation_active) {
        return CRC_DRV_ERROR;
    }

    /* Create a copy of data for processing */
    uint8_t data_copy[CRC_MAX_DATA_SIZE];
    memcpy(data_copy, data, length);

    /* Apply byte ordering if configured */
    if (state->config.acc_ms_byte_first) {
        crc_drv_apply_byte_ordering(data_copy, length, true);
    }

    /* Calculate CRC incrementally */
    if (state->config.mode == CRC_MODE_CRC16_CCITT) {
        state->current_value = crc_drv_calc_crc16_ccitt(data_copy, length,
                                                       (uint16_t)state->current_value,
                                                       state->config.acc_ms_bit_first);
    } else {
        state->current_value = crc_drv_calc_crc32_ieee(data_copy, length,
                                                      state->current_value,
                                                      state->config.acc_ms_bit_first);
    }

    state->data_count += length;

    return CRC_DRV_OK;
}

crc_drv_status_t crc_driver_process_word(crc_context_t context, uint32_t word)
{
    uint8_t word_bytes[4];

    /* Convert word to bytes */
    word_bytes[0] = (uint8_t)(word & 0xFFU);
    word_bytes[1] = (uint8_t)((word >> 8) & 0xFFU);
    word_bytes[2] = (uint8_t)((word >> 16) & 0xFFU);
    word_bytes[3] = (uint8_t)((word >> 24) & 0xFFU);

    /* Write to hardware data register (this triggers CRC calculation) */
    crc_hal_status_t hal_status = crc_hal_write_register(context, CRC_REG_DATA, word);
    if (hal_status != CRC_HAL_OK) {
        return CRC_DRV_HARDWARE_ERROR;
    }

    /* Also process through software calculation */
    return crc_driver_process_data(context, word_bytes, 4U);
}

crc_drv_status_t crc_driver_finalize_calculation(crc_context_t context, uint32_t *result)
{
    if (!g_crc_driver_initialized) {
        return CRC_DRV_NOT_INITIALIZED;
    }

    if (!crc_drv_validate_context(context) || (result == NULL)) {
        return CRC_DRV_INVALID_PARAM;
    }

    crc_context_state_t *state = &g_context_states[context];

    if (!state->calculation_active) {
        return CRC_DRV_ERROR;
    }

    /* Apply output formatting */
    uint32_t final_result = crc_drv_apply_output_formatting(state->current_value, &state->config);

    /* Apply result mask based on CRC type */
    if (state->config.mode == CRC_MODE_CRC16_CCITT) {
        final_result &= 0xFFFFU;
    }

    *result = final_result;

    /* Mark calculation as complete */
    state->calculation_active = false;
    state->calculation_complete = true;

    return CRC_DRV_OK;
}

crc_drv_status_t crc_driver_get_current_value(crc_context_t context, uint32_t *current_value)
{
    if (!g_crc_driver_initialized) {
        return CRC_DRV_NOT_INITIALIZED;
    }

    if (!crc_drv_validate_context(context) || (current_value == NULL)) {
        return CRC_DRV_INVALID_PARAM;
    }

    /* Read current value from hardware register */
    crc_hal_status_t hal_status = crc_hal_read_register(context, CRC_REG_IVAL, current_value);
    if (hal_status != CRC_HAL_OK) {
        /* Fallback to software value */
        *current_value = g_context_states[context].current_value;
    }

    return CRC_DRV_OK;
}

/* =============================================================================
 * Public Function Implementations - High-level Interface
 * =============================================================================*/

crc_drv_status_t crc_driver_calculate_buffer(crc_context_t context, const uint8_t *data,
                                             uint32_t length, uint32_t *result)
{
    if (!g_crc_driver_initialized) {
        return CRC_DRV_NOT_INITIALIZED;
    }

    if (!crc_drv_validate_context(context) || (data == NULL) || (length == 0U) || (result == NULL)) {
        return CRC_DRV_INVALID_PARAM;
    }

    crc_drv_status_t status;

    /* Start calculation */
    status = crc_driver_start_calculation(context, 0U);
    if (status != CRC_DRV_OK) {
        return status;
    }

    /* Process data */
    status = crc_driver_process_data(context, data, length);
    if (status != CRC_DRV_OK) {
        return status;
    }

    /* Finalize and get result */
    return crc_driver_finalize_calculation(context, result);
}

crc_drv_status_t crc_driver_calculate_crc16_direct(const uint8_t *data, uint32_t length,
                                                   uint16_t initial_value, uint16_t *result)
{
    if ((data == NULL) || (length == 0U) || (result == NULL)) {
        return CRC_DRV_INVALID_PARAM;
    }

    if (length > CRC_MAX_DATA_SIZE) {
        return CRC_DRV_INVALID_PARAM;
    }

    *result = crc_drv_calc_crc16_ccitt(data, length, initial_value, false);
    return CRC_DRV_OK;
}

crc_drv_status_t crc_driver_calculate_crc32_direct(const uint8_t *data, uint32_t length,
                                                   uint32_t initial_value, uint32_t *result)
{
    if ((data == NULL) || (length == 0U) || (result == NULL)) {
        return CRC_DRV_INVALID_PARAM;
    }

    if (length > CRC_MAX_DATA_SIZE) {
        return CRC_DRV_INVALID_PARAM;
    }

    uint32_t crc_result = crc_drv_calc_crc32_ieee(data, length, initial_value, false);
    *result = crc_result ^ 0xFFFFFFFFU; /* Apply final XOR for IEEE CRC32 */
    return CRC_DRV_OK;
}

/* =============================================================================
 * Public Function Implementations - Utilities
 * =============================================================================*/

crc_drv_status_t crc_driver_is_context_busy(crc_context_t context, bool *is_busy)
{
    if (!g_crc_driver_initialized) {
        return CRC_DRV_NOT_INITIALIZED;
    }

    if (!crc_drv_validate_context(context) || (is_busy == NULL)) {
        return CRC_DRV_INVALID_PARAM;
    }

    *is_busy = g_context_states[context].calculation_active;
    return CRC_DRV_OK;
}

crc_drv_status_t crc_driver_get_version(uint8_t *major, uint8_t *minor, uint8_t *patch)
{
    if ((major == NULL) || (minor == NULL) || (patch == NULL)) {
        return CRC_DRV_INVALID_PARAM;
    }

    *major = CRC_DRIVER_VERSION_MAJOR;
    *minor = CRC_DRIVER_VERSION_MINOR;
    *patch = CRC_DRIVER_VERSION_PATCH;

    return CRC_DRV_OK;
}

const char* crc_driver_status_to_string(crc_drv_status_t status)
{
    switch (status) {
        case CRC_DRV_OK:                return "CRC_DRV_OK";
        case CRC_DRV_ERROR:             return "CRC_DRV_ERROR";
        case CRC_DRV_INVALID_PARAM:     return "CRC_DRV_INVALID_PARAM";
        case CRC_DRV_CONTEXT_BUSY:      return "CRC_DRV_CONTEXT_BUSY";
        case CRC_DRV_NOT_INITIALIZED:   return "CRC_DRV_NOT_INITIALIZED";
        case CRC_DRV_HARDWARE_ERROR:    return "CRC_DRV_HARDWARE_ERROR";
        default:                        return "UNKNOWN_STATUS";
    }
}

/* =============================================================================
 * Private Function Implementations
 * =============================================================================*/

static bool crc_drv_validate_context(crc_context_t context)
{
    return (context < CRC_CONTEXT_MAX);
}

static crc_drv_status_t crc_drv_update_hardware_config(crc_context_t context)
{
    const crc_config_t *config = &g_context_states[context].config;

    /* Build mode register value */
    uint32_t mode_value = 0U;

    if (config->mode == CRC_MODE_CRC32_IEEE) {
        mode_value |= CRC_MODE_MASK;
    }

    if (config->acc_ms_bit_first) {
        mode_value |= CRC_ACC_MS_BIT_MASK;
    }

    if (config->acc_ms_byte_first) {
        mode_value |= CRC_ACC_MS_BYTE_MASK;
    }

    if (config->out_ms_bit_first) {
        mode_value |= CRC_OUT_MS_BIT_MASK;
    }

    if (config->out_ms_byte_first) {
        mode_value |= CRC_OUT_MS_BYTE_MASK;
    }

    if (config->output_inverted) {
        mode_value |= CRC_OUT_INV_MASK;
    }

    /* Write mode register */
    crc_hal_status_t hal_status = crc_hal_write_register(context, CRC_REG_MODE, mode_value);
    if (hal_status != CRC_HAL_OK) {
        return CRC_DRV_HARDWARE_ERROR;
    }

    /* Write initial value register */
    hal_status = crc_hal_write_register(context, CRC_REG_IVAL, config->initial_value);
    if (hal_status != CRC_HAL_OK) {
        return CRC_DRV_HARDWARE_ERROR;
    }

    return CRC_DRV_OK;
}

static crc_drv_status_t crc_drv_read_hardware_config(crc_context_t context)
{
    crc_config_t *config = &g_context_states[context].config;
    uint32_t mode_value, ival_value;

    /* Read mode register */
    crc_hal_status_t hal_status = crc_hal_read_register(context, CRC_REG_MODE, &mode_value);
    if (hal_status != CRC_HAL_OK) {
        return CRC_DRV_HARDWARE_ERROR;
    }

    /* Read initial value register */
    hal_status = crc_hal_read_register(context, CRC_REG_IVAL, &ival_value);
    if (hal_status != CRC_HAL_OK) {
        return CRC_DRV_HARDWARE_ERROR;
    }

    /* Parse mode register */
    config->mode = (mode_value & CRC_MODE_MASK) ? CRC_MODE_CRC32_IEEE : CRC_MODE_CRC16_CCITT;
    config->acc_ms_bit_first = (mode_value & CRC_ACC_MS_BIT_MASK) != 0U;
    config->acc_ms_byte_first = (mode_value & CRC_ACC_MS_BYTE_MASK) != 0U;
    config->out_ms_bit_first = (mode_value & CRC_OUT_MS_BIT_MASK) != 0U;
    config->out_ms_byte_first = (mode_value & CRC_OUT_MS_BYTE_MASK) != 0U;
    config->output_inverted = (mode_value & CRC_OUT_INV_MASK) != 0U;
    config->initial_value = ival_value;

    return CRC_DRV_OK;
}

static uint16_t crc_drv_calc_crc16_ccitt(const uint8_t *data, uint32_t length,
                                         uint16_t initial, bool msb_bit_first)
{
    uint16_t crc = initial;

    for (uint32_t i = 0U; i < length; i++) {
        uint8_t byte = data[i];

        if (msb_bit_first) {
            /* Process bits from MSB to LSB */
            for (int32_t bit = 7; bit >= 0; bit--) {
                bool bit_val = (byte >> bit) & 1U;
                bool msb = (crc >> 15) & 1U;
                crc = (crc << 1) | (bit_val ? 1U : 0U);
                if (msb) {
                    crc ^= CRC16_CCITT_POLYNOMIAL;
                }
            }
        } else {
            /* Process bits from LSB to MSB */
            for (uint32_t bit = 0U; bit < 8U; bit++) {
                bool bit_val = (byte >> bit) & 1U;
                bool msb = (crc >> 15) & 1U;
                crc = (crc << 1) | (bit_val ? 1U : 0U);
                if (msb) {
                    crc ^= CRC16_CCITT_POLYNOMIAL;
                }
            }
        }
    }

    return crc;
}

static uint32_t crc_drv_calc_crc32_ieee(const uint8_t *data, uint32_t length,
                                        uint32_t initial, bool msb_bit_first)
{
    uint32_t crc = initial;

    for (uint32_t i = 0U; i < length; i++) {
        uint8_t byte = data[i];

        if (msb_bit_first) {
            /* Process bits from MSB to LSB */
            for (int32_t bit = 7; bit >= 0; bit--) {
                bool bit_val = (byte >> bit) & 1U;
                bool msb = (crc >> 31) & 1U;
                crc = (crc << 1) | (bit_val ? 1U : 0U);
                if (msb) {
                    crc ^= CRC32_IEEE_POLYNOMIAL;
                }
            }
        } else {
            /* Process bits from LSB to MSB */
            for (uint32_t bit = 0U; bit < 8U; bit++) {
                bool bit_val = (byte >> bit) & 1U;
                bool msb = (crc >> 31) & 1U;
                crc = (crc << 1) | (bit_val ? 1U : 0U);
                if (msb) {
                    crc ^= CRC32_IEEE_POLYNOMIAL;
                }
            }
        }
    }

    return crc;
}

static void crc_drv_apply_byte_ordering(uint8_t *data, uint32_t length, bool msb_first)
{
    if (msb_first && (length > 1U)) {
        /* Reverse byte order */
        for (uint32_t i = 0U; i < length / 2U; i++) {
            uint8_t temp = data[i];
            data[i] = data[length - 1U - i];
            data[length - 1U - i] = temp;
        }
    }
}

static uint32_t crc_drv_apply_output_formatting(uint32_t value, const crc_config_t *config)
{
    uint32_t result = value;

    /* Apply final XOR for CRC32-IEEE */
    if (config->mode == CRC_MODE_CRC32_IEEE) {
        result ^= 0xFFFFFFFFU;
    }

    /* Apply output inversion if configured */
    if (config->output_inverted) {
        if (config->mode == CRC_MODE_CRC16_CCITT) {
            result ^= 0xFFFFU;
        } else {
            result ^= 0xFFFFFFFFU;
        }
    }

    /* Apply output byte ordering if configured */
    if (config->out_ms_byte_first) {
        if (config->mode == CRC_MODE_CRC32_IEEE) {
            /* Reverse 4 bytes */
            result = ((result & 0xFF000000U) >> 24) |
                    ((result & 0x00FF0000U) >> 8) |
                    ((result & 0x0000FF00U) << 8) |
                    ((result & 0x000000FFU) << 24);
        } else {
            /* Reverse 2 bytes */
            result = ((result & 0xFF00U) >> 8) | ((result & 0x00FFU) << 8);
        }
    }

    /* Note: Bit order reversal is complex and not fully implemented here */
    /* In a real implementation, this would require bit-by-bit reversal */

    return result;
}