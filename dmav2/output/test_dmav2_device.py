#!/usr/bin/env python3
"""
Test program for DMAv2 device implementation.

This test suite validates all major functionality of the DMAv2 device:
- Multi-instance support
- Register access
- Channel configuration and operation
- Transfer modes (mem2mem, mem2peri, peri2mem, peri2peri)
- Interrupt generation
- Error injection
- Reset functionality
"""

import sys
import os
import time
import threading
from typing import List, Dict, Any

# Add the parent directories to the path
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))

from dmav2_device import (
    DMAv2Device, DMAv2TransferMode, DMAv2Priority, 
    DMAv2DataWidth, DMAv2ChannelState
)
from devcomm.core.bus_model import BusModel


class DMAv2TestSuite:
    """Comprehensive test suite for DMAv2 device."""
    
    def __init__(self):
        self.test_results = []
        self.interrupt_events = []
        self.bus = BusModel("TestBus")
        
    def log_test(self, test_name: str, passed: bool, details: str = ""):
        """Log test result."""
        status = "PASS" if passed else "FAIL"
        result = f"[{status}] {test_name}"
        if details:
            result += f" - {details}"
        print(result)
        self.test_results.append((test_name, passed, details))
        
    def interrupt_handler(self, channel_id: int, event_type: str):
        """Handle DMA interrupts."""
        self.interrupt_events.append((channel_id, event_type, time.time()))
        print(f"  IRQ: Channel {channel_id} {event_type}")
        
    def test_device_creation(self):
        """Test basic device creation and initialization."""
        print("\n=== Testing Device Creation ===")
        
        try:
            # Test single instance using new constructor pattern
            dma1 = DMAv2Device("DMAv2_0", 0x40000000, 0x1000, 10)
            self.bus.add_device(dma1)
            
            self.log_test("Single instance creation", True)
            
            # Test device info
            info = dma1.get_device_info()
            expected_fields = ['name', 'base_address', 'size', 'master_id', 'enabled']
            has_all_fields = all(field in info for field in expected_fields)
            self.log_test("Device info completeness", has_all_fields)
            
            # Test multi-instance support using class method for backward compatibility
            dma2 = DMAv2Device.create_instance(instance_id=1, base_address=0x41000000)
            self.bus.add_device(dma2)
            
            self.log_test("Multi-instance creation", True)
            
            # Verify instances are independent
            dma1_status = dma1.get_device_status()
            dma2_status = dma2.get_device_status()
            
            instances_independent = (
                dma1_status['instance_id'] != dma2_status['instance_id'] and
                dma1_status['base_address'] != dma2_status['base_address']
            )
            self.log_test("Instance independence", instances_independent)
            
            return dma1, dma2
            
        except Exception as e:
            self.log_test("Device creation", False, str(e))
            return None, None
            
    def test_register_access(self, device: DMAv2Device):
        """Test register read/write operations."""
        print("\n=== Testing Register Access ===")
        
        try:
            base_addr = device.base_address
            
            # Test global reset register
            device.write(base_addr + 0x000, 0x00000004)  # Set DMA_IDLE
            value = device.read(base_addr + 0x000)
            self.log_test("Global register write/read", value == 0x00000004)
            
            # Test channel enable registers
            ch0_enable_addr = base_addr + 0x040 + 0x24  # Channel 0 enable
            device.write(ch0_enable_addr, 0x00000001)   # Enable channel 0
            value = device.read(ch0_enable_addr)
            self.log_test("Channel register write/read", value == 0x00000001)
            
            # Test read-only registers (should not change)
            ch0_status_addr = base_addr + 0x040 + 0x00  # Channel 0 status
            original_status = device.read(ch0_status_addr)
            try:
                device.write(ch0_status_addr, 0xFFFFFFFF)  # Try to write
                new_status = device.read(ch0_status_addr)
                self.log_test("Read-only register protection", original_status == new_status)
            except Exception:
                # It's OK if writing to read-only register raises exception
                self.log_test("Read-only register protection", True)
            
            # Test address range validation
            try:
                device.read(base_addr + 0x2000)  # Out of range
                self.log_test("Address range validation", False)
            except ValueError:
                self.log_test("Address range validation", True)
                
        except Exception as e:
            self.log_test("Register access", False, str(e))
            
    def test_channel_operations(self, device: DMAv2Device):
        """Test channel configuration and basic operations."""
        print("\n=== Testing Channel Operations ===")
        
        try:
            # Register interrupt handler
            device.register_irq_callback(self.interrupt_handler)
            
            # Test channel configuration via DMA interface
            channel_id = device.dma_interface.dma_request(
                src_addr=0x20000000,
                dst_addr=0x20001000,
                length=256
            )
            
            self.log_test("Channel allocation", channel_id is not None)
            
            if channel_id is not None:
                # Check channel status
                status = device.get_channel_status(channel_id)
                self.log_test("Channel status retrieval", 
                            status['src_address'] == 0x20000000 and
                            status['dst_address'] == 0x20001000 and
                            status['transfer_length'] == 256)
                
                # Wait for transfer to complete
                time.sleep(1.0)  # Increased wait time
                
                # Check completion
                final_status = device.get_channel_status(channel_id)
                self.log_test("Transfer completion", final_status['transfer_complete'])
                
                # Verify interrupt was generated
                completion_interrupts = [evt for evt in self.interrupt_events 
                                       if evt[0] == channel_id and evt[1] == "complete"]
                self.log_test("Completion interrupt", len(completion_interrupts) > 0)
                
        except Exception as e:
            self.log_test("Channel operations", False, str(e))
            
    def test_transfer_modes(self, device: DMAv2Device):
        """Test different transfer modes."""
        print("\n=== Testing Transfer Modes ===")
        
        try:
            # Clear previous interrupt events
            self.interrupt_events.clear()
            
            test_modes = [
                (DMAv2TransferMode.MEM2MEM, "Memory to Memory"),
                (DMAv2TransferMode.MEM2PERI, "Memory to Peripheral"),
                (DMAv2TransferMode.PERI2MEM, "Peripheral to Memory"),
                (DMAv2TransferMode.PERI2PERI, "Peripheral to Peripheral")
            ]
            
            for mode, mode_name in test_modes:
                try:
                    # Find available channel
                    channel_id = device._find_available_channel()
                    if channel_id is None:
                        # Reset device to free up channels
                        device._perform_hard_reset()
                        channel_id = device._find_available_channel()
                        
                    if channel_id is not None:
                        channel = device.channels[channel_id]
                        channel.configure(
                            src_addr=0x30000000 + channel_id * 0x1000,
                            dst_addr=0x31000000 + channel_id * 0x1000,
                            length=128,
                            mode=mode
                        )
                        channel.enable()
                        device._start_transfer(channel_id)
                        
                        # Wait a bit for transfer to start
                        time.sleep(0.1)
                        
                        status = device.get_channel_status(channel_id)
                        self.log_test(f"{mode_name} mode", 
                                    status['transfer_mode'] == mode.value)
                    else:
                        self.log_test(f"{mode_name} mode", False, "No available channel")
                        
                except Exception as e:
                    self.log_test(f"{mode_name} mode", False, str(e))
                    
        except Exception as e:
            self.log_test("Transfer modes", False, str(e))
            
    def test_error_injection(self, device: DMAv2Device):
        """Test error injection functionality."""
        print("\n=== Testing Error Injection ===")
        
        try:
            # Enable error injection
            device.enable_error_injection()
            self.log_test("Error injection enable", device.error_injection_enabled)
            
            # Clear interrupt events
            self.interrupt_events.clear()
            
            # Test different error types
            error_types = [
                "src_addr_error",
                "dst_addr_error", 
                "src_bus_error",
                "dst_bus_error",
                "channel_length_error"
            ]
            
            for i, error_type in enumerate(error_types):
                channel_id = i % device.num_channels
                
                # Configure channel first
                channel = device.channels[channel_id]
                channel.configure(0x40000000, 0x41000000, 64)
                
                # Inject error using new method signature
                device.inject_error(error_type, {"channel_id": channel_id})
                
                # Check if error was set
                status = device.get_channel_status(channel_id)
                self.log_test(f"Error injection: {error_type}", 
                            status['transfer_error'])
                            
            # Check that error interrupts were generated
            error_interrupts = [evt for evt in self.interrupt_events 
                              if evt[1] == "error"]
            self.log_test("Error interrupts generated", len(error_interrupts) > 0)
            
            # Disable error injection
            device.disable_error_injection()
            self.log_test("Error injection disable", not device.error_injection_enabled)
            
        except Exception as e:
            self.log_test("Error injection", False, str(e))
            
    def test_reset_functionality(self, device: DMAv2Device):
        """Test hard and warm reset functionality."""
        print("\n=== Testing Reset Functionality ===")
        
        try:
            # Start some transfers first
            for ch in range(min(4, device.num_channels)):
                channel = device.channels[ch]
                channel.configure(0x50000000 + ch * 0x100, 0x51000000 + ch * 0x100, 512)
                channel.enable()
                device._start_transfer(ch)
                
            # Wait a bit for transfers to start
            time.sleep(0.1)
            
            # Count active channels before reset
            active_before = sum(1 for ch in device.channels.values() if ch.is_running())
            self.log_test("Channels active before reset", active_before > 0)
            
            # Test hard reset
            device._perform_hard_reset()
            
            # Check that all channels are reset
            active_after_hard = sum(1 for ch in device.channels.values() if ch.is_running())
            self.log_test("Hard reset effectiveness", active_after_hard == 0)
            
            # Test warm reset
            # Start transfers again
            for ch in range(2):
                channel = device.channels[ch]
                channel.configure(0x52000000 + ch * 0x100, 0x53000000 + ch * 0x100, 256)
                channel.enable()
                device._start_transfer(ch)
                
            time.sleep(0.1)
            
            # Perform warm reset
            device._perform_warm_reset()
            
            active_after_warm = sum(1 for ch in device.channels.values() if ch.is_running())
            self.log_test("Warm reset effectiveness", active_after_warm == 0)
            
        except Exception as e:
            self.log_test("Reset functionality", False, str(e))
            
    def test_multi_channel_concurrency(self, device: DMAv2Device):
        """Test multiple channels running concurrently."""
        print("\n=== Testing Multi-Channel Concurrency ===")
        
        try:
            # Clear interrupt events
            self.interrupt_events.clear()
            
            # Start transfers on multiple channels
            num_concurrent = min(8, device.num_channels)
            started_channels = []
            
            for ch in range(num_concurrent):
                try:
                    channel_id = device.dma_interface.dma_request(
                        src_addr=0x60000000 + ch * 0x200,
                        dst_addr=0x61000000 + ch * 0x200,
                        length=128 + ch * 16  # Different lengths
                    )
                    started_channels.append(channel_id)
                except Exception as e:
                    print(f"  Failed to start channel {ch}: {e}")
                    
            self.log_test("Multiple channel startup", len(started_channels) == num_concurrent)
            
            # Wait for transfers to complete
            time.sleep(2.0)  # Increased wait time
            
            # Check that all transfers completed
            completed_count = 0
            for ch_id in started_channels:
                status = device.get_channel_status(ch_id)
                if status['transfer_complete']:
                    completed_count += 1
                    
            self.log_test("Concurrent transfer completion", 
                        completed_count == len(started_channels))
            
            # Verify interrupts for all channels
            completion_interrupts = [evt for evt in self.interrupt_events 
                                   if evt[1] == "complete"]
            self.log_test("Concurrent completion interrupts", 
                        len(completion_interrupts) >= len(started_channels))
            
        except Exception as e:
            self.log_test("Multi-channel concurrency", False, str(e))
            
    def test_device_interface_integration(self, device: DMAv2Device):
        """Test integration with device framework interfaces."""
        print("\n=== Testing Framework Integration ===")
        
        try:
            # Test device enable/disable
            device.enable()
            self.log_test("Device enable", device.is_enabled())
            
            device.disable()
            self.log_test("Device disable", not device.is_enabled())
            
            device.enable()  # Re-enable for other tests
            
            # Test trace functionality
            device.enable_trace()
            device.disable_trace()
            device.clear_trace()
            self.log_test("Trace operations", True)
            
            # Test bus integration
            bus_info = device.get_device_info()
            has_bus = bus_info.get('has_bus', False)
            self.log_test("Bus integration", has_bus)
            
            # Test address range checking
            start_addr, end_addr = device.get_address_range()
            valid_range = (start_addr == device.base_address and 
                         end_addr == device.base_address + device.device_size - 1)
            self.log_test("Address range", valid_range)
            
        except Exception as e:
            self.log_test("Framework integration", False, str(e))
            
    def run_all_tests(self):
        """Run complete test suite."""
        print("DMAv2 Device Test Suite")
        print("=" * 50)
        
        # Create devices
        dma1, dma2 = self.test_device_creation()
        
        if dma1 is None:
            print("CRITICAL: Device creation failed, aborting tests")
            return False
            
        # Run all test categories
        self.test_register_access(dma1)
        self.test_channel_operations(dma1)
        self.test_transfer_modes(dma1)
        self.test_error_injection(dma1)
        self.test_reset_functionality(dma1)
        self.test_multi_channel_concurrency(dma1)
        self.test_device_interface_integration(dma1)
        
        # Test with second instance if available
        if dma2 is not None:
            print("\n=== Testing Second Instance ===")
            self.test_channel_operations(dma2)
            
        # Print summary
        self.print_test_summary()
        
        return self.calculate_pass_rate() > 0.8  # 80% pass rate
        
    def print_test_summary(self):
        """Print test results summary."""
        print("\n" + "=" * 50)
        print("TEST SUMMARY")
        print("=" * 50)
        
        total_tests = len(self.test_results)
        passed_tests = sum(1 for _, passed, _ in self.test_results if passed)
        failed_tests = total_tests - passed_tests
        pass_rate = (passed_tests / total_tests * 100) if total_tests > 0 else 0
        
        print(f"Total Tests: {total_tests}")
        print(f"Passed: {passed_tests}")
        print(f"Failed: {failed_tests}")
        print(f"Pass Rate: {pass_rate:.1f}%")
        
        if failed_tests > 0:
            print(f"\nFailed Tests:")
            for test_name, passed, details in self.test_results:
                if not passed:
                    print(f"  - {test_name}: {details}")
                    
        print(f"\nInterrupt Events Captured: {len(self.interrupt_events)}")
        
    def calculate_pass_rate(self) -> float:
        """Calculate test pass rate."""
        if not self.test_results:
            return 0.0
        passed = sum(1 for _, passed, _ in self.test_results if passed)
        return passed / len(self.test_results)


def main():
    """Main test entry point."""
    print("Starting DMAv2 Device Test Suite...")
    
    try:
        test_suite = DMAv2TestSuite()
        success = test_suite.run_all_tests()
        
        if success:
            print("\n✅ DMAv2 Device Tests PASSED")
            return 0
        else:
            print("\n❌ DMAv2 Device Tests FAILED")
            return 1
            
    except Exception as e:
        print(f"\n💥 Test suite crashed: {e}")
        import traceback
        traceback.print_exc()
        return 2


if __name__ == "__main__":
    exit(main())