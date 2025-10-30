# Set the minimum required version of CMake for this project.
cmake_minimum_required(VERSION 3.10)

# Set the target type for try_compile() to be a static library.
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

set(CMAKE_BUILD_TYPE Debug)

# Define the project name and specify that it's a C++ project.
project(UchosKernel)

# Set the C++ standard to use.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the compiler to be used.
set(CMAKE_CXX_COMPILER /usr/bin/clang++)
set(CMAKE_C_COMPILER /usr/bin/clang)

# Set the linker to be used.
set(CMAKE_LINKER /usr/bin/ld.lld)

# Define base directories for ease of reference.
set(BASEDIR "../x86_64-elf")
set(EDK2DIR "~/edk2")
set(ROOTDIR "../")

# Disable the use of rpath.
set(CMAKE_SKIP_RPATH TRUE)
# Enable NASM assembly language support.
enable_language(ASM_NASM)
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <INCLUDES> -f elf64 -o <OBJECT> <SOURCE>")
set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -f elf64")

# Specify the include directories for the compiler.
include_directories(
        ${ROOTDIR}
        ${CMAKE_SOURCE_DIR}
        ${BASEDIR}/include/c++/v1
        ${BASEDIR}/include
        ${BASEDIR}/include/freetype2
        ${EDK2DIR}/MdePkg/Include
        ${EDK2DIR}/MdePkg/Include/X64
)

message(STATUS "Including LIBDIR: ${CMAKE_SOURCE_DIR}")

# Add compile options for both C and C++.
add_compile_options(
        -O0
        -Wall
        -g
        --target=x86_64-elf
        -ffreestanding
        -mno-red-zone
        -nostdlibinc
        -Wno-interrupt-service-routine
        -D__ELF__
        -D_LDBL_EQ_DBL
        -D_GNU_SOURCE
        -D_POSIX_TIMERS
)

# Add specific compile options for C++.
add_compile_options(
        $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
)

# Specify the linker to be used and its options.
set(CMAKE_CXX_LINK_EXECUTABLE "ld.lld <LINK_FLAGS> <OBJECTS>  -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_C_LINK_EXECUTABLE "ld.lld <LINK_FLAGS> <OBJECTS>  -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --entry KernelMain -z norelro --image-base 0x100000 --static -z separate-code")

# Specify the linker directories.
link_directories(${BASEDIR}/lib)

# Define the source files for the main target.
set(SOURCE_FILES
        main.cpp
        newlib_support.c
        libcxx_support.cpp
        asm_utils.asm
        bit_utils.cpp
        elf.cpp
        list.cpp
        panic.cpp
)

# Add subdirectories that contain their own CMakeLists.txt files.
add_subdirectory(memory)
add_subdirectory(graphics)
add_subdirectory(interrupt)
add_subdirectory(timers)
add_subdirectory(task)
add_subdirectory(hardware)
add_subdirectory(fs)
add_subdirectory(syscall)
add_subdirectory(net)
add_subdirectory(tests)

# Define the main target and link it with the necessary libraries.
add_executable(UchosKernel ${SOURCE_FILES})

target_link_libraries(
        UchosKernel
        c
        c++
        c++abi
        freetype
        m
        UchosGraphics
        UchosMemory
        UchosInterrupt
        UchosTimers
        UchosTask
        UchosHardware
        UchosFileSystem
        UchosSyscall
        UchosNet
        UchosTest
)
