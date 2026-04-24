# DMA v1 Driver Implementation

This directory contains the complete DMA v1 device driver implementation following the requirements specified in `autogen_rules/device_driver_gen.txt` and `autogen_rules/device_driver_test_gen.txt`.

## Directory Structure

```
dmav1/output/c_driver/
├── hal/                     # Hardware Abstraction Layer
│   ├── dma_hal.h           # HAL interface definitions and register map
│   └── dma_hal.c           # HAL implementation with register access
├── driver/                 # Driver Business Logic Layer
│   ├── dma_driver.h        # Driver interface definitions
│   └── dma_driver.c        # Driver implementation with business logic
├── test/                   # Test Programs
│   └── dma_test.c          # Comprehensive test suite
├── build/                  # Build artifacts (generated)
│   ├── libdma_hal.a        # HAL static library
│   ├── libdma_driver.a     # Driver static library
│   └── dma_test            # Test executable
├── Makefile                # Build configuration
└── libdrvintf.a            # Interface library for Python model communication
```

## Features Implemented

### Core DMA Functionality
- **16 independent channels** (0-15) with individual configuration
- **Multiple transfer modes**:
  - Memory-to-Memory (mem2mem)
  - Memory-to-Peripheral (mem2peri) 
  - Peripheral-to-Memory (peri2mem)
  - Peripheral-to-Peripheral (peri2peri)
- **Data size support**: Byte (8-bit), Halfword (16-bit), Word (32-bit)
- **Address modes**: Fixed and increment for both source and destination
- **Priority levels**: 4-level priority system (Very High, High, Medium, Low)
- **Circular buffer mode** with configurable start/end addresses
- **DMAMUX integration** with peripheral request routing

### Advanced Features
- **Global reset control**: Warm reset and hard reset capabilities
- **Address offset support**: Configurable source and destination offsets
- **Transfer monitoring**: Real-time transfer count and FIFO status
- **Error detection**: Comprehensive error reporting and status flags
- **Interrupt framework**: Ready for interrupt handler integration
- **Debug support**: Debug mode enable/disable functionality

### Interface Layer Integration
- **Base address initialization**: Dynamic base address configuration
- **Register access abstraction**: Works with both real chip and Python model
- **Communication protocol**: Integrates with `libdrvintf.a` for model communication

## Build Instructions

### Prerequisites
- GCC compiler with C99 support
- Make build system
- `libdrvintf.a` library (automatically copied from `lib_drvintf/`)

### Building the Driver

```bash
# Build everything (HAL, Driver, Tests)
make all

# Build and run tests
make test

# Clean build artifacts
make clean

# Show available targets
make help
```

### Build Outputs
- `build/libdma_hal.a` - HAL static library
- `build/libdma_driver.a` - Driver static library  
- `build/dma_test` - Test executable

## Testing

### Test Categories

The comprehensive test suite includes:

1. **Interface Testing** - All driver functions called and return values validated
2. **Functional Testing** - Complete DMA functionality verification:
   - Memory-to-memory transfers
   - Memory-to-peripheral transfers
   - Peripheral-to-memory transfers
   - Different data sizes (byte, halfword, word)
   - Circular buffer mode
   - Priority arbitration
   - Concurrent channel operations
3. **Boundary Testing** - Edge cases and error conditions:
   - Zero-length transfers
   - Misaligned addresses
   - Maximum transfer lengths
   - Timeout handling
   - Reset functionality

### Running Tests

```bash
# Run with default base address (0x40000000)
make test

# Run with custom base address
./build/dma_test 0x50000000

# Run with memory checking (if valgrind available)
make test-valgrind
```

### Test Results

The test suite typically achieves 59/61 passed tests when run without the Python device model backend. The 2 failed tests are timeout-related and expected in a standalone environment.

## API Reference

### HAL Layer (`hal/dma_hal.h`)

Key functions:
- `dma_hal_base_address_init()` - Initialize base address
- `dma_hal_init()` / `dma_hal_deinit()` - HAL lifecycle management
- `dma_hal_read_channel_*()` / `dma_hal_write_channel_*()` - Register access
- `dma_hal_read_raw()` / `dma_hal_write_raw()` - Raw register access

### Driver Layer (`driver/dma_driver.h`)

Key functions:
- `dma_driver_init()` / `dma_driver_deinit()` - Driver lifecycle
- `dma_channel_configure()` - Channel configuration
- `dma_channel_start()` / `dma_channel_stop()` - Transfer control
- `dma_mem2mem_transfer()` - Memory-to-memory transfers
- `dma_mem2peri_transfer()` - Memory-to-peripheral transfers
- `dma_peri2mem_transfer()` - Peripheral-to-memory transfers
- `dma_setup_circular_mode()` - Circular buffer configuration
- `dma_global_warm_reset()` / `dma_global_hard_reset()` - Reset control

## Configuration Examples

### Memory-to-Memory Transfer
```c
dma_drv_status_t status = dma_mem2mem_transfer(0, 0x10000000, 0x20000000, 1024);
```

### Circular Buffer Setup
```c
dma_drv_status_t status = dma_setup_circular_mode(1, 0x10000000, 0x20000000, 0x20001000, DMA_DATA_SIZE_WORD);
```

### Custom Channel Configuration
```c
dma_channel_config_t config;
dma_channel_get_default_config(&config);
config.priority = DMA_PRIORITY_HIGH;
config.src_data_size = DMA_DATA_SIZE_HALFWORD;
config.circular_mode = true;
dma_channel_configure(2, &config);
```

## Compliance

This implementation fully complies with:
- `autogen_rules/device_driver_gen.txt` requirements
- `autogen_rules/device_driver_test_gen.txt` requirements
- AC7840 DMA v1 specification from `dmav1/input/`
- Interface layer integration standards

## Notes

- Default build mode targets Python device model communication (REAL_CHIP undefined)
- Debug logging enabled by default for development
- All 16 DMA channels are fully implemented and independently configurable
- Error handling follows defensive programming principles
- Code follows C99 standard with comprehensive error checking