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
 * @file ac780x_hsm_reg.h
 *
 * @brief HSM access register inline function definition.
 *
 */

#ifndef AC780X_HSM_REG_H
#define AC780X_HSM_REG_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ===========================================  Includes  =========================================== */
#include "ac7805x.h"


/* ============================================  Define  ============================================ */


/* ======================================  Functions define  ======================================== */
/*!
 * @brief Get HSM uid0.
 *
 * @param[in] none
 * @return HSM uid0 data
 */
__STATIC_INLINE uint32_t HSM_GetUid0(void)
{
    return (HSM->UID0);
}

/*!
 * @brief Get HSM uid1.
 *
 * @param[in] none
 * @return HSM uid1 data
 */
__STATIC_INLINE uint32_t HSM_GetUid1(void)
{
    return (HSM->UID1);
}

/*!
 * @brief Get HSM uid2.
 *
 * @param[in] none
 * @return HSM uid2 data
 */
__STATIC_INLINE uint32_t HSM_GetUid2(void)
{
    return (HSM->UID2);
}

/*!
 * @brief Get HSM uid3.
 *
 * @param[in] none
 * @return HSM uid3 data
 */
__STATIC_INLINE uint32_t HSM_GetUid3(void)
{
    return (HSM->UID3);
}

/*!
 * @brief Set HSM command.
 *
 * @param[in] cmd: HSM command register
 * @return none
 */
__STATIC_INLINE void HSM_SetCmd(uint32_t cmd)
{
    MODIFY_REG32(HSM->CMD, HSM_CMD_CIPMODE_Msk, HSM_CMD_CIPMODE_Pos, cmd);
}

/*!
 * @brief Get HSM command.
 *
 * @return HSM command
 */
__STATIC_INLINE uint8_t HSM_GetCmd(void)
{
    return (uint8_t)((HSM->CMD & HSM_CMD_CIPMODE_Msk) >> HSM_CMD_CIPMODE_Pos);
}

/*!
 * @brief Set encrypt or decrypt.
 *
 * @param[in] enc: encrypt or decrypt
 *               - HSM_ENCRYPT
 *               - HSM_DECRYPT
 * @return none
 */
__STATIC_INLINE void HSM_SetEncDec(uint8_t enc)
{
    MODIFY_REG32(HSM->CMD, HSM_CMD_DEC_Msk, HSM_CMD_DEC_Pos, enc);
}

/*!
 * @brief Get encrypt or decrypt.
 *
 * @return encrypt or decrypt
 */
__STATIC_INLINE uint8_t HSM_GetEncDec(void)
{
    return (uint8_t)((HSM->CMD & HSM_CMD_DEC_Msk) >> HSM_CMD_DEC_Pos);
}

/*!
 * @brief Set key index register.
 *
 * @param[in] index: flash key index(0 ~ 18)
 * @return none
 */
__STATIC_INLINE void HSM_SetKeyId(uint8_t index)
{
    MODIFY_REG32(HSM->CMD, HSM_CMD_KEYID_Msk, HSM_CMD_KEYID_Pos, index);
}

/*!
 * @brief Get key index register.
 *
 * @return index: flash key index(0 ~ 18)
 */
__STATIC_INLINE uint8_t HSM_GetKeyId(void)
{
    return (uint8_t)((HSM->CMD & HSM_CMD_KEYID_Msk) >> HSM_CMD_KEYID_Pos);
}

/*!
 * @brief Set length register.
 *
 * @param[in] len: content length
 * @return none
 */
__STATIC_INLINE void HSM_SetLength(uint32_t len)
{
    MODIFY_REG32(HSM->CMD, HSM_CMD_TXLEN_Msk, HSM_CMD_TXLEN_Pos, len);
}

/*!
 * @brief Set cipher in register.
 *
 * @param[in] data: The source of AES calculate
 * @return none
 */
__STATIC_INLINE void HSM_SetCipherIn(uint32_t data)
{
    HSM->CIPIN = data;
}

/*!
 * @brief Get cipher out register.
 *
 * @param[in] none
 * @return none
 */
__STATIC_INLINE uint32_t HSM_GetCipherOut(void)
{
    return (HSM->CIPOUT);
}

/*!
 * @brief Set IV0 register.
 *
 * @param[in] data: IV0 data
 * @return none
 */
__STATIC_INLINE void HSM_SetIV0(uint32_t data)
{
    HSM->IV0 = data;
}

