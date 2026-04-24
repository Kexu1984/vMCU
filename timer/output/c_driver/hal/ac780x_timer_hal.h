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
 * @file ac780x_timer_hal.h
 *
 * @brief This file provides TIMER HAL (Hardware Abstraction Layer) function declarations.
 *
 */

#ifndef AC780X_TIMER_HAL_H
#define AC780X_TIMER_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* ===========================================  Includes  =========================================== */
#include <stdint.h>

#ifndef REAL_CHIP
#include "interface_layer.h"
#endif

/* ============================================  Define  ============================================ */

/* Base address for TIMER module */
#define TIMER_BASE_ADDRESS    0x40011000UL

/* ===========================================  Typedef  ============================================ */

/*!
 * @brief HAL Status enumeration
 */
typedef enum {
    HAL_OK = 0,
    HAL_ERROR = 1,
    HAL_BUSY = 2,
    HAL_TIMEOUT = 3
} HAL_Status_Type;

/* ====================================  Functions declaration  =================================== */

/*!
 * @brief Initialize TIMER HAL layer
 *
 * @param[in] none
 *
 * @return HAL status code
 */
HAL_Status_Type TIMER_HAL_Init(void);

/*!
 * @brief Deinitialize TIMER HAL layer
 *
 * @param[in] none
 *
 * @return HAL status code
 */
HAL_Status_Type TIMER_HAL_Deinit(void);

/*!
 * @brief Read TIMER register
 *
 * @param[in] address Register address
 *
 * @return Register value
 */
uint32_t TIMER_HAL_ReadReg(uint32_t address);

/*!
 * @brief Write TIMER register
 *
 * @param[in] address Register address
 * @param[in] value   Value to write
 *
 * @return none
 */
void TIMER_HAL_WriteReg(uint32_t address, uint32_t value);

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
void TIMER_HAL_ModifyReg(uint32_t address, uint32_t mask, uint32_t pos, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif /* AC780X_TIMER_HAL_H */
