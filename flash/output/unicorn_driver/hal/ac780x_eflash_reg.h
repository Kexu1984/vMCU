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
 * @file ac780x_eflash_reg.h
 *
 * @brief EFLASH access register inline function definition.
 *
 */

#ifndef AC780X_EFLASH_REG_H
#define AC780X_EFLASH_REG_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ===========================================  Includes  =========================================== */
#include "ac780x_eflash.h"

/* ======================================  Functions define  ======================================== */
/*!
 * @brief Set eflash key sequence register.
 *
 * @param[in] keyValue: unlock eflash controler key value
 * @return none
 */
__STATIC_INLINE void EFLASH_SetKeyReg(uint32_t keyValue)
{
    EFLASH->UKR = keyValue;
}

/*!
 * @brief Get read protection status.
 *
 * @param[in] none
 * @return 0: not in read protection, 1: in read protection
 */
__STATIC_INLINE uint8_t EFLASH_GetReadProtectReg(void)
{
    return (uint8_t)((EFLASH->GSR & EFLASH_GSR_RPRT_Msk) >> EFLASH_GSR_RPRT_Pos);
}

/*!
 * @brief Get the lock bit of eflash controler.
 *
 * @param[in] none
 * @return 0: eflash controler is unlocked, 1: eflash controler is locked
 */
__STATIC_INLINE uint8_t EFLASH_GetCtrlLockBitReg(void)
{
    return (uint8_t)(EFLASH->GSR & EFLASH_GSR_LOCK_Msk) >> EFLASH_GSR_LOCK_Pos;
}

/*!
 * @brief Lock eflash controler.
 *
 * @param[in] none
 * @return none
 */
__STATIC_INLINE void EFLASH_LockCtrlReg(void)
{
    EFLASH->GSR |= EFLASH_GSR_LOCK_Msk;
}

/*!
 * @brief Set program or erase start address.
 *
 * @param[in] startAddress: start address
 * @return none
 */
__STATIC_INLINE void EFLASH_SetStartAddressReg(uint32_t startAddress)
{
    EFLASH->CAR = startAddress;
}

/*!
 * @brief Set command.
 *
 * @param[in] cmd: eflash command
 * @return none
 */
__STATIC_INLINE void EFLASH_SetCommandReg(uint32_t cmd)
{
    MODIFY_REG32(EFLASH->CCR, EFLASH_CCR_CMD_Msk, EFLASH_CCR_CMD_Pos, cmd);
}

/*!
 * @brief Set eflash length field.
 *
 * @param[in] len: length(unit is 64-bit)
 * @return none
 */
__STATIC_INLINE void EFLASH_SetLengthReg(uint32_t len)
{
    MODIFY_REG32(EFLASH->CCR, EFLASH_CCR_LEN_Msk, EFLASH_CCR_LEN_Pos, len);
}

/*!
 * @brief Set eflash data register 0.
 *
 * @param[in] data: eflash data
 * @return none
 */
__STATIC_INLINE void EFLASH_SetDataReg0(uint32_t data)
{
    EFLASH->CDR0 = data;
}

/*!
 * @brief Set eflash data register 1.
 *
 * @param[in] data: eflash data
 * @return none
 */
__STATIC_INLINE void EFLASH_SetDataReg1(uint32_t data)
{
    EFLASH->CDR1 = data;
}

/*!
* @brief Trigger the command to start.
*
* @param[in] none
* @return none
*/
__STATIC_INLINE void EFLASH_TrigCtrlCmdReg(void)
{
    EFLASH->CCR |= EFLASH_CCR_START_Msk;
}

/*!
 * @brief  Get status that if program the last data.
 *
 * @param[in]  none
 * @return last data status
 *         - 0: not last data
 *         - 1: last data
*/
__STATIC_INLINE uint8_t EFLASH_GetLastDataStatusReg(void)
{
    return (uint8_t)((EFLASH->CSR & EFLASH_CSR_DBUFLST_Msk) >> EFLASH_CSR_DBUFLST_Pos);
}