/*!
 * @brief Set IV1 register.
 *
 * @param[in] data: IV1 data
 * @return none
 */
__STATIC_INLINE void HSM_SetIV1(uint32_t data)
{
    HSM->IV1 = data;
}

/*!
 * @brief Set IV2 register.
 *
 * @param[in] data: IV2 data
 * @return none
 */
__STATIC_INLINE void HSM_SetIV2(uint32_t data)
{
    HSM->IV2 = data;
}

/*!
 * @brief Set IV3 register.
 *
 * @param[in] data: IV3 data
 * @return none
 */
__STATIC_INLINE void HSM_SetIV3(uint32_t data)
{
    HSM->IV3 = data;
}

/*!
 * @brief Set RAM key0 register.
 *
 * @param[in] data: RAM key0 data
 * @return none
 */
__STATIC_INLINE void HSM_SetRamKey0(uint32_t data)
{
    HSM->RAM_KEY0 = data;
}

/*!
 * @brief Set RAM key1 register.
 *
 * @param[in] data: RAM key1 data
 * @return none
 */
__STATIC_INLINE void HSM_SetRamKey1(uint32_t data)
{
    HSM->RAM_KEY1 = data;
}

/*!
 * @brief Set RAM key2 register.
 *
 * @param[in] data: RAM key2 data
 * @return none
 */
__STATIC_INLINE void HSM_SetRamKey2(uint32_t data)
{
    HSM->RAM_KEY2 = data;
}

/*!
 * @brief Set RAM key3 register.
 *
 * @param[in] data: RAM key3 data
 * @return none
 */
__STATIC_INLINE void HSM_SetRamKey3(uint32_t data)
{
    HSM->RAM_KEY3 = data;
}

/*!
 * @brief get authentication challenge0 register.
 *
 * @param[in] none
 * @return Authentication challenge0 data
 */
__STATIC_INLINE uint32_t HSM_GetChallenge0(void)
{
    return (HSM->CHA0);
}

/*!
 * @brief Get authentication challenge1 register.
 *
 * @param[in] none
 * @return Authentication challenge1 data
 */
__STATIC_INLINE uint32_t HSM_GetChallenge1(void)
{
    return (HSM->CHA1);
}

/*!
 * @brief Get authentication challenge2 register.
 *
 * @param[in] none
 * @return Authentication challenge2 data
 */
__STATIC_INLINE uint32_t HSM_GetChallenge2(void)
{
    return (HSM->CHA2);
}

/*!
 * @brief Get authentication challenge3 register.
 *
 * @param[in] none
 * @return Authentication challenge3 data data
 */
__STATIC_INLINE uint32_t HSM_GetChallenge3(void)
{
    return (HSM->CHA3);
}

/*!
 * @brief Set authentication response0 register.
 *
 * @param[in] data: authentication response0 data
 * @return none
 */
__STATIC_INLINE void HSM_SetResponse0(uint32_t data)
{
    HSM->RSP0 = data;
}

/*!
 * @brief Set authentication response1 register.
 *
 * @param[in] data: authentication response1 data
 * @return none
 */
__STATIC_INLINE void HSM_SetResponse1(uint32_t data)
{
    HSM->RSP1 = data;
}

/*!
 * @brief Set authentication response2 register.
 *
 * @param[in] data: authentication response2 data
 * @return none
 */
__STATIC_INLINE void HSM_SetResponse2(uint32_t data)
{
    HSM->RSP2 = data;
}

/*!
 * @brief Set authentication response3 register.
 *
 * @param[in] data: authentication response3 data
 * @return none
 */
__STATIC_INLINE void HSM_SetResponse3(uint32_t data)
{
    HSM->RSP3 = data;
}

/*!
 * @brief Set golden mac0 register.
 *
 * @param[in] data: golden mac0 data
 * @return none
 */
__STATIC_INLINE void HSM_SetGoldenMac0(uint32_t data)
{
    HSM->MAC0 = data;
}

/*!
 * @brief Set golden mac1 register.
 *
 * @param[in] data: golden mac1 data
 * @return none
 */
__STATIC_INLINE void HSM_SetGoldenMac1(uint32_t data)
{
    HSM->MAC1 = data;
}

/*!
 * @brief Set golden mac2 register.
 *
 * @param[in] data: golden mac2 data
 * @return none
 */
__STATIC_INLINE void HSM_SetGoldenMac2(uint32_t data)
{
    HSM->MAC2 = data;
}

