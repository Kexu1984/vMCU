# Timer Device Model v1.1

This directory contains the Timer device model implementation for the DevCommV3 framework.

## Overview

The Timer device provides 4 independent 32-bit countdown timers with the following features:

- **4 Independent Timers**: Each timer can operate independently with its own configuration
- **Countdown Operation**: Timers count down from a load value to zero
- **Timer Link Mode**: Higher-numbered timers can be linked to lower-numbered timers for extended timing ranges
- **Shared Interrupt**: All timers share interrupt line #18
- **Global Enable/Disable**: Master control to enable/disable all timers
- **Error Injection**: Support for fault simulation and testing

## Architecture

### Timer Channels
- **Timer 0**: Base timer, cannot be linked to other timers
- **Timer 1**: Can be linked to Timer 0
- **Timer 2**: Can be linked to Timer 1  
- **Timer 3**: Can be linked to Timer 2

### Register Map

| Register | Offset | Description |
|----------|--------|-------------|
| TIMER_MCR | 0x000 | Global control register |
| TIMERx_LDVAL | 0x104+x*0x10 | Load value register |
| TIMERx_CVAL | 0x108+x*0x10 | Current value register (read-only) |
| TIMERx_INIT | 0x10C+x*0x10 | Timer control register |

### Register Bit Fields

**TIMER_MCR Register:**
- Bit[1]: MDIS - Module Disable (1=enabled, 0=disabled)

**TIMERx_INIT Register:**
- Bit[2]: LINKEN - Link Enable (1=enabled, 0=disabled) 
- Bit[1]: TIE - Timer Interrupt Enable (1=enabled, 0=disabled)
- Bit[0]: TEN - Timer Enable (1=enabled, 0=disabled)

## Usage

### Basic Timer Operation

```python
from timer_device import TimerDevice

# Create timer device
timer = TimerDevice("TIMER0", 0x40011000, 0x1000, 0)

# Configure Timer 0 for 1000ms countdown with interrupt
timer.write(0x40011000 + 0x104, 1000)  # TIMER0_LDVAL
timer.write(0x40011000 + 0x10C, 0x3)   # TIMER0_INIT: TEN=1, TIE=1

# Read current value
cval = timer.read(0x40011000 + 0x108)   # TIMER0_CVAL
```

### Timer Link Mode

```python
# Configure Timer 0 as base timer (100ms)
timer.write(0x40011000 + 0x104, 100)   # TIMER0_LDVAL
timer.write(0x40011000 + 0x10C, 0x1)   # TIMER0_INIT: TEN=1

# Configure Timer 1 in link mode (counts 5 Timer 0 pulses = 500ms total)
timer.write(0x40011000 + 0x110, 5)     # TIMER1_LDVAL
timer.write(0x40011000 + 0x118, 0x5)   # TIMER1_INIT: TEN=1, LINKEN=1
```

### Error Injection

```python
# Inject timer stuck error on Timer 0
timer.inject_error("timer_stuck", {"channel_id": 0})

# Inject interrupt failure on Timer 1
timer.inject_error("interrupt_fail", {"channel_id": 1})

# Check injected errors
status = timer.get_device_status()
print(f"Errors: {status['injected_errors']}")
```

## Files

- **`timer_device.py`**: Main Timer device implementation
- **`test_timer_device.py`**: Comprehensive test suite
- **`python_timer.py`**: Python wrapper with demo functions
- **`README.md`**: This documentation file

## Testing

Run the test suite:

```bash
python test_timer_device.py
```

Run the Python demo:

```bash  
python python_timer.py
```

## Implementation Details

### Timer Operation
1. When enabled, timer loads the LDVAL value and starts counting down
2. Every millisecond (in simulation), the timer decrements
3. The CVAL register increments as the timer counts down
4. When timer reaches 0, it generates an interrupt (if enabled) and reloads

### Link Mode Operation
1. In link mode, a timer only decrements when the linked timer expires
2. Timer N links to Timer N-1 (e.g., Timer 3 → Timer 2 → Timer 1 → Timer 0)
3. Timer 0 cannot be linked as it's the base timer

### Threading Model
- Background thread updates all active timers every 10ms
- Thread-safe operation with proper locking
- Graceful shutdown when device is disabled

### Error Injection Types
- **timer_stuck**: Timer stops counting
- **interrupt_fail**: Interrupts are disabled
- **register_corrupt**: Load value is corrupted
- **clock_fail**: Timer runs at incorrect speed

## Integration

The Timer device integrates with the DevCommV3 framework:

- Inherits from `BaseDevice` 
- Uses `RegisterManager` for register handling
- Supports tracing via `TraceManager`
- Triggers interrupts via bus interface
- Follows framework patterns for error handling and logging

## Address Space

- **Base Address**: 0x40011000 (as per specification)
- **Size**: 0x1000 (4KB address space)
- **Interrupt**: IRQ #18 (shared by all 4 timers)

## Compliance

This implementation follows the requirements specified in:
- `timer/input/Timer_Application_Note.md`
- `timer/input/timer_register_map.yaml` 
- `autogen_rules/device_model_gen.txt`