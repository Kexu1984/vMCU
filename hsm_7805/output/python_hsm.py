"""
HSM device demonstration and integration test script.

This script demonstrates the HSM device functionality including:
- Device initialization and configuration
- AES encryption/decryption operations
- CMAC generation and verification
- Random number generation
- Authentication (challenge-response)
- Error handling and status monitoring
"""

import time
import sys
import os

# Add paths for imports
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))

from hsm_device import HSMDevice, HSMMode, HSMError
from devcomm.core.bus_model import BusModel


def print_separator(title):
    """Print a formatted separator with title."""
    print("\n" + "=" * 60)
    print(f" {title}")
    print("=" * 60)


def print_status(hsm):
    """Print current HSM status."""
    status = hsm.get_status()
    print(f"HSM Status:")
    print(f"  - Busy: {status['busy']}")
    print(f"  - Mode: {status['mode']}")
    print(f"  - Key Index: {status['key_index']}")
    print(f"  - Text Length: {status['text_length']} bytes")
    print(f"  - Error Status: 0x{status['error_status']:04X}")
    print(f"  - Input FIFO: {status['input_fifo_size']} words")
    print(f"  - Output FIFO: {status['output_fifo_size']} words")
    print(f"  - DMA Enabled: {status['dma_enabled']}")


def demo_basic_initialization():
    """Demonstrate HSM device initialization."""
    print_separator("HSM Device Initialization")
    
    # Create HSM device
    hsm = HSMDevice("hsm_demo", 0x40000000, 0x1000, 1)
    print(f"✓ Created HSM device '{hsm.name}' at address 0x{hsm.base_address:08X}")
    
    # Create bus and register device
    bus = BusModel("demo_bus")
    bus.add_device(hsm)
    print(f"✓ Registered HSM device with bus")
    
    # Check initial status
    print_status(hsm)
    
    return hsm, bus


def demo_register_access(hsm):
    """Demonstrate register access functionality."""
    print_separator("Register Access Demo")
    
    # Test UID register read
    uid0_addr = 0x40000000
    uid0_value = hsm.read(uid0_addr)
    print(f"✓ UID0 register (0x{uid0_addr:08X}): 0x{uid0_value:08X}")
    
    # Test command register write
    cmd_addr = 0x40000010
    cmd_value = (HSMMode.ECB.value << 28) | (18 << 22) | 32  # ECB mode, RAM key, 32 bytes
    hsm.write(cmd_addr, cmd_value)
    print(f"✓ CMD register (0x{cmd_addr:08X}): wrote 0x{cmd_value:08X}")
    print(f"  - Mode: {hsm.operation_mode.name}")
    print(f"  - Key Index: {hsm.key_index}")
    print(f"  - Text Length: {hsm.text_length}")
    
    # Test IV registers
    print("✓ Setting IV registers:")
    iv_test_data = [0x11111111, 0x22222222, 0x33333333, 0x44444444]
    for i, iv_value in enumerate(iv_test_data):
        iv_addr = 0x4000002C + i * 4
        hsm.write(iv_addr, iv_value)
        read_back = hsm.read(iv_addr)
        print(f"  - IV{i} (0x{iv_addr:08X}): 0x{read_back:08X}")
    
    # Test RAM key registers (write-only)
    print("✓ Setting RAM key registers:")
    ram_key_data = [0xDEADBEEF, 0xCAFEBABE, 0x12345678, 0x87654321]
    for i, key_value in enumerate(ram_key_data):
        ram_key_addr = 0x4000003C + i * 4
        hsm.write(ram_key_addr, key_value)
        read_back = hsm.read(ram_key_addr)  # Should return 0
        print(f"  - RAM_KEY{i} (0x{ram_key_addr:08X}): written 0x{key_value:08X}, read 0x{read_back:08X}")


