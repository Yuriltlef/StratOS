cmake_minimum_required(VERSION 3.25)

set(test ${OS_ELF_TARGET}_test)

add_executable(${test} ${USER_SRC})

configure_user_target_include(${test} 
    "${CMAKE_SOURCE_DIR}/user/libraries/test_inc"
)

configure_target_output(${test} ${PROJECT_OUTPUT_DIR})
