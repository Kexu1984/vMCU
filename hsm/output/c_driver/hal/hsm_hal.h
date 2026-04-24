/**
 * @file hsm_hal.h
 * @brief HSM Hardware Abstraction Layer (HAL) interface definitions
 *
 * This module provides low-level register access functions for the HSM device.
 * All register operations go through the interface layer for communication
 * with the Python device model.
 */

#ifndef HSM_HAL_H
#define HSM_HAL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief HSM HAL status codes
 */
typedef enum {
    HSM_HAL_OK = 0,                 /**< Operation successful */
    HSM_HAL_ERROR,                  /**< General error */
    HSM_HAL_INVALID_PARAM,          /**< Invalid parameter */
    HSM_HAL_NOT_INITIALIZED,        /**< HAL not initialized */
    HSM_HAL_HARDWARE_ERROR          /**< Hardware error */
} hsm_hal_status_t;

/**
 * @brief HSM register offsets
 */
#define HSM_REG_UID0                0x000
#define HSM_REG_UID1                0x004
#define HSM_REG_UID2                0x008
#define HSM_REG_UID3                0x00C
#define HSM_REG_CMD                 0x010
#define HSM_REG_CMD_VALID           0x014
#define HSM_REG_SYS_CTRL            0x018
#define HSM_REG_SYS_STATUS          0x01C
#define HSM_REG_VFY_STATUS          0x020
#define HSM_REG_VFY_STATUS_CLR      0x024
#define HSM_REG_AUTH_CTRL           0x028
#define HSM_REG_AUTH_STATUS         0x02C
#define HSM_REG_AUTH_STATUS_CLR     0x030
#define HSM_REG_CIPHER_IN           0x034
#define HSM_REG_CIPHER_OUT          0x038
#define HSM_REG_IV0                 0x03C
#define HSM_REG_IV1                 0x040
#define HSM_REG_IV2                 0x044
#define HSM_REG_IV3                 0x048
#define HSM_REG_RAM_KEY0            0x04C
#define HSM_REG_RAM_KEY1            0x050
#define HSM_REG_RAM_KEY2            0x054
#define HSM_REG_RAM_KEY3            0x058
#define HSM_REG_RAM_KEY4            0x05C
#define HSM_REG_RAM_KEY5            0x060
#define HSM_REG_RAM_KEY6            0x064
#define HSM_REG_RAM_KEY7            0x068
#define HSM_REG_AUTH_CHALLENGE0     0x06C
#define HSM_REG_AUTH_CHALLENGE1     0x070
#define HSM_REG_AUTH_CHALLENGE2     0x074
#define HSM_REG_AUTH_CHALLENGE3     0x078
#define HSM_REG_AUTH_RESP0          0x07C
#define HSM_REG_AUTH_RESP1          0x080
#define HSM_REG_AUTH_RESP2          0x084
#define HSM_REG_AUTH_RESP3          0x088
#define HSM_REG_GOLDEN_MAC0         0x08C
#define HSM_REG_GOLDEN_MAC1         0x090
#define HSM_REG_GOLDEN_MAC2         0x094
#define HSM_REG_GOLDEN_MAC3         0x098
#define HSM_REG_KEK_AUTH_CODE0      0x09C
#define HSM_REG_KEK_AUTH_CODE1      0x0A0
#define HSM_REG_KEK_AUTH_CODE2      0x0A4
#define HSM_REG_KEK_AUTH_CODE3      0x0A8
#define HSM_REG_KEK_AUTH_CODE4      0x0AC
#define HSM_REG_KEK_AUTH_CODE5      0x0B0
#define HSM_REG_KEK_AUTH_CODE6      0x0B4
#define HSM_REG_KEK_AUTH_CODE7      0x0B8
#define HSM_REG_IRQ_STATUS          0x0BC
#define HSM_REG_IRQ_CLR             0x0C0
#define HSM_REG_CHIP_STATE          0x0C4

/**
 * @brief CMD register bit definitions
 */
