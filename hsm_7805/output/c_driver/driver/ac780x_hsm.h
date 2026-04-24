/* Copyright Statement:
 *
 * This software/firmware and related documentation ("AutoChips Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to AutoChips Inc. and/or its licensors. Without
 * the prior written permission of AutoChips inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of AutoChips Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * AutoChips Inc. (C) 2024. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("AUTOCHIPS SOFTWARE")
 * RECEIVED FROM AUTOCHIPS AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. AUTOCHIPS EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES AUTOCHIPS PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE AUTOCHIPS SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN AUTOCHIPS
 * SOFTWARE. AUTOCHIPS SHALL ALSO NOT BE RESPONSIBLE FOR ANY AUTOCHIPS SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND AUTOCHIPS'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE AUTOCHIPS SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT AUTOCHIPS'S OPTION, TO REVISE OR REPLACE THE
 * AUTOCHIPS SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO AUTOCHIPS FOR SUCH AUTOCHIPS SOFTWARE AT ISSUE.
 */

/*!
 * @file ac780x_hsm.h
 *
 * @brief This file provides HSM integration functions interface.
 *
 */

#ifndef AC780X_HSM_H
#define AC780X_HSM_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ===========================================  Includes  =========================================== */
#include "ac780x_hsm_reg.h"

/*=====================================SOURCE FILE VERSION INFORMATION==============================*/
#define HSM_HAL_MODULE_ID                    (222U)
#define HSM_HAL_SW_MAJOR_VERSION             (1U)
#define HSM_HAL_SW_MINOR_VERSION             (2U)
#define HSM_HAL_SW_PATCH_VERSION             (1U)

/* ============================================  Define  ============================================ */
#define HSM_CRYPTO_BLOCK_LEN                    (128U)
#define HSM_CRYPTO_BLOCK_BYTE                   (HSM_CRYPTO_BLOCK_LEN >> 3U)
#define HSM_TRNG_BLOCK_LEN                      (32U)
#define HSM_TRNG_BLOCK_BYTE                     (HSM_CRYPTO_BLOCK_LEN >> 3U)
/* ===========================================  Typedef  ============================================ */
typedef enum
{
    /* OTP Key Index */
    HSM_CHIP_ROOT_KEY_ID = 0U,
    HSM_BL_VERIFY_KEY_ID = 1U,
    HSM_FW_UPGRADE_KEY_ID = 2U,
    HSM_DEBUG_AUTH_KEY_ID = 3U,
    HSM_FW_INSTALL_KEY_ID = 4U,
    HSM_KEY_READ_KEY_ID = 5U,
    HSM_KEY_INSTALL_KEY_ID = 6U,
    HSM_LOCK_STAT_KEY_ID = 7U,

    /* User Key in FLASH */
    HSM_FLASH_KEY_START_ID = 8U,
    HSM_FLASH_KEY_0_ID = HSM_FLASH_KEY_START_ID,
    HSM_FLASH_KEY_1_ID = HSM_FLASH_KEY_0_ID + 1U,
    HSM_FLASH_KEY_2_ID = HSM_FLASH_KEY_1_ID + 1U,
    HSM_FLASH_KEY_3_ID = HSM_FLASH_KEY_2_ID + 1U,
    HSM_FLASH_KEY_4_ID = HSM_FLASH_KEY_3_ID + 1U,
    HSM_FLASH_KEY_5_ID = HSM_FLASH_KEY_4_ID + 1U,
    HSM_FLASH_KEY_6_ID = HSM_FLASH_KEY_5_ID + 1U,
    HSM_FLASH_KEY_7_ID = HSM_FLASH_KEY_6_ID + 1U,
    HSM_FLASH_KEY_8_ID = HSM_FLASH_KEY_7_ID + 1U,
    HSM_FLASH_KEY_9_ID = HSM_FLASH_KEY_8_ID + 1U,
    HSM_FLASH_KEY_10_ID = HSM_FLASH_KEY_9_ID + 1U,

    HSM_FLASH_KEY_END_ID = HSM_FLASH_KEY_10_ID,

    /* User Key in RAM */
    HSM_RAM_KEY_ID = 18U,
} HSM_KeyIndex;

/*!< HSM encrypted direction: only valid in ECB/CBC command */
typedef enum
{
    HSM_ENCRYPT = 0U,  /*!< Encrypt mode */
    HSM_DECRYPT = 1U,  /*!< Decrypt mode */
} HSM_EncMode;

/*!< HSM cmac direction: only valid in CMAC/AUTH command */
typedef enum
{
    HSM_SIGN = 0U,     /*!< Sign mode */
    HSM_VERIFY = 1U,   /*!< Verify mode */
} HSM_CmacMode;

