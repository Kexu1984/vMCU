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
 * AutoChips Inc. (C) 2016. All rights reserved.
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
#include "ac780x_timer.h"
#include "ac780x_timer_reg.h"
#ifdef REAL_CHIP
#include "ac780x_uart.h"
#include "ac780x_gpio.h"
#include "emul_menu.h"
#include "moduletest.h"
#include <debugzone.h>
#else
#include <ac7805x.h>
#include <stdlib.h>
#include <string.h>
#include "interface_layer.h"
#define APB_BUS_FREQ 1000U // 1KHz for simulation

#endif

static volatile uint32_t g_TimeoutCount[TIMER_CHANNEL_MAX] = {0U};
static uint8_t g_TimerLog[TIMER_CHANNEL_MAX] = {1U};
static TIMER_CHANNEL_Type *g_TIMERCHx[TIMER_CHANNEL_MAX] =
        {TIMER_CHANNEL0, TIMER_CHANNEL1, TIMER_CHANNEL2, TIMER_CHANNEL3};

static void Timer_Callback(void *device, uint32_t wpara, uint32_t lpara)
{
    uint8_t instance = 0;
    instance = TIMER_CHANNEL_INDEX((TIMER_CHANNEL_Type *)device);
    g_TimeoutCount[instance] ++;
    if (g_TimerLog[instance] != 0U)
    {
        DEBUGMSG(DEBUG_ZONE_INFO, ("[CB]Timer(%d)  TO_Count(%d)\r\n",
                        instance, g_TimeoutCount[instance]));
    }
}

int HSM_TestArgs2Int(char *args)
{
    char *end;
    int result = 0;
    int len = strlen(args);
    int base = 10;
    int c;

    end = args + len;
    if (args[0] == '0' && args[1] == 'x')
    {
        base = 16;
        args = &args[2];
        len -= 2;
    }

    for (; len > 0; len--, args++) {
        if (args >= end)
            return -1;

        if (base == 10)
        {
            c = (int)(*args - '0');
            if ((unsigned)c >= 10)
            {
                return -1;
            }
        }
        else
        {
            if (*args >= '0' && *args <= '9')
            {
                c = (int)(*args - '0');
            }
            else if (*args >= 'a' && *args <= 'f')
            {
                c = (int)(*args - 'a') + 10;
            }
            else if (*args >= 'A' && *args <= 'F')
            {
                c = (int)(*args - 'A') + 10;
            }
            else
            {
                return -1;
            }
        }

        result = result * base + c;
    }
    return result;
}

static void ConfigCmd(void *param, int argc, const char **argv)
{
    uint32_t i;
    uint32_t Channel = 0U;
    uint32_t Temp = 0U;
    TIMER_ConfigType timeCfg = {
        .periodValue = APB_BUS_FREQ / 1000 - 1, // 1ms
        .interruptEn = ENABLE,
        .chainModeEn  = DISABLE,
        .timerEn = DISABLE,
        .callBack = Timer_Callback,
    };

    for(i = 0U; i < (uint32_t)argc; i++)
    {
        if (0 == stricmp("-channel", argv[i]))
        {
            Channel = atoi(argv[i+1]);
            i++;
            DEVICE_ASSERT(Channel < 4);
        }
        else if (0 == stricmp("-link", argv[i]))
        {
            Temp = atoi(argv[i+1]);
            i++;
            if (1U == Temp)
            {
                timeCfg.chainModeEn = ENABLE;
            }
        }
        else if (0 == stricmp("-timeout", argv[i])) /* RTC doesn't support.*/
        {
            Temp = HSM_TestArgs2Int(argv[i+1]);
            i++;
            timeCfg.periodValue = Temp;
        }
        else
        {
            DEBUGMSG(DEBUG_ZONE_WARNING, ("Config unkonw param.\r\n"));
        }
    }

    TIMER_Init(g_TIMERCHx[Channel], &timeCfg);
    DEBUGMSG(DEBUG_ZONE_INFO, ("Timer(%d) Timeout(%d) \r\n",  Channel, timeCfg.periodValue));
}

static void StartCmd(void *param, int argc, const char **argv)
{
    uint32_t i;
    uint32_t Channel = 0U;
    uint32_t Log = 1U;

    printf("runs here:%d.\n", __LINE__);
    for(i = 0U; i < (uint32_t)argc; i++)
    {
        if (0 == stricmp("-channel", argv[i]))
        {
            Channel = atoi(argv[i+1]);
            i++;
            DEVICE_ASSERT(Channel < 4);
        }
        else if (0 == stricmp("-log", argv[i]))
        {
            Log = atoi(argv[i+1]);
            i++;
        }
        else
        {
            DEBUGMSG(DEBUG_ZONE_WARNING, ("Config unkonw param.\r\n"));
        }
    }

    g_TimerLog[Channel] = Log;
    TIMER_SetChannel(g_TIMERCHx[Channel], ENABLE);
}

