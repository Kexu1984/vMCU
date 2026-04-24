"""
HSM (Hardware Security Module) device implementation for the DevCommV3 framework.

This module provides an HSM device that supports:
- AES encryption/decryption (ECB, CBC, CFB8, CFB128, OFB, CTR modes)
- MAC generation and verification (CMAC)
- Challenge-response authentication for debug port management
- True Random Number Generator (TRNG)
- DMA and polling operation modes
- Interrupt generation on operation completion
"""

import time
import threading
import random
import hashlib
from enum import Enum
from typing import Dict, Any, Optional, Callable, Union

# Try to import crypto libraries, fall back to simulation if not available
try:
    from Crypto.Cipher import AES
    from Crypto.Hash import CMAC
    from Crypto.Random import get_random_bytes
    CRYPTO_AVAILABLE = True
except ImportError:
    # Fallback implementations for testing without PyCrypto
    CRYPTO_AVAILABLE = False
    
    def get_random_bytes(n):
        """Fallback random bytes generator."""
        return bytes([random.randint(0, 255) for _ in range(n)])
    
    class AES:
        """Simplified AES simulation for testing."""
        MODE_ECB = 1
        MODE_CBC = 2
        MODE_CFB = 3
        MODE_OFB = 5
        MODE_CTR = 6
        
        def __init__(self, key, mode, iv=None, counter=None):
            self.key = key
            self.mode = mode
            self.iv = iv
            self.counter = counter
        
        @staticmethod
        def new(key, mode, iv=None, counter=None, segment_size=128):
            return AES(key, mode, iv, counter)
        
        def encrypt(self, data):
            # Simple XOR simulation for testing
            result = bytearray(len(data))
            for i in range(len(data)):
                result[i] = data[i] ^ self.key[i % len(self.key)]
            return bytes(result)
        
        def decrypt(self, data):
            # Same as encrypt for XOR
            return self.encrypt(data)
    
    class CMAC:
        """Simplified CMAC simulation for testing."""
        def __init__(self, key, ciphermod=None):
            self.key = key
        
        @staticmethod
        def new(key, ciphermod=None):
            return CMAC(key, ciphermod)
        
        def update(self, data):
            self.data = data
        
        def digest(self):
            # Simple hash-based MAC simulation
            return hashlib.sha256(self.key + self.data).digest()[:16]

# Import base classes from the framework
import sys
import os
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))

from devcomm.core.base_device import BaseDevice
from devcomm.core.registers import RegisterType


class HSMCipherMode(Enum):
    """HSM cipher operation modes."""
    ECB = 0x0
    CBC = 0x1
    CFB8 = 0x2
    CFB128 = 0x3
    OFB = 0x4
    CTR = 0x5
    CMAC = 0x6
    AUTH = 0x7
    RANDOM = 0x8  # 1xxx pattern for random number generation


class HSMState(Enum):
    """HSM operational states."""
    IDLE = 0
    EXECUTING = 1
    DONE_SUCCESS = 2
    DONE_ERROR = 3


