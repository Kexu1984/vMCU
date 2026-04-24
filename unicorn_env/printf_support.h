/* Copyright Statement:
 *
 * This software/firmware and related documentation ("AutoChips Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to AutoChips Inc. and/or its licensors.
 *
 * AutoChips Inc. (C) 2024. All rights reserved.
 */

/*!
 * @file printf_support.h
 *
 * @brief Minimal printf support header for baremetal environment
 */

#ifndef PRINTF_SUPPORT_H
#define PRINTF_SUPPORT_H

#ifdef __cplusplus
extern "C" {
#endif

/* ====================================  Functions declaration  =================================== */

/*!
 * @brief Initialize printf debug support
 */
void printf_init(void);

/*!
 * @brief Put a character to debug output
 *
 * @param[in] c Character to output
 */
void putchar(char c);

/*!
 * @brief Put a string to debug output
 *
 * @param[in] str String to output
 */
void puts(const char* str);

/*!
 * @brief Minimal printf implementation for debug output
 *
 * @param[in] format Format string
 * @param[in] ... Variable arguments
 * @return Number of characters printed
 */
int printf(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif /* PRINTF_SUPPORT_H */
