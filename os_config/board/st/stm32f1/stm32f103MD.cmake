set(_START_OF_FLASH_ 0x08000000)                        # STM32F103MD flash starts at 0x08000000

set(_SIZE_OF_FLASH_ "64K")                              # STM32F103MD has 64 KB of flash memory

set(_START_OF_RAM_ 0x20000000)                          # STM32F103MD RAM starts at 0x20000000

set(_SIZE_OF_RAM_ "20K")                                # STM32F103MD has 20 KB of RAM

set(_END_OF_FLASH_ 0x0800FFFF)                          # End of flash (start + size - 1)

set(_END_OF_RAM_  0x20004FFF)                           # End of RAM (start + size - 1)

set(_SIZE_OF_USER_STACK_ 0x1000)                        # Reserve 4 KB for user stack

set(_USER_POOL_SIZE_ 0x2000)                            # Reserve 8 KB for user pool (data + bss + padding)

set(_KERNEL_STACK_SIZE_ 0x1000)                         # Reserve 4 KB for kernel stack

set(_KERNEL_POOL_SIZE_ 0x1000)                          # Reserve 4 KB for kernel pool (static kernel objects)

set(_OS_MEMORY_LAYOUT_TYPE_ "StaticLayout")             # Use static layout: all sections placed at fixed addresses