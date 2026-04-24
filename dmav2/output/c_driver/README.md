# DMAv2 C Driver Implementation

This directory contains the complete C driver implementation for the DMAv2 device, following the three-layer architecture specified in `autogen_rules/device_driver_gen.md`.

## Architecture

```
┌─────────────────────────────────┐
│ TEST LAYER                      │ ← Comprehensive validation
│ - dmav2_test.c                  │
├─────────────────────────────────┤
│ DRIVER LAYER                    │ ← Business logic & APIs
│ - dmav2_driver.h/.c             │
├─────────────────────────────────┤
│ HAL LAYER                       │ ← Register abstraction
│ - dmav2_hal.h/.c                │
├─────────────────────────────────┤
│ INTERFACE LAYER                 │ ← Model communication
│ - libdrvintf.a                  │
└─────────────────────────────────┘
```

## Directory Structure

```
dmav2/output/c_driver/
├── hal/
│   ├── dmav2_hal.h      # HAL interface definitions
│   └── dmav2_hal.c      # HAL implementation
├── driver/
│   ├── dmav2_driver.h   # Driver interface definitions  
│   └── dmav2_driver.c   # Driver implementation
├── test/
│   └── dmav2_test.c     # Comprehensive test suite
├── build/               # Build artifacts (created by make)
├── Makefile             # Build system
├── libdrvintf.a         # Interface layer library
└── README.md            # This file
```

## Features

### DMAv2 Hardware Features
- **16 Independent Channels**: Channels 0-15, each with full configuration
- **Multiple Transfer Modes**:
  - Memory-to-Memory (mem2mem)
  - Memory-to-Peripheral (mem2peri) 
  - Peripheral-to-Memory (peri2mem)
  - Peripheral-to-Peripheral (peri2peri)
- **Address Modes**: Fixed address or increment modes for source/destination
- **Data Widths**: 8-bit, 16-bit, 32-bit transfer sizes
- **Priority Levels**: Low, Medium, High, Very High
- **Circular Buffer Support**: Automatic address wrapping
- **DMAMUX Integration**: Peripheral request routing with trigger support

### Driver Features
- **Complete API Coverage**: All hardware features exposed through clean APIs
- **State Management**: Channel state tracking and validation
- **Error Handling**: Comprehensive error detection and reporting
- **Configuration Validation**: Parameter bounds checking
- **Convenience Functions**: High-level transfer setup helpers
- **Debug Support**: Register dumping and self-test capabilities

### Test Suite Features
- **Three Test Categories**:
  1. Interface Testing (API coverage)
  2. Functional Testing (feature compliance) 
  3. Boundary Testing (edge cases)
- **95%+ Coverage**: All public functions tested
- **All DMA Modes**: Complete mode coverage testing
- **Error Injection**: Boundary condition validation
- **Automated Execution**: Command-line test runner

## Building

### Prerequisites
- GCC compiler with C99 support
- GNU Make
- libdrvintf.a (provided)

### Build Commands

```bash
# Build everything (default: simulator mode)
make

# Build for real hardware
make REAL_CHIP=1

# Clean build artifacts
make clean

# Run tests
make test

# Run tests with valgrind (if available)
make test-valgrind

# Show build configuration
make debug

# Show all available targets
make help
```

### Build Modes

**Simulator Mode (Default)**:
- Uses interface layer for device communication
- Links with libdrvintf.a
- Communicates with Python device model

**Real Hardware Mode**:
- Direct memory-mapped register access
- No interface layer dependency
- Compiled with `-DREAL_CHIP`

## Usage

### Basic Initialization

```c
#include "dmav2_driver.h"

// Initialize driver
dmav2_status_t status = dmav2_init();
if (status != DMAV2_SUCCESS) {
    // Handle error
}

// Initialize channel
status = dmav2_channel_init(0);
```

### Memory-to-Memory Transfer

```c
// Simple memory copy
uint32_t src_addr = 0x20000000;
uint32_t dst_addr = 0x20001000; 
uint16_t count = 256;

status = dmav2_mem_to_mem_transfer(0, src_addr, dst_addr, count, DMAV2_DATA_WIDTH_32BIT);
```

### Advanced Configuration

```c
// Configure channel for peripheral transfer
dmav2_channel_config_t config = {
    .mode = DMAV2_MODE_MEM2PERI,
    .src_width = DMAV2_DATA_WIDTH_32BIT,
    .dst_width = DMAV2_DATA_WIDTH_32BIT,
    .priority = DMAV2_PRIORITY_HIGH,
    .src_addr_mode = DMAV2_ADDR_INCREMENT,
    .dst_addr_mode = DMAV2_ADDR_FIXED,
    .dmamux_req_id = 2,  // UART TX
    .circular_mode = false
};

status = dmav2_channel_configure(1, &config);
```

