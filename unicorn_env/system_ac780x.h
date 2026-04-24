/* Copyright Statement:
 *
 * This software/firmware and related documentation ("AutoChips Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to AutoChips Inc. and/or its licensors.
 *
 * AutoChips Inc. (C) 2024. All rights reserved.
 */

/*!
 * @file system_ac780x.h
 *
 * @brief System header file for AC780x baremetal environment
 *
 * This file provides system-level definitions and function declarations
 * for AC780x series MCUs in baremetal environment.
 */

#ifndef SYSTEM_AC780X_H
#define SYSTEM_AC780X_H

#ifdef __cplusplus
extern "C" {
#endif

/* ===========================================  Includes  =========================================== */
#include <stdint.h>
#include <stddef.h>

/* ============================================  Define  ============================================ */

/* System Clock Values */
#define HSI_VALUE               16000000U   /*!< Internal High Speed oscillator in Hz */
#define HSE_VALUE               25000000U   /*!< External High Speed oscillator in Hz */
#define LSI_VALUE               32000U      /*!< Internal Low Speed oscillator in Hz */
#define LSE_VALUE               32768U      /*!< External Low Speed oscillator in Hz */

/* Processor and Core Peripheral definitions */
#define __CM7_REV               0x0001U     /*!< Core revision r0p1 */
#define __MPU_PRESENT           1U          /*!< MPU present */
#define __NVIC_PRIO_BITS        4U          /*!< Number of Bits used for Priority Levels */
#define __Vendor_SysTickConfig  0U          /*!< Set to 1 if different SysTick Config is used */
#define __FPU_PRESENT           1U          /*!< FPU present */
#define __ICACHE_PRESENT        1U          /*!< Instruction Cache present */
#define __DCACHE_PRESENT        1U          /*!< Data Cache present */
#define __DTCM_PRESENT          1U          /*!< DTCM present */
#define __ITCM_PRESENT          1U          /*!< ITCM present */

/* Memory map definitions */
#define FLASH_BASE              0x08000000UL /*!< FLASH base address */
#define SRAM1_BASE              0x20000000UL /*!< SRAM1 base address */
#define SRAM2_BASE              0x20080000UL /*!< SRAM2 base address */
#define ITCM_BASE               0x00000000UL /*!< ITCM base address */
#define DTCM_BASE               0x20000000UL /*!< DTCM base address */

/* Peripheral base addresses */
#define PERIPH_BASE             0x40000000UL /*!< Peripheral base address */
#define APB1PERIPH_BASE         PERIPH_BASE  /*!< APB1 peripheral base address */
#define APB2PERIPH_BASE         0x40010000UL /*!< APB2 peripheral base address */
#define AHB1PERIPH_BASE         0x40020000UL /*!< AHB1 peripheral base address */
#define AHB2PERIPH_BASE         0x48000000UL /*!< AHB2 peripheral base address */
#define AHB3PERIPH_BASE         0x50000000UL /*!< AHB3 peripheral base address */

/* Core system control */
#define SCS_BASE                0xE000E000UL /*!< System Control Space Base Address */
#define SCB_BASE                (SCS_BASE + 0x0D00UL) /*!< System Control Block Base Address */

/* ===========================================  Typedef  ============================================ */

/* ==========================================  Variables  =========================================== */

extern uint32_t SystemCoreClock;  /*!< System Clock Frequency (Core Clock) */

/* ====================================  Functions declaration  =================================== */

/*!
 * @brief Setup the microcontroller system
 *
 * Initialize the system clock and configure basic peripherals.
 */
void SystemInit(void);

/*!
 * @brief Update SystemCoreClock variable
 *
 * Update SystemCoreClock variable according to Clock Register Values.
 * The SystemCoreClock variable contains the core clock (HCLK), it can be used
 * by the user application to setup the SysTick timer or configure other parameters.
 */
void SystemCoreClockUpdate(void);

/*!
 * @brief Delay function in CPU cycles
 *
 * @param[in] count Number of CPU cycles to delay
 */
void delay_cycles(uint32_t count);

/*!
 * @brief Delay function in microseconds
 *
 * @param[in] us Microseconds to delay
 */
void delay_us(uint32_t us);

/*!
 * @brief Delay function in milliseconds
 *
 * @param[in] ms Milliseconds to delay
 */
void delay_ms(uint32_t ms);

/* Compiler and platform specific definitions */
#ifndef __ASM
  #define __ASM                  __asm        /*!< asm keyword for ARM Compiler */
#endif

#ifndef __INLINE
  #define __INLINE               inline       /*!< inline keyword for ARM Compiler */
#endif

#ifndef __STATIC_INLINE
  #define __STATIC_INLINE        static inline
#endif

#ifndef __FORCEINLINE
  #define __FORCEINLINE          __attribute__((always_inline)) static inline
#endif

#ifndef __NO_RETURN
  #define __NO_RETURN            __attribute__((__noreturn__))
#endif

#ifndef __USED
  #define __USED                 __attribute__((used))
#endif

#ifndef __WEAK
  #define __WEAK                 __attribute__((weak))
#endif

#ifndef __PACKED
  #define __PACKED               __attribute__((packed, aligned(1)))
#endif

#ifndef __PACKED_STRUCT
  #define __PACKED_STRUCT        struct __attribute__((packed, aligned(1)))
#endif

#ifndef __PACKED_UNION
  #define __PACKED_UNION         union __attribute__((packed, aligned(1)))
#endif

#ifndef __UNALIGNED_UINT32
  #define __UNALIGNED_UINT32(x)  (*((__attribute__((packed)) uint32_t *)(x)))
#endif

#ifndef __UNALIGNED_UINT16_WRITE
  #define __UNALIGNED_UINT16_WRITE(addr, val)    ((*((__attribute__((packed)) uint16_t *)(addr))) = (val))
#endif

#ifndef __UNALIGNED_UINT16_READ
  #define __UNALIGNED_UINT16_READ(addr)          (*((__attribute__((packed)) uint16_t *)(addr)))
#endif

#ifndef __UNALIGNED_UINT32_WRITE
  #define __UNALIGNED_UINT32_WRITE(addr, val)    ((*((__attribute__((packed)) uint32_t *)(addr))) = (val))
#endif

#ifndef __UNALIGNED_UINT32_READ
  #define __UNALIGNED_UINT32_READ(addr)          (*((__attribute__((packed)) uint32_t *)(addr)))
#endif

#ifndef RegReadEx
    #define RegReadEx(addr)         (*((volatile uint32_t *)(addr)))
#endif

#ifndef RegWriteEx
    #define RegWriteEx(addr, val)   ((*((volatile uint32_t *)(addr))) = (val))
#endif

#define ARGV_MAX                    (8U)
struct test_env {
    uint32_t argc;
    void *test_func;
    const char *argv[ARGV_MAX];
};
#define TEST_PARAMS                 (sizeof(g_testEnv)/sizeof(struct test_env))
#define UNICORN_STOP()              __asm__("bkpt #0");

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_AC780X_H */