/*!
 * @brief  Get status that if data buffer ready.
 *
 * @param[in] none
 * @return buffer ready status
 *         - 0: data buffer not ready
 *         - 1: data buffer ready
*/
__STATIC_INLINE uint8_t EFLASH_GetDataBufferReg(void)
{
    return (uint8_t)((EFLASH->CSR & EFLASH_CSR_DBUFRDY_Msk) >> EFLASH_CSR_DBUFRDY_Pos);
}

/*!
 * @brief set option byte enable register.
 *
 * @param[in] enable: enable/disable operation option byte area
                - ENABLE
                - DISABLE
 * @return none
 */
__STATIC_INLINE void EFLASH_EnableOptionReg(ACTION_Type enable)
{
    MODIFY_REG32(EFLASH->CCR, EFLASH_CCR_OPTEN_Msk, EFLASH_CCR_OPTEN_Pos, enable);
}

/*!
 * @brief Hardfault interrupt enable.
 *
 * @param[in] none
 * @return none
 */
__STATIC_INLINE void EFLASH_EnableCtrlHardfault(void)
{
    EFLASH->GCR |= EFLASH_GCR_HDFEN_Msk;
}

/*!
 * @brief Hardfault interrupt Disable.
 *
 * @param[in] none
 * @return none
 */
__STATIC_INLINE void EFLASH_DisableCtrlHardfault(void)
{
    EFLASH->GCR &= (~EFLASH_GCR_HDFEN_Msk);
}

/*!
 * @brief  Get status that indicate whether any of the command operations is in process.
 *
 * @param[in]  none
 * @return cmd busy status
 *         - 0: all operations are not in process
 *         - 1: at least one operation in process
*/
__STATIC_INLINE uint8_t EFLASH_GetCmdBusyStatus(void)
{
    return (uint8_t)((EFLASH->CSR & EFLASH_CSR_CMDBUSY_Msk) >> EFLASH_CSR_CMDBUSY_Pos);
}

/*!
 * @brief  Get status that indicate whether LVD warning is generated.
 *
 * @param[in] none
 * @return LVD warning flag
 *         - 0: no LVD warning
 *         - 1: LVD warning is generated
*/
__STATIC_INLINE uint8_t EFLASH_GetLVDWarningReg(void)
{
    return (uint8_t)((EFLASH->LVD & EFLASH_LVD_LVDSTAT_Msk) >> EFLASH_LVD_LVDSTAT_Pos);
}

/*!
 * @brief Flash LVDMON Enable.
 *
 * @param[in] none
 * @return none
 */
__STATIC_INLINE void EFLASH_EnableLVDMON(void)
{
    EFLASH->LVD |= (1UL << EFLASH_LVD_LVDEN_Pos);
}

/*!
 * @brief Flash LVDMON Disable.
 *
 * @param[in] none
 * @return none
 */
__STATIC_INLINE void EFLASH_DisableLVDMON(void)
{
    EFLASH->LVD &= (~EFLASH_LVD_LVDEN_Msk);
}

/*!
 * @brief Get the error status for main memory.
 *
 * @param[in] none
 * @return status value
 */
__STATIC_INLINE uint32_t EFLASH_GetStatusReg(void)
{
    return (EFLASH->CSR & EFLASH_MAIN_STAT_ERROR_BITS);
}

/*!
 * @brief Get the error status for option memory.
 *
 * @param[in] none
 * @return status value
 */
__STATIC_INLINE uint32_t EFLASH_GetOptionStatusReg(void)
{
    return (EFLASH->CSR & EFLASH_OPTION_STAT_ERROR_BITS);
}

/*!
 * @brief fore the program command operation to be finished.
 *
 * @param[in] none
 * @return none
 */
__STATIC_INLINE void EFLASH_FlushOperation(void)
{
    MODIFY_REG32(EFLASH->CCR, EFLASH_CCR_ABORT_Msk, EFLASH_CCR_ABORT_Pos, 1);
}

/*!
 * @brief Get Write protect register 0 value of P-flash.
 *
 * @param[in] none
 * @return Write protect register 0 value  of P-flash
 */