/*!
 * @brief Set golden mac3 register.
 *
 * @param[in] data: golden mac3 data
 * @return none
 */
__STATIC_INLINE void HSM_SetGoldenMac3(uint32_t data)
{
    HSM->MAC3 = data;
}

/*!
 * @brief Get command register unlock status.
 *
 * @param[in] none
 * @return The command register unlock state
            0: locked
            1: unlocked
 */
__STATIC_INLINE uint8_t HSM_GetUnLockStatus(void)
{
    return (uint8_t)((HSM->CST & HSM_CST_ULC_Msk) >> HSM_CST_ULC_Pos);
}

/*!
 * @brief Set command trigger register.
 *
 * @param[in] none
 * @return none
 */
__STATIC_INLINE void HSM_SetTrig(void)
{
    SET_BIT32(HSM->CST, HSM_CST_START_Msk);
}

/*!
 * @brief Set command trigger send ahtnectication response.
 *
 * @param[in] none
 * @return none
 */
__STATIC_INLINE void HSM_SetTrigResponse(void)
{
    HSM->AUTH_STAT = HSM_AUTH_STAT_ARV_Msk;
}

/*!
 * @brief Get HSM tx fifo empty status.
 *
 * @param[in] none
 * @return The tx fifo empty state
            0: not empty
            1: empty
 */
__STATIC_INLINE uint8_t HSM_GetTxFiFoEmptyFlag(void)
{
    return (uint8_t)((HSM->SYS_CTRL & HSM_SYS_CTRL_TE_Msk) >> HSM_SYS_CTRL_TE_Pos);
}

/*!
 * @brief Get HSM rx fifo full status.
 *
 * @param[in] none
 * @return The rx fifo full state
            0: not full
            1: full
 */
__STATIC_INLINE uint8_t HSM_GetRxFiFoFullFlag(void)
{
    return (uint8_t)((HSM->SYS_CTRL & HSM_SYS_CTRL_RF_Msk) >> HSM_SYS_CTRL_RF_Pos);
}

/*!
 * @brief Get HSM idle status.
 *
 * @param[in] none
 * @return The idle state
            0: not idle
            1: idle
 */
__STATIC_INLINE uint8_t HSM_GetIdleStatus(void)
{
    return (uint8_t)((HSM->SYS_CTRL & HSM_SYS_CTRL_IDLE_Msk) >> HSM_SYS_CTRL_IDLE_Pos);
}

/*!
 * @brief Get firmware upgrade verify status.
 *
 * @param[in] none
 * @return The firmware idle state
            0: verify pass
            1: verify fail
 */
__STATIC_INLINE uint8_t HSM_GetFwUpVerifyPassFlag(void)
{
    return (uint8_t)((HSM->VRY_STAT & HSM_VRY_STAT_FVP_Msk) >> HSM_VRY_STAT_FVP_Pos);
}

/*!
 * @brief Get firmware upgrade verify done.
 *
 * @param[in] none
 * @return The firmware verify done state
            0: verify not done
            1: verify done
 */
__STATIC_INLINE uint8_t HSM_GetUpFwVerifyDoneFlag(void)
{
    return (uint8_t)((HSM->VRY_STAT & HSM_VRY_STAT_FVD_Msk) >> HSM_VRY_STAT_FVD_Pos);
}

/*!
 * @brief Get bootloader verify status.
 *
 * @param[in] none
 * @return The bootloader verify state
            0: verify pass
            1: verify fail
 */
__STATIC_INLINE uint8_t HSM_GetBlVerifyPassFlag(void)
{
    return (uint8_t)((HSM->VRY_STAT & HSM_VRY_STAT_BVP_Msk) >> HSM_VRY_STAT_BVP_Pos);
}

/*!
 * @brief Get bootloader upgrade verify done.
 *
 * @param[in] none
 * @return The bootloader verify done state
            0: verify not done
            1: verify done
 */
__STATIC_INLINE uint8_t HSM_GetBlVerifyDoneFlag(void)
{
    return (uint8_t)((HSM->VRY_STAT & HSM_VRY_STAT_BVD_Msk) >> HSM_VRY_STAT_BVD_Pos);
}

/*!
 * @brief Clear firmware upgrade verify status.
 *
 * @param[in] none
 * @return none
 */
__STATIC_INLINE void HSM_ClearFwUpVerifyDoneFlag(void)
{
    HSM->VRY_STAT |= HSM_VRY_STAT_FVD_Msk;
}

