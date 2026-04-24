/**
 * @file dmav2_test.c
 * @brief Comprehensive DMAv2 Driver Test Suite
 *
 * This test program provides comprehensive validation of the DMAv2 driver
 * following the requirements from autogen_rules/device_driver_test_gen.md
 * 
 * Test Categories:
 * 1. Interface Testing (API coverage)
 * 2. Functional Testing (device-specific compliance)
 * 3. Boundary Testing (edge cases and error conditions)
 */

#include "dmav2_driver.h"
#include "interface_layer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>

/**
 * @brief Test configuration constants
 */
#define TEST_DEVICE_ID              1
#define TEST_ADDRESS_SPACE_SIZE     0x1000
#define TEST_TIMEOUT_MS             5000
#define TEST_BUFFER_SIZE            1024

/**
 * @brief Test result counters
 */
static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

/**
 * @brief Test buffer arrays
 */
static uint32_t g_test_src_buffer[TEST_BUFFER_SIZE];
static uint32_t g_test_dst_buffer[TEST_BUFFER_SIZE];
static uint32_t g_test_reference_buffer[TEST_BUFFER_SIZE];

/**
 * @brief Test macros
 */
#define TEST_ASSERT(condition, message) \
    do { \
        g_tests_run++; \
        if (condition) { \
            g_tests_passed++; \
            printf("  ✓ %s\n", message); \
        } else { \
            g_tests_failed++; \
            printf("  ✗ %s\n", message); \
        } \
    } while(0)

#define TEST_CATEGORY(name) \
    printf("\n=== %s ===\n", name)

#define TEST_FUNCTION(name) \
    printf("\n--- %s ---\n", name)

/**
 * @brief Test initialization function (MANDATORY)
 */
int test_init(uint32_t base_address)
{
    printf("Initializing DMAv2 test environment...\n");
    
    // 1. REQUIRED: Initialize interface layer
    if (interface_layer_init() != 0) {
        printf("ERROR: interface_layer_init() failed\n");
        return -1;
    }
    
    // 2. REQUIRED: Register device address space
    if (register_device(TEST_DEVICE_ID, base_address, TEST_ADDRESS_SPACE_SIZE) != 0) {
        printf("ERROR: register_device() failed\n");
        return -1;
    }
    
    // 3. REQUIRED: Initialize HAL base address
    dmav2_hal_base_address_init(base_address);
    
    // 4. REQUIRED: Initialize driver
    if (dmav2_init() != DMAV2_SUCCESS) {
        printf("ERROR: dmav2_init() failed\n");
        return -1;
    }
    
    // Initialize test buffers
    for (int i = 0; i < TEST_BUFFER_SIZE; i++) {
        g_test_src_buffer[i] = 0xDEADBEEF + i;
        g_test_dst_buffer[i] = 0x00000000;
        g_test_reference_buffer[i] = 0xDEADBEEF + i;
    }
    
    printf("Test environment initialized successfully\n");
    return 0;
}

/**
 * @brief Test cleanup function
 */
void test_cleanup(void)
{
    printf("\nCleaning up test environment...\n");
    
    // Deinitialize driver
    dmav2_deinit();
    
    // Unregister device
    unregister_device(TEST_DEVICE_ID);
    
    // Deinitialize interface layer
    interface_layer_deinit();
    
    printf("Test environment cleaned up\n");
}

/**
 * @brief Print test results summary
 */
void print_test_summary(void)
{
    printf("\n============================================================\n");
    printf("TEST SUMMARY\n");
    printf("============================================================\n");
    printf("Total Tests: %d\n", g_tests_run);
    printf("Passed:      %d\n", g_tests_passed);
    printf("Failed:      %d\n", g_tests_failed);
    printf("Success Rate: %.1f%%\n", g_tests_run ? (100.0 * g_tests_passed / g_tests_run) : 0.0);
    printf("============================================================\n");
}

// ============================================================================
// TEST CATEGORY 1: INTERFACE TESTING (MANDATORY)
// ============================================================================

/**
 * @brief Test driver initialization functions
 */
