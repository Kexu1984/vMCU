/**
 * @file ac780x_hsm_hal.c
 * @brief Hardware Abstraction Layer implementation for AC780x HSM module
 */

#include "ac780x_hsm_hal.h"
#include <stddef.h>

/* Global base address storage */
static uint32_t g_ac780x_hsm_base_address = 0;
static bool g_ac780x_hsm_base_address_initialized = false;

ac780x_hsm_hal_status_t ac780x_hsm_hal_base_address_init(uint32_t base_address)
{
    g_ac780x_hsm_base_address = base_address;
    g_ac780x_hsm_base_address_initialized = true;
    return AC780X_HSM_HAL_OK;
}

ac780x_hsm_hal_status_t ac780x_hsm_hal_init(void)
{
    if (!g_ac780x_hsm_base_address_initialized) {
        return AC780X_HSM_HAL_NOT_INITIALIZED;
    }

    /* Additional HAL initialization if needed */

    return AC780X_HSM_HAL_OK;
}

ac780x_hsm_hal_status_t ac780x_hsm_hal_deinit(void)
{
    /* HAL deinitialization */
    g_ac780x_hsm_base_address_initialized = false;
    g_ac780x_hsm_base_address = 0;

    return AC780X_HSM_HAL_OK;
}

ac780x_hsm_hal_status_t ac780x_hsm_hal_read_raw(uint32_t offset, uint32_t *value)
{
    if (!g_ac780x_hsm_base_address_initialized) {
        return AC780X_HSM_HAL_NOT_INITIALIZED;
    }

    if (value == NULL) {
        return AC780X_HSM_HAL_INVALID_PARAM;
    }

    uint32_t absolute_address = g_ac780x_hsm_base_address + offset;
    uint32_t read_value = READ_REGISTER(absolute_address);

    *value = read_value;

    return AC780X_HSM_HAL_OK;
}

ac780x_hsm_hal_status_t ac780x_hsm_hal_write_raw(uint32_t offset, uint32_t value)
{
    if (!g_ac780x_hsm_base_address_initialized) {
        return AC780X_HSM_HAL_NOT_INITIALIZED;
    }

    uint32_t absolute_address = g_ac780x_hsm_base_address + offset;
    WRITE_REGISTER(absolute_address, value);

    return AC780X_HSM_HAL_OK;
}
