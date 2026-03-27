
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was MUSSTLConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

# 检查C++标准
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-std=c++17 HAS_CXX17)
if(NOT HAS_CXX17)
    message(FATAL_ERROR "µSTL requires C++17 support")
endif()

# 包含目标文件
include("${CMAKE_CURRENT_LIST_DIR}/MUSSTLTargets.cmake")

# 提供find_package兼容性
set(MUSSTL_FOUND TRUE)
set(MUSSTL_VERSION 0.1.0)
# 检查必需的定义
if(NOT DEFINED MUSSTL_NO_DYNAMIC_ALLOCATION)
    set(MUSSTL_NO_DYNAMIC_ALLOCATION 1)
endif()

# 提供使用函数
macro(MUSSTL_require_static_allocation)
    if(NOT MUSSTL_NO_DYNAMIC_ALLOCATION)
        message(FATAL_ERROR 
            "MUSSTL requires static allocation. "
            "Define MUSSTL_NO_DYNAMIC_ALLOCATION=1"
        )
    endif()
endmacro()

macro(MUSSTL_require_no_exceptions)
    if(NOT MUSSTL_NO_EXCEPTIONS)
        message(FATAL_ERROR 
            "MUSSTL requires no exceptions. "
            "Define MUSSTL_NO_EXCEPTIONS=1"
        )
    endif()
endmacro()