void test_initialization_interface(void)
{
    TEST_FUNCTION("Driver Initialization Interface");
    
    dmav2_status_t status;
    
    // Test reinitialization
    status = dmav2_init();
    TEST_ASSERT(status == DMAV2_SUCCESS, "dmav2_init() returns success");
    
    // Test deinitialization
    status = dmav2_deinit();
    TEST_ASSERT(status == DMAV2_SUCCESS, "dmav2_deinit() returns success");
    
    // Test re-initialization after deinit
    status = dmav2_init();
    TEST_ASSERT(status == DMAV2_SUCCESS, "dmav2_init() after deinit returns success");
}

/**
 * @brief Test channel management interface functions
 */
void test_channel_management_interface(void)
{
    TEST_FUNCTION("Channel Management Interface");
    
    dmav2_status_t status;
    dmav2_channel_config_t config;
    
    // Test channel initialization
    status = dmav2_channel_init(0);
    TEST_ASSERT(status == DMAV2_SUCCESS, "dmav2_channel_init() returns success");
    
    // Test channel configuration
    memset(&config, 0, sizeof(config));
    config.mode = DMAV2_MODE_MEM2MEM;
    config.src_width = DMAV2_DATA_WIDTH_32BIT;
    config.dst_width = DMAV2_DATA_WIDTH_32BIT;
    config.priority = DMAV2_PRIORITY_MEDIUM;
    
    status = dmav2_channel_configure(0, &config);
    TEST_ASSERT(status == DMAV2_SUCCESS, "dmav2_channel_configure() returns success");
    
    // Test configuration retrieval
    dmav2_channel_config_t retrieved_config;
    status = dmav2_channel_get_config(0, &retrieved_config);
    TEST_ASSERT(status == DMAV2_SUCCESS, "dmav2_channel_get_config() returns success");
    TEST_ASSERT(retrieved_config.mode == config.mode, "Configuration mode matches");
    
    // Test channel deinitialization
    status = dmav2_channel_deinit(0);
    TEST_ASSERT(status == DMAV2_SUCCESS, "dmav2_channel_deinit() returns success");
}

/**
 * @brief Test transfer control interface functions
 */
void test_transfer_control_interface(void)
{
    TEST_FUNCTION("Transfer Control Interface");
    
    dmav2_status_t status;
    dmav2_transfer_config_t transfer_config;
    dmav2_channel_status_t channel_status;
    
    // Initialize channel first
    dmav2_channel_init(1);
    
    // Setup transfer configuration
    memset(&transfer_config, 0, sizeof(transfer_config));
    transfer_config.src_addr = (uintptr_t)g_test_src_buffer;
    transfer_config.dst_addr = (uintptr_t)g_test_dst_buffer;
    transfer_config.transfer_count = 10;
    transfer_config.src_offset = 4;
    transfer_config.dst_offset = 4;
    
    // Test transfer start
    status = dmav2_start_transfer(1, &transfer_config);
    TEST_ASSERT(status == DMAV2_SUCCESS, "dmav2_start_transfer() returns success");
    
    // Test status retrieval
    status = dmav2_get_channel_status(1, &channel_status);
    TEST_ASSERT(status == DMAV2_SUCCESS, "dmav2_get_channel_status() returns success");
    
    // Test transfer stop
    status = dmav2_stop_transfer(1);
    TEST_ASSERT(status == DMAV2_SUCCESS, "dmav2_stop_transfer() returns success");
}

/**
 * @brief Test utility interface functions
 */
void test_utility_interface(void)
{
    TEST_FUNCTION("Utility Interface");
    
    dmav2_status_t status;
    dmav2_global_status_t global_status;
    const char *version_str;
    const char *error_str;
    
    // Test global status
    status = dmav2_get_global_status(&global_status);
    TEST_ASSERT(status == DMAV2_SUCCESS, "dmav2_get_global_status() returns success");
    
    // Test version string
    version_str = dmav2_get_version_string();
    TEST_ASSERT(version_str != NULL, "dmav2_get_version_string() returns non-null");
    TEST_ASSERT(strlen(version_str) > 0, "Version string is not empty");
    
    // Test error string
    error_str = dmav2_get_error_string(DMAV2_SUCCESS);
    TEST_ASSERT(error_str != NULL, "dmav2_get_error_string() returns non-null");
    
    // Test channel validation
    status = dmav2_validate_channel(0);
    TEST_ASSERT(status == DMAV2_SUCCESS, "dmav2_validate_channel(0) returns success");
    
    status = dmav2_validate_channel(16);
    TEST_ASSERT(status == DMAV2_ERROR_CHANNEL_INVALID, "dmav2_validate_channel(16) returns invalid");
}