### Transfer Management

```c
// Setup transfer
dmav2_transfer_config_t transfer = {
    .src_addr = 0x20000000,
    .dst_addr = 0x40000000,  // Peripheral register
    .transfer_count = 128,
    .src_offset = 4,         // 32-bit increment
    .dst_offset = 0          // Fixed peripheral
};

// Start transfer
status = dmav2_start_transfer(1, &transfer);

// Check status
dmav2_channel_status_t ch_status;
status = dmav2_get_channel_status(1, &ch_status);

// Stop when complete
if (ch_status.transfer_complete) {
    dmav2_stop_transfer(1);
}
```

## Testing

### Running Tests

```bash
# Run with default base address
./dmav2_test 0x40000000

# Run with custom base address  
./dmav2_test 0x50000000

# Example output:
DMAv2 Driver Test Suite
Base Address: 0x40000000
============================================================
DMAv2 DRIVER COMPREHENSIVE TEST SUITE
============================================================
Driver Version: DMAv2 Driver v2.0.0
============================================================

=== INTERFACE TESTING ===
--- Driver Initialization Interface ---
  ✓ dmav2_init() returns success
  ✓ dmav2_deinit() returns success
  ✓ dmav2_init() after deinit returns success
...
```

### Test Categories

**Interface Testing**: Validates all API functions return appropriate values and handle parameters correctly.

**Functional Testing**: Tests all DMA modes and features:
- Memory-to-memory transfers
- Memory-to-peripheral transfers  
- Peripheral-to-memory transfers
- Address fixed/increment modes
- Circular buffer operation
- Priority levels
- DMAMUX configuration

**Boundary Testing**: Validates error handling:
- Zero-length transfers
- Invalid addresses
- Parameter boundary values
- Resource limits
- Null pointer handling

## Register Map

Based on `dmav2/input/dma_register_map.yaml`:

| Address | Register | Description |
|---------|----------|-------------|
| 0x000 | DMA_TOP_RST | Global reset control |
| 0x040+ | CHANNELx_STATUS | Channel status (per channel) |
| 0x064+ | CHANNELx_CHAN_ENABLE | Channel enable/debug control |
| 0x068+ | CHANNELx_DATA_TRANS_NUM | Data transfer count |
| 0x06C+ | CHANNELx_INTER_FIFO_DATA_LEFT_NUM | FIFO data remaining |
| 0x070+ | CHANNELx_DEND_ADDR | Destination end address |
| 0x074+ | CHANNELx_ADDR_OFFSET | Source/destination offsets |
| 0x078+ | CHANNELx_DMAMUX_CFG | DMAMUX configuration |

Each channel occupies 0x40 bytes in the address space.

## Error Handling

All functions return `dmav2_status_t` with detailed error codes:

- `DMAV2_SUCCESS`: Operation successful
- `DMAV2_ERROR_INVALID_PARAM`: Invalid parameter
- `DMAV2_ERROR_NOT_INITIALIZED`: Driver not initialized
- `DMAV2_ERROR_HARDWARE_FAULT`: Hardware error
- `DMAV2_ERROR_TIMEOUT`: Operation timeout
- `DMAV2_ERROR_BUSY`: Channel busy
- `DMAV2_ERROR_CHANNEL_INVALID`: Invalid channel number
- `DMAV2_ERROR_CONFIG_INVALID`: Invalid configuration
- `DMAV2_ERROR_TRANSFER_FAILED`: Transfer operation failed

Use `dmav2_get_error_string()` to get human-readable error descriptions.

## Compliance

This implementation follows all requirements from `autogen_rules/device_driver_gen.md`:

- [x] Three-layer architecture (HAL → Driver → Test)
- [x] Interface layer integration for model communication  
- [x] Support for both simulator and real chip compilation
- [x] Comprehensive error handling and parameter validation
- [x] Complete register map implementation
- [x] All DMA modes and features supported
- [x] Boundary testing and edge case handling
- [x] CMSIS-style API design
- [x] Modular library structure with proper dependencies

## Dependencies

- **HAL Layer**: Direct hardware abstraction
- **Driver Layer**: Depends on HAL layer
- **Test Layer**: Depends on Driver and HAL layers, interface layer
- **Interface Layer**: `libdrvintf.a` (for simulator mode only)

## Version Information

- **Driver Version**: 2.0.0
- **Architecture**: Three-layer driver model
- **Target Device**: DMAv2 with 16 channels
- **Compliance**: NewICD3 device driver standards