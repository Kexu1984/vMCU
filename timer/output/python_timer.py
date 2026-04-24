"""
Python wrapper for the Timer device.

This module provides a convenient Python interface for the Timer device,
making it easy to use in Python applications and test scripts.
"""

import sys
import os
sys.path.append(os.path.dirname(__file__))

from timer_device import TimerDevice, TimerState


def create_timer_device(name: str = "TIMER", base_address: int = 0x40011000, 
                       size: int = 0x1000, master_id: int = 0) -> TimerDevice:
    """
    Create a Timer device instance.
    
    Args:
        name: Device name (default: "TIMER")
        base_address: Base address of the timer (default: 0x40011000)
        size: Size of timer address space (default: 0x1000)
        master_id: Master ID for the device (default: 0)
        
    Returns:
        TimerDevice instance
    """
    return TimerDevice(name, base_address, size, master_id)


def demo_timer_basic():
    """Demonstrate basic timer functionality."""
    print("Timer Device Basic Demo")
    print("=" * 30)
    
    # Create timer device
    timer = create_timer_device()
    
    # Create mock bus for interrupt handling
    class MockBus:
        def send_irq(self, master_id, irq_num):
            print(f"Interrupt {irq_num} triggered!")
    
    timer.set_bus(MockBus())
    
    # Configure Timer 0 for 100ms countdown with interrupt
    print("Configuring Timer 0 for 100ms countdown...")
    timer.write(0x40011000 + 0x104, 100)  # TIMER0_LDVAL = 100ms
    timer.write(0x40011000 + 0x10C, 0x3)  # TIMER0_INIT: TEN=1, TIE=1
    
    print(f"Timer 0 status: {timer.get_timer_status(0)}")
    
    # Wait and check
    import time
    time.sleep(0.15)
    
    print(f"Timer 0 status after delay: {timer.get_timer_status(0)}")
    print(f"Device status: {timer.get_device_status()}")
    
    timer.disable()


def demo_timer_link_mode():
    """Demonstrate timer link mode functionality."""
    print("\nTimer Device Link Mode Demo")
    print("=" * 30)
    
    # Create timer device
    timer = create_timer_device()
    
    # Configure Timer 0 as base timer (10ms)
    timer.write(0x40011000 + 0x104, 10)   # TIMER0_LDVAL = 10ms
    timer.write(0x40011000 + 0x10C, 0x1)  # TIMER0_INIT: TEN=1
    
    # Configure Timer 1 in link mode (counts 3 pulses from Timer 0)
    timer.write(0x40011000 + 0x110, 3)    # TIMER1_LDVAL = 3
    timer.write(0x40011000 + 0x118, 0x5)  # TIMER1_INIT: TEN=1, LINKEN=1
    
    print("Timer 0: 10ms countdown (base timer)")
    print("Timer 1: linked to Timer 0, counts 3 pulses")
    
    print(f"Timer 0 status: {timer.get_timer_status(0)}")
    print(f"Timer 1 status: {timer.get_timer_status(1)}")
    
    timer.disable()


def demo_error_injection():
    """Demonstrate error injection functionality."""
    print("\nTimer Device Error Injection Demo")
    print("=" * 30)
    
    # Create timer device
    timer = create_timer_device()
    
    # Configure Timer 0
    timer.write(0x40011000 + 0x104, 50)   # TIMER0_LDVAL = 50ms
    timer.write(0x40011000 + 0x10C, 0x3)  # TIMER0_INIT: TEN=1, TIE=1
    
    print("Before error injection:")
    print(f"Timer 0 status: {timer.get_timer_status(0)}")
    
    # Inject timer stuck error
    timer.inject_error("timer_stuck", {"channel_id": 0})
    
    print("\nAfter injecting timer_stuck error:")
    print(f"Timer 0 status: {timer.get_timer_status(0)}")
    print(f"Injected errors: {timer.get_device_status()['injected_errors']}")
    
    timer.disable()


if __name__ == "__main__":
    print("Timer Device Python Wrapper Demo")
    print("=" * 50)
    
    demo_timer_basic()
    demo_timer_link_mode() 
    demo_error_injection()
    
    print("\nDemo completed!")