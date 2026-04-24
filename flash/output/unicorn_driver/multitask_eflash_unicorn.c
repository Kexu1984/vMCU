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
 * AutoChips Inc. (C) 2021. All rights reserved.
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
 * @file multitask_flash.c
 *
 * @brief This file provides multitask flash/eMemory integration functions.
 *
 */

/* ===========================================  Includes  =========================================== */
#ifdef REAL_CHIP
#include "system_AC7805x.h"
#include "emul_menu.h"
#include "moduletest.h"
#endif
#include <stdlib.h>
#include "ac780x_eflash.h"
//#include "ac780x_optionbyte.h"

/* ============================================  Define  ============================================ */
#define FLASH_TEST_START_ADDRESS                    (0x08040000UL)     /* Test flash start address */
#define FLASH_TEST_END_ADDRESS                      (0x08080000UL)    /* Test flash end address */

#define TEST_BUFFER_SIZE                            (256UL)         /* Test buffer size */

#define FLASH_INFO_START_ADDRESS                    (0x08200000UL)
#define SRAM_START_ADDRESS                          (0x20000000UL)
#define SRAM_END_ADDRESS                            (0x20010000UL)
#define REGISTER_START_ADDRESS                      (0x40000000UL)

#define FLASH_OT_USE_MASS_ERASE                     (0U)
/* ==========================================  Variables  =========================================== */
__align(4) uint8_t g_flashBuffer[32] = {0};
uint32_t g_flashAddr = 0U;
uint32_t g_flashSize = 0U;
uint8_t g_flashLogPrintFormat = 1U;              /* 0: print with 8-bit; 1: print with 32-bit*/
uint32_t g_otTimes = 0U;                         /* number of tests */
uint32_t g_flashLogFreq = 1U;                    /* OT print frequency */
uint32_t looptCount = 1U;                        /* OT times counter */
uint8_t g_onlyDFlash = 0U;

enum
{
    EFLASH_TASK = 0U,
    EFLASH_READ_TASK = 1U,
    EFLASH_TASK_NUM
};

#ifdef REAL_CHIP
static TASK_INFO EFLASH_TEST_TASK[] =
{
    {TASK_FUNC, TASK_NORMAL, M_EFLASH, EFLASH_TASK, 0, (void*)0, 0, NULL},
    {TASK_FUNC, TASK_NORMAL, M_EFLASH, EFLASH_READ_TASK, 0U, (void*)0U, 0U, NULL},
};
#endif

/* ======================================  Functions define  ======================================== */
/*!
 * @brief get str's hex value
 *
 * @param[in] inStr: input string pointer
 * @param[in] hexValue: Hex value of the input str
 * @return uint32_t type pointer
 */
static uint32_t *Str2Hex(const char *inStr, uint32_t *hexValue)
{
    uint32_t i = 0U;
    uint32_t tmpStrLength = strlen(inStr);

    *hexValue = 0U;

    for (i = 0U; i < tmpStrLength; i++)
    {
        if (inStr[i] >= '0' && inStr[i] <= '9')
        {
            *hexValue = (*hexValue) * 16U + inStr[i] - '0';
        }
        else if (inStr[i] <= 'f' && inStr[i] >= 'a')
        {
            *hexValue = (*hexValue) * 16U + inStr[i] - 'a' + 10U;
        }
        else if (inStr[i] <= 'F' && inStr[i] >= 'A')
        {
            *hexValue = (*hexValue) * 16U + inStr[i] - 'A' + 10U;
        }
        else
        {
        }
    }

    return hexValue;
}

/*!
 * @brief Clear FLASH task status
 *
 * @param[in] none
 * @return none
 */
static void FlashClearTaskStatus(void)
{
#ifdef REAL_CHIP
    EFLASH_TEST_TASK[0].TaskFunc = 0U;
#endif
    g_flashAddr = 0U;
    g_flashSize = 0U;
}

/*!
 * @brief Start FLASH read task
 *
 * @param[in] arg: not used
 * @param[in] flag: not used
 * @return 0: success -1: error
 */
