/* Copyright Statement:
 *
 * This software/firmware and related documentation ("AutoChips Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to AutoChips Inc. and/or its licensors.
 *
 * AutoChips Inc. (C) 2024. All rights reserved.
 */

/*!
 * @file system_init.c
 *
 * @brief System initialization functions for AC780x baremetal environment
 *
 * This file provides basic system initialization functions including
 * clock configuration, cache setup, and peripheral initialization.
 */

/* ===========================================  Includes  =========================================== */
#include <stdint.h>
#include <string.h>

/* ============================================  Define  ============================================ */

/* System Clock Configuration */
#define HSI_VALUE               16000000U   /*!< Internal High Speed oscillator */
#define HSE_VALUE               25000000U   /*!< External High Speed oscillator */
#define LSI_VALUE               32000U      /*!< Internal Low Speed oscillator */
#define LSE_VALUE               32768U      /*!< External Low Speed oscillator */

/* System Control Block (SCB) */
#define SCB_BASE                0xE000ED00UL
#define SCB_CPACR_OFFSET        0x88U
#define SCB_CPACR               (*(volatile uint32_t*)(SCB_BASE + SCB_CPACR_OFFSET))

/* Cache Control */
#define SCB_CCSIDR              (*(volatile uint32_t*)(SCB_BASE + 0x80U))
#define SCB_CCSELR              (*(volatile uint32_t*)(SCB_BASE + 0x84U))
#define SCB_ICIALLU             (*(volatile uint32_t*)(SCB_BASE + 0x250U))
#define SCB_DCISW               (*(volatile uint32_t*)(SCB_BASE + 0x260U))
#define SCB_DCCISW              (*(volatile uint32_t*)(SCB_BASE + 0x274U))
#define SCB_CCR                 (*(volatile uint32_t*)(SCB_BASE + 0x14U))

/* ===========================================  Typedef  ============================================ */

/* ==========================================  Variables  =========================================== */

/* System Core Clock variable */
uint32_t SystemCoreClock = HSI_VALUE;

/* Clock configuration table */
static const uint32_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
static const uint32_t APBPrescTable[8]  = {0, 0, 0, 0, 1, 2, 3, 4};

/* ====================================  Functions declaration  =================================== */

static void SystemClock_Config(void);
static void CPU_CACHE_Enable(void);
static void FPU_Enable(void);

/* =====================================  Functions definition  =================================== */

/*!
 * @brief System initialization function
 *
 * This function is called from startup code before main().
 * It performs basic system initialization.
 */
void SystemInit(void)
{
    /* Enable FPU if present */
    FPU_Enable();

    /* Enable CPU caches */
    CPU_CACHE_Enable();

    /* Configure system clocks */
    SystemClock_Config();

    /* Initialize SystemCoreClock variable */
    SystemCoreClockUpdate();
}

/*!
 * @brief Update SystemCoreClock variable
 *
 * Updates the SystemCoreClock variable with current core Clock retrieved from cpu registers.
 */
void SystemCoreClockUpdate(void)
{
    /* For simplicity, assume HSI is used as system clock */
    SystemCoreClock = HSI_VALUE;
}

/*!
 * @brief Configure system clocks
 *
 * Sets up the main system clock source and peripheral clocks.
 */
static void SystemClock_Config(void)
{
    /* This is a simplified clock configuration */
    /* In a real implementation, you would configure:
     * - PLL settings
     * - Clock source selection
     * - Peripheral clock enables
     * - Flash wait states
     */

    /* For now, use default HSI clock */
    SystemCoreClock = HSI_VALUE;
}

/*!
 * @brief Enable CPU caches
 *
 * Enables instruction and data caches for better performance.
 */
static void CPU_CACHE_Enable(void)
{
#if defined(__CORTEX_M) && (__CORTEX_M >= 7U)
    /* Enable I-Cache */
    __asm volatile ("dsb 0xF":::"memory");
    SCB_ICIALLU = 0UL;                     /* invalidate I-Cache */
    __asm volatile ("dsb 0xF":::"memory");
    __asm volatile ("isb 0xF":::"memory");

    /* Enable instruction cache */
    uint32_t ccr = SCB_CCR;
    ccr |= (1UL << 17U);                   /* Set IC bit */
    SCB_CCR = ccr;
    __asm volatile ("dsb 0xF":::"memory");
    __asm volatile ("isb 0xF":::"memory");

    /* Enable D-Cache */
    uint32_t ccsidr;
    uint32_t sets;
    uint32_t ways;

    /* Invalidate D-Cache */
    SCB_CCSELR = 0U;                       /* Level 1 data cache */
    __asm volatile ("dsb 0xF":::"memory");

    ccsidr = SCB_CCSIDR;
    sets = (uint32_t)(((ccsidr & 0x0FFFE000UL) >> 13U) + 1UL);

    do {
        ways = (uint32_t)(((ccsidr & 0x00001FFCUL) >> 2U) + 1UL);
        do {
            SCB_DCISW = (((sets - 1UL) << 5U) | ((ways - 1UL) << 30U));
        } while (--ways != 0UL);
    } while (--sets != 0UL);

    __asm volatile ("dsb 0xF":::"memory");

    /* Enable data cache */
    ccr = SCB_CCR;
    ccr |= (1UL << 16U);                   /* Set DC bit */
    SCB_CCR = ccr;
    __asm volatile ("dsb 0xF":::"memory");
    __asm volatile ("isb 0xF":::"memory");
#endif
}

/*!
 * @brief Enable Floating Point Unit
 *
 * Enables the FPU for floating point operations.
 */
static void FPU_Enable(void)
{
#if defined(__VFP_FP__) && !defined(__SOFTFP__)
    /* Enable CP10 and CP11 coprocessors */
    SCB_CPACR |= (0xFUL << 20U);
    __asm volatile ("dsb 0xF":::"memory");
    __asm volatile ("isb 0xF":::"memory");
#endif
}

/*!
 * @brief Basic delay function
 *
 * @param[in] count Delay count
 */
void delay_cycles(uint32_t count)
{
    volatile uint32_t i;
    for (i = 0; i < count; i++)
    {
        __asm volatile ("nop");
    }
}

/*!
 * @brief Microsecond delay function
 *
 * @param[in] us Microseconds to delay
 */
void delay_us(uint32_t us)
{
    /* Assuming system clock is running, calculate cycles */
    uint32_t cycles = (SystemCoreClock / 1000000U) * us;
    delay_cycles(cycles / 4);  /* Approximate 4 cycles per loop iteration */
}

/*!
 * @brief Millisecond delay function
 *
 * @param[in] ms Milliseconds to delay
 */
void delay_ms(uint32_t ms)
{
    uint32_t i;
    for (i = 0; i < ms; i++)
    {
        delay_us(1000);
    }
}
