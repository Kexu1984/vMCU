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
 * AutoChips Inc. (C) 2020. All rights reserved.
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

/* ===========================================  Includes  =========================================== */
//#include "ac780x.h"
#ifdef REAL_CHIP
#include "moduletest.h"
#include "emul_menu.h"
#include "ac780x_dma.h"
#include "ucos_ii.h"
#include "ac780x_eflash.h"
#else
#include "ac780x_hsm.h"
#include "ac7805x.h"
#include "interface_layer.h"
#endif

/* ============================================  Define  ============================================ */
/**
  * @brief If 'HSM_TEST_ENABLE' set to 1, than each command will dump whole params
  */
#define HSM_TEST_ENABLE             1

/**
  * @brief Note: The total interval timeout for the complete auth flow(getchlg + cmac + auth) is nearly 8seconds,
  *              so auth timeout maybe happend if cmac ops did in Host side.
  *        If 'HSM_SOFT_AUTH_ENABLE' set to 1, we will calculate the auth-mac value by hsm itself after 'getchlg'
  *        cmd was called to verify the whole auth flow.
  */
#define HSM_SOFT_AUTH_ENABLE        1

/**
  * @brief Max bytes length support when input from command.
  */
#define HSM_CRYPTO_BUFFER_MAX_LEN   0x100
/**
  * @brief Describe the max input length for aes-cmac/binary-verification flow in each 'update 'process.
  *        If the message len is larger than HSM_CMAC_MAX_LEN_ONETIME, the test flow will splite into
  *        more than one 'update' flow.
  */
#define HSM_CMAC_MAX_LEN_ONETIME    0x100

/**
  * @brief Global selftest control definitions
  */
#define HSM_SELFTEST_SUPPORT        0
#if HSM_SELFTEST_SUPPORT
/**
  * @brief CMAC Unalignment testcase definitions
  */
#define HSM_CMAC_UNAIGN_SELFTEST    1

/**
  * @brief Multiple flash key add/del testcase definitions
  */
#define HSM_FLASHKEY_SELFTEST       1
#endif
/* ===========================================  Typedef  ============================================ */
/**
  * @brief HSM Test Module Definitions
  */
enum HSM_TEST_MODULE {
    HSM_TEST_SET_KEY,
    HSM_TEST_UID,
    HSM_TEST_IRQ,
    HSM_TEST_AES,
    HSM_TEST_CMAC,
    HSM_TEST_RBG,
    HSM_TEST_STAT,
    HSM_TEST_CHLG,
    HSM_TEST_AUTH,
    HSM_TEST_BINARY_VERIFY,
    HSM_TEST_WRITEDFLASH,
    HSM_TEST_READMEM,
    HSM_TEST_WRITEMEM,
    HSM_TEST_ERASEFLASH,
    HSM_TEST_SECUREBOOT,
    HSM_TEST_RMALLKEY,
};
/* ==========================================  Variables  =========================================== */
static const char* g_hsmCmdName[] = {
    "ECB", "CBC", "CMAC", "Auth", "Random"
};
/* Global Crypto Output Buffer */
static uint8_t g_hsmCryptoOutBuffer[HSM_CRYPTO_BUFFER_MAX_LEN] = {0};

/* Global Crypto Output Length */
static uint32_t g_hsmCryptoOutLen;
/* Glabal CMAC Sign-mode/Verify-mode */
static uint32_t g_hsmCryptoCmacMode;
/* Glabal DMA sync status: Target Number */
static volatile uint16_t g_hsmCryptoDMASyncTotal;
/* Glabal DMA sync status: Request send nums */
static volatile uint16_t g_hsmCryptoDMASyncSend;
/* Glabal DMA sync status: Finished nums */
static volatile uint16_t g_hsmCryptoDMASyncFini;
/* Mark if getchlg called at first */
static BOOL_Type g_hsmGetChlgInit;
static volatile uint32_t g_startTime;
static volatile uint32_t g_endTime;
/* ====================================  Functions declaration  ===================================== */
extern const EMUL_MENUITEM g_hsmTestMenu[];
/* ===================================  Static Functions define  ==================================== */
static uint32_t ApiCostTime()
{
    uint32_t costTime = 0;
    if(g_startTime < g_endTime)
        costTime = g_endTime - g_startTime;
    return costTime;
}

static void HSM_TestDumpBuf(uint32_t *buf, uint32_t len, const char *str)
{
    uint32_t i;
    unsigned char *pBuf = (unsigned char *)buf;

    Debug_Printf("[INFO][HSM] %s\r\n", str);
    for (i = 0; i < len; i++) {
        Debug_Printf("%02x", pBuf[i]);
        Sleep(2);
    }
    Debug_Printf("\r\n");
}

static char * HSM_TestParseArgs(int argc, const char** argv, char *symbol)
{
    int i;

    for (i = 0; i < argc; i++)
    {
        if(0 == stricmp(symbol, argv[i]))
        {
            return (char *)argv[i + 1];
        }
    }

    return NULL;
}

static int HSM_TestArgs2Int(char *args)
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

static char hex2int(char c)
{
    if (c >= '0' && c <= '9')
        return (char)(c - '0');
    else if (c >= 'a' && c <= 'f')
        return (char)(10 + (c - 'a'));
    else if (c >= 'A' && c <= 'F')
        return (char)(10 + (c - 'A'));
    else
        return (char)(0xFF);
}

static void HSM_TestArgs2HexBuf(char *str, uint8_t *hex)
{
    uint32_t i;
    uint32_t hex_size = (uint32_t)strlen(str) >> 1;

    for (i = 0; i < hex_size; ++i) {
        hex[i] = (uint8_t)(hex2int(str[(i << 1)]) << 4) | (uint8_t)hex2int(str[((i << 1) | 1)]);
    }
}

static void HSM_DumpInputArgs(int argc, const char** argv)
{
#if HSM_TEST_ENABLE
    int i;

    for (i = 0; i < argc; i = i + 2)
    {
        Debug_Printf("[INFO][HSM] Arg-[%d]: %s\r\n", i, argv[i]);
        Sleep(1);
        Debug_Printf("[INFO][HSM] >>>>Value:%s\r\n", argv[i + 1]);
        Sleep(10);
    }
#else
    return;
#endif
}

/**
* Str2Hex
*
* @param[in] inStr: input string pointer
* @param[in] hexValue: Hex value of the input str
* @return    uint32_t type pointer
*
* @brief  get str's hex value
*/
static uint32_t *Str2Hex(const char *inStr, uint32_t *hexValue)
{
    uint32_t i = 0, tmpStrLength = 0;

    tmpStrLength = strlen(inStr);
    *hexValue = 0;

    for (i = 0; i < tmpStrLength; i++)
    {
        if (inStr[i] >= '0' && inStr[i] <= '9')
        {
            *hexValue = (*hexValue) * 16 + inStr[i] - '0';
        }
        else if (inStr[i] <= 'f' && inStr[i] >= 'a')
        {
            *hexValue = (*hexValue) * 16 + inStr[i] - 'a' + 10;
        }
        else if (inStr[i] <= 'F' && inStr[i] >= 'A')
        {
            *hexValue = (*hexValue) * 16 + inStr[i] - 'A' + 10;
        }
        else
        {
        }
    }

    return hexValue;
}

static void  HSM_Data2flash(void *param, int argc, const char** argv)
{
    status_t ret = STATUS_SUCCESS;
    int testId = (int)param;

    char *addrString;
    char *msgString;
    char *lenString;

    uint32_t addrInt;
    uint32_t lenInt;
    uint8_t msgBuf[64];

    HSM_DumpInputArgs(argc, argv);
    if (testId != HSM_TEST_WRITEDFLASH)
    {
        Debug_Printf("[ER][HSM] Error Test Id:%d\r\n", testId);
        return;
    }

    addrString = HSM_TestParseArgs(argc, argv, "-addr");
    msgString = HSM_TestParseArgs(argc, argv, "-msg");
    lenString = HSM_TestParseArgs(argc, argv, "-msglen");

    if (!msgString || !addrString || !lenString)
    {
        Debug_Printf("[ER][HSM] datain Test: '-addr' 'msg' '-msglen' must be set.\r\n");
        return;
    }

    (void)Str2Hex(addrString, &addrInt);
    lenInt = HSM_TestArgs2Int(lenString);
    if(lenInt > 48U)
    {
        Debug_Printf("[ER][HSM] datain Test: '-msglen' must be <= 48.\r\n");
        return;
    }

    HSM_TestArgs2HexBuf(msgString, (uint8_t *)msgBuf);

    (void)EFLASH_UnlockCtrl();

    ret = EFLASH_PageProgram(addrInt, (uint32_t *)msgBuf, lenInt);
    if (STATUS_SUCCESS != ret)
    {
        Debug_Printf("[FLASH][Err]Program flash TEST_FAIL in address: \
                                    0x%x, state: 0x%x\r\n", addrInt, EFLASH->CSR);
    }
    else
    {
        Debug_Printf("[FLASH]Program flash TEST_PASS\r\n");
    }
    EFLASH_LockCtrl();

    return;
}

/*!
 * @brief Start FLASH erase
 *
 * @param[in] arg: not used
 * @param[in] flag: not used
 * @return 0: success -1: error
 */
static void HSM_EraseDflash(void *param, int argc, const char** argv)
{
    uint32_t i = 0U;
    int testId = (int)param;

    char *addrString;
    char *lenString;
    uint32_t addrInt;
    uint32_t lenInt;

    status_t ret = STATUS_SUCCESS;
    if (testId != HSM_TEST_ERASEFLASH)
    {
        Debug_Printf("[ER][HSM] Error Test Id:%d\r\n", testId);
        return;
    }
    addrString = HSM_TestParseArgs(argc, argv, "-addr");
    lenString = HSM_TestParseArgs(argc, argv, "-len");
    if(!addrString && !lenString)
    {
        Debug_Printf("[ER][HSM] erase dflash Test: '-addr' 'len' must be set.\r\n");
        return;
    }

    Str2Hex((const char *)addrString, &addrInt);
    lenInt = HSM_TestArgs2Int(lenString);
    (void)EFLASH_UnlockCtrl();
    for (i = 0U; i < lenInt; i++)
    {
        ret = EFLASH_PageErase(addrInt + i * DFLASH_PAGE_SIZE);
        if (STATUS_SUCCESS != ret)
        {
            break;
        }
    }
    if (i < lenInt)
    {
        Debug_Printf("[FLASH][Err]Erase TEST_FAIL in address: \
                                    0x%x, state: 0x%x\r\n", (uint32_t)(addrInt + i * DFLASH_PAGE_SIZE), EFLASH->CSR);
    }
    else
    {
        Debug_Printf("[FLASH]Erase TEST_PASS\r\n");
    }
    EFLASH_LockCtrl();

    return;
}


