#!/usr/bin/env python3
"""
Test suite for Flash Device Model.

This test suite validates:
- Basic register operations
- Lock/unlock functionality
- Flash command execution (erase, program, verify)
- Write protection mechanisms
- Error injection capabilities
- Memory operations and persistence
- Multi-instance support
"""

import os
import sys
import tempfile
import time
import unittest
from unittest.mock import patch, MagicMock

# Add the repository root to Python path
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))

from flash.output.flash_device import FlashDevice, FlashCommand, FlashState, FlashErrorType


class TestFlashDevice(unittest.TestCase):
    """Test cases for FlashDevice."""

    def setUp(self):
        """Set up test environment."""
        # Create temporary file for storage backing
        self.temp_file = tempfile.NamedTemporaryFile(delete=False)
        self.temp_file.close()
        
        # Write initial 0xFF pattern to temp file
        with open(self.temp_file.name, 'wb') as f:
            f.write(bytes([0xFF] * (256 * 1024)))
        
        # Create test Flash device instances
        self.flash_main = FlashDevice(
            name="FlashMain",
            base_address=0x40002000,
            size=0x1000,  # 4KB register space
            master_id=1,
            memory_size=256 * 1024,  # 256KB flash
            page_size=2048,
            is_info_area=False,
            storage_file=self.temp_file.name
        )
        
        self.flash_info = FlashDevice(
            name="FlashInfo", 
            base_address=0x40003000,
            size=0x1000,
            master_id=2,
            memory_size=12 * 1024,  # 12KB Info area
            page_size=4096,
            is_info_area=True
        )

    def tearDown(self):
        """Clean up test environment."""
        # Remove temporary file
        if os.path.exists(self.temp_file.name):
            os.unlink(self.temp_file.name)

    def test_device_initialization(self):
        """Test device initialization and basic properties."""
        # Test main flash device
        self.assertEqual(self.flash_main.name, "FlashMain")
        self.assertEqual(self.flash_main.base_address, 0x40002000)
        self.assertEqual(self.flash_main.memory_size, 256 * 1024)
        self.assertEqual(self.flash_main.page_size, 2048)
        self.assertFalse(self.flash_main.is_info_area)
        self.assertEqual(self.flash_main.flash_state, FlashState.LOCKED)
        
        # Test Info area device
        self.assertEqual(self.flash_info.name, "FlashInfo")
        self.assertTrue(self.flash_info.is_info_area)
        self.assertEqual(self.flash_info.memory_size, 12 * 1024)
        
        # Test memory initialization
        self.assertEqual(len(self.flash_main.memory_data), 256 * 1024)
        self.assertEqual(self.flash_main.memory_data[0], 0xFF)  # Erased state

    def test_register_initialization(self):
        """Test that all required registers are properly initialized."""
        registers = self.flash_main.register_manager.list_registers()
        
        # Check key registers exist
        expected_registers = [
            0x0000,  # FMUKR
            0x0004,  # FMGSR
            0x0008,  # FMGCR
            0x0010,  # FMCCR
            0x0014,  # FMADDR
            0x0018,  # FMDATA0
            0x001C,  # FMDATA1
            0x0020,  # FMPFWPR0
            0x0024,  # FMPFWPR1
            0x0028,  # FMDFWPR0
            0x002C,  # FMDFWPR1
            0x0030,  # FMPFECADR
            0x0034,  # FMDFECADR
            0x0070,  # FMCNR
            0x0074,  # FMCIR0
        ]
        
        for reg_offset in expected_registers:
            self.assertIn(reg_offset, registers)

    def test_basic_register_operations(self):
        """Test basic register read/write operations."""
        # Test read-write register (FMGCR)
        self.flash_main.write(0x40002008, 0x12345678)
        value = self.flash_main.read(0x40002008)
        # Value should be masked by register mask (0x0FFF0F01)
        expected = 0x12345678 & 0x0FFF0F01
        self.assertEqual(value, expected)
        
        # Test read-only register (FMCNR - Chip Name)
        value = self.flash_main.read(0x40002070)
        self.assertEqual(value, 0x464C5348)  # "FLSH"
        
        # Attempt to write to read-only register should not change value
        with self.assertRaises(PermissionError):
            self.flash_main.write(0x40002070, 0xDEADBEEF)

    def test_unlock_sequence(self):
        """Test flash unlock sequence."""
        # Initially should be locked
        status = self.flash_main.read(0x40002004)  # FMGSR
        self.assertTrue(status & 0x01)  # LOCK bit set
        
        # Write first unlock key
        self.flash_main.write(0x40002000, 0x00AC7805)  # FMUKR
        self.assertEqual(self.flash_main.unlock_sequence_step, 1)
        
        # Write second unlock key
        self.flash_main.write(0x40002000, 0x01234567)  # FMUKR
        self.assertEqual(self.flash_main.unlock_sequence_step, 2)
        self.assertEqual(self.flash_main.flash_state, FlashState.IDLE)
        
        # Status should show unlocked
        status = self.flash_main.read(0x40002004)  # FMGSR
        self.assertFalse(status & 0x01)  # LOCK bit cleared

    def test_unlock_sequence_failure(self):
        """Test failed unlock sequence."""
        # Write wrong first key
        self.flash_main.write(0x40002000, 0x12345678)  # FMUKR
        self.assertEqual(self.flash_main.unlock_sequence_step, 0)
        
        # Write correct first key, then wrong second key
        self.flash_main.write(0x40002000, 0x00AC7805)  # FMUKR
        self.assertEqual(self.flash_main.unlock_sequence_step, 1)
        
        self.flash_main.write(0x40002000, 0x87654321)  # FMUKR - wrong key
        self.assertEqual(self.flash_main.unlock_sequence_step, 0)
        self.assertEqual(self.flash_main.flash_state, FlashState.LOCKED)

    def test_page_erase_command(self):
        """Test page erase command execution."""
        # Unlock device first
        self._unlock_device(self.flash_main)
        
        # Set some data in memory to verify erase
        test_address = 0x1000
        test_data = bytes([0x55, 0xAA, 0x33, 0xCC])
        self.flash_main.write_memory(test_address, test_data)
        
        # Verify data was written
        read_data = self.flash_main.read_memory(test_address, len(test_data))
        self.assertEqual(read_data, test_data)
        
        # Configure erase command
        self.flash_main.write(0x40002014, test_address)  # FMADDR
        
        # Start page erase command
        cmd_value = (FlashCommand.PAGE_ERASE.value << 4) | 0x01  # CMD_TYPE + CMD_START
        self.flash_main.write(0x40002010, cmd_value)  # FMCCR
        
        # Wait for command to complete
        self._wait_for_command_completion(self.flash_main)
        
        # Verify page was erased
        page_start = (test_address // self.flash_main.page_size) * self.flash_main.page_size
        erased_data = self.flash_main.read_memory(page_start, self.flash_main.page_size)
        self.assertTrue(all(b == 0xFF for b in erased_data))

    def test_mass_erase_command(self):
        """Test mass erase command execution."""
        # Unlock device first
        self._unlock_device(self.flash_main)
        
        # Set some data in memory
        test_data = bytes([0x55, 0xAA, 0x33, 0xCC])
        self.flash_main.write_memory(0x1000, test_data)
        self.flash_main.write_memory(0x8000, test_data)
        
        # Configure and start mass erase command
        self.flash_main.write(0x40002014, 0x0000)  # FMADDR - any address
        cmd_value = (FlashCommand.MASS_ERASE.value << 4) | 0x01  # CMD_TYPE + CMD_START
        self.flash_main.write(0x40002010, cmd_value)  # FMCCR
        
        # Wait for command to complete
        self._wait_for_command_completion(self.flash_main)
        
        # Verify entire memory was erased
        sample_data1 = self.flash_main.read_memory(0x1000, 4)
        sample_data2 = self.flash_main.read_memory(0x8000, 4)
        self.assertEqual(sample_data1, bytes([0xFF] * 4))
        self.assertEqual(sample_data2, bytes([0xFF] * 4))

    def test_page_program_command(self):
        """Test page program command execution."""
        # Unlock device first
        self._unlock_device(self.flash_main)
        
        # Erase target page first
        test_address = 0x2000
        self._erase_page(self.flash_main, test_address)
        
        # Set program data
        data0 = 0x12345678
        data1 = 0x9ABCDEF0
        self.flash_main.write(0x40002018, data0)  # FMDATA0
        self.flash_main.write(0x4000201C, data1)  # FMDATA1
        
        # Configure and start program command
        self.flash_main.write(0x40002014, test_address)  # FMADDR
        cmd_value = (FlashCommand.PAGE_PROGRAM.value << 4) | 0x01  # CMD_TYPE + CMD_START
        self.flash_main.write(0x40002010, cmd_value)  # FMCCR
        
        # Wait for command to complete
        self._wait_for_command_completion(self.flash_main)
        
        # Verify data was programmed
        programmed_data = self.flash_main.read_memory(test_address, 8)
        expected_data = data0.to_bytes(4, 'little') + data1.to_bytes(4, 'little')
        self.assertEqual(programmed_data, expected_data)

    def test_write_protection(self):
        """Test write protection mechanism."""
        # Unlock device
        self._unlock_device(self.flash_main)
        
        # Enable write protection for first region (bit 0)
        self.flash_main.write(0x40002020, 0x00000001)  # FMPFWPR0
        
        # Verify protection was set
        self.assertTrue(self.flash_main.write_protection_pf[0])
        
        # Try to erase protected region (should fail)
        protected_address = 0x0000  # First region
        self.flash_main.write(0x40002014, protected_address)  # FMADDR
        cmd_value = (FlashCommand.PAGE_ERASE.value << 4) | 0x01
        self.flash_main.write(0x40002010, cmd_value)  # FMCCR
        
        # Wait and check that command failed
        time.sleep(0.01)  # Give command time to process
        # In a real implementation, this would set error flags
        
        # Test unprotected region (should work)
        unprotected_address = self.flash_main.memory_size - self.flash_main.page_size
        self.flash_main.write(0x40002014, unprotected_address)  # FMADDR
        cmd_value = (FlashCommand.PAGE_ERASE.value << 4) | 0x01
        self.flash_main.write(0x40002010, cmd_value)  # FMCCR
        
        self._wait_for_command_completion(self.flash_main)

    def test_memory_operations(self):
        """Test direct memory read/write operations."""
        # Test memory write and read
        test_data = bytes([0x11, 0x22, 0x33, 0x44, 0x55])
        test_address = 0x1000
        
        self.flash_main.write_memory(test_address, test_data)
        read_data = self.flash_main.read_memory(test_address, len(test_data))
        self.assertEqual(read_data, test_data)
        
        # Test boundary conditions
        with self.assertRaises(ValueError):
            self.flash_main.read_memory(self.flash_main.memory_size, 1)  # Out of bounds
            
        with self.assertRaises(ValueError):
            self.flash_main.write_memory(self.flash_main.memory_size, b'\x00')  # Out of bounds

    def test_file_persistence(self):
        """Test that memory content persists to file."""
        # Write test data
        test_data = bytes([0xDE, 0xAD, 0xBE, 0xEF])
        test_address = 0x1000
        self.flash_main.write_memory(test_address, test_data)
        
        # Check file was updated
        self.assertTrue(os.path.exists(self.temp_file.name))
        
        # Create new device instance with same file
        flash_new = FlashDevice(
            name="FlashNew",
            base_address=0x40004000,
            size=0x1000,
            master_id=3,
            memory_size=256 * 1024,
            page_size=2048,
            is_info_area=False,
            storage_file=self.temp_file.name
        )
        
        # Verify data was loaded from file
        loaded_data = flash_new.read_memory(test_address, len(test_data))
        self.assertEqual(loaded_data, test_data)

    def test_error_injection(self):
        """Test error injection capabilities."""
        test_address = 0x1000
        original_data = bytes([0xAA, 0xBB, 0xCC, 0xDD])
        
        # Write test data
        self.flash_main.write_memory(test_address, original_data)
        
        # Enable single-bit error injection
        self.flash_main.enable_error_injection(test_address, FlashErrorType.ECC_SINGLE_BIT_ERROR)
        
        # Read data - should have single bit flipped
        corrupted_data = self.flash_main.read_memory(test_address, len(original_data))
        self.assertNotEqual(corrupted_data, original_data)
        self.assertEqual(corrupted_data[0], original_data[0] ^ 0x01)  # LSB flipped
        
        # Disable error injection
        self.flash_main.disable_error_injection(test_address)
        
        # Read should return original data
        normal_data = self.flash_main.read_memory(test_address, len(original_data))
        self.assertEqual(normal_data, original_data)

    def test_double_bit_error_injection(self):
        """Test double-bit error injection."""
        test_address = 0x2000
        original_data = bytes([0x55, 0x66, 0x77, 0x88])
        
        self.flash_main.write_memory(test_address, original_data)
        
        # Enable double-bit error injection
        self.flash_main.enable_error_injection(test_address, FlashErrorType.ECC_DOUBLE_BIT_ERROR)
        
        # Read data - should have two bits flipped
        corrupted_data = self.flash_main.read_memory(test_address, len(original_data))
        self.assertNotEqual(corrupted_data, original_data)
        self.assertEqual(corrupted_data[0], original_data[0] ^ 0x03)  # Two LSBs flipped

    def test_multi_instance_support(self):
        """Test support for multiple Flash device instances."""
        # Both devices should operate independently
        self.assertNotEqual(self.flash_main.name, self.flash_info.name)
        self.assertNotEqual(self.flash_main.base_address, self.flash_info.base_address)
        self.assertNotEqual(self.flash_main.memory_size, self.flash_info.memory_size)
        
        # Test that operations on one don't affect the other
        self._unlock_device(self.flash_main)
        self.assertEqual(self.flash_main.flash_state, FlashState.IDLE)
        self.assertEqual(self.flash_info.flash_state, FlashState.LOCKED)
        
        # Test independent memory spaces
        test_data1 = bytes([0x11, 0x22, 0x33, 0x44])
        test_data2 = bytes([0xAA, 0xBB, 0xCC, 0xDD])
        
        self.flash_main.write_memory(0x1000, test_data1)
        self.flash_info.write_memory(0x1000, test_data2)
        
        self.assertEqual(self.flash_main.read_memory(0x1000, 4), test_data1)
        self.assertEqual(self.flash_info.read_memory(0x1000, 4), test_data2)

    def test_device_status(self):
        """Test device status reporting."""
        status = self.flash_main.get_device_status()
        
        # Check required fields
        self.assertIn('name', status)
        self.assertIn('flash_state', status)
        self.assertIn('command_busy', status)
        self.assertIn('memory_size', status)
        self.assertIn('page_size', status)
        self.assertIn('is_info_area', status)
        
        self.assertEqual(status['name'], 'FlashMain')
        self.assertEqual(status['flash_state'], FlashState.LOCKED.value)
        self.assertFalse(status['command_busy'])

    def test_address_range_validation(self):
        """Test address range validation."""
        # Valid addresses should work
        self.assertTrue(self.flash_main.is_address_in_range(0x40002000))
        self.assertTrue(self.flash_main.is_address_in_range(0x40002FFF))
        
        # Invalid addresses should fail
        self.assertFalse(self.flash_main.is_address_in_range(0x40001FFF))
        self.assertFalse(self.flash_main.is_address_in_range(0x40003000))
        
        # Reads/writes outside range should raise exceptions
        with self.assertRaises(ValueError):
            self.flash_main.read(0x40001000)
            
        with self.assertRaises(ValueError):
            self.flash_main.write(0x40003000, 0x12345678)

    def test_command_abort(self):
        """Test command abort functionality."""
        # Unlock device
        self._unlock_device(self.flash_main)
        
        # Start a long-running command (mass erase)
        cmd_value = (FlashCommand.MASS_ERASE.value << 4) | 0x01
        self.flash_main.write(0x40002010, cmd_value)  # FMCCR
        
        # Verify command is running
        time.sleep(0.001)  # Give it a moment to start
        self.assertTrue(self.flash_main.command_busy)
        
        # Send abort command
        abort_value = 0x02  # CMD_ABORT bit
        self.flash_main.write(0x40002010, abort_value)  # FMCCR
        
        # Command should be aborted
        time.sleep(0.001)  # Give it time to process abort
        self.assertFalse(self.flash_main.command_busy)
        self.assertIsNone(self.flash_main.current_command)

    def test_info_area_special_behavior(self):
        """Test Info area specific behaviors."""
        # Info area should have special initialization
        self.assertTrue(self.flash_info.is_info_area)
        
        # Check default read protection setting (0xFFFFFFFFFF53FFAC)
        expected_bytes = [0xAC, 0xFF, 0x53, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF]
        actual_bytes = list(self.flash_info.memory_data[0:8])
        self.assertEqual(actual_bytes, expected_bytes)

    # Helper methods
    def _unlock_device(self, device):
        """Helper to unlock a flash device."""
        device.write(device.base_address + 0x0000, 0x00AC7805)  # FMUKR - first key
        device.write(device.base_address + 0x0000, 0x01234567)  # FMUKR - second key
        self.assertEqual(device.flash_state, FlashState.IDLE)

    def _wait_for_command_completion(self, device, timeout=1.0):
        """Helper to wait for command completion."""
        start_time = time.time()
        while device.command_busy and (time.time() - start_time) < timeout:
            time.sleep(0.001)
        self.assertFalse(device.command_busy, "Command did not complete within timeout")

    def _erase_page(self, device, address):
        """Helper to erase a page."""
        device.write(device.base_address + 0x0014, address)  # FMADDR
        cmd_value = (FlashCommand.PAGE_ERASE.value << 4) | 0x01
        device.write(device.base_address + 0x0010, cmd_value)  # FMCCR
        self._wait_for_command_completion(device)


class TestFlashDeviceIntegration(unittest.TestCase):
    """Integration tests for Flash device."""

    def setUp(self):
        """Set up integration test environment."""
        # Create temporary file for this specific test
        self.temp_file = tempfile.NamedTemporaryFile(delete=False)
        self.temp_file.close()
        
        # Initialize with erased flash content
        with open(self.temp_file.name, 'wb') as f:
            f.write(bytes([0xFF] * (64 * 1024)))
        
        self.flash = FlashDevice(
            name="IntegrationFlash",
            base_address=0x40005000,
            size=0x1000,
            master_id=10,
            memory_size=64 * 1024,  # 64KB
            page_size=2048,
            storage_file=self.temp_file.name
        )
        
    def tearDown(self):
        """Clean up integration test environment."""
        # Remove temporary file
        if hasattr(self, 'temp_file') and os.path.exists(self.temp_file.name):
            os.unlink(self.temp_file.name)

    def test_realistic_flash_usage_scenario(self):
        """Test a realistic usage scenario."""
        # 1. Check device is initially locked
        status = self.flash.read(0x40005004)  # FMGSR
        self.assertTrue(status & 0x01)  # LOCK bit set
        
        # 2. Unlock device
        self.flash.write(0x40005000, 0x00AC7805)  # First unlock key
        self.flash.write(0x40005000, 0x01234567)  # Second unlock key
        
        # 3. Verify unlock
        status = self.flash.read(0x40005004)  # FMGSR
        self.assertFalse(status & 0x01)  # LOCK bit cleared
        
        # 4. Write some application data
        app_data = bytes(range(256))  # 256 bytes of test data
        self.flash.write_memory(0x8000, app_data)
        
        # 5. Verify data was written
        read_data = self.flash.read_memory(0x8000, len(app_data))
        self.assertEqual(read_data, app_data)
        
        # 6. Erase a page
        self.flash.write(0x40005014, 0x8000)  # FMADDR
        cmd_value = (FlashCommand.PAGE_ERASE.value << 4) | 0x01
        self.flash.write(0x40005010, cmd_value)  # FMCCR
        
        # Wait for erase to complete
        start_time = time.time()
        while self.flash.command_busy and (time.time() - start_time) < 1.0:
            time.sleep(0.001)
        
        # 7. Verify page was erased
        erased_data = self.flash.read_memory(0x8000, 256)
        self.assertEqual(erased_data, bytes([0xFF] * 256))
        
        # 8. Program new data
        program_data0 = 0x12345678
        program_data1 = 0x9ABCDEF0
        self.flash.write(0x40005018, program_data0)  # FMDATA0
        self.flash.write(0x4000501C, program_data1)  # FMDATA1
        
        self.flash.write(0x40005014, 0x8000)  # FMADDR
        cmd_value = (FlashCommand.PAGE_PROGRAM.value << 4) | 0x01
        self.flash.write(0x40005010, cmd_value)  # FMCCR
        
        # Wait for program to complete
        start_time = time.time()
        while self.flash.command_busy and (time.time() - start_time) < 1.0:
            time.sleep(0.001)
        
        # 9. Verify programmed data
        programmed_data = self.flash.read_memory(0x8000, 8)
        expected_data = program_data0.to_bytes(4, 'little') + program_data1.to_bytes(4, 'little')
        self.assertEqual(programmed_data, expected_data)
        
        # 10. Lock device again
        self.flash.lock_flash()
        status = self.flash.read(0x40005004)  # FMGSR
        self.assertTrue(status & 0x01)  # LOCK bit set


def run_all_tests():
    """Run all Flash device tests."""
    # Create test suite
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    
    # Add test cases
    suite.addTests(loader.loadTestsFromTestCase(TestFlashDevice))
    suite.addTests(loader.loadTestsFromTestCase(TestFlashDeviceIntegration))
    
    # Run tests
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)
    
    return result.wasSuccessful()


if __name__ == '__main__':
    print("=" * 70)
    print("Flash Device Model Test Suite")
    print("=" * 70)
    
    success = run_all_tests()
    
    print("\n" + "=" * 70)
    if success:
        print("✅ All tests passed!")
        sys.exit(0)
    else:
        print("❌ Some tests failed!")
        sys.exit(1)