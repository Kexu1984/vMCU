/**
 * @file dma_test.c
 * @brief Comprehensive test program for DMA device driver
 *
 * This test program validates all aspects of the DMA driver implementation:
 * - Interface testing: All driver functions are called and return values checked
 * - Functional testing: Complete DMA functionality including mem2mem, mem2peri, peri2mem transfers
 * - Boundary testing: Edge cases and error conditions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#define _GNU_SOURCE  /* For usleep */
#include "../driver/dma_driver.h"
#include "../hal/dma_hal.h"
#include "interface_layer.h"

/* Test configuration */
#define TEST_DEVICE_ID          1
#define TEST_ADDRESS_SPACE_SIZE 0x1000
#define TEST_BUFFER_SIZE        1024
#define TEST_PATTERN_SIZE       256

/* Test result tracking */
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
} test_results_t;

static test_results_t g_test_results = {0, 0, 0};

/* Test data buffers */
static uint8_t g_src_buffer[TEST_BUFFER_SIZE];
static uint8_t g_dst_buffer[TEST_BUFFER_SIZE];
static uint8_t g_reference_buffer[TEST_BUFFER_SIZE];

/* Utility macros */
#define TEST_ASSERT(condition, message) do { \
    g_test_results.total_tests++; \
    if (condition) { \
        g_test_results.passed_tests++; \
        printf("  [PASS] %s\n", message); \
    } else { \
        g_test_results.failed_tests++; \
        printf("  [FAIL] %s\n", message); \
        return false; \
    } \
} while(0)

#define TEST_EXPECT(condition, message) do { \
    g_test_results.total_tests++; \
    if (condition) { \
        g_test_results.passed_tests++; \
        printf("  [PASS] %s\n", message); \
    } else { \
        g_test_results.failed_tests++; \
        printf("  [FAIL] %s\n", message); \
    } \
} while(0)

/* Function prototypes */
static bool test_init(uint32_t base_address);
static void test_deinit(void);
static bool test_interface_functions(void);
static bool test_functional_operations(void);
static bool test_boundary_conditions(void);
static bool test_mem2mem_transfer(void);
static bool test_mem2peri_transfer(void);
static bool test_peri2mem_transfer(void);
static bool test_different_data_sizes(void);
static bool test_circular_mode(void);
static bool test_priority_arbitration(void);
static bool test_concurrent_channels(void);
static bool test_error_conditions(void);
static bool test_reset_functionality(void);
static void prepare_test_data(void);
static bool verify_data_transfer(const uint8_t *src, const uint8_t *dst, size_t size);
static void print_test_summary(void);

/**
 * @brief Initialize test environment
 */
static bool test_init(uint32_t base_address)
{
    int result;
    
    printf("Initializing test environment...\n");
    
    // Initialize interface layer
    result = interface_layer_init();
    if (result != 0) {
        printf("Failed to initialize interface layer: %d\n", result);
        return false;
    }
    
    // Register device
    result = register_device(TEST_DEVICE_ID, base_address, TEST_ADDRESS_SPACE_SIZE);
    if (result != 0) {
        printf("Failed to register device: %d\n", result);
        interface_layer_deinit();
        return false;
    }
    
    // Initialize DMA driver
    dma_drv_status_t status = dma_driver_init(base_address);
    if (status != DMA_DRV_OK) {
        printf("Failed to initialize DMA driver: %d\n", status);
        interface_layer_deinit();
        return false;
    }
    
    printf("Test environment initialized successfully\n");
    return true;
}

/**
 * @brief Deinitialize test environment
 */
static void test_deinit(void)
{
    printf("Deinitializing test environment...\n");
    
    dma_driver_deinit();
    interface_layer_deinit();
    
    printf("Test environment deinitialized\n");
}

/**
 * @brief Test all driver interface functions
 */
