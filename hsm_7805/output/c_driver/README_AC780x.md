# AC780x HSM Driver

## Overview

This directory contains the C driver implementation for the AC780x HSM (Hardware Security Module) device. The driver follows a three-layer architecture:

- **HAL Layer** (`hal/`): Hardware abstraction layer for register access
- **Driver Layer** (`driver/`): Business logic and device-specific functionality
- **Test Layer** (`test/`): Comprehensive test suite

## File Structure

```
c_driver_7805/
├── hal/
│   ├── ac780x_hsm_hal.h        # HAL interface definitions
│   └── ac780x_hsm_hal.c        # HAL implementation
├── driver/
│   ├── ac7805x.h               # AC7805x device definitions
│   ├── ac780x_hsm.h            # HSM driver interface
│   ├── ac780x_hsm.c            # HSM driver implementation
│   └── ac780x_hsm_reg.h        # HSM register definitions
├── test/
│   ├── hsm_test_wrapper.c      # Test framework wrapper
│   └── multitask_hsm.c         # Original multitask test
├── interface_layer.h           # Interface layer definitions
├── libdrvintf.a                # Interface library (simulation)
├── libdrvintf_dbg.a           # Debug interface library
├── Makefile                    # Build configuration
└── README.md                   # This file
```

## Building

### Basic Build Commands

```bash
# Build all (recommended - includes test wrapper)
make all

# Build only libraries
make libs

# Build standalone multitask test
make multitask

# Clean build artifacts
make clean

# Show help
make help
```

### Build Options

```bash
# Debug build with debug symbols and debug library
make DEBUG=1

# Build for real hardware (no simulation interface)
make REAL_CHIP=1

# Combined options
make test DEBUG=1 REAL_CHIP=1
```

## Running Tests

### With Test Wrapper (Recommended)

```bash
# Build and run with default base address (0x40000000)
make test
./ac780x_hsm_test

# Run with custom base address
./ac780x_hsm_test 0x50000000
```

### Standalone Multitask Test

```bash
# Build standalone version
make multitask
./multitask_hsm_standalone
```

## Architecture Details

### HAL Layer (`hal/`)

Provides hardware abstraction with:
- Base address management
- Raw register read/write functions
- Support for both simulation and real chip modes

### Driver Layer (`driver/`)

Contains the complete AC780x HSM driver implementation:
- `ac780x_hsm.c/h`: Main driver implementation
- `ac780x_hsm_reg.h`: Register definitions and bit fields
- `ac7805x.h`: Device-specific definitions

### Test Layer (`test/`)

- `hsm_test_wrapper.c`: Standardized test framework
- `multitask_hsm.c`: Original comprehensive test suite

## Compilation Modes

### Simulation Mode (Default)
- Links with `libdrvintf.a` for communication with Python device models
- Requires interface layer initialization
- Suitable for testing and validation

### Real Chip Mode
- Direct memory access to hardware registers
- No interface library dependency
- Compile with `REAL_CHIP=1`

## Interface Layer Integration

The driver integrates with Python device models through:
- `interface_layer_init()`: Initialize communication
- `register_device()`: Register device address space
- `write_register()/read_register()`: Register access functions
- `interface_layer_deinit()`: Cleanup

## Dependencies

- GCC compiler (C99 standard)
- Interface library (`libdrvintf.a` or `libdrvintf_dbg.a`) for simulation
- No external dependencies for real chip compilation

## Examples

```bash
# Standard development workflow
make clean
make test DEBUG=1
./ac780x_hsm_test 0x40000000

# Production build for real hardware
make clean
make test REAL_CHIP=1
./ac780x_hsm_test 0x40000000

# Library-only build for integration
make libs
```

## Notes

- The test wrapper provides standardized initialization and cleanup
- Base address is configurable via command line parameter
- Debug builds include additional logging and use debug interface library
- Real chip builds bypass the simulation interface layer

For more information, run `make help` to see all available targets and options.
