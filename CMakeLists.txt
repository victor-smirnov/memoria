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
    set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -std=c++0x -Wall -fstrict-aliasing -Wstrict-aliasing=2 -Wfatal-errors -Wcomment -Wunused-variable") 
    
    if (CHECK_SYNTAX_ONLY)
        set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -fsyntax-only")
    endif()
else()
    set (MEMORIA_COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} /W3")
endif()

set_property (GLOBAL PROPERTY GLOBAL_SOURCES)

add_subdirectory(src/build-tools)

if (CHECK_SYNTAX_ONLY)
    set(CMAKE_AR "${EXECUTABLE_OUTPUT_PATH}/do-nothing")
    set(CMAKE_RANLIB "${EXECUTABLE_OUTPUT_PATH}/do-nothing")
    set(CMAKE_CXX_LINK_EXECUTABLE "${EXECUTABLE_OUTPUT_PATH}/do-nothing")
    set(CMAKE_C_LINK_EXECUTABLE "${EXECUTABLE_OUTPUT_PATH}/do-nothing")
endif()

add_subdirectory(src)