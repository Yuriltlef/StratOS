option(MUSSTL_BUILD_TESTS "Build tests" OFF)
option(MUSSTL_BUILD_EXAMPLES "Build examples" OFF)
option(MUSSTL_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(MUSSTL_INSTALL "Generate install target" ON)

# 嵌入式系统配置选项
option(MUSSTL_NO_EXCEPTIONS "Disable exception handling" ON)
option(MUSSTL_NO_RTTI "Disable RTTI" ON)
option(MUSSTL_STATIC_ALLOCATION "Force static allocation only" ON)

# 测试开关
option(BUILD_TESTS "All tests." ON)

set(MUSSTL_USER_INSTALL_PREFIX "" CACHE PATH "Custom install prefix for MUSSTL")