/*!
 * @brief Clear bootloader verify status.
 *
 * @param[in] none
 * @return none
 */
__STATIC_INLINE void HSM_ClearBlVerifyDoneFlag(void)
{
    HSM->VRY_STAT |= HSM_VRY_STAT_BVD_Msk;
}

/*!
 * @brief Get HSM authentication challenge valid.
 *
 * @param[in] none
 * @return The authentication challenge valid state
            0: invalid
            1: valid
 */
__STATIC_INLINE uint8_t HSM_GetChallengeValidFlag(void)
{
    return (uint8_t)((HSM->AUTH_STAT & HSM_AUTH_STAT_ACV_Msk) >> HSM_AUTH_STAT_ACV_Pos);
}

/*!
 * @brief Get debug authentication pass status.
 *
 * @param[in] none
 * @return The debug authentication pass state
            0: debug authentication pass
            1: debug authentication not pass
 */
__STATIC_INLINE uint8_t HSM_DebugAuthPassFlag(void)
{
    return (uint8_t)((HSM->AUTH_STAT & HSM_AUTH_STAT_DAP_Msk) >> HSM_AUTH_STAT_DAP_Pos);
}

/*!
 * @brief Get debug authentication timeout fail status.
 *
 * @param[in] none
 * @return The debug authentication timeout fail state
            0: debug authentication timeout fail
            1: debug authentication no timeout
 */
__STATIC_INLINE uint8_t HSM_DebugAuthTimeoutFlag(void)
{
    return (uint8_t)((HSM->AUTH_STAT & HSM_AUTH_STAT_DAF_Msk) >> HSM_AUTH_STAT_DAF_Pos);
}

/*!
 * @brief Get debug authentication done status.
 *
 * @param[in] none
 * @return The debug authentication done state
            0: adebug authentication not done
            1: adebug authentication Done
 */
__STATIC_INLINE uint8_t HSM_DebugAuthDoneFlag(void)
{
    return (uint8_t)((HSM->AUTH_STAT & HSM_AUTH_STAT_DAD_Msk) >> HSM_AUTH_STAT_DAD_Pos);
}

/*!
 * @brief Get chip lock state authentication pass status.
 *
 * @param[in] none
 * @return The chip lock state authentication state
            0: chip lock state authentication not pass
            1: chip lock state authentication pass
 */
__STATIC_INLINE uint8_t HSM_ChipLockStateAuthPassFlag(void)
{
    return (uint8_t)((HSM->AUTH_STAT & HSM_AUTH_STAT_CLA_Msk) >> HSM_AUTH_STAT_CLA_Pos);
}

/*!
 * @brief Get key install authentication pass status.
 *
 * @param[in] none
 * @return The key install pass state
            0: key install authentication pass
            1: key install authentication not pass
 */
__STATIC_INLINE uint8_t HSM_KeyInstallAuthPassFlag(void)
{
    return (uint8_t)((HSM->AUTH_STAT & HSM_AUTH_STAT_KIA_Msk) >> HSM_AUTH_STAT_KIA_Pos);
}

/*!
 * @brief Get key read authentication pass status.
 *
 * @param[in] none
 * @return The key read authentication pass state
            0: key read authentication pass
            1: key read authentication not pass
 */
__STATIC_INLINE uint8_t HSM_KeyReadAuthPassFlag(void)
{
    return (uint8_t)((HSM->AUTH_STAT & HSM_AUTH_STAT_KRA_Msk) >> HSM_AUTH_STAT_KRA_Pos);
}

/*!
 * @brief Get firmware install authentication pass status.
 *
 * @param[in] none
 * @return The firmware install authentication pass state
            0: firmware install authentication pass
            1: firmware install authentication not pass
 */
__STATIC_INLINE uint8_t HSM_FwInstallAuthPassFlag(void)
{
    return (uint8_t)((HSM->AUTH_STAT & HSM_AUTH_STAT_FIA_Msk) >> HSM_AUTH_STAT_FIA_Pos);
}

/*!
 * @brief Get internal authentication pass status.
 *
 * @param[in] none
 * @return The internal authentication pass state
            0: internal authentication fail
            1: firmware install authentication pass
            2: key read authentication pass
            3: key install authentication pass
            4: chip lock state authentication pass
 */
