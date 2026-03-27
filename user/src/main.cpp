/**
 ******************************************************************************
 * @file    main.cpp
 * @author  Yurilt
 * @version V1.0.0
 * @date    03-November-2025
 * @brief   C++主程序入口
 * @note    包含C++程序的main函数和对象初始化
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 Yurilt.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include "mu_sstl/containers/static_array.hpp"
#include <cstdint>

constexpr static mu_sstl::StaticArray<mu_sstl::StaticAllocPolicy<std::uint32_t, std::uint32_t, 1000>> arr{};

int main() {
    auto i = arr.max_size();
    while (true) {
        for (auto i_ : arr) {
            i = i_;
        }
    }
}
