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


static mu_sstl::StaticArray<mu_sstl::StaticAllocPolicy<std::uint32_t, std::uint32_t, 100>> arr;

int main() {
    arr.fill(42); // 使用 StaticArray 的 fill 方法初始化数组
    while (1) {
    }
}
