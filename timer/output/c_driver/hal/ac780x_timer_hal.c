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
 * @file ac780x_timer_hal.c
 *
 * @brief This file provides TIMER HAL (Hardware Abstraction Layer) functions.
 *
 */

/* ===========================================  Includes  =========================================== */
#include "ac780x_timer_reg.h"
#include "ac780x_timer_hal.h"

/* ============================================  Define  ============================================ */

/* ===========================================  Typedef  ============================================ */

/* ==========================================  Variables  =========================================== */

/* ====================================  Functions declaration  =================================== */

/* =====================================  Functions definition  =================================== */

/*!
 * @brief Initialize TIMER HAL layer
 *
 * @param[in] none
 *
 * @return HAL status code
 */
HAL_Status_Type TIMER_HAL_Init(void)
{
    /* TIMER HAL initialization */
    return HAL_OK;
}

/*!
 * @brief Deinitialize TIMER HAL layer
 *
 * @param[in] none
 *
 * @return HAL status code
 */
HAL_Status_Type TIMER_HAL_Deinit(void)
{
    /* TIMER HAL deinitialization */
    return HAL_OK;
}

/*!
 * @brief Read TIMER register
 *
 * @param[in] address Register address
 *
 * @return Register value
 */
uint32_t TIMER_HAL_ReadReg(uint32_t address)
{
#ifdef REAL_CHIP
    return READ_MEM32(address);
#else
    /* Simulation mode - use interface layer */
    return RegReadEx(address);
#endif
}

/*!
 * @brief Write TIMER register
 *
 * @param[in] address Register address
 * @param[in] value   Value to write
 *
 * @return none
 */
void TIMER_HAL_WriteReg(uint32_t address, uint32_t value)
{
#ifdef REAL_CHIP
    WRITE_MEM32(address, value);
#else
    /* Simulation mode - use interface layer */
    RegWriteEx(address, value);
#endif
}

/*!
 * @brief Modify TIMER register bits
 *
 * @param[in] address Register address
 * @param[in] mask    Bit mask
 * @param[in] pos     Bit position
 * @param[in] value   Value to set
 *
 * @return none
 */
void TIMER_HAL_ModifyReg(uint32_t address, uint32_t mask, uint32_t pos, uint32_t value)
{
#ifdef REAL_CHIP
    MODIFY_MEM32(address, mask, pos, value);
#else
    /* Simulation mode - use interface layer */
    uint32_t regValue = RegReadEx(address);
    regValue = (regValue & (~mask)) | ((value << pos) & mask);
    RegWriteEx(address, regValue);
#endif
}
