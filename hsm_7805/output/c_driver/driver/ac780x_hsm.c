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
 * @file ac780x_hsm.c
 *
 * @brief This file provides HSM integration functions.
 *
 */

/* ===========================================  Includes  =========================================== */
#include <string.h>
#include "ac780x_eflash_reg.h"
#ifdef DMA_ENABLE
#include "ac780x_dma_reg.h"
#endif
#include "ac780x_hsm.h"

/* ============================================  Define  ============================================ */
#ifndef HSM_CMAC_UNALIGN_SUPPORT
/**
  * @brief If 'HSM_CMAC_UNALIGN_SUPPORT' is set, than all cmac and cmac-related(bl & fw verify) function
  *        can input a package which length maybe not 4bytes align.
  *        If 'HSM_SOFT_AUTH_ENABLE' set to 0, all input length must be 4bytes aligned.
  */
#define HSM_CMAC_UNALIGN_SUPPORT   (1)
#endif

#define HSM_CMD_TIMEOUT            (0x10000U)   /*!< Write/read timeout */
#define HSM_AUTHENTICATION_LEN     (16U)        /*!< Authentication byte length */
#define HSM_WORDSIZE_BYTES         (4U)         /*!< WordSize Bytes length */
#define HSM_FLASH_KEY_BASE         (0x08300800U)/*!< Flash Address to save customer key */
#define HSM_KEY_PAGE_NUMS          (2U)         /*!< Global page numbers */
#define HSM_KEY_NUMS_ONE_PAGE      (32U)        /*!< Key numbers in one page */
#define HSM_KEY_SLOT_LEN           (32U)        /*!< Size of bytes for one key slot */
#define HSM_PAGE_VALID_LEN         (8U)         /*!< Size of bytes for page valid flag */
#define HSM_PAGE_RESERVE_LEN       (DFLASH_PAGE_SIZE - (HSM_KEY_NUMS_ONE_PAGE * HSM_KEY_SLOT_LEN) - HSM_PAGE_VALID_LEN)

#define HSM_PAGE_VALID_TAG         (0xA5U)      /*!< Page valid tag */
#define HSM_SLOT_INVALID_TAG       (0xA5A5U)    /*!< Slot invalid tag */
#define HSM_KEY_DATA_VALID_TAG     (0x52U)      /*!< Key data valid */
#define BLOCKSIZE                  (16)
/* ===========================================  Typedef  ============================================ */
/**
  * @brief Flash Key Slot: "KeyInfo0" + "KeyInfo1" + 128bit KeyData
  */
/*!< HSM Flash Key structure definitions */
typedef struct
{
    uint64_t slotInValidFlag : 16;/*!< slot valid flag: 0xA5A5 measn invalid, others depends on 'dataValidFlag' */
    uint64_t reservedValue : 48;  /*!< Reserve */
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
    HSM_KeyInfo0 keyInfo0;       /*!< Struct HSM_KeyInfo0 */
    HSM_KeyInfo1 keyInfo1;       /*!< Struct HSM_KeyInfo1 */
    uint8_t keyData[HSM_CRYPTO_BLOCK_BYTE]; /*!< Key Data Content in 16bytes */
} HSM_FlashKey;

/**
  * @brief There are only 32 key slot exist in one key page.
  */
/*!< HSM Flash Key structure definitions */
typedef struct
{
    HSM_FlashKey userKeyp[HSM_KEY_NUMS_ONE_PAGE];  /*!< Key slot */
    uint8_t pageReserve[HSM_PAGE_RESERVE_LEN];     /*!< Reserve */
    uint8_t pageValidFlag[HSM_PAGE_VALID_LEN];     /*!< page valid flag: 0xa5 * 8 means valid, other means invalid */
} HSM_FlashKeyPage;

/* =====================================  Function Declaration  ===================================== */
static void HSM_DMACallback(void *device, uint32_t dmaStatus, uint32_t param);
static status_t HSM_CryptoFifoHandle(HSM_CryptoType cryptoType, uint32_t *in, uint32_t len, uint32_t *out);

/* ==========================================  Variables  =========================================== */
/* A temporary buffer used in CMAC-verification */
static uint32_t s_hsmTempMacBuf[HSM_CRYPTO_BLOCK_BYTE / sizeof(uint32_t)];
/* Record Input Mac buffer address */
static uint32_t *s_hsmInputMacBuf;
/* Customer DMA Callback pointer */
static DeviceCallback_Type s_hsmDmaCallback;
/* Mark if it's the last operations for split-CMAC, if it was set TRUE we will delete s_hsmDmaCallback after DMA done */
static BOOL_Type s_hsmDmaLastOps;
static BOOL_Type s_hsmAesDmaLastOps;
/*Mark the user use bl_verify_key generate/verify CMAC when Async Ops*/
static BOOL_Type s_hsmAesCmacByBlKeyId;
#if HSM_CMAC_UNALIGN_SUPPORT
/* In CMAC-DMA mode, we record the last few bytes for each update ops if inputlen is not 4bytes align */
static BOOL_Type s_hsmCmacInputAlign;
static uint32_t *s_hsmDMAUnalignOutBuf;
static uint32_t s_hsmCmacUnalignBytes;
#endif
/* customer isr callback */
static DeviceCallback_Type s_hsmISRCallback;
#ifdef DMA_ENABLE
static DMA_ConfigType s_hsmDMAConfig =
{
    .channelEn = ENABLE,
    .channelPriority = DMA_PRIORITY_VERY_HIGH,
    .memIncrement = ENABLE,
    .periphIncrement = DISABLE,
    .memSize = DMA_MEM_SIZE_32BIT,
    .periphSize = DMA_PERIPH_SIZE_32BIT,
    .circular = DISABLE,
    .MEM2MEM = DISABLE,
    .memByteMode = DMA_MEM_BYTE_MODE_1TIME,
    .finishInterruptEn = ENABLE,
    .halfFinishInterruptEn = DISABLE,
    .errorInterruptEn = ENABLE,
    .callBack = HSM_DMACallback,
};
#endif

