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
 * @file ac780x_eflash.ld
 *
 * @brief Linker script for AC780x EFLASH baremetal application
 *
 * This linker script defines the memory layout and sections for
 * ARM Cortex-M7 baremetal applications targeting AC780x series MCUs.
 */

/* Entry Point */
ENTRY(Reset_Handler)

/* Generate a link error if heap and stack don't fit into memory */
_Min_Heap_Size = 0x200;      /* required amount of heap  */
_Min_Stack_Size = 0x400;     /* required amount of stack */

/* Specify the memory areas - Single continuous memory space */
MEMORY
{
  MEM (rwx)       : ORIGIN = 0x00000000, LENGTH = 0x10000  /* 32KB RAM equivalent */
}

/* Highest address of the user mode stack */
_estack = ORIGIN(MEM) + LENGTH(MEM);

/* Define output sections - All in continuous memory space */
SECTIONS
{
  /* The startup code goes first */
  . = 0x0;

  /* The program code and other data */
  .text :
  {
    . = ALIGN(4);
    *(.text.Reset_Handler)      /* Reset handler */
    *(.text)                    /* .text sections (code) */
    *(.text*)                   /* .text* sections (code) */
    *(.glue_7)                  /* glue arm to thumb code */
    *(.glue_7t)                 /* glue thumb to arm code */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;                 /* define a global symbols at end of code */
  } >MEM

  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.vectors))           /* Startup code */
    . = ALIGN(4);
  } >MEM

  /* Constant data */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)                  /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)                 /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(4);
  } >MEM

  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >MEM
  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >MEM

  .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >MEM

  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >MEM

  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >MEM

  /* Initialized data sections - no longer needs copying since everything is in same memory */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;                 /* create a global symbol at data start */
    *(.data)                    /* .data sections */
    *(.data*)                   /* .data* sections */
    *(.RamFunc)                 /* .RamFunc sections */

    . = ALIGN(4);
    _edata = .;                 /* define a global symbol at data end */
  } >MEM

  /* Uninitialized data section */
  . = ALIGN(4);
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss secion */
    _sbss = .;                  /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;                  /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >MEM

  /* User_heap_stack section */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >MEM

  /* Stack section */
  .stack :
  {
    . = ALIGN(8);
    _sstack = .;
    . = . + DEFINED(__STACK_SIZE) ? __STACK_SIZE : 0x2000;
    . = ALIGN(8);
    _estack = .;
  } >MEM

  /* Heap section */
  .heap :
  {
    . = ALIGN(8);
    __HeapBase = .;
    _sheap = .;
    . = . + DEFINED(__HEAP_SIZE) ? __HEAP_SIZE : 0x1000;
    . = ALIGN(8);
    __HeapLimit = .;
    _eheap = .;
  } >MEM

  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
