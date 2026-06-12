/**
 * @file PendSV.s
 * @author StratOS Team
 * @brief PendSV 异常处理程序，实现任务上下文切换
 * @version 1.0.0
 * @date 2026-06-07
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
 * 2. 调用 C 函数 scheduler_save_current()，将当前任务栈顶保存到 TCB，
 *    并保存平台上下文（如 FPU 寄存器）。
 * 3. 调用 C 函数 scheduler_get_next()，执行调度算法，获取下一个任务的
 *    TCB 指针，并恢复新任务的平台上下文，返回新任务栈顶。
 * 4. 恢复新任务的寄存器 R4-R11，并设置 PSP 指向新任务栈顶。
 * 5. 异常返回，硬件自动弹出剩余寄存器（R0-R3, R12, LR, PC, xPSR），
 *    切换到新任务执行。
 *
 * 寄存器使用约定：
 * - r0 : 参数传递和返回值（栈指针）
 * - r4-r11 : 被调用者保存寄存器，需要手动保存/恢复
 * - 硬件在进入异常时自动压栈 xPSR, PC, LR, R12, R3-R0
 *
 * 调度器接口要求：
 * - extern "C" void scheduler_save_current(uint32_t* current_sp);
 * - extern "C" uint32_t* scheduler_get_next(void);
 * 这两个函数必须由 C++ 调度器适配器实现。
 */

    .syntax unified          /* 统一语法，支持 Thumb2 指令 */
    .thumb                   /* Thumb 模式 */
    .text                    /* 代码段 */
    .global PendSV_Handler   /* 导出符号，供中断向量表使用 */
    .type PendSV_Handler, %function

PendSV_Handler:
    /**
     *    获取当前任务的进程栈指针（PSP）
     *    将 PSP 值读入 r0，以便后续压栈操作。
     */
    mrs     r0, psp

    /**
     *    保存被调用者保存寄存器（R4-R11）到当前任务栈
     *    stmdb (Store Multiple Decrement Before) 先递减 r0，再存储寄存器。
     *    此时 r0 指向保存完所有寄存器后的新栈顶地址。
     */
    stmdb   r0!, {r4-r11}
    push    {lr}
    /**
     *    调用 C 函数保存当前任务状态
     *    参数：r0 = 当前任务栈顶指针（已包含 R4-R11 压栈后的值）
     *    该函数负责：
     *      - 将 r0 存入当前 TCB 的 sp 字段
     *      - 调用平台上下文策略保存额外状态（如 FPU 寄存器）
     */
    bl      scheduler_save_current

    /**
     *    调用 C 函数获取下一个任务的栈指针
     *    该函数执行调度算法（例如从就绪队列取出下一个 TCB），
     *    更新调度器内部当前任务指针，并恢复新任务的平台上下文。
     *    返回值：r0 = 下一个任务的栈顶指针（uint32_t*）
     */
    bl      scheduler_get_next

    /**
     *    恢复下一个任务的上下文（R4-R11）
     *    ldmia (Load Multiple Increment After) 从 r0 地址加载寄存器，
     *    然后 r0 自增指向栈顶之后的位置。
     *    随后将 r0 写入 PSP，使异常返回后任务使用正确的栈。
     */
    pop     {lr}
    mov     lr , #0xFFFFFFFD
    ldmia   r0!, {r4-r11}
    msr     psp, r0

    /**
     *    异常返回
     *    bx lr 触发异常返回机制，硬件会自动从新 PSP 指向的栈中
     *    弹出 xPSR, PC, LR, R12, R3-R0，并切换到新任务执行。
     */
    bx      lr

    .size   PendSV_Handler, .-PendSV_Handler