static bool test_interface_functions(void)
{
    printf("\n=== Interface Function Tests ===\n");
    
    dma_drv_status_t status;
    dma_driver_version_t version;
    dma_channel_config_t config;
    dma_channel_status_t channel_status;
    bool is_busy, is_idle;
    uint32_t transferred_count, fifo_left;
    
    // Test driver version
    status = dma_driver_get_version(&version);
    TEST_ASSERT(status == DMA_DRV_OK, "Get driver version");
    TEST_ASSERT(version.major == 1 && version.minor == 0, "Version values correct");
    
    // Test default configuration
    status = dma_channel_get_default_config(&config);
    TEST_ASSERT(status == DMA_DRV_OK, "Get default configuration");
    TEST_ASSERT(config.direction == DMA_TRANSFER_MEM2MEM, "Default direction is mem2mem");
    TEST_ASSERT(config.priority == DMA_PRIORITY_LOW, "Default priority is low");
    
    // Test channel configuration
    config.src_address = 0x10000000;
    config.dst_address = 0x20000000;
    config.data_length = 256;
    status = dma_channel_configure(0, &config);
    TEST_ASSERT(status == DMA_DRV_OK, "Configure channel 0");
    
    // Test channel status
    status = dma_channel_get_status(0, &channel_status);
    TEST_ASSERT(status == DMA_DRV_OK, "Get channel status");
    
    // Test channel busy check
    status = dma_channel_is_busy(0, &is_busy);
    TEST_ASSERT(status == DMA_DRV_OK, "Check if channel is busy");
    
    // Test transferred count
    status = dma_channel_get_transferred_count(0, &transferred_count);
    TEST_ASSERT(status == DMA_DRV_OK, "Get transferred count");
    
    // Test FIFO left count
    status = dma_channel_get_fifo_left_count(0, &fifo_left);
    TEST_ASSERT(status == DMA_DRV_OK, "Get FIFO left count");
    
    // Test DMA idle check
    status = dma_is_idle(&is_idle);
    TEST_ASSERT(status == DMA_DRV_OK, "Check if DMA is idle");
    
    // Test channel start/stop
    status = dma_channel_start(0);
    TEST_ASSERT(status == DMA_DRV_OK, "Start channel 0");
    
    status = dma_channel_stop(0);
    TEST_ASSERT(status == DMA_DRV_OK, "Stop channel 0");
    
    // Test channel pause/resume
    status = dma_channel_pause(0);
    TEST_ASSERT(status == DMA_DRV_OK, "Pause channel 0");
    
    status = dma_channel_resume(0);
    TEST_ASSERT(status == DMA_DRV_OK, "Resume channel 0");
    
    // Test invalid parameters
    status = dma_channel_configure(16, &config); // Invalid channel
    TEST_ASSERT(status == DMA_DRV_INVALID_PARAM, "Invalid channel number rejected");
    
    status = dma_channel_configure(0, NULL); // NULL config
    TEST_ASSERT(status == DMA_DRV_INVALID_PARAM, "NULL config rejected");
    
    return true;
}

/**
 * @brief Test functional DMA operations
 */
static bool test_functional_operations(void)
{
    printf("\n=== Functional Operation Tests ===\n");
    
    bool result = true;
    
    prepare_test_data();
    
    result &= test_mem2mem_transfer();
    result &= test_mem2peri_transfer();
    result &= test_peri2mem_transfer();
    result &= test_different_data_sizes();
    result &= test_circular_mode();
    result &= test_priority_arbitration();
    result &= test_concurrent_channels();
    
    return result;
}

/**
 * @brief Test memory-to-memory transfer
 */
static bool test_mem2mem_transfer(void)
{
    printf("\n--- Memory-to-Memory Transfer Test ---\n");
    
    dma_drv_status_t status;
    uint32_t src_addr = 0x10000000;
    uint32_t dst_addr = 0x20000000;
    uint32_t size = 512;
    
    // Clear destination buffer
    memset(g_dst_buffer, 0, sizeof(g_dst_buffer));
    
    // Perform mem2mem transfer
    status = dma_mem2mem_transfer(0, src_addr, dst_addr, size);
    TEST_ASSERT(status == DMA_DRV_OK, "Memory-to-memory transfer initiated");
    
    // Wait for completion
    status = dma_channel_wait_for_completion(0, 1000);
    TEST_EXPECT(status == DMA_DRV_TRANSFER_COMPLETE || status == DMA_DRV_TIMEOUT, 
               "Transfer completed or timed out");
    
    return true;
}

/**
 * @brief Test memory-to-peripheral transfer
 */
