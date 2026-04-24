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
 * @file ac780x_eflash.c
 *
 * @brief This file provides EFLASH integration functions.
 *
 */

/* ===========================================  Includes  =========================================== */
#include "ac780x_eflash_reg.h"

/* ============================================  Define  ============================================ */
#define EFLASH_UNLOCK_KEY1               (0xac7805UL)     /* eflash controler ulock key 1    */
#define EFLASH_UNLOCK_KEY2               (0x01234567UL)   /* eflash controler ulock key 2    */
#define PFLASH_WRITE_PROTECT_NUM         (4UL)            /* P-flash write protection option byte address total count */
#define PFLASH_READ_PROTECT_NUM          (2UL)            /* Read protection option byte address total count */
#define DFLASH_WRITE_PROTECT_NUM         (4UL)            /* D-Flash write protection option byte address total count */
#define EXTERN_RESET_CTRL_NUM            (2UL)            /* External reset control info address total count */
#define OPT_PAGE1_VALID_NUM              (8UL)            /* Option page 1 valid configure word total count */
#define FLASH_GET_BUSY_STATUS_DELAY      (3UL)            /* FLASH get busy status delay */

/* ==========================================  Variables  =========================================== */

/* ======================================  Functions define  ======================================== */
/*!
 * @brief Unlock the eflash controller.
 *
 * @param[in] none
 * @return EFLASH STATUS: eflash operation status
 */
status_t EFLASH_UnlockCtrl(void)
{
    status_t statusRes = STATUS_SUCCESS;
    uint8_t ctrlLockBit;
    uint32_t timeoutCount = 0U;

    ctrlLockBit = EFLASH_GetCtrlLockBitReg();
    while (0U != ctrlLockBit)
    {
        EFLASH_SetKeyReg(EFLASH_UNLOCK_KEY1);
        EFLASH_SetKeyReg(EFLASH_UNLOCK_KEY2);

        if (100U < timeoutCount)
        {
            statusRes = STATUS_TIMEOUT;
            break;
        }
        ctrlLockBit = EFLASH_GetCtrlLockBitReg();
        timeoutCount++;
    }
    EFLASH_EnableLVDMON();

    return statusRes;
}

/*!
 * @brief Lock the eflash controler.
 *
 * @param[in] none
 * @return none
 */
void EFLASH_LockCtrl(void)
{
    EFLASH_DisableLVDMON();
    EFLASH_LockCtrlReg();
}

/*!
 * @brief Read a specified length of data from the eflash specified address.
 *
 * @param[in] addr: eflash address
 * @param[in] buffer: Point to the data bufffer for saving the read data
 * @param[in] len: read length(the unit is word)
 * @return none
 */
void EFLASH_Read(uint32_t addr, uint32_t *buffer, uint32_t len)
{
    uint32_t i, tempAddr = addr;

    DEVICE_ASSERT(((PFLASH_BASE_ADDRESS <= addr) && (PFLASH_END_ADDRESS > addr)) \
                  || ((DFLASH_BASE_ADDRESS <= addr) && (DFLASH_END_ADDRESS > addr)) \
                  || ((CINFO_BASE <= addr) && (CINFO_END > addr)) \
                  || ((DINFO_BASE <= addr) && (DINFO_END > addr)));
    DEVICE_ASSERT(NULL != buffer);

    for (i = 0U; i < len; i++)
    {
        buffer[i] = *(uint32_t*)tempAddr;
        tempAddr += 4U;
    }
}

/*!
 * @brief Perform start command sequence on eflash.
 *
 * @param[in] none
 * @return operation status
 *        - EFLASH_STATUS_SUCCESS: Operation was successful
 *        - EFLASH_STATUS_ERROR:   Operation was failure
 */
static status_t FLASH_StartCmdSequence(void)
{
    status_t  ret = STATUS_SUCCESS;
    volatile uint32_t tempDelay;

    /* Disable irq, avoid read Flash between Clear Cache and Erase Flash. */
    DisableInterrupts;
    /* Clear PFlash Cache. */
    EFLASH_ClearPCache();
    /* Clear DFlash Cache. */
    EFLASH_ClearDCache();
    /* trigger command execute */
    EFLASH_TrigCtrlCmdReg();
    EnableInterrupts;

    if(EFLASH_GetLVDWarningReg() != 0U)
    {
        ret = STATUS_ERROR;
    }
    else
    {
        /* Wait till busy bit is cleared */
        while (0U != EFLASH_GetCmdBusyStatus())
        {
            tempDelay = FLASH_GET_BUSY_STATUS_DELAY;
            while (tempDelay > 0U)
            {
                tempDelay--; /* for delay. */
            }
        };

        /* Check command execute error status */
        if (0U != EFLASH_GetStatusReg())
        {
            ret = STATUS_ERROR;
        }
    }

    return ret;
}