static uint8_t StartReadTask(void *arg, uint32_t flag)
{
    uint32_t i = 0U;

    for (i = 0U; i < g_flashSize;)
    {
        if (1U == g_flashLogPrintFormat)
        {
            DEBUGMSG(DEBUG_ZONE_INFO, ("0x%08x  ", *(uint32_t *)(g_flashAddr + i)));
            i += 4U;
        }
        else
        {
            DEBUGMSG(DEBUG_ZONE_INFO, ("0x%02x  ", *(uint8_t *)(g_flashAddr + i)));
            i += 1U;
        }
        /* ervery line display 4 * 32bit data when select 32bit read way
           ervery line display 8 byte data when select 8bit read way
        */
        if (!(i % (16U / (2U - g_flashLogPrintFormat))))
        {
            DEBUGMSG(DEBUG_ZONE_ERROR, ("\r\n"));
            //Sleep(1U);
        }
    }
    DEBUGMSG(DEBUG_ZONE_INFO, ("\r\n[FLASH]Read task TEST_PASS\r\n"));
#ifdef REAL_CHIP
    EFLASH_TEST_TASK[1].TaskFunc = 0U;
#endif
    DEBUGMSG(DEBUG_ZONE_INFO, ("[FLASH]:Exit StartReadTask\r\n"));

    return 0U;
}

/*!
 * @brief Start FLASH erase task
 *
 * @param[in] arg: not used
 * @param[in] flag: not used
 * @return 0: success -1: error
 */
static uint8_t StartEraseTask(void *arg, uint32_t flag)
{
    uint32_t i = 0U;
    status_t ret = STATUS_SUCCESS;

    (void)EFLASH_UnlockCtrl();
    for (i = 0U; i < g_flashSize; i++)
    {
        ret = EFLASH_PageErase(g_flashAddr + i * DFLASH_PAGE_SIZE);
        if (STATUS_SUCCESS != ret)
        {
            break;
        }
    }
    if (i < g_flashSize)
    {
        DEBUGMSG(DEBUG_ZONE_ERROR, ("[FLASH][Err]Erase task TEST_FAIL in address: \
                                    0x%x, state: 0x%x\r\n", (uint32_t)(g_flashAddr + i * DFLASH_PAGE_SIZE), EFLASH->CSR));
    }
    else
    {
        DEBUGMSG(DEBUG_ZONE_ERROR, ("[FLASH]Erase task TEST_PASS\r\n"));
    }
    EFLASH_LockCtrl();
    FlashClearTaskStatus();
    DEBUGMSG(DEBUG_ZONE_INFO, ("[FLASH]:Exit StartEraseTask\r\n"));

    return 0U;
}

static void dump_bytes(uint8_t *buf, uint32_t len)
{
    uint32_t i = 0U;

    for (i = 0U; i < len; i++)
    {
        DEBUGMSG(DEBUG_ZONE_INFO, ("0x%02x  ", buf[i]));
        if (!((i + 1U) % 16U))
        {
            DEBUGMSG(DEBUG_ZONE_INFO, ("\r\n"));
        }
    }
    if (len % 16U)
    {
        DEBUGMSG(DEBUG_ZONE_INFO, ("\r\n"));
    }
}

/*!
 * @brief Start FLASH program task
 *
 * @param[in] arg: not used
 * @param[in] flag: not used
 * @return 0: success -1: error
 */