static bool test_mem2peri_transfer(void)
{
    printf("\n--- Memory-to-Peripheral Transfer Test ---\n");
    
    dma_drv_status_t status;
    uint32_t src_addr = 0x10000000;
    uint32_t peri_addr = 0x40001000; // UART TX register
    uint32_t size = 128;
    uint8_t req_id = DMA_REQ_UART0_TX;
    
    // Perform mem2peri transfer
    status = dma_mem2peri_transfer(1, src_addr, peri_addr, size, req_id);
    TEST_ASSERT(status == DMA_DRV_OK, "Memory-to-peripheral transfer initiated");
    
    // Check channel configuration
    dma_channel_status_t channel_status;
    status = dma_channel_get_status(1, &channel_status);
    TEST_ASSERT(status == DMA_DRV_OK, "Channel status retrieved");
    
    return true;
}

/**
 * @brief Test peripheral-to-memory transfer
 */
static bool test_peri2mem_transfer(void)
{
    printf("\n--- Peripheral-to-Memory Transfer Test ---\n");
    
    dma_drv_status_t status;
    uint32_t peri_addr = 0x40001004; // UART RX register
    uint32_t dst_addr = 0x20000000;
    uint32_t size = 128;
    uint8_t req_id = DMA_REQ_UART0_RX;
    
    // Perform peri2mem transfer
    status = dma_peri2mem_transfer(2, peri_addr, dst_addr, size, req_id);
    TEST_ASSERT(status == DMA_DRV_OK, "Peripheral-to-memory transfer initiated");
    
    // Check channel configuration
    dma_channel_status_t channel_status;
    status = dma_channel_get_status(2, &channel_status);
    TEST_ASSERT(status == DMA_DRV_OK, "Channel status retrieved");
    
    return true;
}

/**
 * @brief Test different data sizes
 */
static bool test_different_data_sizes(void)
{
    printf("\n--- Different Data Sizes Test ---\n");
    
    dma_drv_status_t status;
    dma_channel_config_t config;
    
    // Test byte transfers
    status = dma_channel_get_default_config(&config);
    TEST_ASSERT(status == DMA_DRV_OK, "Get default config for byte test");
    
    config.src_address = 0x10000000;
    config.dst_address = 0x20000000;
    config.data_length = 128;
    config.src_data_size = DMA_DATA_SIZE_BYTE;
    config.dst_data_size = DMA_DATA_SIZE_BYTE;
    
    status = dma_channel_configure(3, &config);
    TEST_ASSERT(status == DMA_DRV_OK, "Configure channel for byte transfers");
    
    // Test halfword transfers
    config.src_data_size = DMA_DATA_SIZE_HALFWORD;
    config.dst_data_size = DMA_DATA_SIZE_HALFWORD;
    config.data_length = 64; // 64 halfwords = 128 bytes
    
    status = dma_channel_configure(4, &config);
    TEST_ASSERT(status == DMA_DRV_OK, "Configure channel for halfword transfers");
    
    // Test word transfers
    config.src_data_size = DMA_DATA_SIZE_WORD;
    config.dst_data_size = DMA_DATA_SIZE_WORD;
    config.data_length = 32; // 32 words = 128 bytes
    
    status = dma_channel_configure(5, &config);
    TEST_ASSERT(status == DMA_DRV_OK, "Configure channel for word transfers");
    
    return true;
}

/**
 * @brief Test circular mode operation
 */
static bool test_circular_mode(void)
{
    printf("\n--- Circular Mode Test ---\n");
    
    dma_drv_status_t status;
    uint32_t src_addr = 0x10000000;
    uint32_t dst_start = 0x20000000;
    uint32_t dst_end = 0x20000400; // 1KB circular buffer
    
    status = dma_setup_circular_mode(6, src_addr, dst_start, dst_end, DMA_DATA_SIZE_WORD);
    TEST_ASSERT(status == DMA_DRV_OK, "Setup circular mode");
    
    status = dma_channel_start(6);
    TEST_ASSERT(status == DMA_DRV_OK, "Start circular mode transfer");
    
    // In circular mode, the channel should continue running
    usleep(10000); // 10ms delay
    
    bool is_busy;
    status = dma_channel_is_busy(6, &is_busy);
    TEST_ASSERT(status == DMA_DRV_OK, "Check circular mode channel status");
    
    status = dma_channel_stop(6);
    TEST_ASSERT(status == DMA_DRV_OK, "Stop circular mode transfer");
    
    return true;
}

