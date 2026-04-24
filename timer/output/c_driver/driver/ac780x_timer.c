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
 * @file ac780x_timer.c
 *
 * @brief This file provides timer module integration functions.
 *
 */

/* ===========================================  Includes  =========================================== */
#include "ac780x_timer_reg.h"

/* ============================================  Define  ============================================ */

/* ===========================================  Typedef  ============================================ */

/* ==========================================  Variables  =========================================== */
/* TIMER callback pointer */
static DeviceCallback_Type s_timerCallback[TIMER_CHANNEL_MAX] = {NULL};

/* TIMER channel base address */
static TIMER_CHANNEL_Type *const s_timerChnBase[TIMER_CHANNEL_MAX] = {TIMER_CHANNEL0, TIMER_CHANNEL1, TIMER_CHANNEL2, TIMER_CHANNEL3};

/* ====================================  Functions declaration  ===================================== */
/* Interrupt handler define in the startup.s */
void TIMER_Channel_IRQHandler(void);

/* ======================================  Functions define  ======================================== */
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
void TIMER_Init(TIMER_CHANNEL_Type *TIMERCHx, const TIMER_ConfigType *config)
{
    uint8_t instance;

    instance = TIMER_CHANNEL_INDEX(TIMERCHx);
    DEVICE_ASSERT(TIMER_CHANNEL_MAX > instance);
    DEVICE_ASSERT(NULL != config);
#ifdef REAL_CHIP
    /* Enbale timer clock */
    CKGEN_Enable(CLK_TIMER, ENABLE);
    CKGEN_SoftReset(SRST_TIMER, ENABLE);
#endif
    /* Enbale timer module */
    TIMER_Enable(ENABLE);

    /* Set timer channel period value */
    TIMER_SetPeriodLoadValue(TIMERCHx, config->periodValue);

    /* Set timer channel link mode */
    TIMER_SetChainMode(TIMERCHx, config->chainModeEn);

    /* Register callback function */
    s_timerCallback[instance] = config->callBack;

    /* Set timer channel interrupt */
    TIMER_SetInterrupt(TIMERCHx, config->interruptEn);
    if (ENABLE == config->interruptEn)
    {
        NVIC_EnableIRQ(TIMER_CHANNEL_IRQn);
    }
    else
    {
        NVIC_DisableIRQ(TIMER_CHANNEL_IRQn);
    }

    /* Enable timer channel */
    TIMER_SetChannel(TIMERCHx, config->timerEn);
}

/*!
 * @brief TIMER De-initialize.
 *
 * close all channels at the same time
 *
 * @param[in] none
 * @return none
 */
void TIMER_DeInit(void)
{
    uint8_t i;

    for (i = 0U; i < TIMER_CHANNEL_MAX; i++)
    {
        s_timerCallback[i] = NULL;
    }

    NVIC_DisableIRQ(TIMER_CHANNEL_IRQn);
    NVIC_ClearPendingIRQ(TIMER_CHANNEL_IRQn);

#ifdef REAL_CHIP
    CKGEN_SoftReset(SRST_TIMER, DISABLE);
    CKGEN_Enable(CLK_TIMER, DISABLE);
#endif
}

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
void TIMER_DeInitChannel(TIMER_CHANNEL_Type *TIMERCHx)
{
    uint8_t instance;

    instance = TIMER_CHANNEL_INDEX(TIMERCHx);

    DEVICE_ASSERT(TIMER_CHANNEL_MAX > instance);
    s_timerCallback[instance] = NULL;
    NVIC_DisableIRQ(TIMER_CHANNEL_IRQn);
    NVIC_ClearPendingIRQ(TIMER_CHANNEL_IRQn);
    TIMER_SetChannel(TIMERCHx, DISABLE);
    TIMER_SetInterrupt(TIMERCHx, DISABLE);
    TIMER_SetPeriodLoadValue(TIMERCHx, 0);
    TIMER_SetChainMode(TIMERCHx, DISABLE);
    TIMER_ClearTimeoutFlag(TIMERCHx);
}

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
void TIMER_SetCallback(TIMER_CHANNEL_Type *TIMERCHx, const DeviceCallback_Type func)
{
    uint8_t instance;

    instance = TIMER_CHANNEL_INDEX(TIMERCHx);

    DEVICE_ASSERT(TIMER_CHANNEL_MAX > instance);
    s_timerCallback[instance] = func;
}

/*!
 * @brief TIMER interrupt request handler.
 *
 * @param[in] none
 * @return none
 */
void TIMER_Channel_IRQHandler(void)
{
    uint8_t channel;
    uint32_t flag, timTIE;
    TIMER_CHANNEL_Type *TIMERCHx;

    for (channel = 0U; channel < TIMER_CHANNEL_MAX; channel++)
    {
        TIMERCHx = s_timerChnBase[channel];
        /* store device status */
        flag = TIMER_GetTimeoutFlag(TIMERCHx);
        timTIE = TIMER_GetTimeInterruptEn(TIMERCHx);
        printf("channel %d, flag: %d, timTIE: %d, callbk: %p\r\n", channel, flag, timTIE, s_timerCallback[channel]);
        if ((0U != flag) && (0U != timTIE))
        {
            /* clear device status */
            TIMER_ClearTimeoutFlag(TIMERCHx);
            if (NULL != s_timerCallback[channel])
            {
                /* callback */
                s_timerCallback[channel](TIMERCHx, flag, 0U);
            }
        }
    }
}

/* =============================================  EOF  ============================================== */
