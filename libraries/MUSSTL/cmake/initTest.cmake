include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/logUtils.cmake)

function(init_test test_option)
    if(test_option)
        # 保存当前策略状态
        cmake_policy(PUSH)
    
        if(POLICY CMP0048)
            cmake_policy(SET CMP0048 NEW)
        endif()
        if(POLICY CMP0077)
            cmake_policy(SET CMP0077 NEW)
        endif()
        enable_testing()
        include(CTest)

        include(FetchContent)
        # 导入Doctest
        cmake_minimum_required(VERSION 3.5...3.15)
        FetchContent_Declare(
            doctest
            GIT_REPOSITORY https://github.com/doctest/doctest.git
            GIT_TAG v2.4.12
            )
        log_info("Adding libraries: doctest...")
        FetchContent_MakeAvailable(doctest)
        
        log_info("doctest is available.")
        
        add_subdirectory(tests)
        log_info("All tests is now loaded.")
        log_info("Generating done.")
        
        cmake_minimum_required(VERSION 3.15)
        # 恢复策略
        cmake_policy(POP)

    endif()
endfunction()
