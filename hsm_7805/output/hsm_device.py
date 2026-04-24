"""
HSM (Hardware Security Module) device implementation for the DevCommV3 framework.

This module provides a comprehensive HSM device that supports:
- AES-128 encryption/decryption (ECB and CBC modes)
- AES-128 CMAC generation and verification
- True Random Number Generator (TRNG)
- Challenge-response authentication
- Key management (memory-based and RAM-based keys)
- UUID handling
- DMA integration for data transfer
- Interrupt generation on operation completion/errors
- Error injection for fault testing
"""

import time
import threading
import secrets
from enum import Enum
from typing import Dict, Any, Optional, Callable, List, Union
from Crypto.Cipher import AES
from Crypto.Hash import CMAC
from Crypto.Util import Counter

# Import base classes from the framework
import sys
import os
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))

from devcomm.core.base_device import BaseDevice, DMAInterface
from devcomm.core.registers import RegisterType
from devcomm.utils.event_constants import DeviceOperation


class HSMMode(Enum):
    """HSM operation modes."""
    ECB = 0x0
    CBC = 0x1
    CMAC = 0x2
    AUTH = 0x3
    RND = 0x4


class HSMKeyType(Enum):
    """HSM key types and their indices."""
    CHIP_ROOT_KEY = 0
    BL_VERIFY_KEY = 1
    FW_UPGRADE_KEY = 2
    DEBUG_AUTH_KEY = 3
    FW_INSTALL_KEY = 4
    KEY_READ_KEY = 5
    KEY_INSTALL_KEY = 6
    CHIP_STATE_KEY = 7
    RAM_KEY = 18


class HSMError(Enum):
    """HSM error codes."""
    ERR0_RESERVED = 0
    ERR1_LENGTH_NOT_ALIGNED_16 = 1
    ERR2_ZERO_LENGTH = 2
    ERR3_CMAC_KEY_FOR_ECB_CBC = 3
    ERR4_AUTH_KEY_FOR_ECB_CBC = 4
    ERR5_KEYINDEX_TOO_LARGE = 5
    ERR6_CMAC_KEY_WRONG = 6
    ERR7_KEYINDEX_TOO_LARGE_CMAC = 7
    ERR8_AUTH_KEY_WRONG = 8
    ERR9_RND_LENGTH_NOT_ALIGNED_4 = 9
    ERR10_RND_ZERO_LENGTH = 10


