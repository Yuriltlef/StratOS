/**
 * @file init.cpp
 * @author StratOS Team
 * @brief 定义全局静态变量
 * @version 1.0.0
 * @date 2026-06-13
 *
 * @copyright Copyright (c) 2026
 *
 * @details
 * 由于arm none g++ 编译器实现bug，类内联静态变量可能会违反ODR原则
 * 策略包编写者应该提供编写此文件的文档，用来定义全局强符号。
 *
 */

#include "os_config.hpp"
#include "strat_os.hpp"

namespace strat_os::kernel::policy::builtin
{
using os_cfg = user_config::OsConfig;

template <>
os_cfg::Kerneltypes::task_id_type os_cfg::TaskPolicy::tid = 0;

template <>
os_cfg::SchedulerPolicy::tcb_type* os_cfg::SchedulerPolicy::current_task = nullptr;

template <>
os_cfg::SchedulerPolicy::tick_type os_cfg::SchedulerPolicy::time_slice_ticks = os_cfg::SchedulerPolicy::default_slice;

template <>
os_cfg::SchedulerPolicy::tick_type* os_cfg::SchedulerPolicy::time_left = nullptr;

} // namespace strat_os::kernel::policy::builtin

namespace strat_os::kernel::details
{
using kernel = strat_os::os_kernel;

template <>
std::uintptr_t kernel::kernel_stack_pool::Policy::next_free = kernel::kernel_stack_pool::Policy::base;

template <>
std::uintptr_t kernel::kernel_pool::Policy::free_list = kernel::kernel_pool::Policy::base;

template <>
std::uintptr_t kernel::user_stack_pool::Policy::next_free = kernel::user_stack_pool::Policy::base;

template <>
std::uintptr_t kernel::user_pool::Policy::free_list = kernel::user_pool::Policy::base;

} // namespace strat_os::kernel::details

namespace strat_os::kernel::policy::builtin::details
{
using kernel = strat_os::os_kernel;

template <>
kernel::scheduler::Policy::task_lists::ready_list_type* kernel::scheduler::Policy::task_lists::ready_list = nullptr;

template <>
kernel::scheduler::Policy::task_lists::tcb* kernel::scheduler::Policy::task_lists::idle_task = nullptr;

} // namespace strat_os::kernel::policy::builtin::details