/*!
 * @brief Erase specified eflash user area address.
 *
 * @param[in] addr: eflash page addrees to be erased (should be 8-aligned)
 * @return EFLASH STATUS: eflash operation status
 */
status_t EFLASH_PageErase(uint32_t addr)
{
    status_t statusRes;

    /* check eflash address */
    DEVICE_ASSERT(((PFLASH_BASE_ADDRESS <= addr) && (PFLASH_END_ADDRESS > addr)) \
                  || ((DFLASH_BASE_ADDRESS <= addr) && (DFLASH_END_ADDRESS > addr)) \
                  || ((DINFO_BASE <= addr) && (DINFO_END > addr)));

    /* Wait for last operation to be completed */
    if (0U != EFLASH_GetCmdBusyStatus())
    {
        statusRes = STATUS_BUSY;
    }
    else if (EFLASH_GetLVDWarningReg() != 0U)
    {
        statusRes = STATUS_ERROR;
    }
    else
    {
        EFLASH_CLEAR_STAT_ERROR_BITS();
        /* Configure start address */
        EFLASH_SetStartAddressReg(addr);

        /* Configure command */
        EFLASH_SetCommandReg((uint32_t)EFLASH_CMD_PAGERASE);

        /* Trigger to start */
        statusRes = FLASH_StartCmdSequence();
    }

    return statusRes;
}

/*!
 * @brief Mass erase P-Flash or D-Flash.
 *
 * @param[in] addr: Specified eflash addrees to mass erase
 *            - P-Flash address: Mass erase P-Flash
 *            - D-Flash address: Mass erase D-Flash
 * @return EFLASH STATUS: eflash operation status
 */
status_t EFLASH_MassErase(uint32_t addr)
{
    status_t statusRes;

    /* check eflash address */
    DEVICE_ASSERT(((PFLASH_BASE_ADDRESS <= addr) && (PFLASH_END_ADDRESS > addr)) \
                  || ((DFLASH_BASE_ADDRESS <= addr) && (DFLASH_END_ADDRESS > addr)));
    /* Wait for last operation to be completed */
    if (0U != EFLASH_GetCmdBusyStatus())
    {
        statusRes = STATUS_BUSY;
    }
    else if (EFLASH_GetLVDWarningReg() != 0U)
    {
        statusRes = STATUS_ERROR;
    }
    else if (((PFLASH_BASE_ADDRESS <= addr) && (PFLASH_END_ADDRESS > addr)) \
                  && ((0U != EFLASH_GetPWriteProRegLow()) || (0U != EFLASH_GetPWriteProRegHigh())))
    {
        statusRes = STATUS_ERROR;
    }
    else if (((DFLASH_BASE_ADDRESS <= addr) && (DFLASH_END_ADDRESS > addr)) \
                  && ((0U != EFLASH_GetDWriteProRegLow()) || (0U != EFLASH_GetDWriteProRegHigh())))
    {
        statusRes = STATUS_ERROR;
    }
    else
    {
        EFLASH_CLEAR_STAT_ERROR_BITS();
        /* Configure start address */
        EFLASH_SetStartAddressReg(addr);

        /* Configure command */
        EFLASH_SetCommandReg((uint32_t)EFLASH_CMD_MASSRASE);

        /* Trigger to start */
        statusRes = FLASH_StartCmdSequence();
    }

    return statusRes;
}

/*!
 * @brief User page program unit.
 *
 * @param[in] addr: eflash addrees to be programed (should be 8-aligned)
 * @param[in] buffer: Point to the data to be programed
 * @param[in] len: Data length to be programed (the unit is 32-bit, should be 2-aligned)
 * @return EFLASH STATUS: eflash operation status
 */