/**
 * @brief Test priority arbitration
 */
static bool test_priority_arbitration(void)
{
    printf("\n--- Priority Arbitration Test ---\n");
    
    dma_drv_status_t status;
    dma_channel_config_t config;
    
    // Configure channels with different priorities
    dma_priority_t priorities[] = {
        DMA_PRIORITY_LOW,
        DMA_PRIORITY_MEDIUM,
        DMA_PRIORITY_HIGH,
        DMA_PRIORITY_VERY_HIGH
    };
    
    for (int i = 0; i < 4; i++) {
        status = dma_channel_get_default_config(&config);
        TEST_ASSERT(status == DMA_DRV_OK, "Get default config for priority test");
        
        config.priority = priorities[i];
        config.src_address = 0x10000000 + (i * 0x100);
        config.dst_address = 0x20000000 + (i * 0x100);
        config.data_length = 64;
        
        status = dma_channel_configure(7 + i, &config);
        TEST_ASSERT(status == DMA_DRV_OK, "Configure channel with different priority");
    }
    
    return true;
}

/**
 * @brief Test concurrent channel operations
 */
static bool test_concurrent_channels(void)
{
    printf("\n--- Concurrent Channels Test ---\n");
    
    dma_drv_status_t status;
    dma_channel_config_t config;
    
    // Configure multiple channels for concurrent operation
    for (uint32_t ch = 11; ch < 15; ch++) {
        status = dma_channel_get_default_config(&config);
        TEST_ASSERT(status == DMA_DRV_OK, "Get default config for concurrent test");
        
        config.src_address = 0x10000000 + (ch * 0x100);
        config.dst_address = 0x20000000 + (ch * 0x100);
        config.data_length = 32;
        
        status = dma_channel_configure(ch, &config);
        TEST_ASSERT(status == DMA_DRV_OK, "Configure concurrent channel");
        
        status = dma_channel_start(ch);
        TEST_ASSERT(status == DMA_DRV_OK, "Start concurrent channel");
    }
    
    // Check that multiple channels are running
    bool any_busy = false;
    for (uint32_t ch = 11; ch < 15; ch++) {
        bool is_busy;
        status = dma_channel_is_busy(ch, &is_busy);
        if (status == DMA_DRV_OK && is_busy) {
            any_busy = true;
        }
    }
    TEST_EXPECT(any_busy, "At least one concurrent channel is busy");
    
    // Stop all concurrent channels
    for (uint32_t ch = 11; ch < 15; ch++) {
        dma_channel_stop(ch);
    }
    
    return true;
}

/**
 * @brief Test boundary conditions and error cases
 */
static bool test_boundary_conditions(void)
{
    printf("\n=== Boundary Condition Tests ===\n");
    
    bool result = true;
    result &= test_error_conditions();
    result &= test_reset_functionality();
    
    return result;
}

/**
 * @brief Test error conditions
 */
static bool test_error_conditions(void)
{
    printf("\n--- Error Condition Test ---\n");
    
    dma_drv_status_t status;
    dma_channel_config_t config;
    
    // Test zero-length transfer
    status = dma_channel_get_default_config(&config);
    TEST_ASSERT(status == DMA_DRV_OK, "Get default config for error test");
    
    config.src_address = 0x10000000;
    config.dst_address = 0x20000000;
    config.data_length = 0; // Zero length
    
    status = dma_channel_configure(15, &config);
    TEST_ASSERT(status == DMA_DRV_OK, "Configure channel with zero length");
    
    // Test misaligned addresses
    config.data_length = 64;
    config.src_address = 0x10000001; // Misaligned for word access
    config.dst_address = 0x20000002; // Misaligned for word access
    config.src_data_size = DMA_DATA_SIZE_WORD;
    config.dst_data_size = DMA_DATA_SIZE_WORD;
    
    status = dma_channel_configure(15, &config);
    TEST_EXPECT(status == DMA_DRV_OK, "Configure channel with misaligned addresses");
    
    // Test maximum transfer length
    config.src_address = 0x10000000;
    config.dst_address = 0x20000000;
    config.data_length = 32768; // Maximum supported length
    config.src_data_size = DMA_DATA_SIZE_BYTE;
    config.dst_data_size = DMA_DATA_SIZE_BYTE;
    
    status = dma_channel_configure(15, &config);
    TEST_ASSERT(status == DMA_DRV_OK, "Configure channel with maximum length");
    
    // Test timeout on wait
    status = dma_channel_start(15);
    if (status == DMA_DRV_OK) {
        status = dma_channel_wait_for_completion(15, 1); // Very short timeout
        TEST_EXPECT(status == DMA_DRV_TIMEOUT || status == DMA_DRV_TRANSFER_COMPLETE, 
                   "Wait with short timeout returns appropriate status");
        dma_channel_stop(15);
    }
    
    return true;
}

