/**
 * @file kernel_handler_id.hpp
 * @author YT_Minro (yurilt15312@outlook.com)
 * @brief
 * @version 0.1
 * @date 2026-03-15
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#ifndef MU_SSTL_KERNEL_HANDLER_ID_HPP
#define MU_SSTL_KERNEL_HANDLER_ID_HPP

#include <cstdint> /// for uint32_t

namespace mu_sstl
{
namespace kernel_handler_id
{
constexpr std::uint32_t base_array_handler_id{0x0000};          ///< 静态数组错误处理器 ID
constexpr std::uint32_t base_ring_buffer_handler_id{0x0001};    ///< 环形缓冲区错误处理器 ID
constexpr std::uint32_t base_bitmap_handler_id{0x0002};         ///< 位图错误处理器 ID
constexpr std::uint32_t base_intrusive_list_handler_id{0x0003}; ///< 内部链表错误处理器 ID
constexpr std::uint32_t base_priority_queue_handler_id{0x0004}; ///< 优先级队列错误处理器 ID

} // namespace kernel_handler_id
} // namespace mu_sstl

#endif // MU_SSTL_KERNEL_HANDLER_ID_HPP