// ============================================================================
// TEST CATEGORY 2: FUNCTIONAL TESTING (DEVICE-SPECIFIC)
// ============================================================================

/**
 * @brief Test Memory-to-Memory transfer mode
 */
void test_mem2mem_transfer_mode(void)
{
    TEST_FUNCTION("Memory-to-Memory Transfer Mode");
    
    dmav2_status_t status;
    
    // Clear destination buffer
    memset(g_test_dst_buffer, 0, sizeof(g_test_dst_buffer));
    
    // Perform mem2mem transfer
    status = dmav2_mem_to_mem_transfer(0, 
                                      (uintptr_t)g_test_src_buffer,
                                      (uintptr_t)g_test_dst_buffer,
                                      64,  // Transfer 64 words
                                      DMAV2_DATA_WIDTH_32BIT);
    
    TEST_ASSERT(status == DMAV2_SUCCESS, "Memory-to-memory transfer initiated successfully");
    
    // Test the transfer function interface
    status = dmav2_test_mem2mem_mode(2);
    TEST_ASSERT(status == DMAV2_SUCCESS, "dmav2_test_mem2mem_mode() returns success");
}

/**
 * @brief Test Memory-to-Peripheral transfer mode
 */
void test_mem2peri_transfer_mode(void)
{
    TEST_FUNCTION("Memory-to-Peripheral Transfer Mode");
    
    dmav2_status_t status;
    
    // Test mem2peri mode
    status = dmav2_test_mem2peri_mode(3);
    TEST_ASSERT(status == DMAV2_SUCCESS, "Memory-to-peripheral transfer configured successfully");
    
    // Stop the transfer for cleanup
    dmav2_stop_transfer(3);
}

/**
 * @brief Test Peripheral-to-Memory transfer mode
 */
void test_peri2mem_transfer_mode(void)
{
    TEST_FUNCTION("Peripheral-to-Memory Transfer Mode");
    
    dmav2_status_t status;
    
    // Test peri2mem mode
    status = dmav2_test_peri2mem_mode(4);
    TEST_ASSERT(status == DMAV2_SUCCESS, "Peripheral-to-memory transfer configured successfully");
    
    // Stop the transfer for cleanup
    dmav2_stop_transfer(4);
}

/**
 * @brief Test Address Fixed Mode
 */
void test_address_fixed_mode(void)
{
    TEST_FUNCTION("Address Fixed Mode");
    
    dmav2_status_t status;
    
    // Test address fixed mode
    status = dmav2_test_address_fixed_mode(5);
    TEST_ASSERT(status == DMAV2_SUCCESS, "Address fixed mode configured successfully");
    
    // Stop the transfer for cleanup
    dmav2_stop_transfer(5);
}

/**
 * @brief Test Address Increment Mode
 */
void test_address_increment_mode(void)
{
    TEST_FUNCTION("Address Increment Mode");
    
    dmav2_status_t status;
    
    // Test address increment mode
    status = dmav2_test_address_increment_mode(6);
    TEST_ASSERT(status == DMAV2_SUCCESS, "Address increment mode configured successfully");
    
    // Stop the transfer for cleanup
    dmav2_stop_transfer(6);
}

/**
 * @brief Test Circular Mode
 */
void test_circular_mode(void)
{
    TEST_FUNCTION("Circular Buffer Mode");
    
    dmav2_status_t status;
    
    // Test circular mode
    status = dmav2_test_circular_mode(7);
    TEST_ASSERT(status == DMAV2_SUCCESS, "Circular mode configured successfully");
    
    // Stop the transfer for cleanup
    dmav2_stop_transfer(7);
}

/**
 * @brief Test all DMA modes (MANDATORY per requirements)
 */
void test_dma_all_modes(void)
{
    TEST_FUNCTION("All DMA Modes Coverage Test");
    
    // Memory to Memory
    test_mem2mem_transfer_mode();
    
    // Memory to Peripheral
    test_mem2peri_transfer_mode();
    
    // Peripheral to Memory
    test_peri2mem_transfer_mode();
    
    // Address fixed mode
    test_address_fixed_mode();
    
    // Address increment mode
    test_address_increment_mode();
    
    // Circular mode
    test_circular_mode();
    
    printf("All DMA modes tested successfully\n");
}