/*!
 * @brief  read mem.
 *
 * @param[in] param: param
 * @param[in] argc: param counter
 * @param[in] argv: param vector
 * @return none
 */
static void HSM_ReadMem(void *param, int argc, const char **argv)
{
    uint32_t value = 0U;
    int testId = (int)param;
    if (testId != HSM_TEST_READMEM)
    {
        Debug_Printf("[ER][HSM] Error Test Id:%d\r\n", testId);
        return;
    }
    char *addrString;
    char *lenString;
    char *posString;

    uint32_t addrInt;
    uint32_t lenInt;
    uint32_t posInt;

    addrString = HSM_TestParseArgs(argc, argv, "-addr");
    lenString = HSM_TestParseArgs(argc, argv, "-len");
    posString = HSM_TestParseArgs(argc, argv, "-pos");

    if (!addrString)
    {
        Debug_Printf("[ER][HSM] ReadMem: '-addr' must be set.\r\n");
        return;
    }

    Str2Hex((const char *)addrString, &addrInt);

    if(lenString && posString)
    {
        lenInt = HSM_TestArgs2Int(lenString);
        posInt = HSM_TestArgs2Int(posString);
        value = (((*((volatile uint32_t *)addrInt)) >> (posInt)) & (~(0xFFFFFFFFUL << (lenInt))));
        Debug_Printf("read addr(0x%x) pos(%d) len(%d): 0x%x\r\n",
            addrInt, posInt, lenInt, value);
    }
    else if(!lenString && !posString)
    {
        value = *((volatile uint32_t *)addrInt);
        Debug_Printf("read addr(0x%x): 0x%x\r\n", addrInt, value);
    }
    else
    {
        lenInt = HSM_TestArgs2Int(lenString);
        for(uint8_t i=0; i < lenInt; i++)
        {
            value = *((volatile uint8_t *)(addrInt + i));
            Debug_Printf("read addr(0x%x): 0x%x\r\n", addrInt + i, value);
        }
    }
}

/*!
 * @brief  write mem.
 *
 * @param[in] param: param
 * @param[in] argc: param counter
 * @param[in] argv: param vector
 * @return none
 */
static void HSM_WriteMem(void *param, int argc, const char **argv)
{
    int testId = (int)param;
    if (testId != HSM_TEST_WRITEMEM)
    {
        Debug_Printf("[ER][HSM] Error Test Id:%d\r\n", testId);
        return;
    }

    char *addrString;
    char *valString;
    char *posString;
    char *lenString;
    char *maskString;
    char *msgString;
    char *msgLenString;

    uint32_t addrInt;
    uint32_t valInt;
    uint32_t posInt;
    uint32_t lenInt;
    uint32_t maskInt;
    uint32_t msgLenInt;
    uint8_t msgBuf[64];

    addrString = HSM_TestParseArgs(argc, argv, "-addr");
    valString = HSM_TestParseArgs(argc, argv, "-val");
    posString = HSM_TestParseArgs(argc, argv, "-pos");
    lenString = HSM_TestParseArgs(argc, argv, "-len");
    maskString = HSM_TestParseArgs(argc, argv, "-mask");
    msgString = HSM_TestParseArgs(argc, argv, "-msg");
    msgLenString = HSM_TestParseArgs(argc, argv, "-msglen");

    if(!addrString)
    {
        Debug_Printf("[ER][HSM] WriteMem: '-addr' must be set.\r\n");
        return;
    }

    Str2Hex((const char *)addrString, &addrInt);

    if(!valString && !msgString)
    {
        Debug_Printf("[ER][HSM] WriteMem: '-val' or 'msg' must be set.\r\n");
        return;
    }

    if(valString)
    {
        valInt = HSM_TestArgs2Int(valString);
        if(maskString && !posString && !lenString)
        {
            Str2Hex((const char *)maskString, &maskInt);
            valInt = (maskInt & valInt);
            Debug_Printf("write addr(0x%x) mask(%d): 0x%x\r\n", addrInt, maskInt, valInt);
             *((volatile uint32_t *)addrInt) = ((*((volatile uint32_t *)addrInt)) & (~maskInt)) | (valInt);
        }
        else if(posString && lenString && !maskString)
        {
            posInt = HSM_TestArgs2Int(posString);
            lenInt = HSM_TestArgs2Int(lenString);
            valInt = ((~(0xFFFFFFFFUL << lenInt)) & valInt);
            Debug_Printf("write addr(0x%x) pos(%d) len(%d): 0x%x\r\n", addrInt, posInt, lenInt, valInt);
            maskInt = ((~(0xFFFFFFFFUL << lenInt)) << posInt);
            *((volatile uint32_t *)addrInt) = ((*((volatile uint32_t *)addrInt)) & (~maskInt)) | (valInt << posInt);
        }
        else if(!posString && !lenString && !maskString)
        {
            Debug_Printf("write addr(0x%x): 0x%x\r\n", addrInt, valInt);
            *((volatile uint32_t *)addrInt) = valInt;
        }
        else
        {
            Debug_Printf("[ER][HSM] WriteMem param input fail.\r\n");
        }
    }
    else
    {
        HSM_TestArgs2HexBuf(msgString, (uint8_t *)msgBuf);
        if(!msgLenString)
        {
            Debug_Printf("[ER][HSM] WriteMem: 'msglen' must be set.\r\n");
            return;
        }

        msgLenInt = HSM_TestArgs2Int(msgLenString);
        if(msgLenInt > 48)
        {
            Debug_Printf("[ER][HSM] WriteMem: 'msglen' must be set <= 48.\r\n");
            return;
        }
        for(uint8_t i=0; i < msgLenInt; i++)
        {
            *((volatile uint8_t *)(addrInt+i)) = msgBuf[i];
        }
        Debug_Printf("[ER][HSM] WriteMem: Done.\r\n");
    }
}

/*****************************************
 *           HSM_TEST_SET_KEY
 *****************************************/
void HSM_SetKeyTest(void *param, int argc, const char** argv)
{
    int testId = (int)param;
    char *opsString;
    char *typeString;
    char *kidString;
    char *keyString;
    int opsInt;
    int typeInt;
    int kidInt;
    uint32_t keyDataBuf[4];
    HSM_DumpInputArgs(argc, argv);

    if (testId != HSM_TEST_SET_KEY)
    {
        Debug_Printf("[ER][HSM] Error Test Id:%d\r\n", testId);
        return;
    }

    opsString = HSM_TestParseArgs(argc, argv, "-ops");
    typeString = HSM_TestParseArgs(argc, argv, "-type");
    kidString = HSM_TestParseArgs(argc, argv, "-kid");
    keyString = HSM_TestParseArgs(argc, argv, "-key");
    if (!opsString || !typeString || !kidString)
    {
        Debug_Printf("[ER][HSM] SetKey Test: '-ops' '-type' '-kid' must be set.\r\n");
        return;
    }

    opsInt = HSM_TestArgs2Int(opsString);
    typeInt = HSM_TestArgs2Int(typeString);
    kidInt = HSM_TestArgs2Int(kidString);
    if (opsInt > 1 || typeInt > 1)
    {
        Debug_Printf("[ER][HSM] SetKey Test: unexpected param for '-ops' or '-type':%d-%d\r\n",
                 opsInt, typeInt);
        return;
    }

    if (opsInt == 0 && !keyString)
    {
        Debug_Printf("[ER][HSM] SetKey Test: '-key' must be set in add mode.\r\n");
        return;
    }

    if ((kidInt < 8 || kidInt > 18) ||
        (typeInt == 0 && kidInt != 18))
    {
        Debug_Printf("[ER][HSM] SetKey Test: '-kid' error:%d.\r\n", kidInt);
        return;
    }

    HSM_TestArgs2HexBuf(keyString, (uint8_t *)keyDataBuf);
    if (opsInt == 1)
    {
        /* Key delete test */
        if (typeInt)
        {
            if (HSM_UninstallFlashKey((HSM_KeyIndex)(kidInt)) != STATUS_SUCCESS)
            {
                Debug_Printf("[ER][HSM] SetKey Test: Delete Flash Key Failed.\r\n");
            }
            else
            {
                Debug_Printf("[INFO][HSM] SetKey Test: Delete Flash Key Pass\r\n");
            }
        }
        else
        {
            HSM_UninstallRamKey();
            Debug_Printf("[INFO][HSM] SetKey Test: Delete RAM Key Pass\r\n");
        }
    }
    else
    {
        /* Key install test */
        if (typeInt)
        {
            /* Install Flash Key */
            if (HSM_InstallFlashKey((uint32_t *)keyDataBuf, (HSM_KeyIndex)(kidInt)) != STATUS_SUCCESS)
            {
                Debug_Printf("[ER][HSM] SetKey Test: Install Flash Key Failed.\r\n");
            }
            else
            {
                Debug_Printf("[INFO][HSM] SetKey Test: Install Flash Key Pass\r\n");
            }
        }
        else
        {
            /* Install RAM Key */
            HSM_InstallRamKey((uint32_t *)keyDataBuf);
            Debug_Printf("[INFO][HSM] SetKey Test: Install RAM Key Pass\r\n");
        }
    }

    return;
}

/*****************************************
 *           HSM_TEST_UID
 *****************************************/