__STATIC_INLINE uint8_t HSM_InternalAuthPassFlag(void)
{
    return (uint8_t)((HSM->AUTH_STAT & (HSM_AUTH_STAT_FIA_Msk | HSM_AUTH_STAT_KRA_Msk
                    | HSM_AUTH_STAT_KIA_Msk | HSM_AUTH_STAT_CLA_Msk)) >> HSM_AUTH_STAT_FIA_Pos);
}

/*!
 * @brief Get internal authentication timeout fail status.
 *
 * @param[in] none
 * @return The internal authentication timeout fail state
            0: internal authentication timeout fail
            1: internal authentication no timeout
 */
__STATIC_INLINE uint8_t HSM_InternalAuthTimeoutFlag(void)
{
    return (uint8_t)((HSM->AUTH_STAT & HSM_AUTH_STAT_IAT_Msk) >> HSM_AUTH_STAT_IAT_Pos);
}

/*!
 * @brief Get internal authentication done status.
 *
 * @param[in] none
 * @return The internal authentication done state
            0: internal authentication not done
            1: internal authentication Done
 */
__STATIC_INLINE uint8_t HSM_InternalAuthDoneFlag(void)
{
    return (uint8_t)((HSM->AUTH_STAT & HSM_AUTH_STAT_IAD_Msk) >> HSM_AUTH_STAT_IAD_Pos);
}

/*!
 * @brief Clear all authentication status.
 *
 * @param[in] none
 * @return none
 */
__STATIC_INLINE void HSM_ClearInternalAuthStatusFlag(void)
{
    HSM->AUTH_STAT |= (HSM_AUTH_STAT_IAD_Msk | HSM_AUTH_STAT_IAT_Msk
                      | HSM_AUTH_STAT_FIA_Msk | HSM_AUTH_STAT_KRA_Msk
                      | HSM_AUTH_STAT_KIA_Msk | HSM_AUTH_STAT_CLA_Msk);
}

/*!
 * @brief Interrupt enable/disable register.
 *
 * @param[in] state: enabling state
 *               - ENABLE
 *               - DISABLE
 * @return none
 */
__STATIC_INLINE void HSM_EnableIrq(ACTION_Type state)
{
    MODIFY_REG32(HSM->SYS_CTRL, HSM_SYS_CTRL_IRQEN_Msk, HSM_SYS_CTRL_IRQEN_Pos, state);
}

/*!
 * @brief Clear interrupt flag register.
 *
 * @param[in] none
 * @return none
 */
__STATIC_INLINE void HSM_ClearIrqFlag(void)
{
    HSM->STAT |= HSM_STAT_IRQ_CLR_Msk;
}

/*!
 * @brief Get command status register.
 *
 * @param[in] none
 * @return HSM comand state
 */
__STATIC_INLINE uint16_t HSM_GetCmdStatus(void)
{
    return (uint16_t)(HSM->STAT & 0x7FFU);
}

/*!
 * @brief clear command status register.
 *
 * @param[in] none
 * @return HSM comand state
 */
__STATIC_INLINE void HSM_ClearCmdStatus(void)
{
    HSM->STAT |= 0x10000U;
}

/*!
 * @brief DMA enable/disable register.
 *
 * @param[in] state: enabling state
 *               - ENABLE
 *               - DISABLE
 * @return none
 */
__STATIC_INLINE void HSM_EnableDma(ACTION_Type state)
{
    MODIFY_REG32(HSM->SYS_CTRL, HSM_SYS_CTRL_DMAEN_Msk, HSM_SYS_CTRL_DMAEN_Pos, state);
}

/*!
 * @brief get hsm lock/unlock left switch times.
 *
 * @param[in] none
 * @return HSM lock state
 */
__STATIC_INLINE uint8_t HSM_GetSwitchLeftTimes(void)
{
#ifdef FLASH_ENABLE
    return (uint8_t)((EFLASH->LCR & EFLASH_LCR_LSLT_Msk) >> EFLASH_LCR_LSLT_Pos);
#else
    return 0U;
#endif /* FLASH_ENABLE */
}

/*!
 * @brief get hsm lock/unlock status.
 *
 * @param[in] none
 * @return HSM lock state
 */
__STATIC_INLINE uint8_t HSM_GetChipState(void)
{
#ifdef FLASH_ENABLE
    return (uint8_t)((EFLASH->LCR & EFLASH_LCR_LS_Msk) >> EFLASH_LCR_LS_Pos);
#else
    return 0U;
#endif /* FLASH_ENABLE */
}

#ifdef __cplusplus
}
#endif

#endif /* AC780X_HSM_REG_H */

/* =============================================  EOF  ============================================== */