static status_t EFLASH_PageProgramUnit(uint32_t addr, uint32_t *buffer, uint32_t len, uint32_t cntBase)
{
    status_t statusRes = STATUS_SUCCESS;
    uint32_t dataCount = 0x0U;
    volatile uint32_t tempDelay;
    volatile uint8_t delayI;
    (void)cntBase;

    /* Wait for last operation to be completed */
    if (0U != EFLASH_GetCmdBusyStatus())
    {
        statusRes = STATUS_BUSY;
    }
    else if (EFLASH_GetLVDWarningReg() != 0U)
    {
        statusRes = STATUS_ERROR;
    }
    else
    {
        EFLASH_CLEAR_STAT_ERROR_BITS();
        /* Configure start address */
        EFLASH_SetStartAddressReg(addr);
        /* Configure length (unit is 64-bit)*/
        printf("len=%d\r\n", len);
        EFLASH_SetLengthReg(len / 2U);
        /* Configure command */
        printf("cmd=0x%x\r\n", (uint32_t)EFLASH_CMD_PAGEPROGRAM);
        EFLASH_SetCommandReg((uint32_t)EFLASH_CMD_PAGEPROGRAM);

        printf("data0=0x%x, data1=0x%x\r\n", buffer[0], buffer[1]);
        EFLASH_SetDataReg0(buffer[0]);
        EFLASH_SetDataReg1(buffer[1]);

        /* Disable irq, avoid read Flash between Clear Cache and Program Flash. */
        DisableInterrupts;
        /* Clear PFlash Cache. */
        EFLASH_ClearPCache();
        /* Clear DFlash Cache. */
        EFLASH_ClearDCache();

        /* Trigger to start */
        printf("TrigCtrlCmdReg\r\n");
        EFLASH_TrigCtrlCmdReg();
        EnableInterrupts;

        /*program data */
        for (dataCount = 2U; dataCount < len; )
        {
            printf("data buf status: 0x%x, status: 0x%x\r\n", EFLASH_GetDataBufferReg(), EFLASH_GetStatusReg());
            /* Check data buff ready and error */
            if ((EFLASH_GetDataBufferReg() != 0U) && (EFLASH_GetStatusReg() == 0U))
            {
                EFLASH_SetDataReg0(buffer[dataCount]);
                dataCount++;
                EFLASH_SetDataReg1(buffer[dataCount]);
                dataCount++;
            }
            else if ((EFLASH_GetStatusReg() != 0U) || (EFLASH_GetLVDWarningReg() != 0U))
            {
                break;
            }
            else
            {
                for (delayI = 6U; delayI > 0U; )
                {
                    delayI--; /* for delay. */
                }
            }
        }
        /* Check whether the command is finished */
        while ((0U != EFLASH_GetCmdBusyStatus()) && (0U == EFLASH_GetLVDWarningReg()))
        {
            printf("1111data buf status: 0x%x, status: 0x%x\r\n", EFLASH_GetCmdBusyStatus(), EFLASH_GetLVDWarningReg());
            tempDelay = FLASH_GET_BUSY_STATUS_DELAY;
            while (tempDelay > 0U)
            {
                tempDelay--; /* for delay. */
            }
        }
        if (0U != EFLASH_GetStatusReg())
        {
            printf("status: 0x%x\r\n", EFLASH_GetStatusReg());
            statusRes = STATUS_ERROR;
        }
    }

    return statusRes;
}

/*!
 * @brief User page program.
 *
 * @param[in] addr: eflash addrees to be programed (should be 8-aligned)
 * @param[in] buffer: Point to the data to be programed
 * @param[in] len: Data length to be programed (the unit is 32-bit, should be 2-aligned)
 * @return EFLASH STATUS: eflash operation status
 */