static uint8_t StartProgramTask(void *arg, uint32_t flag)
{
    uint16_t i = 0U, j = 0U;
    status_t ret = STATUS_SUCCESS;

    (void)EFLASH_UnlockCtrl();
    while (g_flashSize >= TEST_BUFFER_SIZE)
    {
        for (j = 0U; j < TEST_BUFFER_SIZE; j += 32U)
        {
            for (i = j; i < j + 32U; i++)
            {
                g_flashBuffer[i % 32U] = i;
            }
            printf("Program at 0x%x\r\n", g_flashAddr);
            //dump_bytes(g_flashBuffer, 32U);
            ret = EFLASH_PageProgram(g_flashAddr, (uint32_t *)g_flashBuffer, 8);
            if (STATUS_SUCCESS != ret)
            {
                break;
            }
            g_flashAddr += 32U;
        }
        g_flashSize -= TEST_BUFFER_SIZE;
    }

    if (g_flashSize <= TEST_BUFFER_SIZE)
    {
        for (j = 0U; j < g_flashSize; j += 32U)
        {
            for (i = j; i < j + 32U; i++)
            {
                g_flashBuffer[i % 32U] = i;
            }
            printf("Program remain at 0x%x\r\n", g_flashAddr);
            //dump_bytes(g_flashBuffer, 32U);
            ret = EFLASH_PageProgram(g_flashAddr, (uint32_t *)g_flashBuffer, 8);
            if (STATUS_SUCCESS != ret)
            {
                break;
            }
            g_flashAddr += 32U;
        }
    }

    if (STATUS_SUCCESS != ret)
    {
        DEBUGMSG(DEBUG_ZONE_ERROR, ("[FLASH][Err]Program task TEST_FAIL in address: 0x%x, state: 0x%x, ret=%d\r\n",
             g_flashAddr, EFLASH->CSR, ret));
    }
    else
    {
        DEBUGMSG(DEBUG_ZONE_ERROR, ("[FLASH]Program task TEST_PASS\r\n"));
    }
    EFLASH_LockCtrl();

    FlashClearTaskStatus();
    DEBUGMSG(DEBUG_ZONE_INFO, ("[FLASH]:Exit StartProgramTask\r\n"));

    return 0U;
}

/*!
 * @brief Start FLASH OT test task
 *
 * @param[in] arg: not used
 * @param[in] flag: not used
 * @return 0: success -1: error
 */
static uint8_t StartFOTTask(void *arg, uint32_t flag)
{
    uint32_t pageAddr = FLASH_TEST_START_ADDRESS;
    uint32_t testRange[2][2] = {{PFLASH_BASE_ADDRESS + 0x30000U, PFLASH_END_ADDRESS}, {DFLASH_BASE_ADDRESS, DFLASH_END_ADDRESS}};
    uint32_t i = 0U, j = 0U;
    status_t ret = STATUS_SUCCESS;
    uint32_t dataSrc[2] = {0};
    uint32_t readData1[2] = {0};
    uint32_t startIndex = g_onlyDFlash;

    (void)EFLASH_UnlockCtrl();

    while (looptCount < (g_otTimes + 1U))
    {
        /* P&D-Flash OT test */
        for (j = startIndex; j < 2U; j++)
        {
            for (pageAddr = testRange[j][0]; pageAddr < testRange[j][1];)
            {
                ret = EFLASH_PageErase(pageAddr);
                if (ret != STATUS_SUCCESS)
                {
                    DEBUGMSG(DEBUG_ZONE_ERROR, ("[FLASH][Err]:FLASH OT test Erase sector TEST_FAIL pageAddr:"
                                                "0x%x, stat: 0x%x\r\n", pageAddr, EFLASH->CSR));
                    FlashClearTaskStatus();
                    return 1U;
                }
                else
                {
                    /*Do nothing*/
                }

                for (i = pageAddr; i < (pageAddr + DFLASH_PAGE_SIZE); i += 4U)
                {
                    (void)EFLASH_Read(i, (uint32_t *)readData1, 1U);
                    if (readData1[0] != 0xFFFFFFFFU)
                    {
                        DEBUGMSG(DEBUG_ZONE_ERROR, ("[FLASH][Err]FLASH OT TEST_FAIL to Compare with 0xff for test Erase!\
                                                    data[0x%x]:0x%x\r\n", i, *(uint32_t *)i));

                        FlashClearTaskStatus();
                        return 1U;
                    }
                }

                for (i = 0U; i < DFLASH_PAGE_SIZE; i += 8U)
                {
                    dataSrc[0] = (uint32_t)rand();
                    dataSrc[1] = (uint32_t)rand();
                    ret = EFLASH_PageProgram(pageAddr + i, dataSrc, 2U);
                    (void)EFLASH_Read(pageAddr + i , readData1, 2U);
                    if ((STATUS_SUCCESS != ret) || (dataSrc[0] != readData1[0]) || (dataSrc[1] != readData1[1]))
                    {
                        printf("Flash fail[%d] expect:0x%x, 0x%x, read:0x%x, 0x%x\r\n", i, dataSrc[0], dataSrc[1], readData1[0], readData1[1]);
                        break;
                    }
                }
                if (i < DFLASH_PAGE_SIZE)
                {
                    DEBUGMSG(DEBUG_ZONE_ERROR, ("[FLASH][Err]FLASH OT TEST_FAIL times:%d\r\n", looptCount));
                    FlashClearTaskStatus();
                    return 1U;
                }
                pageAddr += DFLASH_PAGE_SIZE;
                //Sleep(1U);
            }
        }
        if (!(looptCount % g_flashLogFreq))
        {
            DEBUGMSG(DEBUG_ZONE_INFO, ("\r\n[FLASH]FLASH OT OK times:%d\r\n", looptCount));
        }
        looptCount++;
    };
    EFLASH_LockCtrl();
    FlashClearTaskStatus();
    looptCount = 0U;
    DEBUGMSG(DEBUG_ZONE_INFO, ("[FLASH]Exit StartFOTTask, TEST_PASS\r\n"));

    return 0U;
}