/**
 * @brief Test priority levels
 */
void test_priority_levels(void)
{
    TEST_FUNCTION("Priority Levels");
    
    dmav2_status_t status;
    
    // Test all priority levels
    status = dmav2_set_channel_priority(8, DMAV2_PRIORITY_LOW);
    TEST_ASSERT(status == DMAV2_SUCCESS, "Set LOW priority successful");
    
    status = dmav2_set_channel_priority(9, DMAV2_PRIORITY_MEDIUM);
    TEST_ASSERT(status == DMAV2_SUCCESS, "Set MEDIUM priority successful");
    
    status = dmav2_set_channel_priority(10, DMAV2_PRIORITY_HIGH);
    TEST_ASSERT(status == DMAV2_SUCCESS, "Set HIGH priority successful");
    
    status = dmav2_set_channel_priority(11, DMAV2_PRIORITY_VERY_HIGH);
    TEST_ASSERT(status == DMAV2_SUCCESS, "Set VERY_HIGH priority successful");
}

/**
 * @brief Test DMAMUX configuration
 */
void test_dmamux_configuration(void)
{
    TEST_FUNCTION("DMAMUX Configuration");
    
    dmav2_status_t status;
    dmav2_channel_config_t config;
    
    // Initialize channel
    dmav2_channel_init(12);
    
    // Test DMAMUX configuration with different request IDs
    memset(&config, 0, sizeof(config));
    config.mode = DMAV2_MODE_MEM2PERI;
    config.dmamux_req_id = 1;  // UART RX
    config.dmamux_trigger_enable = false;
    
    status = dmav2_channel_configure(12, &config);
    TEST_ASSERT(status == DMAV2_SUCCESS, "DMAMUX REQ_ID=1 configuration successful");
    
    // Test with trigger enable
    config.dmamux_req_id = 2;  // UART TX
    config.dmamux_trigger_enable = true;
    
    status = dmav2_channel_configure(12, &config);
    TEST_ASSERT(status == DMAV2_SUCCESS, "DMAMUX with trigger enable successful");
}

// ============================================================================
// TEST CATEGORY 3: BOUNDARY TESTING (MANDATORY)
// ============================================================================

/**
 * @brief Test zero-length data transfer
 */
void test_zero_length_data_transfer(void)
{
    TEST_FUNCTION("Zero-Length Data Transfer");
    
    dmav2_status_t status;
    dmav2_transfer_config_t config;
    
    // Initialize channel
    dmav2_channel_init(13);
    
    // Setup zero-length transfer
    memset(&config, 0, sizeof(config));
    config.src_addr = (uintptr_t)g_test_src_buffer;
    config.dst_addr = (uintptr_t)g_test_dst_buffer;
    config.transfer_count = 0;  // Zero length
    
    status = dmav2_start_transfer(13, &config);
    TEST_ASSERT(status == DMAV2_ERROR_CONFIG_INVALID, "Zero-length transfer properly rejected");
}

/**
 * @brief Test maximum parameter values
 */
void test_maximum_transfer_size(void)
{
    TEST_FUNCTION("Maximum Transfer Size");
    
    dmav2_status_t status;
    dmav2_transfer_config_t config;
    
    // Initialize channel
    dmav2_channel_init(14);
    
    // Setup maximum transfer count
    memset(&config, 0, sizeof(config));
    config.src_addr = (uintptr_t)g_test_src_buffer;
    config.dst_addr = (uintptr_t)g_test_dst_buffer;
    config.transfer_count = 65535;  // Maximum 16-bit value
    config.src_offset = 4;
    config.dst_offset = 4;
    
    status = dmav2_start_transfer(14, &config);
    TEST_ASSERT(status == DMAV2_SUCCESS, "Maximum transfer count accepted");
    
    // Stop transfer for cleanup
    dmav2_stop_transfer(14);
}

/**
 * @brief Test invalid addresses
 */