status_t EFLASH_PageProgram(uint32_t addr, uint32_t *buffer, uint32_t len)
{
    status_t statusRes = STATUS_SUCCESS;
    uint32_t dataLength1 = 0U;
    uint32_t dataLength2 = 0U;
    uint32_t addrTemp = 0U;
    uint16_t pageNum = 0U;
    uint16_t pageSize = (uint16_t)PFLASH_PAGE_SIZE;
    uint16_t pageLen = 0U;

    /* check eflash address */
    DEVICE_ASSERT(((PFLASH_BASE_ADDRESS <= addr) && (PFLASH_END_ADDRESS > addr)) \
                  || ((DFLASH_BASE_ADDRESS <= addr) && (DFLASH_END_ADDRESS > addr)) \
                  || ((DINFO_BASE <= addr) && (DINFO_END > addr)));
    DEVICE_ASSERT((len != 0U) && (len % 2 == 0));
    DEVICE_ASSERT(NULL != buffer);

    if((addr >= PFLASH_BASE_ADDRESS) && (addr < PFLASH_END_ADDRESS) && ((addr + (len * 4U)) <= PFLASH_END_ADDRESS))
    {
        pageSize = (uint16_t)PFLASH_PAGE_SIZE;
    }
    else if((addr >= DFLASH_BASE_ADDRESS) && (addr < DFLASH_END_ADDRESS) && ((addr + (len * 4U)) <= DFLASH_END_ADDRESS))
    {
        pageSize = (uint16_t)DFLASH_PAGE_SIZE;
    }
    else if((addr >= DINFO_BASE) && (addr < DINFO_END) && ((addr + (len * 4U)) <= DINFO_END))
    {
        pageSize = (uint16_t)DFLASH_PAGE_SIZE;
    }
    else
    {
        statusRes = STATUS_ERROR;
        return statusRes;
    }

    if(((addr % pageSize) + (len * 4U)) > pageSize)
    {
        dataLength2 = ((addr + (len * 4U)) % pageSize) / 4U;
        dataLength1 = (pageSize - (addr % pageSize)) / 4U;
        statusRes = EFLASH_PageProgramUnit(addr, buffer, dataLength1, 0U);

        if (STATUS_SUCCESS == statusRes)
        {
            uint16_t i = 0;
            pageLen = pageSize / 4U;
            pageNum = (uint16_t)((len - dataLength1) / pageLen);
            addrTemp = addr +(pageSize - (addr % pageSize));

            for (i = 0U; i < pageNum; i++)
            {
                statusRes = EFLASH_PageProgramUnit(addrTemp + ((uint32_t)pageSize * i), buffer, (uint32_t)pageLen, dataLength1 + ((uint32_t)pageLen * i));
                if (STATUS_SUCCESS != statusRes)
                {
                    break;
                }
            }
            if (dataLength2 != 0U)
            {
                statusRes = EFLASH_PageProgramUnit(addrTemp + ((uint32_t)pageSize * i), buffer, dataLength2, dataLength1 + ((uint32_t)pageLen * i));
            }
        }
    }
    else
    {
        statusRes = EFLASH_PageProgramUnit(addr, buffer, len, 0U);
    }
    return statusRes;
}

/*!
 * @brief Verify whether the page erase operation is performed successfully.
 *
 * @param[in] addr: eflash addrees to be verified (should be 8-aligned)
 * @param[in] len: Specified page erase verify length(the unit is 32-bit, should be 2-aligned)
 * @return EFLASH STATUS: eflash operation status
 */
status_t EFLASH_PageEraseVerify(uint32_t addr, uint32_t len)
{
    status_t statusRes;

    /* check eflash address */
    DEVICE_ASSERT(((PFLASH_BASE_ADDRESS <= addr) && (PFLASH_END_ADDRESS > addr)) \
                  || ((DFLASH_BASE_ADDRESS <= addr) && (DFLASH_END_ADDRESS > addr)));
    DEVICE_ASSERT((len != 0U) && (len % 2U == 0U));

    /* Wait for last operation to be completed */
    if (0U != EFLASH_GetCmdBusyStatus())
    {
        statusRes = STATUS_BUSY;
    }
    else
    {
        EFLASH_CLEAR_STAT_ERROR_BITS();
        /* Configure start address */
        EFLASH_SetStartAddressReg(addr);

        /* Configure command and CMD_ST goes back to 0 */
        EFLASH_SetCommandReg((uint32_t)EFLASH_CMD_PAGERASEVERIFY);
        /* Configure length (unit is 64-bit)*/
        EFLASH_SetLengthReg(len / 2U);

        /* Trigger to start */
        statusRes = FLASH_StartCmdSequence();
    }

    return statusRes;
}

/*!
 * @brief Verify whether the mass erase operation is performed successfully.
 *
 * @param[in] addr: Specified eflash addrees to mass erase verify
 *            - P-Flash address: Mass erase verify P-Flash
 *            - D-Flash address: Mass erase verify D-Flash
 * @return EFLASH STATUS: eflash operation status
 */