#define HSM_CMD_CIPHER_MODE_MASK    0xF0000000
#define HSM_CMD_CIPHER_MODE_SHIFT   28
#define HSM_CMD_DECRYPT_EN          (1 << 27)
#define HSM_CMD_KEY_IDX_MASK        0x07C00000
#define HSM_CMD_KEY_IDX_SHIFT       22
#define HSM_CMD_AES256_EN           (1 << 20)
#define HSM_CMD_TXT_LEN_MASK        0x000FFFFF

/**
 * @brief SYS_CTRL register bit definitions
 */
#define HSM_SYS_CTRL_IRQ_EN         (1 << 1)
#define HSM_SYS_CTRL_DMA_EN         (1 << 0)

/**
 * @brief SYS_STATUS register bit definitions
 */
#define HSM_SYS_STATUS_TX_FIFO_EMPTY    (1 << 8)
#define HSM_SYS_STATUS_TX_FIFO_FULL     (1 << 7)
#define HSM_SYS_STATUS_RX_FIFO_EMPTY    (1 << 6)
#define HSM_SYS_STATUS_RX_FIFO_FULL     (1 << 5)
#define HSM_SYS_STATUS_CPU_CMD_ON_GOING (1 << 4)
#define HSM_SYS_STATUS_CMD_RG_UNLOCK    (1 << 3)
#define HSM_SYS_STATUS_HSM_IDLE         (1 << 2)
#define HSM_SYS_STATUS_HSM_CMD_DONE_MASK 0x3

/**
 * @brief AUTH_STATUS register bit definitions
 */
#define HSM_AUTH_STATUS_AUTH_CHALLENGE_VALID    (1 << 16)
#define HSM_AUTH_STATUS_DEBUG_UNLOCK_AUTH_PASS  (1 << 11)
#define HSM_AUTH_STATUS_DEBUG_UNLOCK_AUTH_TMO_FAIL (1 << 10)
#define HSM_AUTH_STATUS_DEBUG_UNLOCK_AUTH_DONE  (1 << 9)

/**
 * @brief HSM cipher modes
 */
typedef enum {
    HSM_CIPHER_MODE_ECB = 0x0,
    HSM_CIPHER_MODE_CBC = 0x1,
    HSM_CIPHER_MODE_CFB8 = 0x2,
    HSM_CIPHER_MODE_CFB128 = 0x3,
    HSM_CIPHER_MODE_OFB = 0x4,
    HSM_CIPHER_MODE_CTR = 0x5,
    HSM_CIPHER_MODE_CMAC = 0x6,
    HSM_CIPHER_MODE_AUTH = 0x7,
    HSM_CIPHER_MODE_RANDOM = 0x8
} hsm_cipher_mode_t;

/**
 * @brief Initialize HSM HAL base address
 * @param base_address Register base address for HSM device
 * @return HAL status code
 */
hsm_hal_status_t hsm_hal_base_address_init(uint32_t base_address);

/**
 * @brief Initialize HSM HAL (requires base address already set)
 * @return HAL status code
 */
hsm_hal_status_t hsm_hal_init(void);

/**
 * @brief Deinitialize HSM HAL
 * @return HAL status code
 */
hsm_hal_status_t hsm_hal_deinit(void);

/**
 * @brief Raw register read using base + offset
 * @param offset Register offset from base address
 * @param value Pointer to store read value
 * @return HAL status code
 */
hsm_hal_status_t hsm_hal_read_raw(uint32_t offset, uint32_t *value);

/**
 * @brief Raw register write using base + offset
 * @param offset Register offset from base address
 * @param value Value to write
 * @return HAL status code
 */
hsm_hal_status_t hsm_hal_write_raw(uint32_t offset, uint32_t value);

/**
 * @brief Direct register read with absolute address
 * @param address Absolute register address
 * @param value Pointer to store read value
 * @return HAL status code
 */
hsm_hal_status_t hsm_hal_read_register_direct(uint32_t address, uint32_t *value);