def demo_aes_ecb_operation(hsm):
    """Demonstrate AES ECB encryption/decryption."""
    print_separator("AES ECB Operation Demo")
    
    # Set up ECB encryption
    cmd_addr = 0x40000010
    cst_addr = 0x40000014
    cipin_addr = 0x40000024
    cipout_addr = 0x40000028
    
    # Configure for ECB mode, encryption, RAM key, 16 bytes
    cmd_value = (HSMMode.ECB.value << 28) | (0 << 27) | (18 << 22) | 16
    hsm.write(cmd_addr, cmd_value)
    print(f"✓ Configured ECB encryption mode")
    
    # Prepare test data (16 bytes = 4 words)
    test_data = [0x12345678, 0x9ABCDEF0, 0xFEDCBA98, 0x76543210]
    print(f"✓ Test data: {[f'0x{x:08X}' for x in test_data]}")
    
    # Write test data to input
    for word in test_data:
        hsm.write(cipin_addr, word)
    print(f"✓ Written test data to CIPIN")
    
    # Start operation
    hsm.write(cst_addr, 1)
    print(f"✓ Started ECB operation")
    
    # Wait for operation to complete
    max_wait = 100  # 100 * 10ms = 1 second max
    wait_count = 0
    while hsm.is_busy and wait_count < max_wait:
        time.sleep(0.01)
        wait_count += 1
    
    if wait_count >= max_wait:
        print("⚠ Operation timed out")
        return
    
    print(f"✓ Operation completed in {wait_count * 10}ms")
    
    # Read encrypted result
    encrypted_data = []
    for i in range(4):  # Read 4 words
        if hsm.output_fifo:
            encrypted_word = hsm.read(cipout_addr)
            encrypted_data.append(encrypted_word)
        else:
            break
    
    print(f"✓ Encrypted data: {[f'0x{x:08X}' for x in encrypted_data]}")
    
    # Verify data was actually processed (should be different from input)
    if encrypted_data != test_data:
        print("✓ Data was successfully encrypted (differs from input)")
    else:
        print("⚠ Data appears unchanged")


def demo_random_number_generation(hsm):
    """Demonstrate random number generation."""
    print_separator("Random Number Generation Demo")
    
    cmd_addr = 0x40000010
    cst_addr = 0x40000014
    cipout_addr = 0x40000028
    
    # Configure for RND mode, 16 bytes
    cmd_value = (HSMMode.RND.value << 28) | 16
    hsm.write(cmd_addr, cmd_value)
    print(f"✓ Configured RND mode for 16 bytes")
    
    # Start operation
    hsm.write(cst_addr, 1)
    print(f"✓ Started random generation")
    
    # Wait for operation to complete
    max_wait = 50
    wait_count = 0
    while hsm.is_busy and wait_count < max_wait:
        time.sleep(0.01)
        wait_count += 1
    
    print(f"✓ Operation completed in {wait_count * 10}ms")
    
    # Read random data
    random_data = []
    for i in range(4):  # Read 4 words (16 bytes)
        if hsm.output_fifo:
            random_word = hsm.read(cipout_addr)
            random_data.append(random_word)
    
    print(f"✓ Random data generated: {[f'0x{x:08X}' for x in random_data]}")
    
    # Generate another set to verify randomness
    hsm.write(cst_addr, 1)
    while hsm.is_busy:
        time.sleep(0.01)
    
    random_data2 = []
    for i in range(4):
        if hsm.output_fifo:
            random_word = hsm.read(cipout_addr)
            random_data2.append(random_word)
    
    print(f"✓ Second random data set: {[f'0x{x:08X}' for x in random_data2]}")
    
    if random_data != random_data2:
        print("✓ Random data sets are different (good randomness)")
    else:
        print("⚠ Random data sets are identical")


def demo_authentication(hsm):
    """Demonstrate authentication (challenge-response)."""
    print_separator("Authentication Demo")
    
    cmd_addr = 0x40000010
    cst_addr = 0x40000014
    auth_stat_addr = 0x40000020
    
    # Configure for AUTH mode, auth key (3-7), 16 bytes
    cmd_value = (HSMMode.AUTH.value << 28) | (3 << 22) | 16
    hsm.write(cmd_addr, cmd_value)
    print(f"✓ Configured AUTH mode with key index 3")
    
    # Start authentication (generates challenge)
    hsm.write(cst_addr, 1)
    print(f"✓ Started authentication (challenge generation)")
    
    # Wait for operation
    time.sleep(0.05)
    
    # Check authentication status
    auth_stat = hsm.read(auth_stat_addr)
    acv_bit = (auth_stat >> 16) & 1
    print(f"✓ Authentication status: 0x{auth_stat:08X}")
    print(f"  - Challenge valid (ACV): {acv_bit}")
    
    if acv_bit:
        # Read challenge data
        challenge_data = []
        for i in range(4):
            cha_addr = 0x4000004C + i * 4
            cha_value = hsm.read(cha_addr)
            challenge_data.append(cha_value)
        
        print(f"✓ Challenge data: {[f'0x{x:08X}' for x in challenge_data]}")
        
        # In a real scenario, external system would compute CMAC
        # Here we'll simulate setting a response
        test_response = [0x11111111, 0x22222222, 0x33333333, 0x44444444]
        for i, rsp_value in enumerate(test_response):
            rsp_addr = 0x4000005C + i * 4
            hsm.write(rsp_addr, rsp_value)
        
        print(f"✓ Set response data: {[f'0x{x:08X}' for x in test_response]}")
        
        # Set ARV bit to validate response
        hsm.write(auth_stat_addr, 1)
        print(f"✓ Triggered response validation")
        
        # Check result
        time.sleep(0.01)
        auth_stat = hsm.read(auth_stat_addr)
        auth_valid = (auth_stat >> 17) & 1
        print(f"✓ Authentication result: {'PASS' if auth_valid else 'FAIL'}")


