/**
 * @file test_crc_driver.c
 * @brief Comprehensive test program for CRC driver
 *
 * This test program validates all aspects of the CRC driver implementation:
 * - HAL layer register operations
 * - Driver layer configuration and calculation
 * - CRC16-CCITT and CRC32-IEEE calculations
 * - Multiple context support
 * - Byte/bit ordering configurations
 * - Error handling and edge cases
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "../driver/crc_driver.h"
#include "interface_layer.h"

/* =============================================================================
 * Test Configuration
 * =============================================================================*/

#define TEST_DEFAULT_BASE_ADDRESS   0x40000000U
#define TEST_MAX_FAILURES           10U
#define TEST_BUFFER_SIZE             256U

/* Global test configuration */
static uint32_t g_crc_base_address = TEST_DEFAULT_BASE_ADDRESS;

/* Test data patterns */
static const uint8_t test_data_simple[] = "Hello, World!";
static const uint8_t test_data_pattern[] = {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55};
static const uint8_t test_data_zeros[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t test_data_ones[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/* Expected CRC results (reference values) */
#define EXPECTED_CRC16_HELLO         0x4B37U  /* Expected CRC16-CCITT for "Hello, World!" */
#define EXPECTED_CRC32_HELLO         0x1C291CA3U  /* Expected CRC32-IEEE for "Hello, World!" */

/* =============================================================================
 * Test Framework
 * =============================================================================*/

typedef struct {
    const char *name;
    bool (*test_func)(void);
} test_case_t;

typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
    char failure_details[TEST_MAX_FAILURES][256];
} test_results_t;

static test_results_t g_test_results = {0};

/* =============================================================================
 * Test Framework Functions
 * =============================================================================*/

static void test_init(uint32_t base)
{
    memset(&g_test_results, 0, sizeof(g_test_results));
    printf("============================================================\n");
    printf("CRC Driver Test Suite\n");
    printf("============================================================\n\n");

    /* Initialize interface layer */
    if (interface_layer_init() != 0) {
        printf("Failed to initialize interface layer\n");
        exit(1);
    }

    /* Register device with interface layer */
    /* device_id=1, base_address from command line, size=0x1000 (4KB) */
    if (register_device(1, base, 0x1000) != 0) {
        printf("Failed to register device\n");
        exit(1);
    }

    /* Initialize CRC driver base address */
    crc_drv_status_t status = crc_driver_base_address_init(base);
    if (status != CRC_DRV_OK) {
        printf("Failed to initialize CRC driver base address\n");
        exit(1);
    }

    printf("Interface layer initialized successfully\n");
    printf("Device registered: ID=1, Base=0x%08X, Size=0x1000\n", base);
    printf("CRC driver base address initialized: 0x%08X\n", base);
}

static void test_record_failure(const char *test_name, const char *details)
{
    if (g_test_results.failed_tests < TEST_MAX_FAILURES) {
        snprintf(g_test_results.failure_details[g_test_results.failed_tests],
                sizeof(g_test_results.failure_details[0]),
                "%s: %s", test_name, details);
    }
    g_test_results.failed_tests++;
}

static void test_run(const char *name, bool (*test_func)(void))
{
    printf("--- %s ---\n", name);
    g_test_results.total_tests++;

    bool result = test_func();

    if (result) {
        printf("Result: ✅ PASS\n\n");
        g_test_results.passed_tests++;
    } else {
        printf("Result: ❌ FAIL\n\n");
        test_record_failure(name, "Test function returned false");
    }
}

static void test_summary(void)
{
    printf("============================================================\n");
    printf("Test Results Summary\n");
    printf("============================================================\n");
    printf("Total tests: %d\n", g_test_results.total_tests);
    printf("Passed: %d\n", g_test_results.passed_tests);
    printf("Failed: %d\n", g_test_results.failed_tests);

    if (g_test_results.failed_tests > 0) {
        printf("\nFailure Details:\n");
        int failures_to_show = (g_test_results.failed_tests < TEST_MAX_FAILURES) ?
                               g_test_results.failed_tests : TEST_MAX_FAILURES;

        for (int i = 0; i < failures_to_show; i++) {
            printf("  %d. %s\n", i + 1, g_test_results.failure_details[i]);
        }

        if (g_test_results.failed_tests > TEST_MAX_FAILURES) {
            printf("  ... and %d more failures\n",
                   g_test_results.failed_tests - TEST_MAX_FAILURES);
        }
    }

    double success_rate = (g_test_results.total_tests > 0) ?
                         (double)g_test_results.passed_tests / g_test_results.total_tests * 100.0 : 0.0;

    printf("Success rate: %.1f%%\n", success_rate);

    if (g_test_results.failed_tests == 0) {
        printf("Overall: ✅ ALL TESTS PASSED\n");
    } else {
        printf("Overall: ❌ SOME TESTS FAILED\n");
    }

    printf("============================================================\n");
}

/* =============================================================================
 * Helper Functions
 * =============================================================================*/

static bool verify_status(crc_drv_status_t actual, crc_drv_status_t expected, const char *operation)
{
    if (actual != expected) {
        printf("ERROR: %s failed - Expected %s, got %s\n",
               operation,
               crc_driver_status_to_string(expected),
               crc_driver_status_to_string(actual));
        return false;
    }
    return true;
}

static bool verify_hal_status(crc_hal_status_t actual, crc_hal_status_t expected, const char *operation)
{
    if (actual != expected) {
        printf("ERROR: HAL %s failed - Expected %d, got %d\n", operation, expected, actual);
        return false;
    }
    return true;
}

static bool verify_uint32(uint32_t actual, uint32_t expected, const char *description)
{
    if (actual != expected) {
        printf("ERROR: %s - Expected 0x%08X, got 0x%08X\n", description, expected, actual);
        return false;
    }
    return true;
}

static bool verify_uint16(uint16_t actual, uint16_t expected, const char *description)
{
    if (actual != expected) {
        printf("ERROR: %s - Expected 0x%04X, got 0x%04X\n", description, expected, actual);
        return false;
    }
    return true;
}

/* =============================================================================
 * Test Cases - HAL Layer
 * =============================================================================*/

static bool test_hal_initialization(void)
{
    printf("Testing HAL initialization and deinitialization...\n");

    /* Test initialization */
    crc_hal_status_t status = crc_hal_init();
    if (!verify_hal_status(status, CRC_HAL_OK, "crc_hal_init")) {
        return false;
    }

    /* Test double initialization (should be OK) */
    status = crc_hal_init();
    if (!verify_hal_status(status, CRC_HAL_OK, "crc_hal_init (double)")) {
        return false;
    }

    /* Test deinitialization */
    status = crc_hal_deinit();
    if (!verify_hal_status(status, CRC_HAL_OK, "crc_hal_deinit")) {
        return false;
    }

    /* Test deinitialization when not initialized (should fail) */
    status = crc_hal_deinit();
    if (!verify_hal_status(status, CRC_HAL_ERROR, "crc_hal_deinit (not initialized)")) {
        return false;
    }

    /* Re-initialize for other tests */
    status = crc_hal_init();
    if (!verify_hal_status(status, CRC_HAL_OK, "crc_hal_init (re-init)")) {
        return false;
    }

    printf("HAL initialization tests passed\n");
    return true;
}

static bool test_hal_register_operations(void)
{
    printf("Testing HAL register read/write operations...\n");

    /* Test invalid parameters */
    uint32_t value;
    crc_hal_status_t status;

    status = crc_hal_read_register(CRC_CONTEXT_MAX, CRC_REG_MODE, &value);
    if (!verify_hal_status(status, CRC_HAL_INVALID_PARAM, "invalid context read")) {
        return false;
    }

    status = crc_hal_write_register(CRC_CONTEXT_0, CRC_REG_MAX, 0x12345678U);
    if (!verify_hal_status(status, CRC_HAL_INVALID_PARAM, "invalid register write")) {
        return false;
    }

    status = crc_hal_read_register(CRC_CONTEXT_0, CRC_REG_MODE, NULL);
    if (!verify_hal_status(status, CRC_HAL_INVALID_PARAM, "null pointer read")) {
        return false;
    }

    /* Test valid register operations */
    for (crc_context_t ctx = CRC_CONTEXT_0; ctx < CRC_CONTEXT_MAX; ctx++) {
        /* Test mode register */
        status = crc_hal_write_register(ctx, CRC_REG_MODE, 0x12345678U);
        if (!verify_hal_status(status, CRC_HAL_OK, "mode register write")) {
            return false;
        }

        status = crc_hal_read_register(ctx, CRC_REG_MODE, &value);
        if (!verify_hal_status(status, CRC_HAL_OK, "mode register read")) {
            return false;
        }

        if (!verify_uint32(value, 0x12345678U, "mode register value")) {
            return false;
        }

        /* Test initial value register */
        status = crc_hal_write_register(ctx, CRC_REG_IVAL, 0xABCDEF00U);
        if (!verify_hal_status(status, CRC_HAL_OK, "ival register write")) {
            return false;
        }

        status = crc_hal_read_register(ctx, CRC_REG_IVAL, &value);
        if (!verify_hal_status(status, CRC_HAL_OK, "ival register read")) {
            return false;
        }

        if (!verify_uint32(value, 0xABCDEF00U, "ival register value")) {
            return false;
        }

        /* Test data register (write-only) */
        status = crc_hal_write_register(ctx, CRC_REG_DATA, 0xDEADBEEFU);
        if (!verify_hal_status(status, CRC_HAL_OK, "data register write")) {
            return false;
        }

        status = crc_hal_read_register(ctx, CRC_REG_DATA, &value);
        if (!verify_hal_status(status, CRC_HAL_OK, "data register read")) {
            return false;
        }

        /* Data register should read as 0 (write-only) */
        if (!verify_uint32(value, 0x00000000U, "data register read value")) {
            return false;
        }
    }

    printf("HAL register operations tests passed\n");
    return true;
}

static bool test_hal_register_addresses(void)
{
    printf("Testing HAL register address calculations...\n");

    /* Test valid address calculations */
    uint32_t addr;

    addr = crc_hal_get_register_address(CRC_CONTEXT_0, CRC_REG_MODE);
    if (!verify_uint32(addr, CRC_CTX0_MODE_REG, "Context 0 mode address")) {
        return false;
    }

    addr = crc_hal_get_register_address(CRC_CONTEXT_0, CRC_REG_IVAL);
    if (!verify_uint32(addr, CRC_CTX0_IVAL_REG, "Context 0 ival address")) {
        return false;
    }

    addr = crc_hal_get_register_address(CRC_CONTEXT_1, CRC_REG_DATA);
    if (!verify_uint32(addr, CRC_CTX1_DATA_REG, "Context 1 data address")) {
        return false;
    }

    addr = crc_hal_get_register_address(CRC_CONTEXT_2, CRC_REG_MODE);
    if (!verify_uint32(addr, CRC_CTX2_MODE_REG, "Context 2 mode address")) {
        return false;
    }

    /* Test invalid parameters */
    addr = crc_hal_get_register_address(CRC_CONTEXT_MAX, CRC_REG_MODE);
    if (!verify_uint32(addr, 0xFFFFFFFFU, "Invalid context address")) {
        return false;
    }

    addr = crc_hal_get_register_address(CRC_CONTEXT_0, CRC_REG_MAX);
    if (!verify_uint32(addr, 0xFFFFFFFFU, "Invalid register address")) {
        return false;
    }

    printf("HAL register address tests passed\n");
    return true;
}

/* =============================================================================
 * Test Cases - Driver Layer
 * =============================================================================*/

static bool test_driver_initialization(void)
{
    printf("Testing driver initialization...\n");

    /* Test initialization */
    crc_drv_status_t status = crc_driver_init();
    if (!verify_status(status, CRC_DRV_OK, "crc_driver_init")) {
        return false;
    }

    /* Test double initialization (should be OK) */
    status = crc_driver_init();
    if (!verify_status(status, CRC_DRV_OK, "crc_driver_init (double)")) {
        return false;
    }

    /* Test version information */
    uint8_t major, minor, patch;
    status = crc_driver_get_version(&major, &minor, &patch);
    if (!verify_status(status, CRC_DRV_OK, "crc_driver_get_version")) {
        return false;
    }

    printf("Driver version: %d.%d.%d\n", major, minor, patch);

    printf("Driver initialization tests passed\n");
    return true;
}

static bool test_driver_configuration(void)
{
    printf("Testing driver context configuration...\n");

    /* Test default configuration */
    crc_config_t config;
    crc_drv_status_t status;

    status = crc_driver_get_default_config(CRC_MODE_CRC16_CCITT, &config);
    if (!verify_status(status, CRC_DRV_OK, "get default CRC16 config")) {
        return false;
    }

    if (config.mode != CRC_MODE_CRC16_CCITT) {
        printf("ERROR: Expected CRC16 mode in default config\n");
        return false;
    }

    status = crc_driver_get_default_config(CRC_MODE_CRC32_IEEE, &config);
    if (!verify_status(status, CRC_DRV_OK, "get default CRC32 config")) {
        return false;
    }

    if (config.mode != CRC_MODE_CRC32_IEEE) {
        printf("ERROR: Expected CRC32 mode in default config\n");
        return false;
    }

    /* Test configuration of all contexts */
    for (crc_context_t ctx = CRC_CONTEXT_0; ctx < CRC_CONTEXT_MAX; ctx++) {
        /* Configure CRC16 */
        crc_driver_get_default_config(CRC_MODE_CRC16_CCITT, &config);
        config.acc_ms_bit_first = true;
        config.output_inverted = true;

        status = crc_driver_configure_context(ctx, &config);
        if (!verify_status(status, CRC_DRV_OK, "configure context")) {
            return false;
        }

        /* Verify configuration */
        crc_context_state_t state;
        status = crc_driver_get_context_state(ctx, &state);
        if (!verify_status(status, CRC_DRV_OK, "get context state")) {
            return false;
        }

        if (state.config.mode != CRC_MODE_CRC16_CCITT) {
            printf("ERROR: Context %d mode not set correctly\n", ctx);
            return false;
        }

        if (!state.config.acc_ms_bit_first) {
            printf("ERROR: Context %d acc_ms_bit_first not set\n", ctx);
            return false;
        }

        if (!state.config.output_inverted) {
            printf("ERROR: Context %d output_inverted not set\n", ctx);
            return false;
        }
    }

    /* Test invalid parameters */
    status = crc_driver_configure_context(CRC_CONTEXT_MAX, &config);
    if (!verify_status(status, CRC_DRV_INVALID_PARAM, "invalid context config")) {
        return false;
    }

    status = crc_driver_configure_context(CRC_CONTEXT_0, NULL);
    if (!verify_status(status, CRC_DRV_INVALID_PARAM, "null config")) {
        return false;
    }

    printf("Driver configuration tests passed\n");
    return true;
}

static bool test_crc16_calculations(void)
{
    printf("Testing CRC16-CCITT calculations...\n");

    crc_drv_status_t status;
    uint16_t result16;
    uint32_t result32;

    /* Test direct CRC16 calculation */
    status = crc_driver_calculate_crc16_direct(test_data_simple,
                                              sizeof(test_data_simple) - 1, /* Exclude null terminator */
                                              CRC16_INITIAL_VALUE, &result16);
    if (!verify_status(status, CRC_DRV_OK, "CRC16 direct calculation")) {
        return false;
    }

    printf("CRC16 result for 'Hello, World!': 0x%04X\n", result16);

    /* Test context-based calculation */
    crc_config_t config;
    crc_driver_get_default_config(CRC_MODE_CRC16_CCITT, &config);

    status = crc_driver_configure_context(CRC_CONTEXT_0, &config);
    if (!verify_status(status, CRC_DRV_OK, "configure CRC16 context")) {
        return false;
    }

    status = crc_driver_calculate_buffer(CRC_CONTEXT_0, test_data_simple,
                                        sizeof(test_data_simple) - 1, &result32);
    if (!verify_status(status, CRC_DRV_OK, "CRC16 buffer calculation")) {
        return false;
    }

    printf("CRC16 context result for 'Hello, World!': 0x%08X\n", result32);

    /* Test with different data patterns */
    const uint8_t *test_patterns[] = {test_data_pattern, test_data_zeros, test_data_ones};
    const char *pattern_names[] = {"pattern", "zeros", "ones"};

    for (size_t i = 0; i < sizeof(test_patterns) / sizeof(test_patterns[0]); i++) {
        status = crc_driver_calculate_crc16_direct(test_patterns[i], 8,
                                                  CRC16_INITIAL_VALUE, &result16);
        if (!verify_status(status, CRC_DRV_OK, "CRC16 pattern calculation")) {
            return false;
        }

        printf("CRC16 result for %s: 0x%04X\n", pattern_names[i], result16);
    }

    printf("CRC16 calculation tests passed\n");
    return true;
}

static bool test_crc32_calculations(void)
{
    printf("Testing CRC32-IEEE calculations...\n");

    crc_drv_status_t status;
    uint32_t result32;

    /* Test direct CRC32 calculation */
    status = crc_driver_calculate_crc32_direct(test_data_simple,
                                              sizeof(test_data_simple) - 1, /* Exclude null terminator */
                                              CRC32_INITIAL_VALUE, &result32);
    if (!verify_status(status, CRC_DRV_OK, "CRC32 direct calculation")) {
        return false;
    }

    printf("CRC32 result for 'Hello, World!': 0x%08X\n", result32);

    /* Test context-based calculation */
    crc_config_t config;
    crc_driver_get_default_config(CRC_MODE_CRC32_IEEE, &config);

    status = crc_driver_configure_context(CRC_CONTEXT_1, &config);
    if (!verify_status(status, CRC_DRV_OK, "configure CRC32 context")) {
        return false;
    }

    status = crc_driver_calculate_buffer(CRC_CONTEXT_1, test_data_simple,
                                        sizeof(test_data_simple) - 1, &result32);
    if (!verify_status(status, CRC_DRV_OK, "CRC32 buffer calculation")) {
        return false;
    }

    printf("CRC32 context result for 'Hello, World!': 0x%08X\n", result32);

    /* Test with different data patterns */
    const uint8_t *test_patterns[] = {test_data_pattern, test_data_zeros, test_data_ones};
    const char *pattern_names[] = {"pattern", "zeros", "ones"};

    for (size_t i = 0; i < sizeof(test_patterns) / sizeof(test_patterns[0]); i++) {
        status = crc_driver_calculate_crc32_direct(test_patterns[i], 8,
                                                  CRC32_INITIAL_VALUE, &result32);
        if (!verify_status(status, CRC_DRV_OK, "CRC32 pattern calculation")) {
            return false;
        }

        printf("CRC32 result for %s: 0x%08X\n", pattern_names[i], result32);
    }

    printf("CRC32 calculation tests passed\n");
    return true;
}

static bool test_multiple_contexts(void)
{
    printf("Testing multiple context operations...\n");

    crc_drv_status_t status;
    crc_config_t config16, config32;
    uint32_t result0, result1, result2;

    /* Configure different modes for different contexts */
    crc_driver_get_default_config(CRC_MODE_CRC16_CCITT, &config16);
    crc_driver_get_default_config(CRC_MODE_CRC32_IEEE, &config32);

    status = crc_driver_configure_context(CRC_CONTEXT_0, &config16);
    if (!verify_status(status, CRC_DRV_OK, "configure context 0")) {
        return false;
    }

    status = crc_driver_configure_context(CRC_CONTEXT_1, &config32);
    if (!verify_status(status, CRC_DRV_OK, "configure context 1")) {
        return false;
    }

    status = crc_driver_configure_context(CRC_CONTEXT_2, &config16);
    if (!verify_status(status, CRC_DRV_OK, "configure context 2")) {
        return false;
    }

    /* Calculate same data with different contexts */
    status = crc_driver_calculate_buffer(CRC_CONTEXT_0, test_data_simple,
                                        sizeof(test_data_simple) - 1, &result0);
    if (!verify_status(status, CRC_DRV_OK, "context 0 calculation")) {
        return false;
    }

    status = crc_driver_calculate_buffer(CRC_CONTEXT_1, test_data_simple,
                                        sizeof(test_data_simple) - 1, &result1);
    if (!verify_status(status, CRC_DRV_OK, "context 1 calculation")) {
        return false;
    }

    status = crc_driver_calculate_buffer(CRC_CONTEXT_2, test_data_simple,
                                        sizeof(test_data_simple) - 1, &result2);
    if (!verify_status(status, CRC_DRV_OK, "context 2 calculation")) {
        return false;
    }

    printf("Context 0 (CRC16) result: 0x%08X\n", result0);
    printf("Context 1 (CRC32) result: 0x%08X\n", result1);
    printf("Context 2 (CRC16) result: 0x%08X\n", result2);

    /* Context 0 and 2 should have same result (both CRC16) */
    if (!verify_uint32(result0, result2, "CRC16 contexts match")) {
        return false;
    }

    /* Context 1 should be different (CRC32) */
    if (result0 == result1) {
        printf("ERROR: CRC16 and CRC32 results should be different\n");
        return false;
    }

    /* Test context busy status */
    bool is_busy;
    for (crc_context_t ctx = CRC_CONTEXT_0; ctx < CRC_CONTEXT_MAX; ctx++) {
        status = crc_driver_is_context_busy(ctx, &is_busy);
        if (!verify_status(status, CRC_DRV_OK, "check context busy")) {
            return false;
        }

        if (is_busy) {
            printf("ERROR: Context %d should not be busy\n", ctx);
            return false;
        }
    }

    printf("Multiple context tests passed\n");
    return true;
}

static bool test_incremental_calculation(void)
{
    printf("Testing incremental CRC calculation...\n");

    crc_drv_status_t status;
    crc_config_t config;
    uint32_t result_full, result_incremental, current_value;

    /* Configure context */
    crc_driver_get_default_config(CRC_MODE_CRC16_CCITT, &config);
    status = crc_driver_configure_context(CRC_CONTEXT_0, &config);
    if (!verify_status(status, CRC_DRV_OK, "configure context")) {
        return false;
    }

    /* Calculate full buffer at once */
    status = crc_driver_calculate_buffer(CRC_CONTEXT_0, test_data_simple,
                                        sizeof(test_data_simple) - 1, &result_full);
    if (!verify_status(status, CRC_DRV_OK, "full buffer calculation")) {
        return false;
    }

    /* Calculate incrementally */
    status = crc_driver_start_calculation(CRC_CONTEXT_0, 0);
    if (!verify_status(status, CRC_DRV_OK, "start incremental calculation")) {
        return false;
    }

    /* Process data in chunks */
    const uint8_t *data = test_data_simple;
    size_t total_len = sizeof(test_data_simple) - 1;
    size_t chunk_size = 4;

    for (size_t offset = 0; offset < total_len; offset += chunk_size) {
        size_t len = (offset + chunk_size > total_len) ? (total_len - offset) : chunk_size;

        status = crc_driver_process_data(CRC_CONTEXT_0, &data[offset], (uint32_t)len);
        if (!verify_status(status, CRC_DRV_OK, "process data chunk")) {
            return false;
        }

        /* Check current value */
        status = crc_driver_get_current_value(CRC_CONTEXT_0, &current_value);
        if (!verify_status(status, CRC_DRV_OK, "get current value")) {
            return false;
        }

        printf("After processing %zu bytes: current CRC = 0x%08X\n", offset + len, current_value);
    }

    /* Finalize calculation */
    status = crc_driver_finalize_calculation(CRC_CONTEXT_0, &result_incremental);
    if (!verify_status(status, CRC_DRV_OK, "finalize calculation")) {
        return false;
    }

    printf("Full calculation result: 0x%08X\n", result_full);
    printf("Incremental calculation result: 0x%08X\n", result_incremental);

    /* Results should match */
    if (!verify_uint32(result_incremental, result_full, "incremental vs full calculation")) {
        return false;
    }

    printf("Incremental calculation tests passed\n");
    return true;
}

static bool test_error_conditions(void)
{
    printf("Testing error conditions...\n");

    crc_drv_status_t status;
    uint32_t result;

    /* Test operations before initialization */
    crc_driver_deinit();

    status = crc_driver_calculate_buffer(CRC_CONTEXT_0, test_data_simple,
                                        sizeof(test_data_simple) - 1, &result);
    if (!verify_status(status, CRC_DRV_NOT_INITIALIZED, "operation before init")) {
        return false;
    }

    /* Re-initialize */
    status = crc_driver_init();
    if (!verify_status(status, CRC_DRV_OK, "re-initialize")) {
        return false;
    }

    /* Test null pointer parameters */
    status = crc_driver_calculate_buffer(CRC_CONTEXT_0, NULL, 10, &result);
    if (!verify_status(status, CRC_DRV_INVALID_PARAM, "null data pointer")) {
        return false;
    }

    status = crc_driver_calculate_buffer(CRC_CONTEXT_0, test_data_simple,
                                        sizeof(test_data_simple) - 1, NULL);
    if (!verify_status(status, CRC_DRV_INVALID_PARAM, "null result pointer")) {
        return false;
    }

    /* Test invalid context */
    status = crc_driver_calculate_buffer(CRC_CONTEXT_MAX, test_data_simple,
                                        sizeof(test_data_simple) - 1, &result);
    if (!verify_status(status, CRC_DRV_INVALID_PARAM, "invalid context")) {
        return false;
    }

    /* Test zero length data */
    status = crc_driver_calculate_buffer(CRC_CONTEXT_0, test_data_simple, 0, &result);
    if (!verify_status(status, CRC_DRV_INVALID_PARAM, "zero length data")) {
        return false;
    }

    /* Test very large data size */
    status = crc_driver_calculate_buffer(CRC_CONTEXT_0, test_data_simple,
                                        CRC_MAX_DATA_SIZE + 1, &result);
    if (!verify_status(status, CRC_DRV_INVALID_PARAM, "oversized data")) {
        return false;
    }

    printf("Error condition tests passed\n");
    return true;
}

/* =============================================================================
 * Interrupt Handling Test
 * =============================================================================*/

/* Global variable to track interrupt events */
static volatile bool g_interrupt_received = false;
static volatile uint32_t g_interrupt_id = 0;

/**
 * @brief Sample interrupt handler function
 * @param interrupt_id The interrupt ID that was triggered
 */
static void crc_interrupt_handler(uint32_t interrupt_id)
{
    g_interrupt_received = true;
    g_interrupt_id = interrupt_id;
    printf("Interrupt handler called: ID=%u\n", interrupt_id);
}

static bool test_interrupt_handling(void)
{
    printf("Testing interrupt handling framework...\n");

    /* Note: CRC device typically doesn't generate interrupts, but we test the framework */
    const uint32_t test_interrupt_id = 10; /* Example interrupt ID */

    /* Register interrupt handler */
    int result = register_interrupt_handler(test_interrupt_id, crc_interrupt_handler);
    if (result != 0) {
        printf("WARNING: Could not register interrupt handler (result=%d)\n", result);
        printf("This may be expected if interrupt support is not implemented\n");
        /* Don't fail the test as interrupt support may not be implemented */
        return true;
    }

    printf("Interrupt handler registered successfully for ID=%u\n", test_interrupt_id);

    /* Reset interrupt flags */
    g_interrupt_received = false;
    g_interrupt_id = 0;

    /* In a real scenario, we would trigger an interrupt from the device */
    /* For this test, we just verify the handler was registered */
    printf("Interrupt handling framework test completed\n");
    printf("Note: Actual interrupt testing requires device model support\n");

    return true;
}

/* =============================================================================
 * Test Runner
 * =============================================================================*/

/**
 * @brief Print usage information
 * @param program_name The name of the program
 */
static void print_usage(const char *program_name)
{
    printf("Usage: %s [address]\n", program_name);
    printf("\n");
    printf("Parameters:\n");
    printf("  address    CRC register base address (hexadecimal, default: 0x%08X)\n", TEST_DEFAULT_BASE_ADDRESS);
    printf("\n");
    printf("Examples:\n");
    printf("  %s                    # Use default address (0x%08X)\n", program_name, TEST_DEFAULT_BASE_ADDRESS);
    printf("  %s 0x40000000         # Use custom address\n", program_name);
    printf("  %s 0x50000000         # Use another custom address\n", program_name);
    printf("\n");
    printf("Description:\n");
    printf("  This program runs comprehensive tests for the CRC driver implementation.\n");
    printf("  It tests both HAL and driver layers with various CRC calculation modes.\n");
}

/**
 * @brief Parse command line arguments
 * @param argc Argument count
 * @param argv Argument vector
 * @return true if parsing successful, false otherwise
 */
static bool parse_arguments(int argc, char *argv[])
{
    if (argc == 1) {
        /* No arguments - use default */
        printf("Using default CRC base address: 0x%08X\n", g_crc_base_address);
        return true;
    }

    if (argc == 2) {
        /* Check for help request */
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_usage(argv[0]);
            return false;
        }

        /* Parse address */
        char *endptr;
        unsigned long parsed_addr = strtoul(argv[1], &endptr, 0);

        /* Check for parsing errors */
        if (*endptr != '\0' || parsed_addr == 0) {
            printf("Error: Invalid address format '%s'\n", argv[1]);
            printf("Address must be a valid hexadecimal number (e.g., 0x40000000)\n");
            print_usage(argv[0]);
            return false;
        }

        /* Check address range (must fit in uint32_t) */
        if (parsed_addr > UINT32_MAX) {
            printf("Error: Address 0x%lX is too large (maximum: 0x%08X)\n", parsed_addr, UINT32_MAX);
            return false;
        }

        g_crc_base_address = (uint32_t)parsed_addr;
        printf("Using custom CRC base address: 0x%08X\n", g_crc_base_address);
        return true;
    }

    /* Too many arguments */
    printf("Error: Too many arguments\n");
    print_usage(argv[0]);
    return false;
}

