#!/usr/bin/env python3
"""
Python integration test for HSM device model.

This script demonstrates the HSM device model functionality and provides
integration testing with the DevComm framework.
"""

import sys
import os
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))

from hsm_device import HSMDevice
import time


def test_hsm_basic_functionality():
    """Test basic HSM device functionality."""
    print("Testing HSM basic functionality...")
    
    # Create HSM device instance
    hsm = HSMDevice("HSM_TEST", 0x40000000, 0x1000, 1)
    
    # Test UID reading
    print("\n1. Testing UID registers:")
    for i in range(4):
        uid_value = hsm.read(0x40000000 + i * 4)
        print(f"   UID{i}: 0x{uid_value:08X}")
    
    # Test system status
    print("\n2. Testing system status:")
    status = hsm.read(0x40000000 + 0x01C)
    print(f"   SYS_STATUS: 0x{status:08X}")
    print(f"   - TX FIFO Empty: {bool(status & (1 << 8))}")
    print(f"   - RX FIFO Empty: {bool(status & (1 << 6))}")
    print(f"   - HSM Idle: {bool(status & (1 << 2))}")
    
    # Test interrupt status
    print("\n3. Testing interrupt status:")
    irq_status = hsm.read(0x40000000 + 0x0BC)
    print(f"   IRQ_STATUS: 0x{irq_status:08X}")
    
    # Test authentication challenge generation
    print("\n4. Testing authentication:")
    # Start authentication by writing to AUTH_CTRL
    hsm.write(0x40000000 + 0x028, 1)  # ext_auth_resp_valid
    
    # Check auth status
    auth_status = hsm.read(0x40000000 + 0x02C)
    print(f"   AUTH_STATUS: 0x{auth_status:08X}")
    print(f"   - Challenge Valid: {bool(auth_status & (1 << 16))}")
    
    if auth_status & (1 << 16):  # challenge_valid
        print("   Authentication Challenge:")
        for i in range(4):
            challenge = hsm.read(0x40000000 + 0x06C + i * 4)
            print(f"     CHALLENGE{i}: 0x{challenge:08X}")
    
    print("\n5. Testing random number generation:")
    # Configure for random generation
    hsm.write(0x40000000 + 0x010, (0x8 << 28) | 32)  # mode=random, len=32 bytes
    hsm.write(0x40000000 + 0x014, 1)  # start operation
    
    # Wait for completion
    time.sleep(0.1)
    
    # Read random data
    print("   Random data:")
    for i in range(8):  # 32 bytes = 8 words
        status = hsm.read(0x40000000 + 0x01C)
        if not (status & (1 << 8)):  # TX FIFO not empty
            random_data = hsm.read(0x40000000 + 0x038)
            print(f"     Word {i}: 0x{random_data:08X}")
    
    print("\nBasic functionality test completed successfully!")


def test_hsm_aes_operations():
    """Test AES encryption/decryption operations."""
    print("\nTesting HSM AES operations...")
    
    # Create HSM device instance
    hsm = HSMDevice("HSM_AES", 0x40000000, 0x1000, 1)
    
    # Test data
    test_data = [0x00112233, 0x44556677, 0x8899AABB, 0xCCDDEEFF]
    test_key = [0x2B7E1516, 0x28AED2A6, 0xABF71588, 0x09CF4F3C]
    
    print("\n1. Testing AES ECB encryption:")
    
    # Set up RAM key
    for i, key_word in enumerate(test_key):
        hsm.write(0x40000000 + 0x04C + i * 4, key_word)
    
    # Configure for ECB encryption
    cmd = (0x0 << 28) | (0 << 27) | (22 << 22) | (0 << 20) | 16  # ECB, encrypt, RAM key, AES128, 16 bytes
    hsm.write(0x40000000 + 0x010, cmd)
    
    # Write test data
    for data_word in test_data:
        hsm.write(0x40000000 + 0x034, data_word)
    
    # Start operation
    hsm.write(0x40000000 + 0x014, 1)
    
    # Wait for completion
    time.sleep(0.1)
    
    # Read encrypted data
    print("   Encrypted data:")
    encrypted_data = []
    for i in range(4):
        status = hsm.read(0x40000000 + 0x01C)
        if not (status & (1 << 8)):  # TX FIFO not empty
            encrypted_word = hsm.read(0x40000000 + 0x038)
            encrypted_data.append(encrypted_word)
            print(f"     Word {i}: 0x{encrypted_word:08X}")
    
    print("\n2. Testing AES ECB decryption:")
    
    # Configure for ECB decryption
    cmd = (0x0 << 28) | (1 << 27) | (22 << 22) | (0 << 20) | 16  # ECB, decrypt, RAM key, AES128, 16 bytes
    hsm.write(0x40000000 + 0x010, cmd)
    
    # Write encrypted data back
    for data_word in encrypted_data:
        hsm.write(0x40000000 + 0x034, data_word)
    
    # Start operation
    hsm.write(0x40000000 + 0x014, 1)
    
    # Wait for completion
    time.sleep(0.1)
    
    # Read decrypted data
    print("   Decrypted data:")
    for i in range(4):
        status = hsm.read(0x40000000 + 0x01C)
        if not (status & (1 << 8)):  # TX FIFO not empty
            decrypted_word = hsm.read(0x40000000 + 0x038)
            print(f"     Word {i}: 0x{decrypted_word:08X} (expected: 0x{test_data[i]:08X})")
    
    print("\nAES operations test completed!")


def test_hsm_cmac_operations():
    """Test CMAC generation operations."""
    print("\nTesting HSM CMAC operations...")
    
    # Create HSM device instance
    hsm = HSMDevice("HSM_CMAC", 0x40000000, 0x1000, 1)
    
    # Test data
    test_data = [0x00112233, 0x44556677, 0x8899AABB, 0xCCDDEEFF]
    test_key = [0x2B7E1516, 0x28AED2A6, 0xABF71588, 0x09CF4F3C]
    
    print("\n1. Testing CMAC generation:")
    
    # Set up RAM key
    for i, key_word in enumerate(test_key):
        hsm.write(0x40000000 + 0x04C + i * 4, key_word)
    
    # Configure for CMAC
    cmd = (0x6 << 28) | (0 << 27) | (22 << 22) | (0 << 20) | 16  # CMAC, RAM key, AES128, 16 bytes
    hsm.write(0x40000000 + 0x010, cmd)
    
    # Write test data
    for data_word in test_data:
        hsm.write(0x40000000 + 0x034, data_word)
    
    # Start operation
    hsm.write(0x40000000 + 0x014, 1)
    
    # Wait for completion
    time.sleep(0.1)
    
    # Read CMAC result
    print("   CMAC result:")
    cmac_data = []
    for i in range(4):
        status = hsm.read(0x40000000 + 0x01C)
        if not (status & (1 << 8)):  # TX FIFO not empty
            cmac_word = hsm.read(0x40000000 + 0x038)
            cmac_data.append(cmac_word)
            print(f"     Word {i}: 0x{cmac_word:08X}")
    
    print("\nCMAC operations test completed!")


def main():
    """Main test function."""
    print("=" * 60)
    print("HSM Device Model Integration Test")
    print("=" * 60)
    
    try:
        test_hsm_basic_functionality()
        test_hsm_aes_operations() 
        test_hsm_cmac_operations()
        
        print("\n" + "=" * 60)
        print("All HSM integration tests completed successfully!")
        print("=" * 60)
        
    except Exception as e:
        print(f"\nError during testing: {e}")
        import traceback
        traceback.print_exc()
        return 1
    
    return 0


if __name__ == "__main__":
    sys.exit(main())