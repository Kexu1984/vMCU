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
 * @file ac780x_timer_reg.h
 *
 * @brief Timer module access register inline function definition.
 *
 */

#ifndef AC780X_TIMER_REG_H
#define AC780X_TIMER_REG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ===========================================  Includes  =========================================== */
#include "ac780x_timer.h"

/* ============================================  Define  ============================================ */

/* ===========================================  Typedef  ============================================ */

/* ==========================================  Variables  =========================================== */

/* ====================================  Functions declaration  ===================================== */

/* ======================================  Functions define  ======================================== */
#define __STATIC_INLINE static inline
/*!
 * @brief Enable/disable timer module.
 *
 * This function enable the TIMER module.
 * It should be called before setup any timer channel.
 *
 * @param[in] state: enabling state
 *               - ENABLE
 *               - DISABLE
 * @return none
 */
__STATIC_INLINE void TIMER_Enable(ACTION_Type state)
{
    MODIFY_REG32(TIMER_CTRL->MCR, TIMER_CTRL_MCR_MDIS_Msk, TIMER_CTRL_MCR_MDIS_Pos, state);
}

/*!
* @brief Set timer channel period load value.
*
* @param[in] TIMERCHx: timer channel
                - TIMER_CHANNEL0
                - TIMER_CHANNEL1
                - TIMER_CHANNEL2
                - TIMER_CHANNEL3
* @param[in] value: period value
* @return none
*/
#define TIMER_SetPeriodLoadValue(TIMERCHx, value)      ((TIMERCHx)->LDVAL = (value))

/*!
* @brief Set timer channel current timer value.
*
* @param[in] TIMERCHx: timer channel
                - TIMER_CHANNEL0
                - TIMER_CHANNEL1
                - TIMER_CHANNEL2
                - TIMER_CHANNEL3
* @param[in] value: current timer value
* @return none
*/
#define TIMER_SetCurrentValue(TIMERCHx, value)      (TIMERCHx->CVAL = value)

/*!
* @brief Get timer channel period load value.
*
* @param[in] TIMERCHx: timer channel
                - TIMER_CHANNEL0
                - TIMER_CHANNEL1
                - TIMER_CHANNEL2
                - TIMER_CHANNEL3
* @return Timer channel period value
*/
#define TIMER_GetPeriodLoadValue(TIMERCHx)      (TIMERCHx->LDVAL)

/*!
* @brief Get the current timer channel counting value.
*
* @param[in] TIMERCHx: timer channel
                - TIMER_CHANNEL0
                - TIMER_CHANNEL1
                - TIMER_CHANNEL2
                - TIMER_CHANNEL3
* @return Current timer channel counting value
*/
#define TIMER_GetCurrentValue(TIMERCHx)    (TIMERCHx->CVAL)

/*!
* @brief Set timer channel.
*
* @param[in] TIMERCHx: timer channel
                - TIMER_CHANNEL0
                - TIMER_CHANNEL1
                - TIMER_CHANNEL2
                - TIMER_CHANNEL3
* @param[in] state: enabling state
                - ENABLE
                - DISABLE
* @return none
*/
__STATIC_INLINE void TIMER_SetChannel(TIMER_CHANNEL_Type *TIMERCHx, ACTION_Type state)
{
    MODIFY_REG32(TIMERCHx->INIT, TIMER_CHANNEL_INIT_TEN_Msk, TIMER_CHANNEL_INIT_TEN_Pos, state);
}

/*!
* @brief Enable/disable timer channel interrupt.
*
* @param[in] TIMERCHx: timer channel
                - TIMER_CHANNEL0
                - TIMER_CHANNEL1
                - TIMER_CHANNEL2
                - TIMER_CHANNEL3
* @return none
*/
__STATIC_INLINE void TIMER_SetInterrupt(TIMER_CHANNEL_Type *TIMERCHx, ACTION_Type state)
{
    MODIFY_REG32(TIMERCHx->INIT, TIMER_CHANNEL_INIT_TIE_Msk, TIMER_CHANNEL_INIT_TIE_Pos, state);
}

/*!
* @brief Set timer channel chain mode.
*
* timer channel 0 cannot be chain
*
* @param[in] TIMERCHx: timer channel
                - TIMER_CHANNEL0
                - TIMER_CHANNEL1
                - TIMER_CHANNEL2
                - TIMER_CHANNEL3
* @return none
*/
__STATIC_INLINE void TIMER_SetChainMode(TIMER_CHANNEL_Type *TIMERCHx, ACTION_Type state)
{
    MODIFY_REG32(TIMERCHx->INIT, TIMER_CHANNEL_INIT_CHN_Msk, TIMER_CHANNEL_INIT_CHN_Pos, state);
}

/*!
* @brief Get timer channel timeout flag.
*
* @param[in] TIMERCHx: timer channel
                - TIMER_CHANNEL0
                - TIMER_CHANNEL1
                - TIMER_CHANNEL2
                - TIMER_CHANNEL3
* @return Timer channel timeout Flag
*/
#define TIMER_GetTimeoutFlag(TIMERCHx)    READ_BIT32((TIMERCHx)->TF, TIMER_CHANNEL_TF_TFLG_Msk)

/*!
* @brief Get timer channel timeout flag.
*
* @param[in] TIMERCHx: timer channel
                - TIMER_CHANNEL0
                - TIMER_CHANNEL1
                - TIMER_CHANNEL2
                - TIMER_CHANNEL3
* @return Timer channel timeout Flag
*/
#define TIMER_GetTimeInterruptEn(TIMERCHx)    READ_BIT32((TIMERCHx)->INIT, TIMER_CHANNEL_INIT_TIE_Msk)

/*!
* @brief Clear timer channel timeout flag.
*
* @param[in] TIMERCHx: timer channel
                - TIMER_CHANNEL0
                - TIMER_CHANNEL1
                - TIMER_CHANNEL2
                - TIMER_CHANNEL3
* @return none
*/
#define TIMER_ClearTimeoutFlag(TIMERCHx)    SET_BIT32((TIMERCHx)->TF, TIMER_CHANNEL_TF_TFLG_Msk)

#ifdef __cplusplus
}
#endif

#endif /* AC780X_TIMER_REG_H */

/* =============================================  EOF  ============================================== */
