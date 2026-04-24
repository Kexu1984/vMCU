# CRC Driver C Implementation

This directory contains a complete C language implementation of a CRC (Cyclic Redundancy Check) driver based on the register specification in `../input/register.yaml`.

## Architecture

The implementation follows a layered architecture with clear separation of concerns:

### 1. HAL Layer (Hardware Abstraction Layer)
- **Location**: `hal/`
- **Files**: `crc_hal.h`, `crc_hal.c`
- **Purpose**: Provides low-level register access functions
- **Features**:
  - Direct register read/write operations
  - Register address calculations
  - Hardware initialization and reset
  - Parameter validation
  - Simulated register storage for testing

### 2. Driver Layer (Business Logic Layer)
- **Location**: `driver/`
- **Files**: `crc_driver.h`, `crc_driver.c`
- **Purpose**: Implements CRC calculation business logic
- **Features**:
  - CRC16-CCITT and CRC32-IEEE calculations
  - Multiple context management (3 independent contexts)
  - Configurable byte/bit ordering
  - Incremental calculation support
  - High-level API for common operations

### 3. Test Layer
- **Location**: `test/`
- **Files**: `test_crc_driver.c`
- **Purpose**: Comprehensive testing of all driver functionality
- **Features**:
  - HAL layer validation
  - Driver layer testing
  - Multiple CRC algorithms verification
  - Error condition testing
  - Performance verification

## Supported Features

### CRC Algorithms
- **CRC16-CCITT**: Polynomial x^16 + x^12 + x^5 + 1 (0x1021)
- **CRC32-IEEE**: Polynomial 0x04C11DB7 (IEEE 802.3 standard)

### Hardware Features (per register specification)
- **3 Independent Contexts**: Each with separate configuration and state
- **Configurable Parameters**:
  - CRC mode selection (CRC16/CRC32)
  - Input byte ordering (MSB/LSB first)
  - Input bit ordering (MSB/LSB first)
  - Output byte ordering (MSB/LSB first)
  - Output bit ordering (MSB/LSB first)
  - Output inversion

### Register Map
Based on `register.yaml` specification:

#### Context 0 (Base: 0x000)
- `0x000`: CtxMode - Configuration register
- `0x004`: CRC_IVAL - Initial value register
- `0x008`: CRC_DATA - Data input register (write-only)

#### Context 1 (Base: 0x020)
- `0x020`: CtxMode - Configuration register
- `0x024`: CRC_IVAL - Initial value register
- `0x028`: CRC_DATA - Data input register (write-only)

#### Context 2 (Base: 0x040)
- `0x040`: CtxMode - Configuration register
- `0x044`: CRC_IVAL - Initial value register
- `0x048`: CRC_DATA - Data input register (write-only)

### CtxMode Register Fields
- Bit 0: `CRC_MODE` - CRC type (0=CRC16, 1=CRC32)
- Bit 1: `ACC_MS_BIT` - Accumulate MS bit first
- Bit 2: `ACC_MS_BYTE` - Accumulate MS byte first
- Bit 3: `OUT_MS_BIT` - Output MS bit first
- Bit 4: `OUT_MS_BYTE` - Output MS byte first
- Bit 5: `OUT_INV` - Invert output

## Building

### Prerequisites
- GCC compiler (or compatible C compiler)
- Make utility
- Optional: valgrind, clang-format, cppcheck, doxygen

### Build Commands

```bash
# Build all libraries and test executable
make all

# Build and run tests
make test

# Run tests with memory checking (if valgrind available)
make test-valgrind

# Clean build artifacts
make clean

# Install to system directories
sudo make install

# Format code
make format

# Run static analysis
make analyze

# Generate documentation
make docs

# Show help
make help
```

### Build Artifacts
- `build/libcrc_hal.a` - HAL static library
- `build/libcrc_driver.a` - Driver static library
- `build/test_crc_driver` - Test executable

## Usage Examples

### Basic CRC Calculation

```c
#include "driver/crc_driver.h"

int main() {
    // Initialize driver
    crc_driver_init(0x40000000);
    
    // Calculate CRC16 for data
    uint8_t data[] = "Hello, World!";
    uint16_t crc16_result;
    
    crc_driver_calculate_crc16_direct(data, sizeof(data)-1, 
                                     CRC16_INITIAL_VALUE, &crc16_result);
    
    printf("CRC16: 0x%04X\n", crc16_result);
    
    // Calculate CRC32 for data
    uint32_t crc32_result;
    crc_driver_calculate_crc32_direct(data, sizeof(data)-1, 
                                     CRC32_INITIAL_VALUE, &crc32_result);
    
    printf("CRC32: 0x%08X\n", crc32_result);
    
    crc_driver_deinit();
    return 0;
}
```

### Context-based Calculation

```c
#include "driver/crc_driver.h"

int main() {
    // Initialize driver
    crc_driver_init(0x40000000);
    
    // Configure context for CRC32
    crc_config_t config;
    crc_driver_get_default_config(CRC_MODE_CRC32_IEEE, &config);
    crc_driver_configure_context(CRC_CONTEXT_0, &config);
    
    // Calculate using context
    uint8_t data[] = "Hello, World!";
    uint32_t result;
    
    crc_driver_calculate_buffer(CRC_CONTEXT_0, data, sizeof(data)-1, &result);
    
    printf("CRC32 (context): 0x%08X\n", result);
    
    crc_driver_deinit();
    return 0;
}
```

### Incremental Calculation