/*!< HSM Crypto command type */
typedef enum
{
    HSM_AES_ECB = 0U,   /*!< ECB mode */
    HSM_AES_CBC = 1U,   /*!< CBC mode */
    HSM_AES_CMAC = 2U,  /*!< CMAC mode */
    HSM_AES_AUTH = 3U,  /*!< AUTH mode */
    HSM_GET_RND = 4U,   /*!< Get random data */
} HSM_CryptoType;

/* ====================================  Functions declaration  ===================================== */
/*!
 * @brief ECB encrypt or decrypt by customer key.
 *
 * @param[in] enc:
 *                - HSM_ENCRYPT
 *                - HSM_DECRYPT
 * @param[in] inBuf: address of crypto data(must be 4bytes align)
 * @param[in] inLen: The size of crypto data in bytes(inBuf len = outBuf len and align with 16bytes)
 * @param[out] outBuf: address of crypto result(must be 4bytes align)
 * @param[in] keyId: type of HSM_KeyIndex: Flash key (8 ~ 17) + Ram Key (18)
 * @param[in] cb: customer callback with DMA
 *                NULL: means polling mode
 *                not NULL: means DMA mode
 *                        callback param:
 *                               - device: HSM
 *                               - wpara: The command status
 *                               - lpara: Unused
 * @return ret: HSM operation status
 */
status_t HSM_CryptoAesECB(HSM_EncMode enc, uint32_t *inBuf, uint32_t inLen, uint32_t *outBuf, HSM_KeyIndex keyId,
                               DeviceCallback_Type cb);

/*!
 * @brief CBC encrypt or decrypt by customer key.
 *
 * @param[in] enc:
 *                - HSM_ENCRYPT
 *                - HSM_DECRYPT
 * @param[in] inBuf: address of crypto data(must be 4bytes align)
 * @param[in] inLen: The size of crypto data in bytes(inBuf len should equal to outBuf len and align with 16bytes)
 * @param[out] outBuf: address of crypto result(must be 4bytes align)
 * @param[in] ivBuf: address of iv(must be 4bytes align)
 * @param[in] keyId: type of HSM_KeyIndex: Flash key (8 ~ 17) + Ram Key (18)
 * @param[in] cb: customer callback with DMA
 *                NULL: means polling mode
 *                not NULL: means DMA mode
 *                        callback param:
 *                               - device: HSM
 *                               - wpara: The command status
 *                               - lpara: Unused
 * @return ret: HSM operation status
 */
status_t HSM_CryptoAesCBC(HSM_EncMode enc, uint32_t *inBuf, uint32_t inLen, uint32_t *outBuf, uint32_t *ivBuf,
                               HSM_KeyIndex keyId, DeviceCallback_Type cb);

/*!
 * @brief CMAC sign or verify by customer key.
 *
 * @param[in] enc:
 *                - HSM_SIGN
 *                - HSM_VERIFY
 * @param[in] totalLen: The size of total crypto data in bytes
 * @param[in] keyId: type of HSM_KeyIndex: Flash key (8 ~ 17) + Ram Key (18)
 * @param[in] macbuf: the buffer address of expected mac value(128bit buffer)(must be 4bytes align)
 * @param[in] cb: customer callback with DMA
 *                NULL: means polling mode
 *                not NULL: means DMA mode
 *                        callback param:
 *                               - device: HSM
 *                               - wpara: The command status
 *                               - lpara: Unused
 * @return ret: HSM operation status
 */
status_t HSM_CryptoAesCMACStart(HSM_CmacMode enc, uint32_t totalLen, HSM_KeyIndex keyId, uint32_t *macbuf,
                                       DeviceCallback_Type cb);

/*!
 * @brief CMAC sign or verify in Update mode by customer key.
 *
 * @param[in] inBuf: address of crypto data in this time. It must be 4bytes align.
 * @param[in] inLen: The size of crypto data in bytes in this time. It must be 4bytes align if not last package.
 * @param[in] lastPackage: Mark if this input is last package and result will report after this time
 * @return ret: HSM operation status
 */
status_t HSM_CryptoAesCMACUpdate(uint32_t *inBuf, uint32_t inLen, BOOL_Type lastPackage);

/*!
 * @brief Authentication init stage: get challenge from HSM
 *
 * @param[in] keyId: otp key id
 *                  - 3: Debug auth key
 *                  - 4: Firmware install key
 *                  - 5: Key read key
 *                  - 6: Key install key
 *                  - 7: Chip state key
 * @param[out] chlgBuf: address of authentication challenge(fixed in 16bytes)
 * @return ret: HSM operation status
 */
status_t HSM_CryptoAuthInit(HSM_KeyIndex keyId, uint32_t *chlgBuf);

