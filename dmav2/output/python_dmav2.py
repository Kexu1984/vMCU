#!/usr/bin/env python3
"""
Simple DMAv2 usage example and interface.

This module provides a simple interface for using the DMAv2 device
and demonstrates common usage patterns.
"""

import sys
import os
import time

# Add path for imports
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))

from dmav2_device import DMAv2Device, DMAv2TransferMode, DMAv2Priority
from devcomm.core.bus_model import BusModel


class DMAv2Simple:
    """Simplified interface for DMAv2 device."""
    
    def __init__(self, instance_id: int = 0, base_address: int = 0x40000000):
        self.device = DMAv2Device(instance_id, base_address)
        self.bus = BusModel("DMAv2_Bus")
        self.bus.add_device(self.device)
        
        # Setup interrupt handler
        self.transfer_events = []
        self.device.register_irq_callback(self._interrupt_handler)
        
    def _interrupt_handler(self, channel_id: int, event_type: str):
        """Handle DMA interrupts."""
        self.transfer_events.append({
            'channel': channel_id,
            'event': event_type,
            'timestamp': time.time()
        })
        print(f"DMA Event: Channel {channel_id} - {event_type}")
        
    def memory_copy(self, src_addr: int, dst_addr: int, size: int) -> int:
        """
        Perform a simple memory-to-memory copy.
        
        Args:
            src_addr: Source memory address
            dst_addr: Destination memory address  
            size: Number of bytes to copy
            
        Returns:
            Channel ID used for the transfer
        """
        channel_id = self.device.dma_interface.dma_request(src_addr, dst_addr, size)
        print(f"Started memory copy: 0x{src_addr:08X} -> 0x{dst_addr:08X} ({size} bytes) on channel {channel_id}")
        return channel_id
        
    def wait_for_completion(self, channel_id: int, timeout: float = 5.0) -> bool:
        """
        Wait for a transfer to complete.
        
        Args:
            channel_id: Channel to wait for
            timeout: Maximum time to wait in seconds
            
        Returns:
            True if transfer completed, False if timeout
        """
        start_time = time.time()
        
        while time.time() - start_time < timeout:
            status = self.device.get_channel_status(channel_id)
            if status['transfer_complete']:
                return True
            if status['transfer_error']:
                print(f"Transfer error on channel {channel_id}")
                return False
            time.sleep(0.1)
            
        print(f"Timeout waiting for channel {channel_id}")
        return False
        
    def get_transfer_progress(self, channel_id: int) -> dict:
        """Get transfer progress information."""
        status = self.device.get_channel_status(channel_id)
        
        if status['transfer_length'] > 0:
            progress_percent = (status['data_transferred'] / status['transfer_length']) * 100
        else:
            progress_percent = 0
            
        return {
            'channel_id': channel_id,
            'progress_percent': progress_percent,
            'bytes_transferred': status['data_transferred'],
            'total_bytes': status['transfer_length'],
            'state': status['state'],
            'completed': status['transfer_complete'],
            'error': status['transfer_error']
        }
        
    def reset_device(self):
        """Reset the DMA device."""
        self.device._perform_hard_reset()
        print("DMA device reset")
        
    def get_device_info(self) -> dict:
        """Get device information."""
        return self.device.get_device_status()
        
    def enable_error_injection(self):
        """Enable error injection for testing."""
        self.device.enable_error_injection()
        
    def inject_error(self, channel_id: int, error_type: str):
        """Inject an error for testing."""
        self.device.inject_error(channel_id, error_type)


def demo():
    """Demonstrate DMAv2 usage."""
    print("DMAv2 Device Demo")
    print("=" * 50)
    
    # Create DMA controller
    dma = DMAv2Simple(instance_id=0, base_address=0x40000000)
    
    print("DMA device created and initialized")
    print(f"Device info: {dma.get_device_info()}")
    
    # Simulate some memory transfers
    transfers = [
        (0x20000000, 0x30000000, 256),
        (0x20001000, 0x30001000, 512),
        (0x20002000, 0x30002000, 1024),
    ]
    
    active_channels = []
    
    print(f"\nStarting {len(transfers)} transfers...")
    for src, dst, size in transfers:
        channel_id = dma.memory_copy(src, dst, size)
        active_channels.append(channel_id)
        
    # Wait for all transfers to complete
    print("\nWaiting for transfers to complete...")
    completed = 0
    for channel_id in active_channels:
        if dma.wait_for_completion(channel_id, timeout=10.0):
            progress = dma.get_transfer_progress(channel_id)
            print(f"Channel {channel_id}: {progress['progress_percent']:.1f}% complete")
            completed += 1
        else:
            print(f"Channel {channel_id}: Failed or timeout")
            
    print(f"\nCompleted {completed}/{len(transfers)} transfers")
    
    # Show events
    print(f"\nCaptured {len(dma.transfer_events)} events:")
    for event in dma.transfer_events:
        print(f"  Channel {event['channel']}: {event['event']}")
        
    # Demonstrate error injection
    print("\nTesting error injection...")
    dma.enable_error_injection()
    
    error_channel = dma.memory_copy(0x40000000, 0x50000000, 128)
    time.sleep(0.1)  # Let transfer start
    dma.inject_error(error_channel, "src_bus_error")
    
    time.sleep(0.5)  # Wait for error handling
    
    error_progress = dma.get_transfer_progress(error_channel)
    print(f"Error channel status: {error_progress}")
    
    print("\nDemo completed!")


if __name__ == "__main__":
    demo()