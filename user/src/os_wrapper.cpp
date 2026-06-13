/**
 * @file os_wrapper.cpp
 * @author StratOS Team
 * @brief 供 PendSV 汇编调用的 C 包装函数（优化版）
 * @version 1.1.0
 * @date 2026-06-13
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件实现了一个 `extern "C"` 函数 `scheduler_switch`，用于在 PendSV 异常处理程序
 * 中一次性完成当前任务状态保存、调度选择下一个任务并返回新任务的栈顶指针。
 * 相比之前分离为 `scheduler_save_current` 和 `scheduler_get_next` 两个函数，
 * 此版本减少了函数调用开销，提高了上下文切换效率。
 *
 * 函数签名与 `pend_sv_handler.s` 中的汇编调用严格匹配：
 * - uint32_t* scheduler_switch(uint32_t* current_sp)
 *
 * 该函数直接调用 `strat_os::os_kernel` 中的调度器适配器接口，
 * 并处理平台上下文的保存/恢复（若策略支持）。
 *
 * 此外，本文件还提供了 SysTick 中断处理函数和硬故障处理函数。
 */

#include "strat_os.hpp" // 包含 strat_os::os_kernel 定义
#include <cstdint>

// 类型别名简化代码
using Kernel      = strat_os::os_kernel;
using Scheduler   = Kernel::scheduler;
using PlatformCtx = typename Scheduler::platform_context;
using Contex      = typename PlatformCtx::platform_context_type;
using EmptyBase   = strat_os::kernel::EmptyBase;

extern "C" {

/**
 * @brief 执行任务切换（保存当前任务、调度、恢复新任务上下文）
 * @param current_sp 当前任务栈顶指针（已包含 R4-R11 压栈后的值）
 * @return 新任务的栈顶指针（R4 地址）
 *
 * @details
 * 该函数由 PendSV 汇编调用，参数位于 r0 寄存器。
 * 它执行以下操作：
 * 1. 将当前任务的栈顶（即传入的 current_sp）存入 TCB 的 `sp` 字段。
 * 2. 若平台上下文非空，则调用平台上下文的 `save` 方法保存当前任务的额外状态。
 * 3. 调用调度器的 `schedule()` 选择下一个要运行的任务。
 * 4. 更新当前任务指针为新任务。
 * 5. （可选）恢复新任务的平台上下文（当前已禁用，通过 `if constexpr (false)` 条件编译消除）。
 * 6. 返回新任务的栈顶指针（R4 地址），供 PendSV 汇编恢复寄存器。
 *
 * @note 该函数合并了原来的 `scheduler_save_current` 和 `scheduler_get_next`，
 *       减少了函数调用次数，优化了上下文切换性能。
 * @note 平台上下文的恢复代码被暂时禁用（`if constexpr (false)`），因为当前平台
 *       （Cortex-M3）无 FPU 等需要额外恢复的状态。如有需要可修改条件。
 */
uint32_t* scheduler_switch(uint32_t* current_sp) noexcept {
    // 获取当前任务 TCB
    auto* current = Scheduler::get_current();
    if (current) {
        // 保存当前任务的栈顶（R4 地址）
        current->sp = reinterpret_cast<std::uintptr_t>(current_sp);
        // 保存平台上下文（如果支持）
        if constexpr (PlatformCtx::supports_platform_context) {
            Contex& ctx = static_cast<Contex&>(*current);
            PlatformCtx::save(&ctx);
        }
    }

    // 调度下一个任务
    auto* next = Scheduler::schedule();
    Scheduler::set_current(next);

    // 恢复新任务的平台上下文
    if constexpr (PlatformCtx::supports_platform_context) {
        Contex& ctx = static_cast<Contex&>(*next);
        Scheduler::platform_context_policy::restore(&ctx);
    }

    // 返回新任务的栈顶指针（R4 地址）
    return reinterpret_cast<uint32_t*>(next->sp);
}

/**
 * @brief SysTick 中断处理函数
 * @details 由系统节拍定时器中断调用，通知调度器一个时钟节拍已过，
 *          用于时间片轮转和系统延时等。
 */
void SysTick_Handler(void) {
    strat_os::os_kernel::scheduler::tick();
}

/**
 * @brief 硬故障（HardFault）处理函数
 * @details 当 CPU 发生不可恢复的错误时被调用，读取故障状态寄存器并进入死循环。
 *          生产环境中可在此记录故障信息、复位系统或点亮故障指示灯。
 *
 * @note 当前注释掉了串口打印，避免在故障时依赖可能不稳定的硬件。
 *       用户可根据需要启用调试输出。
 */
void HardFault_Handler(void) {
    uint32_t cfsr  = SCB->CFSR;
    uint32_t hfsr  = SCB->HFSR;
    uint32_t mmfar = SCB->MMFAR;
    uint32_t bfar  = SCB->BFAR;
    // dxprintf("HardFault: CFSR=0x%X HFSR=0x%X MMFAR=0x%X BFAR=0x%X\n", cfsr, hfsr, mmfar, bfar);
    while (true) {}
}

} // extern "C"