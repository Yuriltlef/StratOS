/**
 * @file led_time_slice.cpp
 * @brief 双任务利用时间片轮转实现 LED 闪烁（PC13）
 */

#include "debug.hpp"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "strat_os.hpp"

/**
 * @file os_wrapper.cpp
 * @author StratOS Team
 * @brief 供 PendSV 汇编调用的 C 包装函数
 * @version 1.0.0
 * @date 2026-06-07
 *
 * @copyright Copyright (c) 2026 StratOS
 *
 * @details
 * 本文件实现了两个 `extern "C"` 函数，用于在 PendSV 异常处理程序中
 * 保存当前任务状态和获取下一个任务的栈指针，用户需要根据平台提供此函数
 *
 * 这些函数直接调用 `strat_os::os_kernel` 中的调度器适配器接口，
 * 并处理平台上下文的保存/恢复（若策略非空）。
 *
 * 函数签名与 `pend_sv_handler.s` 中的汇编调用严格匹配：
 * - scheduler_save_current(uint32_t* current_sp)
 * - scheduler_get_next(void)
 */

// #include "strat_os.hpp" // 包含 strat_os::os_kernel 定义
// #include "user/inc/debug.hpp"
#include <cstdint>

// 类型别名简化代码
using Kernel      = strat_os::os_kernel;
using Scheduler   = Kernel::scheduler;
using PlatformCtx = typename Scheduler::platform_context_policy::platform_context_type;
using EmptyBase   = strat_os::kernel::EmptyBase;

extern "C" {

/**
 * @brief 保存当前任务的状态（栈指针 + 平台上下文）
 * @param current_sp 当前任务栈顶指针（已包含 R4-R11 压栈后的值）
 *
 * 该函数由 PendSV 汇编调用，参数位于 r0 寄存器。
 * 它将栈顶存入当前 TCB 的 `sp` 字段，并调用平台上下文策略的 `save` 方法。
 */
void scheduler_save_current(uint32_t* current_sp) noexcept {
    auto* current = Scheduler::get_current();
    if (current) {
        current->sp = reinterpret_cast<std::uintptr_t>(current_sp);
        // 若平台上下文非空，则保存
        if constexpr (false) {
            PlatformCtx& ctx = static_cast<PlatformCtx&>(*current);
            Scheduler::platform_context_policy::save(&ctx);
        }
    }
}

/**
 * @brief 执行调度算法，返回下一个任务的栈顶指针
 * @return 下一个任务的栈顶指针（uint32_t*）
 *
 * 该函数调用调度器的 `schedule()` 获取下一个任务的 TCB 指针，
 * 更新当前任务指针，恢复新任务的平台上下文，并返回其栈顶地址。
 * 返回值将直接用于 PendSV 中的 `ldmia` 和 `msr psp` 指令。
 */
uint32_t* scheduler_get_next(void) noexcept {
    auto* next = Scheduler::schedule();
    Scheduler::set_current(next);
    uint32_t pc = *(reinterpret_cast<uint32_t*>(next->sp) + 8 + 6); // R0 + 6 words
    if constexpr (false) {
        PlatformCtx& ctx = static_cast<PlatformCtx&>(*next);
        Scheduler::platform_context_policy::restore(&ctx);
    } else {
    }
    dxprintf("get retuen next->sp = 0x%x, PC = 0x%X\n", next->sp, pc);
    dxprintf("current sp: 0x%x\n", Scheduler::get_current()->sp);
    return reinterpret_cast<uint32_t*>(next->sp);
}

void SysTick_Handler(void) {
    // 调用调度器的节拍处理
    strat_os::os_kernel::scheduler::tick();
}

void HardFault_Handler(void) {
    uint32_t cfsr  = SCB->CFSR;
    uint32_t hfsr  = SCB->HFSR;
    uint32_t mmfar = SCB->MMFAR;
    uint32_t bfar  = SCB->BFAR;
    dxprintf("HardFault: CFSR=0x%X HFSR=0x%X MMFAR=0x%X BFAR=0x%X\n", cfsr, hfsr, mmfar, bfar);
    while (1) {};
}

} // extern "C"

// 任务1：将 PC13 输出高电平（LED 灭）
class LedOffTask {
  public:
    void operator()() {
        while (true) {
            GPIO_SetBits(GPIOC, GPIO_Pin_13); // 高电平，LED 灭
                                              // 主动让出 CPU，但不使用延时，依赖时间片自动切换
                                              // 由于两个任务优先级相同且时间片轮转，调度器会自动切换任务
            dprint("I am task1!\n");
        }
    }
};

// 任务2：将 PC13 输出低电平（LED 亮）
class LedOnTask {
  public:
    void operator()() {
        while (true) {
            GPIO_ResetBits(GPIOC, GPIO_Pin_13); // 低电平，LED 亮
            dprint("I am task2!\n");
        }
    }
};

// 初始化 LED 引脚
void led_init() {
    GPIO_InitTypeDef gpio;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    gpio.GPIO_Pin   = GPIO_Pin_13;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &gpio);
    // 初始设为高电平（LED 灭）
    GPIO_SetBits(GPIOC, GPIO_Pin_13);
}
static LedOnTask on_task;
static LedOffTask off_task;

int main() {
    // 硬件初始化
    USART_Config();
    led_init();
    dxprintf("UserStack base = 0x%x, size = %u\n",
             (unsigned int)strat_os::kernel::details::UserStackStaticLayout::base,
             (unsigned int)strat_os::kernel::details::UserStackStaticLayout::size);

    using kernel = strat_os::os_kernel;

    dprint("init system...\n");
    // 内核初始化
    kernel::init();
    dprint("init success!\n");

    // 创建两个任务，优先级相同（例如 1），栈大小各 256 字节
    dprint("creat task1...\n");
    auto* tcb1 = kernel::create_task(on_task, 1, 1024);
    dprint("creat task1 success!\n");
    dxprintf("Task1: TCB=0x%x, sp=0x%x\n", tcb1, tcb1->sp);

    dprint("creat task2...\n");
    auto* tcb2 = kernel::create_task(off_task, 1, 1024);
    dxprintf("Task2: TCB=0x%x, sp=0x%x\n", tcb2, tcb2->sp);
    dprint("creat task2 success!\n");

    // 启动调度器（时间片轮转调度器会根据配置的时间片自动切换任务）
    dprint("start...\n");
    kernel::start();
    dprint("error: main\n");
    while (1) {};
}