void HSM_GetUIDTest(void *param, int argc, const char** argv)
{
    uint32_t keyDataBuf[4];
    int testId = (int)param;

    HSM_DumpInputArgs(argc, argv);

    if (testId != HSM_TEST_UID)
    {
        Debug_Printf("[ER][HSM] Error Test Id:%d\r\n", testId);
        return;
    }

    HSM_GetUID(keyDataBuf);

    HSM_TestDumpBuf(keyDataBuf, 16, "UID Data:");
    return;
}

/*****************************************
 *           HSM_TEST_IRQ
 *****************************************/
static void __HSM_TestIrqCallback(void *device, uint32_t cmd, uint32_t status)
{
    (void)device;
    Debug_Printf("[INFO][HSM] Interrupt happend OK with(CMD: %s, Status: %x)\r\n", g_hsmCmdName[cmd], status);
}

void HSM_IRQEnableTest(void *param, int argc, const char** argv)
{
    char *enableStr;
    int enableInt;
    int testId = (int)param;

    HSM_DumpInputArgs(argc, argv);

    if (testId != HSM_TEST_IRQ)
    {
        Debug_Printf("[ER][HSM] Error Test Id:%d\r\n", testId);
        return;
    }

    if (argc != 2)
    {
        Debug_Printf("[ER][HSM] IRQ Test: Param num error:%d\r\n", argc);
        return;
    }

    enableStr = HSM_TestParseArgs(argc, argv, "-enable");
    if (!enableStr)
    {
        Debug_Printf("[ER][HSM] IRQ Test: '-enable' should be set\r\n");
        return;
    }

    enableInt = HSM_TestArgs2Int(enableStr);
    switch (enableInt)
    {
        case 0:
            NVIC_DisableIRQ(HSM_IRQn);
            HSM_EnableIrq(DISABLE);
            HSM_InstallCallback(NULL);
            Debug_Printf("[INFO][HSM] Interrupt Disable OK\r\n");
            break;
        case 1:
            HSM_InstallCallback(__HSM_TestIrqCallback);
            HSM_EnableIrq(ENABLE);
            NVIC_EnableIRQ(HSM_IRQn);
            Debug_Printf("[INFO][HSM] Interrupt Enable OK\r\n");
            break;
        default:
            Debug_Printf("[ER][HSM] IRQ Test: '-enable' set unknown:%d\r\n", enableInt);
    }

    return;
}

/*****************************************
 *           HSM_TEST_AES
 *****************************************/
static void __HSM_TestAESAsyncCallback(void *device, uint32_t status, uint32_t cmd)
{
    (void)device;
    Debug_Printf("[INFO][HSM] AES DMA process finished with Status: %x, cmd:%x\r\n", status, cmd);
    HSM_TestDumpBuf((uint32_t *)g_hsmCryptoOutBuffer, g_hsmCryptoOutLen, "AES Async Output");
}

void HSM_CryptoAesTest(void *param, int argc, const char** argv)
{
    char *encStr;
    char *modeStr;
    char *kidStr;
    char *keyStr;
    char *ivStr;
    char *syncStr;
    char *msgStr;
    char *msgLenStr;
    char *addrStr;
    int  addrInt;
    int encInt;
    int modeInt;
    int kidInt;
    uint8_t keyData[16] = {0};
    uint8_t ivData[16] = {0};
    int syncInt;
    uint8_t *msgPtr;
    uint8_t msgData[HSM_CRYPTO_BUFFER_MAX_LEN] = {0};
    int msgLenInt;
    int testId = (int)param;
    status_t res;
    DeviceCallback_Type cb = NULL;

    HSM_DumpInputArgs(argc, argv);

    if (testId != HSM_TEST_AES)
    {
        Debug_Printf("[ER][HSM] Error Test Id:%d\r\n", testId);
        return;
    }

    /* Parse Args String */
    encStr = HSM_TestParseArgs(argc, argv, "-functype");
    modeStr = HSM_TestParseArgs(argc, argv, "-modetype");
    kidStr = HSM_TestParseArgs(argc, argv, "-kid");
    if (!kidStr)
    {
        keyStr = HSM_TestParseArgs(argc, argv, "-key");
    }
    else
    {
        keyStr = NULL;

    }
    syncStr = HSM_TestParseArgs(argc, argv, "-sync");
    msgStr = HSM_TestParseArgs(argc, argv, "-msg");
    msgLenStr = HSM_TestParseArgs(argc, argv, "-msglen");
    addrStr = HSM_TestParseArgs(argc, argv, "-addr");

    /* Parse Args Data */
    encInt = HSM_TestArgs2Int(encStr);
    modeInt = HSM_TestArgs2Int(modeStr);
    if (modeInt)
    {
        ivStr = HSM_TestParseArgs(argc, argv, "-iv");
        HSM_TestArgs2HexBuf(ivStr, ivData);
    }
    else
    {
        ivStr = NULL;
    }
    if (!kidStr)
    {
        HSM_TestArgs2HexBuf(keyStr, keyData);
    }
    else
    {
        kidInt = HSM_TestArgs2Int(kidStr);
    }

    if(!msgStr && addrStr)
    {
        addrInt = HSM_TestArgs2Int(addrStr);
        msgPtr = (uint8_t *)addrInt;
    }
    else if(msgStr && !addrStr)
    {
        HSM_TestArgs2HexBuf(msgStr, msgData);
        msgPtr = msgData;
    }
    else
    {
        Debug_Printf("[ER][HSM] AES Test: msg and addr not NULL at the same time \r\n");
        return;
    }

    syncInt = HSM_TestArgs2Int(syncStr);
    msgLenInt = HSM_TestArgs2Int(msgLenStr);

    /* Param check */
    if (encInt > 1 || modeInt > 1 || syncInt > 1)
    {
        Debug_Printf("[ER][HSM] AES Test: unexpected param:%d-%d-%d\r\n", encInt, modeInt, syncInt);
        return;
    }
    if (msgLenInt == 0 || msgLenInt > HSM_CRYPTO_BUFFER_MAX_LEN || (msgLenInt & 0xF))
    {
        Debug_Printf("[ER][HSM] AES Test: unexpected msgLen:%d\r\n", msgLenInt);
        return;
    }
    if ((keyStr && (strlen(keyStr) != 32)) ||
        (ivStr && (strlen(ivStr) != 32)))
    {
        Debug_Printf("[ER][HSM] AES Test: ivLen/keyLen not right:%d-%d\r\n",
            strlen(keyStr) / 2, strlen(ivStr) / 2);
        return;
    }
    if (kidStr && (kidInt < HSM_FLASH_KEY_START_ID || kidInt > HSM_RAM_KEY_ID))
    {
        Debug_Printf("[ER][HSM] AES Test: unsupport keyIndex:%d\r\n", kidInt);
        return;
    }

    /* Test Handle */
    if (keyStr)
    {
        HSM_InstallRamKey((uint32_t *)keyData);
        kidInt = HSM_RAM_KEY_ID;
    }
    if (syncInt)
    {
        cb = __HSM_TestAESAsyncCallback;
        g_hsmCryptoOutLen = msgLenInt;
        g_hsmCryptoCmacMode = encInt;
    }

    memset(g_hsmCryptoOutBuffer, 0, HSM_CRYPTO_BUFFER_MAX_LEN);

    Debug_Printf("[INFO][HSM] AES Test Start with: %s-%s mode in key-%d\r\n",
                                modeInt?"[CBC]":"[ECB]", syncInt?"[DMA]":"[Polling]", kidInt);
    g_startTime = OSTimeGet();

    if (modeInt)
    {
        res = HSM_CryptoAesCBC((HSM_EncMode)encInt, (uint32_t *)msgPtr, msgLenInt, (uint32_t *)g_hsmCryptoOutBuffer,
                               (uint32_t *)ivData, (HSM_KeyIndex)kidInt, cb);
    }
    else
    {
        res = HSM_CryptoAesECB((HSM_EncMode)encInt, (uint32_t *)msgPtr, msgLenInt, (uint32_t *)g_hsmCryptoOutBuffer,
                               (HSM_KeyIndex)kidInt, cb);
    }

    if (res)
    {
        Debug_Printf("[ER][HSM] AES process error happend: %d\r\n", res);
        return;
    }
    else
    {
        g_endTime = OSTimeGet();
        Debug_Printf("[INFO][HSM] AES Test Finished, cost time:%dms\r\n", ApiCostTime());
        HSM_TestDumpBuf((uint32_t *)g_hsmCryptoOutBuffer, msgLenInt, "AES Result:");
    }

    /* Free Source*/
    if (keyStr)
    {
        HSM_UninstallRamKey();
    }
    return;
}

/*****************************************
 *           HSM_TEST_CMAC
 *****************************************/
static void __HSM_TestCMACAsyncCallback(void *device, uint32_t status, uint32_t cmd)
{
    (void)device;

    g_hsmCryptoDMASyncFini++;
    Debug_Printf("[INFO][HSM] CMAC Test DMA handler in-%d(Total:%d) status:%x, cmd:%x!\r\n",
             g_hsmCryptoDMASyncFini, g_hsmCryptoDMASyncTotal, status, cmd);

    if (g_hsmCryptoDMASyncFini == g_hsmCryptoDMASyncTotal)
    {
        if (status == STATUS_SUCCESS)
        {
            g_endTime = OSTimeGet();
            if (!g_hsmCryptoCmacMode)
            {
                Debug_Printf("[INFO][HSM] CMAC sign Pass!\r\n");
                HSM_TestDumpBuf((uint32_t *)g_hsmCryptoOutBuffer, g_hsmCryptoOutLen, "CMAC Async Signature");
            }
            else
                Debug_Printf("[INFO][HSM] CMAC DMA proces verify Pass!\r\n");
            Debug_Printf("[INFO][HSM] CMAC DMA proces Done! cost time:%dms\r\n", ApiCostTime());
        }
        else
        {
            Debug_Printf("[ER][HSM] CMAC Test DMA handler finished with Fail:%x!\r\n", status);
        }
    }
}

