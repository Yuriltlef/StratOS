/**
 * @file PendSV.s
 * @author StratOS Team
 * @brief PendSV 异常处理程序，实现任务上下文切换
 * @version 1.1.0
 * @date 2026-06-13
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件实现 PendSV 中断服务例程，用于 ARM Cortex-M 系列处理器的任务切换。
 * PendSV 是一个可悬起的异常，通常被设置为最低优先级，以确保上下文切换
 * 在所有其他中断处理完成后执行。
 *
 * 主要流程：
 * 1. 保存当前任务的上下文（寄存器 R4-R11）到其任务栈中。
 * 2. 调用 C 函数 scheduler_switch()，该函数完成：
 *    - 保存当前任务栈顶到 TCB，并保存平台上下文（可选）
 *    - 执行调度算法，选择下一个任务
 *    - 更新当前任务指针，返回新任务的栈顶指针（R4 地址）
 * 3. 恢复新任务的寄存器 R4-R11，并设置 PSP 指向新任务栈顶。
 * 4. 异常返回，硬件自动弹出剩余寄存器（R0-R3, R12, LR, PC, xPSR），
 *    切换到新任务执行。
 *
 * 寄存器使用约定：
 * - r0 : 参数传递和返回值（栈指针）
 * - r4-r11 : 被调用者保存寄存器，需要手动保存/恢复
 * - 硬件在进入异常时自动压栈 xPSR, PC, LR, R12, R3-R0
 *
 * 调度器接口要求：
 * - extern "C" uint32_t* scheduler_switch(uint32_t* current_sp);
 * 该函数由 C++ 调度器适配器实现。
 */

    .syntax unified          /* 统一语法，支持 Thumb2 指令 */
    .thumb                   /* Thumb 模式 */
    .text                    /* 代码段 */
    .global PendSV_Handler   /* 导出符号，供中断向量表使用 */
    .type PendSV_Handler, %function

PendSV_Handler:
    /**
     * 获取当前任务的进程栈指针（PSP）
     * 将 PSP 值读入 r0，以便后续压栈操作。
     */
    mrs     r0, psp

    /**
     * 保存被调用者保存寄存器（R4-R11）到当前任务栈
     * stmdb (Store Multiple Decrement Before) 先递减 r0，再存储寄存器。
     * 此时 r0 指向保存完所有寄存器后的新栈顶地址（即 R4 地址）。
     */
    stmdb   r0!, {r4-r11}

    /**
     * 保存 EXC_RETURN 值（当前 LR）到主栈（MSP）
     * 因为后面调用 C 函数会破坏 LR，所以需要先保存。
     */
    push    {lr}

    /**
     * 调用 C 函数执行调度和上下文切换
     * 参数：r0 = 当前任务栈顶指针（R4 地址）
     * 返回值：r0 = 下一个任务的栈顶指针（R4 地址）
     */
    bl      scheduler_switch

    /**
     * 恢复 EXC_RETURN 值
     * 目前手动设置确保使用 PSP。
     */
    pop     {lr}
    mov     lr, #0xFFFFFFFD

    /**
     * 恢复下一个任务的上下文（R4-R11）
     * ldmia (Load Multiple Increment After) 从 r0 地址加载寄存器，
     * 然后 r0 自增指向栈顶之后的位置（即 xPSR 地址）。
     * 随后将 r0 写入 PSP，使异常返回后任务使用正确的栈。
     */
    ldmia   r0!, {r4-r11}
    msr     psp, r0

    /**
     * 异常返回
     * bx lr 触发异常返回机制，硬件会自动从新 PSP 指向的栈中
     * 弹出 xPSR, PC, LR, R12, R3-R0，并跳转到新任务执行。
     */
    bx      lr

    .size   PendSV_Handler, .-PendSV_Handler