class HSMDevice(BaseDevice):
    """HSM device implementation."""

    # Memory addresses for keys (16 bytes each)
    KEY_MEMORY_BASE = 0x8300010
    UUID_MEMORY_BASE = 0x8300000

    # Interrupt number
    IRQ_NUMBER = 20

    def __init__(self, name: str, base_address: int, size: int, master_id: int):
        # Initialize device state
        self.operation_mode = HSMMode.ECB
        self.key_index = 0
        self.text_length = 0
        self.decrypt_enable = False
        self.is_busy = False
        self.operation_complete = False

        # Key storage
        self.memory_keys = {}  # Memory-based keys (indices 0-7)
        self.ram_keys = [0] * 4  # RAM keys (128-bit split into 4x32-bit)
        self.uuid_data = [0] * 4  # UUID data (128-bit split into 4x32-bit)

        # FIFO buffers
        self.input_fifo = []
        self.output_fifo = []
        self.max_fifo_size = 16  # 16 words deep

        # Crypto state
        self.iv_data = [0] * 4  # IV data (128-bit split into 4x32-bit)
        self.challenge_data = [0] * 4  # Challenge data
        self.response_data = [0] * 4  # Response data
        self.mac_data = [0] * 4  # MAC data

        # Status flags
        self.error_status = 0
        self.auth_status = 0x80078  # Default value from register map
        self.verify_status = 0

        # DMA support
        self.dma_interface = DMAInterface()
        self.dma_enabled = False

        # Thread for background operations
        self.operation_thread = None
        self.operation_lock = threading.Lock()

        # Error injection
        self.injected_errors = set()

        # Initialize base device
        super().__init__(name, base_address, size, master_id)

    def init(self) -> None:
        """Initialize HSM device registers and state."""
        # Initialize memory-based keys with dummy values
        for key_type in [HSMKeyType.CHIP_ROOT_KEY, HSMKeyType.BL_VERIFY_KEY,
                        HSMKeyType.FW_UPGRADE_KEY, HSMKeyType.DEBUG_AUTH_KEY,
                        HSMKeyType.FW_INSTALL_KEY, HSMKeyType.KEY_READ_KEY,
                        HSMKeyType.KEY_INSTALL_KEY, HSMKeyType.CHIP_STATE_KEY]:
            # Generate dummy 128-bit key for each type
            self.memory_keys[key_type.value] = secrets.token_bytes(16)

        # Initialize UUID with dummy data
        uuid_bytes = secrets.token_bytes(16)
        for i in range(4):
            self.uuid_data[i] = int.from_bytes(uuid_bytes[i*4:(i+1)*4], 'little')

        # Define UID registers (0x000-0x00C)
        for i in range(4):
            self.register_manager.define_register(
                offset=i * 4,
                name=f"UID{i}",
                register_type=RegisterType.READ_ONLY,
                reset_value=self.uuid_data[i],
                read_callback=self._uid_read_callback
            )

        # Define CMD register (0x010)
        self.register_manager.define_register(
            offset=0x010,
            name="CMD",
            register_type=RegisterType.READ_WRITE,
            reset_value=0x00000000,
            write_callback=self._cmd_write_callback
        )

        # Define CST register (0x014)
        self.register_manager.define_register(
            offset=0x014,
            name="CST",
            register_type=RegisterType.READ_WRITE,
            reset_value=0x00010000,  # unlock bit set by default
            write_callback=self._cst_write_callback,
            read_callback=self._cst_read_callback
        )

        # Define SYS_CTRL register (0x018)
        self.register_manager.define_register(
            offset=0x018,
            name="SYS_CTRL",
            register_type=RegisterType.READ_WRITE,
            reset_value=0x002D0000,  # Default from register map
            write_callback=self._sys_ctrl_write_callback,
            read_callback=self._sys_ctrl_read_callback
        )

        # Define VRY_STAT register (0x01C)
        self.register_manager.define_register(
            offset=0x01C,
            name="VRY_STAT",
            register_type=RegisterType.READ_WRITE,
            reset_value=0x00000000,
            write_callback=self._vry_stat_write_callback
        )

        # Define AUTH_STAT register (0x020)
        self.register_manager.define_register(
            offset=0x020,
            name="AUTH_STAT",
            register_type=RegisterType.READ_WRITE,
            reset_value=0x00080078,
            write_callback=self._auth_stat_write_callback
        )

        # Define CIPIN register (0x024)
        self.register_manager.define_register(
            offset=0x024,
            name="CIPIN",
            register_type=RegisterType.READ_WRITE,
            reset_value=0x00000000,
            write_callback=self._cipin_write_callback
        )

        # Define CIPOUT register (0x028)
        self.register_manager.define_register(
            offset=0x028,
            name="CIPOUT",
            register_type=RegisterType.READ_WRITE,
            reset_value=0x00000000,
            read_callback=self._cipout_read_callback
        )

        # Define IV registers (0x02C-0x038)
        for i in range(4):
            self.register_manager.define_register(
                offset=0x02C + i * 4,
                name=f"IV{i}",
                register_type=RegisterType.READ_WRITE,
                reset_value=0x00000000,
                write_callback=self._iv_write_callback
            )

        # Define RAM_KEY registers (0x03C-0x048) - write-only, read returns 0
        for i in range(4):
            self.register_manager.define_register(
                offset=0x03C + i * 4,
                name=f"RAM_KEY{i}",
                register_type=RegisterType.READ_WRITE,
                reset_value=0x00000000,
                write_callback=self._ram_key_write_callback,
                read_callback=self._ram_key_read_callback
            )

        # Define Challenge registers (0x04C-0x058)
        for i in range(4):
            self.register_manager.define_register(
                offset=0x04C + i * 4,
                name=f"CHA{i}",
                register_type=RegisterType.READ_WRITE,
                reset_value=0x00000000,
                write_callback=self._cha_write_callback
            )

        # Define Response registers (0x05C-0x068)
        for i in range(4):
            self.register_manager.define_register(
                offset=0x05C + i * 4,
                name=f"RSP{i}",
                register_type=RegisterType.READ_WRITE,
                reset_value=0x00000000,
                write_callback=self._rsp_write_callback
            )

        # Define MAC registers (0x06C-0x078)
        for i in range(4):
            self.register_manager.define_register(
                offset=0x06C + i * 4,
                name=f"MAC{i}",
                register_type=RegisterType.READ_WRITE,
                reset_value=0x00000000,
                write_callback=self._mac_write_callback
            )

        # Define STAT register (0x07C)
        self.register_manager.define_register(
            offset=0x07C,
            name="STAT",
            register_type=RegisterType.READ_WRITE,
            reset_value=0x00000000,
            write_callback=self._stat_write_callback,
            read_callback=self._stat_read_callback
        )

    def _read_implementation(self, offset: int, width: int) -> int:
        """Device-specific read implementation."""
        return self.register_manager.read_register(self, offset, width)

    def _write_implementation(self, offset: int, value: int, width: int) -> None:
        """Device-specific write implementation."""
        self.register_manager.write_register(self, offset, value, width)

    # Register callback implementations
    def _uid_read_callback(self, device_instance, offset: int, current_value: int) -> int:
        """UID register read callback."""
        index = offset // 4
        return self.uuid_data[index] if index < 4 else 0

    def _cmd_write_callback(self, device_instance, offset: int, value: int) -> None:
        """CMD register write callback."""
        # Extract fields from command register
        self.operation_mode = HSMMode((value >> 28) & 0x7)
        self.decrypt_enable = bool((value >> 27) & 0x1)
        self.key_index = (value >> 22) & 0x1F
        self.text_length = value & 0x7FFFF

        self.trace_manager.log_device_event(
            self.name, self.name, DeviceOperation.WRITE,
            {
                "register": "CMD",
                "mode": self.operation_mode.name,
                "decrypt": self.decrypt_enable,
                "key_index": self.key_index,
                "text_length": self.text_length
            }
        )

    def _cst_write_callback(self, device_instance, offset: int, value: int) -> None:
        """CST register write callback."""
        start_bit = value & 0x1
        if start_bit and not self.is_busy:
            # Start HSM operation
            self._start_operation()

    def _cst_read_callback(self, device_instance, offset: int, current_value: int) -> int:
        """CST register read callback."""
        unlock_bit = 0 if self.is_busy else 1
        return (unlock_bit << 16) | 0

    def _sys_ctrl_write_callback(self, device_instance, offset: int, value: int) -> None:
        """SYS_CTRL register write callback."""
        irq_enable = bool((value >> 1) & 0x1)
        dma_enable = bool(value & 0x1)

        if dma_enable != self.dma_enabled:
            self.dma_enabled = dma_enable
            if dma_enable:
                self.dma_interface.enable_dma(0)  # Use DMA channel 0
            else:
                self.dma_interface.disable_dma()

    def _sys_ctrl_read_callback(self, device_instance, offset: int, current_value: int) -> int:
        """SYS_CTRL register read callback."""
        # Build status bits based on current state
        te_bit = 0 if len(self.output_fifo) > 0 else 1
        tf_bit = 1 if len(self.output_fifo) >= self.max_fifo_size else 0
        re_bit = 0 if len(self.input_fifo) > 0 else 1
        rf_bit = 1 if len(self.input_fifo) >= self.max_fifo_size else 0
        bsy_bit = 1 if self.is_busy else 0
        idle_bit = 0 if self.is_busy else 1

        # Build the status value
        status = (te_bit << 21) | (tf_bit << 20) | (re_bit << 19) | (rf_bit << 18)
        status |= (bsy_bit << 17) | (idle_bit << 16)

        # Add DMA and IRQ enable bits from current register value
        status |= current_value & 0x3  # Preserve DMAEN and IRQEN bits

        return status

    def _vry_stat_write_callback(self, device_instance, offset: int, value: int) -> None:
        """VRY_STAT register write callback - clear bits on write."""
        # Clear bits that are written with 1
        self.verify_status &= ~value

    def _auth_stat_write_callback(self, device_instance, offset: int, value: int) -> None:
        """AUTH_STAT register write callback."""
        # Handle ARV bit (bit 0) - authentication response valid
        if value & 0x1:
            self._process_authentication_response()

    def _cipin_write_callback(self, device_instance, offset: int, value: int) -> None:
        """CIPIN register write callback - add data to input FIFO."""
        if len(self.input_fifo) < self.max_fifo_size:
            self.input_fifo.append(value)
            self._process_input_data()

    def _cipout_read_callback(self, device_instance, offset: int, current_value: int) -> int:
        """CIPOUT register read callback - get data from output FIFO."""
        if self.output_fifo:
            return self.output_fifo.pop(0)
        return 0

    def _iv_write_callback(self, device_instance, offset: int, value: int) -> None:
        """IV register write callback."""
        index = (offset - 0x02C) // 4
        if 0 <= index < 4:
            self.iv_data[index] = value

    def _ram_key_write_callback(self, device_instance, offset: int, value: int) -> None:
        """RAM_KEY register write callback."""
        index = (offset - 0x03C) // 4
        if 0 <= index < 4:
            self.ram_keys[index] = value

    def _ram_key_read_callback(self, device_instance, offset: int, current_value: int) -> int:
        """RAM_KEY register read callback - always returns 0."""
        return 0

    def _cha_write_callback(self, device_instance, offset: int, value: int) -> None:
        """Challenge register write callback."""
        index = (offset - 0x04C) // 4
        if 0 <= index < 4:
            self.challenge_data[index] = value

    def _rsp_write_callback(self, device_instance, offset: int, value: int) -> None:
        """Response register write callback."""
        index = (offset - 0x05C) // 4
        if 0 <= index < 4:
            self.response_data[index] = value

    def _mac_write_callback(self, device_instance, offset: int, value: int) -> None:
        """MAC register write callback."""
        index = (offset - 0x06C) // 4
        if 0 <= index < 4:
            self.mac_data[index] = value

    def _stat_write_callback(self, device_instance, offset: int, value: int) -> None:
        """STAT register write callback."""
        # Clear bit (bit 16) clears all error status
        if value & (1 << 16):
            self.error_status = 0

    def _stat_read_callback(self, device_instance, offset: int, current_value: int) -> int:
        """STAT register read callback - return current error status."""
        return self.error_status

    # HSM operation implementations
    def _start_operation(self) -> None:
        """Start HSM operation based on current mode."""
        if self.is_busy:
            return

        # Validate operation parameters
        error = self._validate_operation()
        if error is not None:
            self._set_error(error)
            return

        self.is_busy = True
        self.operation_complete = False

        # Start operation in background thread
        if self.operation_thread and self.operation_thread.is_alive():
            return

        self.operation_thread = threading.Thread(target=self._execute_operation)
        self.operation_thread.daemon = True
        self.operation_thread.start()

    def _validate_operation(self) -> Optional[HSMError]:
        """Validate operation parameters."""
        # Check for injected errors first
        if HSMError.ERR0_RESERVED in self.injected_errors:
            return HSMError.ERR0_RESERVED

        if self.operation_mode in [HSMMode.ECB, HSMMode.CBC]:
            # ECB/CBC mode validations
            if self.text_length == 0:
                return HSMError.ERR2_ZERO_LENGTH
            if self.text_length % 16 != 0:
                return HSMError.ERR1_LENGTH_NOT_ALIGNED_16
            if self.key_index > 18:
                return HSMError.ERR5_KEYINDEX_TOO_LARGE
            if self.key_index in [3, 4, 5, 6, 7]:  # Auth keys
                return HSMError.ERR4_AUTH_KEY_FOR_ECB_CBC

        elif self.operation_mode == HSMMode.CMAC:
            # CMAC mode validations
            if self.key_index > 18:
                return HSMError.ERR7_KEYINDEX_TOO_LARGE_CMAC
            if self.key_index != 18:  # Only RAM key allowed for CMAC
                return HSMError.ERR6_CMAC_KEY_WRONG

        elif self.operation_mode == HSMMode.AUTH:
            # Authentication mode validations
            if self.key_index < 3 or self.key_index > 7:
                return HSMError.ERR8_AUTH_KEY_WRONG

        elif self.operation_mode == HSMMode.RND:
            # Random number generation validations
            if self.text_length == 0:
                return HSMError.ERR10_RND_ZERO_LENGTH
            if self.text_length % 4 != 0:
                return HSMError.ERR9_RND_LENGTH_NOT_ALIGNED_4

        return None

    def _set_error(self, error: HSMError) -> None:
        """Set error status."""
        self.error_status |= (1 << error.value)
        self.is_busy = False
        self._trigger_interrupt_if_enabled()

    def _execute_operation(self) -> None:
        """Execute HSM operation in background thread."""
        try:
            with self.operation_lock:
                if self.operation_mode == HSMMode.ECB:
                    self._execute_aes_ecb()
                elif self.operation_mode == HSMMode.CBC:
                    self._execute_aes_cbc()
                elif self.operation_mode == HSMMode.CMAC:
                    self._execute_aes_cmac()
                elif self.operation_mode == HSMMode.AUTH:
                    self._execute_authentication()
                elif self.operation_mode == HSMMode.RND:
                    self._execute_random_generation()

                self.operation_complete = True
                self.is_busy = False
                self._trigger_interrupt_if_enabled()

        except Exception as e:
            self.trace_manager.log_device_event(
                self.name, self.name, "OPERATION_ERROR",
                {"error": str(e), "mode": self.operation_mode.name}
            )
            self.is_busy = False

    def _get_key_bytes(self, key_index: int) -> bytes:
        """Get key bytes for given key index."""
        if key_index == 18:  # RAM key
            key_bytes = bytearray()
            for i in range(4):
                key_bytes.extend(self.ram_keys[i].to_bytes(4, 'little'))
            return bytes(key_bytes)
        elif key_index in self.memory_keys:
            return self.memory_keys[key_index]
        else:
            # Return dummy key for unknown indices
            return b'\x00' * 16

    def _execute_aes_ecb(self) -> None:
        """Execute AES ECB operation."""
        key_bytes = self._get_key_bytes(self.key_index)
        cipher = AES.new(key_bytes, AES.MODE_ECB)

        # Simulate processing 16-byte blocks
        bytes_processed = 0
        while bytes_processed < self.text_length:
            # Wait for input data
            while len(self.input_fifo) < 4:  # Need 16 bytes (4 words)
                time.sleep(0.001)

            # Get 16 bytes from input FIFO
            input_block = bytearray()
            for _ in range(4):
                if self.input_fifo:
                    word = self.input_fifo.pop(0)
                    input_block.extend(word.to_bytes(4, 'little'))

            # Pad if necessary
            while len(input_block) < 16:
                input_block.append(0)

            # Encrypt or decrypt
            if self.decrypt_enable:
                output_block = cipher.decrypt(bytes(input_block))
            else:
                output_block = cipher.encrypt(bytes(input_block))

            # Add to output FIFO
            for i in range(0, 16, 4):
                word = int.from_bytes(output_block[i:i+4], 'little')
                self.output_fifo.append(word)

            bytes_processed += 16

    def _execute_aes_cbc(self) -> None:
        """Execute AES CBC operation."""
        key_bytes = self._get_key_bytes(self.key_index)

        # Get IV from IV registers
        iv_bytes = bytearray()
        for i in range(4):
            iv_bytes.extend(self.iv_data[i].to_bytes(4, 'little'))

        cipher = AES.new(key_bytes, AES.MODE_CBC, bytes(iv_bytes))

        # Process data similar to ECB but with CBC mode
        bytes_processed = 0
        input_data = bytearray()

        # Collect all input data first for CBC
        while bytes_processed < self.text_length:
            while len(self.input_fifo) < 4 and bytes_processed < self.text_length:
                time.sleep(0.001)

            for _ in range(4):
                if self.input_fifo and bytes_processed < self.text_length:
                    word = self.input_fifo.pop(0)
                    input_data.extend(word.to_bytes(4, 'little'))
                    bytes_processed = min(bytes_processed + 4, self.text_length)

        # Pad to 16-byte boundary
        while len(input_data) % 16 != 0:
            input_data.append(0)

        # Encrypt/decrypt
        if self.decrypt_enable:
            output_data = cipher.decrypt(bytes(input_data))
        else:
            output_data = cipher.encrypt(bytes(input_data))

        # Add to output FIFO
        for i in range(0, len(output_data), 4):
            word = int.from_bytes(output_data[i:i+4], 'little')
            self.output_fifo.append(word)

    def _execute_aes_cmac(self) -> None:
        """Execute AES CMAC operation."""
        key_bytes = self._get_key_bytes(self.key_index)

        # Collect all input data
        input_data = bytearray()
        bytes_processed = 0

        while bytes_processed < self.text_length:
            while len(self.input_fifo) == 0 and bytes_processed < self.text_length:
                time.sleep(0.001)

            if self.input_fifo:
                word = self.input_fifo.pop(0)
                remaining = self.text_length - bytes_processed
                if remaining >= 4:
                    input_data.extend(word.to_bytes(4, 'little'))
                    bytes_processed += 4
                else:
                    # Handle last partial word
                    word_bytes = word.to_bytes(4, 'little')
                    input_data.extend(word_bytes[:remaining])
                    bytes_processed += remaining

        # Generate CMAC
        cobj = CMAC.new(key_bytes, ciphermod=AES)
        cobj.update(bytes(input_data))
        mac_result = cobj.digest()

        # Add MAC result to output FIFO
        for i in range(0, 16, 4):
            word = int.from_bytes(mac_result[i:i+4], 'little')
            self.output_fifo.append(word)

    def _execute_authentication(self) -> None:
        """Execute authentication (challenge-response)."""
        # Generate random challenge
        challenge = secrets.token_bytes(16)

        # Store challenge in CHA registers
        for i in range(4):
            self.challenge_data[i] = int.from_bytes(challenge[i*4:(i+1)*4], 'little')

        # Set ACV bit in AUTH_STAT register
        self.auth_status |= (1 << 16)  # Set challenge valid bit

    def _process_authentication_response(self) -> None:
        """Process authentication response."""
        # Get response from RSP registers
        response_bytes = bytearray()
        for i in range(4):
            response_bytes.extend(self.response_data[i].to_bytes(4, 'little'))

        # Get challenge
        challenge_bytes = bytearray()
        for i in range(4):
            challenge_bytes.extend(self.challenge_data[i].to_bytes(4, 'little'))

        # Compute expected CMAC
        key_bytes = self._get_key_bytes(self.key_index)
        cobj = CMAC.new(key_bytes, ciphermod=AES)
        cobj.update(bytes(challenge_bytes))
        expected_mac = cobj.digest()

        # Compare with response
        if bytes(response_bytes) == expected_mac:
            self.auth_status |= (1 << 17)  # Set auth valid bit
        else:
            self.auth_status &= ~(1 << 17)  # Clear auth valid bit

    def _execute_random_generation(self) -> None:
        """Execute random number generation."""
        bytes_generated = 0
        while bytes_generated < self.text_length:
            # Generate 4 random bytes
            random_word = secrets.randbits(32)
            self.output_fifo.append(random_word)
            bytes_generated += 4

    def _trigger_interrupt_if_enabled(self) -> None:
        """Trigger interrupt if enabled."""
        # Check if IRQ is enabled in SYS_CTRL register
        sys_ctrl_value = self.register_manager.registers[0x018].value
        if sys_ctrl_value & (1 << 1):  # IRQEN bit
            if self.bus:
                self.bus.send_irq(self.master_id, self.IRQ_NUMBER)
                self.trace_manager.log_device_event(
                    self.name, self.name, "INTERRUPT_TRIGGERED",
                    {"irq_number": self.IRQ_NUMBER}
                )

    def _process_input_data(self) -> None:
        """Process input data if operation is running."""
        # This method can be extended to handle streaming data processing
        pass

    # Error injection interface
    def inject_error(self, error_type: Union[HSMError, str]) -> None:
        """Inject an error for fault testing."""
        if isinstance(error_type, str):
            try:
                error_type = HSMError[error_type.upper()]
            except KeyError:
                raise ValueError(f"Unknown error type: {error_type}")

        self.injected_errors.add(error_type)
        self.trace_manager.log_device_event(
            self.name, self.name, "ERROR_INJECTED",
            {"error_type": error_type.name}
        )

    def clear_injected_errors(self) -> None:
        """Clear all injected errors."""
        self.injected_errors.clear()
        self.trace_manager.log_device_event(
            self.name, self.name, "ERRORS_CLEARED", {}
        )

    def get_status(self) -> Dict[str, Any]:
        """Get current HSM status."""
        return {
            "busy": self.is_busy,
            "mode": self.operation_mode.name if hasattr(self.operation_mode, 'name') else str(self.operation_mode),
            "key_index": self.key_index,
            "text_length": self.text_length,
            "error_status": self.error_status,
            "input_fifo_size": len(self.input_fifo),
            "output_fifo_size": len(self.output_fifo),
            "dma_enabled": self.dma_enabled
        }

    # DMA interface methods
    def enable_dma_mode(self, channel_id: int = 0) -> None:
        """Enable DMA mode for this HSM device."""
        self.dma_interface.enable_dma(channel_id)
        self.dma_enabled = True

    def disable_dma_mode(self) -> None:
        """Disable DMA mode for this HSM device."""
        self.dma_interface.disable_dma()
        self.dma_enabled = False

    def is_dma_ready(self) -> bool:
        """Check if HSM is ready for DMA operations."""
        return self.dma_interface.is_dma_ready()