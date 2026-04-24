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
 * @file ac780x_timer.h
 *
 * @brief This file provides timer module integration functions interfaces.
 *
 */

#ifndef AC780X_TIMER_H
#define AC780X_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

/* ===========================================  Includes  =========================================== */
#include "ac7805x.h"

/* ============================================  Define  ============================================ */
/*!
 * @brief TIMER channel instance index macro.
 */
#define TIMER_CHANNEL_INDEX(TIMERCHx)    ((uint8_t)((((uint32_t)&(TIMERCHx)->LDVAL) - TIMER_CHANNEL0_BASE) >> 4U))

/* ===========================================  Typedef  ============================================ */
/*!
 * @brief TIMER channel configuration structure.
 */
typedef struct
{
    uint32_t periodValue;           /*!< TIMER channel period value */
    ACTION_Type chainModeEn;         /*!< TIMER channel linkmode enable */
    ACTION_Type interruptEn;        /*!< TIMER channel interrupt enable */
    DeviceCallback_Type callBack;   /*!< TIMER channel callback pointer */
    ACTION_Type timerEn;            /*!< TIMER channel enable/disable */
} TIMER_ConfigType;

/* ==========================================  Variables  =========================================== */

/* ====================================  Functions declaration  ===================================== */
/*!
 * @brief TIMER initialize.
 *
 * @param[in] TIMERCHx: timer channel
                - TIMER_CHANNEL0
                - TIMER_CHANNEL1
                - TIMER_CHANNEL2
                - TIMER_CHANNEL3
 * @param[in] config: pointer to configuration structure
 * @return none
 */
void TIMER_Init(TIMER_CHANNEL_Type *TIMERCHx, const TIMER_ConfigType *config);

/*!
 * @brief TIMER De-initialize.
 *
 * close all channels at the same time
 *
 * @param[in] none
 * @return none
 */
void TIMER_DeInit(void);

/*!
 * @brief TIMER channel De-initialize.
 *
 * only close single channel
 *
 * @param[in] TIMERCHx: timer channel
                - TIMER_CHANNEL0
                - TIMER_CHANNEL1
                - TIMER_CHANNEL2
                - TIMER_CHANNEL3
 * @return none
 */
void TIMER_DeInitChannel(TIMER_CHANNEL_Type *TIMERCHx);

/*!
 * @brief Set timer callback function.
 *
 * @param[in] TIMERCHx: timer channel
                - TIMER_CHANNEL0
                - TIMER_CHANNEL1
                - TIMER_CHANNEL2
                - TIMER_CHANNEL3
 * @return none
 */
void TIMER_SetCallback(TIMER_CHANNEL_Type *TIMERCHx, const DeviceCallback_Type func);

#ifdef __cplusplus
}
#endif

#endif /* AC780X_TIMER_H */

/* =============================================  EOF  ============================================== */
