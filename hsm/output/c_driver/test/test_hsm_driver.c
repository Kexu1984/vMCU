/**
 * @file test_hsm_driver.c
 * @brief Comprehensive test suite for HSM driver
 *
 * This test program validates all HSM driver functionality including:
 * - HAL layer register access
 * - Driver layer business logic
 * - AES encryption/decryption operations
 * - CMAC generation and verification
 * - Authentication mechanisms
 * - Random number generation
 * - Error handling and boundary conditions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

#include "../hal/hsm_hal.h"
#include "../driver/hsm_driver.h"
#include "../interface_layer.h"

/**
 * @brief Test framework definitions
 */
#define TEST_PASS   1
#define TEST_FAIL   0

typedef struct {
    const char *test_name;
    int (*test_func)(void);
} test_case_t;

/**
 * @brief Test statistics
 */
static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

/**
 * @brief Default base address
 */
#define DEFAULT_BASE_ADDRESS 0x40000000

/**
 * @brief Test data patterns
 */
static const uint8_t test_data_16[] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
};

static const uint8_t test_key_128[] = {
    0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
    0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
};

static const uint8_t test_iv[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

/**
 * @brief Test helper functions
 */
void print_hex(const char *label, const uint8_t *data, size_t len)
{
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

int run_test(const char *test_name, int (*test_func)(void))
{
    printf("Running test: %s... ", test_name);
    fflush(stdout);

    int result = test_func();
    g_tests_run++;

    if (result == TEST_PASS) {
        printf("PASS\n");
        g_tests_passed++;
    } else {
        printf("FAIL\n");
        g_tests_failed++;
    }

    return result;
}

/**
 * @brief Test UID register reading
 */
int test_uid_registers(void)
{
    uint8_t uid[16];

    hsm_drv_status_t status = hsm_driver_get_uid(uid);
    if (status != HSM_DRV_OK) {
        printf("UID read failed: %d\n", status);
        return TEST_FAIL;
    }

    print_hex("UID", uid, 16);

    // Verify UID is not all zeros (should have OTP values)
    bool all_zero = true;
    for (int i = 0; i < 16; i++) {
        if (uid[i] != 0) {
            all_zero = false;
            break;
        }
    }

    if (all_zero) {
        printf("UID is all zeros - unexpected\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/**
 * @brief Test system status reading
 */
int test_system_status(void)
{
    bool tx_empty, rx_full, busy;

    hsm_drv_status_t status = hsm_driver_get_status(&tx_empty, &rx_full, &busy);
    if (status != HSM_DRV_OK) {
        printf("Status read failed: %d\n", status);
        return TEST_FAIL;
    }

    printf("System status - TX empty: %d, RX full: %d, Busy: %d\n",
           tx_empty, rx_full, busy);

    // Initially, TX should be empty and device should not be busy
    if (!tx_empty || busy) {
        printf("Unexpected initial status\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/**
 * @brief Test configuration management
 */
int test_configuration(void)
{
    hsm_config_t config;

    // Test get default configuration
    hsm_drv_status_t status = hsm_driver_get_default_config(HSM_MODE_ECB, &config);
    if (status != HSM_DRV_OK) {
        printf("Get default config failed: %d\n", status);
        return TEST_FAIL;
    }

    // Verify default values
    if (config.mode != HSM_MODE_ECB ||
        config.decrypt_enable != false ||
        config.key_index != 0 ||
        config.aes256_enable != false) {
        printf("Default config values incorrect\n");
        return TEST_FAIL;
    }

    // Test context configuration
    config.mode = HSM_MODE_CBC;
    config.key_index = HSM_KEY_TYPE_RAM;
    config.aes256_enable = false;
    config.data_length = 16;
    config.interrupt_enable = true;
    memcpy(config.iv, test_iv, 16);
    memcpy(config.key, test_key_128, 16);

    status = hsm_driver_configure_context(0, &config);
    if (status != HSM_DRV_OK) {
        printf("Configure context failed: %d\n", status);
        return TEST_FAIL;
    }

    // Test get context configuration
    hsm_config_t read_config;
    status = hsm_driver_get_context_config(0, &read_config);
    if (status != HSM_DRV_OK) {
        printf("Get context config failed: %d\n", status);
        return TEST_FAIL;
    }

    // Verify configuration was stored correctly
    if (read_config.mode != HSM_MODE_CBC ||
        read_config.key_index != HSM_KEY_TYPE_RAM ||
        memcmp(read_config.iv, test_iv, 16) != 0) {
        printf("Context config mismatch\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/**
 * @brief Test AES ECB encryption/decryption
 */
int test_aes_ecb(void)
{
    hsm_config_t config;
    uint8_t encrypted[16];
    uint8_t decrypted[16];
    uint32_t output_len;

    // Configure for ECB encryption
    hsm_drv_status_t status = hsm_driver_get_default_config(HSM_MODE_ECB, &config);
    if (status != HSM_DRV_OK) {
        return TEST_FAIL;
    }

    config.key_index = HSM_KEY_TYPE_RAM;
    config.decrypt_enable = false;
    memcpy(config.key, test_key_128, 16);

    status = hsm_driver_configure_context(0, &config);
    if (status != HSM_DRV_OK) {
        printf("ECB config failed: %d\n", status);
        return TEST_FAIL;
    }

    // Test encryption
    output_len = sizeof(encrypted);
    status = hsm_driver_aes_operation(0, test_data_16, 16, encrypted, &output_len);
    if (status != HSM_DRV_OK) {
        printf("ECB encryption failed: %d\n", status);
        return TEST_FAIL;
    }

    if (output_len != 16) {
        printf("ECB encryption output length mismatch: %u\n", output_len);
        return TEST_FAIL;
    }

    print_hex("ECB plaintext", test_data_16, 16);
    print_hex("ECB encrypted", encrypted, 16);

    // Test decryption
    config.decrypt_enable = true;
    status = hsm_driver_configure_context(0, &config);
    if (status != HSM_DRV_OK) {
        return TEST_FAIL;
    }

    output_len = sizeof(decrypted);
    status = hsm_driver_aes_operation(0, encrypted, 16, decrypted, &output_len);
    if (status != HSM_DRV_OK) {
        printf("ECB decryption failed: %d\n", status);
        return TEST_FAIL;
    }

    print_hex("ECB decrypted", decrypted, 16);

    // Verify decryption matches original
    if (memcmp(test_data_16, decrypted, 16) != 0) {
        printf("ECB decryption mismatch\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/**
 * @brief Test AES CBC encryption/decryption
 */
int test_aes_cbc(void)
{
    hsm_config_t config;
    uint8_t encrypted[16];
    uint8_t decrypted[16];
    uint32_t output_len;

    // Configure for CBC encryption
    hsm_drv_status_t status = hsm_driver_get_default_config(HSM_MODE_CBC, &config);
    if (status != HSM_DRV_OK) {
        return TEST_FAIL;
    }

    config.key_index = HSM_KEY_TYPE_RAM;
    config.decrypt_enable = false;
    memcpy(config.key, test_key_128, 16);
    memcpy(config.iv, test_iv, 16);

    status = hsm_driver_configure_context(0, &config);
    if (status != HSM_DRV_OK) {
        printf("CBC config failed: %d\n", status);
        return TEST_FAIL;
    }

    // Test encryption
    output_len = sizeof(encrypted);
    status = hsm_driver_aes_operation(0, test_data_16, 16, encrypted, &output_len);
    if (status != HSM_DRV_OK) {
        printf("CBC encryption failed: %d\n", status);
        return TEST_FAIL;
    }

    print_hex("CBC plaintext", test_data_16, 16);
    print_hex("CBC encrypted", encrypted, 16);

    // Test decryption
    config.decrypt_enable = true;
    status = hsm_driver_configure_context(0, &config);
    if (status != HSM_DRV_OK) {
        return TEST_FAIL;
    }

    output_len = sizeof(decrypted);
    status = hsm_driver_aes_operation(0, encrypted, 16, decrypted, &output_len);
    if (status != HSM_DRV_OK) {
        printf("CBC decryption failed: %d\n", status);
        return TEST_FAIL;
    }

    print_hex("CBC decrypted", decrypted, 16);

    // Verify decryption matches original
    if (memcmp(test_data_16, decrypted, 16) != 0) {
        printf("CBC decryption mismatch\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/**
 * @brief Test CMAC generation
 */
int test_cmac_generation(void)
{
    hsm_config_t config;
    uint8_t mac_output[16];

    // Configure for CMAC
    hsm_drv_status_t status = hsm_driver_get_default_config(HSM_MODE_CMAC, &config);
    if (status != HSM_DRV_OK) {
        return TEST_FAIL;
    }

    config.key_index = HSM_KEY_TYPE_RAM;
    memcpy(config.key, test_key_128, 16);

    status = hsm_driver_configure_context(0, &config);
    if (status != HSM_DRV_OK) {
        printf("CMAC config failed: %d\n", status);
        return TEST_FAIL;
    }

    // Compute CMAC
    status = hsm_driver_compute_cmac(0, test_data_16, 16, mac_output);
    if (status != HSM_DRV_OK) {
        printf("CMAC computation failed: %d\n", status);
        return TEST_FAIL;
    }

    print_hex("CMAC input", test_data_16, 16);
    print_hex("CMAC output", mac_output, 16);

    // Verify MAC is not all zeros (basic sanity check)
    bool all_zero = true;
    for (int i = 0; i < 16; i++) {
        if (mac_output[i] != 0) {
            all_zero = false;
            break;
        }
    }

    if (all_zero) {
        printf("CMAC output is all zeros - unexpected\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/**
 * @brief Test CMAC verification
 */
int test_cmac_verification(void)
{
    hsm_config_t config;
    uint8_t mac_output[16];
    bool verify_result;

    // Configure for CMAC
    hsm_drv_status_t status = hsm_driver_get_default_config(HSM_MODE_CMAC, &config);
    if (status != HSM_DRV_OK) {
        return TEST_FAIL;
    }

    config.key_index = HSM_KEY_TYPE_RAM;
    memcpy(config.key, test_key_128, 16);

    status = hsm_driver_configure_context(0, &config);
    if (status != HSM_DRV_OK) {
        return TEST_FAIL;
    }

    // First compute MAC
    status = hsm_driver_compute_cmac(0, test_data_16, 16, mac_output);
    if (status != HSM_DRV_OK) {
        return TEST_FAIL;
    }

    // Then verify with correct MAC
    status = hsm_driver_verify_cmac(0, test_data_16, 16, mac_output, &verify_result);
    if (status != HSM_DRV_OK) {
        printf("CMAC verification failed: %d\n", status);
        return TEST_FAIL;
    }

    if (!verify_result) {
        printf("CMAC verification result incorrect - should be true\n");
        return TEST_FAIL;
    }

    // Test with incorrect MAC
    uint8_t wrong_mac[16];
    memcpy(wrong_mac, mac_output, 16);
    wrong_mac[0] ^= 0xFF; // Corrupt first byte

    status = hsm_driver_verify_cmac(0, test_data_16, 16, wrong_mac, &verify_result);
    if (status != HSM_DRV_OK) {
        printf("CMAC verification with wrong MAC failed: %d\n", status);
        return TEST_FAIL;
    }

    if (verify_result) {
        printf("CMAC verification result incorrect - should be false\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/**
 * @brief Test random number generation
 */
int test_random_generation(void)
{
    uint8_t random1[32];
    uint8_t random2[32];

    // Generate first random block
    hsm_drv_status_t status = hsm_driver_generate_random(random1, sizeof(random1));
    if (status != HSM_DRV_OK) {
        printf("Random generation 1 failed: %d\n", status);
        return TEST_FAIL;
    }

    // Generate second random block
    status = hsm_driver_generate_random(random2, sizeof(random2));
    if (status != HSM_DRV_OK) {
        printf("Random generation 2 failed: %d\n", status);
        return TEST_FAIL;
    }

    print_hex("Random 1", random1, 32);
    print_hex("Random 2", random2, 32);

    // Verify the two blocks are different (very high probability)
    if (memcmp(random1, random2, 32) == 0) {
        printf("Random blocks are identical - unexpected\n");
        return TEST_FAIL;
    }

    // Basic entropy check - should not be all same value
    bool all_same = true;
    for (int i = 1; i < 32; i++) {
        if (random1[i] != random1[0]) {
            all_same = false;
            break;
        }
    }

    if (all_same) {
        printf("Random data has no entropy - all bytes same\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/**
 * @brief Test authentication mechanism
 */
int test_authentication(void)
{
    hsm_auth_result_t auth_result;

    // Start authentication to get challenge
    hsm_drv_status_t status = hsm_driver_start_authentication(&auth_result);
    if (status != HSM_DRV_OK) {
        printf("Start authentication failed: %d\n", status);
        return TEST_FAIL;
    }

    printf("Auth challenge valid: %d\n", auth_result.challenge_valid);

    if (!auth_result.challenge_valid) {
        printf("Challenge not valid after auth start\n");
        return TEST_FAIL;
    }

    print_hex("Auth challenge", auth_result.challenge, 16);

    // For testing, submit the challenge as response (will fail in real system)
    uint8_t test_response[16];
    memcpy(test_response, auth_result.challenge, 16);

    status = hsm_driver_submit_auth_response(test_response, &auth_result);
    if (status != HSM_DRV_OK) {
        printf("Submit auth response failed: %d\n", status);
        return TEST_FAIL;
    }

    printf("Auth success: %d, Auth timeout: %d\n",
           auth_result.auth_success, auth_result.auth_timeout);

    // Authentication should fail with wrong response
    if (auth_result.auth_success) {
        printf("Authentication unexpectedly succeeded\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/**
 * @brief Test error conditions
 */
int test_error_conditions(void)
{
    hsm_drv_status_t status;
    uint8_t buffer[16];
    uint32_t output_len = sizeof(buffer);

    // Test invalid context ID
    status = hsm_driver_aes_operation(999, test_data_16, 16, buffer, &output_len);
    if (status != HSM_DRV_INVALID_PARAM) {
        printf("Expected INVALID_PARAM for bad context ID, got %d\n", status);
        return TEST_FAIL;
    }

    // Test NULL pointers
    status = hsm_driver_aes_operation(0, NULL, 16, buffer, &output_len);
    if (status != HSM_DRV_INVALID_PARAM) {
        printf("Expected INVALID_PARAM for NULL input, got %d\n", status);
        return TEST_FAIL;
    }

    status = hsm_driver_get_uid(NULL);
    if (status != HSM_DRV_INVALID_PARAM) {
        printf("Expected INVALID_PARAM for NULL UID buffer, got %d\n", status);
        return TEST_FAIL;
    }

    // Test operation on unconfigured context
    status = hsm_driver_aes_operation(0, test_data_16, 16, buffer, &output_len);
    if (status == HSM_DRV_OK) {
        printf("Expected error for unconfigured context\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/**
 * @brief Test interrupt handling
 */
int test_interrupt_handling(void)
{
    uint32_t irq_status;

    // Read initial interrupt status
    hsm_drv_status_t status = hsm_driver_get_interrupt_status(&irq_status);
    if (status != HSM_DRV_OK) {
        printf("Get IRQ status failed: %d\n", status);
        return TEST_FAIL;
    }

    printf("Initial IRQ status: 0x%08X\n", irq_status);

    // Clear interrupts
    status = hsm_driver_clear_interrupt();
    if (status != HSM_DRV_OK) {
        printf("Clear IRQ failed: %d\n", status);
        return TEST_FAIL;
    }

    // Read status again to verify clearing
    status = hsm_driver_get_interrupt_status(&irq_status);
    if (status != HSM_DRV_OK) {
        printf("Get IRQ status after clear failed: %d\n", status);
        return TEST_FAIL;
    }

    printf("IRQ status after clear: 0x%08X\n", irq_status);

    if (irq_status != 0) {
        printf("IRQ status not cleared properly\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/**
 * @brief Test boundary conditions
 */
int test_boundary_conditions(void)
{
    // Test with maximum data length
    const uint32_t max_len = 0xFFFFF; // 20-bit limit
    hsm_config_t config;

    hsm_drv_status_t status = hsm_driver_get_default_config(HSM_MODE_RANDOM, &config);
    if (status != HSM_DRV_OK) {
        return TEST_FAIL;
    }

    config.data_length = max_len;
    status = hsm_driver_configure_context(0, &config);
    if (status != HSM_DRV_OK) {
        printf("Config with max length failed: %d\n", status);
        return TEST_FAIL;
    }

    // Test with invalid data length (too large)
    config.data_length = max_len + 1;
    status = hsm_driver_configure_context(0, &config);
    if (status != HSM_DRV_INVALID_PARAM) {
        printf("Expected INVALID_PARAM for oversized length, got %d\n", status);
        return TEST_FAIL;
    }

    // Test with invalid key index
    config.data_length = 16;
    config.key_index = 23; // Beyond valid range
    status = hsm_driver_configure_context(0, &config);
    if (status != HSM_DRV_INVALID_PARAM) {
        printf("Expected INVALID_PARAM for invalid key index, got %d\n", status);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/**
 * @brief Test cleanup and deinitialization
 */
int test_cleanup(void)
{
    // Test driver deinitialization
    hsm_drv_status_t drv_status = hsm_driver_deinit();
    if (drv_status != HSM_DRV_OK) {
        printf("Driver deinit failed: %d\n", drv_status);
        return TEST_FAIL;
    }

    // Test HAL deinitialization
    hsm_hal_status_t hal_status = hsm_hal_deinit();
    if (hal_status != HSM_HAL_OK) {
        printf("HAL deinit failed: %d\n", hal_status);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/**
 * @brief Test suite definition
 */
static const test_case_t test_suite[] = {
    {"UID Registers", test_uid_registers},
    {"System Status", test_system_status},
    {"Configuration Management", test_configuration},
    {"AES ECB Mode", test_aes_ecb},
    {"AES CBC Mode", test_aes_cbc},
    {"CMAC Generation", test_cmac_generation},
    {"CMAC Verification", test_cmac_verification},
    {"Random Generation", test_random_generation},
    {"Authentication", test_authentication},
    {"Error Conditions", test_error_conditions},
    {"Interrupt Handling", test_interrupt_handling},
    {"Boundary Conditions", test_boundary_conditions},
    {"Cleanup", test_cleanup},
};

/**
 * @brief Initialize test environment
 */
int test_init(uint32_t base_address)
{
    printf("Initializing HSM test environment with base address 0x%08X\n", base_address);

    // Initialize interface layer
    int result = interface_layer_init();
    if (result != 0) {
        printf("Interface layer initialization failed: %d\n", result);
        return TEST_FAIL;
    }

    // Register device with Python model
    result = register_device(1, base_address, 0x1000);
    if (result != 0) {
        printf("Device registration failed: %d\n", result);
        interface_layer_deinit();
        return TEST_FAIL;
    }

    hsm_driver_base_address_init(base_address);
    hsm_driver_init();
    printf("Test environment initialized successfully\n");
    return TEST_PASS;
}

/**
 * @brief Print usage information
 */
void print_usage(const char *program_name)
{
    printf("Usage: %s [base_address]\n", program_name);
    printf("  base_address: HSM device base address (default: 0x%08X)\n", DEFAULT_BASE_ADDRESS);
    printf("  --help      : Show this help message\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s                    # Use default address\n", program_name);
    printf("  %s 0x40000000         # Use custom address\n", program_name);
}

/**
 * @brief Main test program
 */
int main(int argc, char *argv[])
{
    uint32_t base_address = DEFAULT_BASE_ADDRESS;

    // Parse command line arguments
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }

        char *endptr;
        base_address = strtoul(argv[1], &endptr, 0);
        if (*endptr != '\0') {
            printf("Error: Invalid base address format: %s\n", argv[1]);
            print_usage(argv[0]);
            return 1;
        }
    }

    printf("=================================================\n");
    printf("HSM Driver Comprehensive Test Suite\n");
    printf("=================================================\n");
    printf("Base Address: 0x%08X\n", base_address);
    printf("Test Coverage: HAL + Driver + Integration\n");
    printf("=================================================\n\n");

    // Initialize test environment
    if (test_init(base_address) != TEST_PASS) {
        printf("Test environment initialization failed!\n");
        return 1;
    }

    printf("\n");

    // Run all tests
    const int num_tests = sizeof(test_suite) / sizeof(test_suite[0]);

    for (int i = 0; i < num_tests; i++) {
        run_test(test_suite[i].test_name, test_suite[i].test_func);
    }

    // Print results summary
    printf("\n=================================================\n");
    printf("Test Results Summary\n");
    printf("=================================================\n");
    printf("Total tests: %d\n", g_tests_run);
    printf("Passed:      %d\n", g_tests_passed);
    printf("Failed:      %d\n", g_tests_failed);
    printf("Success rate: %.1f%%\n",
           g_tests_run > 0 ? (100.0 * g_tests_passed / g_tests_run) : 0.0);
    printf("=================================================\n");

    // Cleanup
    interface_layer_deinit();

    if (g_tests_failed > 0) {
        printf("\nSome tests failed. Please check the output above.\n");
        return 1;
    } else {
        printf("\nAll tests passed successfully!\n");
        return 0;
    }
}