void test_invalid_address_access(void)
{
    TEST_FUNCTION("Invalid Address Access");
    
    dmav2_status_t status;
    dmav2_transfer_config_t config;
    
    // Initialize channel
    dmav2_channel_init(15);
    
    // Test null source address
    memset(&config, 0, sizeof(config));
    config.src_addr = 0;  // Invalid address
    config.dst_addr = (uintptr_t)g_test_dst_buffer;
    config.transfer_count = 10;
    
    status = dmav2_start_transfer(15, &config);
    TEST_ASSERT(status == DMAV2_ERROR_CONFIG_INVALID, "Null source address properly rejected");
    
    // Test null destination address
    config.src_addr = (uintptr_t)g_test_src_buffer;
    config.dst_addr = 0;  // Invalid address
    
    status = dmav2_start_transfer(15, &config);
    TEST_ASSERT(status == DMAV2_ERROR_CONFIG_INVALID, "Null destination address properly rejected");
}

/**
 * @brief Test parameter boundary values
 */
void test_parameter_boundary_values(void)
{
    TEST_FUNCTION("Parameter Boundary Values");
    
    dmav2_status_t status;
    dmav2_channel_config_t config;
    
    // Test invalid channel numbers
    status = dmav2_validate_channel(255);
    TEST_ASSERT(status == DMAV2_ERROR_CHANNEL_INVALID, "Channel 255 properly rejected");
    
    status = dmav2_validate_channel(16);
    TEST_ASSERT(status == DMAV2_ERROR_CHANNEL_INVALID, "Channel 16 properly rejected");
    
    status = dmav2_validate_channel(15);
    TEST_ASSERT(status == DMAV2_SUCCESS, "Channel 15 accepted");
    
    // Test invalid priority values
    status = dmav2_set_channel_priority(0, (dmav2_priority_t)99);
    TEST_ASSERT(status == DMAV2_ERROR_INVALID_PARAM, "Invalid priority value rejected");
    
    // Test invalid configuration parameters
    memset(&config, 0, sizeof(config));
    config.mode = (dmav2_transfer_mode_t)99;  // Invalid mode
    
    status = dmav2_validate_config(&config);
    TEST_ASSERT(status == DMAV2_ERROR_CONFIG_INVALID, "Invalid transfer mode rejected");
    
    // Test invalid data width
    config.mode = DMAV2_MODE_MEM2MEM;
    config.src_width = (dmav2_data_width_t)99;  // Invalid width
    
    status = dmav2_validate_config(&config);
    TEST_ASSERT(status == DMAV2_ERROR_CONFIG_INVALID, "Invalid data width rejected");
}

/**
 * @brief Test resource limit handling
 */
void test_resource_limit_handling(void)
{
    TEST_FUNCTION("Resource Limit Handling");
    
    dmav2_status_t status;
    dmav2_transfer_config_t config;
    
    // Initialize multiple channels to test resource limits
    for (int i = 0; i < DMAV2_MAX_CHANNELS; i++) {
        status = dmav2_channel_init(i);
        TEST_ASSERT(status == DMAV2_SUCCESS, "Channel initialization within limits");
    }
    
    // Test starting transfers on all channels
    memset(&config, 0, sizeof(config));
    config.src_addr = (uintptr_t)g_test_src_buffer;
    config.dst_addr = (uintptr_t)g_test_dst_buffer;
    config.transfer_count = 10;
    config.src_offset = 4;
    config.dst_offset = 4;
    
    int successful_starts = 0;
    for (int i = 0; i < DMAV2_MAX_CHANNELS; i++) {
        status = dmav2_start_transfer(i, &config);
        if (status == DMAV2_SUCCESS) {
            successful_starts++;
        }
    }
    
    TEST_ASSERT(successful_starts > 0, "At least one transfer started successfully");
    
    // Cleanup - stop all transfers
    for (int i = 0; i < DMAV2_MAX_CHANNELS; i++) {
        dmav2_stop_transfer(i);
    }
}

/**
 * @brief Test null pointer parameters
 */