__STATIC_INLINE uint32_t EFLASH_GetPWriteProRegLow(void)
{
    return (EFLASH->PWPR0);
}

/*!
 * @brief Get Write protect register 1 value of P-flash.
 *
 * @param[in] none
 * @return Write protect register 1 value of P-flash
 */
__STATIC_INLINE uint32_t EFLASH_GetPWriteProRegHigh(void)
{
    return (EFLASH->PWPR1);
}

/*!
 * @brief Get Write protect register 0 value of D-flash.
 *
 * @param[in] none
 * @return Write protect register 0 value of D-flash
 */
__STATIC_INLINE uint32_t EFLASH_GetDWriteProRegLow(void)
{
    return (EFLASH->DWPR0);
}

/*!
 * @brief Get Write protect register 1 value of D-flash.
 *
 * @param[in] none
 * @return Write protect register 1 value of D-flash
 */
__STATIC_INLINE uint32_t EFLASH_GetDWriteProRegHigh(void)
{
    return (EFLASH->DWPR1);
}

/*!
 * @brief Set P-Flash page0~31 write protection status by register.
 *
 * @param[in] protectStatus: P-Flash write protection status value
 * @return none
 */
__STATIC_INLINE void EFLASH_SetPWriteProRegLow(uint32_t protectStatus)
{
    EFLASH->PWPR0 = protectStatus;
}

/*!
 * @brief Set P-Flash page32~63 write protection status by register.
 *
 * @param[in] protectStatus: P-Flash write protection status value
 * @return none
 */
__STATIC_INLINE void EFLASH_SetPWriteProRegHigh(uint32_t protectStatus)
{
    EFLASH->PWPR1 = protectStatus;
}

/*!
 * @brief Set D-Flash page0~31 write protection status by register.
 *
 * @param[in] protectStatus: D-Flash write protection status value
 * @return none
 */
__STATIC_INLINE void EFLASH_SetDWriteProRegLow(uint32_t protectStatus)
{
    EFLASH->DWPR0 = protectStatus;
}

/*!
 * @brief Set D-Flash page32~63 write protection status by register.
 *
 * @param[in] protectStatus: D-Flash write protection status value
 * @return none
 */
__STATIC_INLINE void EFLASH_SetDWriteProRegHigh(uint32_t protectStatus)
{
    EFLASH->DWPR1 = protectStatus;
}

/*!
 * @brief Get ECC error status of P-flash.
 *
 * @param[in] none
 * @return ECC error status of P-flash.
 */
__STATIC_INLINE uint32_t EFLASH_GetPECCErrorStatus(void)
{
    return ((EFLASH->CSR & EFLASH_CSR_PECCSTA_Msk) >> EFLASH_CSR_PECCSTA_Pos);
}

/*!
 * @brief Get ECC error status of D-flash.
 *
 * @param[in] none
 * @return ECC error status of D-flash.
 */
__STATIC_INLINE uint32_t EFLASH_GetDECCErrorStatus(void)
{
    return ((EFLASH->CSR & EFLASH_CSR_DECCSTA_Msk) >> EFLASH_CSR_DECCSTA_Pos);
}

/*!
 * @brief Clear ECC 2-bit error status of P-flash.
 *
 * @param[in] none
 * @return none
 */
__STATIC_INLINE void EFLASH_ClearPECCErrorStatus(void)
{
    EFLASH->CSR |= EFLASH_CSR_PECCSTA_Msk;
}

/*!
 * @brief Clear ECC 2-bit error status of D-flash.
 *
 * @param[in] none
 * @return none
 */
__STATIC_INLINE void EFLASH_ClearDECCErrorStatus(void)
{
    EFLASH->CSR |= EFLASH_CSR_DECCSTA_Msk;
}

/*!
 * @brief Get ECC 2-bit error address of P-flash.
 *
 * @param[in] none
 * @return ECC 2-bit error address of P-flash
 */
__STATIC_INLINE uint32_t EFLASH_GetPECCErrorAddress(void)
{
    return (EFLASH->PEADR);
}

