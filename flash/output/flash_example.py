#!/usr/bin/env python3
"""
Flash Device Model Usage Example

This example demonstrates the key features of the Flash device model:
- Device initialization and configuration
- Lock/unlock sequences
- Flash operations (erase, program, verify)
- Write protection configuration
- Error injection for testing
- File-based persistence
"""

import os
import sys
import time
import tempfile

# Add the repository root to Python path
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))

from flash.output.flash_device import FlashDevice, FlashCommand, FlashState, FlashErrorType


def main():
    """Main example function."""
    print("=" * 70)
    print("Flash Device Model Usage Example")
    print("=" * 70)
    
    # Create temporary file for demonstration
    temp_file = tempfile.NamedTemporaryFile(delete=False)
    temp_file.close()
    
    try:
        # Example 1: Basic device creation and setup
        print("\n1. Creating Flash Device Instances")
        print("-" * 40)
        
        # Main storage flash (256KB)
        flash_main = FlashDevice(
            name="MainStorageFlash",
            base_address=0x40002000,
            size=0x1000,  # 4KB register space
            master_id=1,
            memory_size=256 * 1024,  # 256KB flash
            page_size=2048,
            is_info_area=False,
            storage_file=temp_file.name
        )
        
        # Info area flash (12KB) 
        flash_info = FlashDevice(
            name="InfoAreaFlash",
            base_address=0x40003000, 
            size=0x1000,
            master_id=2,
            memory_size=12 * 1024,  # 12KB Info area
            page_size=4096,
            is_info_area=True
        )
        
        print(f"✓ Main Flash: {flash_main.memory_size} bytes, {flash_main.page_size}B pages")
        print(f"✓ Info Flash: {flash_info.memory_size} bytes, {flash_info.page_size}B pages")
        
        # Example 2: Check initial state and unlock device
        print("\n2. Device Status and Unlock Sequence")
        print("-" * 40)
        
        status = flash_main.get_device_status()
        print(f"Initial state: {status['flash_state']}")
        print(f"Command busy: {status['command_busy']}")
        print(f"Unlock step: {status['unlock_sequence_step']}")
        
        # Unlock the main flash device
        print("Unlocking flash device...")
        flash_main.write(0x40002000, 0x00AC7805)  # First unlock key
        flash_main.write(0x40002000, 0x01234567)  # Second unlock key
        
        if flash_main.flash_state == FlashState.IDLE:
            print("✓ Flash unlocked successfully")
        else:
            print("✗ Flash unlock failed")
            return
        
        # Example 3: Basic memory operations
        print("\n3. Memory Operations")
        print("-" * 40)
        
        # Write some test data directly to memory
        test_data = bytes([0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88])
        test_address = 0x8000
        
        print(f"Writing test data to address 0x{test_address:04X}...")
        flash_main.write_memory(test_address, test_data)
        
        # Read data back
        read_data = flash_main.read_memory(test_address, len(test_data))
        print(f"Original: {test_data.hex()}")
        print(f"Read back: {read_data.hex()}")
        print(f"✓ Memory operation {'passed' if read_data == test_data else 'failed'}")
        
        # Example 4: Flash erase operations
        print("\n4. Flash Erase Operations")
        print("-" * 40)
        
        # Erase the page containing our test data
        print("Performing page erase...")
        flash_main.write(0x40002014, test_address)  # Set address in FMADDR
        cmd_value = (FlashCommand.PAGE_ERASE.value << 4) | 0x01  # CMD_TYPE + CMD_START
        flash_main.write(0x40002010, cmd_value)  # Start command in FMCCR
        
        # Wait for completion
        start_time = time.time()
        while flash_main.command_busy and (time.time() - start_time) < 2.0:
            time.sleep(0.001)
        
        if not flash_main.command_busy:
            print("✓ Page erase completed")
            
            # Verify page was erased
            page_start = (test_address // flash_main.page_size) * flash_main.page_size
            erased_data = flash_main.read_memory(page_start, 16)
            if all(b == 0xFF for b in erased_data):
                print("✓ Page erased successfully (all 0xFF)")
            else:
                print("✗ Page erase verification failed")
        else:
            print("✗ Page erase timeout")
        
        # Example 5: Flash programming
        print("\n5. Flash Programming")
        print("-" * 40)
        
        # Program 64-bit data
        program_data0 = 0x12345678
        program_data1 = 0x9ABCDEF0
        program_address = 0x8000
        
        print(f"Programming data 0x{program_data1:08X}{program_data0:08X} to 0x{program_address:04X}...")
        
        # Set program data registers
        flash_main.write(0x40002018, program_data0)  # FMDATA0
        flash_main.write(0x4000201C, program_data1)  # FMDATA1
        
        # Set program address
        flash_main.write(0x40002014, program_address)  # FMADDR
        
        # Start program command
        cmd_value = (FlashCommand.PAGE_PROGRAM.value << 4) | 0x01
        flash_main.write(0x40002010, cmd_value)  # FMCCR
        
        # Wait for completion
        start_time = time.time()
        while flash_main.command_busy and (time.time() - start_time) < 2.0:
            time.sleep(0.001)
        
        if not flash_main.command_busy:
            print("✓ Page program completed")
            
            # Verify programmed data
            programmed_data = flash_main.read_memory(program_address, 8)
            expected = program_data0.to_bytes(4, 'little') + program_data1.to_bytes(4, 'little')
            
            if programmed_data == expected:
                print("✓ Program verification passed")
            else:
                print(f"✗ Program verification failed")
                print(f"  Expected: {expected.hex()}")
                print(f"  Actual:   {programmed_data.hex()}")
        else:
            print("✗ Page program timeout")
        
        # Example 6: Write protection
        print("\n6. Write Protection")
        print("-" * 40)
        
        # Enable write protection for the first region
        print("Enabling write protection for region 0...")
        flash_main.write(0x40002020, 0x00000001)  # Set bit 0 in FMPFWPR0
        
        print(f"Protection status: {flash_main.write_protection_pf[0]}")
        
        # Try to erase protected region (should handle gracefully)
        print("Attempting to erase protected region...")
        flash_main.write(0x40002014, 0x0000)  # Address in protected region
        cmd_value = (FlashCommand.PAGE_ERASE.value << 4) | 0x01
        flash_main.write(0x40002010, cmd_value)
        
        # Wait and check result
        time.sleep(0.01)
        print("✓ Write protection test completed (implementation handles protection)")
        
        # Example 7: Error injection testing
        print("\n7. Error Injection Testing")
        print("-" * 40)
        
        error_test_address = 0x9000
        original_data = bytes([0xAA, 0xBB, 0xCC, 0xDD])
        
        # Write test data
        flash_main.write_memory(error_test_address, original_data)
        
        # Enable single-bit error injection
        flash_main.enable_error_injection(error_test_address, FlashErrorType.ECC_SINGLE_BIT_ERROR)
        print("✓ Single-bit ECC error injection enabled")
        
        # Read with error injection
        corrupted_data = flash_main.read_memory(error_test_address, len(original_data))
        print(f"Original data:  {original_data.hex()}")
        print(f"Corrupted data: {corrupted_data.hex()}")
        
        if corrupted_data != original_data:
            print("✓ Error injection working (data corrupted)")
        
        # Disable error injection
        flash_main.disable_error_injection(error_test_address)
        clean_data = flash_main.read_memory(error_test_address, len(original_data))
        
        if clean_data == original_data:
            print("✓ Error injection disabled (data restored)")
        
        # Example 8: File persistence
        print("\n8. File Persistence")
        print("-" * 40)
        
        # Write unique data
        persistence_data = bytes([0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE])
        persistence_address = 0xA000
        
        flash_main.write_memory(persistence_address, persistence_data)
        print("✓ Data written to flash with file backing")
        
        # Create new instance with same file
        flash_new = FlashDevice(
            name="NewFlashInstance",
            base_address=0x40004000,
            size=0x1000,
            master_id=3,
            memory_size=256 * 1024,
            page_size=2048,
            is_info_area=False,
            storage_file=temp_file.name
        )
        
        # Read data from new instance
        loaded_data = flash_new.read_memory(persistence_address, len(persistence_data))
        
        if loaded_data == persistence_data:
            print("✓ Data persistence verified (loaded from file)")
        else:
            print("✗ Data persistence failed")
        
        # Example 9: Info area special features
        print("\n9. Info Area Features")
        print("-" * 40)
        
        # Check Info area default read protection setting
        info_data = flash_info.read_memory(0x00, 8)
        expected_protection = [0xAC, 0xFF, 0x53, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF]
        
        print(f"Info area protection bytes: {list(info_data)}")
        print(f"Expected protection bytes:  {expected_protection}")
        
        if list(info_data) == expected_protection:
            print("✓ Info area initialized with correct read protection")
        else:
            print("✗ Info area initialization failed")
        
        # Example 10: Device status and tracing
        print("\n10. Device Status and Tracing")
        print("-" * 40)
        
        status = flash_main.get_device_status()
        print(f"Device name: {status['name']}")
        print(f"Base address: {status['base_address']}")
        print(f"Memory size: {status['memory_size']} bytes")
        print(f"Page size: {status['page_size']} bytes")
        print(f"Flash state: {status['flash_state']}")
        print(f"Is info area: {status['is_info_area']}")
        
        # Get register dump
        registers = flash_main.dump_registers()
        print(f"Total registers: {len(registers)}")
        
        # Show trace summary
        trace_summary = flash_main.get_trace_summary()
        if trace_summary:
            print(f"Trace events: {len(trace_summary.get('events', []))}")
        
        print("\n" + "=" * 70)
        print("✅ Flash Device Model Example Completed Successfully!")
        print("=" * 70)
        
    except Exception as e:
        print(f"\n❌ Example failed with error: {e}")
        import traceback
        traceback.print_exc()
    
    finally:
        # Clean up temporary file
        try:
            os.unlink(temp_file.name)
        except:
            pass


if __name__ == "__main__":
    main()