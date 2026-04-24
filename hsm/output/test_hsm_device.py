#!/usr/bin/env python3
"""
Unit tests for HSM device model.

This module provides comprehensive unit tests for the HSM device implementation,
testing all major functionality and edge cases.
"""

import unittest
import sys
import os
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))

from hsm_device import HSMDevice, HSMCipherMode, HSMState


class TestHSMDevice(unittest.TestCase):
    """Test cases for HSM device functionality."""
    
    def setUp(self):
        """Set up test fixtures."""
        self.hsm = HSMDevice("TestHSM", 0x40000000, 0x1000, 1)
        
    def test_device_initialization(self):
        """Test device initialization."""
        # Verify device is properly initialized
        self.assertEqual(self.hsm.name, "TestHSM")
        self.assertEqual(self.hsm.base_address, 0x40000000)
        self.assertEqual(self.hsm.size, 0x1000)
        self.assertEqual(self.hsm.state, HSMState.IDLE)
        
        # Check that registers are defined (we have exactly 50+ registers)
        self.assertGreaterEqual(len(self.hsm.register_manager.registers), 50)
    
    def test_uid_registers(self):
        """Test UID register reading."""
        # UID registers should be read-only with OTP values
        uid0 = self.hsm.read(0x40000000 + 0x000)
        uid1 = self.hsm.read(0x40000000 + 0x004)
        uid2 = self.hsm.read(0x40000000 + 0x008)
        uid3 = self.hsm.read(0x40000000 + 0x00C)
        
        # Should have non-zero values from OTP simulation
        self.assertNotEqual(uid0, 0)
        self.assertNotEqual(uid1, 0)
        self.assertNotEqual(uid2, 0)
        self.assertNotEqual(uid3, 0)
        
        # Attempting to write should not change values (but may raise PermissionError)
        original_uid0 = uid0
        try:
            self.hsm.write(0x40000000 + 0x000, 0x12345678)
        except PermissionError:
            pass  # Expected for read-only register
        new_uid0 = self.hsm.read(0x40000000 + 0x000)
        self.assertEqual(original_uid0, new_uid0)
    
    def test_system_control_and_status(self):
        """Test system control and status registers."""
        # Test SYS_CTRL register
        self.hsm.write(0x40000000 + 0x018, 0x3)  # Enable IRQ and DMA
        self.assertTrue(self.hsm.irq_en)
        self.assertTrue(self.hsm.dma_en)
        
        # Test SYS_STATUS register
        status = self.hsm.read(0x40000000 + 0x01C)
        
        # Initially TX FIFO should be empty
        self.assertTrue(status & (1 << 8))  # tx_fifo_empty
        
        # Initially RX FIFO should be empty
        self.assertTrue(status & (1 << 6))  # rx_fifo_empty
        
        # HSM should be idle initially
        self.assertTrue(status & (1 << 2))  # hsm_idle
    
    def test_command_configuration(self):
        """Test command register configuration."""
        # Configure for AES ECB mode
        cmd_value = (0x0 << 28) | (1 << 27) | (22 << 22) | (1 << 20) | 32
        self.hsm.write(0x40000000 + 0x010, cmd_value)
        
        # Verify configuration was applied
        self.assertEqual(self.hsm.cipher_mode, HSMCipherMode.ECB)
        self.assertTrue(self.hsm.decrypt_en)
        self.assertEqual(self.hsm.key_idx, 22)
        self.assertTrue(self.hsm.aes256_en)
        self.assertEqual(self.hsm.txt_len, 32)
    
    def test_iv_ram_key_configuration(self):
        """Test IV and RAM key configuration."""
        # Test IV registers
        test_iv = [0x00010203, 0x04050607, 0x08090A0B, 0x0C0D0E0F]
        for i, iv_word in enumerate(test_iv):
            self.hsm.write(0x40000000 + 0x03C + i * 4, iv_word)
        
        # Verify IV was stored correctly
        expected_iv = bytearray()
        for word in test_iv:
            expected_iv.extend([
                (word >> 0) & 0xFF,
                (word >> 8) & 0xFF,
                (word >> 16) & 0xFF,
                (word >> 24) & 0xFF
            ])
        self.assertEqual(self.hsm.iv, expected_iv)
        
        # Test RAM key registers
        test_key = [0x2B7E1516, 0x28AED2A6, 0xABF71588, 0x09CF4F3C]
        for i, key_word in enumerate(test_key):
            self.hsm.write(0x40000000 + 0x04C + i * 4, key_word)
        
        # Verify key was stored correctly
        expected_key = bytearray()
        for word in test_key:
            expected_key.extend([
                (word >> 0) & 0xFF,
                (word >> 8) & 0xFF,
                (word >> 16) & 0xFF,
                (word >> 24) & 0xFF
            ])
        # Pad to 32 bytes
        expected_key.extend([0] * 16)
        self.assertEqual(self.hsm.ram_key, expected_key)
    
    def test_authentication_challenge(self):
        """Test authentication challenge generation."""
        # Initially, challenge should be valid (generated during init)
        auth_status = self.hsm.read(0x40000000 + 0x02C)
        self.assertTrue(auth_status & (1 << 16))  # auth_challenge_valid
        
        # Read challenge
        challenge = []
        for i in range(4):
            challenge_word = self.hsm.read(0x40000000 + 0x06C + i * 4)
            challenge.append(challenge_word)
        
        # Challenge should not be all zeros
        self.assertNotEqual(challenge, [0, 0, 0, 0])
    
    def test_fifo_operations(self):
        """Test FIFO input/output operations."""
        # Initially FIFOs should be empty
        self.assertEqual(len(self.hsm.rx_fifo), 0)
        self.assertEqual(len(self.hsm.tx_fifo), 0)
        
        # Write data to CIPHER_IN (RX FIFO)
        test_data = [0x12345678, 0x9ABCDEF0]
        for data_word in test_data:
            self.hsm.write(0x40000000 + 0x034, data_word)
        
        # RX FIFO should now have 8 bytes (2 words * 4 bytes)
        self.assertEqual(len(self.hsm.rx_fifo), 8)
        
        # Add some data to TX FIFO manually for testing
        self.hsm.tx_fifo.extend([0x11, 0x22, 0x33, 0x44])
        
        # Read from CIPHER_OUT should return data from TX FIFO
        output_word = self.hsm.read(0x40000000 + 0x038)
        expected_word = (0x44 << 24) | (0x33 << 16) | (0x22 << 8) | 0x11
        self.assertEqual(output_word, expected_word)
        
        # TX FIFO should now be empty
        self.assertEqual(len(self.hsm.tx_fifo), 0)
    
    def test_interrupt_handling(self):
        """Test interrupt status and clearing."""
        # Initially IRQ status should be 0
        irq_status = self.hsm.read(0x40000000 + 0x0BC)
        self.assertEqual(irq_status, 0)
        
        # Set some error condition
        self.hsm.irq_status = 0x5  # Set some error bits
        
        # Read IRQ status
        irq_status = self.hsm.read(0x40000000 + 0x0BC)
        self.assertEqual(irq_status, 0x5)
        
        # Clear IRQ
        self.hsm.write(0x40000000 + 0x0C0, 1)
        
        # IRQ status should be cleared
        irq_status = self.hsm.read(0x40000000 + 0x0BC)
        self.assertEqual(irq_status, 0)
    
    def test_golden_mac_registers(self):
        """Test golden MAC register access."""
        test_mac = [0x01234567, 0x89ABCDEF, 0xFEDCBA98, 0x76543210]
        
        # Write golden MAC
        for i, mac_word in enumerate(test_mac):
            self.hsm.write(0x40000000 + 0x08C + i * 4, mac_word)
        
        # Verify MAC was stored correctly
        expected_mac = bytearray()
        for word in test_mac:
            expected_mac.extend([
                (word >> 0) & 0xFF,
                (word >> 8) & 0xFF,
                (word >> 16) & 0xFF,
                (word >> 24) & 0xFF
            ])
        self.assertEqual(self.hsm.golden_mac, expected_mac)
    
    def test_address_range_validation(self):
        """Test address range validation."""
        # Valid address within range
        valid_address = self.hsm.base_address + 0x100
        self.assertTrue(self.hsm.is_address_in_range(valid_address))
        
        # Invalid address below range
        invalid_low = self.hsm.base_address - 1
        self.assertFalse(self.hsm.is_address_in_range(invalid_low))
        
        # Invalid address above range
        invalid_high = self.hsm.base_address + self.hsm.size
        self.assertFalse(self.hsm.is_address_in_range(invalid_high))
    
    def test_device_reset(self):
        """Test device reset functionality."""
        # Set some state
        self.hsm.state = HSMState.EXECUTING
        self.hsm.cmd_on_going = True
        self.hsm.rx_fifo.extend([1, 2, 3, 4])
        self.hsm.tx_fifo.extend([5, 6, 7, 8])
        self.hsm.irq_status = 0xFF
        
        # Reset device
        self.hsm.reset()
        
        # Verify state is reset
        self.assertEqual(self.hsm.state, HSMState.IDLE)
        self.assertFalse(self.hsm.cmd_on_going)
        self.assertEqual(len(self.hsm.rx_fifo), 0)
        self.assertEqual(len(self.hsm.tx_fifo), 0)
        self.assertEqual(self.hsm.irq_status, 0)
        
        # Challenge should be regenerated
        self.assertTrue(self.hsm.auth_challenge_valid)
    
    def test_cipher_mode_enum(self):
        """Test cipher mode enumeration."""
        # Test all cipher modes
        modes = [
            (HSMCipherMode.ECB, 0x0),
            (HSMCipherMode.CBC, 0x1),
            (HSMCipherMode.CFB8, 0x2),
            (HSMCipherMode.CFB128, 0x3),
            (HSMCipherMode.OFB, 0x4),
            (HSMCipherMode.CTR, 0x5),
            (HSMCipherMode.CMAC, 0x6),
            (HSMCipherMode.AUTH, 0x7),
            (HSMCipherMode.RANDOM, 0x8)
        ]
        
        for mode_enum, mode_value in modes:
            self.assertEqual(mode_enum.value, mode_value)
    
    def test_error_conditions(self):
        """Test error handling."""
        # Test write to read-only register (should raise PermissionError)
        with self.assertRaises(PermissionError):
            self.hsm.write(0x40000000 + 0x000, 0x12345678)  # UID0 is read-only
        
        # Test read from write-only register (should raise PermissionError)
        with self.assertRaises(PermissionError):
            value = self.hsm.read(0x40000000 + 0x034)  # CIPHER_IN is write-only
    
    def tearDown(self):
        """Clean up test fixtures."""
        self.hsm = None