/*!
 * @brief Get ECC 2-bit error address of D-flash.
 *
 * @param[in] none
 * @return ECC 2-bit error address of D-flash
 */
__STATIC_INLINE uint32_t EFLASH_GetDECCErrorAddress(void)
{
    return (EFLASH->DEADR);
}

/*!
 * @brief Clear ECC 2-bit error address of P-flash.
 *
 * @param[in] none
 * @return none
 */
__STATIC_INLINE void EFLASH_ClearPECCErrorAddress(void)
{
    EFLASH->PEADR = 0U;
}

/*!
 * @brief Clear ECC 2-bit error address of D-flash.
 *
 * @param[in] none
 * @return none
 */
__STATIC_INLINE void EFLASH_ClearDECCErrorAddress(void)
{
    EFLASH->DEADR = 0U;
}

/*!
 * @brief Clear D-Flash Cache.
 *
 * @param[in] none
 * @return none
 */
__STATIC_INLINE void EFLASH_ClearDCache(void)
{
    EFLASH->GCR |= (EFLASH_GCR_DCACHECLR_Msk);
}

/*!
 * @brief Clear P-Flash Cache.
 *
 * @param[in] none
 * @return none
 */
__STATIC_INLINE void EFLASH_ClearPCache(void)
{
    EFLASH->GCR |= (EFLASH_GCR_PCACHECLR_Msk);
}

/*!
 * @brief Get secure boot enable status.
 *
 * @param[in] none
 * @return secure boot enable status
 */
__STATIC_INLINE uint8_t EFLASH_GetSecureBootEnableStatus(void)
{
    return ((EFLASH->GSR & EFLASH_GSR_SECBOOT_Msk) >> EFLASH_GSR_SECBOOT_Pos);
}

/*!
 * @brief Get the status of whether to bypass secure boot.
 *
 * @param[in] none
 * @return Bypass secure boot status
 */
__STATIC_INLINE uint8_t EFLASH_GetBypassSecureBootStatus(void)
{
    return ((EFLASH->GSR & EFLASH_GSR_SECBOOTWK_Msk) >> EFLASH_GSR_SECBOOTWK_Pos);
}

/*!
 * @brief Get the left times to switch unclok/lock.
 *
 * @param[in] none
 * @return Left times to switch unclok/lock
 */
__STATIC_INLINE uint8_t EFLASH_GetLeftTimes(void)
{
    return ((EFLASH->LCR & EFLASH_LCR_LSLT_Msk) >> EFLASH_LCR_LSLT_Pos);
}

/*!
 * @brief Get lock status.
 *
 * @param[in] none
 * @return Lock status
 */
__STATIC_INLINE uint8_t EFLASH_GetLockStatus(void)
{
    return ((EFLASH->LCR & EFLASH_LCR_LS_Msk) >> EFLASH_LCR_LS_Pos);
}

/*!
 * @brief Get the the number of flash pages occupied by image area.
 *
 * @param[in] none
 * @return The number of flash pages occupied by image area
 */
__STATIC_INLINE uint32_t EFLASH_GetImagePageNum(void)
{
    return ((EFLASH->SECSIZE & EFLASH_SECSIZE_IMGSIZE_Msk) >> EFLASH_SECSIZE_IMGSIZE_Pos);
}

/*!
 * @brief Get the the number of flash pages occupied by rom area.
 *
 * @param[in] none
 * @return The number of flash pages occupied by rom area
 */
__STATIC_INLINE uint32_t EFLASH_GetRomPageNum(void)
{
    return ((EFLASH->SECSIZE & EFLASH_SECSIZE_ROMSIZE_Msk) >> EFLASH_SECSIZE_ROMSIZE_Pos);
}

/*!
 * @brief Get the start address of image area.
 *
 * @param[in] none
 * @return The start address of image area
 */
__STATIC_INLINE uint32_t EFLASH_ImageStartAddr(void)
{
    return (EFLASH->IMGADR);
}

#ifdef __cplusplus
}
#endif

#endif /* AC780X_EFLASH_REG_H */

/* =============================================  EOF  ============================================== */
