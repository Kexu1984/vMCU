/**
 * @file ac780x_hsm_hal.h
 * @brief Hardware Abstraction Layer for AC780x HSM module
 *
 * This file provides the HAL interface for AC780x HSM register access
 * and base address management.
 */

#ifndef AC780X_HSM_HAL_H
#define AC780X_HSM_HAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* HAL Status Codes */
typedef enum {
    AC780X_HSM_HAL_OK = 0,
    AC780X_HSM_HAL_ERROR,
    AC780X_HSM_HAL_INVALID_PARAM,
    AC780X_HSM_HAL_NOT_INITIALIZED,
    AC780X_HSM_HAL_HARDWARE_ERROR
} ac780x_hsm_hal_status_t;

/* Register Access Macros */
#ifdef REAL_CHIP
/* Direct memory access for real chip */
#define WRITE_REGISTER(addr, value)  *(volatile uint32_t *)(addr) = (value)
#define READ_REGISTER(addr)          *(volatile uint32_t *)(addr)
#else
/* Interface layer access for simulation */
#include "interface_layer.h"
int write_register(uint32_t address, uint32_t data, uint32_t size);
uint32_t read_register(uint32_t address, uint32_t size);
#define WRITE_REGISTER(addr, value)  (void)write_register(addr, value, 4)
#define READ_REGISTER(addr)          read_register(addr, 4)
#endif

/**
 * @brief Initialize HAL base address
 * @param base_address Register base address for HSM device
 * @return HAL status code
 */
ac780x_hsm_hal_status_t ac780x_hsm_hal_base_address_init(uint32_t base_address);

/**
 * @brief Initialize HAL (requires base address already set)
 * @return HAL status code
 */
ac780x_hsm_hal_status_t ac780x_hsm_hal_init(void);

/**
 * @brief Deinitialize HAL
 * @return HAL status code
 */
ac780x_hsm_hal_status_t ac780x_hsm_hal_deinit(void);

/**
 * @brief Raw register read using base + offset
 * @param offset Register offset from base address
 * @param value Pointer to store read value
 * @return HAL status code
 */
ac780x_hsm_hal_status_t ac780x_hsm_hal_read_raw(uint32_t offset, uint32_t *value);

/**
 * @brief Raw register write using base + offset
 * @param offset Register offset from base address
 * @param value Value to write
 * @return HAL status code
 */
ac780x_hsm_hal_status_t ac780x_hsm_hal_write_raw(uint32_t offset, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif /* AC780X_HSM_HAL_H */
