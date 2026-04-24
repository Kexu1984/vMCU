"""
Flash device implementation for the DevCommV3 framework.

This module provides a Flash memory device that supports:
- Non-volatile storage with file-based backing
- Lock/unlock mechanism with key sequence
- Page erase, mass erase, and page program operations
- Write/read protection mechanisms
- ECC functionality simulation
- Info area and main storage area support
- Command-based operation through control registers
- Error injection for testing purposes
"""

import os
import time
import threading
from enum import Enum
from typing import Dict, Any, Optional, Callable, Union
import logging
import traceback

# Import base classes from the framework
import sys
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))

from devcomm.core.base_device import BaseDevice
from devcomm.core.registers import RegisterType


class FlashCommand(Enum):
    """Flash command types."""
    PAGE_ERASE = 0x01
    OPTION_PAGE_ERASE = 0x02
    MASS_ERASE = 0x03

    PAGE_PROGRAM = 0x11
    OPTION_PAGE_PROGRAM = 0x12

    PAGE_VERIFY = 0x21
    MASS_VERIFY = 0x22


class FlashState(Enum):
    """Flash operation states."""
    IDLE = "idle"
    BUSY = "busy"
    LOCKED = "locked"
    ERROR = "error"


class FlashErrorType(Enum):
    """Flash error types for injection."""
    OPERATION_PASS = "operation_pass"
    P_WRITE_PROTECTION_VIOLATION = "pflash_write_protection_violation"
    D_WRITE_PROTECTION_VIOLATION = "dflash_write_protection_violation"
    READ_PROTECTION_VIOLATION = "read_protection_violation"
    VERIFY_ERR = "verify_error"
    PROGRAM_ERROR = "program_error by fault CAR address"
    ERASE_ERROR = "erase_error by fault CAR address"
    ACCERR = "access_error by fault CAR address"
    ECC_SINGLE_BIT_ERROR = "ecc_single_bit_error"
    ECC_DOUBLE_BIT_ERROR = "ecc_double_bit_error"
    COMMAND_ERROR = "command_error"



