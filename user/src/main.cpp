/**
 * @file led_time_slice.cpp
 * @brief 双任务利用时间片轮转实现 LED 闪烁（PC13）
 */

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "strat_os.hpp"
#include "user/libraries/test_log/inc/debug.hpp"


// 任务1：将 PC13 输出高电平（LED 灭）
class LedOffTask {
  public:
    void operator()() {
        while (true) {
            GPIO_SetBits(GPIOC, GPIO_Pin_13); // 高电平，LED 灭
            dprint("I am task1!\n");
        }
    }
};

// 任务2：将 PC13 输出低电平（LED 亮）
class LedOnTask {
  public:
    // 初始化硬件资源
    LedOnTask() {
        GPIO_InitTypeDef gpio;
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
        gpio.GPIO_Pin   = GPIO_Pin_13;
        gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
        gpio.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(GPIOC, &gpio);
        // 初始设为高电平（LED 灭）
        GPIO_SetBits(GPIOC, GPIO_Pin_13);
    }
    void operator()() {
        while (true) {
            GPIO_ResetBits(GPIOC, GPIO_Pin_13); // 低电平，LED 亮
            dprint("I am task2!\n");
        }
    }
};

LedOnTask on_task{};
LedOffTask off_task{};

int main() {

    using kernel = strat_os::os_kernel;

    dxprintf("UserStack base = 0x%x, size = %u\n",
             (unsigned int)strat_os::kernel::details::UserStackStaticLayout::base,
             (unsigned int)strat_os::kernel::details::UserStackStaticLayout::size);
    dprint("init system...\n");

    // 内核初始化
    kernel::init();

    dprint("init success!\n");

    // 创建两个任务，优先级相同（例如 1），栈大小各 1024 字节
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