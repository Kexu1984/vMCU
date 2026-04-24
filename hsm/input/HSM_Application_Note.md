# HSM Application Note

## Overview
The HSM module is a part of a MCU, provides the encryption, decryption, MAC, true random number and MCU's debug port (JTAG) management features.

## Feature List
- Supports AES ECB, CBC, CFB8, CFB128, OFB, CTR mode, encryption and decryption, key size is 128 or 256.
- Supports generating AES 128/256 MAC or verifying the MAC.
- Supports enabling or disabling the debug port by a challendge-response authentication mechanism.
- Supports a true random number generator, which is implemented by the RO as the physical noise.
- Supports DMA operation or Polling opeartion.

## Functional Description
The HSM consists of serveal key components:
### Crypto Engine
Crypto Engine is a mainly component of the HSM module, it performs the data encryption or decryption operation. The key of the cryptographic job is from the OTP by setting the key index (CMD.key_idx) or from the RAM_KEY registers.
The mode of the crypto engine can be configured as ECB/CBC/CFB8/CFB128/OFB/CTR by the CMD.mode.
### Challenge-response
It's used for managing the MCU's debug port. By default, the debug port is disabled and the operator can enable it after the authentication flow.
The authentication flow uses the challenge-response mechanism to check if the operator is authorized.
The challenge-response flow is as following:
- Operator requests the challenge-response flow by writing the register AUTH_CTRL.erv.
- Operator checks the AUTH_STATUS.auth_challenge_valid if HSM returns the challenge number.
- If no any error ocurring and the AUTH_STATUS.auth_challenge_valid is 1, the operator reads the nonce data from the register AUTH_CHALLENGE0~AUTH_CHALLENGE3.
- Operator encrypts the nonce with the debug key and gets the nonce_cipher.
- Operator writes the nonce_cipher into the register AUTH_RESP0~AUTH_RESP3, then wait the status AUTH_STATUS.debug_unlock_auth_pass.
- If any error occur, the opeartor can read the error number from the AUTH_STATUS register. The opeartor can clean the error status and re-start the authentication flow.

### True Random Number Generator (TRNG)
The TRNG component provides the nonce to the challenge-response component, operator can also request a random number from the TRNG for other applications.
- Operator sets the CMD.cipher_mode as 4'b1xx, and the length of the random bit strings CMD.txt_len.
- Opeartor trigs the module to work by setting the bit CMD_VALID.vld as 1.
- Opeartor reads the random number from the FIFO register (CIPHER_OUT) if SYS_STATUS.tx_fifo_empty is 0, until meeting the length the operator requests (CMD.txt_len).
- If any error occur, the operator can check the register SYS_STATUS.

### Control and status Logic
- Register interface for configuration
- Interrupt management
- Status Register for software monitoring

## Initialzation Sequence
- Enable the Clock and release the reset
- Check if the register is locked.

## Secure Boot
- If the secure boot enable flag in the OTP is enabled, after the system booting up, the result can be checked in the register VFY_STATUSA.

## Operation Examples
- Encryption, decryption operation with DMA
  - Preparing the data to be calculated.
  - Configuring the module mode, key_idx or ram_key, iv, txt_len and other parameters.
  - Enabling the DMA mode and configuring the DMA registers (data source, destination, length and so on).
  - wait the job done by polling the status register or the interrupt signal.
  - the destination's content is the calculated data.
- Encryption, decryption operation without DMA
  - Configuring the module mode, key_idx or ram_key, iv, txt_len and other parameters.
  - for data_len >0: 
  - Writing the data to be calculated into the register CIPHER_IN if the rx_empty is 0.
  - trig the job start and wait it done.
  - Reading the calculated data from the register CIPER_OUT if the rx_emtpy is 0.
  - data_len = data_len - 32
- Generate MAC
  - Preparing the data to be calculated.
  - Configuring the module mode, key_idx or ram_key, iv, txt_len and other paraters.
  - Enabling the DMA mode or using the polling mode, the logic is descripted as above.
  - After the job being done, reading the result out from the register CHIPER_OUT if using polling mode or reading the result out from the destination address if using the DMA mode.
- Verify MAC automaticlly 
  - While the mode is CMAC and the key_idx is 1, the module will verify the data automaticlly with the expected MAC value, which are written into the register GOLDEN_MAC0~GOLDEN_MAC3 by software.
## Interrupt Handling
- The interrupt number is defined by the system.
- The events in the register IRQ_STATUS will trig a interrupt to outside.
- Write 1 to the register IRQ_CLR will clear the module's irq status.

## Error Conditions
- The debug port authentication status can check the register AUTH_STATUS to get the details. Bit 16th and bit 1st indicate the authentication flow failed.
- The error of the module will trig the irq to outside, the status can be read from the register IRQ_STATUS.


## Clock and Reset
The HSM module uses one clock and reset. The clock enable bit and reset bit is defined in the CRU module. Before requesting the HSM service the clock should be enabled and the reset should be released.