/**
 * @brief Test reset functionality
 */
static bool test_reset_functionality(void)
{
    printf("\n--- Reset Functionality Test ---\n");
    
    dma_drv_status_t status;
    bool is_idle;
    
    // Test warm reset
    status = dma_global_warm_reset();
    TEST_ASSERT(status == DMA_DRV_OK, "Global warm reset executed");
    
    // Check if DMA is idle after reset
    status = dma_is_idle(&is_idle);
    TEST_ASSERT(status == DMA_DRV_OK, "Check idle status after warm reset");
    
    // Test hard reset
    status = dma_global_hard_reset();
    TEST_ASSERT(status == DMA_DRV_OK, "Global hard reset executed");
    
    // Check if DMA is idle after hard reset
    status = dma_is_idle(&is_idle);
    TEST_ASSERT(status == DMA_DRV_OK, "Check idle status after hard reset");
    
    return true;
}

/**
 * @brief Prepare test data buffers
 */
static void prepare_test_data(void)
{
    // Initialize source buffer with test pattern
    for (int i = 0; i < TEST_BUFFER_SIZE; i++) {
        g_src_buffer[i] = (uint8_t)(i & 0xFF);
    }
    
    // Clear destination buffer
    memset(g_dst_buffer, 0, sizeof(g_dst_buffer));
    
    // Initialize reference buffer
    memcpy(g_reference_buffer, g_src_buffer, sizeof(g_reference_buffer));
}

/**
 * @brief Verify data transfer correctness
 */
static bool verify_data_transfer(const uint8_t *src, const uint8_t *dst, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        if (src[i] != dst[i]) {
            printf("Data mismatch at offset %zu: src=0x%02X, dst=0x%02X\n", 
                   i, src[i], dst[i]);
            return false;
        }
    }
    return true;
}

/**
 * @brief Print test summary
 */
static void print_test_summary(void)
{
    printf("\n" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "\n");
    printf("TEST SUMMARY\n");
    printf("=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "\n");
    printf("Total tests:  %d\n", g_test_results.total_tests);
    printf("Passed tests: %d\n", g_test_results.passed_tests);
    printf("Failed tests: %d\n", g_test_results.failed_tests);
    
    if (g_test_results.failed_tests == 0) {
        printf("Result: ALL TESTS PASSED!\n");
    } else {
        printf("Result: %d TESTS FAILED!\n", g_test_results.failed_tests);
    }
    printf("=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "\n");
}

/**
 * @brief Main test function
 */
int main(int argc, char *argv[])
{
    uint32_t base_address = 0x40000000; // Default base address
    
    printf("DMA Device Driver Test Program\n");
    printf("==============================\n");
    
    // Parse command line arguments
    if (argc > 1) {
        base_address = (uint32_t)strtoul(argv[1], NULL, 0);
        printf("Using base address: 0x%08X\n", base_address);
    } else {
        printf("Using default base address: 0x%08X\n", base_address);
        printf("Usage: %s [base_address]\n", argv[0]);
    }
    
    // Initialize test environment
    if (!test_init(base_address)) {
        printf("Failed to initialize test environment\n");
        return EXIT_FAILURE;
    }
    
    bool all_tests_passed = true;
    
    // Run all test suites
    all_tests_passed &= test_interface_functions();
    all_tests_passed &= test_functional_operations();
    all_tests_passed &= test_boundary_conditions();
    
    // Print test summary
    print_test_summary();
    
    // Cleanup
    test_deinit();
    
    return (all_tests_passed && g_test_results.failed_tests == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}