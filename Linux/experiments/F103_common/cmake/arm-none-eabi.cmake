# Cortex-M3 裸机交叉编译工具链文件
#
# 用法：
#   cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=../F103_common/cmake/arm-none-eabi.cmake
#
# 工具链定位顺序：
#   1. -DARM_TOOLCHAIN_DIR=<包含 arm-none-eabi-gcc 的目录>
#   2. 环境变量 ARM_TOOLCHAIN_DIR
#   3. PATH 中的 arm-none-eabi-gcc
#   4. Arm 官方 Windows 安装器的默认布局（Program Files 下的版本化目录）

set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 交叉编译不产生可在本机运行的程序；试编译阶段只做到能生成目标文件即可
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

if(NOT ARM_TOOLCHAIN_DIR AND DEFINED ENV{ARM_TOOLCHAIN_DIR})
  set(ARM_TOOLCHAIN_DIR "$ENV{ARM_TOOLCHAIN_DIR}")
endif()

if(ARM_TOOLCHAIN_DIR)
  find_program(F103_ARM_GCC arm-none-eabi-gcc HINTS "${ARM_TOOLCHAIN_DIR}" NO_DEFAULT_PATH)
else()
  find_program(F103_ARM_GCC arm-none-eabi-gcc)
  if(NOT F103_ARM_GCC)
    file(GLOB _f103_gcc_candidates
      "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/*/bin/arm-none-eabi-gcc.exe"
      "C:/Program Files/Arm GNU Toolchain arm-none-eabi/*/bin/arm-none-eabi-gcc.exe")
    list(SORT _f103_gcc_candidates)
    if(_f103_gcc_candidates)
      list(GET _f103_gcc_candidates 0 F103_ARM_GCC)
    endif()
  endif()
endif()

if(NOT F103_ARM_GCC)
  message(FATAL_ERROR
    "找不到 arm-none-eabi-gcc。请把工具链 bin 目录加入 PATH，"
    "或传 -DARM_TOOLCHAIN_DIR=<bin 目录>。")
endif()

get_filename_component(ARM_TOOLCHAIN_DIR "${F103_ARM_GCC}" DIRECTORY)
# Windows 上探测到的是 arm-none-eabi-gcc.exe；拼装其余工具时要沿用同一扩展名
get_filename_component(_f103_exe_suffix "${F103_ARM_GCC}" EXT)
message(STATUS "F103 工具链目录: ${ARM_TOOLCHAIN_DIR}")

set(CMAKE_C_COMPILER   "${ARM_TOOLCHAIN_DIR}/arm-none-eabi-gcc${_f103_exe_suffix}")
set(CMAKE_ASM_COMPILER "${ARM_TOOLCHAIN_DIR}/arm-none-eabi-gcc${_f103_exe_suffix}")
set(CMAKE_OBJCOPY      "${ARM_TOOLCHAIN_DIR}/arm-none-eabi-objcopy${_f103_exe_suffix}")
set(CMAKE_SIZE         "${ARM_TOOLCHAIN_DIR}/arm-none-eabi-size${_f103_exe_suffix}")

# 裸机无操作系统：CMake 不在目标路径下搜索库和头文件
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
