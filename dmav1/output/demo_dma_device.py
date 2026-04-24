#!/usr/bin/env python3
"""
Quick demonstration of DMA device functionality.

This script provides a simple demonstration of the key DMA device features
without running the full test suite.
"""

import sys
import os
import time

# Add framework to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..'))

from dmav1.output.dma_device import DMADevice


def demonstrate_basic_dma():
    """Demonstrate basic DMA functionality."""
    print("="*60)
    print("DMA Device Basic Functionality Demonstration")
    print("="*60)
    
    # Create DMA device
    print("\n1. Creating DMA device...")
    dma = DMADevice(
        name="DemosDMA",
        base_address=0x40000000,
        size=0x1000,
        master_id=1,
        num_channels=16
    )
    
    # Register interrupt handler
    interrupts = []
    def interrupt_handler(channel_id, interrupt_type):
        interrupts.append((channel_id, interrupt_type))
        print(f"   Interrupt: Channel {channel_id} - {interrupt_type}")
    
    dma.register_irq_callback(interrupt_handler)
    
    print(f"   Created DMA controller with {dma.num_channels} channels")
    
    # Demonstrate basic register operations
    print("\n2. Testing register operations...")
    
    # Test global status register
    status = dma.read(0x40000000, 4)  # DMA_TOP_RST register
    print(f"   Global status: 0x{status:08X} (DMA_IDLE bit should be set)")
    
    # Configure channel 0
    print("\n3. Configuring DMA channel 0...")
    channel_id = 0
    base_addr = 0x40000000
    
    # Channel 0 configuration addresses
    config_addr = base_addr + 0x50    # Channel 0 config
    length_addr = base_addr + 0x54    # Channel 0 length  
    saddr_addr = base_addr + 0x58     # Channel 0 source start addr
    daddr_addr = base_addr + 0x60     # Channel 0 dest start addr
    enable_addr = base_addr + 0x64    # Channel 0 enable
    
    # Configure: direction=0 (P2M), circular=0, word transfers, high priority
    config_value = (0 << 7) | (0 << 6) | (2 << 4) | (2 << 2) | 2
    dma.write(config_addr, config_value, 4)
    print(f"   Configuration: 0x{config_value:08X}")
    
    # Set transfer parameters
    dma.write(length_addr, 256, 4)        # 256 bytes
    dma.write(saddr_addr, 0x20000000, 4)  # Source address
    dma.write(daddr_addr, 0x20001000, 4)  # Destination address
    print(f"   Transfer: 256 bytes from 0x20000000 to 0x20001000")
    
    # Get channel info before transfer
    print("\n4. Channel status before transfer:")
    info = dma.get_channel_info(channel_id)
    print(f"   State: {info['state']}")
    print(f"   Priority: {info['priority']}")
    print(f"   Direction: {info['direction']}")
    print(f"   Data size: {info['source_size']}")
    
    # Start transfer
    print("\n5. Starting DMA transfer...")
    interrupts.clear()
    dma.write(enable_addr, 0x00000001, 4)
    
    # Wait for transfer to complete
    print("   Waiting for transfer completion...")
    time.sleep(1.0)
    
    # Check results
    print("\n6. Transfer results:")
    info = dma.get_channel_info(channel_id)
    print(f"   Final state: {info['state']}")
    print(f"   Data transferred: {info['data_transferred']} bytes")
    print(f"   Finish flag: {info['finish_flag']}")
    print(f"   Half finish flag: {info['half_finish_flag']}")
    
    print(f"\n7. Interrupts generated: {len(interrupts)}")
    for ch_id, int_type in interrupts:
        print(f"   Channel {ch_id}: {int_type}")
    
    # Test multiple channels
    print("\n8. Testing multiple channels concurrently...")
    channels = [1, 2, 3]
    
    for ch_id in channels:
        channel_offset = ch_id * 0x40
        config_addr = base_addr + 0x50 + channel_offset
        length_addr = base_addr + 0x54 + channel_offset
        saddr_addr = base_addr + 0x58 + channel_offset
        daddr_addr = base_addr + 0x60 + channel_offset
        enable_addr = base_addr + 0x64 + channel_offset
        
        # Configure with different priorities
        priority = ch_id % 4
        config_value = (0 << 7) | (0 << 6) | (2 << 4) | (2 << 2) | priority
        dma.write(config_addr, config_value, 4)
        dma.write(length_addr, 128, 4)
        dma.write(saddr_addr, 0x20000000 + ch_id * 0x1000, 4)
        dma.write(daddr_addr, 0x20010000 + ch_id * 0x1000, 4)
    
    # Start all channels
    interrupts.clear()
    for ch_id in channels:
        channel_offset = ch_id * 0x40
        enable_addr = base_addr + 0x64 + channel_offset
        dma.write(enable_addr, 0x00000001, 4)
        print(f"   Started channel {ch_id}")
    
    # Wait for completion
    time.sleep(1.5)
    
    print("\n9. Multiple channel results:")
    for ch_id in channels:
        info = dma.get_channel_info(ch_id)
        print(f"   Channel {ch_id}: {info['state']}, "
              f"Priority: {info['priority']}, "
              f"Transferred: {info['data_transferred']}")
    
    print(f"\n   Total interrupts from multiple channels: {len(interrupts)}")
    
    # Global status
    print("\n10. Final global status:")
    global_status = dma.get_global_status()
    for key, value in global_status.items():
        print(f"   {key}: {value}")
    
    print("\n" + "="*60)
    print("DMA Device Demonstration Complete!")
    print("="*60)
    
    return True


