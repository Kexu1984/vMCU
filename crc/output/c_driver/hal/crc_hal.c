/**
 * @file crc_hal.c
 * @brief CRC Hardware Abstraction Layer (HAL) Implementation
 *
 * This file implements the hardware abstraction layer for CRC device register access.
 * It provides low-level register read/write functions and basic hardware operations.
 */

#include "crc_hal.h"
#include <string.h>
#include <stdio.h>

#ifndef CRC_HAL_DEBUG
#define CRC_HAL_DEBUG 1
#endif
#if CRC_HAL_DEBUG
#define HAL_LOG(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define HAL_LOG(fmt, ...) (void)0
#endif

int write_register(uint32_t address, uint32_t data, uint32_t size);
int read_register(uint32_t address, uint32_t size);
/* =============================================================================
 * Private Variables
 * =============================================================================*/

/** Base address of CRC device */
static uint32_t g_crc_base_address = 0U;

/** HAL base address initialization status */
static bool g_crc_base_address_initialized = false;

/** HAL initialization status */
static bool g_crc_hal_initialized = false;

/* =============================================================================
 * Private Function Prototypes
 * =============================================================================*/

/**
 * @brief Simulate register read operation
 * @param address Register address
 * @return Register value (simulated)
 */
static uint32_t crc_hal_read_raw(uint32_t address);

/**
 * @brief Simulate register write operation
 * @param address Register address
 * @param value Value to write
 */
static void crc_hal_write_raw(uint32_t address, uint32_t value);

/**
 * @brief Validate context parameter
 * @param context Context to validate
 * @return true if valid, false otherwise
 */
static bool crc_hal_validate_context(crc_context_t context);

/**
 * @brief Validate register parameter
 * @param reg Register to validate
 * @return true if valid, false otherwise
 */
static bool crc_hal_validate_register(crc_register_t reg);

/* =============================================================================
 * Private Variables for Register Simulation
 * =============================================================================*/

/** Simulated register storage */
static uint32_t g_register_storage[CRC_NUM_CONTEXTS][CRC_REG_MAX];

/* =============================================================================
 * Public Function Implementations
 * =============================================================================*/

crc_hal_status_t crc_hal_base_address_init(uint32_t base_address)
{
    g_crc_base_address = base_address;
    g_crc_base_address_initialized = true;

    HAL_LOG("CRC HAL base address initialized: 0x%08X\n", base_address);
    return CRC_HAL_OK;
}

crc_hal_status_t crc_hal_init(void)
{
    if (!g_crc_base_address_initialized) {
        printf("ERROR: CRC HAL base address not initialized. Call crc_hal_base_address_init() first.\n");
        return CRC_HAL_ERROR;
    }

    if (g_crc_hal_initialized) {
        return CRC_HAL_OK; /* Already initialized */
    }

    g_crc_hal_initialized = true;

    /* Initialize simulated register storage to default values */
    memset(g_register_storage, 0, sizeof(g_register_storage));

    /* Set default values according to register.yaml specification */
    for (uint32_t ctx = 0U; ctx < CRC_NUM_CONTEXTS; ctx++) {
        g_register_storage[ctx][CRC_REG_MODE] = 0x00000000U; /* Default CtxMode */
        g_register_storage[ctx][CRC_REG_IVAL] = 0x00000000U; /* Default CRC_IVAL */
        g_register_storage[ctx][CRC_REG_DATA] = 0x00000000U; /* Default CRC_DATA */
    }

    return CRC_HAL_OK;
}

crc_hal_status_t crc_hal_deinit(void)
{
    if (!g_crc_hal_initialized) {
        return CRC_HAL_ERROR;
    }

    g_crc_hal_initialized = false;

    /* Clear register storage */
    memset(g_register_storage, 0, sizeof(g_register_storage));

    /* Note: We don't reset base address initialization status */
    /* This allows re-initialization without re-setting the base address */

    return CRC_HAL_OK;
}

crc_hal_status_t crc_hal_read_register(crc_context_t context, crc_register_t reg, uint32_t *value)
{
    if (!g_crc_hal_initialized) {
        return CRC_HAL_ERROR;
    }

    if (value == NULL) {
        return CRC_HAL_INVALID_PARAM;
    }

    if (!crc_hal_validate_context(context) || !crc_hal_validate_register(reg)) {
        return CRC_HAL_INVALID_PARAM;
    }

    uint32_t address = crc_hal_get_register_address(context, reg);
    if (address == 0xFFFFFFFFU) {
        return CRC_HAL_INVALID_PARAM;
    }

    *value = crc_hal_read_raw(address);

    return CRC_HAL_OK;
}

