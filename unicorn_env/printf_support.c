/* Copyright Statement:
 *
 * This software/firmware and related documentation ("AutoChips Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to AutoChips Inc. and/or its licensors.
 *
 * AutoChips Inc. (C) 2024. All rights reserved.
 */

/*!
 * @file printf_support.c
 *
 * @brief Minimal printf support for baremetal environment
 *
 * This file provides basic printf functionality for debugging output
 * in baremetal environment without standard library support.
 */

/* ===========================================  Includes  =========================================== */
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include "printf_support.h"

/* ============================================  Define  ============================================ */

/* ITM (Instrumentation Trace Macrocell) Stimulus Port definitions */
#define ITM_BASE                0xE0000000UL
#define ITM_STIM0               (*(volatile uint32_t*)(ITM_BASE + 0x00UL))
#define ITM_TER                 (*(volatile uint32_t*)(ITM_BASE + 0xE00UL))
#define ITM_TCR                 (*(volatile uint32_t*)(ITM_BASE + 0xE80UL))

/* DWT (Data Watchpoint and Trace) definitions */
#define DWT_BASE                0xE0001000UL
#define DWT_CTRL                (*(volatile uint32_t*)(DWT_BASE + 0x00UL))

/* CoreDebug definitions */
#define CoreDebug_BASE          0xE000EDF0UL
#define CoreDebug_DEMCR         (*(volatile uint32_t*)(CoreDebug_BASE + 0x0CUL))

/* ===========================================  Typedef  ============================================ */

/* ==========================================  Variables  =========================================== */

static char print_buffer[256];

/* ====================================  Functions declaration  =================================== */

static void itm_putchar(char c);
static int int_to_str(int value, char* str, int base);
static int uint_to_str(unsigned int value, char* str, int base);

/* =====================================  Functions definition  =================================== */

/*!
 * @brief Initialize ITM for debug output
 */
void printf_init(void)
{
    return;
    /* Enable trace */
    CoreDebug_DEMCR |= (1UL << 24U);  /* Set TRCENA bit */

    /* Enable ITM */
    ITM_TCR |= (1UL << 0U) |           /* Set ITMENA bit */
               (1UL << 3U);            /* Set TXENA bit */

    /* Enable stimulus port 0 */
    ITM_TER |= (1UL << 0U);

    /* Enable DWT cycle counter */
    DWT_CTRL |= (1UL << 0U);
}

/*!
 * @brief Send a character via ITM
 *
 * @param[in] c Character to send
 */
static void itm_putchar(char c)
{
    ITM_STIM0 = (uint8_t)c;
}

/*!
 * @brief Put a character to debug output
 *
 * @param[in] c Character to output
 */
void putchar(char c)
{
    itm_putchar(c);
}

/*!
 * @brief Put a string to debug output
 *
 * @param[in] str String to output
 */
void puts(const char* str)
{
    while (*str)
    {
        putchar(*str++);
    }
    putchar('\n');
}

/*!
 * @brief Convert integer to string
 *
 * @param[in] value Value to convert
 * @param[out] str Output string buffer
 * @param[in] base Number base (2, 8, 10, 16)
 * @return Length of string
 */
static int int_to_str(int value, char* str, int base)
{
    char temp[32];
    int i = 0;
    int len = 0;
    int is_negative = 0;

    if (value == 0)
    {
        str[0] = '0';
        str[1] = '\0';
        return 1;
    }

    if (value < 0 && base == 10)
    {
        is_negative = 1;
        value = -value;
    }

    while (value > 0)
    {
        int digit = value % base;
        temp[i++] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
        value /= base;
    }

    if (is_negative)
    {
        str[len++] = '-';
    }

    /* Reverse the string */
    while (i > 0)
    {
        str[len++] = temp[--i];
    }

    str[len] = '\0';
    return len;
}

/*!
 * @brief Convert unsigned integer to string
 *
 * @param[in] value Value to convert
 * @param[out] str Output string buffer
 * @param[in] base Number base (2, 8, 10, 16)
 * @return Length of string
 */
static int uint_to_str(unsigned int value, char* str, int base)
{
    char temp[32];
    int i = 0;
    int len = 0;

    if (value == 0)
    {
        str[0] = '0';
        str[1] = '\0';
        return 1;
    }

    while (value > 0)
    {
        int digit = value % base;
        temp[i++] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
        value /= base;
    }

    /* Reverse the string */
    while (i > 0)
    {
        str[len++] = temp[--i];
    }

    str[len] = '\0';
    return len;
}

/*!
 * @brief Minimal printf implementation
 *
 * @param[in] format Format string
 * @param[in] ... Variable arguments
 * @return Number of characters printed
 */
int printf(const char* format, ...)
{
    va_list args;
    const char* p;
    char* buf_ptr = print_buffer;
    int printed = 0;

    va_start(args, format);

    for (p = format; *p != '\0'; p++)
    {
        if (*p != '%')
        {
            *buf_ptr++ = *p;
            printed++;
            continue;
        }

        p++; /* Skip '%' */

        switch (*p)
        {
            case 'c':
            {
                char c = (char)va_arg(args, int);
                *buf_ptr++ = c;
                printed++;
                break;
            }

            case 'd':
            case 'i':
            {
                int value = va_arg(args, int);
                char temp[32];
                int len = int_to_str(value, temp, 10);
                strcpy(buf_ptr, temp);
                buf_ptr += len;
                printed += len;
                break;
            }

            case 'u':
            {
                unsigned int value = va_arg(args, unsigned int);
                char temp[32];
                int len = uint_to_str(value, temp, 10);
                strcpy(buf_ptr, temp);
                buf_ptr += len;
                printed += len;
                break;
            }

            case 'x':
            case 'X':
            {
                unsigned int value = va_arg(args, unsigned int);
                char temp[32];
                int len = uint_to_str(value, temp, 16);
                strcpy(buf_ptr, temp);
                buf_ptr += len;
                printed += len;
                break;
            }

            case 's':
            {
                char* str = va_arg(args, char*);
                if (str == NULL)
                    str = "(null)";
                int len = strlen(str);
                strcpy(buf_ptr, str);
                buf_ptr += len;
                printed += len;
                break;
            }

            case '%':
            {
                *buf_ptr++ = '%';
                printed++;
                break;
            }

            default:
            {
                *buf_ptr++ = '%';
                *buf_ptr++ = *p;
                printed += 2;
                break;
            }
        }
    }

    va_end(args);

    /* Null terminate and output */
    *buf_ptr = '\0';

    /* Send the buffer via ITM */
    for (int i = 0; i < printed; i++)
    {
        itm_putchar(print_buffer[i]);
    }

    return printed;
}