def demonstrate_circular_mode():
    """Demonstrate circular buffer mode (quick test)."""
    print("\nCircular Buffer Mode Demonstration:")
    print("-" * 40)
    
    dma = DMADevice("CircularDMA", 0x41000000, 0x1000, 2, 4)
    
    # Configure channel for circular mode
    channel_id = 0
    base_addr = 0x41000000
    
    config_addr = base_addr + 0x50
    length_addr = base_addr + 0x54
    saddr_addr = base_addr + 0x58
    daddr_addr = base_addr + 0x60
    enable_addr = base_addr + 0x64
    
    # Configure: circular=1, byte transfers, medium priority
    config_value = (0 << 7) | (1 << 6) | (0 << 4) | (0 << 2) | 1
    dma.write(config_addr, config_value, 4)
    dma.write(length_addr, 32, 4)  # Small buffer
    dma.write(saddr_addr, 0x20000000, 4)
    dma.write(daddr_addr, 0x20002000, 4)
    
    print("   Configured for circular mode, 32-byte buffer")
    
    # Start transfer
    interrupts = []
    def circ_interrupt_handler(ch_id, int_type):
        interrupts.append((ch_id, int_type))
    
    dma.register_irq_callback(circ_interrupt_handler)
    dma.write(enable_addr, 0x00000001, 4)
    
    # Let it run briefly
    time.sleep(0.1)
    
    info = dma.get_channel_info(channel_id)
    transferred = info['data_transferred']
    
    # Stop circular mode
    dma.write(enable_addr, 0x00000000, 4)
    
    print(f"   Circular mode transferred: {transferred} bytes")
    print(f"   Multiple cycles: {'Yes' if transferred > 32 else 'No'}")
    print(f"   Interrupts generated: {len(interrupts)}")
    
    return transferred > 32


def main():
    """Main demonstration function."""
    print("DMA Device Model Demonstration")
    print("Starting basic functionality test...")
    
    try:
        basic_result = demonstrate_basic_dma()
        circular_result = demonstrate_circular_mode()
        
        print(f"\nDemonstration Results:")
        print(f"Basic functionality: {'✓ Success' if basic_result else '✗ Failed'}")
        print(f"Circular mode: {'✓ Success' if circular_result else '✗ Failed'}")
        
        if basic_result and circular_result:
            print("\n🎉 All demonstrations completed successfully!")
            return 0
        else:
            print("\n⚠️  Some demonstrations failed.")
            return 1
            
    except Exception as e:
        print(f"\n❌ Demonstration failed with error: {e}")
        return 1


if __name__ == "__main__":
    exit(main())