/**
 * @brief FLASH read test
 *
 * @param[in] param: parameter
 * @param[in] argc: argument count
 * @param[in] argv: argument value
 * @return none
*/
void FLASH_ReadTest(void *param, int argc, const char** argv)
{
    g_flashSize = 64U;

    (void)Str2Hex(argv[0], &g_flashAddr);

    if ((uint32_t)atoi(argv[1]) > 0U)
    {
        g_flashSize = atoi(argv[1]);
    }
    /* address range manage ,refer memory map table from CD for the bounadry address which can be accessed */
    if (   ((g_flashAddr >= OPTION_BASE)  &&  (g_flashAddr < OPTION_END)) \
            || ((g_flashAddr >= PFLASH_BASE_ADDRESS)     &&  (g_flashAddr < DFLASH_END_ADDRESS)) \
            || ((g_flashAddr >= SRAM_START_ADDRESS)      &&  (g_flashAddr < SRAM_END_ADDRESS)) \
            || ((g_flashAddr >= REGISTER_START_ADDRESS)  &&  (g_flashAddr < 0x60000000U))
       )
    {
        DEBUGMSG(DEBUG_ZONE_ERROR, ("[FLASH]address:0x%x, length:0x%x\r\n", g_flashAddr, g_flashSize));
    }
    else
    {
        DEBUGMSG(DEBUG_ZONE_ERROR, ("[FLASH][Err]Cli address parameters error,TEST_FAIL\r\n\r\n"));
        return;
    }

#ifdef REAL_CHIP
    if (EFLASH_TEST_TASK[1].TaskFunc)
    {
        DEBUGMSG(DEBUG_ZONE_ERROR, ("[FLASH][Err]Task is running\r\n"));
        return;
    }
    EFLASH_TEST_TASK[1].type = TASK_FUNC;
    EFLASH_TEST_TASK[1].TaskFunc = StartReadTask;
    (void)AddTask(&EFLASH_TEST_TASK[1], 1U);
#else
    StartReadTask(NULL, 0U);
#endif
}

/**
 * @brief FLASH erase test
 *
 * @param[in] param: parameter
 * @param[in] argc: argument count
 * @param[in] argv: argument value
 * @return none
 */
void FLASH_EraseTask(void *param, int argc, const char** argv)
{
    (void)Str2Hex(argv[0], &g_flashAddr);
    g_flashSize = atoi(argv[1]);/* erase page amount */

    if (g_flashAddr % 8)
    {
        DEBUGMSG(DEBUG_ZONE_ERROR, ("[FLASH][Err]CLI parameter not 4-align,TEST_FAIL\r\n"));
        return;
    }

#ifdef REAL_CHIP
    if (EFLASH_TEST_TASK[0].TaskFunc)
    {
        DEBUGMSG(DEBUG_ZONE_ERROR, ("[FLASH][Err]Task is running,TEST_FAIL\r\n"));
        return;
    }
    EFLASH_TEST_TASK[0].type = TASK_FUNC;
    EFLASH_TEST_TASK[0].TaskFunc = StartEraseTask;
    (void)RemoveTask(&EFLASH_TEST_TASK[1], 1U);
    (void)AddTask(&EFLASH_TEST_TASK[0], 1U);
#else
    StartEraseTask(NULL, 0U);
#endif
}

/**
 * @brief FLASH program test
 *
 * @param[in] param: parameter
 * @param[in] argc: argument count
 * @param[in] argv: argument value
 * @return none
 */
