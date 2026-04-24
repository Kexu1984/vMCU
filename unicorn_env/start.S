/*
 * Copyright Statement:
 *
 * This software/firmware and related documentation ("AutoChips Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to AutoChips Inc. and/or its licensors.
 *
 * AutoChips Inc. (C) 2024. All rights reserved.
 */

/*!
 * @file start.S
 *
 * @brief Startup assembly code for ARM Cortex-M7 baremetal environment
 *
 * This file provides the reset handler and basic CPU initialization for
 * running C programs in a baremetal environment.
 */

    .syntax unified
    .cpu cortex-m7
    .fpu fpv5-sp-d16
    .thumb

/* Stack configuration */
    .equ    STACK_SIZE, 0x2000    /* 8KB stack size */

/* Vector table */
    .section .vectors,"a",%progbits
    .type   g_pfnVectors, %object
    .size   g_pfnVectors, .-g_pfnVectors

g_pfnVectors:
    .word   _estack                     /* Top of Stack */
    .word   Reset_Handler               /* Reset Handler */
    .word   NMI_Handler                 /* NMI Handler */
    .word   HardFault_Handler           /* Hard Fault Handler */
    .word   MemManage_Handler           /* MPU Fault Handler */
    .word   BusFault_Handler            /* Bus Fault Handler */
    .word   UsageFault_Handler          /* Usage Fault Handler */
    .word   0                           /* Reserved */
    .word   0                           /* Reserved */
    .word   0                           /* Reserved */
    .word   0                           /* Reserved */
    .word   SVC_Handler                 /* SVCall Handler */
    .word   DebugMon_Handler            /* Debug Monitor Handler */
    .word   0                           /* Reserved */
    .word   PendSV_Handler              /* PendSV Handler */
    .word   SysTick_Handler             /* SysTick Handler */

    /* External Interrupts */
    .rept   256                         /* 256 external interrupt vectors */
    .word   Default_Handler
    .endr

    .size   g_pfnVectors, . - g_pfnVectors

/* Reset handler */
    .section .text.Reset_Handler
    .weak   Reset_Handler
    .type   Reset_Handler, %function

Reset_Handler:
    /* Set stack pointer */
    ldr     sp, =_estack

    /* Enable FPU if available */
#ifdef __VFP_FP__
    /* CPACR is located at address 0xE000ED88 */
    ldr     r0, =0xE000ED88
    ldr     r1, [r0]
    /* Enable CP10 and CP11 coprocessors (bits 20-23) */
    orr     r1, r1, #(0xF << 20)
    str     r1, [r0]
    dsb
    isb
#endif

    /* Since everything is in continuous memory space, no need to copy data */
    /* Just zero fill the bss segment */
    ldr     r2, =_sbss
    b       LoopFillZerobss

/* Zero fill the bss segment */
FillZerobss:
    movs    r3, #0
    str     r3, [r2], #4

LoopFillZerobss:
    ldr     r3, = _ebss
    cmp     r2, r3
    bcc     FillZerobss

    /* Initialize system clock and peripherals */
    //bl      SystemInit

    /* Call the main function */
    bl      main

    /* Infinite loop if main returns */
InfiniteLoop:
    b       InfiniteLoop

    .size   Reset_Handler, . - Reset_Handler

/* Default exception handlers */
    .section .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
    b       Infinite_Loop
    .size   Default_Handler, . - Default_Handler

/* Weak exception handlers */
    .macro  def_irq_handler handler_name
    .weak   \handler_name
    .thumb_set \handler_name, Default_Handler
    .endm

    def_irq_handler    NMI_Handler
    def_irq_handler    HardFault_Handler
    def_irq_handler    MemManage_Handler
    def_irq_handler    BusFault_Handler
    def_irq_handler    UsageFault_Handler
    def_irq_handler    SVC_Handler
    def_irq_handler    DebugMon_Handler
    def_irq_handler    PendSV_Handler
    def_irq_handler    SysTick_Handler

/* Default system initialization function (weak) */
    .section .text.SystemInit
    .weak   SystemInit
    .type   SystemInit, %function
SystemInit:
    bx      lr
    .size   SystemInit, . - SystemInit

/* Stack definition */
    .section .stack
    .align  3
#ifdef __STACK_SIZE
    .equ    Stack_Size, __STACK_SIZE
#else
    .equ    Stack_Size, STACK_SIZE
#endif
    .globl  __StackTop
    .globl  __StackLimit
__StackLimit:
    .space  Stack_Size
    .size   __StackLimit, . - __StackLimit
__StackTop:
    .size   __StackTop, . - __StackTop

/* Heap definition (optional) */
    .section .heap
    .align  3
#ifdef __HEAP_SIZE
    .equ    Heap_Size, __HEAP_SIZE
#else
    .equ    Heap_Size, 0
#endif
    .globl  __HeapBase
    .globl  __HeapLimit
__HeapBase:
    .if     Heap_Size
    .space  Heap_Size
    .endif
    .size   __HeapBase, . - __HeapBase
__HeapLimit:
    .size   __HeapLimit, . - __HeapLimit

/* Provide stack symbols */
    .globl _estack
_estack = __StackTop