static void StopCmd(void *param, int argc, const char **argv)
{
    uint32_t i;
    uint32_t Channel = 0U;

    for(i = 0U; i < (uint32_t)argc; i++)
    {
        if (0 == stricmp("-channel", argv[i]))
        {
            Channel = atoi(argv[i+1]);
            i++;
            DEVICE_ASSERT(Channel < 4);
        }
        else
        {
            DEBUGMSG(DEBUG_ZONE_WARNING, ("Config unkonw param.\r\n"));
        }
    }

    TIMER_SetChannel(g_TIMERCHx[Channel], DISABLE);
}

static void ReadValueCmd(void *param, int argc, const char **argv)
{
    uint32_t i;
    uint32_t Channel = 0U;
    uint32_t Value = 0U;
    for(i = 0U; i < (uint32_t)argc; i++)
    {
        if (0 == stricmp("-channel", argv[i]))
        {
            Channel = atoi(argv[i+1]);
            i++;
            DEVICE_ASSERT(Channel < 4);
        }
        else
        {
            DEBUGMSG(DEBUG_ZONE_WARNING, ("Config unkonw param.\r\n"));
        }
    }

    Value = TIMER_GetCurrentValue(g_TIMERCHx[Channel]);
    DEBUGMSG(DEBUG_ZONE_DEBUG, ("TO_Count(%d), CurrentValue(%d).\r\n", g_TimeoutCount[Channel], Value));
}

static void DeInitChannelCmd(void *param, int argc, const char **argv)
{
    uint32_t i;
    uint32_t Channel = 0U;
    for(i = 0U; i < (uint32_t)argc; i++)
    {
        if (0 == stricmp("-channel", argv[i]))
        {
            Channel = atoi(argv[i+1]);
            i++;
            DEVICE_ASSERT(Channel < 4);
        }
        else
        {
            DEBUGMSG(DEBUG_ZONE_WARNING, ("Config unkonw param.\r\n"));
        }
    }

   TIMER_DeInitChannel(g_TIMERCHx[Channel]);
    DEBUGMSG(DEBUG_ZONE_DEBUG, ("DeInitChannel(%d).\r\n", Channel));
}

static void DeInitCmd(void *param, int argc, const char **argv)
{
   TIMER_DeInit();
    DEBUGMSG(DEBUG_ZONE_DEBUG, ("DeInit Timer.\r\n"));
}

const EMUL_MENUITEM g_timTestMenu[] =
{
#ifdef REAL_CHIP
    {
        "a",
        "about timer test.\r\n",
        "This is a timer test module for AC780x.\r\n",
        NULL,
        NULL,
    },
    {
        "b",
        "back",
        "Back to main menu",
        GotoMenu,
        (void *)g_mainMenu,
    },
#endif
    {
        "config",
        "config timer channel.\r\n",
        "-channel,  0~3: Timer channel index.\r\n"
        "-link: link enable for timer. 0: Disabled, 1: Enabled. \r\n"
        "-Timeout, Specifiy Timer value (clock tick \r\n",
        ConfigCmd,
        0U,
    },
    {
        "Start",
        "Start timer channel.\r\n",
        "-channel,  0~3: Timer channel index. \r\n"
        "-log,  0 disable, 1 enable. \r\n",
        StartCmd,
        0U,
    },
    {
        "Stop",
        "Stop Timer or RTC \r\n",
        "-channel,  0~3: Timer channel index. \r\n",
        StopCmd,
        0U,
    },
    {
        "ReadValue",
        "Read current value of Timer \r\n",
        "-channel,  0~3: Timer channel index. \r\n",
        ReadValueCmd,
        0U,
    },
    {
        "DeInitChannel",
        "De-init Channel\r\n",
        "-channel,  0~3: Timer channel index. \r\n",
        DeInitChannelCmd,
        0U,
    },
    {
        "DeInit",
        "De-init Timer\r\n",
        "De-init Timer mod.\r\n",
        DeInitCmd,
        0U,
    },
    {
        0U,
    },
};

#ifndef REAL_CHIP
void TIMER_Channel_IRQHandler(void);
int main(int argc, const char **argv)
{
    int ret = 0;
    int i;
    const EMUL_MENUITEM *item;

    DEBUGMSG(DEBUG_ZONE_INFO, ("AC780x TIMER Driver Test Starting...\r\n"));
    DEBUGMSG(DEBUG_ZONE_INFO, ("=====================================\r\n"));

    // Initialize interface layer
    int result = interface_layer_init();
    if (result != 0) {
        printf("Interface layer initialization failed: %d\n", result);
        return -1;
    }

    // Register device with Python model
    result = register_device(6, TIMER_CTRL_BASE, 0x1000);
    if (result != 0) {
        printf("Device registration failed: %d\n", result);
        interface_layer_deinit();
        return -1;
    }

    register_interrupt_handler(TIMER_CHANNEL_IRQn, TIMER_Channel_IRQHandler);

    for (i = 0; g_timTestMenu[i].name != 0; i++)
    {
        item = &g_timTestMenu[i];
        if (strcmp(argv[1], item->name) == 0)
        {
            printf("Timer test:%s start with param:%s.\n", item->name, argv[2]);
            item->handler(item->param, argc - 2, &argv[2]);
            goto fin;
        }
    }

fin:
    printf("finished: %s\n", argv[1]);
    while(1);
    interface_layer_deinit();
    unregister_device(6);
    return 0;
}
#endif