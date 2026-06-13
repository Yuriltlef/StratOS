/**
 * @file STM32F103md_static_in.ld
 * @brief Linker script for StratOS pure static memory layout (absolute addresses)
 *
 * This script calculates the base addresses of each RAM section using CMake variables
 * and places sections explicitly, ensuring a contiguous and non-overlapping layout.
 */

ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)      : ORIGIN = @_START_OF_FLASH_@, LENGTH = @_SIZE_OF_FLASH_@
    RAM   (xrw)     : ORIGIN = @_START_OF_RAM_@,   LENGTH = @_SIZE_OF_RAM_@
}

/* Calculate base addresses of each section (relative to RAM start) */
_user_stack_base  = ORIGIN(RAM);
_user_pool_base   = _user_stack_base + @_SIZE_OF_USER_STACK_@;
_kernel_stack_base = _user_pool_base + @_USER_POOL_SIZE_@;
_kernel_pool_base  = _kernel_stack_base + @_KERNEL_STACK_SIZE_@;

/* Ensure total RAM usage does not exceed available RAM */
ASSERT( _kernel_pool_base + @_KERNEL_POOL_SIZE_@ <= ORIGIN(RAM) + LENGTH(RAM),
        "RAM overflow: total sections exceed RAM size" );

SECTIONS
{
    /* ====== Flash sections (unchanged) ====== */
    .isr_vector : {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } >FLASH

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
        _etext = .;
    } >FLASH

    .rodata : {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } >FLASH

    /* .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
    .ARM : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } >FLASH */

    .preinit_array : {
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array*))
        PROVIDE_HIDDEN (__preinit_array_end = .);
    } >FLASH

    .init_array : {
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
    } >FLASH

    .fini_array : {
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT(.fini_array.*)))
        KEEP (*(.fini_array*))
        PROVIDE_HIDDEN (__fini_array_end = .);
    } >FLASH

    /* ====== RAM sections (explicit base addresses) ====== */

    /* User Stack */
    .user_stack _user_stack_base : {
        . = ALIGN(8);
        _suser_stack = .;
        . = . + @_SIZE_OF_USER_STACK_@;
        _euser_stack = .;
    } > RAM

    /* User Pool (contains .data, .bss, and filler) */
    .user_pool _user_pool_base : {
        . = ALIGN(4);
        _suser_pool = .;

        /* .data section (initialized data) */
        . = ALIGN(4);
        _sdata = .;
        *(.data .data.*)
        . = ALIGN(4);
        _edata = .;

        /* .bss section (uninitialized data) */
        . = ALIGN(4);
        _sbss = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;

        /* Fill to the fixed user pool size */
        . = _suser_pool + @_USER_POOL_SIZE_@;
        _euser_pool = .;
    } > RAM AT> FLASH
    _sidata = LOADADDR(.user_pool) + (_sdata - _suser_pool);  /* LMA of .data inside .user_pool */

    /* Kernel Stack */
    .kernel_stack _kernel_stack_base : {
        . = ALIGN(8);
        _skernel_stack = .;
        . = . + @_KERNEL_STACK_SIZE_@;
        _ekernel_stack = .;
    } > RAM

    /* Kernel Pool */
    .kernel_pool _kernel_pool_base : {
        . = ALIGN(4);
        _skernel_pool = .;
        *(.kernel_pool .kernel_pool.*)
        . = . + @_KERNEL_POOL_SIZE_@;
        _ekernel_pool = .;
    } > RAM

    /* Discard unwanted sections */
    /DISCARD/ : {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }

    .ARM.attributes 0 : { *(.ARM.attributes) }
}

/* Initial MSP points to kernel stack top */
_estack = _ekernel_stack;
_eflash = @_END_OF_FLASH_@;