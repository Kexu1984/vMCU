/**
 * @file hsm_hal.c
 * @brief HSM Hardware Abstraction Layer (HAL) implementation
 *
 * This module provides low-level register access functions for the HSM device.
 * All register operations use the interface layer for communication with
 * the Python device model.
 */

#include "hsm_hal.h"
#include "../interface_layer.h"
#include <stddef.h>

/**
 * @brief Global base address storage
 */
static uint32_t g_hsm_base_address = 0;
static bool g_hsm_base_address_initialized = false;

hsm_hal_status_t hsm_hal_base_address_init(uint32_t base_address)
{
    g_hsm_base_address = base_address;
    g_hsm_base_address_initialized = true;
    return HSM_HAL_OK;
}

hsm_hal_status_t hsm_hal_init(void)
{
    if (!g_hsm_base_address_initialized) {
        return HSM_HAL_NOT_INITIALIZED;
    }
    
    return HSM_HAL_OK;
}

hsm_hal_status_t hsm_hal_deinit(void)
{
    g_hsm_base_address_initialized = false;
    g_hsm_base_address = 0;
    return HSM_HAL_OK;
}

hsm_hal_status_t hsm_hal_read_raw(uint32_t offset, uint32_t *value)
{
    if (!g_hsm_base_address_initialized) {
        return HSM_HAL_NOT_INITIALIZED;
    }
    
    if (value == NULL) {
        return HSM_HAL_INVALID_PARAM;
    }
    
    uint32_t absolute_address = g_hsm_base_address + offset;
    uint32_t read_value = read_register(absolute_address, sizeof(uint32_t));
    
    *value = read_value;
    return HSM_HAL_OK;
}

hsm_hal_status_t hsm_hal_write_raw(uint32_t offset, uint32_t value)
{
    if (!g_hsm_base_address_initialized) {
        return HSM_HAL_NOT_INITIALIZED;
    }
    
    uint32_t absolute_address = g_hsm_base_address + offset;
    write_register(absolute_address, value, sizeof(uint32_t));
    
    return HSM_HAL_OK;
}

hsm_hal_status_t hsm_hal_read_register_direct(uint32_t address, uint32_t *value)
{
    if (value == NULL) {
        return HSM_HAL_INVALID_PARAM;
    }
    
    uint32_t read_value = read_register(address, sizeof(uint32_t));
    *value = read_value;
    
    return HSM_HAL_OK;
}

hsm_hal_status_t hsm_hal_write_register_direct(uint32_t address, uint32_t value)
{
    write_register(address, value, sizeof(uint32_t));
    return HSM_HAL_OK;
}

hsm_hal_status_t hsm_hal_read_uid(uint32_t uid_index, uint32_t *value)
{
    if (uid_index > 3 || value == NULL) {
        return HSM_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = HSM_REG_UID0 + (uid_index * 4);
    return hsm_hal_read_raw(offset, value);
}

hsm_hal_status_t hsm_hal_write_cmd(uint32_t cipher_mode, bool decrypt_en,
                                   uint32_t key_idx, bool aes256_en, uint32_t txt_len)
{
    if (cipher_mode > 0xF || key_idx > 0x1F || txt_len > 0xFFFFF) {
        return HSM_HAL_INVALID_PARAM;
    }
    
    uint32_t cmd_value = 0;
    cmd_value |= (cipher_mode & 0xF) << HSM_CMD_CIPHER_MODE_SHIFT;
    cmd_value |= decrypt_en ? HSM_CMD_DECRYPT_EN : 0;
    cmd_value |= (key_idx & 0x1F) << HSM_CMD_KEY_IDX_SHIFT;
    cmd_value |= aes256_en ? HSM_CMD_AES256_EN : 0;
    cmd_value |= txt_len & HSM_CMD_TXT_LEN_MASK;
    
    return hsm_hal_write_raw(HSM_REG_CMD, cmd_value);
}

hsm_hal_status_t hsm_hal_start_operation(void)
{
    return hsm_hal_write_raw(HSM_REG_CMD_VALID, 1);
}

hsm_hal_status_t hsm_hal_read_sys_status(uint32_t *status)
{
    if (status == NULL) {
        return HSM_HAL_INVALID_PARAM;
    }
    
    return hsm_hal_read_raw(HSM_REG_SYS_STATUS, status);
}

hsm_hal_status_t hsm_hal_write_sys_ctrl(bool irq_en, bool dma_en)
{
    uint32_t ctrl_value = 0;
    ctrl_value |= irq_en ? HSM_SYS_CTRL_IRQ_EN : 0;
    ctrl_value |= dma_en ? HSM_SYS_CTRL_DMA_EN : 0;
    
    return hsm_hal_write_raw(HSM_REG_SYS_CTRL, ctrl_value);
}

hsm_hal_status_t hsm_hal_write_cipher_in(uint32_t data)
{
    return hsm_hal_write_raw(HSM_REG_CIPHER_IN, data);
}

hsm_hal_status_t hsm_hal_read_cipher_out(uint32_t *data)
{
    if (data == NULL) {
        return HSM_HAL_INVALID_PARAM;
    }
    
    return hsm_hal_read_raw(HSM_REG_CIPHER_OUT, data);
}

hsm_hal_status_t hsm_hal_write_iv(uint32_t iv_index, uint32_t value)
{
    if (iv_index > 3) {
        return HSM_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = HSM_REG_IV0 + (iv_index * 4);
    return hsm_hal_write_raw(offset, value);
}

hsm_hal_status_t hsm_hal_write_ram_key(uint32_t key_index, uint32_t value)
{
    if (key_index > 7) {
        return HSM_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = HSM_REG_RAM_KEY0 + (key_index * 4);
    return hsm_hal_write_raw(offset, value);
}

hsm_hal_status_t hsm_hal_read_auth_challenge(uint32_t challenge_index, uint32_t *value)
{
    if (challenge_index > 3 || value == NULL) {
        return HSM_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = HSM_REG_AUTH_CHALLENGE0 + (challenge_index * 4);
    return hsm_hal_read_raw(offset, value);
}

hsm_hal_status_t hsm_hal_write_auth_resp(uint32_t resp_index, uint32_t value)
{
    if (resp_index > 3) {
        return HSM_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = HSM_REG_AUTH_RESP0 + (resp_index * 4);
    return hsm_hal_write_raw(offset, value);
}

hsm_hal_status_t hsm_hal_read_auth_status(uint32_t *status)
{
    if (status == NULL) {
        return HSM_HAL_INVALID_PARAM;
    }
    
    return hsm_hal_read_raw(HSM_REG_AUTH_STATUS, status);
}

hsm_hal_status_t hsm_hal_write_golden_mac(uint32_t mac_index, uint32_t value)
{
    if (mac_index > 3) {
        return HSM_HAL_INVALID_PARAM;
    }
    
    uint32_t offset = HSM_REG_GOLDEN_MAC0 + (mac_index * 4);
    return hsm_hal_write_raw(offset, value);
}

hsm_hal_status_t hsm_hal_read_irq_status(uint32_t *status)
{
    if (status == NULL) {
        return HSM_HAL_INVALID_PARAM;
    }
    
    return hsm_hal_read_raw(HSM_REG_IRQ_STATUS, status);
}

hsm_hal_status_t hsm_hal_clear_irq(void)
{
    return hsm_hal_write_raw(HSM_REG_IRQ_CLR, 1);
}