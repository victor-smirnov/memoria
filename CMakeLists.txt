cmake_minimum_required (VERSION 2.8.5)
project (Memoria)

# Gcc/MinGW/MSVC are supported
#
# Intel C++ for Windows is supported as MSVC variant but not fully tested.
# Use variables to disable components which are fail to build.
# Intel C++ for Linux is to be done
#

#TODO Use cmake options 

# available variables to be used via -D switch in command line:
# 
# BUILD_STREAM_CONTAINER           build StreamContainer
# BUILD_GENERIC_CONTAINER          build GenericContainer (disabled by default)
# 
# CHECK_SYNTAX_ONLY                do not produce binaries, only check syntax (GCC/MinGW)
#
# MEMORIA_COMPILE_FLAGS            Additional flags for compiler
# MEMORIA_LINK_FLAGS               Additional flags for linker
# 
# BUIDL_TOOLS                      Build tools (default: true)
# BUILD_TESTS                      Build tests (default: true)
#
# MEMORIA_TESTS                    Enumeration of tests to be compiled. See src/tests/CmakeLists.txt for defaults.
# MEMORIA_TOOLS                    Enumeration of tests to be compiled. See src/tools/CmakeLists.txt for defaults.
#
# EMIT_LLVM                        Emit LLVM bitcode for executables using clang compiler
# BUILD_ASAN                       Build with address sanitizer enabled




if (NOT DEFINED BUILD_STREAM_CONTAINER)
    set (BUILD_STREAM_CONTAINER true)
endif()

if (NOT DEFINED BUILD_GENERIC_CONTAINER)
    set (BUILD_GENERIC_CONTAINER false)
endif ()

STRING(FIND ${CMAKE_CXX_COMPILER} "icl.exe" ICL_COMPILER_WIN_POS REVERSE)
STRING(FIND ${CMAKE_CXX_COMPILER} "clang"   CLANG_COMPILER_POS REVERSE)

if (ICL_COMPILER_WIN_POS EQUAL -1)
    set (INTEL_COMPILER false)
else()
    set (INTEL_COMPILER true)     
endif()

if (CLANG_COMPILER_POS EQUAL -1)
    set (CLANG_COMPILER false)
else()
    set (CLANG_COMPILER true)
endif()

if (NOT DEFINED EMIT_LLVM)
    set(EMIT_LLVM false)
endif()

if (NOT DEFINED BUILD_ASAN)
    set(BUILD_ASAN false)
endif()

if (NOT DEFINED EMIT_AST)
    set(EMIT_AST false)
endif()

if (UNIX AND NOT CYGWIN)
	set (BUILD_UNIX true)
else()
	set (BUILD_UNIX false)
endif()

if (MSVC OR INTEL_COMPILER)
	set (BUILD_MSVC true)
else()
	set (BUILD_MSVC false)
endif()


set (LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set (EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

if (NOT DEFINED CHECK_SYNTAX_ONLY)
    set (CHECK_SYNTAX_ONLY false)
endif()


if (CMAKE_COMPILER_IS_GNUCXX OR CLANG_COMPILER OR MINGW)

    set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}  -std=c++11 -Wall ")
    set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}  -fstrict-aliasing -Wstrict-aliasing=2 -Wfatal-errors -ftemplate-backtrace-limit=0 ")
    set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}  -Wcomment -Wunused-variable")
    
    set (MEMORIA_LINK_FLAGS "${MEMORIA_LINK_FLAGS} -lstdc++ ")
    
    if (BUILD_ASAN AND NOT EMIT_LLVM)
        set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -fsanitize=address")
        set (MEMORIA_LINK_FLAGS "${MEMORIA_LINK_FLAGS} -fsanitize=address")
    endif()
    
else()
    set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} /W3")
endif()

if (EMIT_LLVM)
    if (EMIT_AST)
        message(FATAL_ERROR "EMIT_LLVM can't be used with EMIT_AST")
    endif()

    if (CLANG_COMPILER)
        set (MEMORIA_COMPILE_FLAGS "-emit-llvm -fno-use-cxa-atexit -fexceptions ${MEMORIA_COMPILE_FLAGS}")
    
        set(CMAKE_AR "llvm-ar")
        set(CMAKE_RANLIB "llvm-ranlib")
    
        set(CMAKE_CXX_LINK_EXECUTABLE "llvm-link <OBJECTS> -o <TARGET>.bc")
        set(CMAKE_C_LINK_EXECUTABLE "llvm-link <OBJECTS> -o <TARGET>.bc")
    else()
        message(FATAL_ERROR "EMIT_LLVM is supported only for clang compiler")
    endif()
endif()


if (EMIT_AST)
    if (NOT CLANG_COMPILER)
        message(FATAL_ERROR "EMIT_AST is supported only for clang compiler")
    endif()
endif()

set_property (GLOBAL PROPERTY GLOBAL_SOURCES)

add_subdirectory(src/build-tools)

add_subdirectory(src)