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
 * @file ac780x_eflash.h
 *
 * @brief This file provides EFLASH integration functions interface.
 *
 */

#ifndef AC780X_EFLASH_H
#define AC780X_EFLASH_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ===========================================  Includes  =========================================== */
#include "ac7805x.h"

/*=====================================SOURCE FILE VERSION INFORMATION==============================*/
#define EFLASH_HAL_MODULE_ID                    (311U)
#define EFLASH_HAL_SW_MAJOR_VERSION             (1U)
#define EFLASH_HAL_SW_MINOR_VERSION             (1U)
#define EFLASH_HAL_SW_PATCH_VERSION             (1U)

/* ============================================  Define  ============================================ */
#define PFLASH_BASE_ADDRESS         (0x08000000UL)                      /*!< Based address of P-flash user area */
#define PFLASH_SIZE                 (0x00040000UL)                      /*!< Total size for P-flash user area */
#define PFLASH_PAGE_SIZE            (0x00001000UL)                      /*!< P-flash page size */
#define PFLASH_END_ADDRESS          (PFLASH_BASE_ADDRESS + PFLASH_SIZE) /*!< End address of P-flash user area */
#define PFLASH_PAGE_AMOUNT          (PFLASH_SIZE / PFLASH_PAGE_SIZE)    /*!< P-flash page total page amount */
#define PFLASH_WRITE_UNIT_SIZE      (8UL)                               /*!< P-Flash program unit size */

#define DFLASH_BASE_ADDRESS         (0x08100000UL)                      /*!< Based address of D-flash user area */
#define DFLASH_SIZE                 (0x00020000UL)                      /*!< Total size for D-flash user area */
#define DFLASH_PAGE_SIZE            (0x00000800UL)                      /*!< D-flash page size */
#define DFLASH_END_ADDRESS          (DFLASH_BASE_ADDRESS + DFLASH_SIZE) /*!< End address of D-flash user area */
#define DFLASH_PAGE_AMOUNT          (DFLASH_SIZE / DFLASH_PAGE_SIZE)    /*!< D-flash page total page amount */
#define DFLASH_WRITE_UNIT_SIZE      (8UL)                               /*!< D-Flash program unit size */

#define CINFO_BASE                  (0x08200000UL)                      /*!< Control info base address */
#define OPTION_BASE                 (0x08200000UL)                      /*!< Option byte base address */
#define OPTION_END                  (0x08202000UL)                      /*!< Option byte end address */
#define CINFO_END                   (0x08202000UL)                      /*!< Control info end address */
#define CINFO_PAGE_SIZE             (0x00001000UL)                      /*!< Control info page size */

#define PFLASH_WRITE_PROTECT_BASE   (CINFO_BASE + CINFO_PAGE_SIZE)      /*!< P-Flash write protect address */
#define DFLASH_WRITE_PROTECT_BASE   (CINFO_BASE + CINFO_PAGE_SIZE + 0x10UL) /*!< D-Flash write protect address */
#define EXTERN_RESET_CTRL_BASE      (CINFO_BASE + CINFO_PAGE_SIZE + 0x20UL) /*!< External reset control address */

#define DINFO_BASE                  (0x08300000UL)                      /*!< Data info base address */
#define DINFO_END                   (0x08301800UL)                      /*!< Data info end address */
#define DINFO_PAGE_SIZE             (0x00000800UL)                      /*!< Data info page size */

#define EFLASH_MAIN_STAT_ERROR_BITS     (EFLASH_CSR_PWVIO_Msk  | \
                                         EFLASH_CSR_RVIO_Msk   | \
                                         EFLASH_CSR_DWVIO_Msk  | \
                                         EFLASH_CSR_PGMERR_Msk | \
                                         EFLASH_CSR_ERSERR_Msk | \
                                         EFLASH_CSR_VRFERR_Msk | \
                                         EFLASH_CSR_ACCERR_Msk)

#define EFLASH_OPTION_STAT_ERROR_BITS   (EFLASH_CSR_OPTRERR_Msk  | \
                                         EFLASH_CSR_OPTPWERR_Msk | \
                                         EFLASH_CSR_OPTDWERR_Msk)

#define EFLASH_CLEAR_STAT_ERROR_BITS()   (EFLASH->CSR |= \
                                          EFLASH_CSR_PWVIO_Msk | \
                                          EFLASH_CSR_RVIO_Msk  | \
                                          EFLASH_CSR_DWVIO_Msk)

/* ===========================================  Typedef  ============================================ */
/*!
 * @brief FLASH command enumeration.
 */
typedef enum
{
    EFLASH_CMD_IDLE             = 0x00U,     /*!< Idle command */
    EFLASH_CMD_PAGERASE         = 0x01U,     /*!< Page erase command */
    EFLASH_CMD_PROTECTERASE     = 0x02U,     /*!< Protect byte page erase command */
    EFLASH_CMD_MASSRASE         = 0x03U,     /*!< Mass erase command */
    EFLASH_CMD_PAGEPROGRAM      = 0x11U,     /*!< Page program command */
    EFLASH_CMD_PROTECTROGRAM    = 0x12U,     /*!< Protect byte page program command */
    EFLASH_CMD_PAGERASEVERIFY   = 0x21U,     /*!< Page erase verify command */
    EFLASH_CMD_MASSRASEVERIFY   = 0x22U      /*!< Mass erase verify command */
} EFLASH_CmdType;                            /*!< eflash command */