```c
#include "driver/crc_driver.h"

int main() {
    // Initialize and configure
    crc_driver_init(0x40000000);
    
    crc_config_t config;
    crc_driver_get_default_config(CRC_MODE_CRC16_CCITT, &config);
    crc_driver_configure_context(CRC_CONTEXT_0, &config);
    
    // Start calculation
    crc_driver_start_calculation(CRC_CONTEXT_0, 0);
    
    // Process data in chunks
    uint8_t chunk1[] = "Hello, ";
    uint8_t chunk2[] = "World!";
    
    crc_driver_process_data(CRC_CONTEXT_0, chunk1, sizeof(chunk1)-1);
    crc_driver_process_data(CRC_CONTEXT_0, chunk2, sizeof(chunk2)-1);
    
    // Finalize and get result
    uint32_t result;
    crc_driver_finalize_calculation(CRC_CONTEXT_0, &result);
    
    printf("Incremental CRC16: 0x%08X\n", result);
    
    crc_driver_deinit();
    return 0;
}
```

## API Reference

### HAL Layer API

#### Initialization
- `crc_hal_init(base_address)` - Initialize HAL
- `crc_hal_deinit()` - Deinitialize HAL

#### Register Access
- `crc_hal_read_register(context, reg, value)` - Read register
- `crc_hal_write_register(context, reg, value)` - Write register
- `crc_hal_read_register_direct(address, value)` - Direct read
- `crc_hal_write_register_direct(address, value)` - Direct write

#### Utilities
- `crc_hal_get_register_address(context, reg)` - Get register address
- `crc_hal_reset_context(context)` - Reset context
- `crc_hal_reset_all_contexts()` - Reset all contexts

### Driver Layer API

#### Initialization
- `crc_driver_init(base_address)` - Initialize driver
- `crc_driver_deinit()` - Deinitialize driver

#### Configuration
- `crc_driver_configure_context(context, config)` - Configure context
- `crc_driver_get_default_config(mode, config)` - Get default config
- `crc_driver_reset_context(context)` - Reset context
- `crc_driver_get_context_state(context, state)` - Get context state

#### CRC Calculation
- `crc_driver_calculate_buffer(context, data, length, result)` - Calculate buffer
- `crc_driver_calculate_crc16_direct(data, length, initial, result)` - Direct CRC16
- `crc_driver_calculate_crc32_direct(data, length, initial, result)` - Direct CRC32

#### Incremental Calculation
- `crc_driver_start_calculation(context, initial)` - Start calculation
- `crc_driver_process_data(context, data, length)` - Process data chunk
- `crc_driver_process_word(context, word)` - Process 32-bit word
- `crc_driver_finalize_calculation(context, result)` - Finalize and get result
- `crc_driver_get_current_value(context, value)` - Get current value

#### Utilities
- `crc_driver_is_context_busy(context, is_busy)` - Check if busy
- `crc_driver_get_version(major, minor, patch)` - Get version
- `crc_driver_status_to_string(status)` - Convert status to string

## Error Handling

All functions return status codes indicating success or failure:

### HAL Status Codes
- `CRC_HAL_OK` - Operation successful
- `CRC_HAL_ERROR` - General error
- `CRC_HAL_INVALID_PARAM` - Invalid parameter
- `CRC_HAL_TIMEOUT` - Operation timeout
- `CRC_HAL_BUSY` - Device busy

### Driver Status Codes
- `CRC_DRV_OK` - Operation successful
- `CRC_DRV_ERROR` - General error
- `CRC_DRV_INVALID_PARAM` - Invalid parameter
- `CRC_DRV_CONTEXT_BUSY` - Context busy
- `CRC_DRV_NOT_INITIALIZED` - Driver not initialized
- `CRC_DRV_HARDWARE_ERROR` - Hardware error

## Testing

The test suite (`test/test_crc_driver.c`) provides comprehensive validation:

### Test Categories
1. **HAL Layer Tests**
   - Initialization/deinitialization
   - Register operations
   - Address calculations
   - Error conditions

2. **Driver Layer Tests**
   - Driver initialization
   - Context configuration
   - CRC calculations (CRC16/CRC32)
   - Multiple context operations
   - Incremental calculations
   - Error handling

3. **Algorithm Verification**
   - Known test vectors
   - Different data patterns
   - Edge cases

### Running Tests

```bash
# Run all tests
make test

# Sample output:
# ============================================================
# CRC Driver Test Suite
# ============================================================
# 
# --- test_hal_initialization ---
# Result: ✅ PASS
# 
# --- test_hal_register_operations ---
# Result: ✅ PASS
# 
# ... (more tests)
# 
# ============================================================
# Test Results Summary
# ============================================================
# Total tests: 10
# Passed: 10
# Failed: 0
# Success rate: 100.0%
# Overall: ✅ ALL TESTS PASSED
```

## Compatibility

This implementation is designed to be compatible with:
- **Python CRC Device Model**: Located in `../crc_device.py`
- **Register Specification**: Based on `../input/register.yaml`
- **DevComm Framework**: Follows same architectural principles

## File Structure

```
c_driver/
├── hal/
│   ├── crc_hal.h          # HAL header
│   └── crc_hal.c          # HAL implementation
├── driver/
│   ├── crc_driver.h       # Driver header
│   └── crc_driver.c       # Driver implementation
├── test/
│   └── test_crc_driver.c  # Test program
├── build/                 # Build directory (created by make)
├── Makefile              # Build configuration
└── README.md             # This file
```

## Future Enhancements

Potential improvements for future versions:
1. **Hardware Integration**: Replace simulated registers with actual hardware access
2. **DMA Support**: Add DMA integration for large data transfers
3. **Interrupt Support**: Implement completion interrupts
4. **Performance Optimization**: Add lookup table-based CRC algorithms
5. **Additional Algorithms**: Support for more CRC variants
6. **Real-time Features**: Add timing guarantees and priority levels

## License

This implementation is part of the DeviceModel_V1.0 project and follows the same licensing terms.