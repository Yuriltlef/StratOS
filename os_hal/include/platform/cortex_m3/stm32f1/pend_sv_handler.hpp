/**
 * @file pend_sv_handler.hpp
 * @author StratOS Team
 * @brief 内置 Cortex-M3 PendSV 异常处理程序（内联汇编实现）
 * @version 1.0.0
 * @date 2026-04-03
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件提供了针对 Cortex-M3 的 PendSV 中断服务程序实现，
 * 使用内联汇编保存/恢复任务寄存器，并调用调度器的 C++ 函数
 * 进行任务切换。该实现符合 StratOS 的静态策略模式，
 * 作为平台策略的一部分，无需单独汇编文件。
 *
 * @note PendSV 是任务切换的核心异常，必须被正确实现。
 * @warning 该函数必须声明为 `extern "C"` 并使用 `__attribute__((naked))`，
 *          以确保编译器不生成序言/尾声代码，从而精确控制栈布局。
 */
#pragma once

#ifndef STRATOS_POLICY_CORTEX_M3_STM32F1_PEND_SV_HANDLER_HPP
#define STRATOS_POLICY_CORTEX_M3_STM32F1_PEND_SV_HANDLER_HPP

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PendSV 异常处理程序（汇编实现）
 *
 * 该函数由 Cortex-M3 的中断向量表调用，负责保存当前任务的寄存器、
 * 调用调度器选择下一个任务、恢复新任务的寄存器，并返回。
 * 由于需要精确控制栈操作，函数使用 `naked` 属性并内联汇编。
 *
 * @note 依赖两个外部 C++ 函数（由调度器提供）：
 *       - void scheduler_save_current(uint32_t* sp): 保存当前任务栈指针。
 *       - uint32_t* scheduler_get_next(): 获取下一个任务的栈指针。
 */
__attribute__((naked)) static void PendSV_Handler(void) {
    __asm volatile(
        // 1. 保存当前任务的寄存器
        "   mrs     r0, psp\n"           // r0 = 当前进程栈指针 (PSP)
        "   stmdb   r0!, {r4-r11}\n"     // 将 r4-r11 压入任务栈，同时更新 r0 (新栈顶)
        "   msr     psp, r0\n"           // 更新 PSP 为保存后的栈顶（可选，但有利于调试）

        // 2. 调用 C++ 函数保存当前任务上下文
        "   mov     r1, r0\n"            // 将栈顶作为参数传递给 scheduler_save_current
        "   bl      scheduler_save_current\n"

        // 3. 调用 C++ 函数获取下一个任务的栈指针
        "   bl      scheduler_get_next\n" // 返回值（新任务的栈顶）在 r0 中

        // 4. 恢复新任务的寄存器
        "   ldmia   r0!, {r4-r11}\n"     // 从新任务栈中弹出 r4-r11
        "   msr     psp, r0\n"           // 设置 PSP 为新任务的栈顶

        // 5. 返回，异常返回时将使用 PSP 作为栈指针
        "   bx      lr\n"
    );
}

#ifdef __cplusplus
}
#endif

#endif // STRATOS_POLICY_CORTEX_M3_STM32F1_PEND_SV_HANDLER_HPP