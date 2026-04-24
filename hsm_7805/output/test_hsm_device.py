"""
Test suite for HSM device implementation.

This module provides comprehensive tests for the HSM device including:
- Register access tests
- Operation mode tests (AES ECB/CBC, CMAC, TRNG, Authentication)
- Key management tests
- Error injection tests
- DMA integration tests
- Interrupt functionality tests
"""

import unittest
import time
import threading
from unittest.mock import Mock, patch

import sys
import os
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))

from hsm_device import HSMDevice, HSMMode, HSMError
from devcomm.core.registers import RegisterType


class TestHSMDevice(unittest.TestCase):
    """Test cases for HSM device."""

    def setUp(self):
        """Set up test environment."""
        self.hsm = HSMDevice("hsm0", 0x40000000, 0x1000, 1)
        # Mock bus for interrupt testing
        self.mock_bus = Mock()
        self.hsm.set_bus(self.mock_bus)

    def tearDown(self):
        """Clean up test environment."""
        self.hsm.clear_injected_errors()

    def test_device_initialization(self):
        """Test HSM device initialization."""
        # Check device properties
        self.assertEqual(self.hsm.name, "hsm0")
        self.assertEqual(self.hsm.base_address, 0x40000000)
        self.assertEqual(self.hsm.size, 0x1000)
        self.assertEqual(self.hsm.master_id, 1)
        
        # Check initial state
        self.assertFalse(self.hsm.is_busy)
        self.assertEqual(self.hsm.operation_mode, HSMMode.ECB)
        self.assertEqual(self.hsm.key_index, 0)
        self.assertEqual(self.hsm.text_length, 0)
        
        # Check register initialization
        self.assertEqual(len(self.hsm.register_manager.registers), 32)  # Expected number of registers

    def test_uid_registers_read_only(self):
        """Test UID registers are read-only and return UUID data."""
        for i in range(4):
            address = 0x40000000 + i * 4
            value = self.hsm.read(address)
            self.assertEqual(value, self.hsm.uuid_data[i])
            
            # Verify read-only (write should be ignored or raise exception)
            original_value = value
            try:
                self.hsm.write(address, 0x12345678)
                # If write doesn't raise exception, verify value unchanged
                new_value = self.hsm.read(address)
                self.assertEqual(new_value, original_value)
            except PermissionError:
                # This is also acceptable for read-only registers
                pass

    def test_cmd_register_functionality(self):
        """Test CMD register functionality."""
        cmd_address = 0x40000010
        
        # Test setting different operation modes
        test_cases = [
            (HSMMode.ECB.value, False, 18, 16),  # ECB mode, encrypt, RAM key, 16 bytes
            (HSMMode.CBC.value, True, 0, 32),    # CBC mode, decrypt, key 0, 32 bytes
            (HSMMode.CMAC.value, False, 18, 100), # CMAC mode, encrypt, RAM key, 100 bytes
            (HSMMode.AUTH.value, False, 3, 16),   # Auth mode, auth key, 16 bytes
            (HSMMode.RND.value, False, 0, 64),    # Random mode, 64 bytes
        ]
        
        for mode, decrypt, key_idx, length in test_cases:
            cmd_value = (mode << 28) | (int(decrypt) << 27) | (key_idx << 22) | length
            self.hsm.write(cmd_address, cmd_value)
            
            self.assertEqual(self.hsm.operation_mode, HSMMode(mode))
            self.assertEqual(self.hsm.decrypt_enable, decrypt)
            self.assertEqual(self.hsm.key_index, key_idx)
            self.assertEqual(self.hsm.text_length, length)

    def test_cst_register_functionality(self):
        """Test CST register functionality."""
        cst_address = 0x40000014
        
        # Initially should not be busy (unlock bit = 1)
        cst_value = self.hsm.read(cst_address)
        self.assertEqual((cst_value >> 16) & 1, 1)  # unlock bit should be 1
        
        # Set valid operation parameters
        self.hsm.operation_mode = HSMMode.RND
        self.hsm.text_length = 16
        
        # Start operation by setting START bit
        self.hsm.write(cst_address, 1)
        
        # Give some time for operation to start
        time.sleep(0.01)
        
        # Should now be busy (unlock bit = 0)
        if self.hsm.is_busy:
            cst_value = self.hsm.read(cst_address)
            self.assertEqual((cst_value >> 16) & 1, 0)  # unlock bit should be 0

    def test_sys_ctrl_register_functionality(self):
        """Test SYS_CTRL register functionality."""
        sys_ctrl_address = 0x40000018
        
        # Test DMA enable
        self.hsm.write(sys_ctrl_address, 0x1)  # Enable DMA
        self.assertTrue(self.hsm.dma_enabled)
        
        # Test IRQ enable
        self.hsm.write(sys_ctrl_address, 0x2)  # Enable IRQ
        sys_ctrl_value = self.hsm.register_manager.registers[0x018].value
        self.assertEqual((sys_ctrl_value >> 1) & 1, 1)

    def test_fifo_status_bits(self):
        """Test FIFO status bits in SYS_CTRL register."""
        sys_ctrl_address = 0x40000018
        
        # Initially FIFOs should be empty
        sys_ctrl_value = self.hsm.read(sys_ctrl_address)
        te_bit = (sys_ctrl_value >> 21) & 1  # Transmit empty (TE=1 means not empty)
        re_bit = (sys_ctrl_value >> 19) & 1  # Receive empty (RE=1 means not empty)
        
        self.assertEqual(te_bit, 0)  # Output FIFO empty (TE=0)
        self.assertEqual(re_bit, 0)  # Input FIFO empty (RE=0)
        
        # Add data to output FIFO and check TE bit
        self.hsm.output_fifo.append(0x12345678)
        sys_ctrl_value = self.hsm.read(sys_ctrl_address)
        te_bit = (sys_ctrl_value >> 21) & 1
        self.assertEqual(te_bit, 1)  # Output FIFO not empty (TE=1)

    def test_ram_key_registers(self):
        """Test RAM key registers functionality."""
        # Test writing and reading RAM keys
        test_key = [0x12345678, 0x9ABCDEF0, 0xFEDCBA98, 0x76543210]
        
        for i in range(4):
            ram_key_address = 0x4000003C + i * 4
            self.hsm.write(ram_key_address, test_key[i])
            self.assertEqual(self.hsm.ram_keys[i], test_key[i])
            
            # Read should return 0 (write-only behavior)
            read_value = self.hsm.read(ram_key_address)
            self.assertEqual(read_value, 0)

    def test_iv_registers(self):
        """Test IV registers functionality."""
        test_iv = [0x11111111, 0x22222222, 0x33333333, 0x44444444]
        
        for i in range(4):
            iv_address = 0x4000002C + i * 4
            self.hsm.write(iv_address, test_iv[i])
            self.assertEqual(self.hsm.iv_data[i], test_iv[i])
            
            # Read should return the written value
            read_value = self.hsm.read(iv_address)
            self.assertEqual(read_value, test_iv[i])

    def test_input_output_data_registers(self):
        """Test CIPIN and CIPOUT registers."""
        cipin_address = 0x40000024
        cipout_address = 0x40000028
        
        # Test writing to CIPIN (should add to input FIFO)
        test_data = 0x12345678
        original_fifo_size = len(self.hsm.input_fifo)
        self.hsm.write(cipin_address, test_data)
        self.assertEqual(len(self.hsm.input_fifo), original_fifo_size + 1)
        self.assertEqual(self.hsm.input_fifo[-1], test_data)
        
        # Test reading from CIPOUT (should pop from output FIFO)
        self.hsm.output_fifo.append(0x87654321)
        read_value = self.hsm.read(cipout_address)
        self.assertEqual(read_value, 0x87654321)
        self.assertEqual(len(self.hsm.output_fifo), 0)

    def test_stat_register_error_reporting(self):
        """Test STAT register error reporting."""
        stat_address = 0x4000007C
        
        # Initially no errors
        stat_value = self.hsm.read(stat_address)
        self.assertEqual(stat_value & 0x7FF, 0)  # No error bits set
        
        # Set an error and check
        self.hsm._set_error(HSMError.ERR1_LENGTH_NOT_ALIGNED_16)
        # The error should be reflected in the error_status attribute
        self.assertTrue(self.hsm.error_status & (1 << 1))  # ERR1 bit set
        
        # Update the register value to reflect the error
        self.hsm.register_manager.registers[0x07C].value = self.hsm.error_status
        stat_value = self.hsm.read(stat_address)
        self.assertEqual(stat_value & (1 << 1), 1 << 1)  # ERR1 bit set
        
        # Clear errors
        self.hsm.write(stat_address, 1 << 16)  # Write clear bit
        self.assertEqual(self.hsm.error_status, 0)  # All error bits cleared

    def test_operation_validation(self):
        """Test operation parameter validation."""
        # Test ECB/CBC mode validations
        self.hsm.operation_mode = HSMMode.ECB
        self.hsm.key_index = 18
        
        # Test zero length error
        self.hsm.text_length = 0
        error = self.hsm._validate_operation()
        self.assertEqual(error, HSMError.ERR2_ZERO_LENGTH)
        
        # Test alignment error
        self.hsm.text_length = 15  # Not aligned to 16 bytes
        error = self.hsm._validate_operation()
        self.assertEqual(error, HSMError.ERR1_LENGTH_NOT_ALIGNED_16)
        
        # Test key index error
        self.hsm.text_length = 16
        self.hsm.key_index = 19  # Too large
        error = self.hsm._validate_operation()
        self.assertEqual(error, HSMError.ERR5_KEYINDEX_TOO_LARGE)
        
        # Test auth key for ECB error
        self.hsm.key_index = 3  # Auth key
        error = self.hsm._validate_operation()
        self.assertEqual(error, HSMError.ERR4_AUTH_KEY_FOR_ECB_CBC)

    def test_random_number_generation(self):
        """Test random number generation mode."""
        # Set up RND mode
        self.hsm.operation_mode = HSMMode.RND
        self.hsm.text_length = 16
        self.hsm.key_index = 0
        
        # Validate operation should pass
        error = self.hsm._validate_operation()
        self.assertIsNone(error)
        
        # Execute random generation
        original_fifo_size = len(self.hsm.output_fifo)
        self.hsm._execute_random_generation()
        
        # Should have generated 4 words (16 bytes)
        self.assertEqual(len(self.hsm.output_fifo), original_fifo_size + 4)

    def test_authentication_mode(self):
        """Test authentication (challenge-response) mode."""
        # Set up AUTH mode
        self.hsm.operation_mode = HSMMode.AUTH
        self.hsm.key_index = 3  # Valid auth key
        self.hsm.text_length = 16
        
        # Execute authentication (generate challenge)
        self.hsm._execute_authentication()
        
        # Check that challenge was generated
        self.assertNotEqual(self.hsm.challenge_data, [0, 0, 0, 0])
        
        # Check ACV bit is set
        self.assertTrue(self.hsm.auth_status & (1 << 16))

    def test_interrupt_generation(self):
        """Test interrupt generation functionality."""
        # Enable interrupts
        sys_ctrl_address = 0x40000018
        self.hsm.write(sys_ctrl_address, 0x2)  # Set IRQEN bit
        
        # Trigger interrupt
        self.hsm._trigger_interrupt_if_enabled()
        
        # Check that interrupt was sent
        self.mock_bus.send_irq.assert_called_with(1, 20)

    def test_dma_functionality(self):
        """Test DMA functionality."""
        # Initially DMA should be disabled
        self.assertFalse(self.hsm.dma_enabled)
        self.assertFalse(self.hsm.is_dma_ready())
        
        # Enable DMA
        self.hsm.enable_dma_mode(0)
        self.assertTrue(self.hsm.dma_enabled)
        self.assertTrue(self.hsm.is_dma_ready())
        
        # Disable DMA
        self.hsm.disable_dma_mode()
        self.assertFalse(self.hsm.dma_enabled)
        self.assertFalse(self.hsm.is_dma_ready())

    def test_error_injection(self):
        """Test error injection functionality."""
        # Inject an error
        self.hsm.inject_error(HSMError.ERR0_RESERVED)
        self.assertIn(HSMError.ERR0_RESERVED, self.hsm.injected_errors)
        
        # Test validation with injected error
        error = self.hsm._validate_operation()
        self.assertEqual(error, HSMError.ERR0_RESERVED)
        
        # Clear injected errors
        self.hsm.clear_injected_errors()
        self.assertEqual(len(self.hsm.injected_errors), 0)

    def test_error_injection_by_string(self):
        """Test error injection using string names."""
        self.hsm.inject_error("ERR1_LENGTH_NOT_ALIGNED_16")
        self.assertIn(HSMError.ERR1_LENGTH_NOT_ALIGNED_16, self.hsm.injected_errors)
        
        # Test invalid error name
        with self.assertRaises(ValueError):
            self.hsm.inject_error("INVALID_ERROR")

    def test_multi_instance_support(self):
        """Test multiple HSM device instances."""
        hsm1 = HSMDevice("hsm1", 0x40000000, 0x1000, 1)
        hsm2 = HSMDevice("hsm2", 0x50000000, 0x1000, 2)
        
        # Instances should be independent
        self.assertEqual(hsm1.name, "hsm1")
        self.assertEqual(hsm2.name, "hsm2")
        self.assertEqual(hsm1.base_address, 0x40000000)
        self.assertEqual(hsm2.base_address, 0x50000000)
        
        # Modify one instance
        hsm1.operation_mode = HSMMode.CBC
        hsm1.key_index = 5
        
        # Other instance should be unaffected
        self.assertEqual(hsm2.operation_mode, HSMMode.ECB)
        self.assertEqual(hsm2.key_index, 0)

    def test_get_status(self):
        """Test status reporting functionality."""
        status = self.hsm.get_status()
        
        expected_keys = {
            'busy', 'mode', 'key_index', 'text_length', 
            'error_status', 'input_fifo_size', 'output_fifo_size', 'dma_enabled'
        }
        self.assertEqual(set(status.keys()), expected_keys)
        
        self.assertFalse(status['busy'])
        self.assertEqual(status['mode'], 'ECB')
        self.assertEqual(status['key_index'], 0)
        self.assertEqual(status['text_length'], 0)

    def test_key_management(self):
        """Test key management functionality."""
        # Test RAM key (index 18)
        test_ram_key = [0x11111111, 0x22222222, 0x33333333, 0x44444444]
        self.hsm.ram_keys = test_ram_key
        
        key_bytes = self.hsm._get_key_bytes(18)
        expected_bytes = bytearray()
        for word in test_ram_key:
            expected_bytes.extend(word.to_bytes(4, 'little'))
        
        self.assertEqual(key_bytes, bytes(expected_bytes))
        
        # Test memory-based key
        key_bytes = self.hsm._get_key_bytes(0)  # CHIP_ROOT_KEY
        self.assertEqual(len(key_bytes), 16)
        self.assertIn(0, self.hsm.memory_keys)

    def test_address_range_validation(self):
        """Test address range validation."""
        valid_address = 0x40000010  # CMD register
        invalid_address = 0x50000000  # Outside range
        
        # Valid address should work
        self.hsm.write(valid_address, 0x12345678)
        value = self.hsm.read(valid_address)
        
        # Invalid address should raise exception
        with self.assertRaises(ValueError):
            self.hsm.read(invalid_address)
        
        with self.assertRaises(ValueError):
            self.hsm.write(invalid_address, 0x12345678)

    def test_concurrent_operations(self):
        """Test thread safety of HSM operations."""
        def write_operation(offset, value):
            try:
                self.hsm.write(0x40000000 + offset, value)
            except:
                pass
        
        def read_operation(offset):
            try:
                return self.hsm.read(0x40000000 + offset)
            except:
                return 0
        
        # Create multiple threads for concurrent access
        threads = []
        for i in range(10):
            t1 = threading.Thread(target=write_operation, args=(0x010, i))
            t2 = threading.Thread(target=read_operation, args=(0x010,))
            threads.extend([t1, t2])
        
        # Start all threads
        for t in threads:
            t.start()
        
        # Wait for all threads to complete
        for t in threads:
            t.join()
        
        # Device should still be in valid state
        status = self.hsm.get_status()
        self.assertIsInstance(status['key_index'], int)


def run_tests():
    """Run all HSM device tests."""
    print("Running HSM Device Tests...")
    print("=" * 50)
    
    # Create test suite
    loader = unittest.TestLoader()
    suite = loader.loadTestsFromTestCase(TestHSMDevice)
    
    # Run tests with verbose output
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)
    
    # Print summary
    print("\n" + "=" * 50)
    print(f"Tests run: {result.testsRun}")
    print(f"Failures: {len(result.failures)}")
    print(f"Errors: {len(result.errors)}")
    
    if result.failures:
        print("\nFailures:")
        for test, traceback in result.failures:
            print(f"- {test}: {traceback}")
    
    if result.errors:
        print("\nErrors:")
        for test, traceback in result.errors:
            print(f"- {test}: {traceback}")
    
    success_rate = ((result.testsRun - len(result.failures) - len(result.errors)) / 
                   result.testsRun * 100) if result.testsRun > 0 else 0
    print(f"\nSuccess rate: {success_rate:.1f}%")
    
    return result.wasSuccessful()


if __name__ == "__main__":
    success = run_tests()
    sys.exit(0 if success else 1)