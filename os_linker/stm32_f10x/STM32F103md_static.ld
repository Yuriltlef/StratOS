/**
 * @file STM32F103md_static_in.ld
 * @brief Linker script for StratOS pure static memory layout on STM32F103
 *
 * This script defines the memory layout for a pure static RTOS configuration:
 * - FLASH: code, read-only data, init arrays
 * - RAM:   divided into four consecutive zones (low to high address):
 *   1. User Stack        (one contiguous area subdivided into multiple task stacks)
 *   2. User Pool         (contains .data, .bss, and filler to fixed size)
 *   3. Kernel Stack      (main stack for MSP)
 *   4. Kernel Pool       (kernel objects like TCBs, queues, etc.)
 *
 * All sizes are provided via CMake variables (placeholders) replaced at build time.
 *
 * PLACEHOLDERS (replaced by CMake):
 * - 0x08000000   : Start address of FLASH memory (e.g., 0x08000000)
 * - 64K    : Size of FLASH memory (e.g., 64K)
 * - 0x20000000     : Start address of RAM (e.g., 0x20000000)
 * - 20K      : Size of RAM (e.g., 20K)
 * - 0x0800FFFF     : End address of FLASH memory (e.g., 0x0800FFFF)
 * - 0x20004FFF       : End address of RAM (e.g., 0x20004FFF)
 * - 0x1000 : Total size of the user stack area (bytes)
 * - 0x2000   : Total size of the user pool (bytes)
 * - 0x1000: Size of the kernel stack (bytes)
 * - 0x1000 : Size of the kernel pool (bytes)
 *
 * Assertions are placed to catch layout errors at link time.
 */

/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack (initial MSP value, can be overridden) */
_estack = 0x20004FFF;    /* end of RAM */

_eflash = 0x0800FFFF;  /* end of FLASH */

/* Memory Regions */
MEMORY
{
    FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 64K
    RAM   (xrw)     : ORIGIN = 0x20000000,   LENGTH = 20K
}

/* Sections */
SECTIONS
{
    /* ====== Flash sections ====== */

    /* Interrupt vector table (must be at beginning of FLASH) */
    .isr_vector : {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } >FLASH

    /* Executable code */
    .text : {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)

        KEEP (*(.init))
        KEEP (*(.fini))

        . = ALIGN(4);
        _etext = .;                     /* end of code in FLASH */
    } >FLASH

    /* Read-only data (constants, strings) */
    .rodata : {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } >FLASH

    /* ARM exception handling tables */
    .ARM.extab : {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    .ARM : {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > FLASH

    /* Constructor/destructor arrays */
    .preinit_array : {
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array*))
        PROVIDE_HIDDEN (__preinit_array_end = .);
    } > FLASH

    .init_array : {
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
    } > FLASH

    .fini_array : {
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT(.fini_array.*)))
        KEEP (*(.fini_array*))
        PROVIDE_HIDDEN (__fini_array_end = .);
    } > FLASH

    /* ====== RAM sections (from low to high address) ====== */

    /******** User Stack (a single block subdivided by kernel into task stacks) ********/
    .user_stack : {
        . = ALIGN(8);
        _suser_stack = .;
        . += 0x1000;
        _euser_stack = .;
    } > RAM
    ASSERT( _euser_stack <= ORIGIN(RAM) + LENGTH(RAM),
            "Error: User stack exceeds RAM boundary" )

    /******** User Pool (contains .data, .bss, and filler to reach fixed size) ********/
    _suser_pool = .;                     /* mark start of user pool */

    /* Load address of .data section (initial values stored in FLASH) */
    _sidata = LOADADDR(.data);

    /* Initialized data (copy from FLASH to RAM at startup) */
    .data : AT(_sidata) {
        . = ALIGN(4);
        _sdata = .;
        *(.data .data.*)
        . = ALIGN(4);
        _edata = .;
    } > RAM

    /* Uninitialized data (zero-initialized at startup) */
    .bss : {
        . = ALIGN(4);
        _sbss = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* Filler to guarantee the user pool has exactly 0x2000 bytes */
    .user_pool_fill : {
        . = ALIGN(4);
        /* Compute remaining bytes needed to reach the fixed size */
        LONG( 0x2000 - ( . - _suser_pool ) );
        . = . + (0x2000 - ( . - _suser_pool ));
        _euser_pool = .;
    } > RAM
    ASSERT( (. - _suser_pool) <= 0x2000,
            "Error: User pool exceeded its reserved size" )

    /******** Kernel Stack (stack for MSP, e.g., interrupts, kernel services) ********/
    .kernel_stack : {
        . = ALIGN(8);
        _skernel_stack = .;
        . = . + 0x1000;
        _ekernel_stack = .;
    } > RAM
    ASSERT( _ekernel_stack <= ORIGIN(RAM) + LENGTH(RAM),
            "Error: Kernel stack exceeds RAM boundary" )

    /******** Kernel Pool (static kernel objects: TCBs, queues, semaphores, etc.) ********/
    .kernel_pool : {
        . = ALIGN(4);
        _skernel_pool = .;
        *(.kernel_pool .kernel_pool.*)
        . = . + 0x1000;
        _ekernel_pool = .;
    } > RAM
    ASSERT( _ekernel_pool <= ORIGIN(RAM) + LENGTH(RAM),
            "Error: Kernel pool exceeds RAM boundary" )

    /* Final sanity check: total RAM usage does not overflow */
    ASSERT( _ekernel_pool <= ORIGIN(RAM) + LENGTH(RAM),
            "Error: RAM overflow! Total sections exceed available RAM." )

    /* ====== Discard unwanted sections from standard libraries ====== */
    /DISCARD/ : {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }

    /* ARM attribute section (always at the end) */
    .ARM.attributes 0 : { *(.ARM.attributes) }
}