class HSMDevice(BaseDevice):
    """HSM device implementation with crypto capabilities."""
    
    def __init__(self, name: str = "HSM", base_address: int = 0x40000000, 
                 size: int = 0x1000, master_id: int = 1):
        # Device state
        self.state = HSMState.IDLE
        self.cipher_mode = HSMCipherMode.ECB
        self.decrypt_en = False
        self.key_idx = 0
        self.aes256_en = False
        self.txt_len = 0
        self.irq_en = False
        self.dma_en = False
        
        # Crypto context
        self.current_cipher = None
        self.iv = bytearray(16)  # 128-bit IV
        self.ram_key = bytearray(32)  # 256-bit key (max)
        self.auth_challenge = bytearray(16)  # 128-bit challenge
        self.auth_resp = bytearray(16)  # 128-bit response
        self.golden_mac = bytearray(16)  # 128-bit golden MAC
        self.kek_auth_code = bytearray(32)  # 256-bit KEK auth code
        
        # FIFO buffers
        self.rx_fifo = []  # Input data FIFO
        self.tx_fifo = []  # Output data FIFO
        self.fifo_size = 16  # FIFO depth
        
        # Authentication state
        self.auth_challenge_valid = False
        self.debug_unlock_auth_pass = False
        self.auth_timeout = 30.0  # 30 second timeout
        self.auth_start_time = 0
        
        # Random number generator
        self.rng_seed = random.randint(0, 0xFFFFFFFF)
        
        # Status flags
        self.cmd_on_going = False
        self.cmd_done = 0  # 0=idle, 1=error, 2=success
        self.irq_status = 0
        
        # Initialize parent
        super().__init__(name, base_address, size, master_id)
    
    def init(self) -> None:
        """Initialize HSM registers and state."""
        # UID registers (read-only, OTP values)
        self.register_manager.define_register(
            0x000, "UID0", RegisterType.READ_ONLY, 0x12345678
        )
        self.register_manager.define_register(
            0x004, "UID1", RegisterType.READ_ONLY, 0x9ABCDEF0
        )
        self.register_manager.define_register(
            0x008, "UID2", RegisterType.READ_ONLY, 0x11223344
        )
        self.register_manager.define_register(
            0x00C, "UID3", RegisterType.READ_ONLY, 0x55667788
        )
        
        # Command register
        self.register_manager.define_register(
            0x010, "CMD", RegisterType.READ_WRITE, 0x00000000,
            write_callback=self._cmd_write_callback
        )
        
        # Command valid register
        self.register_manager.define_register(
            0x014, "CMD_VALID", RegisterType.READ_WRITE, 0x00000000,
            write_callback=self._cmd_valid_write_callback
        )
        
        # System control register
        self.register_manager.define_register(
            0x018, "SYS_CTRL", RegisterType.READ_WRITE, 0x00000000,
            write_callback=self._sys_ctrl_write_callback
        )
        
        # System status register (some fields computed)
        self.register_manager.define_register(
            0x01C, "SYS_STATUS", RegisterType.READ_ONLY, 0x000000A6,
            read_callback=self._sys_status_read_callback
        )
        
        # Verify status register
        self.register_manager.define_register(
            0x020, "VFY_STATUS", RegisterType.READ_ONLY, 0x00000000
        )
        
        # Verify status clear register
        self.register_manager.define_register(
            0x024, "VFY_STATUS_CLR", RegisterType.WRITE_ONLY, 0x00000000,
            write_callback=self._vfy_status_clr_write_callback
        )
        
        # Auth control register
        self.register_manager.define_register(
            0x028, "AUTH_CTRL", RegisterType.READ_WRITE, 0x00000000,
            write_callback=self._auth_ctrl_write_callback
        )
        
        # Auth status register
        self.register_manager.define_register(
            0x02C, "AUTH_STATUS", RegisterType.READ_ONLY, 0x00000000,
            read_callback=self._auth_status_read_callback
        )
        
        # Auth status clear register
        self.register_manager.define_register(
            0x030, "AUTH_STATUS_CLR", RegisterType.WRITE_ONLY, 0x00000000,
            write_callback=self._auth_status_clr_write_callback
        )
        
        # Cipher input/output registers
        self.register_manager.define_register(
            0x034, "CIPHER_IN", RegisterType.WRITE_ONLY, 0x00000000,
            write_callback=self._cipher_in_write_callback
        )
        
        self.register_manager.define_register(
            0x038, "CIPHER_OUT", RegisterType.READ_ONLY, 0x00000000,
            read_callback=self._cipher_out_read_callback
        )
        
        # IV registers
        for i in range(4):
            # Create callback with proper closure
            def make_iv_callback(idx):
                return lambda dev, off, val: self._iv_write_callback(dev, off, val, idx)
            
            self.register_manager.define_register(
                0x03C + i * 4, f"IV{i}", RegisterType.READ_WRITE, 0x00000000,
                write_callback=make_iv_callback(i)
            )
        
        # RAM key registers
        for i in range(8):
            # Create callback with proper closure
            def make_ram_key_callback(idx):
                return lambda dev, off, val: self._ram_key_write_callback(dev, off, val, idx)
                
            self.register_manager.define_register(
                0x04C + i * 4, f"RAM_KEY{i}", RegisterType.READ_WRITE, 0x00000000,
                write_callback=make_ram_key_callback(i)
            )
        
        # Auth challenge registers (read-only)
        for i in range(4):
            # Create callback with proper closure
            def make_auth_challenge_callback(idx):
                return lambda dev, off, val: self._auth_challenge_read_callback(dev, off, val, idx)
                
            self.register_manager.define_register(
                0x06C + i * 4, f"AUTH_CHALLENGE{i}", RegisterType.READ_ONLY, 0x00000000,
                read_callback=make_auth_challenge_callback(i)
            )
        
        # Auth response registers
        for i in range(4):
            # Create callback with proper closure
            def make_auth_resp_callback(idx):
                return lambda dev, off, val: self._auth_resp_write_callback(dev, off, val, idx)
                
            self.register_manager.define_register(
                0x07C + i * 4, f"AUTH_RESP{i}", RegisterType.READ_WRITE, 0x00000000,
                write_callback=make_auth_resp_callback(i)
            )
        
        # Golden MAC registers
        for i in range(4):
            # Create callback with proper closure
            def make_golden_mac_callback(idx):
                return lambda dev, off, val: self._golden_mac_write_callback(dev, off, val, idx)
                
            self.register_manager.define_register(
                0x08C + i * 4, f"GOLDEN_MAC{i}", RegisterType.READ_WRITE, 0x00000000,
                write_callback=make_golden_mac_callback(i)
            )
        
        # KEK auth code registers
        for i in range(8):
            # Create callback with proper closure
            def make_kek_auth_callback(idx):
                return lambda dev, off, val: self._kek_auth_write_callback(dev, off, val, idx)
                
            self.register_manager.define_register(
                0x09C + i * 4, f"KEK_AUTH_CODE{i}", RegisterType.READ_WRITE, 0x00000000,
                write_callback=make_kek_auth_callback(i)
            )
        
        # IRQ status register
        self.register_manager.define_register(
            0x0BC, "IRQ_STATUS", RegisterType.READ_ONLY, 0x00000000,
            read_callback=self._irq_status_read_callback
        )
        
        # IRQ clear register
        self.register_manager.define_register(
            0x0C0, "IRQ_CLR", RegisterType.WRITE_ONLY, 0x00000000,
            write_callback=self._irq_clr_write_callback
        )
        
        # Chip state register
        self.register_manager.define_register(
            0x0C4, "CHIP_STATE", RegisterType.READ_ONLY, 0x00000000
        )
        
        # Generate initial auth challenge
        self._generate_auth_challenge()
        
        self.trace_manager.log_device_event(self.name, self.name, "INIT", 
                                          {"registers": len(self.register_manager.registers)})
    
    def _read_implementation(self, offset: int, width: int) -> int:
        """Device-specific read implementation using register manager."""
        if offset in self.register_manager.registers:
            return self.register_manager.read_register(self, offset, width)
        else:
            # Return 0 for undefined addresses
            return 0
    
    def _write_implementation(self, offset: int, value: int, width: int) -> None:
        """Device-specific write implementation using register manager."""
        if offset in self.register_manager.registers:
            self.register_manager.write_register(self, offset, value, width)
        # Ignore writes to undefined addresses
    
    # Register callback implementations
    
    def _cmd_write_callback(self, device_instance, offset: int, value: int) -> None:
        """Handle CMD register write."""
        self.cipher_mode = HSMCipherMode((value >> 28) & 0xF)
        self.decrypt_en = bool((value >> 27) & 1)
        self.key_idx = (value >> 22) & 0x1F
        self.aes256_en = bool((value >> 20) & 1)
        self.txt_len = value & 0xFFFFF
        
        self.trace_manager.log_device_event(self.name, self.name, "CMD_CONFIG",
                                          {"cipher_mode": self.cipher_mode.name,
                                           "decrypt": self.decrypt_en,
                                           "key_idx": self.key_idx,
                                           "aes256": self.aes256_en,
                                           "txt_len": self.txt_len})
    
    def _cmd_valid_write_callback(self, device_instance, offset: int, value: int) -> None:
        """Handle CMD_VALID register write - start operation."""
        if value & 1:
            self._start_operation()
    
    def _sys_ctrl_write_callback(self, device_instance, offset: int, value: int) -> None:
        """Handle SYS_CTRL register write."""
        self.irq_en = bool((value >> 1) & 1)
        self.dma_en = bool(value & 1)
        
        self.trace_manager.log_device_event(self.name, self.name, "SYS_CTRL",
                                          {"irq_en": self.irq_en, "dma_en": self.dma_en})
    
    def _sys_status_read_callback(self, device_instance, offset: int, current_value: int) -> int:
        """Handle SYS_STATUS register read - compute dynamic status."""
        status = 0
        
        # FIFO status
        if len(self.tx_fifo) == 0:
            status |= (1 << 8)  # tx_fifo_empty
        if len(self.tx_fifo) >= self.fifo_size:
            status |= (1 << 7)  # tx_fifo_full
        if len(self.rx_fifo) == 0:
            status |= (1 << 6)  # rx_fifo_empty
        if len(self.rx_fifo) >= self.fifo_size:
            status |= (1 << 5)  # rx_fifo_full
        
        # Command status
        if self.cmd_on_going:
            status |= (1 << 4)  # cpu_cmd_on_going
        if not self.cmd_on_going:
            status |= (1 << 3)  # cmd_rg_unlock
        if self.state == HSMState.IDLE:
            status |= (1 << 2)  # hsm_idle
        
        # Command done status
        status |= self.cmd_done & 0x3  # hsm_cmd_done
        
        return status
    
    def _cipher_in_write_callback(self, device_instance, offset: int, value: int) -> None:
        """Handle CIPHER_IN register write - add data to RX FIFO."""
        if len(self.rx_fifo) < self.fifo_size:
            # Convert 32-bit word to bytes
            data_bytes = [
                (value >> 0) & 0xFF,
                (value >> 8) & 0xFF,
                (value >> 16) & 0xFF,
                (value >> 24) & 0xFF
            ]
            self.rx_fifo.extend(data_bytes)
            
            # If we have enough data and operation is configured, start processing
            if not self.cmd_on_going and len(self.rx_fifo) >= 4:
                self._process_cipher_data()
    
    def _cipher_out_read_callback(self, device_instance, offset: int, current_value: int) -> int:
        """Handle CIPHER_OUT register read - get data from TX FIFO."""
        if len(self.tx_fifo) >= 4:
            # Get 4 bytes and convert to 32-bit word
            data_bytes = []
            for _ in range(4):
                data_bytes.append(self.tx_fifo.pop(0))
            
            value = (data_bytes[3] << 24) | (data_bytes[2] << 16) | \
                   (data_bytes[1] << 8) | data_bytes[0]
            return value
        return 0
    
    def _iv_write_callback(self, device_instance, offset: int, value: int, idx: int) -> None:
        """Handle IV register write."""
        # Store IV bytes in little-endian format
        start_idx = idx * 4
        self.iv[start_idx:start_idx + 4] = [
            (value >> 0) & 0xFF,
            (value >> 8) & 0xFF,
            (value >> 16) & 0xFF,
            (value >> 24) & 0xFF
        ]
    
    def _ram_key_write_callback(self, device_instance, offset: int, value: int, idx: int) -> None:
        """Handle RAM_KEY register write."""
        # Store key bytes in little-endian format
        start_idx = idx * 4
        self.ram_key[start_idx:start_idx + 4] = [
            (value >> 0) & 0xFF,
            (value >> 8) & 0xFF,
            (value >> 16) & 0xFF,
            (value >> 24) & 0xFF
        ]
    
    def _auth_ctrl_write_callback(self, device_instance, offset: int, value: int) -> None:
        """Handle AUTH_CTRL register write."""
        if value & 1:  # ext_auth_resp_valid
            self._validate_auth_response()
    
    def _auth_status_read_callback(self, device_instance, offset: int, current_value: int) -> int:
        """Handle AUTH_STATUS register read."""
        status = 0
        
        if self.auth_challenge_valid:
            status |= (1 << 16)
        if self.debug_unlock_auth_pass:
            status |= (1 << 11)
        
        # Check for timeout
        if self.auth_challenge_valid and \
           (time.time() - self.auth_start_time) > self.auth_timeout:
            status |= (1 << 10)  # debug_unlock_auth_tmo_fail
            self.auth_challenge_valid = False
        
        return status
    
    def _auth_challenge_read_callback(self, device_instance, offset: int, current_value: int, idx: int) -> int:
        """Handle AUTH_CHALLENGE register read."""
        if self.auth_challenge_valid:
            start_idx = idx * 4
            value = (self.auth_challenge[start_idx + 3] << 24) | \
                   (self.auth_challenge[start_idx + 2] << 16) | \
                   (self.auth_challenge[start_idx + 1] << 8) | \
                   self.auth_challenge[start_idx + 0]
            return value
        return 0
    
    def _auth_resp_write_callback(self, device_instance, offset: int, value: int, idx: int) -> None:
        """Handle AUTH_RESP register write."""
        start_idx = idx * 4
        self.auth_resp[start_idx:start_idx + 4] = [
            (value >> 0) & 0xFF,
            (value >> 8) & 0xFF,
            (value >> 16) & 0xFF,
            (value >> 24) & 0xFF
        ]
    
    def _golden_mac_write_callback(self, device_instance, offset: int, value: int, idx: int) -> None:
        """Handle GOLDEN_MAC register write."""
        start_idx = idx * 4
        self.golden_mac[start_idx:start_idx + 4] = [
            (value >> 0) & 0xFF,
            (value >> 8) & 0xFF,
            (value >> 16) & 0xFF,
            (value >> 24) & 0xFF
        ]
    
    def _kek_auth_write_callback(self, device_instance, offset: int, value: int, idx: int) -> None:
        """Handle KEK_AUTH_CODE register write."""
        start_idx = idx * 4
        self.kek_auth_code[start_idx:start_idx + 4] = [
            (value >> 0) & 0xFF,
            (value >> 8) & 0xFF,
            (value >> 16) & 0xFF,
            (value >> 24) & 0xFF
        ]
    
    def _irq_status_read_callback(self, device_instance, offset: int, current_value: int) -> int:
        """Handle IRQ_STATUS register read."""
        return self.irq_status
    
    def _irq_clr_write_callback(self, device_instance, offset: int, value: int) -> None:
        """Handle IRQ_CLR register write."""
        if value & 1:
            self.irq_status = 0
            self.trace_manager.log_device_event(self.name, self.name, "IRQ_CLEAR", {})
    
    def _vfy_status_clr_write_callback(self, device_instance, offset: int, value: int) -> None:
        """Handle VFY_STATUS_CLR register write."""
        if value & 1:
            # Clear verification status flags
            self.register_manager.registers[0x020].value = 0
    
    def _auth_status_clr_write_callback(self, device_instance, offset: int, value: int) -> None:
        """Handle AUTH_STATUS_CLR register write."""
        # Clear corresponding auth status bits based on written value
        pass
    
    # Core functionality implementations
    
    def _generate_auth_challenge(self) -> None:
        """Generate a new authentication challenge."""
        self.auth_challenge = bytearray(get_random_bytes(16))
        self.auth_challenge_valid = True
        self.auth_start_time = time.time()
        
        self.trace_manager.log_device_event(self.name, self.name, "AUTH_CHALLENGE_GEN",
                                          {"challenge": self.auth_challenge.hex()})
    
    def _validate_auth_response(self) -> None:
        """Validate authentication response."""
        # For simulation, use a simple validation
        # In real hardware, this would encrypt the challenge with debug key
        expected_resp = hashlib.sha256(self.auth_challenge).digest()[:16]
        
        if bytes(self.auth_resp) == expected_resp:
            self.debug_unlock_auth_pass = True
            self.trace_manager.log_device_event(self.name, self.name, "AUTH_SUCCESS", {})
        else:
            self.debug_unlock_auth_pass = False
            self.trace_manager.log_device_event(self.name, self.name, "AUTH_FAIL", {})
    
    def _start_operation(self) -> None:
        """Start the configured HSM operation."""
        if self.cmd_on_going:
            return
        
        self.cmd_on_going = True
        self.state = HSMState.EXECUTING
        self.cmd_done = 0
        
        # Start operation in background thread
        operation_thread = threading.Thread(target=self._execute_operation)
        operation_thread.daemon = True
        operation_thread.start()
        
        self.trace_manager.log_device_event(self.name, self.name, "OPERATION_START",
                                          {"mode": self.cipher_mode.name})
    
    def _execute_operation(self) -> None:
        """Execute the configured operation."""
        try:
            if self.cipher_mode.value >= 0x8:  # Random number generation
                self._generate_random_data()
            elif self.cipher_mode == HSMCipherMode.CMAC:
                self._compute_mac()
            elif self.cipher_mode == HSMCipherMode.AUTH:
                self._perform_authentication()
            else:
                self._perform_cipher_operation()
            
            self.state = HSMState.DONE_SUCCESS
            self.cmd_done = 2  # Success
            
        except Exception as e:
            self.state = HSMState.DONE_ERROR
            self.cmd_done = 1  # Error
            self.irq_status |= 1  # Set error flag
            self.trace_manager.log_device_event(self.name, self.name, "OPERATION_ERROR",
                                              {"error": str(e)})
        
        finally:
            self.cmd_on_going = False
            
            # Send interrupt if enabled
            if self.irq_en:
                self.trigger_interrupt(0)
    
    def _generate_random_data(self) -> None:
        """Generate random data."""
        bytes_needed = (self.txt_len + 3) // 4 * 4  # Round up to 4-byte boundary
        random_data = get_random_bytes(bytes_needed)
        
        # Add to TX FIFO
        for byte in random_data:
            if len(self.tx_fifo) < self.fifo_size * 4:
                self.tx_fifo.append(byte)
        
        self.trace_manager.log_device_event(self.name, self.name, "RANDOM_GEN",
                                          {"bytes": bytes_needed})
    
    def _compute_mac(self) -> None:
        """Compute CMAC."""
        if not self.rx_fifo:
            return
        
        # Get data from RX FIFO
        data = bytes(self.rx_fifo[:self.txt_len])
        self.rx_fifo = self.rx_fifo[self.txt_len:]
        
        # Get key
        key = self._get_key()
        
        # Compute CMAC
        cipher = CMAC.new(key, ciphermod=AES)
        cipher.update(data)
        mac = cipher.digest()
        
        # Add to TX FIFO
        for byte in mac:
            if len(self.tx_fifo) < self.fifo_size * 4:
                self.tx_fifo.append(byte)
        
        self.trace_manager.log_device_event(self.name, self.name, "CMAC_COMPUTE",
                                          {"data_len": len(data), "mac": mac.hex()})
    
    def _perform_authentication(self) -> None:
        """Perform authentication operation."""
        self._generate_auth_challenge()
    
    def _perform_cipher_operation(self) -> None:
        """Perform AES cipher operation."""
        if not self.rx_fifo or self.txt_len == 0:
            return
        
        # Get data from RX FIFO (align to block size if needed)
        block_size = 16
        if self.cipher_mode in [HSMCipherMode.ECB, HSMCipherMode.CBC, HSMCipherMode.CFB128]:
            data_len = (self.txt_len + block_size - 1) // block_size * block_size
        else:
            data_len = self.txt_len
        
        data = bytes(self.rx_fifo[:data_len])
        self.rx_fifo = self.rx_fifo[data_len:]
        
        # Pad data if necessary
        if len(data) % block_size != 0 and self.cipher_mode in [HSMCipherMode.ECB, HSMCipherMode.CBC]:
            padding_len = block_size - (len(data) % block_size)
            data += b'\x00' * padding_len
        
        # Get key and IV
        key = self._get_key()
        iv = bytes(self.iv) if self.cipher_mode != HSMCipherMode.ECB else None
        
        # Create cipher
        if self.cipher_mode == HSMCipherMode.ECB:
            cipher = AES.new(key, AES.MODE_ECB)
        elif self.cipher_mode == HSMCipherMode.CBC:
            cipher = AES.new(key, AES.MODE_CBC, iv)
        elif self.cipher_mode == HSMCipherMode.CFB8:
            cipher = AES.new(key, AES.MODE_CFB, iv, segment_size=8)
        elif self.cipher_mode == HSMCipherMode.CFB128:
            cipher = AES.new(key, AES.MODE_CFB, iv)
        elif self.cipher_mode == HSMCipherMode.OFB:
            cipher = AES.new(key, AES.MODE_OFB, iv)
        elif self.cipher_mode == HSMCipherMode.CTR:
            # Use IV as counter
            if CRYPTO_AVAILABLE:
                from Crypto.Util import Counter
                ctr = Counter.new(128, initial_value=int.from_bytes(iv, 'big'))
                cipher = AES.new(key, AES.MODE_CTR, counter=ctr)
            else:
                cipher = AES.new(key, AES.MODE_CTR, iv)
        else:
            raise ValueError(f"Unsupported cipher mode: {self.cipher_mode}")
        
        # Perform encryption/decryption
        if self.decrypt_en and self.cipher_mode in [HSMCipherMode.ECB, HSMCipherMode.CBC]:
            result = cipher.decrypt(data)
        else:
            result = cipher.encrypt(data)
        
        # Add result to TX FIFO
        for byte in result:
            if len(self.tx_fifo) < self.fifo_size * 4:
                self.tx_fifo.append(byte)
        
        self.trace_manager.log_device_event(self.name, self.name, "CIPHER_OP",
                                          {"mode": self.cipher_mode.name,
                                           "decrypt": self.decrypt_en,
                                           "data_len": len(data),
                                           "result_len": len(result)})
    
    def _process_cipher_data(self) -> None:
        """Process data from CIPHER_IN when not using command mode."""
        # This would be called for streaming operations
        pass
    
    def _get_key(self) -> bytes:
        """Get the encryption key based on key index."""
        if self.key_idx == 22:  # RAM key
            key_len = 32 if self.aes256_en else 16
            return bytes(self.ram_key[:key_len])
        else:
            # For simulation, use hardcoded keys
            if self.aes256_en:
                return b'\x01' * 32  # 256-bit key
            else:
                return b'\x01' * 16  # 128-bit key
    
    def _reset_implementation(self) -> None:
        """Device-specific reset implementation."""
        self.state = HSMState.IDLE
        self.cmd_on_going = False
        self.cmd_done = 0
        self.irq_status = 0
        self.rx_fifo.clear()
        self.tx_fifo.clear()
        self.auth_challenge_valid = False
        self.debug_unlock_auth_pass = False
        
        # Generate new auth challenge
        self._generate_auth_challenge()
        
        self.trace_manager.log_device_event(self.name, self.name, "RESET_COMPLETE", {})