status_t EFLASH_MassEraseVerify(uint32_t addr)
{
    status_t statusRes;

    /* check eflash address */
    DEVICE_ASSERT(((PFLASH_BASE_ADDRESS <= addr) && (PFLASH_END_ADDRESS > addr)) \
                  || ((DFLASH_BASE_ADDRESS <= addr) && (DFLASH_END_ADDRESS > addr)));

    /* Wait for last operation to be completed */
    if (0U != EFLASH_GetCmdBusyStatus())
    {
        statusRes = STATUS_BUSY;
    }
    else
    {
        EFLASH_CLEAR_STAT_ERROR_BITS();

        /* Configure start address */
        EFLASH_SetStartAddressReg(addr);

        /* Configure command and CMD_ST goes back to 0 */
        EFLASH_SetCommandReg((uint32_t)EFLASH_CMD_MASSRASEVERIFY);

        /* Trigger to start */
        statusRes = FLASH_StartCmdSequence();
    }

    return statusRes;
}

/*!
 * @brief Option page erase.
 *
 * @param[in] addr: option addrees to erase (should be 8-aligned)
 * @return EFLASH STATUS: eflash operation status
 */
static status_t EFLASH_OptionPageErase(uint32_t addr)
{
    status_t statusRes;

    /* Check option byte address */
    DEVICE_ASSERT(OPTION_BASE <= addr);
    DEVICE_ASSERT(OPTION_END > addr);

    /* Wait for last operation to be completed */
    if (0U != EFLASH_GetCmdBusyStatus())
    {
        statusRes = STATUS_BUSY;
    }
    else if (EFLASH_GetLVDWarningReg() != 0U)
    {
        statusRes = STATUS_ERROR;
    }
    else
    {
        EFLASH_CLEAR_STAT_ERROR_BITS();
        /* Configure start address */
        EFLASH_SetStartAddressReg(addr);
        /* Configure command */
        EFLASH_SetCommandReg((uint32_t)EFLASH_CMD_PROTECTERASE);
        /* Enable option byte area operation */
        EFLASH_EnableOptionReg(ENABLE);
        /* Trigger to start */
        statusRes = FLASH_StartCmdSequence();
    }
    /* Disable option byte area operation */
    EFLASH_EnableOptionReg(DISABLE);

    return statusRes;
}

/*!
 * @brief Programs data to specified option page address.
 *
 * @param[in] addr: option addrees (should be 8-aligned)
 * @param[in] buffer: Point to the data bufffer to be programed
 * @param[in] len: Data length to be programed (the unit is 32-bit, should be 2-aligned)
 * @return EFLASH STATUS: eflash operation status
 */
static status_t EFLASH_OptionPageProgram(uint32_t addr, uint32_t *buffer, uint32_t len)
{
    status_t statusRes = STATUS_SUCCESS;
    uint32_t dataCount = 0U;
    volatile uint32_t tempDelay;
    volatile uint8_t delayI;

    /* check option byte address */
    DEVICE_ASSERT(OPTION_BASE <= addr);
    DEVICE_ASSERT(OPTION_END > addr);
    DEVICE_ASSERT(NULL != buffer);
    DEVICE_ASSERT((len != 0U) && (len % 2U == 0U));

    /* Wait for last operation to be completed */
    if (0U != EFLASH_GetCmdBusyStatus())
    {
        statusRes = STATUS_BUSY;
    }
    else if (EFLASH_GetLVDWarningReg() != 0U)
    {
        statusRes = STATUS_ERROR;
    }
    else
    {
        EFLASH_CLEAR_STAT_ERROR_BITS();
        /* Configure start address */
        EFLASH_SetStartAddressReg(addr);
        /* Configure length (unit is 64-bit) */
        EFLASH_SetLengthReg(len / 2U);
        /* Configure command */
        EFLASH_SetCommandReg((uint32_t)EFLASH_CMD_PROTECTROGRAM);
        /* Enable option byte area operation */
        EFLASH_EnableOptionReg(ENABLE);
        /* Trigger to start */
        EFLASH_TrigCtrlCmdReg();

        /* User program data */
        while (dataCount < len)
        {
            if ((EFLASH_GetDataBufferReg() != 0U) && (EFLASH_GetStatusReg() == 0U))
            {
                EFLASH_SetDataReg0(buffer[dataCount]);
                dataCount++;
                EFLASH_SetDataReg1(buffer[dataCount]);
                dataCount++;
            }
            else if ((EFLASH_GetStatusReg() != 0U) || (EFLASH_GetLVDWarningReg() != 0U))
            {
                break;
            }
            else
            {
                for (delayI = 6U; delayI > 0U; )
                {
                    delayI--; /* for delay. */
                }
            }
        }

        /* Check whether the command is finished */
        while ((0U != EFLASH_GetCmdBusyStatus()) && (0U == EFLASH_GetLVDWarningReg()))
        {
            tempDelay = FLASH_GET_BUSY_STATUS_DELAY;
            while (tempDelay > 0U)
            {
                tempDelay--; /* for delay. */
            }
        }
        if (0U != EFLASH_GetStatusReg())
        {
            statusRes = STATUS_ERROR;
        }
    }
    /* Disable option byte area operation */
    EFLASH_EnableOptionReg(DISABLE);

    return statusRes;
}