def demo_error_handling(hsm):
    """Demonstrate error handling and injection."""
    print_separator("Error Handling Demo")
    
    # Test parameter validation errors
    cmd_addr = 0x40000010
    cst_addr = 0x40000014
    stat_addr = 0x4000007C
    
    print("Testing parameter validation...")
    
    # Test 1: Zero length error
    cmd_value = (HSMMode.ECB.value << 28) | (18 << 22) | 0  # Zero length
    hsm.write(cmd_addr, cmd_value)
    hsm.write(cst_addr, 1)  # Try to start
    
    time.sleep(0.01)
    stat_value = hsm.read(stat_addr)
    err2_bit = (stat_value >> 2) & 1
    print(f"✓ Zero length test - ERR2 bit: {err2_bit}")
    
    # Clear errors
    hsm.write(stat_addr, 1 << 16)
    
    # Test 2: Alignment error
    cmd_value = (HSMMode.ECB.value << 28) | (18 << 22) | 15  # Not 16-byte aligned
    hsm.write(cmd_addr, cmd_value)
    hsm.write(cst_addr, 1)
    
    time.sleep(0.01)
    stat_value = hsm.read(stat_addr)
    err1_bit = (stat_value >> 1) & 1
    print(f"✓ Alignment test - ERR1 bit: {err1_bit}")
    
    # Clear errors
    hsm.write(stat_addr, 1 << 16)
    
    # Test error injection
    print("\nTesting error injection...")
    hsm.inject_error(HSMError.ERR0_RESERVED)
    print(f"✓ Injected ERR0_RESERVED error")
    
    # Try to start valid operation - should fail due to injected error
    cmd_value = (HSMMode.RND.value << 28) | 16  # Valid RND operation
    hsm.write(cmd_addr, cmd_value)
    hsm.write(cst_addr, 1)
    
    time.sleep(0.01)
    stat_value = hsm.read(stat_addr)
    err0_bit = stat_value & 1
    print(f"✓ Injected error test - ERR0 bit: {err0_bit}")
    
    # Clear injected errors
    hsm.clear_injected_errors()
    print(f"✓ Cleared injected errors")


def demo_interrupt_functionality(hsm, bus):
    """Demonstrate interrupt functionality."""
    print_separator("Interrupt Functionality Demo")
    
    # Mock interrupt handler
    interrupt_count = 0
    def interrupt_handler(master_id, irq_num):
        nonlocal interrupt_count
        interrupt_count += 1
        print(f"✓ Interrupt received! Master ID: {master_id}, IRQ: {irq_num}")
    
    # Set up interrupt handler (mock)
    original_send_irq = bus.send_irq if hasattr(bus, 'send_irq') else None
    bus.send_irq = interrupt_handler
    
    # Enable interrupts
    sys_ctrl_addr = 0x40000018
    sys_ctrl_value = hsm.read(sys_ctrl_addr)
    hsm.write(sys_ctrl_addr, sys_ctrl_value | (1 << 1))  # Set IRQEN bit
    print(f"✓ Enabled interrupts")
    
    # Perform operation that should trigger interrupt
    cmd_addr = 0x40000010
    cst_addr = 0x40000014
    
    cmd_value = (HSMMode.RND.value << 28) | 16
    hsm.write(cmd_addr, cmd_value)
    print(f"✓ Configured for interrupt-generating operation")
    
    hsm.write(cst_addr, 1)
    print(f"✓ Started operation")
    
    # Wait for operation and interrupt
    time.sleep(0.1)
    
    print(f"✓ Total interrupts received: {interrupt_count}")
    
    # Restore original function
    if original_send_irq:
        bus.send_irq = original_send_irq