/* ======================================  Static Functions define  ======================================== */
static uint8_t s_hsmAesKeySchedule[176]; // AES key schedule
/* S-box */
static const uint8_t sBox[256] =
{
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5,
    0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0,
    0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc,
    0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a,
    0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
    0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b,
    0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85,
    0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
    0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17,
    0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88,
    0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c,
    0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9,
    0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6,
    0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
    0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94,
    0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68,
    0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

static const uint8_t rCon[44] =
{
    0x0, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00,
    0x10, 0x00, 0x00, 0x00,
    0x20, 0x00, 0x00, 0x00,
    0x40, 0x00, 0x00, 0x00,
    0x80, 0x00, 0x00, 0x00,
    0x1b, 0x00, 0x00, 0x00,
    0x36, 0x00, 0x00, 0x00
};

static const uint8_t matmul[4][4] =
{
    {0x02, 0x03, 0x01, 0x01},
    {0x01, 0x02, 0x03, 0x01},
    {0x01, 0x01, 0x02, 0x03},
    {0x03, 0x01, 0x01, 0x02}
};

static const uint8_t constZero[16] =
{
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

static const uint8_t constRb[16] =
{
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x87
};

static void HSM_AddRoundKey(uint8_t state[4][4], uint8_t *key)
{
    uint8_t row, col;
    for (row = 0; row < 4; row++)
    {
        for (col = 0; col < 4; col++)
        {
            state[col][row] ^= key[col * 4 + row];
        }
    }
}

static void HSM_BlockXor(uint8_t *dst, uint8_t *a, uint8_t *b)
{
    uint8_t j;
    for (j = 0; j < 16; j++)
    {
        dst[j] = a[j] ^ b[j];
    }
}

static uint8_t *HSM_SubWord(uint8_t *word)
{
    uint8_t i;
    for (i = 0; i < 4; i++)
    {
        word[i] = sBox[word[i]];
    }

    return word;
}

static uint8_t *HSM_RotWord(uint8_t *word)
{
    uint8_t tmp[4];
    uint8_t i;

    memcpy(tmp, word, 4);

    for (i = 0; i < 4; i++)
    {
        word[i] = tmp[(i + 1) % 4];
    }

    return word;
}

static void HSM_SubBytes(uint8_t state[4][4])
{
    uint8_t row, col;
    for (row = 0; row < 4; row++)
    {
        for (col = 0; col < 4; col++)
        {
            state[col][row] = sBox[state[col][row]];
        }
    }
}

static void HSM_ShiftRows(uint8_t state[4][4])
{
    uint8_t tmp[4];
    uint8_t row, col;
    for (row = 1; row < 4; row++)
    {
        for (col = 0; col < 4; col++)
        {
            tmp[col] = state[(row + col) % 4][row];
        }

        for (col = 0; col < 4; col++)
        {
            state[col][row] = tmp[col];
        }
    }
}

static uint8_t HSM_Mul(uint8_t a, uint8_t b)
{
    uint8_t sb[4];
    uint8_t out = 0;
    uint8_t i;
    sb[0] = b;
    for (i = 1; i < 4; i++)
    {
        sb[i] = sb[i - 1] << 1;
        if (sb[i - 1] & 0x80)
        {
            sb[i] ^= 0x1b;
        }
    }

    for (i = 0; i < 4; i++)
    {
        if (a >> i & 0x01)
        {
            out ^= sb[i];
        }
    }

    return out;
}

static void HSM_MixColumns(uint8_t state[4][4])
{
    uint8_t tmp[4];
    uint8_t row, col, i, j;
    for (col = 0; col < 4; col++)
    {
        for (row = 0; row < 4; row++)
        {
            tmp[row] = state[col][row];
        }

        for (i = 0; i < 4; i++)
        {
            state[col][i] = 0x00;
            for (j= 0; j < 4; j++)
            {
                state[col][i] ^= HSM_Mul(matmul[i][j], tmp[j]);
            }
        }
    }
}

static void HSM_Cipher(uint8_t *in, uint8_t *out, uint8_t *w, uint8_t nk, uint8_t nr)
{
    uint8_t state[4][4];
    uint8_t round;

    memcpy(state, in, 4 *nk);

    HSM_AddRoundKey(state, w);
    for (round = 0; round < nr; round++)
    {
        HSM_SubBytes(state);
        HSM_ShiftRows(state);
        if (round != (nr - 1))
        {
            HSM_MixColumns(state);
        }
        HSM_AddRoundKey(state, (uint8_t *)(w + (round + 1) *16));
    }
    memcpy(out, state, nk *4);
}

static void HSM_KeyExpansion(uint8_t *key, uint8_t *w, uint8_t nk, uint8_t nr)
{
    static uint8_t tmp[4];
    uint8_t i, j;
    memcpy(w, key, 4 *nk);

    for (i = 4 *nk; i < 4 *(nr + 1) *4; i += 4)
    {
        memcpy(tmp, w + i - 4, 4);

        if (i % (nk *4) == 0)
        {
            HSM_SubWord(HSM_RotWord(tmp));
            for (j = 0; j < 4; j++)
            {
                tmp[j] ^= rCon[i / nk + j];
            }
        }
        else if (nk > 6 && (i % (nk *4)) == 16)
        {
            HSM_SubWord(tmp);
        }

        for (j = 0; j < 4; j++)
        {
            w[i + j] = w[i - nk *4 + j] ^ tmp[j];
        }
    }
}

static void HSM_BlockLeftShift(uint8_t *dst, uint8_t *src)
{
    uint8_t ovf = 0x00;
    int i;
    for (i = 15; i >= 0; i--)
    {
        dst[i] = src[i] << 1;
        dst[i] |= ovf;
        ovf = (src[i] & 0x80) ? 1 : 0;
    }
}

static void HSM_SoftWareAesEncrypt(uint8_t *inBuf, uint8_t *outBuf, uint8_t *key)
{
    uint8_t nk = 4, nr = 10;

    HSM_KeyExpansion(key, s_hsmAesKeySchedule, nk, nr);
    HSM_Cipher(inBuf, outBuf, s_hsmAesKeySchedule, nk, nr);
}

static void HSM_GenerateSubkey(uint8_t *key, uint8_t *k1, uint8_t *k2)
{
    uint8_t L[16];

    HSM_SoftWareAesEncrypt((uint8_t *)constZero, L, key);
    HSM_BlockLeftShift(k1, L);

    if (L[0] & 0x80)
    {
        HSM_BlockXor(k1, k1, (uint8_t *)constRb);
    }

    HSM_BlockLeftShift(k2, k1);

    if (k1[0] & 0x80)
    {
        HSM_BlockXor(k2, k2, (uint8_t *)constRb);
    }
}

/*!
 * @brief generate mac by software.
 *
 * @param[in] inBuf : message buf pointer.
 * @param[in] length : message buf length, must 16bytes align.
 * @param[in] outBuf : mac buffer pointer.
 * @param[in] key : key buf addres.
 * @return none
 */
uint32_t HSM_SoftWareGenerateCmac(uint8_t *inBuf, uint32_t length, uint8_t *outBuf, uint8_t *key)
{
    uint8_t k1[16] = {0};
    uint8_t k2[16] = {0};
    uint8_t x[16] = {0};  // Initialize to 0
    uint8_t y[16];
    uint8_t block[16];
    uint8_t n;
    uint8_t i;

    if (length == 0)
    {
        return STATUS_ERROR;
    }

    if (length % BLOCKSIZE != 0 || (length > 128))
    {
        return STATUS_ERROR;
    }

    HSM_GenerateSubkey(key, k1, k2);

    n = (length / BLOCKSIZE);

    for (i = 0; i < n - 1; i++)
    {
        memcpy(block, inBuf + i * BLOCKSIZE, BLOCKSIZE);
        HSM_BlockXor(y, block, x);
        HSM_SoftWareAesEncrypt(y, x, key);
    }

    memcpy(block, inBuf + (n - 1) * BLOCKSIZE, BLOCKSIZE);
    HSM_BlockXor(block, block, k1);
    HSM_BlockXor(y, block, x);

    HSM_SoftWareAesEncrypt(y, outBuf, key);

    return STATUS_SUCCESS;
}

/*!
 * @brief HSM get command status.
 *
 * @return status_t
 */
static status_t HSM_ParseCmdStatus(void)
{
    status_t result = STATUS_SUCCESS;
    uint16_t status = HSM_GetCmdStatus();

    if (0U == status)
    {
        return STATUS_SUCCESS;
    }

    if (0U != (status & HSM_STAT_STAT0_Msk))
    {
        result = STATUS_HSM_FLASH_KEY_INVALID;
    }
    else if (0U != (status & (HSM_STAT_STAT1_Msk | HSM_STAT_STAT2_Msk)))
    {
        result = STATUS_HSM_CIPHER_LEN_INVALID;
    }
    else if (0U != (status & (HSM_STAT_STAT3_Msk | HSM_STAT_STAT4_Msk | HSM_STAT_STAT5_Msk)))
    {
        result = STATUS_HSM_CIPHER_KEY_INVALID;
    }
    else if (0U != (status & (HSM_STAT_STAT6_Msk | HSM_STAT_STAT7_Msk)))
    {
        result = STATUS_HSM_CMAC_KEY_INVALID;
    }
    else if (0U != (status & HSM_STAT_STAT8_Msk))
    {
        result = STATUS_HSM_AUTH_KEY_INVALID;
    }
    else if ((0U != (status & HSM_STAT_STAT9_Msk)) || (0U != (status & HSM_STAT_STAT10_Msk)))
    {
        result = STATUS_HSM_RANDOM_LEN_INVALID;
    }
    else
    {
        /* Do Nothing */
    }

    return result;
}

/*!
 * @brief HSM set iv.
 *
 * @param[out] The iv pointer
 * @return none
 */
static void HSM_SetIv(uint32_t *iv)
{
    HSM_SetIV0(iv[0]);
    HSM_SetIV1(iv[1]);
    HSM_SetIV2(iv[2]);
    HSM_SetIV3(iv[3]);
}

/*!
 * @brief Set HSM authentication response.
 *
 * @param[in] response: The pointer if authentication response
 * @return none
 */
static void HSM_SetAuthResponse(uint32_t *response)
{
    HSM_SetResponse0(response[0]);
    HSM_SetResponse1(response[1]);
    HSM_SetResponse2(response[2]);
    HSM_SetResponse3(response[3]);
}

/*!
 * @brief Check CMAC result in DMA mode.
 *
 * @return status_t
 */
static status_t HSM_CmacResultCheck(void)
{
    status_t status = STATUS_SUCCESS;
    uint8_t keyId;

    /* If in CMAC-Verify mode, we should compare result */
    keyId = HSM_GetKeyId();
    if ((keyId == (uint8_t)HSM_BL_VERIFY_KEY_ID) && (s_hsmAesCmacByBlKeyId == FALSE))
    {
        if (0U == HSM_GetBlVerifyPassFlag())
        {
            status = STATUS_HSM_AUTH_FAIL;
        }
    }
    else if (keyId == (uint8_t)HSM_FW_UPGRADE_KEY_ID)
    {
        if (0U == HSM_GetFwUpVerifyPassFlag())
        {
            status = STATUS_HSM_AUTH_FAIL;
        }
    }
    else if ((keyId >= (uint8_t)HSM_FLASH_KEY_START_ID) && (keyId <= (uint8_t)HSM_RAM_KEY_ID))
    {
        if (HSM_GetEncDec() == (uint8_t)HSM_VERIFY)
        {
            if (memcmp(s_hsmTempMacBuf, s_hsmInputMacBuf, HSM_CRYPTO_BLOCK_BYTE) != 0)
            {
                status = STATUS_HSM_AUTH_FAIL;
            }
            s_hsmInputMacBuf = NULL;
        }
    }
    else
    {
        if (keyId != HSM_BL_VERIFY_KEY_ID)
        {
            DEVICE_ASSERT(0);
        }
    }

    if (status == STATUS_SUCCESS)
    {
        status = HSM_ParseCmdStatus();
    }

    return status;
}

/*!
 * @brief Global DMA channel handler callback.
 *
 * @param[in] device: device struct pointer
 * @param[in] dmaStatus: current channel status
 * @param[in] param: not used param
 * @return none
 */
static void HSM_DMACallback(void *device, uint32_t dmaStatus, uint32_t param)
{
    status_t status = STATUS_SUCCESS;

    (void)device;
    (void)param;
    DEVICE_ASSERT(s_hsmDmaCallback != NULL);

#if HSM_CMAC_UNALIGN_SUPPORT
    /* if last package which handled by dma is not aligned, we should handle the remain bytes at first */
    if (FALSE == s_hsmCmacInputAlign && FALSE == s_hsmAesDmaLastOps)
    {
        status = HSM_CryptoFifoHandle(HSM_AES_CMAC, &s_hsmCmacUnalignBytes, HSM_WORDSIZE_BYTES, s_hsmDMAUnalignOutBuf);
        s_hsmCmacInputAlign = TRUE;
    }

    if (status == STATUS_SUCCESS)
    {
#endif
    if ((dmaStatus & DMA_CHANNEL_STATUS_TRANS_ERROR_Msk) != 0U)
    {
        status = STATUS_ERROR;
    }
    else
    {
        if (TRUE == s_hsmDmaLastOps && FALSE == s_hsmAesDmaLastOps)
        {
            status = HSM_CmacResultCheck();
        }
        else if (FALSE == s_hsmDmaLastOps && TRUE == s_hsmAesDmaLastOps)
        {
            status = HSM_ParseCmdStatus();
            s_hsmAesDmaLastOps = FALSE;
        }
    }

#if HSM_CMAC_UNALIGN_SUPPORT
    }
#endif
    s_hsmDmaCallback(HSM, status, (uint32_t)HSM_GetCmd());

    if ((TRUE == s_hsmDmaLastOps) || (status == STATUS_ERROR))
    {
        s_hsmDmaCallback = NULL;
    }

    HSM_EnableDma(DISABLE);
}

/*!
 * @brief DMA channel0 and channel1 init.
 *
 * @param[in] in: The pointer of crypt data
 * @param[in] inWordSize: The word size of crypto data
 * @param[out] out: The pointer of crypto result
 * @param[in] out: The word size of crypto result
 * @param[in] isFirst: if first package
 * @return ret: HSM operation status
 */
static void HSM_DMAInit(uint32_t *in, uint32_t inWordSize, uint32_t *out, uint32_t outWordSize, BOOL_Type isFirst)
{
#ifdef DMA_ENABLE
    if (NULL != out)
    {
        s_hsmDMAConfig.memStartAddr = (uint32_t)out;
        s_hsmDMAConfig.memEndAddr = s_hsmDMAConfig.memStartAddr + outWordSize * 4U + 1U;
        s_hsmDMAConfig.transferNum = outWordSize;
        s_hsmDMAConfig.periphStartAddr = (uint32_t)(&(HSM->CIPOUT));
        s_hsmDMAConfig.direction = DMA_READ_FROM_PERIPH;
        s_hsmDMAConfig.periphSelect = DMA_PEPIRH_HSM_TX;
        DMA_DeInit(DMA0_CHANNEL1);
        DMA_Init(DMA0_CHANNEL1, &s_hsmDMAConfig);
    }
    if (NULL != in)
    {
        s_hsmDMAConfig.memStartAddr = (uint32_t)in;
        s_hsmDMAConfig.memEndAddr = s_hsmDMAConfig.memStartAddr + inWordSize * 4U + 1U;

        s_hsmDMAConfig.transferNum = inWordSize;
        s_hsmDMAConfig.periphStartAddr = (uint32_t)(&(HSM->CIPIN));
        s_hsmDMAConfig.direction = DMA_READ_FROM_MEM;
        s_hsmDMAConfig.periphSelect = DMA_PEPIRH_HSM_RX;
        DMA_DeInit(DMA0_CHANNEL0);
        DMA_Init(DMA0_CHANNEL0, &s_hsmDMAConfig);
        if (NULL != out)
        {
            /* Only handle output callback */
            DMA_SetCallback(DMA0_CHANNEL0, NULL);
        }
    }

    if (TRUE == isFirst)
    {
        HSM_EnableDma(ENABLE);
    }
#endif
}

/*!
 * @brief Get HSM authentication challenge.
 *
 * @param[out] challenge: The pointer of authentication challenge
 * @return ret: HSM operation status
 */
static status_t HSM_GetChallenge(uint32_t *challenge)
{
    status_t ret = STATUS_SUCCESS;
    volatile uint32_t timeout = HSM_CMD_TIMEOUT;

    while ((0U == HSM_GetChallengeValidFlag()) && (timeout > 0U))
    {
        timeout--;
    }
    if (timeout > 0U)
    {
        challenge[0] = HSM_GetChallenge0();
        challenge[1] = HSM_GetChallenge1();
        challenge[2] = HSM_GetChallenge2();
        challenge[3] = HSM_GetChallenge3();
    }
    else
    {
        ret = STATUS_TIMEOUT;
    }

    return ret;
}

/*!
 * @brief Write HSM RX fifo and read data from HSM TX fifo.
 *
 * @param[in] cryptoType: The HSM Command Type
 * @param[in] in: The data pointer for writing to HSM RX fifo
 * @param[in] byteSize: The byte size
 * @param[out] out: The data pointer for reading from HSM TX fifo
 * @return ret: HSM operation status
 */
static status_t HSM_CryptoFifoHandle(HSM_CryptoType cryptoType, uint32_t *in, uint32_t len, uint32_t *out)
{
    status_t status;
    volatile uint32_t timeout;
    uint32_t blockSize;
    uint32_t blockInLen;
    uint32_t blockOutLen;
    uint32_t outSize;
    uint32_t inSize;
    uint32_t tempData;

    status = HSM_ParseCmdStatus();
    if (status != STATUS_SUCCESS)
    {
        s_hsmDmaCallback = NULL;
        return status;
    }

    switch (cryptoType)
    {
    case HSM_AES_ECB:
    case HSM_AES_CBC:
        inSize = len;
        outSize = inSize;
        blockInLen = HSM_CRYPTO_BLOCK_BYTE;
        blockOutLen = HSM_CRYPTO_BLOCK_BYTE;
        break;
    case HSM_GET_RND:
        inSize = 0;
        outSize = len;
        blockInLen = 0;
        blockOutLen = outSize;
        break;
    case HSM_AES_CMAC:
        inSize = len;
        outSize = HSM_CRYPTO_BLOCK_BYTE;
        blockInLen = inSize;
        blockOutLen = HSM_CRYPTO_BLOCK_BYTE;
        break;
    default:
        DEVICE_ASSERT(0);
        return STATUS_ERROR;
    }

    while (outSize > 0U)
    {
        timeout = HSM_CMD_TIMEOUT;

        if (NULL != in)
        {
            blockSize = blockInLen;
            /* Set input in block length */
            while (blockSize > 0U)
            {
                while ((0U != HSM_GetRxFiFoFullFlag()) && (timeout > 0U))
                {
                    timeout--;
                }

                if ((0U != HSM_GetRxFiFoFullFlag()) || (timeout == 0U))
                {
                    return STATUS_TIMEOUT;
                }

                timeout = HSM_CMD_TIMEOUT;

                /* Corner case handle: if input len not word align */
                if (blockSize < 4U)
                {
                    tempData = 0U;
                    memcpy((void *)&tempData, (void *)in, blockSize);
                    HSM_SetCipherIn(tempData);
                    blockSize = 0U;
                }
                else
                {
                    HSM_SetCipherIn(in[0]);
                    blockSize -= 4U;
                }
                in++;
            }
        }

        if (NULL != out)
        {
            /* Get output in block length */
            blockSize = blockOutLen;
            while (blockSize > 0U)
            {
                while ((0U != HSM_GetTxFiFoEmptyFlag()) && (timeout > 0U))
                {
                    timeout--;
                }

                if ((0U != HSM_GetTxFiFoEmptyFlag()) || (timeout == 0U))
                {
                    return STATUS_TIMEOUT;
                }

                timeout = HSM_CMD_TIMEOUT;
                out[0] = HSM_GetCipherOut();
                out++;

                blockSize -= 4U;
            }
            outSize -= blockOutLen;
        }
        else
        {
            /* If no output generate, then return */
            break;
        }
    }

    return STATUS_SUCCESS;
}

/*!
 * @brief Config HSM AES Context.
 *
 * @param[in] cmd: The HSM Command Type
 * @param[in] enc: enc type(HSM_EncMode/HSM_CmacMode)
 * @param[in] inLen: The byte size
 * @param[in] keyId: input key Index
 * @return ret: HSM operation status
 */
static void HSM_CryptoAesConfig(uint8_t cmd, uint8_t enc, uint32_t inLen, HSM_KeyIndex keyId)
{
    HSM_SetCmd(cmd);
    /* enc/dec is valid only in ECB/CBC mode */
    HSM_SetEncDec(enc);
    HSM_SetKeyId((uint8_t)keyId);
    HSM_SetLength(inLen);
}

#if HSM_CMAC_UNALIGN_SUPPORT
/*!
 * @brief Context init for unalignment case.
 *
 * @return None
 */
static void HSM_CmacUnalignInit(void)
{
    /* init unaligned config */
    s_hsmCmacInputAlign = TRUE;
    s_hsmDMAUnalignOutBuf = 0U;
    s_hsmCmacUnalignBytes = 0U;
}

/*!
 * @brief Save the unalignment bytes from the tail of last package into unalign context.
 *
 * @param[in] inBuf: input packge buffer address
 * @param[in/out] inLen: The last package byte size, and will be aligned
 * @return ret: HSM operation status
 */
static BOOL_Type HSM_CmacSaveUnalignTail(uint32_t *inBuf, uint32_t *inLen)
{
    uint32_t unalignLen = *inLen & 3U;
    BOOL_Type unalignStatus;

    if (0U != unalignLen)
    {
        memcpy(&s_hsmCmacUnalignBytes, (char *)inBuf + *inLen - unalignLen, unalignLen);
        *inLen -= unalignLen;
        unalignStatus = TRUE;
    }
    else
    {
        unalignStatus = FALSE;
    }

    return unalignStatus;
}
#endif /* HSM_CMAC_UNALIGN_SUPPORT */

/*!
 * @brief Config HSM CMAC Context.
 *
 * @param[in] enc:
 *                - HSM_SIGN
 *                - HSM_VERIFY
 * @param[in] totalLen: The size of total crypto data in bytes
 * @param[in] keyId: type of HSM_KeyIndex: Flash key (8 ~ 17) + Ram Key (18)
 * @param[in] macbuf: the buffer address of expected mac value(128bit buffer)
 * @param[in] cb: customer callback with DMA
 *                NULL: means polling mode
 *                not NULL: means DMA mode
 *                        callback param:
 *                               - device: HSM
 *                               - wpara: The command status
 *                               - lpara: Unused
 * @return ret: HSM operation status
 */
static status_t HSM_CmacConfig(HSM_CmacMode enc, uint32_t totalLen, HSM_KeyIndex keyId, uint32_t *macBuf,
                                   DeviceCallback_Type cb)
{
    if (1U != HSM_GetUnLockStatus())
    {
        return STATUS_BUSY;
    }

    if ((keyId == HSM_BL_VERIFY_KEY_ID) && (NULL != cb))
    {
        s_hsmAesCmacByBlKeyId = TRUE;
    }
    else
    {
        s_hsmAesCmacByBlKeyId = FALSE;
    }

    HSM_CryptoAesConfig((uint8_t)HSM_AES_CMAC, (uint8_t)enc, totalLen, keyId);

#if HSM_CMAC_UNALIGN_SUPPORT
    /* init unaligned config */
    HSM_CmacUnalignInit();
#endif

    s_hsmDmaCallback = cb;
    s_hsmInputMacBuf = macBuf;
    s_hsmDmaLastOps = FALSE;

    return STATUS_SUCCESS;
}

/*!
 * @brief CMAC Update handle will DMA mode.
 *
 * @param[in] inBuf: The buffer address for current input package
 * @param[in] inLen: The package byte size
 * @param[in] outBuf: The buffer address of the output. If null,
 *                    it means there are still some package have not complete.
 * @param[in] outLen: The output buffer byte size
 * @param[in] lastPackage: Mark if this input is last package and result will report after this time
 * @return ret: HSM operation status
 */
static status_t HSM_CmacRunDMA(uint32_t *inBuf, uint32_t inLen, uint32_t *outBuf, uint32_t outLen,
                                    BOOL_Type lastPackage)
{
    status_t status = STATUS_SUCCESS;
    BOOL_Type firstPackage;

    firstPackage = ((1U == HSM_GetUnLockStatus()) ? TRUE : FALSE);

    s_hsmDmaLastOps = lastPackage;
#if HSM_CMAC_UNALIGN_SUPPORT
    if (TRUE == lastPackage)
    {
        /* Handle unaligned bytes for last package */
        if (TRUE == HSM_CmacSaveUnalignTail(inBuf, &inLen))
        {
            s_hsmCmacInputAlign = FALSE;
            s_hsmDMAUnalignOutBuf = outBuf;

            outBuf = NULL;
            outLen = 0U;
        }
        else
        {
            s_hsmCmacInputAlign = TRUE;
        }

        if (inLen == 0U)
        {
            /* if remain bytes is not 4bytes enough after unalign handled, dma callback will be called in local */
            s_hsmDmaLastOps = FALSE;
        }
    }
#endif

    if (inLen > 0U)
    {
        HSM_DMAInit(inBuf, (inLen >> 2U), outBuf, outLen >> 2U, firstPackage);
    }
#if HSM_CMAC_UNALIGN_SUPPORT
    else
    {
        DEVICE_ASSERT(TRUE == lastPackage);

        /* If last package is too short(< 4bytes), we will work with polling mode */
        status = HSM_CryptoFifoHandle(HSM_AES_CMAC, &s_hsmCmacUnalignBytes, HSM_WORDSIZE_BYTES, s_hsmDMAUnalignOutBuf);

        /* checking status and calling customer callback are also need to be done */
        s_hsmCmacInputAlign = TRUE;

        if (status != STATUS_ERROR)
        {
            status = HSM_CmacResultCheck();
        }

        s_hsmDmaCallback(HSM, status, (uint32_t)HSM_GetCmd());
        s_hsmDmaCallback = NULL;

        HSM_EnableDma(DISABLE);
    }
#endif

    if (firstPackage)
    {
        /* Note:
         *   1. set-trig can be called only one time for each total transaction
         *   2. set-trig must be called after DMA inited
         *   3. set-trig must be called before FifoHandle
         */
        HSM_SetTrig();
    }

    return status;
}

/*!
 * @brief CMAC Update handle will CPU Polling mode.
 *
 * @param[in] inBuf: The buffer address for current input package
 * @param[in] inLen: The package byte size
 * @param[in] outBuf: The buffer address of the output. If null,
 *                    it means there are still some package have not complete
 * @return ret: HSM operation status
 */
static status_t HSM_CmacRunPolling(uint32_t *inBuf, uint32_t inLen, uint32_t *outBuf)
{
    /* Handle Data Transfer in Polling Mode */
    return HSM_CryptoFifoHandle(HSM_AES_CMAC, inBuf, inLen, outBuf);
}

/*!
 * @brief Make sure if key-read access is enabled.
 *
 * @return BOOL_Type: TRUE: access enable; FALSE: access disable
 */
static BOOL_Type HSM_isKeyPageAccessValid(void)
{
    BOOL_Type lockStatus = FALSE;
    BOOL_Type access = TRUE;

    /* check current status: if in lock mode, we should check Key_Read & Key_Install access is enabled first.*/
    lockStatus = (BOOL_Type)HSM_GetChipState();

    if (lockStatus == TRUE)
    {
        /* The Key-Read access and Key-Install access must be enabled at first. */
        if ((0U == (HSM_InternalAuthPassFlag() >> (HSM_KEY_READ_KEY_ID - HSM_FW_INSTALL_KEY_ID))) ||
            (0U == (HSM_InternalAuthPassFlag() >> (HSM_KEY_INSTALL_KEY_ID - HSM_FW_INSTALL_KEY_ID))))
        {
            access = FALSE;
        }
    }

    return access;
}

/*!
 * @brief Make sure the input index of the key-page is valid.
 *
 * @param[in] pageIndex: the index of key-page need to be check
 * @return BOOL_Type: TRUE: valid; FALSE: invalid
 */
static BOOL_Type HSM_isKeyPageValid(uint32_t pageIndex)
{
    HSM_FlashKeyPage *keyPage = (HSM_FlashKeyPage *)(HSM_FLASH_KEY_BASE);
    BOOL_Type valid = TRUE;
    int i;

    keyPage += pageIndex;

    for (i = 0; i < HSM_PAGE_VALID_LEN; i++)
    {
        if (keyPage->pageValidFlag[i] != HSM_PAGE_VALID_TAG)
        {
            valid = FALSE;
            break;
        }
    }

    return valid;
}

/*!
 * @brief Make sure the input index of the key-slot could be used.
 *
 * @param[in] slotIndex: the index of key-slot need to be check
 * @param[in] pageIndex: the index of key-page need to be check
 * @return BOOL_Type: TRUE: Usable; FALSE: Un-usable
 */
static BOOL_Type HSM_isKeySlotUsable(uint32_t slotIndex, uint32_t pageIndex)
{
    HSM_FlashKey *keySlot = (HSM_FlashKey *)(HSM_FLASH_KEY_BASE + (pageIndex * DFLASH_PAGE_SIZE));
    BOOL_Type usable = TRUE;
    uint8_t *ptr;
    uint8_t i;

    keySlot += slotIndex;

    ptr = (uint8_t *)keySlot;
    for (i = 0; i < HSM_KEY_SLOT_LEN; i++)
    {
        if (ptr[i] != 0xFFU)
        {
            usable = FALSE;
            break;
        }
    }

    return usable;
}

/*!
 * @brief Make sure the input index of the key-slot could be deprecated.
 *
 * @param[in] slotIndex: the index of key-slot need to be check
 * @param[in] pageIndex: the index of key-page need to be check
 * @return BOOL_Type: TRUE: the key-slot can be deprecated; FALSE: can not deprecated
 */
static BOOL_Type HSM_isKeySlotValid(uint32_t slotIndex, uint32_t pageIndex)
{
    HSM_FlashKey *keySlot = (HSM_FlashKey *)(HSM_FLASH_KEY_BASE + (pageIndex * DFLASH_PAGE_SIZE));
    BOOL_Type valid = TRUE;
    uint8_t *ptr;
    uint8_t i;

    keySlot += slotIndex;

    ptr = (uint8_t *)keySlot;
    for (i = 0; i < sizeof(HSM_KeyInfo0); i++)
    {
        if (ptr[i] != 0xFFU)
        {
            valid = FALSE;
            break;
        }
    }

    return valid;
}

/*!
 * @brief Find current valid key-page and output the final index.
 *
 * @param[out] pageIndex: output the valid index of key-page
 * @return status_t:
 *         STATUS_ERROR: all key-page is broken;
 *         STATUS_SUCCESS: pageIndex is output successfull
 */
static status_t HSM_FindValidKeyPage(uint32_t *pageIndex)
{
    status_t finded = STATUS_ERROR;
    uint32_t pageId;

    for (pageId = 0; pageId < HSM_KEY_PAGE_NUMS; pageId++)
    {
        if (HSM_isKeyPageValid(pageId))
        {
            *pageIndex = pageId;
            finded = STATUS_SUCCESS;
            break;
        }
    }

    return finded;
}

/*!
 * @brief Find an usable key-slot to install a new key from the input key-page.
 *
 * @param[out] slotIndex: output the usable index of key-slot
 * @param[in] pageIndex: the index of key-page need to be check
 * @return status_t:
 *         STATUS_ERROR: all key-slot are used-up;
 *         STATUS_SUCCESS: slotIndex is output successfull
 */
static status_t HSM_FindUsableKeySlot(uint32_t *slotIndex, uint32_t pageIndex)
{
    status_t finded = STATUS_ERROR;
    uint32_t slotId;

    for (slotId = 0; slotId < HSM_KEY_NUMS_ONE_PAGE; slotId++)
    {
        /* If current slot is A5A5, than key slot is invalid which means it could be used directly */
        if (HSM_isKeySlotUsable(slotId, pageIndex))
        {
            *slotIndex = slotId;
            finded = STATUS_SUCCESS;
            break;
        }
    }

    return finded;
}

/*!
 * @brief Find the key-slot which include the target keyindex.
 *
 * @param[out] slotIndex: output the usable index of key-slot
 * @param[in] pageIndex: the index of key-page need to be check
 * @param[in] keyIndex: the index of key need to be check
 * @return status_t:
 *         STATUS_ERROR: all key-slot are used-up;
 *         STATUS_SUCCESS: slotIndex is output successfull
 */
static status_t HSM_FindKeySlotByIndex(uint32_t *slotIndex, uint32_t pageIndex, uint8_t keyIndex)
{
    status_t finded = STATUS_ERROR;
    HSM_FlashKey *keySlot = (HSM_FlashKey *)(HSM_FLASH_KEY_BASE + (pageIndex * DFLASH_PAGE_SIZE));
    uint32_t slotId;

    for (slotId = 0; slotId < HSM_KEY_NUMS_ONE_PAGE; slotId++)
    {
        /* If current slot is A5A5, than key slot is invalid */
        if (keySlot[slotId].keyInfo0.slotInValidFlag == 0xFFFFU)
        {
            if ((keySlot[slotId].keyInfo1.keyIndex == keyIndex) &&
                (keySlot[slotId].keyInfo1.dataValidFlag == HSM_KEY_DATA_VALID_TAG))
            {
                *slotIndex = slotId;
                finded = STATUS_SUCCESS;
                break;
            }
        }
    }

    return finded;
}

/*!
 * @brief Init the key-page: set the first page as valid page, and second page as backup.
 *
 * @param[out] pageIndex: output the index of the final valid key-page
 * @return status_t:
 *         STATUS_ERROR: error happend in flash operation;
 *         STATUS_SUCCESS: pageIndex is output successfull
 */
static status_t HSM_InitKeyPage(uint32_t *pageIndex)
{
    status_t initStatus = STATUS_SUCCESS;
    status_t flashStatus;
    HSM_FlashKeyPage *keyPage;
    uint8_t pageValid[HSM_PAGE_VALID_LEN];

    /* if no key page valid, we should re-init all key page */
    flashStatus = EFLASH_PageErase(HSM_FLASH_KEY_BASE);
    flashStatus |= EFLASH_PageErase(HSM_FLASH_KEY_BASE + DFLASH_PAGE_SIZE);
    if (flashStatus != STATUS_SUCCESS)
    {
        initStatus = STATUS_ERROR;
    }
    else
    {
        /* set first page valid at default */
        keyPage = (HSM_FlashKeyPage *)HSM_FLASH_KEY_BASE;
        memset(pageValid, HSM_PAGE_VALID_TAG, HSM_PAGE_VALID_LEN);
        flashStatus = EFLASH_PageProgram((uint32_t)keyPage->pageValidFlag, (uint32_t *)pageValid,
                                         HSM_PAGE_VALID_LEN / sizeof(uint32_t));
        if (flashStatus != STATUS_SUCCESS)
        {
            initStatus = STATUS_ERROR;
        }
        else
        {
            *pageIndex = 0U;
        }
    }

    return initStatus;
}

/*!
 * @brief Update key-page: set the first page as invalid, and second page as active.
 *
 * @param[out] pageIndex: output the index of the final valid key-page
 * @return status_t:
 *         STATUS_ERROR: error happend in flash operation;
 *         STATUS_SUCCESS: pageIndex is output successfull
 */
static status_t HSM_UpdateKeyPage(uint32_t *pageIndex)
{
    status_t flashStatus;
    HSM_FlashKeyPage *mainKeyPage = (HSM_FlashKeyPage *)HSM_FLASH_KEY_BASE;
    HSM_FlashKeyPage *backupKeyPage = mainKeyPage + 1U;
    HSM_FlashKey *mainKeySlot = (HSM_FlashKey *)mainKeyPage;
    HSM_FlashKey *backupKeySlot = (HSM_FlashKey *)backupKeyPage;
    uint8_t flashWriteBuffer[HSM_PAGE_VALID_LEN];
    uint32_t i;

    /* If the default page(0) is been disabled and the backup keypage(1) already updated, we reinit 2 keypages */
    if ((FALSE == HSM_isKeyPageValid(0)) || (TRUE == HSM_isKeyPageValid(1U)))
    {
        if (HSM_InitKeyPage(pageIndex) != STATUS_SUCCESS)
        {
            return STATUS_ERROR;
        }
        else
        {
            *pageIndex = 0;
            return STATUS_SUCCESS;
        }
    }

    /* Copy valid key slot from main page to backup page */
    for (i = 0; i < HSM_KEY_NUMS_ONE_PAGE; i++)
    {
        if (TRUE == HSM_isKeySlotValid(i, 0U))
        {
            flashStatus = EFLASH_PageProgram((uint32_t)(backupKeySlot + i), (uint32_t *)(mainKeySlot + i),
                                             sizeof(HSM_FlashKey) / sizeof(uint32_t));
            if (flashStatus != STATUS_SUCCESS)
            {
                return STATUS_ERROR;
            }
        }
    }

    /* mark the backup keypage as valid state */
    memset(flashWriteBuffer, HSM_PAGE_VALID_TAG, HSM_PAGE_VALID_LEN);
    flashStatus = EFLASH_PageProgram((uint32_t)backupKeyPage->pageValidFlag, (uint32_t *)flashWriteBuffer,
                                     HSM_PAGE_VALID_LEN / sizeof(uint32_t));
    if (flashStatus != STATUS_SUCCESS)
    {
        return STATUS_ERROR;
    }

    /* clean the default keypage */
    flashStatus = EFLASH_PageErase(HSM_FLASH_KEY_BASE);
    if (flashStatus != STATUS_SUCCESS)
    {
        return STATUS_ERROR;
    }

    *pageIndex = 1U;

    return STATUS_SUCCESS;
}

/*!
 * @brief Install Key Data into target key-page and key-slot.
 *
 * @param[in] pageIndex: the index of the target key-page
 * @param[in] slotIndex: the index of the target key-slot
 * @param[in] keyData: the key data string (16bytes)
 * @param[in] keyIndex: the index of the target key(0-8)
 * @return status_t:
 *         STATUS_ERROR: error happend in flash operation;
 *         STATUS_SUCCESS: setup finished
 */
static status_t HSM_SetupKeyData(uint32_t pageIndex, uint32_t slotIndex, uint32_t *keyData, uint32_t keyIndex)
{
    status_t setupStatus = STATUS_SUCCESS;
    status_t flashStatus;
    HSM_FlashKeyPage *keyPage = ((HSM_FlashKeyPage *)HSM_FLASH_KEY_BASE) + pageIndex;
    HSM_FlashKey *keySlot = (HSM_FlashKey *)keyPage;
    HSM_KeyInfo1 keyInfo = {0};

    keySlot += slotIndex;

    keyInfo.dataValidFlag = HSM_KEY_DATA_VALID_TAG;
    keyInfo.keyIndex = (uint8_t)(keyIndex - HSM_FLASH_KEY_START_ID);
    keyInfo.keySlot = (uint8_t)slotIndex;

    flashStatus = EFLASH_PageProgram((uint32_t)&keySlot->keyInfo1, (uint32_t *)&keyInfo,
                                      sizeof(HSM_KeyInfo1) / sizeof(uint32_t));
    flashStatus |= EFLASH_PageProgram((uint32_t)&keySlot->keyData, keyData,
                                      HSM_CRYPTO_BLOCK_BYTE / sizeof(uint32_t));
    if (flashStatus != STATUS_SUCCESS)
    {
        setupStatus = STATUS_ERROR;
    }

    return setupStatus;
}

/*!
 * @brief Invalid the target key-slot.
 *
 * @param[in] pageIndex: the index of the target key-page
 * @param[in] slotIndex: the index of the target key-slot
 * @return status_t:
 *         STATUS_ERROR: error happend in flash operation;
 *         STATUS_SUCCESS: invalid finished
 */
static status_t HSM_InvalidKeySlot(uint32_t pageIndex, uint32_t slotIndex)
{
    status_t invalidStatus = STATUS_SUCCESS;
    status_t flashStatus;
    HSM_FlashKeyPage *keyPage = ((HSM_FlashKeyPage *)HSM_FLASH_KEY_BASE) + pageIndex;
    HSM_FlashKey *keySlot = (HSM_FlashKey *)keyPage;
    HSM_KeyInfo0 keyInfo = {0};

    keyInfo.slotInValidFlag = (uint16_t)HSM_SLOT_INVALID_TAG;

    keySlot += slotIndex;
    flashStatus = EFLASH_PageProgram((uint32_t)&keySlot->keyInfo0, (uint32_t *)&keyInfo,
                                     sizeof(HSM_KeyInfo1) / sizeof(uint32_t));
    if (flashStatus != STATUS_SUCCESS)
    {
        invalidStatus = STATUS_ERROR;
    }

    return invalidStatus;
}

/* ======================================  Global Functions define  ======================================== */
/*!
 * @brief ECB encrypt or decrypt by customer key.
 *
 * @param[in] enc:
 *                - HSM_ENCRYPT
 *                - HSM_DECRYPT
 * @param[in] inBuf: address of crypto data
 * @param[in] inLen: The size of crypto data in bytes(inBuf len = outBuf len and align with 16bytes)
 * @param[out] outBuf: address of crypto result
 * @param[in] keyId: type of HSM_KeyIndex: Flash key (8 ~ 17) + Ram Key (18)
 * @param[in] cb: customer callback with DMA
 *                NULL: means polling mode
 *                not NULL: means DMA mode
 *                        callback param:
 *                               - device: HSM
 *                               - wpara: The command status
 *                               - lpara: Unused
 * @return ret: HSM operation status
 */
status_t HSM_CryptoAesECB(HSM_EncMode enc, uint32_t *inBuf, uint32_t inLen, uint32_t *outBuf, HSM_KeyIndex keyId,
                               DeviceCallback_Type cb)
{
    status_t cryptoStatus;

    DEVICE_ASSERT((NULL != inBuf) && (NULL != outBuf));
    DEVICE_ASSERT((inLen > 0U) && ((inLen % HSM_CRYPTO_BLOCK_BYTE) == 0U));
    DEVICE_ASSERT((keyId <= HSM_RAM_KEY_ID) && (keyId >= HSM_FLASH_KEY_START_ID));

    if (1U != HSM_GetUnLockStatus())
    {
        cryptoStatus = STATUS_BUSY;
    }
    else
    {
        HSM_CryptoAesConfig((uint8_t)HSM_AES_ECB, (uint8_t)enc, inLen, keyId);

        if (NULL != cb)
        {
            s_hsmDmaCallback = cb;
            s_hsmAesDmaLastOps = TRUE;
            HSM_DMAInit(inBuf, inLen >> 2U, outBuf, inLen >> 2U, TRUE);
            HSM_SetTrig();

            cryptoStatus = STATUS_SUCCESS;
        }
        else
        {
            HSM_SetTrig();

            cryptoStatus = HSM_CryptoFifoHandle(HSM_AES_ECB, inBuf, inLen, outBuf);
        }
    }

    return cryptoStatus;
}

/*!
 * @brief CBC encrypt or decrypt by customer key.
 *
 * @param[in] enc:
 *                - HSM_ENCRYPT
 *                - HSM_DECRYPT
 * @param[in] inBuf: address of crypto data
 * @param[in] inLen: The size of crypto data in bytes(inBuf len = outBuf len and align with 16bytes)
 * @param[out] outBuf: address of crypto result
 * @param[in] ivBuf: address of iv
 * @param[in] keyId: type of HSM_KeyIndex: Flash key (8 ~ 17) + Ram Key (18)
 * @param[in] cb: customer callback with DMA
 *                NULL: means polling mode
 *                not NULL: means DMA mode
 *                        callback param:
 *                               - device: HSM
 *                               - wpara: The command status
 *                               - lpara: Unused
 * @return ret: HSM operation status
 */
status_t HSM_CryptoAesCBC(HSM_EncMode enc, uint32_t *inBuf, uint32_t inLen, uint32_t *outBuf, uint32_t *ivBuf,
                               HSM_KeyIndex keyId, DeviceCallback_Type cb)
{
    status_t cryptoStatus;

    DEVICE_ASSERT((NULL != inBuf) && (NULL != outBuf) && (NULL != ivBuf));
    DEVICE_ASSERT((inLen > 0U) && ((inLen % HSM_CRYPTO_BLOCK_BYTE) == 0U));
    DEVICE_ASSERT((keyId <= HSM_RAM_KEY_ID) && (keyId >= HSM_FLASH_KEY_START_ID));

    if (1U != HSM_GetUnLockStatus())
    {
        cryptoStatus = STATUS_BUSY;
    }
    else
    {
        HSM_CryptoAesConfig((uint8_t)HSM_AES_CBC, (uint8_t)enc, inLen, keyId);
        HSM_SetIv(ivBuf);

        if (NULL != cb)
        {
            s_hsmDmaCallback = cb;
            s_hsmAesDmaLastOps = TRUE;
            HSM_DMAInit(inBuf, inLen >> 2U, outBuf, inLen >> 2U, TRUE);
            HSM_SetTrig();

            cryptoStatus = STATUS_SUCCESS;
        }
        else
        {
            HSM_SetTrig();

            cryptoStatus = HSM_CryptoFifoHandle(HSM_AES_CBC, inBuf, inLen, outBuf);
        }
    }

    return cryptoStatus;
}

/*!
 * @brief CMAC sign or verify by customer key.
 *
 * @param[in] enc:
 *                - HSM_SIGN
 *                - HSM_VERIFY
 * @param[in] totalLen: The size of total crypto data in bytes
 * @param[in] keyId: type of HSM_KeyIndex: Flash key (8 ~ 17) + Ram Key (18)
 * @param[in] macbuf: the buffer address of expected mac value(128bit buffer)
 * @param[in] cb: customer callback with DMA
 *                NULL: means polling mode
 *                not NULL: means DMA mode
 *                        callback param:
 *                               - device: HSM
 *                               - wpara: The command status
 *                               - lpara: Unused
 * @return ret: HSM operation status
 */
status_t HSM_CryptoAesCMACStart(HSM_CmacMode enc, uint32_t totalLen, HSM_KeyIndex keyId, uint32_t *macbuf,
                                       DeviceCallback_Type cb)
{
    DEVICE_ASSERT(((keyId >= HSM_FLASH_KEY_START_ID) && (keyId <= HSM_RAM_KEY_ID) || (keyId == HSM_BL_VERIFY_KEY_ID)));
    DEVICE_ASSERT(totalLen > 0U);
    DEVICE_ASSERT(NULL != macbuf);
#if !HSM_CMAC_UNALIGN_SUPPORT
    DEVICE_ASSERT(0U == (totalLen & 3U));
#endif

    return HSM_CmacConfig(enc, totalLen, keyId, macbuf, cb);
}

/*!
 * @brief CMAC sign or verify in Update mode by customer key.
 *
 * @param[in] inBuf: address of crypto data in this time. It must be 4bytes align.
 * @param[in] inLen: The size of crypto data in bytes in this time. It must be 4bytes align if not last package.
 * @param[in] lastPackage: Mark if this input is last package and result will report after this time
 * @return ret: HSM operation status
 */
status_t HSM_CryptoAesCMACUpdate(uint32_t *inBuf, uint32_t inLen, BOOL_Type lastPackage)
{
    status_t ret;
    uint32_t *outBuf;
    uint32_t outLen;
    uint8_t enc = HSM_GetEncDec();
    BOOL_Type firstPackage;

    DEVICE_ASSERT(NULL != inBuf);
    DEVICE_ASSERT(inLen > 0U);
#if HSM_CMAC_UNALIGN_SUPPORT
    if (lastPackage == FALSE)
    {
#endif
    DEVICE_ASSERT(0U == (inLen & 3U));
#if HSM_CMAC_UNALIGN_SUPPORT
    }
#endif

    firstPackage = ((1U == HSM_GetUnLockStatus()) ? TRUE : FALSE);

    /* If last package, we should update the dma mark */
    s_hsmDmaLastOps = lastPackage;

    if (enc == HSM_SIGN)
    {
        outBuf = ((lastPackage == TRUE) ? s_hsmInputMacBuf : NULL);
    }
    else
    {
        outBuf = ((lastPackage == TRUE) ? s_hsmTempMacBuf : NULL);
    }

    outLen = ((lastPackage == TRUE) ? HSM_CRYPTO_BLOCK_BYTE : 0);

    if (NULL != s_hsmDmaCallback)
    {
        ret = HSM_CmacRunDMA(inBuf, inLen, outBuf, outLen, lastPackage);
    }
    else
    {
        if (TRUE == firstPackage)
        {
            /* Note:
             *   1. set-trig can be called only one time for each total transaction
             *   2. set-trig must be called before the first data input into cipher-in
             */
            HSM_SetTrig();
        }
        ret = HSM_CmacRunPolling(inBuf, inLen, outBuf);

        /* If in verify mode and last package, we should compare mac data */
        if ((ret == STATUS_SUCCESS) && (lastPackage == TRUE) && (enc == (uint8_t)HSM_VERIFY))
        {
            if (memcmp(s_hsmTempMacBuf, s_hsmInputMacBuf, HSM_CRYPTO_BLOCK_BYTE) != 0)
            {
                ret = STATUS_HSM_AUTH_FAIL;
            }
        }
    }

    return ret;
}

/*!
 * @brief Get HSM authentication challenge.
 *
 * @param[in] keyId: otp key id
 *                  - 3: Debug auth key
 *                  - 4: Firmware install key
 *                  - 5: Key read key
 *                  - 6: Key install key
 *                  - 7: Chip state key
 * @param[out] challenge: The pointer of authentication challenge
 * @return ret: HSM operation status
 */
status_t HSM_CryptoAuthInit(HSM_KeyIndex keyId, uint32_t *chlgBuf)
{
    status_t cryptoStatus;

    DEVICE_ASSERT(NULL != chlgBuf);
    DEVICE_ASSERT((keyId <= HSM_LOCK_STAT_KEY_ID) && (keyId >= HSM_DEBUG_AUTH_KEY_ID));

    if (1U != HSM_GetUnLockStatus())
    {
        cryptoStatus = STATUS_BUSY;
    }
    else
    {
        HSM_SetCmd((uint8_t)HSM_AES_AUTH);
        HSM_SetLength(HSM_AUTHENTICATION_LEN);
        HSM_SetKeyId((uint8_t)keyId);
        HSM_SetTrig();

        cryptoStatus = HSM_ParseCmdStatus();
        if (cryptoStatus == STATUS_SUCCESS)
        {
            cryptoStatus = HSM_GetChallenge(chlgBuf);
        }
    }

    return cryptoStatus;
}

/*!
 * @brief Get HSM authentication challenge.
 *
 * @param[in] keyId: otp key id
 *                  - 3: Debug auth key
 *                  - 4: Firmware install key
 *                  - 5: Key read key
 *                  - 6: Key install key
 *                  - 7: Chip state key
 * @param[out] challenge: The pointer of authentication challenge
 * @return ret: HSM operation status
 */
status_t HSM_CryptoAuthFinish(HSM_KeyIndex keyId, uint32_t *respBuf)
{
    status_t cryptoStatus = STATUS_SUCCESS;

    DEVICE_ASSERT(NULL != respBuf);
    DEVICE_ASSERT((keyId <= HSM_LOCK_STAT_KEY_ID) && (keyId >= HSM_DEBUG_AUTH_KEY_ID));

    HSM_SetAuthResponse(respBuf);
    HSM_SetTrigResponse();

    if (keyId == HSM_DEBUG_AUTH_KEY_ID)
    {
        while (0U == HSM_DebugAuthDoneFlag())
        {
        }

        if (0U != HSM_DebugAuthTimeoutFlag())
        {
            cryptoStatus = STATUS_TIMEOUT;
        }
        else
        {
            if (0U == HSM_DebugAuthPassFlag())
            {
                cryptoStatus = STATUS_HSM_AUTH_FAIL;
            }
        }
    }
    else
    {
        while (0U == HSM_InternalAuthDoneFlag())
        {
        }

        if (0U != HSM_InternalAuthTimeoutFlag())
        {
            cryptoStatus = STATUS_TIMEOUT;
        }
        else
        {
            if (0U == (HSM_InternalAuthPassFlag() >> (keyId - HSM_FW_INSTALL_KEY_ID)))
            {
                cryptoStatus = STATUS_HSM_AUTH_FAIL;
            }
        }
    }
    return cryptoStatus;
}

/*!
 * @brief Verify fimrware/bootloader by otp key.
 *
 * @param[in] totalLen: The size of total crypto data in bytes
 * @param[in] keyId: type of HSM_KeyIndex: HSM_BL_VERIFY_KEY_ID or HSM_FW_UPGRADE_KEY_ID
 * @param[in] macbuf: the buffer address of expected mac value(128bit buffer)
 * @param[in] cb: customer callback with DMA
 *                NULL: means polling mode
 *                not NULL: means DMA mode
 *                        callback param:
 *                               - device: HSM
 *                               - wpara: The command status
 *                               - lpara: Unused
 * @return ret: HSM operation status
 */
status_t HSM_FirmVerifyStart(uint32_t totalLen, HSM_KeyIndex keyId, uint32_t *macBuf, DeviceCallback_Type cb)
{
    DEVICE_ASSERT((keyId == HSM_BL_VERIFY_KEY_ID) || (keyId == HSM_FW_UPGRADE_KEY_ID));
    DEVICE_ASSERT(totalLen > 0U);
    DEVICE_ASSERT(NULL != macBuf);
#if !HSM_CMAC_UNALIGN_SUPPORT
    DEVICE_ASSERT(0U == (totalLen & 3U));
#endif

    /* MAC Data must be set before CMD-Trig start */
    HSM_SetGoldenMac0(macBuf[0]);
    HSM_SetGoldenMac1(macBuf[1]);
    HSM_SetGoldenMac2(macBuf[2]);
    HSM_SetGoldenMac3(macBuf[3]);

    return HSM_CmacConfig(HSM_VERIFY, totalLen, keyId, macBuf, cb);
}

/*!
 * @brief Verify fimrware/bootloader in Upgrade mode by otp key.
 *
 * @param[in] inBuf: address of crypto data in this time. It must be 4bytes align.
 * @param[in] inLen: The size of crypto data in bytes in this time. It must be 4bytes align if not last package.
 * @param[in] lastPackage: Mark if this input is last package and result will report after this time
 * @return ret: HSM operation status
 */
status_t HSM_FirmVerifyUpdate(uint32_t *inBuf, uint32_t inLen, BOOL_Type lastPackage)
{
    status_t ret = STATUS_SUCCESS;
    HSM_KeyIndex keyId = (HSM_KeyIndex)HSM_GetKeyId();
    BOOL_Type firstPackage;
    uint32_t *outBuf = NULL;
    uint32_t outLen = 0;

    DEVICE_ASSERT(NULL != inBuf);
    DEVICE_ASSERT(inLen > 0U);
    DEVICE_ASSERT((keyId == HSM_BL_VERIFY_KEY_ID) || (keyId == HSM_FW_UPGRADE_KEY_ID));
#if HSM_CMAC_UNALIGN_SUPPORT
    if (lastPackage == FALSE)
    {
#endif
    DEVICE_ASSERT(0U == (inLen & 3U));
#if HSM_CMAC_UNALIGN_SUPPORT
    }
#endif

    firstPackage = ((1U == HSM_GetUnLockStatus()) ? TRUE : FALSE);

    if (keyId == HSM_BL_VERIFY_KEY_ID)
    {
        /* only BL key maybe output mac value if HSM_FW_INSTALL_KEY auth pass */
        if (HSM_InternalAuthPassFlag() & HSM_AUTH_STAT_ARV_Msk)
        {
            if (TRUE == lastPackage)
            {
                outBuf = (uint32_t *)s_hsmInputMacBuf;
                outLen = HSM_CRYPTO_BLOCK_BYTE;
            }
        }
    }

    if (NULL != s_hsmDmaCallback)
    {
        ret = HSM_CmacRunDMA(inBuf, inLen, outBuf, outLen, lastPackage);
    }
    else
    {
        if (TRUE == firstPackage)
        {
            /* Note:
             *   1. set-trig can be called only one time for each total transaction
             *   2. set-trig must be called before the first data input into cipher-in
             */
            HSM_SetTrig();
        }

        ret = HSM_CmacRunPolling(inBuf, inLen, outBuf);

        if (TRUE == lastPackage)
        {
            if (ret == STATUS_SUCCESS)
            {
                if (keyId == HSM_BL_VERIFY_KEY_ID)
                {
                    /* wait verify done */
                    while (0U == HSM_GetBlVerifyDoneFlag())
                    {
                    }
                    /* Get verify status */
                    if (0U == HSM_GetBlVerifyPassFlag())
                    {
                        ret = STATUS_HSM_AUTH_FAIL;
                    }
                }
                else
                {
                    /* wait verify done */
                    while (0U == HSM_GetUpFwVerifyDoneFlag())
                    {
                    }
                    /* Get verify status */
                    if (0U == HSM_GetFwUpVerifyPassFlag())
                    {
                        ret = STATUS_HSM_AUTH_FAIL;
                    }
                }
            }
        }
    }

    return ret;
}

/*!
 * @brief Get HSM rand data with polling mode.
 *
 * @param[in] outLen: The byte size of rand data, should 4-aligned
 * @param[out] outBuf: address of rand data buffer
 * @return ret: HSM operation status
 */
status_t HSM_CryptoGetRandom(uint32_t outLen, uint32_t *outBuf)
{
    DEVICE_ASSERT(NULL != outBuf);
    DEVICE_ASSERT((outLen > 0U) && (0U == (outLen % HSM_TRNG_BLOCK_BYTE)));

    if (1U != HSM_GetUnLockStatus())
    {
        return STATUS_BUSY;
    }

    HSM_SetCmd((uint8_t)HSM_GET_RND);
    HSM_SetLength(outLen);
    HSM_SetTrig();

    return HSM_CryptoFifoHandle(HSM_GET_RND, NULL, outLen, outBuf);
}

/*!
 * @brief Get UID from HSM
 *
 * @param[out] uid: address of uid buffer(fixed in 16bytes)
 * @return ret: HSM operation status
 */
void HSM_GetUID(uint32_t *uid)
{
    DEVICE_ASSERT(NULL != uid);

    uid[0] = HSM_GetUid0();
    uid[1] = HSM_GetUid1();
    uid[2] = HSM_GetUid2();
    uid[3] = HSM_GetUid3();
}

/*!
 * @brief Install ram key in HSM
 *
 * @param[in] key: key data buffer(fixed in 16bytes)
 * @return ret: HSM operation status
 */
void HSM_InstallRamKey(uint32_t *key)
{
    DEVICE_ASSERT(NULL != key);

    HSM_SetRamKey0(key[0]);
    HSM_SetRamKey1(key[1]);
    HSM_SetRamKey2(key[2]);
    HSM_SetRamKey3(key[3]);
}

/*!
 * @brief Revoke ram key in HSM
 *
 * @return None
 */
void HSM_UninstallRamKey(void)
{
    HSM_SetRamKey0(0);
    HSM_SetRamKey1(0);
    HSM_SetRamKey2(0);
    HSM_SetRamKey3(0);
}

/*!
 * @brief Install flash key in HSM
 *
 * @param[in] key: key data buffer(fixed in 16bytes)
 * @param[in] keyIndex: keyIndex(8-17);
 * @return status: STAUS_ERROR/STATUS_SUCCESS
 */
status_t HSM_InstallFlashKey(uint32_t *key, HSM_KeyIndex keyIndex)
{
    status_t status = STATUS_SUCCESS;
    uint32_t pageIndex;
    uint32_t slotIndex;

    DEVICE_ASSERT(NULL != key);
    DEVICE_ASSERT((keyIndex >= HSM_FLASH_KEY_START_ID) && (keyIndex <= HSM_FLASH_KEY_END_ID));

    if (EFLASH_UnlockCtrl() != STATUS_SUCCESS)
    {
        return STATUS_ERROR;
    }

    if (FALSE == HSM_isKeyPageAccessValid())
    {
        /* The Key-Read access and Key-Install access must be enabled at first. */
        status = STATUS_ERROR;
        goto install_fini;
    }

    if (HSM_FindValidKeyPage(&pageIndex) != STATUS_SUCCESS)
    {
        /* If no valid page could be used, we clean all key-page */
        if (HSM_InitKeyPage(&pageIndex) != STATUS_SUCCESS)
        {
            status = STATUS_ERROR;
            goto install_fini;
        }
    }

    if (HSM_FindKeySlotByIndex(&slotIndex, pageIndex, (uint8_t)(keyIndex - HSM_FLASH_KEY_START_ID)) == STATUS_SUCCESS)
    {
        /* If correspond index already exist, invalid it first */
        if (HSM_InvalidKeySlot(pageIndex, slotIndex) != STATUS_SUCCESS)
        {
            status = STATUS_ERROR;
            goto install_fini;
        }
    }

    if (HSM_FindUsableKeySlot(&slotIndex, pageIndex) != STATUS_SUCCESS)
    {
        /* If no valid slot could be used, we update to backup keypage */
        if (HSM_UpdateKeyPage(&pageIndex) != STATUS_SUCCESS)
        {
            status = STATUS_ERROR;
            goto install_fini;
        }
        /* re-find usabe slot in new key page */
        if (HSM_FindUsableKeySlot(&slotIndex, pageIndex) != STATUS_SUCCESS)
        {
            status = STATUS_ERROR;
            goto install_fini;
        }
    }

    /* Final Install Operations */
    if (HSM_SetupKeyData(pageIndex, slotIndex, key, keyIndex) != STATUS_SUCCESS)
    {
        status = STATUS_ERROR;
        goto install_fini;
    }

install_fini:
    EFLASH_LockCtrl();

    return status;
}

/*!
 * @brief Revoke flash key in HSM
 *
 * @param[in] keyIndex: index of key(8 ~ 17)
 * @return status: STAUS_ERROR/STATUS_SUCCESS
 */
status_t HSM_UninstallFlashKey(HSM_KeyIndex keyIndex)
{
    status_t status = STATUS_SUCCESS;
    uint32_t slotIndex;
    uint32_t pageIndex;

    DEVICE_ASSERT((keyIndex >= HSM_FLASH_KEY_START_ID) && (keyIndex <= HSM_FLASH_KEY_END_ID));

    if (EFLASH_UnlockCtrl() != STATUS_SUCCESS)
    {
        return STATUS_ERROR;
    }

    if (HSM_FindValidKeyPage(&pageIndex) != STATUS_SUCCESS)
    {
        status = STATUS_ERROR;
        goto Uninstall_fini;
    }

    if (HSM_FindKeySlotByIndex(&slotIndex, pageIndex, (uint8_t)(keyIndex - HSM_FLASH_KEY_START_ID)) != STATUS_SUCCESS)
    {
        status = STATUS_ERROR;
        goto Uninstall_fini;
    }

    if (HSM_InvalidKeySlot(pageIndex, slotIndex) != STATUS_SUCCESS)
    {
        status = STATUS_ERROR;
        goto Uninstall_fini;
    }

Uninstall_fini:
    EFLASH_LockCtrl();

    return status;
}

/*!
 * @brief remove all flash key in HSM
 *
 * @return status: STAUS_ERROR/STATUS_SUCCESS
 */
status_t HSM_UninstallAllFlashKey(void)
{
    status_t status = STATUS_SUCCESS;
    uint32_t pageIndex = 0U;

    if (EFLASH_UnlockCtrl() != STATUS_SUCCESS)
    {
        return STATUS_ERROR;
    }

    if (FALSE == HSM_isKeyPageAccessValid())
    {
        /* The Key-Read access and Key-Install access must be enabled at first. */
        status = STATUS_ERROR;
        goto Uninstall_fini;
    }

    /* If no valid page could be used, we clean all key-page */
    if (HSM_InitKeyPage(&pageIndex) != STATUS_SUCCESS)
    {
        status = STATUS_ERROR;
        goto Uninstall_fini;
    }

Uninstall_fini:
    EFLASH_LockCtrl();

    return status;
}

/*!
 * @brief Install HSM command complete interrupt call back function.
 *
 * @param[in] callback: type of hsm_isrcallback_t, if input NULL, callback will be deinit
 *                      callback param:
 *                                     - device: HSM
 *                                     - wpara: The command currently be processed
 *                                     - lpara: The STAT register value
 * @return none
 */
void HSM_InstallCallback(DeviceCallback_Type callback)
{
    s_hsmISRCallback = callback;
}

/*!
 * @brief HSM Interrupt Handler
 *
 * @param[in] none
 * @return none
 */
void HSM_IRQHandler(void)
{
    HSM_ClearIrqFlag();

    /* store device status */
    if (NULL != s_hsmISRCallback)
    {
        s_hsmISRCallback((void *)HSM, (uint32_t)HSM_GetCmd(), (uint32_t)HSM_GetCmdStatus());
    }
}

/*!
 * @brief Get module version information.
 *
 * @param[out] versionInfo: Module version information address.
 * @return void
 */
void HSM_GetVersionInfo(VersionInfo_Type *versionInfo)
{
    DEVICE_ASSERT(versionInfo != NULL);

    versionInfo->moduleID = HSM_HAL_MODULE_ID;
    versionInfo->majorVersion = HSM_HAL_SW_MAJOR_VERSION;
    versionInfo->minorVersion = HSM_HAL_SW_MINOR_VERSION;
    versionInfo->patchVersion = HSM_HAL_SW_PATCH_VERSION;
}
/* =============================================  EOF  ============================================== */