/*!
 * @brief Enable EFLASH read protection.
 *
 * This API is used to set(enable) EFLASH read protection. Set(enable) read
 * protection will take effect after reset and the all EFLASH main memory(user
 * pages) will return with 0xAAAAAAAA when read by JTAG/SWD. EFLASH main memory
 * can not be erased or programed by code/JTAG/SWD when in read protection status.
 *
 * @param[in] none
 * @return EFLASH STATUS: eflash operation status
 */
status_t EFLASH_EnableReadProtection(void)
{
    status_t statusRes;
    uint32_t optBuffer[PFLASH_READ_PROTECT_NUM] = {0U};

    EFLASH_CLEAR_STAT_ERROR_BITS();
    statusRes = EFLASH_OptionPageErase(CINFO_BASE);
    if (STATUS_SUCCESS != statusRes)
    {
        statusRes = STATUS_ERROR;
    }
    else
    {
        optBuffer[0] = 0x5AA55AA5U;
        optBuffer[1] = 0xFFFFFFFFU;
        statusRes = EFLASH_OptionPageProgram(CINFO_BASE, optBuffer, PFLASH_READ_PROTECT_NUM);
    }

    return statusRes;
}

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
status_t EFLASH_DisableReadProtection(void)
{
    status_t statusRes;
    uint32_t optBuffer[PFLASH_READ_PROTECT_NUM] = {0U};

    EFLASH_CLEAR_STAT_ERROR_BITS();
    statusRes = EFLASH_OptionPageErase(CINFO_BASE);
    if (STATUS_SUCCESS != statusRes)
    {
        statusRes = STATUS_ERROR;
    }
    else
    {
        optBuffer[0] = 0xFF53FFACU;//0xFFFFFFACU
        optBuffer[1] = 0xFFFFFFFFU;
        statusRes = EFLASH_OptionPageProgram(CINFO_BASE, optBuffer, PFLASH_READ_PROTECT_NUM);
    }

    return statusRes;
}

/*!
 * @brief Set P-Flash write protection by option byte.
 *
 * @param[in] protectStatus: write protection status value
 * @return EFLASH STATUS: eflash operation status
 */
status_t EFLASH_PFSetWriteProtection(uint32_t *protectStatus)
{
    status_t statusRes;
    uint32_t optBuffer[OPT_PAGE1_VALID_NUM] = {0U};

    DEVICE_ASSERT(NULL != protectStatus);

    EFLASH_Read(PFLASH_WRITE_PROTECT_BASE, optBuffer, OPT_PAGE1_VALID_NUM);
    EFLASH_CLEAR_STAT_ERROR_BITS();
    statusRes = EFLASH_OptionPageErase(PFLASH_WRITE_PROTECT_BASE);
    if (STATUS_SUCCESS == statusRes)
    {
        optBuffer[0] = protectStatus[0];
        optBuffer[1] = ~protectStatus[0];
        optBuffer[2] = protectStatus[1];
        optBuffer[3] = ~protectStatus[1];
        statusRes = EFLASH_OptionPageProgram(PFLASH_WRITE_PROTECT_BASE, optBuffer, OPT_PAGE1_VALID_NUM);
        if (STATUS_SUCCESS != statusRes)
        {
            statusRes = STATUS_ERROR;
        }
    }
    else
    {
        statusRes = STATUS_ERROR;
    }

    return statusRes;
}

/*!
 * @brief Set D-Flash write protection by option byte.
 *
 * @param[in] protectStatus: write protection status value
 * @return EFLASH STATUS: eflash operation status
 */
