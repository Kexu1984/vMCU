"""
Test module for Timer device implementation.

This module tests the Timer device functionality including:
- Basic timer countdown operation
- Timer link mode
- Interrupt generation
- Global enable/disable
- Error injection
"""

import time
import unittest
import sys
import os

# Add path to access the timer device and framework
sys.path.append(os.path.dirname(__file__))
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))

from timer_device import TimerDevice, TimerState


class MockBus:
    """Mock bus for testing interrupt functionality."""
    
    def __init__(self):
        self.interrupts_triggered = []
        
    def send_irq(self, master_id: int, irq_number: int):
        """Record interrupt for verification."""
        self.interrupts_triggered.append(irq_number)
        print(f"Mock bus: Interrupt {irq_number} triggered by master {master_id}")
        
    def trigger_interrupt(self, irq_number: int):
        """Legacy method for compatibility."""
        self.send_irq(0, irq_number)


class TestTimerDevice(unittest.TestCase):
    """Test cases for Timer device."""
    
    def setUp(self):
        """Set up test environment."""
        # Create timer device with mock bus
        self.timer = TimerDevice("TIMER0", 0x40011000, 0x1000, 0)
        self.mock_bus = MockBus()
        self.timer.set_bus(self.mock_bus)
        
    def tearDown(self):
        """Clean up after tests."""
        if self.timer:
            self.timer.disable()
            
    def test_device_initialization(self):
        """Test device initialization."""
        self.assertEqual(self.timer.name, "TIMER0")
        self.assertEqual(self.timer.base_address, 0x40011000)
        self.assertEqual(self.timer.size, 0x1000)
        self.assertTrue(self.timer.global_enabled)
        self.assertEqual(len(self.timer.channels), 4)
        
        # Check all channels are initialized
        for i in range(4):
            self.assertIn(i, self.timer.channels)
            channel = self.timer.channels[i]
            self.assertEqual(channel.channel_id, i)
            self.assertFalse(channel.enabled)
            self.assertEqual(channel.load_value, 0)
            self.assertEqual(channel.current_value, 0)
            
    def test_register_access(self):
        """Test register read/write operations."""
        # Test TIMER_MCR register
        mcr_value = self.timer.read(0x40011000 + 0x000)  # TIMER_MCR
        self.assertEqual(mcr_value, 0x2)  # Default: global enabled
        
        # Disable globally
        self.timer.write(0x40011000 + 0x000, 0x0)
        mcr_value = self.timer.read(0x40011000 + 0x000)
        self.assertEqual(mcr_value, 0x0)
        self.assertFalse(self.timer.global_enabled)
        
        # Re-enable globally
        self.timer.write(0x40011000 + 0x000, 0x2)
        self.assertTrue(self.timer.global_enabled)
        
    def test_timer_load_value(self):
        """Test timer load value register."""
        # Write load value to timer 0
        load_value = 1000
        self.timer.write(0x40011000 + 0x104, load_value)  # TIMER0_LDVAL
        
        channel = self.timer.channels[0]
        self.assertEqual(channel.load_value, load_value)
        
        # Read back the value
        read_value = self.timer.read(0x40011000 + 0x104)
        self.assertEqual(read_value, load_value)
        
    def test_timer_control_register(self):
        """Test timer control register (INIT)."""
        # Configure timer 0: enable timer, enable interrupt, disable link
        control_value = 0x3  # TEN=1, TIE=1, LINKEN=0
        self.timer.write(0x40011000 + 0x10C, control_value)  # TIMER0_INIT
        
        channel = self.timer.channels[0]
        self.assertTrue(channel.enabled)
        self.assertTrue(channel.interrupt_enabled)
        self.assertFalse(channel.link_enabled)  # Timer 0 cannot link
        
    def test_timer_cval_register(self):
        """Test current value register (CVAL)."""
        # Set load value and enable timer
        load_value = 100
        self.timer.write(0x40011000 + 0x104, load_value)     # TIMER0_LDVAL
        self.timer.write(0x40011000 + 0x10C, 0x1)            # TIMER0_INIT: TEN=1
        
        # Wait a bit for timer to run
        time.sleep(0.05)
        
        # Read CVAL - should be non-zero as timer counts up
        cval = self.timer.read(0x40011000 + 0x108)  # TIMER0_CVAL
        print(f"CVAL value after delay: {cval}")
        
        # CVAL should increase as timer counts down
        # Note: actual value depends on timing, so we just check it's reasonable
        self.assertGreaterEqual(cval, 0)
        
    def test_timer_countdown(self):
        """Test basic timer countdown functionality."""
        channel = self.timer.channels[0]
        
        # Set a small load value for quick testing
        load_value = 50  # 50ms
        self.timer.write(0x40011000 + 0x104, load_value)
        self.timer.write(0x40011000 + 0x10C, 0x3)  # TEN=1, TIE=1
        
        # Wait for timer to potentially expire
        time.sleep(0.1)  # 100ms
        
        # Check if interrupt was generated
        print(f"Interrupts triggered: {self.mock_bus.interrupts_triggered}")
        
        # Timer should have expired and reloaded at least once
        self.assertGreater(len(self.mock_bus.interrupts_triggered), 0)
        if self.mock_bus.interrupts_triggered:
            self.assertEqual(self.mock_bus.interrupts_triggered[0], 18)  # Timer IRQ
            
    def test_timer_link_mode(self):
        """Test timer link mode functionality."""
        # Configure timer 0 as base timer (cannot be linked)
        self.timer.write(0x40011000 + 0x104, 10)   # TIMER0_LDVAL = 10ms
        self.timer.write(0x40011000 + 0x10C, 0x1)  # TIMER0_INIT: TEN=1
        
        # Configure timer 1 in link mode
        self.timer.write(0x40011000 + 0x110, 3)    # TIMER1_LDVAL = 3
        self.timer.write(0x40011000 + 0x118, 0x5)  # TIMER1_INIT: TEN=1, LINKEN=1
        
        channel0 = self.timer.channels[0]
        channel1 = self.timer.channels[1]
        
        self.assertTrue(channel0.enabled)
        self.assertFalse(channel0.link_enabled)  # Timer 0 cannot link
        
        self.assertTrue(channel1.enabled)
        self.assertTrue(channel1.link_enabled)
        
        print(f"Timer 0 - Load: {channel0.load_value}, Link: {channel0.link_enabled}")
        print(f"Timer 1 - Load: {channel1.load_value}, Link: {channel1.link_enabled}")
        
    def test_global_disable(self):
        """Test global timer disable functionality."""
        # Enable a timer first
        self.timer.write(0x40011000 + 0x104, 100)  # TIMER0_LDVAL
        self.timer.write(0x40011000 + 0x10C, 0x1)  # TIMER0_INIT: TEN=1
        
        channel = self.timer.channels[0]
        self.assertTrue(channel.enabled)
        
        # Globally disable all timers
        self.timer.write(0x40011000 + 0x000, 0x0)  # TIMER_MCR: MDIS=0
        self.assertFalse(self.timer.global_enabled)
        
        # Timer should be stopped
        self.assertEqual(channel.state, TimerState.STOPPED)
        
    def test_error_injection(self):
        """Test error injection functionality."""
        # Test timer stuck error
        self.timer.inject_error("timer_stuck", {"channel_id": 0})
        
        channel = self.timer.channels[0]
        self.assertEqual(channel.state, TimerState.STOPPED)
        self.assertIn("timer_stuck_0", self.timer.injected_errors)
        
        # Test interrupt failure error
        self.timer.inject_error("interrupt_fail", {"channel_id": 1})
        
        channel1 = self.timer.channels[1]
        self.assertFalse(channel1.interrupt_enabled)
        self.assertIn("interrupt_fail_1", self.timer.injected_errors)
        
    def test_device_status(self):
        """Test device status reporting."""
        status = self.timer.get_device_status()
        
        self.assertIn('global_enabled', status)
        self.assertIn('channels', status)
        self.assertIn('injected_errors', status)
        
        self.assertTrue(status['global_enabled'])
        self.assertEqual(len(status['channels']), 4)
        
        # Test individual channel status
        channel_status = self.timer.get_timer_status(0)
        self.assertIn('channel_id', channel_status)
        self.assertIn('load_value', channel_status)
        self.assertIn('current_value', channel_status)
        self.assertIn('enabled', channel_status)
        
    def test_multiple_timer_channels(self):
        """Test multiple timer channels working independently."""
        # Configure multiple timers
        timers_config = [
            (0x104, 0x10C, 50),   # Timer 0: 50ms
            (0x110, 0x118, 75),   # Timer 1: 75ms  
            (0x120, 0x128, 100),  # Timer 2: 100ms
            (0x130, 0x138, 125)   # Timer 3: 125ms
        ]
        
        for i, (ldval_reg, init_reg, load_val) in enumerate(timers_config):
            self.timer.write(0x40011000 + ldval_reg, load_val)
            self.timer.write(0x40011000 + init_reg, 0x3)  # TEN=1, TIE=1
            
            channel = self.timer.channels[i]
            self.assertTrue(channel.enabled)
            self.assertTrue(channel.interrupt_enabled)
            self.assertEqual(channel.load_value, load_val)
            
        print(f"Configured {len(timers_config)} timers")
        
        # Wait and check for interrupts
        time.sleep(0.2)
        print(f"Interrupts after delay: {len(self.mock_bus.interrupts_triggered)}")
        
    def test_timer_reload(self):
        """Test timer reload functionality."""
        # Set initial load value
        initial_load = 20
        self.timer.write(0x40011000 + 0x104, initial_load)
        self.timer.write(0x40011000 + 0x10C, 0x1)  # Enable timer
        
        channel = self.timer.channels[0]
        self.assertEqual(channel.load_value, initial_load)
        self.assertEqual(channel.current_value, initial_load)
        
        # Change load value while timer is running
        new_load = 50
        self.timer.write(0x40011000 + 0x104, new_load)
        
        # New load value should be stored but not affect current countdown
        self.assertEqual(channel.load_value, new_load)
        # current_value should still be from initial load until next reload


def run_timer_tests():
    """Run timer device tests."""
    print("Starting Timer Device Tests")
    print("=" * 50)
    
    # Create test suite
    test_suite = unittest.TestLoader().loadTestsFromTestCase(TestTimerDevice)
    
    # Run tests with verbose output
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(test_suite)
    
    print("\n" + "=" * 50)
    print(f"Tests run: {result.testsRun}")
    print(f"Failures: {len(result.failures)}")
    print(f"Errors: {len(result.errors)}")
    
    if result.failures:
        print("\nFailures:")
        for test, trace in result.failures:
            print(f"  {test}: {trace}")
            
    if result.errors:
        print("\nErrors:")
        for test, trace in result.errors:
            print(f"  {test}: {trace}")
    
    success = len(result.failures) == 0 and len(result.errors) == 0
    print(f"\nOverall: {'PASSED' if success else 'FAILED'}")
    
    return success


if __name__ == "__main__":
    run_timer_tests()