void FLASH_ProgramTest(void *param, int argc, const char** argv)
{
    (void)Str2Hex(argv[0], &g_flashAddr);
    g_flashSize = atoi(argv[1]); /* program byte amount */

    if (g_flashSize % 8)
    {
        DEBUGMSG(DEBUG_ZONE_ERROR, ("[FLASH][Err]CLI parameter not 8-align,TEST_FAIL\r\n"));
        return;
    }

#ifdef REAL_CHIP
    if (EFLASH_TEST_TASK[0].TaskFunc)
    {
        DEBUGMSG(DEBUG_ZONE_ERROR, ("[FLASH][Err]Task is running,TEST_FAIL\r\n"));
        return;
    }

    EFLASH_TEST_TASK[0].type = TASK_FUNC;
    EFLASH_TEST_TASK[0].TaskFunc = StartProgramTask;
    (void)AddTask(&EFLASH_TEST_TASK[0], 1U);
#else
    StartProgramTask(NULL, 0U);
#endif
}

/**
 * @brief FLASH set write protect test
 *
 * @param[in] param: parameter
 * @param[in] argc: argument count
 * @param[in] argv: argument value
 * @return none
 */
void FLASH_WriteProtectTest(void *param, int argc, const char** argv)
{
    uint32_t protectValue[2] = {0};
    uint32_t index = 0;
    status_t res = STATUS_SUCCESS;
    uint8_t wrtProtMode;

    index = strtol(argv[0], 0, 0);
    Str2Hex(argv[1], &protectValue[0]);
    Str2Hex(argv[2], &protectValue[1]);
    wrtProtMode = atoi(argv[3]);

    printf("[EFLASH]:TestWriteProtect:index:0x%x,protectValue[0]:0x%x,protectValue[1]:0x%x\r\n", index, protectValue[0], protectValue[1]);

    EFLASH_UnlockCtrl();

    if(!wrtProtMode)
    {
        if (index == 0)
        {
            EFLASH_PFSetWriteProtection(protectValue);
        }
        else
        {
            EFLASH_DFSetWriteProtection(protectValue);
        }
    }
    else
    {
        EFLASH_SetWriteProtectionByReg(index,protectValue[0], protectValue[1]);
    }

    if (res != STATUS_SUCCESS)
    {
        printf("\r\n [ER][EFLASH]: EFLASH_SetWriteProtect Fail!\r\n");
    }
    EFLASH_LockCtrl();
}

/**
 * @brief FLASH OT test
 *
 * @param[in] param: parameter
 * @param[in] argc: argument count
 * @param[in] argv: argument value
 * @return none
 */
void FLASH_OTTest(void *param, int argc, const char** argv)
{
    g_otTimes = atoi(argv[0]);

    if (argc > 1 && argv)
    {
        g_onlyDFlash = atoi(argv[1]);
    }
    else
    {
        g_onlyDFlash = 0;
    }

#ifdef REAL_CHIP
    if (EFLASH_TEST_TASK[0].TaskFunc)
    {
        DEBUGMSG(DEBUG_ZONE_ERROR, ("[FLASH][Err]Task is running,TEST_FAIL\r\n"));
        return;
    }
    EFLASH_TEST_TASK[0].type = TASK_FUNC;
    EFLASH_TEST_TASK[0].TaskFunc = StartFOTTask;
    (void)AddTask(&EFLASH_TEST_TASK[0], 1U);
#else
    StartFOTTask(NULL, 0U);
#endif
}
/**
 * @brief FLASH stop task
 *
 * @param[in] param: parameter
 * @param[in] argc: argument count
 * @param[in] argv: argument value
 * @return none
 */
void FLASH_Stop(void *param, int argc, const char** argv)
{
#ifdef REAL_CHIP
    int i = 0U;
    for (i = 0U; i < EFLASH_TASK_NUM; i++)
    {
        if (EFLASH_TEST_TASK[i].TaskFunc)
        {
            (void)RemoveTask(&EFLASH_TEST_TASK[i], 1U);
            EFLASH_TEST_TASK[i].TaskFunc = 0U;
            DEBUGMSG(DEBUG_ZONE_ERROR, ("[FLASH]StopTask\r\n"));
        }
    }
#endif
}

/**
 * @brief FLASH log setting test
 *
 * @param[in] param: parameter
 * @param[in] argc: argument count
 * @param[in] argv: argument value
 * @return none
 */