status_t EFLASH_DFSetWriteProtection(uint32_t *protectStatus)
{
    status_t statusRes;
    uint32_t optBuffer[OPT_PAGE1_VALID_NUM] = {0U};

    DEVICE_ASSERT(NULL != protectStatus);

    EFLASH_Read(DFLASH_WRITE_PROTECT_BASE, optBuffer, OPT_PAGE1_VALID_NUM);
    EFLASH_CLEAR_STAT_ERROR_BITS();
    statusRes = EFLASH_OptionPageErase(DFLASH_WRITE_PROTECT_BASE);
    if (STATUS_SUCCESS == statusRes)
    {
        optBuffer[4] = protectStatus[0];
        optBuffer[5] = ~protectStatus[0];
        optBuffer[6] = protectStatus[1];
        optBuffer[7] = ~protectStatus[1];
        statusRes = EFLASH_OptionPageProgram(DFLASH_WRITE_PROTECT_BASE, optBuffer, OPT_PAGE1_VALID_NUM);
        if (STATUS_SUCCESS != statusRes)
        {
            statusRes = STATUS_ERROR;
        }
    }
    else
    {
        statusRes = STATUS_ERROR;
    }

    return statusRes;
}

/*!
 * @brief Set P&D-Flash write protection by register.
 *
 * @param[in] sel: P&D-Flash select
              - 0: P-Flash
              - 1: D-Flash
 * @param[in] protectStatusLow: write protection status value for page 0~31
 * @param[in] protectStatusHigh: write protection status value for page 32~64
 * @return EFLASH STATUS: eflash operation status
 */
status_t EFLASH_SetWriteProtectionByReg(uint8_t sel, uint32_t protectStatusLow, uint32_t protectStatusHigh)
{
    status_t statusRes = STATUS_SUCCESS;

    /* Write to protection register */
    if (0U == sel)
    {
        EFLASH_SetPWriteProRegHigh(protectStatusHigh);
        EFLASH_SetPWriteProRegLow(protectStatusLow);
        if ((protectStatusLow != EFLASH_GetPWriteProRegLow()) || (protectStatusHigh != EFLASH_GetPWriteProRegHigh()))
        {
            statusRes = STATUS_ERROR;
        }
    }
    else
    {
        EFLASH_SetDWriteProRegHigh(protectStatusHigh);
        EFLASH_SetDWriteProRegLow(protectStatusLow);
        if ((protectStatusLow != EFLASH_GetDWriteProRegLow()) || (protectStatusHigh != EFLASH_GetDWriteProRegHigh()))
        {
            statusRes = STATUS_ERROR;
        }
    }

    return statusRes;
}

/*!
 * @brief Set external reset enable or disable.
 *
 * @param[in] state: external reset status
 * @return EFLASH STATUS: eflash operation status
 */
status_t EFLASH_SetExReset(ACTION_Type state)
{
    status_t statusRes;
    uint32_t optBuffer[OPT_PAGE1_VALID_NUM] = {0U};

    EFLASH_Read(PFLASH_WRITE_PROTECT_BASE, optBuffer, OPT_PAGE1_VALID_NUM);
    EFLASH_CLEAR_STAT_ERROR_BITS();
    statusRes = EFLASH_PageErase(PFLASH_WRITE_PROTECT_BASE);
    if (STATUS_SUCCESS == statusRes)
    {
        optBuffer[8] = (ENABLE == state) ? 0xFFFFFFFFU : 0xFFFFFFF0U;
        optBuffer[9] = 0xFFFFFFFFU;
        statusRes = EFLASH_OptionPageProgram(PFLASH_WRITE_PROTECT_BASE, optBuffer, OPT_PAGE1_VALID_NUM);
        if (STATUS_SUCCESS != statusRes)
        {
            statusRes = STATUS_ERROR;
        }
    }
    else
    {
        statusRes = STATUS_ERROR;
    }

    return statusRes;
}

/*!
 * @brief Get module version information.
 *
 * @param[out] versionInfo: Module version information address.
 * @return void
 */
void EFLASH_GetVersionInfo(VersionInfo_Type *versionInfo)
{
    DEVICE_ASSERT(versionInfo != NULL);

    versionInfo->moduleID = EFLASH_HAL_MODULE_ID;
    versionInfo->majorVersion = EFLASH_HAL_SW_MAJOR_VERSION;
    versionInfo->minorVersion = EFLASH_HAL_SW_MINOR_VERSION;
    versionInfo->patchVersion = EFLASH_HAL_SW_PATCH_VERSION;
}

/* =============================================  EOF  ============================================== */