/* ====================================  Functions declaration  ===================================== */
/*!
 * @brief Unlock the eflash controller
 *
 * @param[in] none
 * @return EFLASH STATUS: eflash operation status
 */
status_t EFLASH_UnlockCtrl(void);

/*!
 * @brief Lock the eflash control
 *
 * @param[in] none
 * @return none
 */
void EFLASH_LockCtrl(void);

/*!
 * @brief Read a specified length of data from the eflash specified address
 *
 * @param[in] addr: eflash address
 * @param[in] buffer: Point to the data bufffer for saving the read data
 * @param[in] len: read length(the unit is word)
 * @return none
 */
void EFLASH_Read(uint32_t addr, uint32_t *buffer, uint32_t len);

/*!
 * @brief Erase specified eflash user area address.
 *
 * @param[in] addr: Specified eflash addrees to be erased
 * @return EFLASH STATUS: eflash operation status
 */
status_t EFLASH_PageErase(uint32_t addr);

/*!
 * @brief Mass erase elfash user area.
 *
 * @param[in] addr: Specified eflash addrees to be erased
 * @return EFLASH STATUS: eflash operation status
 */
status_t EFLASH_MassErase(uint32_t addr);

/*!
 * @brief User page program.
 *
 * @param[in] addr: eflash addrees to be programed (should be 8-aligned)
 * @param[in] buffer: Point to the data to be programed
 * @param[in] len: Data length to be programed (the unit is 32-bit, should be 2-aligned)
 * @return EFLASH STATUS: eflash operation status
 */
status_t EFLASH_PageProgram(uint32_t addr, uint32_t *buffer, uint32_t len);

/*!
 * @brief Verify whether the mass erase operation is performed successfully.
 *
 * @param[in] addr: Specified eflash addrees to be verified
 * @return EFLASH STATUS: eflash operation status
 */
status_t EFLASH_MassEraseVerify(uint32_t addr);

/*!
 * @brief Verify whether the page erase operation is performed successfully.
 *
 * @param[in] addr: Specified eflash addrees to be verified
 * @param[in] len: Specified page erase verify length(the unit is 32-bit, should be 2-aligned)
 * @return EFLASH STATUS: eflash operation status
 */
status_t EFLASH_PageEraseVerify(uint32_t addr, uint32_t len);

/*!
 * @brief Enable eflash read protection
 *
 * This API is used to set(enable) eflash read protection. Set(enable) read
 * protection will take effect after reset and the all eflash main memory(user
 * pages) will return with 0xAAAAAAAA when read by JTAG/SWD. EFLASH main memory
 * can not be erased or programed by code/JTAG/SWD when in read protection status.
 *
 * @param[in] none
 * @return EFLASH STATUS: eflash operation status
 */
status_t EFLASH_EnableReadProtection(void);

/*!
 * @brief Disable EFLASH read protection.
 *
 * This API is used to clear(disable) EFLASH read protection. Clear(disable) read
 * protection will take effect after reset and the EFLASH main memory(user pages)
 * will be erased to 0xFFFFFFFF automatically.
 *
 * @param[in] none
 * @return EFLASH STATUS: eflash operation status
 */
status_t EFLASH_DisableReadProtection(void);

/*!
 * @brief Set P-Flash write protection.
 *
 * @param[in] protectStatus: write protection status value
 * @return EFLASH STATUS: eflash operation status
 */
status_t EFLASH_PFSetWriteProtection(uint32_t *protectStatus);

/*!
 * @brief Set D-Flash write protection.
 *
 * @param[in] protectStatus: write protection status value
 * @return EFLASH STATUS: eflash operation status
 */
status_t EFLASH_DFSetWriteProtection(uint32_t *protectStatus);

/*!
 * @brief Set external reset enable or disable.
 *
 * @param[in] state: external reset status
 * @return EFLASH STATUS: eflash operation status
 */
status_t EFLASH_SetExReset(ACTION_Type state);

/*!
 * @brief Set P&D-Flash write protection by register.
 *
 * @param[in] sel: P&D-Flash select
              - 0: P-Flash
              - 1: D-Flash
 * @param[in] protectStatusLow: write protection status value for page 0~31
 * @param[in] protectStatusHigh: write protection status value for page 32~63
 * @return EFLASH STATUS: eflash operation status
 */
status_t EFLASH_SetWriteProtectionByReg(uint8_t sel, uint32_t protectStatusLow, uint32_t protectStatusHigh);

/*!
 * @brief Get module version information.
 *
 * @param[out] versionInfo: Module version information address.
 * @return void
 */
void EFLASH_GetVersionInfo(VersionInfo_Type *versionInfo);

#ifdef __cplusplus
}
#endif

#endif /* AC780X_EFLASH_H */

/* =============================================  EOF  ============================================== */