void FLASH_LogTest(void *param, int argc, const char** argv)
{
    if ((uint32_t)atoi(argv[1]) > 1U)
    {
        DEBUGMSG(DEBUG_ZONE_ERROR, ("[FLASH][Err]FLASH_LogTest CLI parameter error,TEST_FAIL\r\n"));
        return;
    }
    g_flashLogFreq = atoi(argv[0]);
    g_flashLogPrintFormat = atoi(argv[1]);

    DEBUGMSG(DEBUG_ZONE_INFO, ("[FLASH]FLASH_LogTest setting TEST_PASS\r\n"));
}

/**
 * @brief FLASH exit test
 *
 * @param[in] param: parameter
 * @param[in] argc: argument count
 * @param[in] argv: argument value
 * @return none
 */
void FLASH_ExitTest(void *param, int argc, const char** argv)
{
    looptCount = 0;
    DEBUGMSG(DEBUG_ZONE_INFO, ("[FLASH]FLASH_ExitTest, TEST_PASS\r\n"));
}

const EMUL_MENUITEM g_eflashTestMenu[] =
{
#ifdef REAL_CHIP
    {
        "b",
        "back",
        "Back to Sub Test Menu",
        GotoMenu,
        (void *)g_mainMenu,
    },
#endif
    {
        "frd",
        "flash read test(default read 64 bytes)",
        "frd addr len",
        FLASH_ReadTest,
        0,
    },

    {
        "fer",
        "flash erase test",
        "fer addr num",
        FLASH_EraseTask,
        0,
    },

    {
        "fwrt",
        "flash write test",
        "fwrt addr size",
        FLASH_ProgramTest,
        0,
    },

    {
        "fwp",
        "flash set write protection",
        "fwp flashProtValue",
        FLASH_WriteProtectTest,
        0,
    },

    {
        "fot",
        "flash ot test;",
        "fot times onlyDFlash",
        FLASH_OTTest,
        0,
    },

    {
        "log",
        "log setting;",
        "log times 0/1(8bit/32bit)",
        FLASH_LogTest,
        0,
    },
    {
        "exit",
        "FLASH exit",
        "none",
        FLASH_Stop,
        0U,
    },

    {
        0,
    },
};

#ifndef REAL_CHIP

#include <system_ac780x.h>

struct test_env g_testEnv[] = {
    {
        .argc = 4,
        .test_func = FLASH_ProgramTest,
        .argv = {"eflash_test", "fwrt", "0x8100000", "32"}
    },
    {
        .argc = 4,
        .test_func = FLASH_ReadTest,
        .argv = {"eflash_test", "frd", "0x8100000", "32"}
    },
    {
        .argc = 4,
        .test_func = FLASH_EraseTask,
        .argv = {"eflash_test", "fer", "0x8100000", "1"}
    },
    {
        .argc = 5,
        .test_func = FLASH_WriteProtectTest,
        .argv = {"eflash_test", "fwp", "0x1", "0x1","1"}
    },
#ifdef FLASH_OT_TEST
    {
        .argc = 3,
        .test_func = FLASH_OTTest,
        .argv = {"eflash_test", "fot", "10"}
    },
#endif
};

int main(void)
{
    unsigned int i, j;
    const EMUL_MENUITEM *item;
    struct test_env *p_env;

    printf("=====================================\r\n");
    DEBUGMSG(DEBUG_ZONE_INFO, ("AC780x Eflash Driver Test Starting...\r\n"));
    DEBUGMSG(DEBUG_ZONE_INFO, ("=====================================\r\n"));

    for (j = 0; j < TEST_PARAMS; j++)
    {
        p_env = &g_testEnv[j];
        for (i = 0; g_eflashTestMenu[i].name != 0; i++)
        {
            item = &g_eflashTestMenu[i];
            if (strcmp(p_env->argv[1], item->name) == 0)
            {
                printf("Eflash test:%s start with param:%s.\n", item->name, p_env->argv[2]);
                item->handler(item->param, p_env->argc - 2, &p_env->argv[2]);
                printf("finished: %s\n", p_env->argv[1]);
                break;
            }
        }
    }

    UNICORN_STOP();
    return 0;
}
#endif