void test_null_pointer_parameters(void)
{
    TEST_FUNCTION("Null Pointer Parameters");
    
    dmav2_status_t status;
    
    // Test null configuration pointer
    status = dmav2_channel_configure(0, NULL);
    TEST_ASSERT(status == DMAV2_ERROR_INVALID_PARAM, "Null config pointer rejected");
    
    // Test null transfer config pointer
    status = dmav2_start_transfer(0, NULL);
    TEST_ASSERT(status == DMAV2_ERROR_INVALID_PARAM, "Null transfer config pointer rejected");
    
    // Test null status pointer
    status = dmav2_get_channel_status(0, NULL);
    TEST_ASSERT(status == DMAV2_ERROR_INVALID_PARAM, "Null status pointer rejected");
    
    // Test null global status pointer
    status = dmav2_get_global_status(NULL);
    TEST_ASSERT(status == DMAV2_ERROR_INVALID_PARAM, "Null global status pointer rejected");
    
    // Test null configuration retrieval pointer
    status = dmav2_channel_get_config(0, NULL);
    TEST_ASSERT(status == DMAV2_ERROR_INVALID_PARAM, "Null config retrieval pointer rejected");
}

/**
 * @brief Comprehensive boundary condition test
 */
void test_boundary_conditions(void)
{
    TEST_FUNCTION("Comprehensive Boundary Conditions");
    
    // Test zero-length operations
    test_zero_length_data_transfer();
    
    // Test maximum parameter values
    test_maximum_transfer_size();
    
    // Test invalid addresses
    test_invalid_address_access();
    
    // Test parameter limits
    test_parameter_boundary_values();
    
    // Test resource exhaustion
    test_resource_limit_handling();
    
    // Test null pointers
    test_null_pointer_parameters();
    
    printf("All boundary conditions tested\n");
}

// ============================================================================
// TEST EXECUTION FRAMEWORK
// ============================================================================

/**
 * @brief Run all interface tests
 */
void run_interface_tests(void)
{
    TEST_CATEGORY("INTERFACE TESTING");
    
    test_initialization_interface();
    test_channel_management_interface();
    test_transfer_control_interface();
    test_utility_interface();
}

/**
 * @brief Run all functional tests
 */
void run_functional_tests(void)
{
    TEST_CATEGORY("FUNCTIONAL TESTING");
    
    test_dma_all_modes();
    test_priority_levels();
    test_dmamux_configuration();
}

/**
 * @brief Run all boundary tests
 */
void run_boundary_tests(void)
{
    TEST_CATEGORY("BOUNDARY TESTING");
    
    test_boundary_conditions();
}

/**
 * @brief Run self-test
 */
void run_self_test(void)
{
    TEST_CATEGORY("SELF-TEST");
    
    dmav2_status_t status = dmav2_self_test();
    TEST_ASSERT(status == DMAV2_SUCCESS, "Driver self-test passed");
}

/**
 * @brief Run complete test suite
 */
int run_all_tests(void)
{
    printf("\n============================================================\n");
    printf("DMAv2 DRIVER COMPREHENSIVE TEST SUITE\n");
    printf("============================================================\n");
    printf("Driver Version: %s\n", dmav2_get_version_string());
    printf("============================================================\n");
    
    // Reset counters
    g_tests_run = 0;
    g_tests_passed = 0;
    g_tests_failed = 0;
    
    // Run test categories
    run_interface_tests();
    run_functional_tests();
    run_boundary_tests();
    run_self_test();
    
    // Print summary
    print_test_summary();
    
    // Return 0 for success, -1 for failure
    return (g_tests_failed == 0) ? 0 : -1;
}

/**
 * @brief Main test function (MANDATORY template)
 */
int main(int argc, char *argv[])
{
    // MANDATORY: Support address parameter
    if (argc != 2) {
        printf("Usage: %s <base_address>\n", argv[0]);
        printf("Example: %s 0x40000000\n", argv[0]);
        return -1;
    }
    
    uint32_t base_address = strtoul(argv[1], NULL, 0);
    printf("DMAv2 Driver Test Suite\n");
    printf("Base Address: 0x%08X\n", base_address);
    
    // MANDATORY: Initialize test environment
    if (test_init(base_address) != 0) {
        printf("Test initialization failed\n");
        return -1;
    }
    
    // Execute test suite
    int result = run_all_tests();
    
    // MANDATORY: Cleanup
    test_cleanup();
    
    if (result == 0) {
        printf("\n🎉 ALL TESTS PASSED! 🎉\n");
    } else {
        printf("\n❌ SOME TESTS FAILED ❌\n");
    }
    
    return result;
}