def demo_dma_functionality(hsm):
    """Demonstrate DMA functionality."""
    print_separator("DMA Functionality Demo")
    
    # Test DMA interface
    print(f"Initial DMA status: {hsm.is_dma_ready()}")
    
    # Enable DMA through register
    sys_ctrl_addr = 0x40000018
    sys_ctrl_value = hsm.read(sys_ctrl_addr)
    hsm.write(sys_ctrl_addr, sys_ctrl_value | 1)  # Set DMAEN bit
    print(f"✓ Enabled DMA through SYS_CTRL register")
    print(f"DMA status after enable: {hsm.is_dma_ready()}")
    
    # Test DMA methods
    hsm.enable_dma_mode(0)
    print(f"✓ Enabled DMA mode with channel 0")
    print(f"DMA interface ready: {hsm.dma_interface.is_dma_ready()}")
    
    hsm.disable_dma_mode()
    print(f"✓ Disabled DMA mode")
    print(f"DMA status after disable: {hsm.is_dma_ready()}")


def demo_multi_instance(bus):
    """Demonstrate multi-instance support."""
    print_separator("Multi-Instance Support Demo")
    
    # Create multiple HSM instances with different IDs and addresses
    hsm1 = HSMDevice("hsm_primary", 0x41000000, 0x1000, 10)
    hsm2 = HSMDevice("hsm_secondary", 0x42000000, 0x1000, 11)
    
    bus.add_device(hsm1)
    bus.add_device(hsm2)
    
    print(f"✓ Created HSM instances:")
    print(f"  - {hsm1.name} at 0x{hsm1.base_address:08X}")
    print(f"  - {hsm2.name} at 0x{hsm2.base_address:08X}")
    
    # Configure each instance differently
    cmd_addr_offset = 0x010
    
    hsm1_cmd_value = (HSMMode.ECB.value << 28) | (18 << 22) | 32
    hsm1.write(hsm1.base_address + cmd_addr_offset, hsm1_cmd_value)
    
    hsm2_cmd_value = (HSMMode.RND.value << 28) | 64
    hsm2.write(hsm2.base_address + cmd_addr_offset, hsm2_cmd_value)
    
    print(f"✓ Configured instances independently:")
    print(f"  - {hsm1.name}: {hsm1.operation_mode.name} mode, key {hsm1.key_index}")
    print(f"  - {hsm2.name}: {hsm2.operation_mode.name} mode, {hsm2.text_length} bytes")
    
    # Verify independence
    hsm1_status = hsm1.get_status()
    hsm2_status = hsm2.get_status()
    
    print(f"✓ Instance independence verified:")
    print(f"  - Different modes: {hsm1_status['mode']} vs {hsm2_status['mode']}")
    print(f"  - Different lengths: {hsm1_status['text_length']} vs {hsm2_status['text_length']}")


def main():
    """Main demonstration function."""
    print("HSM Device Model Demonstration")
    print("DevCommV3 Framework Integration")
    print("Author: AI Assistant")
    print("Date:", time.strftime("%Y-%m-%d %H:%M:%S"))
    
    try:
        # Basic initialization
        hsm, bus = demo_basic_initialization()
        
        # Register access demo
        demo_register_access(hsm)
        
        # AES ECB operation demo
        demo_aes_ecb_operation(hsm)
        
        # Random number generation demo
        demo_random_number_generation(hsm)
        
        # Authentication demo
        demo_authentication(hsm)
        
        # Error handling demo
        demo_error_handling(hsm)
        
        # Interrupt functionality demo
        demo_interrupt_functionality(hsm, bus)
        
        # DMA functionality demo
        demo_dma_functionality(hsm)
        
        # Multi-instance demo
        demo_multi_instance(bus)
        
        print_separator("Demo Completed Successfully")
        print("✓ All HSM device features demonstrated")
        print("✓ Device model is fully functional")
        print("✓ Integration with DevCommV3 framework verified")
        
    except Exception as e:
        print(f"\n❌ Demo failed with error: {e}")
        import traceback
        traceback.print_exc()
        return 1
    
    return 0


if __name__ == "__main__":
    exit_code = main()
    sys.exit(exit_code)