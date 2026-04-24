# Flash Device Model

This directory contains the complete Flash device model implementation for the DevCommV3 framework.

## Files

- `flash_device.py` - Main Flash device model implementation
- `test_flash_device.py` - Comprehensive test suite
- `c_driver/` - C HAL driver interface (generated)
- `README.md` - This documentation file

## Features

### Core Flash Functionality
- **Non-volatile storage** with file-backed memory simulation
- **Lock/unlock mechanism** with secure key sequence (0x00AC7805, 0x01234567)
- **Multiple flash operations**:
  - Page erase (2KB or 4KB pages)
  - Mass erase (entire flash)
  - Page program (64-bit programming units)
  - Page verify and mass verify
  - Option byte operations for Info area
- **Configurable page sizes**: 2048B or 4096B
- **Info area support**: Special 12KB region for configuration data
- **Write protection**: 64-region protection with register or Info area control
- **Read protection**: Configurable via Info area settings

### Framework Integration
- Inherits from `BaseDevice` base class  
- Uses `RegisterManager` for register handling
- Supports multi-instance deployment
- Integrated trace functionality
- Bus model integration
- Thread-safe operations

### Advanced Features
- **Error Injection**: Simulate ECC errors and protection violations for testing
- **ECC Simulation**: Single-bit and double-bit error detection/correction
- **Command State Machine**: Handles complex flash operation sequences
- **File Persistence**: Non-volatile memory backed by real files
- **Multi-threading**: Background command execution with proper synchronization

## Register Map

Based on `flash/input/flash_registers.yaml`:

| Address | Register | Description |
|---------|----------|-------------|
| 0x000 | FMUKR | Flash Memory Unlock Register (write-only) |
| 0x004 | FMGSR | Flash Memory General Status Register (read-only) |
| 0x008 | FMGCR | Flash Memory General Control Register |
| 0x010 | FMCCR | Flash Memory Command Control Register |
| 0x014 | FMADDR | Flash Memory Address Register |
| 0x018 | FMDATA0 | Flash Memory Data Register 0 (lower 32-bit) |
| 0x01C | FMDATA1 | Flash Memory Data Register 1 (upper 32-bit) |
| 0x020 | FMPFWPR0 | PFlash Write Protection Register 0 |
| 0x024 | FMPFWPR1 | PFlash Write Protection Register 1 |
| 0x028 | FMDFWPR0 | DFlash Write Protection Register 0 |
| 0x02C | FMDFWPR1 | DFlash Write Protection Register 1 |
| 0x030 | FMPFECADR | PFlash ECC Error Address Register |
| 0x034 | FMDFECADR | DFlash ECC Error Address Register |
| 0x040 | FMPFEIM0 | PFlash Error Injection Mask Register 0 |
| 0x044 | FMPFEIM1 | PFlash Error Injection Mask Register 1 |
| 0x050 | FMDFEIM0 | DFlash Error Injection Mask Register 0 |
| 0x054 | FMDFEIM1 | DFlash Error Injection Mask Register 1 |
| 0x070 | FMCNR | Chip Name Register (read-only) |
| 0x074+ | FMCIR0-2 | Chip UUID Registers (read-only) |

## Usage Examples

### Basic Device Creation

```python
from flash.output.flash_device import FlashDevice

# Create main storage flash (256KB)
flash_main = FlashDevice(
    name="MainFlash",
    base_address=0x40002000,    # Register base address
    size=0x1000,                # Register space size
    master_id=1,
    memory_size=256 * 1024,     # Flash memory size
    page_size=2048,             # Page size
    is_info_area=False,         # Main storage area
    storage_file="flash_main.bin"  # Persistence file
)

# Create Info area flash (12KB)
flash_info = FlashDevice(
    name="InfoFlash", 
    base_address=0x40003000,
    size=0x1000,
    master_id=2,
    memory_size=12 * 1024,      # Info area size
    page_size=4096,
    is_info_area=True           # Info area
)
```

### Unlock and Basic Operations

```python
# Step 1: Unlock flash device
flash_main.write(0x40002000, 0x00AC7805)  # First unlock key
flash_main.write(0x40002000, 0x01234567)  # Second unlock key

# Step 2: Check unlock status
status = flash_main.read(0x40002004)  # FMGSR
is_unlocked = not bool(status & 0x01)  # LOCK bit cleared

# Step 3: Erase a page
flash_main.write(0x40002014, 0x8000)     # Set address in FMADDR
flash_main.write(0x40002010, 0x11)       # Page erase command in FMCCR

# Wait for completion (check BUSY bit in FMGSR)
while flash_main.read(0x40002004) & 0x02:  # BUSY bit
    time.sleep(0.001)
```