void HSM_CryptoCMACTest(void *param, int argc, const char** argv)
{
    char *encStr;
    char *kidStr;
    char *keyStr;
    char *syncStr;
    char *msgStr;
    char *addrStr;
    char *msgLenStr;
    char *macStr;
    int encInt;
    int kidInt;
    uint8_t keyData[16] = {0};
    int syncInt;
    uint8_t msgData[HSM_CRYPTO_BUFFER_MAX_LEN] = {0};
    uint32_t addrInt;
    int msgLenInt;
    int processLen;
    int updateLen;
    int testId = (int)param;
    status_t res;
    BOOL_Type lastPackage;
    DeviceCallback_Type cb = NULL;

    HSM_DumpInputArgs(argc, argv);

    if (testId != HSM_TEST_CMAC)
    {
        Debug_Printf("[ER][HSM] Error Test Id:%d\r\n", testId);
        return;
    }

    /* Parse Args String */
    encStr = HSM_TestParseArgs(argc, argv, "-functype");
    kidStr = HSM_TestParseArgs(argc, argv, "-kid");
    if (!kidStr)
    {
        keyStr = HSM_TestParseArgs(argc, argv, "-key");
    }
    else
    {
        keyStr = NULL;

    }
    syncStr = HSM_TestParseArgs(argc, argv, "-sync");
    addrStr = HSM_TestParseArgs(argc, argv, "-addr");
    if (!addrStr)
    {
        msgStr = HSM_TestParseArgs(argc, argv, "-msg");
    }
    else
    {
        msgStr = NULL;

    }
    msgLenStr = HSM_TestParseArgs(argc, argv, "-msglen");
    macStr = HSM_TestParseArgs(argc, argv, "-mac");

    if (!encStr || !syncStr || !msgLenStr ||
        (!msgStr && !addrStr) || (!kidStr && !keyStr))
    {
        Debug_Printf("[ER][HSM] CMAC Test: '-enc' '-sync' '-msg/-addr' '-kid/-key' should be set\r\n");
        return;
    }

    /* Parse Args Data */
    encInt = HSM_TestArgs2Int(encStr);
    if (!kidStr)
    {
        HSM_TestArgs2HexBuf(keyStr, keyData);
    }
    else
    {
        kidInt = HSM_TestArgs2Int(kidStr);
    }
    syncInt = HSM_TestArgs2Int(syncStr);
    if (!msgStr)
    {
        Str2Hex((const char *)addrStr, &addrInt);
    }
    else
    {
        HSM_TestArgs2HexBuf(msgStr, msgData);
    }
    msgLenInt = HSM_TestArgs2Int(msgLenStr);
    memset(g_hsmCryptoOutBuffer, 0U, HSM_CRYPTO_BUFFER_MAX_LEN);
    if (macStr)
    {
        HSM_TestArgs2HexBuf(macStr, g_hsmCryptoOutBuffer);
    }

    /* Param check */
    if (encInt > 1U || syncInt > 1U)
    {
        Debug_Printf("[ER][HSM] CMAC Test: unexpected param:%d-%d\r\n", encInt, syncInt);
        return;
    }
    if (!addrInt && !msgLenInt)
    {
        Debug_Printf("[ER][HSM] CMAC Test: unexpected msgLen:%d\r\n", msgLenInt);
        return;
    }
    if (keyStr && (strlen(keyStr) != 32U))
    {
        Debug_Printf("[ER][HSM] CMAC Test: keyLen not right:%d\r\n", strlen(keyStr) / 2U);
        return;
    }
    if (kidStr && (kidInt < (int)HSM_FLASH_KEY_START_ID || kidInt > (int)HSM_RAM_KEY_ID))
    {
        if (kidInt != HSM_BL_VERIFY_KEY_ID)
        {
            Debug_Printf("[ER][HSM] CMAC Test: unsupport keyIndex:%d\r\n", kidInt);
            return;
        }
    }

    /* Test Handle */
    if (keyStr)
    {
        HSM_InstallRamKey((uint32_t *)keyData);
        kidInt = HSM_RAM_KEY_ID;
    }
    if (syncInt)
    {
        cb = __HSM_TestCMACAsyncCallback;
        g_hsmCryptoOutLen = 16;
        g_hsmCryptoCmacMode = encInt;
    }

    Debug_Printf("[INFO][HSM] CMAC Test Start with: %s-%s mode in key-%d\r\n",
                                encInt?"[Verify]":"[Sign]", syncInt?"[DMA]":"[Polling]", kidInt);
    g_startTime = OSTimeGet();
    res = HSM_CryptoAesCMACStart((HSM_CmacMode)encInt, msgLenInt, (HSM_KeyIndex)kidInt,
                                 (uint32_t *)g_hsmCryptoOutBuffer, cb);
    if (res)
    {
        Debug_Printf("[ER][HSM] CMAC start process error happend: %d\r\n", res);
        return;
    }

    if (!msgStr)
    {
        processLen = 0;
        g_hsmCryptoDMASyncTotal = (msgLenInt / HSM_CMAC_MAX_LEN_ONETIME) + \
                                  ((msgLenInt % HSM_CMAC_MAX_LEN_ONETIME)?1U:0U);
        g_hsmCryptoDMASyncSend = 0;
        g_hsmCryptoDMASyncFini = 0;

        /* multi-update process if input is too large */
        while (processLen < msgLenInt)
        {
            if (msgLenInt - processLen > HSM_CMAC_MAX_LEN_ONETIME)
            {
                updateLen = HSM_CMAC_MAX_LEN_ONETIME;
                lastPackage = FALSE;
            }
            else
            {
                updateLen = msgLenInt - processLen;
                lastPackage = TRUE;
            }

            /* sycn previous dma stauts before send next dma request */
            while (1)
            {
                if (g_hsmCryptoDMASyncSend == g_hsmCryptoDMASyncFini)
                {
                    break;
                }
            }
            g_hsmCryptoDMASyncSend++;
            res = HSM_CryptoAesCMACUpdate((uint32_t *)addrInt, updateLen, lastPackage);
            if (res)
            {
                Debug_Printf("[ER][HSM] CMAC DMA Test: update process failed: %x in loop(%x-%x)\r\n",
                                            res, processLen, msgLenInt);
                return;
            }
            if (!syncInt)
            {
                g_hsmCryptoDMASyncFini++;
            }
            processLen += updateLen;
            addrInt += updateLen;
        }
    }
    else
    {
        g_hsmCryptoDMASyncTotal = (msgLenInt / HSM_CMAC_MAX_LEN_ONETIME) + \
                                  ((msgLenInt % HSM_CMAC_MAX_LEN_ONETIME)?1U:0U);
        g_hsmCryptoDMASyncSend = 0;
        g_hsmCryptoDMASyncFini = 0;

        /* sycn previous dma stauts before send next dma request */
            while (1)
            {
                if (g_hsmCryptoDMASyncSend == g_hsmCryptoDMASyncFini)
                {
                    break;
                }
            }

        g_hsmCryptoDMASyncSend++;
        res = HSM_CryptoAesCMACUpdate((uint32_t *)msgData, msgLenInt, TRUE);
        if (res)
        {
            //Debug_Printf("[ER][HSM] CMAC [%s] Polling Test: verify failed: %x\r\n",
            //         (kidInt == HSM_BL_VERIFY_KEY_ID)?"Bootloader":"Upgrade-Firmware", res);
        }
        else
        {
            //Debug_Printf("[INFO][HSM] CMAC [%s] Polling Test: verify PASS\r\n",
            //        (kidInt == HSM_BL_VERIFY_KEY_ID)?"Bootloader":"Upgrade-Firmware");
        }
    }

    if (!syncInt)
    {
        g_endTime = OSTimeGet();
        if (encInt)
        {
            if (res)
            {
                Debug_Printf("[ER][HSM] CMAC verify update process : Verify FAIL:%x!\r\n", res);
            }
            else
            {
                Debug_Printf("[INFO][HSM] CMAC verify update process : Verify PASS! cost time:%dms\r\n", ApiCostTime());
            }
        }
        else
        {
            if (res)
            {
                Debug_Printf("[ER][HSM] CMAC sign update process error: %d\r\n", res);
            }
            else
            {
                Debug_Printf("[INFO][HSM] CMAC sign update PASS! cost time:%dms\r\n", ApiCostTime());
                HSM_TestDumpBuf((uint32_t *)g_hsmCryptoOutBuffer, 16, "CMAC Signature:");
            }
        }
    }
    else
    {
        if (res)
        {
            Debug_Printf("[ER][HSM] CMAC update process error happend in async: %d\r\n", res);
        }
    }

    /* Free Source*/
    if (keyStr)
    {
        HSM_UninstallRamKey();
    }
    return;
}

/*****************************************
 *           HSM_TEST_RBG
 *****************************************/
void HSM_CryptoRBGTest(void *param, int argc, const char** argv)
{
    char *lenStr;
    int lenInt;
    status_t res;
    int testId = (int)param;

    HSM_DumpInputArgs(argc, argv);

    if (testId != HSM_TEST_RBG)
    {
        Debug_Printf("[ER][HSM] Error Test Id:%d\r\n", testId);
        return;
    }

    /* Parse Args string */
    lenStr = HSM_TestParseArgs(argc, argv, "-len");
    if (!lenStr)
    {
        Debug_Printf("[ER][HSM] RBG Test: '-len' should be config\r\n");
        return;
    }

    /* Parse Args data */
    lenInt = HSM_TestArgs2Int(lenStr);
    /* Param Check */
    if (lenInt > HSM_CRYPTO_BUFFER_MAX_LEN)
    {
        Debug_Printf("[ER][HSM] RBG Test: 'len' data error:%d\r\n", lenInt);
        return;
    }
    if(lenInt % 4 == 0)
    {
        HSM_ClearCmdStatus();
    }
    memset(g_hsmCryptoOutBuffer, 0, HSM_CRYPTO_BUFFER_MAX_LEN);

    Debug_Printf("[INFO][HSM] RBG Test: start get %d bytes!\r\n", lenInt);
    g_startTime = OSTimeGet();
    res = HSM_CryptoGetRandom(lenInt, (uint32_t *)g_hsmCryptoOutBuffer);
    if (res)
    {
        Debug_Printf("[ER][HSM] RBG Test: error happend:%d\r\n", res);
        return;
    }
    else
    {
        g_endTime = OSTimeGet();
        Debug_Printf("[INFO][HSM][%dms] RBG Test: get random pass.cost time:%dms\r\n", ApiCostTime());
        HSM_TestDumpBuf((uint32_t *)g_hsmCryptoOutBuffer, lenInt, "Random Bytes:");
    }

    return;
}