/*!
 * @brief Authentication finish stage: verify mac with challenge by HSM
 *
 * @param[in] keyId: otp key id
 *                  - 3: Debug auth key
 *                  - 4: Firmware install key
 *                  - 5: Key read key
 *                  - 6: Key install key
 *                  - 7: Chip state key
 * @param[in] respBuf: address of signature(fixed in 16bytes)
 * @return ret: HSM operation status
 */
status_t HSM_CryptoAuthFinish(HSM_KeyIndex keyId, uint32_t *respBuf);

/*!
 * @brief Verify fimrware/bootloader by otp key.
 *
 * @param[in] totalLen: The size of total crypto data in bytes
 * @param[in] keyId: type of HSM_KeyIndex: HSM_BL_VERIFY_KEY_ID or HSM_FW_UPGRADE_KEY_ID
 * @param[in] macbuf: the buffer address of expected mac value(128bit buffer)
 * @param[in] cb: customer callback with DMA
 *                NULL: means polling mode
 *                not NULL: means DMA mode
 *                        callback param:
 *                               - device: HSM
 *                               - wpara: The command status
 *                               - lpara: Unused
 * @return ret: HSM operation status
 */
status_t HSM_FirmVerifyStart(uint32_t totalLen, HSM_KeyIndex keyId, uint32_t *macBuf, DeviceCallback_Type cb);

/*!
 * @brief Verify fimrware/bootloader in Upgrade mode by otp key.
 *
 * @param[in] inBuf: address of crypto data in this time. It must be 4bytes align.
 * @param[in] inLen: The size of crypto data in bytes in this time. It must be 4bytes align if not last package.
 * @param[in] lastPackage: Mark if this input is last package and result will report after this time
 * @return ret: HSM operation status
 */
status_t HSM_FirmVerifyUpdate(uint32_t *inBuf, uint32_t inLen, BOOL_Type lastPackage);

/*!
 * @brief Get HSM rand data with polling mode.
 *
 * @param[in] outLen: The byte size of rand data, should 4-aligned
 * @param[out] outBuf: address of rand data buffer(must be 4bytes align)
 * @return ret: HSM operation status
 */
status_t HSM_CryptoGetRandom(uint32_t outLen, uint32_t *outBuf);

/*!
 * @brief Get UID from HSM
 *
 * @param[out] uid: address of uid buffer(fixed in 16bytes)(must be 4bytes align)
 * @return ret: HSM operation status
 */
void HSM_GetUID(uint32_t *uid);

/*!
 * @brief Install ram key in HSM
 *
 * @param[in] key: key data buffer(fixed in 16bytes)(must be 4bytes align)
 * @return ret: HSM operation status
 */
void HSM_InstallRamKey(uint32_t *key);

/*!
 * @brief Revoke ram key in HSM
 *
 * @return None
 */
void HSM_UninstallRamKey(void);

/*!
 * @brief Install flash key in HSM
 *
 * @param[in] key: key data buffer(fixed in 16bytes)(must be 4bytes align)
 * @param[in] keyId: type of HSM_KeyIndex: Flash key (8 ~ 17) + Ram Key (18)
 * @return status: STAUS_ERROR/STATUS_SUCCESS
 */
status_t HSM_InstallFlashKey(uint32_t *key, HSM_KeyIndex keyIndex);

/*!
 * @brief Revoke flash key in HSM
 *
 * @param[in] keyId: type of HSM_KeyIndex: Flash key (8 ~ 17) + Ram Key (18)
 * @return status: STAUS_ERROR/STATUS_SUCCESS
 */
status_t HSM_UninstallFlashKey(HSM_KeyIndex keyIndex);

/*!
 * @brief remove all flash key in HSM
 *
 * @return status: STAUS_ERROR/STATUS_SUCCESS
 */
status_t HSM_UninstallAllFlashKey(void);

/*!
 * @brief Install HSM command complete interrupt call back function.
 *
 * @param[in] callback: type of hsm_isrcallback_t, if input NULL, callback will be deinit
 *                      callback param:
 *                                     - device: HSM
 *                                     - wpara: The command currently be processed
 *                                     - lpara: The STAT register value
 * @return none
 */
void HSM_InstallCallback(DeviceCallback_Type callback);

/*!
 * @brief generate mac by software.
 *
 * @param[in] inBuf : message buf pointer.
 * @param[in] length : message buf length.
 * @param[in] outBuf : mac buffer.
 * @param[in] key : key buf addres.
 * @return none
 */
uint32_t HSM_SoftWareGenerateCmac(uint8_t *inBuf, uint32_t length, uint8_t *outBuf, uint8_t *key);

/*!
 * @brief Get module version information.
 *
 * @param[out] versionInfo: Module version information address.
 * @return void
 */
void HSM_GetVersionInfo(VersionInfo_Type *versionInfo);

#ifdef __cplusplus
}
#endif

#endif /* AC780X_HSM_H */

/* =============================================  EOF  ============================================== */
