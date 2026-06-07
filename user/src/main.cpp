/**
 * @file led_time_slice.cpp
 * @brief 双任务利用时间片轮转实现 LED 闪烁（PC13）
 */

#include "debug.hpp"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "strat_os.hpp"

// 任务1：将 PC13 输出高电平（LED 灭）
class LedOffTask {
  public:
    void operator()() {
        while (true) {
            GPIO_SetBits(GPIOC, GPIO_Pin_13); // 高电平，LED 灭
            dprint("task1!\n");
            // 主动让出 CPU，但不使用延时，依赖时间片自动切换
            // 由于两个任务优先级相同且时间片轮转，调度器会自动切换任务
        }
    }
};

// 任务2：将 PC13 输出低电平（LED 亮）
class LedOnTask {
  public:
    void operator()() {
        while (true) {
            GPIO_ResetBits(GPIOC, GPIO_Pin_13); // 低电平，LED 亮
            dprint("task2!\n");
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

    using kernel = strat_os::os_kernel;

    dprint("init system...\n");
    // 内核初始化
    kernel::init();
    dprint("init success!\n");

    GPIO_ResetBits(GPIOC, GPIO_Pin_13); // 低电平，LED 亮

    // 创建两个任务，优先级相同（例如 1），栈大小各 256 字节

    dprint("creat task1...\n");
    kernel::create_task(on_task, 1, 256);
    dprint("creat task1 success!\n");

    dprint("creat task2...\n");
    kernel::create_task(off_task, 1, 256);
    dprint("creat task2 success!\n");

    // 启动调度器（时间片轮转调度器会根据配置的时间片自动切换任务）
    dprint("start...\n");
    kernel::start();

    // 不会执行到这里
    while (1) {};

strat_os::kernel::policy::builtin::details::TaskLists<strat_os::kernel::policy::builtin::KernelTypesPolicy, strat_os::hal::policy::builtin::CortexM3Stm32F1PlatformContextPolicy, strat_os::kernel::policy::builtin::UserTcbDataPolicy, (strat_os::config::MemoryLayoutType)0, 16ul, 128ul>::ready_list;
strat_os::kernel::policy::builtin::details::TaskLists<strat_os::kernel::policy::builtin::KernelTypesPolicy, strat_os::hal::policy::builtin::CortexM3Stm32F1PlatformContextPolicy, strat_os::kernel::policy::builtin::UserTcbDataPolicy, (strat_os::config::MemoryLayoutType)0, 16ul, 128ul>::ready_list;
}