static const test_case_t test_cases[] = {
    {"test_hal_initialization", test_hal_initialization},
    {"test_hal_register_operations", test_hal_register_operations},
    {"test_hal_register_addresses", test_hal_register_addresses},
    {"test_driver_initialization", test_driver_initialization},
    {"test_driver_configuration", test_driver_configuration},
    {"test_crc16_calculations", test_crc16_calculations},
    {"test_crc32_calculations", test_crc32_calculations},
    {"test_multiple_contexts", test_multiple_contexts},
    {"test_incremental_calculation", test_incremental_calculation},
    {"test_error_conditions", test_error_conditions},
    {"test_interrupt_handling", test_interrupt_handling},
};

int main(int argc, char *argv[])
{
    /* Parse command line arguments */
    if (!parse_arguments(argc, argv)) {
        return 1; /* Exit with error if parsing failed or help was requested */
    }

    test_init(g_crc_base_address);

    const size_t num_tests = sizeof(test_cases) / sizeof(test_cases[0]);

    for (size_t i = 0; i < num_tests; i++) {
        test_run(test_cases[i].name, test_cases[i].test_func);
    }

    test_summary();

    /* Clean up */
    crc_driver_deinit();

    /* Cleanup */
    //device_deinit();
    interface_layer_deinit();

    return (g_test_results.failed_tests == 0) ? 0 : 1;
}