class FlashMemory:
    """
    Flash memory class for managing file-based non-volatile storage.
    Handles the actual flash memory data with file backing.
    """

    def __init__(self, dev, name: str, memory_size: int, page_size: int,
                 programming_unit: int, storage_file: Optional[str] = None):
        """
        Initialize flash memory.

        Args:
            name: Memory name (PFlash, DFlash, Info)
            memory_size: Memory size in bytes
            page_size: Page size for erase operations
            programming_unit: Minimum programming unit size in bits
            storage_file: Optional file path for persistent storage
        """
        self.dev = dev
        self.name = name
        self.memory_size = memory_size
        self.page_size = page_size
        self.programming_unit = programming_unit  # In bits (64 or 128)
        self.storage_file = storage_file

        # Initialize memory data
        self.memory_data = bytearray(memory_size)
        self._initialize_memory()

        # Protection state
        self.write_protection = [False] * 64  # 64 protection regions
        self.read_protection_enabled = False

        self._lock = threading.RLock()

    def _initialize_memory(self):
        """Initialize flash memory with appropriate default values."""
        if self.storage_file and os.path.exists(self.storage_file):
            # Load from existing file
            with open(self.storage_file, 'rb') as f:
                data = f.read()
                self.memory_data[:len(data)] = data
        else:
            # Initialize with 0xFF (erased state)
            for i in range(len(self.memory_data)):
                self.memory_data[i] = 0xFF

        # Special initialization for Info area
        if self.name == "Info" and len(self.memory_data) >= 8:
            # Set default read protection bytes (0xFFFFFFFFFF53FFAC)
            # This means read protection is ENABLED by default
            self.memory_data[0:8] = [0xAC, 0xFF, 0x53, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF]

    def read(self, address: int, size: int = 4) -> bytes:
        """Read from flash memory."""
        with self._lock:
            if address + size > len(self.memory_data):
                raise ValueError(f"Memory read out of bounds: 0x{address:08X}+{size}")

            # Check read protection
            if self.read_protection_enabled and self.name != "Info":
                raise PermissionError(f"Read protection enabled for {self.name}")

            return bytes(self.memory_data[address:address + size])

    def write(self, address: int, data: bytes) -> None:
        """
        Write to flash memory directly (for testing/initialization only).
        Normal flash writes should go through flash controller commands.
        """
        with self._lock:
            if address + len(data) > len(self.memory_data):
                raise ValueError(f"Memory write out of bounds: 0x{address:08X}+{len(data)}")

            # Flash programming can only change 1->0, not 0->1
            for i, byte_val in enumerate(data):
                self.memory_data[address + i] &= byte_val

            self._save_to_file()

    def erase_page(self, address: int) -> None:
        """Erase a flash page (set to 0xFF)."""
        with self._lock:
            # Check write protection
            if self._is_write_protected(address):
                raise PermissionError(f"Page at 0x{address:08X} is write protected")

            page_start = (address // self.page_size) * self.page_size
            page_end = page_start + self.page_size

            if page_end <= len(self.memory_data):
                for i in range(page_start, page_end):
                    self.memory_data[i] = 0xFF

            self._save_to_file()

            return

    def erase_mass(self) -> None:
        """Erase entire flash memory."""
        with self._lock:
            # Check write protection for entire memory
            for addr in range(0, self.memory_size, self.page_size):
                if self._is_write_protected(addr):
                    raise PermissionError(f"Flash has write protected regions")

            # Erase all memory
            for i in range(len(self.memory_data)):
                self.memory_data[i] = 0xFF

            self._save_to_file()

    def program(self, address: int, data: bytes) -> None:
        """Program flash memory (can only change 1->0)."""
        with self._lock:
            # Check write protection
            if self._is_write_protected(address):
                raise PermissionError(f"Address 0x{address:08X} is write protected")

            if address + len(data) > len(self.memory_data):
                raise ValueError(f"Program out of bounds: 0x{address:08X}+{len(data)}")

            # Flash programming: can only change 1->0 (AND operation)
            for i, byte_val in enumerate(data):
                self.memory_data[address + i] &= byte_val

            self._save_to_file()

    def _is_write_protected(self, address: int) -> bool:
        """Check if address is write protected."""
        region = int(address / self.page_size)
        self.dev.trace_manager.log_device_event(self.dev.name, self.name, "check_write_protection",
                                               {"flash": self.name,
                                                "address": f"0x{address:08X}", "region": region,
                                                "protected": self.write_protection[region]})
        return self.write_protection[region]

    def set_write_protection(self, protection_bits: int, is_upper: bool = False) -> None:
        """Set write protection bits."""
        offset = 32 if is_upper else 0
        for i in range(32):
            if protection_bits & (1 << i):
                self.write_protection[offset + i] = True
                self.dev.trace_manager.log_device_event(self.dev.name, self.name, "add_write_protection",
                                               {"flash": self.name,
                                                "reg": f"0x{protection_bits:08X}",
                                                "region": i + offset})

    def _save_to_file(self) -> None:
        """Save memory content to backing file."""
        if self.storage_file:
            try:
                with open(self.storage_file, 'wb') as f:
                    f.write(self.memory_data)
            except Exception as e:
                logging.error(f"Failed to save {self.name} memory to file: {e}")


class FlashController:
    """
    Flash controller class for managing registers and commands.
    Controls the flash memories through command interface.
    """

    def __init__(self, name: str):
        """Initialize flash controller."""
        self.name = name
        self.flash_state = FlashState.LOCKED
        self.unlock_sequence_step = 0
        self.command_busy = False
        self.current_command = None
        self.command_address = 0
        self.command_length = 0

        # Error injection for testing
        self.error_injection_enabled = False
        self.injected_errors = {}

        self._lock = threading.RLock()

    def is_unlocked(self) -> bool:
        """Check if controller is unlocked."""
        return self.flash_state != FlashState.LOCKED

    def process_unlock_key(self, key: int) -> None:
        """Process unlock key sequence."""
        with self._lock:
            if self.unlock_sequence_step == 0 and key == 0x00AC7805:
                self.unlock_sequence_step = 1
            elif self.unlock_sequence_step == 1 and key == 0x01234567:
                self.unlock_sequence_step = 2
                self.flash_state = FlashState.IDLE
            else:
                self.unlock_sequence_step = 0
                self.flash_state = FlashState.LOCKED

    def lock(self) -> None:
        """Lock the flash controller."""
        with self._lock:
            self.flash_state = FlashState.LOCKED
            self.unlock_sequence_step = 0


class FlashDevice(BaseDevice):
    """
    Flash memory device implementation.

    This device manages multiple address spaces:
    - Flash controller registers: 0x40002000 ~ 0x40003000
    - PFlash memory: 0x08000000 ~ 0x08040000 (256KB)
    - DFlash memory: 0x08100000 ~ 0x08120000 (128KB)
    - Info flash memory: 0x08200000 ~ 0x08202000 (12KB)
    """

    def __init__(self, name: str, base_address: int, size: int, master_id: int, extra_space: Optional[list] = None):
        """
        Initialize Flash device with multiple address spaces.

        Args:
            name: Device name
            base_address: Register space base address (should be 0x40002000)
            size: Register space size (should be 0x1000)
            master_id: Device master ID
            extra_space: List of (base_address, size) tuples for memory spaces
        """
        # Initialize flash controller and memories
        self.flash_controller = FlashController(f"{name}_Controller")
        self.data_len = 0

        # Create flash memory instances with appropriate storage files
        storage_dir = os.path.join("./", name.lower())
        os.makedirs(storage_dir, exist_ok=True)

        self.pflash = FlashMemory(
            self,
            "PFlash",
            256 * 1024,  # 256KB
            4096,        # 4K page size
            128,         # 128-bit programming unit
            os.path.join(storage_dir, "pflash.bin")
        )

        self.dflash = FlashMemory(
            self,
            "DFlash",
            128 * 1024,  # 128KB
            2048,        # 2K page size
            128,         # 128-bit programming unit
            os.path.join(storage_dir, "dflash.bin")
        )

        # Handle info area creation based on legacy parameter
        self.info_flash = FlashMemory(
            self,
            "Info",
            12 * 1024,   # 12KB
            4096,        # 4K page size
            64,          # 64-bit programming unit
            os.path.join(storage_dir, "info.bin")
        )

        # Define address spaces
        if extra_space is None:
            extra_space = [
                (0x08000000, 0x08040000),  # PFlash: 256KB
                (0x08100000, 0x08120000),  # DFlash: 128KB
                (0x08200000, 0x08202000),  # Info: 12KB
            ]

        # Store legacy parameters for backward compatibility
        self._legacy_memory_size = 0
        self._legacy_page_size = 0
        self._legacy_is_info_area = True
        self._legacy_storage_file = 0

        # Initialize base device with extra address spaces
        super().__init__(name, base_address, size, master_id, extra_space)

    def init(self):
        """Initialize Flash device registers."""
        # FMUKR - Flash Memory Unlock Register (0x2000)
        self.register_manager.define_register(
            0x0000, "FMUKR", RegisterType.WRITE_ONLY, 0x00000000, 0xFFFFFFFF,
            write_callback=self._unlock_callback
        )

        # FMGSR - Flash Memory General Status Register (0x2004)
        self.register_manager.define_register(
            0x0004, "FMGSR", RegisterType.READ_WRITE, 0x00000001, 0xFFFFFFFF,  # LOCK bit set
            read_callback=self._status_read_callback,
            write_callback=self._lock_callback
        )

        # FMGCR - Flash Memory General Control Register (0x2008)
        self.register_manager.define_register(
            0x0008, "FMGCR", RegisterType.READ_WRITE, 0x00000000, 0x0FFF0F01
        )

        # FMCSR - Flash Memory Command Status Register (0x200C)
        # Contains various status and error flags per YAML specification
        self.register_manager.define_register(
            0x000C, "FMCSR", RegisterType.READ_WRITE, 0x00000000, 0x003FF77F,
            read_callback=self._status_register_read_callback,
            write_callback=self._status_register_write_callback
        )

        # FMCCR - Flash Memory Command Control Register (0x2010)
        self.register_manager.define_register(
            0x0010, "FMCCR", RegisterType.READ_WRITE, 0x00000000, 0x00FFFFFF,
            write_callback=self._command_control_callback
        )

        # FMADDR - Flash Memory Address Register (0x2014)
        self.register_manager.define_register(
            0x0014, "FMADDR", RegisterType.READ_WRITE, 0x00000000, 0xFFFFFFFF
        )

        # FMDATA0 - Flash Memory Data Register 0 (0x2018)
        self.register_manager.define_register(
            0x0018, "FMDATA0", RegisterType.READ_WRITE, 0x00000000, 0xFFFFFFFF
        )

        # FMDATA1 - Flash Memory Data Register 1 (0x201C)
        self.register_manager.define_register(
            0x001C, "FMDATA1", RegisterType.READ_WRITE, 0x00000000, 0xFFFFFFFF,
            write_callback=self._fmdata1_write_callback
        )

        # Write Protection Registers
        # FMPFWPR0/1 - PFlash Write Protection Registers (0x2020, 0x2024)
        self.register_manager.define_register(
            0x0020, "FMPFWPR0", RegisterType.READ_WRITE, 0x00000000, 0xFFFFFFFF,
            write_callback=self._pf_write_protection_callback
        )
        self.register_manager.define_register(
            0x0024, "FMPFWPR1", RegisterType.READ_WRITE, 0x00000000, 0xFFFFFFFF,
            write_callback=self._pf_write_protection_callback
        )

        # FMDFWPR0/1 - DFlash Write Protection Registers (0x2028, 0x202C)
        self.register_manager.define_register(
            0x0028, "FMDFWPR0", RegisterType.READ_WRITE, 0x00000000, 0xFFFFFFFF,
            write_callback=self._df_write_protection_callback
        )
        self.register_manager.define_register(
            0x002C, "FMDFWPR1", RegisterType.READ_WRITE, 0x00000000, 0xFFFFFFFF,
            write_callback=self._df_write_protection_callback
        )

        # ECC Error Address Registers (RW1C - Read/Write 1 to Clear per YAML)
        # FMPFECADR - PFlash ECC Error Address Register (0x2030)
        self.register_manager.define_register(
            0x0030, "FMPFECADR", RegisterType.READ_WRITE, 0x00000000, 0xFFFFFFFF,
            write_callback=self._ecc_error_address_write_callback
        )

        # FMDFECADR - DFlash ECC Error Address Register (0x2034)
        self.register_manager.define_register(
            0x0034, "FMDFECADR", RegisterType.READ_WRITE, 0x00000000, 0xFFFFFFFF,
            write_callback=self._ecc_error_address_write_callback
        )

        # Error Injection Mask Registers
        # FMPFEIM0/1 - PFlash Error Injection Mask Registers (0x2040, 0x2044)
        self.register_manager.define_register(
            0x0040, "FMPFEIM0", RegisterType.READ_WRITE, 0x00000000, 0xFFFFFFFF
        )
        self.register_manager.define_register(
            0x0044, "FMPFEIM1", RegisterType.READ_WRITE, 0x00000000, 0xFFFFFFFF
        )

        # FMDFEIM0/1 - DFlash Error Injection Mask Registers (0x2050, 0x2054)
        self.register_manager.define_register(
            0x0050, "FMDFEIM0", RegisterType.READ_WRITE, 0x00000000, 0xFFFFFFFF
        )
        self.register_manager.define_register(
            0x0054, "FMDFEIM1", RegisterType.READ_WRITE, 0x00000000, 0xFFFFFFFF
        )

        # Additional registers from yaml file
        self._initialize_additional_registers()

        # Handle default write protection config during system reset
        self._initialize_default_writeprotection()

    def _initialize_default_writeprotection(self):
        # setup default pflash write protection from info area
        self.pflash_wp_idx_lower = int.from_bytes(self.info_flash.memory_data[0x1000:0x1004], byteorder="little")  # 小端
        for bit in range(32):
            if self.pflash_wp_idx_lower & (1 << bit):
                self.pflash.write_protection[bit] = True

        self.pflash_wp_idx_upper = int.from_bytes(self.info_flash.memory_data[0x1008:0x100C], byteorder="little")  # 小端
        for bit in range(32):
            if self.pflash_wp_idx_upper & (1 << bit):
                self.pflash.write_protection[bit + 32] = True

        # setup default dflash write protection from info area
        self.dflash_wp_idx_lower = int.from_bytes(self.info_flash.memory_data[0x1010:0x1014], byteorder="little")  # 小端
        for bit in range(32):
            if self.dflash_wp_idx_lower & (1 << bit):
                self.dflash.write_protection[bit] = True

        self.dflash_wp_idx_upper = int.from_bytes(self.info_flash.memory_data[0x1018:0x101C], byteorder="little")  # 小端
        for bit in range(32):
            if self.dflash_wp_idx_upper & (1 << bit):
                self.dflash.write_protection[bit + 32] = True

        self.trace_manager.log_device_event(self.name, self.name, "init_write_protection",
                                              {"pflash_lower": f"0x{self.pflash_wp_idx_lower:08X}",
                                               "pflash_upper": f"0x{self.pflash_wp_idx_upper:08X}",
                                               "dflash_lower": f"0x{self.dflash_wp_idx_lower:08X}",
                                               "dflash_upper": f"0x{self.dflash_wp_idx_upper:08X}"})

    def _initialize_additional_registers(self):
        """Initialize remaining registers from the YAML specification."""
        # FMLVD - Flash Memory LVD Register (0x2060)
        self.register_manager.define_register(
            0x0060, "FMLVD", RegisterType.READ_WRITE, 0x00000000, 0x00000003
        )

        # FMKEY - Flash Memory Key Register (0x2064)
        self.register_manager.define_register(
            0x0064, "FMKEY", RegisterType.READ_WRITE, 0x00000000, 0x0000003F
        )

        # Info/ID Registers (Read-Only)
        # FMCNR - Chip Name Register (0x2070)
        self.register_manager.define_register(
            0x0070, "FMCNR", RegisterType.READ_ONLY, 0x464C5348, 0x00FFFFFF  # "FLSH"
        )

        # UUID Registers
        self.register_manager.define_register(
            0x0074, "FMCIR0", RegisterType.READ_ONLY, 0x12345678, 0xFFFFFFFF
        )
        self.register_manager.define_register(
            0x0078, "FMCIR1", RegisterType.READ_ONLY, 0x9ABCDEF0, 0xFFFFFFFF
        )

        self.register_manager.define_register(
            0x0080, "FMCIR2", RegisterType.READ_ONLY, 0x55667788, 0xFFFFFFFF
        )

        # Package Info Register
        self.register_manager.define_register(
            0x0084, "FMPKG", RegisterType.READ_ONLY, 0x000000A5, 0x000000FF
        )

        # FMINFO_MJST - Flash Memory INFO Major Register (0x2088)
        self.register_manager.define_register(
            0x0088, "FMINFO_MJST", RegisterType.READ_ONLY, 0x00000000, 0xFFFFFFFF
        )

        # FMDBG - Flash Memory Debug Register (0x208C)
        self.register_manager.define_register(
            0x008C, "FMDBG", RegisterType.READ_WRITE, 0x00000000, 0x0000001F
        )

        # FMSPCLK - Flash Special Clock Register (0x2090)
        self.register_manager.define_register(
            0x0090, "FMSPCLK", RegisterType.READ_WRITE, 0x00000000, 0xFFFFFFFF
        )

        # PFlash Special Memory Write Timing Registers
        self.register_manager.define_register(
            0x00A0, "FMPF_SMWTIME", RegisterType.READ_WRITE, 0x00000000, 0x007FFFFF
        )
        self.register_manager.define_register(
            0x00A4, "FMPF_SMWCNT", RegisterType.READ_WRITE, 0x00000000, 0x0001FFFF
        )

        # DFlash Special Memory Write Timing Registers
        self.register_manager.define_register(
            0x00A8, "FMDF_SMWTIME", RegisterType.READ_WRITE, 0x00000000, 0x007FFFFF
        )
        self.register_manager.define_register(
            0x00AC, "FMDF_SMWCNT", RegisterType.READ_WRITE, 0x00000000, 0x0001FFFF
        )

        # PFlash Special Memory Programming/Erase High Voltage Registers
        self.register_manager.define_register(
            0x00B0, "FMPF_SMPWHV0", RegisterType.READ_WRITE, 0x00000000, 0xFFFFFFFF
        )
        self.register_manager.define_register(
            0x00B4, "FMPF_SMPWHV1", RegisterType.READ_WRITE, 0x00000000, 0xFFFFFFFF
        )
        self.register_manager.define_register(
            0x00B8, "FMPF_SMEWHV0", RegisterType.READ_WRITE, 0x00000000, 0xFFFFFFFF
        )
        self.register_manager.define_register(
            0x00BC, "FMPF_SMEWHV1", RegisterType.READ_WRITE, 0x00000000, 0xFFFFFFFF
        )

        # DFlash Special Memory Programming/Erase High Voltage Registers
        self.register_manager.define_register(
            0x00C0, "FMDF_SMPWHV0", RegisterType.READ_WRITE, 0x00000000, 0xFFFFFFFF
        )
        self.register_manager.define_register(
            0x00C4, "FMDF_SMPWHV1", RegisterType.READ_WRITE, 0x00000000, 0xFFFFFFFF
        )
        self.register_manager.define_register(
            0x00C8, "FMDF_SMEWHV0", RegisterType.READ_WRITE, 0x00000000, 0xFFFFFFFF
        )
        self.register_manager.define_register(
            0x00CC, "FMDF_SMEWHV1", RegisterType.READ_WRITE, 0x00000000, 0xFFFFFFFF
        )

    def _unlock_callback(self, device, offset: int, value: int) -> None:
        """Handle unlock sequence writes to FMUKR register."""
        self.flash_controller.process_unlock_key(value)

        if self.flash_controller.unlock_sequence_step == 1:
            self.trace_manager.log_device_event(self.name, self.name, "flash_unlock_step1",
                                              {"key": f"0x{value:08X}"})
        elif self.flash_controller.unlock_sequence_step == 2:
            self.trace_manager.log_device_event(self.name, self.name, "flash_unlocked",
                                              {"key": f"0x{value:08X}"})
        else:
            self.trace_manager.log_device_event(self.name, self.name, "flash_unlock_failed",
                                              {"key": f"0x{value:08X}"})

    def _lock_callback(self, device, offset: int, value: int) -> None:
        """Handle unlock sequence writes to FMUKR register."""
        if value & 0x1:
            self.flash_controller.lock()
            self.trace_manager.log_device_event(self.name, self.name, "flash_lock",
                                              {"lockvalue": f"0x{value:08X}"})

    def _status_read_callback(self, device, offset: int, value: int) -> int:
        """Update status register on read."""
        # Update status bits based on current state
        status_value = 0

        # LOCK bit (bit 0)
        if self.flash_controller.flash_state == FlashState.LOCKED:
            status_value |= 0x01

        # BUSY bit (bit 1)
        if self.flash_controller.command_busy:
            status_value |= 0x02

        # FMRPRT bit (bit 2) - Read protection status
        if self.info_flash.read_protection_enabled:
            status_value |= 0x04

        # WAKEUP_EN bit (bit 3) - implementation specific

        return status_value

    def _status_register_read_callback(self, device, offset: int, value: int) -> int:
        """Update FMCSR status register bits on read."""
        # Get current register value
        fmcsr_reg = self.register_manager.registers[0x000C]
        current_value = fmcsr_reg.value

        # Update dynamic status bits
        # CMDBUSY bit (bit 0)
        if self.flash_controller.command_busy:
            current_value |= 0x01
        else:
            current_value &= ~0x01

        # Update the register value
        fmcsr_reg.value = current_value
        return current_value

    def _status_register_write_callback(self, device, offset: int, value: int) -> None:
        """Handle writes to FMCSR register - clear status bits."""
        fmcsr_reg = self.register_manager.registers[0x000C]
        current_value = fmcsr_reg.value

        # Clear bits that are set in the write value (write-1-to-clear for most flags)
        # Preserve read-only bits like CMDBUSY, DBUFRDY, DBUFLST
        read_only_mask = 0x07  # bits 0, 1, 2 are more dynamic

        # Clear error flags when written with 1
        if value & (1 << 21):  # FMCCF
            current_value &= ~(1 << 21)
        if value & (1 << 20):  # FMCCFTF
            current_value &= ~(1 << 20)
        if value & (1 << 19):  # HSMRVIOF
            current_value &= ~(1 << 19)
        if value & (1 << 18):  # OPTDWERR
            current_value &= ~(1 << 18)
        if value & (1 << 17):  # OPTPWERR
            current_value &= ~(1 << 17)
        if value & (1 << 16):  # OPTRERR
            current_value &= ~(1 << 16)
        if value & (1 << 11):  # ACCERR
            current_value &= ~(1 << 11)
        if value & (1 << 9):   # PGMERR
            current_value &= ~(1 << 9)
        if value & (1 << 8):   # VRFERR
            current_value &= ~(1 << 8)
        if value & (1 << 6):   # RVIO (RW1C per YAML)
            current_value &= ~(1 << 6)
        if value & (1 << 5):   # DWVIO
            current_value &= ~(1 << 5)
        if value & (1 << 4):   # PWVIO
            current_value &= ~(1 << 4)

        # ECC status fields (bits 15:12) - clear when written
        if value & 0xF000:
            current_value &= ~0xF000

        fmcsr_reg.value = current_value

    def _ecc_error_address_write_callback(self, device, offset: int, value: int) -> None:
        """Handle writes to ECC error address registers (RW1C behavior)."""
        if offset == 0x0030:  # FMPFECADR
            reg = self.register_manager.registers[0x0030]
        elif offset == 0x0034:  # FMDFECADR
            reg = self.register_manager.registers[0x0034]
        else:
            return

        # RW1C behavior: writing 1 clears the corresponding bit
        current_value = reg.value
        clear_mask = value  # bits written as 1 will be cleared
        new_value = current_value & ~clear_mask
        reg.value = new_value

    def _set_fmcsr_flag(self, flag_name: str, value: bool) -> None:
        """Set or clear a flag in the FMCSR register."""
        fmcsr_reg = self.register_manager.registers[0x000C]
        current_value = fmcsr_reg.value

        # Define bit positions for each flag
        flag_bits = {
            "FMCCF": 21,     # Flash command completion flag
            "FMCCFTF": 20,   # Command conflict flag
            "HSMRVIOF": 19,  # Option byte DATAx verification error
            "OPTDWERR": 18,  # Option byte D_WPRT_EN verification error
            "OPTPWERR": 17,  # Option byte P_WPRT_EN verification error
            "OPTRERR": 16,   # Option byte RDP verification error
            "DECCSTA": 14,   # DFlash ECC status (2 bits: 15:14)
            "PECCSTA": 12,   # PFlash ECC status (2 bits: 13:12)
            "ACCERR": 11,    # Address error
            "PGMERR": 9,     # Programming error
            "VRFERR": 8,     # Verification error
            "RVIO": 6,       # Read protection violation
            "DWVIO": 5,      # DFlash write protection violation
            "PWVIO": 4,      # PFlash write protection violation
            "DBUFLST": 2,    # Data buffer last
            "DBUFRDY": 1,    # Data buffer ready
            "CMDBUSY": 0     # Command busy
        }

        if flag_name == "DBUFRDY":
            self.trace_manager.log_device_event(self.name, self.name, "DBUFRDY_flag_set",
                                              {"value": value})

        if flag_name in flag_bits:
            bit_pos = flag_bits[flag_name]
            if value:
                current_value |= (1 << bit_pos)
            else:
                current_value &= ~(1 << bit_pos)
            fmcsr_reg.value = current_value

    def _command_control_callback(self, device, offset: int, value: int) -> None:
        """Handle command control register writes."""
        if not self.flash_controller.is_unlocked():
            self.trace_manager.log_device_event(self.name, self.name, "flash_command_locked",
                                              {"command": f"0x{value:08X}"})
            return

        cmd_start = (value >> 0) & 1
        cmd_abort = (value >> 1) & 1
        cmd_opten = (value >> 2) & 1
        cmd_type = (value >> 4) & 0xFF
        cmd_len = (value >> 12) & 0xFFF

        if cmd_abort:
            self._abort_command()
        elif cmd_start and not self.flash_controller.command_busy:
            self._start_command(cmd_type, cmd_len, cmd_opten)

    def _fmdata1_write_callback(self, device, offset: int, value: int) -> None:
        """Handle fmdata1 register writes."""
        self._set_fmcsr_flag("DBUFRDY", False)  # Clear data buffer ready flag & it will be set after command finished
        self.data_len += 8  # User input 8bytes (2x 32-bit registers)

    def _setup_command_status(self, status: bool) -> None:
        '''Set up command status before starting a command.'''
        self.flash_controller.command_busy = status
        self._set_fmcsr_flag("CMDBUSY", status)  # Set command busy flag
        command_name = self.flash_controller.current_command.name if self.flash_controller.current_command else "None"

        if status == False:
            self.flash_controller.current_command = None
            self._set_fmcsr_flag("FMCCF", True)  # Command completion flag
            self._set_fmcsr_flag("DBUFRDY", True)  # Set data buffer ready flag when command completes
            # Clear START bit in FMCCR
            self.register_manager.registers[0x0010].value = self.register_manager.registers[0x0010].value & ~0x1


        self.trace_manager.log_device_event(self.name, self.name, "Command Status",
                                            {"command": command_name,
                                             "busy": self.flash_controller.command_busy,
                                             "fmcrs": f"0x{self.register_manager.registers[0x000C].value:08X}"})

    def _start_command(self, cmd_type: int, cmd_len: int, cmd_opten: bool) -> None:
        """Start flash command execution."""
        try:
            command = FlashCommand(cmd_type)
            self.flash_controller.current_command = command
            self.flash_controller.command_length = cmd_len

            # Get command address
            addr_reg = self.register_manager.registers[0x0014]  # FMADDR
            self.flash_controller.command_address = addr_reg.value

            self.trace_manager.log_device_event(self.name, self.name, "flash_command_start",
                                              {"command": command.name, "address": f"0x{self.flash_controller.command_address:08X}",
                                               "length": cmd_len, "opten": cmd_opten})

            # Execute command in background thread
            threading.Thread(target=self._execute_command, daemon=True).start()

        except ValueError:
            self.trace_manager.log_device_event(self.name, self.name, "flash_command_invalid",
                                              {"command_type": f"0x{cmd_type:02X}"})

    def _execute_command(self) -> None:
        """Execute flash command (runs in background thread)."""
        command_name = self.flash_controller.current_command.name if self.flash_controller.current_command else "Unknown"

        # Set CMDBUSY flag in FMCSR
        self._setup_command_status(True)
        flash_memory = self._get_flash_for_address(self.flash_controller.command_address)
        try:
            if self.flash_controller.current_command == FlashCommand.PAGE_ERASE:
                self._execute_page_erase()
            elif self.flash_controller.current_command == FlashCommand.OPTION_PAGE_ERASE:
                self._execute_page_erase()
            elif self.flash_controller.current_command == FlashCommand.MASS_ERASE:
                self._execute_mass_erase()
            elif self.flash_controller.current_command == FlashCommand.PAGE_PROGRAM:
                self._execute_page_program()
            elif self.flash_controller.current_command == FlashCommand.OPTION_PAGE_PROGRAM:
                self._execute_page_program()
            elif self.flash_controller.current_command == FlashCommand.PAGE_VERIFY:
                self._execute_page_verify()
            elif self.flash_controller.current_command == FlashCommand.MASS_VERIFY:
                self._execute_page_verify()
            else:
                raise ValueError(f"Unsupported command: {self.flash_controller.current_command}")

            # Command completed successfully
            self.register_manager.registers[0x10].value = self.register_manager.registers[0x10].value & ~0x1
            self.trace_manager.log_device_event(self.name, self.name, "flash_command_complete",
                                          {"command": command_name})

        except PermissionError as e:
            # Handle write protection violations
            print(f"DEBUG: PermissionError caught in _execute_command: {e}")
            try:
                if "write protected" in str(e).lower():
                    if flash_memory.name == "PFlash":
                        self._set_fmcsr_flag("PWVIO", True)  # PFlash write protection violation
                    elif flash_memory.name == "DFlash":
                        self._set_fmcsr_flag("DWVIO", True)  # DFlash write protection violation
                    else:
                        os._exit(1)

                self.trace_manager.log_device_event(self.name, self.name, "flash_command_protection_error",
                                                  {"command": command_name, "error": str(e)})
            except Exception as nested_e:
                print(f"DEBUG: Nested exception in PermissionError handler: {nested_e}")

        except ValueError as e:
            # Handle address errors
            print(f"DEBUG: ValueError caught in _execute_command: {e}")
            try:
                if "address" in str(e).lower() or "out of bounds" in str(e).lower():
                    self._set_fmcsr_flag("ACCERR", True)  # Address error
                self.trace_manager.log_device_event(self.name, self.name, "flash_command_address_error",
                                                  {"command": command_name, "error": str(e)})
            except Exception as nested_e:
                print(f"DEBUG: Nested exception in ValueError handler: {nested_e}")

        except Exception as e:
            # General command error
            print(f"DEBUG: General exception caught in _execute_command: {e}")
            print(f"DEBUG: Exception type: {type(e)}")
            print(f"DEBUG: Traceback: {traceback.format_exc()}")
            try:
                self._set_fmcsr_flag("PGMERR", True)  # Programming error (general)
                self.trace_manager.log_device_event(self.name, self.name, "flash_command_error",
                                                  {"command": command_name, "error": str(e)})
            except Exception as nested_e:
                print(f"DEBUG: Nested exception in general exception handler: {nested_e}")

        finally:
            # 使用finally块确保这段代码一定会执行
            print(f"DEBUG: Finally block executing - setting command status to False")
            try:
                # Set command completion flag in FMCSR
                self._setup_command_status(False)
                print(f"DEBUG: Command status set to False successfully")
            except Exception as final_e:
                print(f"DEBUG: Exception in finally block: {final_e}")
                # 即使_setup_command_status失败，也要确保基本的清理
                try:
                    self.flash_controller.command_busy = False
                    self._set_fmcsr_flag("CMDBUSY", False)
                    self.flash_controller.current_command = None
                    self._set_fmcsr_flag("FMCCF", True)  # Command completion flag
                except Exception as cleanup_e:
                    print(f"DEBUG: Exception in cleanup: {cleanup_e}")

    def _execute_page_erase(self) -> None:
        """Execute page erase operation."""
        address = self.flash_controller.command_address
        flash_memory = self._get_flash_for_address(address)

        # Simulate timing
        time.sleep(0.001)  # 1ms erase time

        # Convert absolute address to flash-relative address
        relative_address = self._get_relative_address(address)
        flash_memory.erase_page(relative_address)

    def _execute_mass_erase(self) -> None:
        """Execute mass erase operation."""
        address = self.flash_controller.command_address
        flash_memory = self._get_flash_for_address(address)

        # Simulate timing
        time.sleep(0.01)  # 10ms erase time

        flash_memory.erase_mass()

    def _execute_page_program(self) -> None:
        """Execute page program operation."""
        address = self.flash_controller.command_address
        flash_memory = self._get_flash_for_address(address)

        csr_reg = self.register_manager.registers[0x000C]  # FMCSR
        len_reg = self.register_manager.registers[0x0010]  # FMCCR
        length = ((len_reg.value >> 12) & 0xFFF);  # Length in D-Words

        relative_address = self._get_relative_address(address)
        while length > 0:
            while self.data_len == 0:
                time.sleep(0.0001)  # Wait for data to be written
            # Get program data from registers
            data0_reg = self.register_manager.registers[0x0018]  # FMDATA0
            data1_reg = self.register_manager.registers[0x001C]  # FMDATA1
            self.data_len -= 8  # Consumed 8 bytes (2x 32-bit registers)

            # Program data (64-bit units as specified in docs)
            prog_data = []
            prog_data.extend(data0_reg.value.to_bytes(4, 'little'))
            prog_data.extend(data1_reg.value.to_bytes(4, 'little'))

            # Simulate timing
            # time.sleep(0.0005)  # 0.5ms program time

            # handle info area read protection enabling
            if flash_memory == self.info_flash:
                # Check if the programmed data enables read protection
                if relative_address >= 0x20:
                    raise ValueError("Info area programming offset out of bounds:" .format(relative_address))

                if relative_address % 8 != 0:
                    raise ValueError("Info area programming offset must be 8-byte aligned:" .format(relative_address))

            # Convert absolute address to flash-relative address
            flash_memory.program(relative_address, bytes(prog_data))

            # Move to next programming unit
            relative_address += len(prog_data)
            length -= 1  # Each loop programs 2 D-Words (64 bits)
            self._set_fmcsr_flag("DBUFRDY", True)  # Set data buffer ready flag


    def _execute_page_verify(self) -> None:
        """Execute page verify operation."""
        address = self.flash_controller.command_address
        flash_memory = self._get_flash_for_address(address)

        # Simulate timing
        time.sleep(0.0001)  # 0.1ms verify time

        # Convert absolute address to flash-relative address
        relative_address = self._get_relative_address(address)

        # Verify implementation would check if page is erased
        page_start = (relative_address // flash_memory.page_size) * flash_memory.page_size
        page_end = min(page_start + flash_memory.page_size, len(flash_memory.memory_data))

        for i in range(page_start, page_end):
            if flash_memory.memory_data[i] != 0xFF:
                raise Exception("Verify failed")

    def _abort_command(self) -> None:
        """Abort current flash command."""
        if self.flash_controller.command_busy:
            self._setup_command_status(False)
            self.flash_controller.current_command = None
            # Clear START bit in FMCCR
            self.register_manager.registers[0x10].value = self.register_manager.registers[0x10].value & ~0x1
            self.trace_manager.log_device_event(self.name, self.name, "flash_command_aborted", {})

    def _get_flash_for_address(self, address: int) -> FlashMemory:
        """Get the appropriate flash memory for a given address."""
        if 0x08000000 <= address < 0x08040000:  # PFlash
            return self.pflash
        elif 0x08100000 <= address < 0x08120000:  # DFlash
            return self.dflash
        elif 0x08200000 <= address < 0x08202000:  # Info
            return self.info_flash
        else:
            raise ValueError(f"Invalid flash memory address: 0x{address:08X}")

    def _get_relative_address(self, address: int) -> int:
        """Convert absolute address to flash-relative address."""
        if 0x08000000 <= address < 0x08040000:  # PFlash
            return address - 0x08000000
        elif 0x08100000 <= address < 0x08120000:  # DFlash
            return address - 0x08100000
        elif 0x08200000 <= address < 0x08202000:  # Info
            return address - 0x08200000
        else:
            raise ValueError(f"Invalid flash memory address: 0x{address:08X}")

    def _pf_write_protection_callback(self, device, offset: int, value: int) -> None:
        """Handle PFlash write protection register updates."""
        if offset == 0x0020:  # FMPFWPR0
            self.pflash.set_write_protection(value, is_upper=False)
        elif offset == 0x0024:  # FMPFWPR1
            self.pflash.set_write_protection(value, is_upper=True)

    def _df_write_protection_callback(self, device, offset: int, value: int) -> None:
        """Handle DFlash write protection register updates."""
        if offset == 0x0028:  # FMDFWPR0
            self.dflash.set_write_protection(value, is_upper=False)
        elif offset == 0x002C:  # FMDFWPR1
            self.dflash.set_write_protection(value, is_upper=True)

    def _read_implementation(self, offset: int, width: int) -> int:
        """
        Read from flash device - handles both registers and memory spaces.

        Register space (0x40002000-0x40003000): Use register manager
        Memory spaces: Direct memory access
        """
        # print("###FlashDevice: _read_implementation called with offset 0x{0:08X}, width {1}".format(offset, width))
        # For register space, use register manager
        if offset < 0:
            absolute_address = offset + self.base_address
            if not self.is_address_in_range(absolute_address):
                raise ValueError(f"Read offset 0x{offset:08X} out of range")
        else:
            if offset + self.base_address < self.base_address + self.size:
                return self.register_manager.read_register(self, offset, width)
            else:
                raise ValueError(f"Read offset 0x{offset:08X} out of range")

        # Check if it's in one of the memory spaces
        flash_memory = self._get_flash_for_address(absolute_address)
        relative_address = self._get_relative_address(absolute_address)
        data = flash_memory.read(relative_address, width)

        # Convert bytes to integer (little-endian)
        if width == 1:
            return data[0]
        elif width == 2:
            return int.from_bytes(data[:2], 'little')
        else:
            return int.from_bytes(data[:4], 'little')

        #except ValueError:
        #    # Address not in any known flash memory space - delegate to parent
        #    return super()._read_implementation(offset, width)

    def _write_implementation(self, offset: int, value: int, width: int) -> None:
        """
        Write to flash device - only registers allowed, memory writes raise exception.

        Register space (0x40002000-0x40003000): Use register manager
        Memory spaces: Raise exception as required
        """
        # print("###FlashDevice: _write_implementation called with offset 0x{0:08X}, width {1}".format(offset, width))
        # For register space, use register manager
        if offset < 0:
            absolute_address = offset + self.base_address
            if not self.is_address_in_range(absolute_address):
                raise ValueError(f"Read offset 0x{offset:08X} out of range")
        else:
            if offset + self.base_address < self.base_address + self.size:
                self.register_manager.write_register(self, offset, value, width)
                return
            else:
                raise ValueError(f"Read offset 0x{offset:08X} out of range")

        # For memory spaces, raise exception as per requirements
        absolute_address = self.base_address + offset

        # Check if it's in one of the memory spaces

        flash_memory = self._get_flash_for_address(absolute_address)
        # Memory writes not allowed - must use flash commands
        raise PermissionError(f"Direct write to flash memory at 0x{absolute_address:08X} not allowed. Use flash commands instead.")

    def read_memory_direct(self, address: int, size: int = 4) -> bytes:
        """
        Direct memory read for testing and debugging.
        This bypasses normal access controls.
        """
        try:
            flash_memory = self._get_flash_for_address(address)
            relative_address = self._get_relative_address(address)
            return flash_memory.read(relative_address, size)
        except ValueError:
            raise ValueError(f"Invalid flash memory address: 0x{address:08X}")

    def write_memory_direct(self, address: int, data: bytes) -> None:
        """
        Direct memory write for testing and initialization.
        This bypasses normal access controls and write protection.
        """
        try:
            flash_memory = self._get_flash_for_address(address)
            relative_address = self._get_relative_address(address)
            flash_memory.write(relative_address, data)
        except ValueError:
            raise ValueError(f"Invalid flash memory address: 0x{address:08X}")

    # Legacy compatibility methods for existing tests
    @property
    def memory_size(self) -> int:
        """Legacy property for backward compatibility."""
        if self._legacy_memory_size:
            return self._legacy_memory_size
        return self.pflash.memory_size

    @property
    def page_size(self) -> int:
        """Legacy property for backward compatibility."""
        if self._legacy_page_size:
            return self._legacy_page_size
        return self.pflash.page_size

    @property
    def is_info_area(self) -> bool:
        """Legacy property for backward compatibility."""
        return self._legacy_is_info_area if self._legacy_is_info_area is not None else False

    @property
    def flash_state(self) -> FlashState:
        """Legacy property for backward compatibility."""
        return self.flash_controller.flash_state

    @property
    def command_busy(self) -> bool:
        """Legacy property for backward compatibility."""
        return self.flash_controller.command_busy

    @property
    def current_command(self):
        """Legacy property for backward compatibility."""
        return self.flash_controller.current_command

    @property
    def unlock_sequence_step(self) -> int:
        """Legacy property for backward compatibility."""
        return self.flash_controller.unlock_sequence_step

    @property
    def memory_data(self) -> bytearray:
        """Legacy property for backward compatibility - returns appropriate flash data."""
        if self.is_info_area:
            return self.info_flash.memory_data
        else:
            return self.pflash.memory_data

    @property
    def write_protection_pf(self) -> list:
        """Legacy property for backward compatibility."""
        return self.pflash.write_protection

    @property
    def write_protection_df(self) -> list:
        """Legacy property for backward compatibility."""
        return self.dflash.write_protection

    @property
    def error_injection_enabled(self) -> bool:
        """Legacy property for backward compatibility."""
        return self.flash_controller.error_injection_enabled

    @property
    def injected_errors(self) -> dict:
        """Legacy property for backward compatibility."""
        return self.flash_controller.injected_errors

    def read_memory(self, address: int, size: int = 4) -> bytes:
        """Legacy method for backward compatibility."""
        # For legacy tests, treat as PFlash relative address
        data = self.pflash.read(address, size)

        # Apply error injection if enabled (for backward compatibility)
        if self.flash_controller.error_injection_enabled and address in self.flash_controller.injected_errors:
            error_type = self.flash_controller.injected_errors[address]
            if error_type == FlashErrorType.ECC_SINGLE_BIT_ERROR:
                # Flip one bit in the data
                data_array = bytearray(data)
                if len(data_array) > 0:
                    data_array[0] ^= 0x01  # Flip LSB
                return bytes(data_array)
            elif error_type == FlashErrorType.ECC_DOUBLE_BIT_ERROR:
                # Flip two bits in the data
                data_array = bytearray(data)
                if len(data_array) > 0:
                    data_array[0] ^= 0x03  # Flip two LSBs
                return bytes(data_array)

        return data

    def write_memory(self, address: int, data: bytes) -> None:
        """Legacy method for backward compatibility."""
        # For legacy tests, treat as PFlash relative address
        self.pflash.write(address, data)

    # Error injection methods for testing
    def enable_error_injection(self, address: int, error_type: FlashErrorType) -> None:
        """Enable error injection at specific address."""
        self.flash_controller.error_injection_enabled = True
        self.flash_controller.injected_errors[address] = error_type
        self.trace_manager.log_device_event(self.name, self.name, "error_injection_enabled",
                                          {"address": f"0x{address:08X}", "error_type": error_type.value})

    def disable_error_injection(self, address: int = 0) -> None:
        """Disable error injection (all or specific address)."""
        if address is None:
            self.flash_controller.injected_errors.clear()
            self.flash_controller.error_injection_enabled = False
        else:
            self.flash_controller.injected_errors.pop(address, None)
            if not self.flash_controller.injected_errors:
                self.flash_controller.error_injection_enabled = False

        self.trace_manager.log_device_event(self.name, self.name, "error_injection_disabled",
                                          {"address": f"0x{address:08X}" if address else "all"})

    def get_device_status(self) -> Dict[str, Any]:
        """Get comprehensive device status."""
        return {
            **self.get_device_info(),
            'flash_state': self.flash_controller.flash_state.value,
            'command_busy': self.flash_controller.command_busy,
            'current_command': self.flash_controller.current_command.name if self.flash_controller.current_command else None,
            'memory_size': self.pflash.memory_size,
            'page_size': self.pflash.page_size,
            'is_info_area': False,
            'unlock_sequence_step': self.flash_controller.unlock_sequence_step,
            'error_injection_enabled': self.flash_controller.error_injection_enabled,
            'injected_error_count': len(self.flash_controller.injected_errors),
            'pflash_size': self.pflash.memory_size,
            'dflash_size': self.dflash.memory_size,
            'info_flash_size': self.info_flash.memory_size
        }

    def lock_flash(self) -> None:
        """Lock the flash device."""
        with self.lock:
            self.flash_controller.lock()
            # Set LOCK bit in status register
            status_reg = self.register_manager.registers[0x0004]  # FMGSR
            status_reg.value |= 0x01
            self.trace_manager.log_device_event(self.name, self.name, "flash_locked", {})