/*****************************************
 *           HSM_TEST_STAT
 *****************************************/
void HSM_GetStatTest(void *param, int argc, const char** argv)
{
    uint32_t status;
    int testId = (int)param;

    HSM_DumpInputArgs(argc, argv);

    if (testId != HSM_TEST_STAT)
    {
        Debug_Printf("[ER][HSM] Error Test Id:%d\r\n", testId);
        return;
    }

    status = HSM_GetCmdStatus();
    Debug_Printf("[INFO][HSM] STAT Test: current hsm status:0x%08x.\r\n", status);

    return;
}

/*****************************************
 *           HSM_TEST_CHLG
 *****************************************/
void HSM_GetChlgTest(void *param, int argc, const char** argv)
{
    char *kidStr;
    int kidInt;
    uint32_t status;
    int testId = (int)param;

    HSM_DumpInputArgs(argc, argv);

    if (testId != HSM_TEST_CHLG)
    {
        Debug_Printf("[ER][HSM] Error Test Id:%d\r\n", testId);
        return;
    }

    /* Parse Args string */
    kidStr = HSM_TestParseArgs(argc, argv, "-kid");
    if (!kidStr)
    {
        Debug_Printf("[ER][HSM] Chlg Test: '-kid' should be config\r\n");
        return;
    }

    /* Parse Args data */
    kidInt = HSM_TestArgs2Int(kidStr);
    if (kidInt < HSM_DEBUG_AUTH_KEY_ID || kidInt > HSM_LOCK_STAT_KEY_ID)
    {
        Debug_Printf("[ER][HSM] Chlg Test: unsupported 'kid': %d\r\n", kidInt);
        return;
    }

    memset(g_hsmCryptoOutBuffer, 0, 16);
    status = HSM_CryptoAuthInit((HSM_KeyIndex)kidInt, (uint32_t *)g_hsmCryptoOutBuffer);
    HSM_TestDumpBuf((uint32_t *)g_hsmCryptoOutBuffer, 16, "challenge:");
    if (status)
    {
        Debug_Printf("[ER][HSM] Chlg Test: get chlg error: %x.\r\n", status);
    }
    else
    {
#if HSM_SOFT_AUTH_ENABLE
        /* default otp key is all 'F' */
        uint32_t test_key[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
        uint32_t g_macbuf[4];

        HSM_InstallRamKey(test_key);
        status = HSM_SoftWareGenerateCmac(g_hsmCryptoOutBuffer, 16 ,(uint8_t *)g_macbuf,(uint8_t *)test_key);
        if (status)
        {
            Debug_Printf("[ER][HSM] HSM_SoftWareGenerateCmac Fail!status: %d\r\n", status);
        }
        HSM_TestDumpBuf((uint32_t *)g_macbuf, 16, "g_macbuf: ");
        status = HSM_CryptoAuthFinish((HSM_KeyIndex)kidInt, (uint32_t *)g_macbuf);
        if (status)
        {
            Debug_Printf("[ER][HSM] Auth Failed:%x.\r\n", status);
            HSM_TestDumpBuf((uint32_t *)g_hsmCryptoOutBuffer, 16, "Challenge: ");
            HSM_TestDumpBuf((uint32_t *)g_macbuf, 16, "Signature: ");
        }
        else
        {
            Debug_Printf("[INFO][HSM] Auth Pass for key-%d.\r\n", kidInt);
        }

        g_hsmGetChlgInit = FALSE;
#endif
        Debug_Printf("[INFO][HSM] Chlg Test: get success.\r\n");
        HSM_TestDumpBuf((uint32_t *)g_hsmCryptoOutBuffer, 16, "Challenge: ");
        g_hsmGetChlgInit = TRUE;
    }

    return;
}

/*****************************************
 *           HSM_TEST_AUTH
 *****************************************/
void HSM_AuthTest(void *param, int argc, const char** argv)
{
    char *kidStr;
    char *macStr;
    int kidInt;
    uint32_t status;
    int testId = (int)param;

    if (!g_hsmGetChlgInit)
    {
        Debug_Printf("[ER][HSM] getchlg command should be called at first\r\n");
        return;
    }

    HSM_DumpInputArgs(argc, argv);

    if (testId != HSM_TEST_AUTH)
    {
        Debug_Printf("[ER][HSM] Error Test Id:%d\r\n", testId);
        return;
    }

    /* Parse Args string */
    kidStr = HSM_TestParseArgs(argc, argv, "-kid");
    macStr = HSM_TestParseArgs(argc, argv, "-mac");
    if (!kidStr || !macStr)
    {
        Debug_Printf("[ER][HSM] AUTH Test: '-kid' or '-mac' should be config\r\n");
        return;
    }

    /* Parse Args data */
    kidInt = HSM_TestArgs2Int(kidStr);
    if (kidInt < HSM_DEBUG_AUTH_KEY_ID || kidInt > HSM_LOCK_STAT_KEY_ID)
    {
        Debug_Printf("[ER][HSM] AUTH Test: unsupported 'kid': %d\r\n", kidInt);
        return;
    }
    if ((strlen(macStr) >> 1) != 16)
    {
        Debug_Printf("[ER][HSM] AUTH Test: 'kid' length error: %d\r\n", strlen(macStr));
        return;
    }

    memset(g_hsmCryptoOutBuffer, 0, HSM_CRYPTO_BUFFER_MAX_LEN);
    HSM_TestArgs2HexBuf(macStr, g_hsmCryptoOutBuffer);

    Debug_Printf("[INFO][HSM] AUTH Test: start auth for key:%d!\r\n", kidInt);
    g_startTime = OSTimeGet();
    status = HSM_CryptoAuthFinish((HSM_KeyIndex)kidInt, (uint32_t *)g_hsmCryptoOutBuffer);
    if (status)
    {
        Debug_Printf("[ER][HSM] AUTH Test: auth failed(%x) for key:%d.\r\n", status, kidInt);
    }
    else
    {
        g_endTime = OSTimeGet();
        Debug_Printf("[INFO][HSM] AUTH Test: key(%d) auth Pass.cost time:%dms\r\n", kidInt, ApiCostTime());
    }
    g_hsmGetChlgInit = FALSE;

    return;
}

/*****************************************
 *         HSM_TEST_BINARY_VERIFY
 *****************************************/
static void __HSM_TestVerifyAsyncCallback(void *device, uint32_t status, uint32_t cmd)
{
    (void)device;

    g_hsmCryptoDMASyncFini++;
    Debug_Printf("[INFO][HSM] Binary-Verify Test DMA handler in-%d(Total:%d) status:%x, cmd:%x!\r\n",
             g_hsmCryptoDMASyncFini, g_hsmCryptoDMASyncTotal, status, cmd);

    if (g_hsmCryptoDMASyncFini == g_hsmCryptoDMASyncTotal)
    {
        if (status == STATUS_SUCCESS)
        {
            g_endTime = OSTimeGet();
            Debug_Printf("[INFO][HSM] Binary-Verify Test DMA handler verify Pass! cost time:%dms\r\n", ApiCostTime());
        }
        else
        {
            Debug_Printf("[ER][HSM] Binary-Verify Test DMA handler finished with Fail:%x!\r\n", status);
        }
    }
}

void HSM_VerifyTest(void *param, int argc, const char** argv)
{
    char *kidStr;
    char *syncStr;
    char *msgStr;
    char *addrStr;
    char *msgLenStr;
    char *macStr;
    int kidInt;
    int syncInt;
    uint8_t msgData[HSM_CRYPTO_BUFFER_MAX_LEN] = {0};
    uint32_t addrInt;
    int msgLenInt;
    int testId = (int)param;
    status_t res;
    int updateLen;
    int processLen;
    BOOL_Type lastPackage;
    DeviceCallback_Type cb = NULL;

    HSM_DumpInputArgs(argc, argv);

    if (testId != HSM_TEST_BINARY_VERIFY)
    {
        Debug_Printf("[ER][HSM] Error Test Id:%d\r\n", testId);
        return;
    }

    /* Parse Args String */
    kidStr = HSM_TestParseArgs(argc, argv, "-kid");
    syncStr = HSM_TestParseArgs(argc, argv, "-sync");
    addrStr = HSM_TestParseArgs(argc, argv, "-addr");
    if (!addrStr)
    {
        msgStr = HSM_TestParseArgs(argc, argv, "-msg");
    }
    else
    {
        msgStr = NULL;

    }
    msgLenStr = HSM_TestParseArgs(argc, argv, "-msglen");
    macStr = HSM_TestParseArgs(argc, argv, "-mac");

    if (!syncStr || !msgLenStr || !macStr || !kidStr || (!msgStr && !addrStr))
    {
        Debug_Printf("[ER][HSM] Binary-Verify Test: '-sync' '-msg' '-mac' '-kid' '-msg/-addr' should be set\r\n");
        return;
    }

    /* Parse Args Data */
    kidInt = HSM_TestArgs2Int(kidStr);
    syncInt = HSM_TestArgs2Int(syncStr);
    if (!msgStr)
    {
        Str2Hex((const char *)addrStr, &addrInt);
    }
    else
    {
        HSM_TestArgs2HexBuf(msgStr, msgData);
    }
    memset(g_hsmCryptoOutBuffer, 0, HSM_CRYPTO_BUFFER_MAX_LEN);
    msgLenInt = HSM_TestArgs2Int(msgLenStr);
    HSM_TestArgs2HexBuf(macStr, g_hsmCryptoOutBuffer);

    /* Param check */
    if (kidInt != HSM_BL_VERIFY_KEY_ID && kidInt != HSM_FW_UPGRADE_KEY_ID)
    {
        Debug_Printf("[ER][HSM] Binary-Verify Test: unsupported keyIndex:%d\r\n", kidInt);
        return;
    }
    if (!addrInt &&
        (msgLenInt == 0 || msgLenInt > HSM_CRYPTO_BUFFER_MAX_LEN))
    {
        Debug_Printf("[ER][HSM] Binary-Verify Test: unexpected msgLen:%d\r\n", msgLenInt);
        return;
    }

    /* Test Handle */
    if (syncInt)
    {
        cb = __HSM_TestVerifyAsyncCallback;
        g_hsmCryptoOutLen = 16;
    }

    Debug_Printf("[INFO][HSM] Binary-Verify Test: [%s] %dbytes verification start...\r\n",
            (kidInt == HSM_BL_VERIFY_KEY_ID)?"Bootloader":"Upgrade-Firmware", msgLenInt);
    g_startTime = OSTimeGet();
    res = HSM_FirmVerifyStart(msgLenInt, (HSM_KeyIndex)kidInt, (uint32_t *)g_hsmCryptoOutBuffer, cb);
    if (res)
    {
        Debug_Printf("[ER][HSM] Binary-Verify start process error happend: %d\r\n", res);
        return;
    }

    if (!msgStr)
    {
        processLen = 0;
        g_hsmCryptoDMASyncTotal = (msgLenInt / HSM_CMAC_MAX_LEN_ONETIME) + \
                                  ((msgLenInt % HSM_CMAC_MAX_LEN_ONETIME)?1U:0U);
        g_hsmCryptoDMASyncSend = 0;
        g_hsmCryptoDMASyncFini = 0;

        /* multi-update process if input is too large */
        while (processLen < msgLenInt)
        {
            if (msgLenInt - processLen > HSM_CMAC_MAX_LEN_ONETIME)
            {
                updateLen = HSM_CMAC_MAX_LEN_ONETIME;
                lastPackage = FALSE;
            }
            else
            {
                updateLen = msgLenInt - processLen;
                lastPackage = TRUE;
            }

            /* sycn previous dma stauts before send next dma request */
            while (1)
            {
                if (g_hsmCryptoDMASyncSend == g_hsmCryptoDMASyncFini)
                {
                    break;
                }
            }
            g_hsmCryptoDMASyncSend++;
            res = HSM_FirmVerifyUpdate((uint32_t *)addrInt, updateLen, lastPackage);
            if (res)
            {
                Debug_Printf("[ER][HSM] Binary-Verify DMA Test: update process failed: %x in loop(%x-%x)\r\n",
                                            res, processLen, msgLenInt);
                return;
            }
            if (!syncInt)
            {
                g_hsmCryptoDMASyncFini++;
            }

            processLen += updateLen;
            addrInt += updateLen;
        }

        if (!syncInt)
        {
            if (res)
            {
                Debug_Printf("[ER][HSM] Binary-Verify [%s] Polling Test: verify failed: %x\r\n",
                         (kidInt == HSM_BL_VERIFY_KEY_ID)?"Bootloader":"Upgrade-Firmware", res);
            }
            else
            {
                g_endTime = OSTimeGet();
                Debug_Printf("[INFO][HSM]Binary-Verify [%s] Polling Test: verify PASS! cost time:%dms\r\n",
                         (kidInt == HSM_BL_VERIFY_KEY_ID)?"Bootloader":"Upgrade-Firmware", ApiCostTime());
            }
        }
    }
    else
    {
        res = HSM_FirmVerifyUpdate((uint32_t *)msgData, msgLenInt, TRUE);
        if (!syncInt)
        {
            if (res)
            {
                Debug_Printf("[ER][HSM] Binary-Verify [%s] Polling Test: verify failed: %x\r\n",
                         (kidInt == HSM_BL_VERIFY_KEY_ID)?"Bootloader":"Upgrade-Firmware", res);
            }
            else
            {
                g_endTime = OSTimeGet();
                Debug_Printf("[INFO][HSM][%dms] Binary-Verify [%s] Polling Test: verify PASS! cost time:%dms\r\n",
                         (kidInt == HSM_BL_VERIFY_KEY_ID)?"Bootloader":"Upgrade-Firmware", ApiCostTime());
            }
        }
    }

    return;
}

#if HSM_SELFTEST_SUPPORT
#include "ac780x_eflash_reg.h"
#if HSM_CMAC_UNAIGN_SELFTEST
/*****************************************
 *         Unaligned Cmac Test
 *****************************************/
/* Expected Result for 0x10003 number of "0xFF" as input */
#define HSM_UNALIGN_CMAC_TESTLEN    (0x10003U)
#define HSM_UNALIGN_CMAC_TESTBASE   (0x8020000U)
static const char *g_hsmSelfTestCmacResult = "97539f20d3f9b9ef5471dd17681994d9";
static void __HSM_TestUnalignAsyncCallback(void *device, uint32_t status, uint32_t cmd)
{
    (void)device;
    (void)cmd;

    g_hsmCryptoDMASyncFini++;
}

static void __HSM_TestUnalignedCmac(BOOL_Type isdma)
{
    status_t res;
    DeviceCallback_Type cb = NULL;
    uint32_t inverter[] = {1, 2, 4, 8, 16, 20};
    int i, j;
    uint8_t result[16];
    uint8_t expect[16];
    uint32_t testInput, lenGroup;

    HSM_TestArgs2HexBuf((char *)g_hsmSelfTestCmacResult, expect);
    for (i = 0; i < sizeof(inverter) / sizeof(inverter[0]); i++)
    {
        memset(result, 0, 16);
        if (isdma)
        {
            g_hsmCryptoDMASyncTotal = inverter[i];
            g_hsmCryptoDMASyncSend = 0;
            g_hsmCryptoDMASyncFini = 0;
            cb = __HSM_TestUnalignAsyncCallback;
        }

        res = HSM_CryptoAesCMACStart(HSM_SIGN, HSM_UNALIGN_CMAC_TESTLEN, (HSM_KeyIndex)18, (uint32_t *)result, cb);
        if (res)
        {
            Debug_Printf("[ER][HSM] Unalign SelfTest: start failed(%x): %d-%d\r\n", res, i);
            return;
        }

        for (j = 0; j < inverter[i]; j++)
        {
            lenGroup = (HSM_UNALIGN_CMAC_TESTLEN & ~0xFFFU) / inverter[i];
            testInput = HSM_UNALIGN_CMAC_TESTBASE + j * lenGroup;
            if (j == inverter[i] - 1)
            {
                lenGroup = (HSM_UNALIGN_CMAC_TESTLEN - lenGroup * j);
            }
            if (isdma)
            {
                /* sycn previous dma stauts before send next dma request */
                while (1)
                {
                    if (g_hsmCryptoDMASyncSend == g_hsmCryptoDMASyncFini)
                    {
                        break;
                    }
                }
                g_hsmCryptoDMASyncSend++;
            }
            res = HSM_CryptoAesCMACUpdate((uint32_t *)testInput, lenGroup, (j == inverter[i] - 1)?TRUE:FALSE);
            if (res)
            {
                Debug_Printf("[ER][HSM] Unalign SelfTest: update failed: %x in loop(%d-%d)\r\n",
                                            res, i, j);
                return;
            }
        }

        if (isdma)
        {
            /* sycn previous dma stauts before send next dma request */
            while (1)
            {
                if (g_hsmCryptoDMASyncSend == g_hsmCryptoDMASyncFini)
                {
                    break;
                }
            }
        }

        if (!memcmp(result, expect, 16))
        {
            Debug_Printf("[INFO][HSM] Unalign SelfTest: result Pass in group:%d\r\n", i);
        }
        else
        {
            Debug_Printf("[ER][HSM] Unalign SelfTest: result failed in group:%d\r\n", i);
        }
    }
}

void HSM_UnalignCmacSelfTest(void *param, int argc, const char** argv)
{
    uint8_t key[16];
    int i;

    (void)param;
    (void)argc;
    (void)argv;

    if (EFLASH_UnlockCtrl() != STATUS_SUCCESS)
    {
        Debug_Printf("[ER][HSM] Unalign SelfTest: unlock failed\r\n");
        return;
    }

    /* Init input buffer as all "0xFF" */
    for (i = 0; i <= HSM_UNALIGN_CMAC_TESTLEN / PFLASH_PAGE_SIZE; i++)
    {
        if (EFLASH_PageErase(HSM_UNALIGN_CMAC_TESTBASE + i * PFLASH_PAGE_SIZE) != STATUS_SUCCESS)
        {
            Debug_Printf("[ER][HSM] Unalign SelfTest: erase(%x) failed\r\n",
                                         (HSM_UNALIGN_CMAC_TESTBASE + i * PFLASH_PAGE_SIZE));
            return;
        }
    }

    memset(key, 0xFF, 16);

    HSM_InstallRamKey((uint32_t *)key);

    __HSM_TestUnalignedCmac(FALSE);
    __HSM_TestUnalignedCmac(TRUE);

    EFLASH_LockCtrl();
}
#endif /* HSM_CMAC_UNAILGN_SELFTEST */

#if HSM_FLASHKEY_SELFTEST
/*****************************************
 *         FlashKey Stress Test
 *****************************************/
/*!< HSM Flash Key structure definitions */
typedef struct
{
    uint64_t slotInValidFlag : 16; /*!< slot valid flag: 0xA5A5 measn invalid, others depends on 'dataValidFlag' */
    uint64_t reservedValue : 48; /*!< Reserve */
} HSM_KeyInfo0;
/*!< HSM Flash Key structure definitions */
typedef struct
{
    uint32_t keyIndex : 4;       /*!< keyIndex: 0-9 = HSM input keyID - 8 */
    uint32_t keySlot : 5;        /*!< slot Id */
    uint32_t reservedValue0 : 15;/*!< Reserve */
    uint32_t dataValidFlag : 8;  /*!< keyData valid flag: 0x52 means valid, other means invalid */
    uint32_t reservedValue1;     /*!< Reserve */
} HSM_KeyInfo1;

/*!< HSM Flash Key structure definitions */
typedef struct
{
    HSM_KeyInfo0 keyInfo0;
    HSM_KeyInfo1 keyInfo1;
    uint8_t keyData[16];
} HSM_FlashKey;

/**
  * @brief There are only 32 key slot exist in one key page.
  */
/*!< HSM Flash Key structure definitions */
typedef struct
{
    HSM_FlashKey userKeyp[32];      /*!< Key slot */
    uint8_t pageReserve[0x400 - 8]; /*!< Reserve */
    uint32_t pageValidFlag0;
    uint32_t pageValidFlag1;
} HSM_FlashKeyPage;
static void __HSM_FlashKeyNormalTest(void)
{
    HSM_FlashKeyPage *flashPage;
    HSM_FlashKey *flashKey;
    uint32_t key[4];
    status_t status;
    int i;

    /* check another key page is disabled */
    flashPage = (HSM_FlashKeyPage *)0x8300800U + 1;
    if (flashPage->pageValidFlag0 == 0xA5A5A5A5 && flashPage->pageValidFlag1 == 0xA5A5A5A5)
    {
        Debug_Printf("[ER][HSM] Flashkey SelfTest: Normal other page unexpected valid\r\n");
        return;
    }

    flashPage = (HSM_FlashKeyPage *)0x8300800U;
    for (i = 0; i < 8; i++)
    {
        memset(key, i, 16);
        status = HSM_InstallFlashKey(key, (HSM_KeyIndex)(i + 8));
        if (status)
        {
            Debug_Printf("[ER][HSM] Flashkey SelfTest: Normal pre-install-%d failed\r\n",
                     i);
            return;
        }

        /* check pageflag, slot flag, keydata, key flag */
        if (flashPage->pageValidFlag0 != 0xA5A5A5A5 || flashPage->pageValidFlag1 != 0xA5A5A5A5)
        {
            Debug_Printf("[ER][HSM] Flashkey SelfTest: Normal page notvalid\r\n");
            return;
        }

        flashKey = (HSM_FlashKey *)flashPage;
        flashKey += i;
        if (flashKey->keyInfo0.slotInValidFlag == 0xA5A5)
        {
            Debug_Printf("[ER][HSM] Flashkey SelfTest: Normal slot-%d notvalid\r\n", i);
            return;
        }

        if (flashKey->keyInfo1.dataValidFlag != 0x52)
        {
            Debug_Printf("[ER][HSM] Flashkey SelfTest: Normal dataflag-%d notvalid\r\n", i);
            return;
        }

        if (memcmp(flashKey->keyData, key, 16))
        {
            Debug_Printf("[ER][HSM] Flashkey SelfTest: Normal key-%d data notvalid\r\n", i);
            return;
        }
    }

    for (i = 0; i < 8; i++)
    {
        status = HSM_UninstallFlashKey((HSM_KeyIndex)(i + 8));
        if (status)
        {
            Debug_Printf("[ER][HSM] Flashkey SelfTest: Normal pre-Uninstall-%d failed\r\n", i);
            return;
        }

        flashKey = (HSM_FlashKey *)flashPage;
        flashKey += i;
        if (flashKey->keyInfo0.slotInValidFlag != 0xA5A5)
        {
            Debug_Printf("[ER][HSM] Flashkey SelfTest: Normal slot-%d still valid\r\n", i);
            return;
        }
    }

    Debug_Printf("[INFO][HSM] Flashkey SelfTest: Normal Test Pass\r\n");
}

static void __HSM_FlashKeyRepeatTest(void)
{
    HSM_FlashKeyPage *flashPage;
    HSM_FlashKey *flashKey;
    uint32_t key[4];
    status_t status;
    int i, j;

    flashPage = (HSM_FlashKeyPage *)0x8300800U;
    for (j = 0; j < 3; j++)
    {
        for (i = 0; i < 8; i++)
        {
            memset(key, (uint8_t)(j << 4) | (uint8_t)i, 16);
            status = HSM_InstallFlashKey(key, (HSM_KeyIndex)(i + 8));
            if (status)
            {
                Debug_Printf("[ER][HSM] Flashkey SelfTest: Repeat pre-install-%d failed\r\n", i);
                return;
            }
        }
    }

    /* Now, all slot in main page is used-up.
     * If we trigger another install operation, it will cause page update.
     */
    memset(key, (uint8_t)0x30 | 7, 16);
    status = HSM_InstallFlashKey(key, (HSM_KeyIndex)(7 + 8));
    if (status)
    {
        Debug_Printf("[ER][HSM] Flashkey SelfTest: Repeat install for update failed\r\n");
        return;
    }

    /* if write number larger than 32, we check if page update happed */
    /* check default key page is disabled */
    flashPage = (HSM_FlashKeyPage *)0x8300800U;
    if (flashPage->pageValidFlag0 != 0xFFFFFFFF || flashPage->pageValidFlag1 != 0xFFFFFFFF)
    {
        Debug_Printf("[ER][HSM] Flashkey SelfTest: Repeat main page not update\r\n");
        return;
    }
    flashPage = (HSM_FlashKeyPage *)0x8300800U + 1;
    if (flashPage->pageValidFlag0 != 0xA5A5A5A5 || flashPage->pageValidFlag1 != 0xA5A5A5A5)
    {
        Debug_Printf("[ER][HSM] Flashkey SelfTest: Repeat backup page not update\r\n");
        return;
    }

    for (i = 0; i < 8; i++)
    {
        flashKey = (HSM_FlashKey *)flashPage;
        if (i != 0)
        {
            flashKey += 23 + i;
            memset(key, (uint8_t)(0x20 + i - 1), 16);
        }
        else
        {
            memset(key, (uint8_t)0x30 + (uint8_t)7, 16);
        }

        if (memcmp(flashKey->keyData, key, 16))
        {
            Debug_Printf("[ER][HSM] Flashkey SelfTest: Repeat key-%d data notvalid\r\n", i);
            return;
        }

        if (flashKey->keyInfo1.dataValidFlag != 0x52)
        {
            Debug_Printf("[ER][HSM] Flashkey SelfTest: Repeat dataflag-%d notvalid\r\n", i);
            return;
        }
    }

    Debug_Printf("[INFO][HSM] Flashkey SelfTest: Repeat Test Pass\r\n");
}

void HSM_FlashKeySelfTest(void *param, int argc, const char** argv)
{
    if (EFLASH_UnlockCtrl() != STATUS_SUCCESS)
    {
        Debug_Printf("[ER][HSM] Flashkey SelfTest: unlock failed\r\n");
        return;
    }

    /* Init the whole key flash before test */
    if (EFLASH_PageErase(0x8300800U) != STATUS_SUCCESS ||
        EFLASH_PageErase(0x8300800U + DFLASH_PAGE_SIZE) != STATUS_SUCCESS)
    {
        Debug_Printf("[ER][HSM] Flashkey SelfTest: erase failed\r\n");
        return;
    }

    __HSM_FlashKeyNormalTest();

    __HSM_FlashKeyRepeatTest();

    EFLASH_LockCtrl();
}
#endif /* HSM_FLASHKEY_SELFTEST */
#endif /* HSM_SELFTEST_SUPPORT */

typedef void (*pFunction)(void);

/*!
 * @brief jump to app
 *
 * @param[in] none
 *
 * @return none
 */
static void goToApp(uint32_t appAddr)
{
    uint32_t appMsp = 0;
    uint32_t appResetHandler = 0;
    //if (((*(__IO uint32_t *)APP_START_ADDRESS) >= 0x08000000) && ((*(__IO uint32_t *)APP_START_ADDRESS) <= 0x0803FFFF))
    //{
    /* disable interrupt */
    //__asm("CPSID i");
    //DisableInterrupts;
    /*Get Main stack pointer*/
    appMsp = *(__IO uint32_t*)appAddr;
    appResetHandler = *(__IO uint32_t *)(appAddr + 4);
    /* set MSP */
    //__set_MSP(appMsp);

    /* Jump to App */
    ((pFunction)appResetHandler)();

    //}
    /* never come there*/
    Debug_Printf("[E][HSM]Jump to APP FAIL!\r\n");
}

status_t HSM_verifyAppImage(uint32_t appAddress, uint32_t appCodeImageSize, uint8_t *macBuf)
{
    status_t ret = STATUS_ERROR;
    /* mac value for app image */
    //uint8_t macBuf[16] = {0x28, 0x72, 0x5b, 0x7d, 0xc8, 0x07, 0x0c, 0x51,0xa7, 0xa0, 0x94, 0x7e,0x81, 0xc8, 0x81, 0x14};
    uint32_t processLen = 0;
    uint32_t updateLen = 0;
    BOOL_Type lastPackage = FALSE;
    uint32_t hsmCryptoDMASyncSend = 0;
    uint32_t hsmCryptoDMASyncFini = 0;

    HSM_KeyIndex kidInt = HSM_BL_VERIFY_KEY_ID;

    g_startTime = OSTimeGet();
    ret = HSM_FirmVerifyStart(appCodeImageSize, kidInt, (uint32_t *)macBuf, NULL);
    if (ret)
    {
        Debug_Printf("[ER][HSM] Binary-Verify start process error happend: %d\r\n", ret);
        return ret;
    }

    /* multi-update process if input is too large */
    while (processLen < appCodeImageSize)
    {
        if (appCodeImageSize - processLen > HSM_CMAC_MAX_LEN_ONETIME)
        {
            updateLen = HSM_CMAC_MAX_LEN_ONETIME;
            lastPackage = FALSE;
        }
        else
        {
            updateLen = appCodeImageSize - processLen;
            lastPackage = TRUE;
        }

        /* sycn previous dma stauts before send next dma request */
        while (1)
        {
            if (hsmCryptoDMASyncSend == hsmCryptoDMASyncFini)
            {
                break;
            }
        }
        hsmCryptoDMASyncSend++;
        ret = HSM_FirmVerifyUpdate((uint32_t *)appAddress, updateLen, lastPackage);
        if (ret)
        {
            Debug_Printf("[ER][HSM] Binary-Verify DMA Test: update process failed: %x in loop(%x-%x)\r\n",
                                        ret, processLen, appCodeImageSize);
            return ret;
        }

        hsmCryptoDMASyncFini++;

        processLen += updateLen;
        appAddress += updateLen;
    }

    if (ret)
    {
        Debug_Printf("[ER][HSM] Binary-Verify [%s] Polling Test: verify failed: %x\r\n",
                 (kidInt == HSM_BL_VERIFY_KEY_ID)?"Bootloader":"Upgrade-Firmware", ret);
    }
    else
    {
        g_endTime = OSTimeGet();
        Debug_Printf("[INFO][HSM]Binary-Verify [%s] Polling Test: verify PASS! cost time:%dms\r\n",
                 (kidInt == HSM_BL_VERIFY_KEY_ID)?"Bootloader":"Upgrade-Firmware", ApiCostTime());
    }

    return ret;
}

/*  already setup ok, eg.otp key , enable secureboot..*/
/* rom area (pflash), start addr:0x08000000. the area store secureboot flow code */
void HSM_SecureBootFlow(void *param, int argc, const char** argv)
{
    status_t ret = STATUS_ERROR;
    uint8_t macBuf[16] = {0x28, 0x72, 0x5b, 0x7d, 0xc8, 0x07, 0x0c, 0x51,0xa7, 0xa0, 0x94, 0x7e,0x81, 0xc8, 0x81, 0x14};
    /*verify app image address*/
    uint32_t appAddress = 0x08020000;
    /*verif app image bytes */
    uint32_t appCodeImageSize = 10 * 1024;
    Debug_Printf("[I][HSM]HSM_SecureBoot Flow!\r\n");
    char *appAddrString;
    char *appCodeImageSizeString;
    char *macString;
    appAddrString = HSM_TestParseArgs(argc, argv, "-addr");
    appCodeImageSizeString = HSM_TestParseArgs(argc, argv, "-size");
    macString = HSM_TestParseArgs(argc, argv, "-mac");
    if (appAddrString && appCodeImageSizeString && macString)
    {
        Debug_Printf("[I][HSM]use input param!\r\n");
        Str2Hex((const char *)appAddrString, &appAddress);
        appCodeImageSize = HSM_TestArgs2Int(appCodeImageSizeString);
        HSM_TestArgs2HexBuf(macString, (uint8_t *)macBuf);
    }
    else
    {
        Debug_Printf("[I][HSM]use default value!\r\n");
    }

    Debug_Printf("[I][HSM]appAddress :0x%08x!\r\n", appAddress);
    Debug_Printf("[I][HSM]appCodeImageSize :0x%x!\r\n", appCodeImageSize);

    /* secureboot flow enable */
    if ((EFLASH->GSR & EFLASH_GSR_SECBOOT_Msk) >> EFLASH_GSR_SECBOOT_Pos)
    {
        Debug_Printf("[I][HSM]SecureBoot Enable!\r\n");
        ret = HSM_verifyAppImage(appAddress, appCodeImageSize, macBuf);
        if (ret == STATUS_SUCCESS)
        {
            Debug_Printf("[I][HSM]verify app image success\r\n");
            /* verify app image success*/
            /* jump to app */
            goToApp(appAddress);
        }
        else
        {
            /* verify app image fail */
            Debug_Printf("[I][HSM]verify app image fail\r\n");
        }
    }
    else
    {
        Debug_Printf("[I][HSM]SecureBoot Disable!\r\n");
    }
}

void HSM_RmAllFlashKey(void *param, int argc, const char** argv)
{
    uint32_t status = 0;
    status = HSM_UninstallAllFlashKey();
    if (status != 0)
    {
        Debug_Printf("[INFO][HSM] remove all flash key fail:0x%x\r\n", status);
    }
    else
    {
        Debug_Printf("[INFO][HSM] remove all flash key pass\r\n", status);
    }
}

/*****************************************
 *  Global Test Command Table ---- HSM
 *****************************************/
const EMUL_MENUITEM g_hsmTestMenu[] =
{
#ifdef REAL_CHIP
    {
        "b",
        "\r\n  -back",
        "Back to main menu",
        GotoMenu,
        (void *)g_mainMenu,
    },
#endif

    {
        "setkey",
        "\r\n  -Install/Uninstall user key",
        "setkey -type [0/1] -key [key hex string] <-del> keyindex",
        HSM_SetKeyTest,
        (void *)HSM_TEST_SET_KEY,
    },

    {
        "getuid",
        "\r\n  -Dump Chip UID(128bit)",
        "getuid",
        HSM_GetUIDTest,
        (void *)HSM_TEST_UID,
    },

    {
        "irq",
        "\r\n  -Enable(1)/Disable(0) HSM Interrupt",
        "irq -enable 0/1",
        HSM_IRQEnableTest,
        (void *)HSM_TEST_IRQ,
    },

    {
        "aes",
        "\r\n  -AES-ECB(0)/AES-CBC(1) Encrypt(0)/Decrypt(1)",
        "aes -functype [0/1] -modtype [0/1] -<kid/key> <[KeyIndex]/[KeyData]> -iv [ivData] -sync [0/1] -msg [msgData] -msgLen [msgLen]",
        HSM_CryptoAesTest,
        (void *)HSM_TEST_AES,
    },

    {
        "cmac",
        "\r\n  -CMAC-Sign(0)/CMAC-Verify(1)",
        "cmac -functype [0/1] -<kid/key> <[KeyIndex]/[KeyData]> -iv [ivData] -sync [0/1] -<msg/addr> <[msgData]/[address]> -msgLen [msgLen] -mac [macData]",
        HSM_CryptoCMACTest,
        (void *)HSM_TEST_CMAC,
    },

    {
        "rbg",
        "\r\n  -Get random bytes in wordsize",
        "rbg -len [wordsize]",
        HSM_CryptoRBGTest,
        (void *)HSM_TEST_RBG,
    },

    {
        "getstat",
        "\r\n  -Get HSM status",
        "getstat",
        HSM_GetStatTest,
        (void *)HSM_TEST_STAT,
    },

    {
        "getchlg",
        "\r\n  -Get authentication challenge data",
        "getchlg -kid [keyIndex: 3-7]",
        HSM_GetChlgTest,
        (void *)HSM_TEST_CHLG,
    },

    {
        "auth",
        "\r\n  -Do authentication for specific access controled by input key",
        "auth -kid [keyIndex: 3-7] -mac [macValue]",
        HSM_AuthTest,
        (void *)HSM_TEST_AUTH,
    },

    {
        "verify",
        "\r\n  -Verify the validity of the bootloader or upgrade firmware",
        "verify -kid [keyIndex: 1-2] -sync [0/1] <[-msg]/[-addr]> [msgData]/[address] -msglen [length] -mac [macData]",
        HSM_VerifyTest,
        (void *)HSM_TEST_BINARY_VERIFY,
    },

#if HSM_SELFTEST_SUPPORT
#if HSM_CMAC_UNAIGN_SELFTEST
    {
        "st-align",
        "\r\n  -Verify the cmac in unlignment case selftest",
        "st-align",
        HSM_UnalignCmacSelfTest,
        NULL,
    },
#endif
#if HSM_FLASHKEY_SELFTEST
    {
        "st-flk",
        "\r\n  -Verify the flash key operation selftest",
        "st-flk",
        HSM_FlashKeySelfTest,
        NULL,
    },
#endif
#endif
    {
        "datain",
        "\r\n  -Write message to dflash",
        "datain -addr [dflash addr] -msg [message] -msglen [length]",
        HSM_Data2flash,
        (void *)HSM_TEST_WRITEDFLASH,
    },

    {
        "readmem",
        "\r\n  -read memory",
        "readmem -addr [sram/reg addr] -pos [start pos] -len [read len]",
        HSM_ReadMem,
        (void *)HSM_TEST_READMEM,
    },

    {
        "writemem",
        "\r\n  -write memory",
        "writemem -addr [sram/reg addr] -val [value] -pos [start pos] -len [read len] -mask[mask]",
        HSM_WriteMem,
        (void *)HSM_TEST_WRITEMEM,
    },

    {
        "erase",
        "\r\n  -erase dflash",
        "writemem -addr [addr] -len [erase len]",
        HSM_EraseDflash,
        (void *)HSM_TEST_ERASEFLASH,
    },

    {
        "sb",
        "\r\n  -secure boot test",
        "sb -addr [addr] -len [erase len]",
        HSM_SecureBootFlow,
        (void *)HSM_TEST_SECUREBOOT,
    },

    {
        "rmallkey",
        "\r\n -remove all flash key",
        "",
        HSM_RmAllFlashKey,
        (void *)HSM_TEST_RMALLKEY,
    },

    {
        0,
    },
};

#ifndef REAL_CHIP
void hsm_interrupt_callback(uint32_t irq)
{
    (void)irq; // Placeholder for actual interrupt handling logic
    DEBUGMSG(DEBUG_ZONE_INFO, ("[INFO][HSM] HSM Interrupt Callback triggered.\r\n"));
}

int main(int argc, const char** argv)
{
    int i;
    const EMUL_MENUITEM *item;

    if (argc < 2)
    {
        printf("Usage: %s <command> [options]\n", argv[0]);
        return -1;
    }

    printf("Initializing HSM test environment with base address 0x%08X\n", HSM_BASE);

    // Initialize interface layer
    int result = interface_layer_init();
    if (result != 0) {
        printf("Interface layer initialization failed: %d\n", result);
        return -1;
    }

    // Register device with Python model
    result = register_device(5, HSM_BASE, 0x1000);
    if (result != 0) {
        printf("Device registration failed: %d\n", result);
        interface_layer_deinit();
        return -1;
    }

    register_interrupt_handler(HSM_IRQn, hsm_interrupt_callback);

    for (i = 0; g_hsmTestMenu[i].name != 0; i++)
    {
        item = &g_hsmTestMenu[i];
        if (strcmp(argv[1], item->name) == 0)
        {
            printf("Hsm test:%s start with param:%s.\n", item->name, argv[2]);
            item->handler(item->param, argc - 2, &argv[2]);
            goto fin;
        }
    }

fin:
    interface_layer_deinit();
    unregister_device(5);
    //printf("Unknown command: %s\n", argv[1]);
    return -1;
}
#endif