crc_hal_status_t crc_hal_write_register(crc_context_t context, crc_register_t reg, uint32_t value)
{
    if (!g_crc_hal_initialized) {
        return CRC_HAL_ERROR;
    }

    if (!crc_hal_validate_context(context) || !crc_hal_validate_register(reg)) {
        return CRC_HAL_INVALID_PARAM;
    }

    /* Data register is write-only, others are read-write */
    if (reg == CRC_REG_DATA) {
        /* For data register, we don't store the value but trigger CRC calculation */
        /* This will be handled by the driver layer */
    }

    uint32_t address = crc_hal_get_register_address(context, reg);
    if (address == 0xFFFFFFFFU) {
        return CRC_HAL_INVALID_PARAM;
    }

    crc_hal_write_raw(address, value);

    return CRC_HAL_OK;
}

crc_hal_status_t crc_hal_read_register_direct(uint32_t address, uint32_t *value)
{
    if (!g_crc_hal_initialized) {
        return CRC_HAL_ERROR;
    }

    if (value == NULL) {
        return CRC_HAL_INVALID_PARAM;
    }

    /* Direct access expects absolute address */
    HAL_LOG("HAL READ DIRECT: abs=0x%08X\n", address);
    *value = read_register(address, 4);

    return CRC_HAL_OK;
}

crc_hal_status_t crc_hal_write_register_direct(uint32_t address, uint32_t value)
{
    if (!g_crc_hal_initialized) {
        return CRC_HAL_ERROR;
    }

    /* Direct access expects absolute address */
    HAL_LOG("HAL WRITE DIRECT: abs=0x%08X value=0x%08X\n", address, value);
    write_register(address, value, 4);

    return CRC_HAL_OK;
}

uint32_t crc_hal_get_register_address(crc_context_t context, crc_register_t reg)
{
    if (!crc_hal_validate_context(context) || !crc_hal_validate_register(reg) || g_crc_base_address == 0U) {
        return 0xFFFFFFFFU;
    }

    uint32_t base_offset = (uint32_t)context * CRC_CONTEXT_SIZE;

    switch (reg) {
        case CRC_REG_MODE:
            return base_offset + CRC_CTX_MODE_OFFSET;
        case CRC_REG_IVAL:
            return base_offset + CRC_CTX_IVAL_OFFSET;
        case CRC_REG_DATA:
            return base_offset + CRC_CTX_DATA_OFFSET;
        default:
            return 0xFFFFFFFFU;
    }
}

crc_hal_status_t crc_hal_reset_all_contexts(void)
{
    if (!g_crc_hal_initialized) {
        return CRC_HAL_ERROR;
    }

    /* Reset all contexts */
    for (crc_context_t ctx = CRC_CONTEXT_0; ctx < CRC_CONTEXT_MAX; ctx++) {
        crc_hal_status_t status = crc_hal_reset_context(ctx);
        if (status != CRC_HAL_OK) {
            return status;
        }
    }

    return CRC_HAL_OK;
}

crc_hal_status_t crc_hal_reset_context(crc_context_t context)
{
    if (!g_crc_hal_initialized) {
        return CRC_HAL_ERROR;
    }

    if (!crc_hal_validate_context(context)) {
        return CRC_HAL_INVALID_PARAM;
    }

    /* Reset context registers to default values */
    crc_hal_status_t status;

    status = crc_hal_write_register(context, CRC_REG_MODE, 0x00000000U);
    if (status != CRC_HAL_OK) {
        return status;
    }

    status = crc_hal_write_register(context, CRC_REG_IVAL, 0x00000000U);
    if (status != CRC_HAL_OK) {
        return status;
    }

    /* Note: DATA register is write-only, no need to reset */

    return CRC_HAL_OK;
}

/* =============================================================================
 * Private Function Implementations
 * =============================================================================*/

static uint32_t crc_hal_read_raw(uint32_t address)
{
    /* address is an offset within the CRC register space; add base for absolute */
    uint32_t abs_addr = g_crc_base_address + address;
    HAL_LOG("HAL READ: base=0x%08X offset=0x%08X abs=0x%08X\n", g_crc_base_address, address, abs_addr);
    return read_register(abs_addr, 4);
}

static void crc_hal_write_raw(uint32_t address, uint32_t value)
{
    /* address is an offset within the CRC register space; add base for absolute */
    uint32_t abs_addr = g_crc_base_address + address;
    HAL_LOG("HAL WRITE: base=0x%08X offset=0x%08X abs=0x%08X value=0x%08X\n", g_crc_base_address, address, abs_addr, value);
    write_register(abs_addr, value, 4);
}

static bool crc_hal_validate_context(crc_context_t context)
{
    return (context < CRC_CONTEXT_MAX);
}

static bool crc_hal_validate_register(crc_register_t reg)
{
    return (reg < CRC_REG_MAX);
}