class TestHSMCipherModes(unittest.TestCase):
    """Test cases for HSM cipher mode functionality."""
    
    def setUp(self):
        """Set up test fixtures."""
        self.hsm = HSMDevice("TestHSM", 0x40000000, 0x1000, 1)
    
    def test_cipher_mode_configuration(self):
        """Test different cipher mode configurations."""
        modes = [
            (HSMCipherMode.ECB, "ECB mode"),
            (HSMCipherMode.CBC, "CBC mode"),
            (HSMCipherMode.CFB8, "CFB8 mode"),
            (HSMCipherMode.CFB128, "CFB128 mode"),
            (HSMCipherMode.OFB, "OFB mode"),
            (HSMCipherMode.CTR, "CTR mode"),
            (HSMCipherMode.CMAC, "CMAC mode"),
            (HSMCipherMode.AUTH, "Authentication mode"),
            (HSMCipherMode.RANDOM, "Random generation mode")
        ]
        
        for mode, description in modes:
            with self.subTest(mode=description):
                cmd_value = (mode.value << 28) | 16  # mode + 16 byte length
                self.hsm.write(0x40000000 + 0x010, cmd_value)
                self.assertEqual(self.hsm.cipher_mode, mode)


if __name__ == '__main__':
    # Run the unit tests
    unittest.main(verbosity=2)