/**
 * @brief Direct register write with absolute address
 * @param address Absolute register address
 * @param value Value to write
 * @return HAL status code
 */
hsm_hal_status_t hsm_hal_write_register_direct(uint32_t address, uint32_t value);

/**
 * @brief Read UID register
 * @param uid_index UID register index (0-3)
 * @param value Pointer to store UID value
 * @return HAL status code
 */
hsm_hal_status_t hsm_hal_read_uid(uint32_t uid_index, uint32_t *value);

/**
 * @brief Write CMD register
 * @param cipher_mode Cipher mode (4 bits)
 * @param decrypt_en Decrypt enable flag
 * @param key_idx Key index (5 bits)
 * @param aes256_en AES-256 enable flag
 * @param txt_len Text length (20 bits)
 * @return HAL status code
 */
hsm_hal_status_t hsm_hal_write_cmd(uint32_t cipher_mode, bool decrypt_en,
                                   uint32_t key_idx, bool aes256_en, uint32_t txt_len);

/**
 * @brief Write CMD_VALID register to start operation
 * @return HAL status code
 */
hsm_hal_status_t hsm_hal_start_operation(void);

/**
 * @brief Read SYS_STATUS register
 * @param status Pointer to store status value
 * @return HAL status code
 */
hsm_hal_status_t hsm_hal_read_sys_status(uint32_t *status);

/**
 * @brief Write SYS_CTRL register
 * @param irq_en Interrupt enable flag
 * @param dma_en DMA enable flag
 * @return HAL status code
 */
hsm_hal_status_t hsm_hal_write_sys_ctrl(bool irq_en, bool dma_en);

/**
 * @brief Write cipher input data
 * @param data 32-bit data word to write
 * @return HAL status code
 */
hsm_hal_status_t hsm_hal_write_cipher_in(uint32_t data);

/**
 * @brief Read cipher output data
 * @param data Pointer to store 32-bit data word
 * @return HAL status code
 */
hsm_hal_status_t hsm_hal_read_cipher_out(uint32_t *data);

/**
 * @brief Write IV register
 * @param iv_index IV register index (0-3)
 * @param value IV value to write
 * @return HAL status code
 */
hsm_hal_status_t hsm_hal_write_iv(uint32_t iv_index, uint32_t value);

/**
 * @brief Write RAM key register
 * @param key_index Key register index (0-7)
 * @param value Key value to write
 * @return HAL status code
 */
hsm_hal_status_t hsm_hal_write_ram_key(uint32_t key_index, uint32_t value);

/**
 * @brief Read authentication challenge
 * @param challenge_index Challenge register index (0-3)
 * @param value Pointer to store challenge value
 * @return HAL status code
 */
hsm_hal_status_t hsm_hal_read_auth_challenge(uint32_t challenge_index, uint32_t *value);

/**
 * @brief Write authentication response
 * @param resp_index Response register index (0-3)
 * @param value Response value to write
 * @return HAL status code
 */
hsm_hal_status_t hsm_hal_write_auth_resp(uint32_t resp_index, uint32_t value);

/**
 * @brief Read authentication status
 * @param status Pointer to store auth status
 * @return HAL status code
 */
hsm_hal_status_t hsm_hal_read_auth_status(uint32_t *status);

/**
 * @brief Write golden MAC register
 * @param mac_index MAC register index (0-3)
 * @param value MAC value to write
 * @return HAL status code
 */
hsm_hal_status_t hsm_hal_write_golden_mac(uint32_t mac_index, uint32_t value);

/**
 * @brief Read IRQ status
 * @param status Pointer to store IRQ status
 * @return HAL status code
 */
hsm_hal_status_t hsm_hal_read_irq_status(uint32_t *status);

/**
 * @brief Clear IRQ status
 * @return HAL status code
 */
hsm_hal_status_t hsm_hal_clear_irq(void);

#ifdef __cplusplus
}
#endif

#endif /* HSM_HAL_H */