### Page Programming

```python
# Step 1: Set programming data (64-bit units)
flash_main.write(0x40002018, 0x12345678)  # FMDATA0 (lower 32-bit)
flash_main.write(0x4000201C, 0x9ABCDEF0)  # FMDATA1 (upper 32-bit)

# Step 2: Set programming address
flash_main.write(0x40002014, 0x8000)      # FMADDR

# Step 3: Start programming command
flash_main.write(0x40002010, 0x111)       # Page program command + START

# Step 4: Wait for completion
while flash_main.read(0x40002004) & 0x02:  # Check BUSY bit
    time.sleep(0.001)
```

### Write Protection Configuration

```python
# Enable write protection for first 4 regions (bits 0-3)
flash_main.write(0x40002020, 0x0000000F)  # FMPFWPR0

# Try to erase protected region (will fail)
flash_main.write(0x40002014, 0x0000)      # Protected address
flash_main.write(0x40002010, 0x11)        # Erase command
```

### Direct Memory Operations (for testing)

```python
# Write data directly to flash memory
test_data = bytes([0x11, 0x22, 0x33, 0x44])
flash_main.write_memory(0x8000, test_data)

# Read data back
read_data = flash_main.read_memory(0x8000, len(test_data))
assert read_data == test_data
```

### Error Injection (Testing)

```python
from flash.output.flash_device import FlashErrorType

# Enable single-bit ECC error at address 0x1000
flash_main.enable_error_injection(0x1000, FlashErrorType.ECC_SINGLE_BIT_ERROR)

# Read will return corrupted data (1 bit flipped)
original_data = flash_main.read_memory(0x1000, 4)
corrupted_data = flash_main.read_memory(0x1000, 4)  # LSB flipped

# Disable error injection
flash_main.disable_error_injection(0x1000)
```

## Flash Command Types

| Command Code | Operation | Description |
|-------------|-----------|-------------|
| 0x01 | Page Erase | Erase a single page (2KB/4KB) |
| 0x02 | Option Page Erase | Erase page in Info area |
| 0x03 | Mass Erase | Erase entire flash memory |
| 0x04 | Reference Cell Erase | Internal operation |
| 0x11 | Page Program | Program 64-bit data units |
| 0x12 | Option Page Program | Program data in Info area |
| 0x21 | Page Verify | Verify page erase completion |
| 0x22 | Mass Verify | Verify mass erase completion |

## Testing

Run the comprehensive test suite:

```bash
cd /path/to/DeviceModel_V1.1
python3 flash/output/test_flash_device.py
```

The test suite validates:
- Device initialization and configuration
- Register operations and access control
- Lock/unlock functionality with key sequences
- Flash command execution (erase, program, verify)
- Write protection mechanisms
- Error injection capabilities
- File-based persistence
- Multi-instance support
- Realistic usage scenarios

## Implementation Details

### Thread Safety
- All register and memory operations are protected by device locks
- Background command execution using worker threads
- Atomic command state management

### Performance Considerations
- Simulated command timing (configurable)
- File-based persistence for non-volatile behavior
- Efficient memory management with bytearray storage
- Minimal blocking operations in register interface

### Error Handling
- Comprehensive error detection and reporting
- Write protection violation handling
- ECC error simulation with configurable injection points
- Command abort support for emergency situations

### Memory Management
- File-backed storage for persistence across device resets
- Configurable initial memory state (erased 0xFF or loaded from file)
- Support for both main storage and Info area configurations
- Protection region management (64 regions per device)

## Architecture Compliance

This implementation follows all requirements from `autogen_rules/device_model_gen.txt`:

- [x] Inherits from `BaseDevice`
- [x] Uses `RegisterManager` for register handling  
- [x] Supports multi-instance deployment
- [x] Implements device-specific read/write callbacks
- [x] Provides error injection capability
- [x] Integrates trace functionality
- [x] Supports interrupt generation
- [x] Follows register map specifications from YAML
- [x] Includes comprehensive testing
- [x] Supports file-based non-volatile storage

## File Storage Format

The flash device uses binary files to simulate non-volatile memory:

- **Main Storage**: Raw binary data, initialized to 0xFF (erased state)
- **Info Area**: Special initialization with read protection bytes at offset 0x00
- **Persistence**: Automatic save on write operations, load on device creation
- **Portability**: Standard binary format compatible across platforms

## Future Enhancements

Potential areas for enhancement:
- Advanced ECC algorithms with configurable polynomials
- Wear leveling simulation for endurance testing
- Power-fail simulation and recovery mechanisms
- Integration with DMA controllers for bulk operations
- Performance profiling and optimization tools
- Support for additional flash variants and configurations