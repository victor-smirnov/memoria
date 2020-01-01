
# Copyright 2016 Victor Smirnov
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# available variables to be used via -D switch in command line:
# 
# MEMORIA_COMPILE_FLAGS            Additional flags for compiler
# MEMORIA_LINK_FLAGS               Additional flags for linker

cmake_minimum_required (VERSION 3.6)
project (Memoria)

set(VERSION "0.1.0")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Memoria.cmake)

set (MEMORIA_API_CXX_STANDARD 14)
set (MEMORIA_INTERNAL_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD ${MEMORIA_API_CXX_STANDARD})

if (NOT CMAKE_BUILD_TYPE)
     set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()


IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
    option(USE_ASAN "Compile with Address Sanitizer ON (Default with Debug build)" ON)
else()
    option(USE_ASAN "Compile with Address Sanitizer ON (Default with Debug build)" OFF)
ENDIF()

option(ASAN_SANITIZE_STACKS "Compile with fiber stacks sanitizing ON (If supported, implies ASAN)" ON)

option(USE_TSAN "Compile with Thread Sanitizer ON" OFF)
option(USE_USAN "Compile with UB Sanitizer ON (Default with Debug build)" OFF)


option(BUILD_TESTS      "Build Unit/Functional tests" OFF)
option(BUILD_SANDBOX    "Build selected programs in the project's sandbox" OFF)
option(BUILD_STATIC     "Build statically linked binaries" OFF)
option(BUILD_EXAMPLES   "Build project examples" OFF)
option(BUILD_BENCHMARKS "Build project benchmarks" OFF)
option(BUILD_WITH_BACKTRACE "Include backtrace suport on Linux" OFF)
option(BUILD_WITH_STATIC_BOOST "Use static boost libraries" OFF)
option(BUILD_EMIT_LLVM  "Emit LLVM bytecode for compiled binaries. Implies Clang is the compiler." OFF)

option(ICU_DEBUG        "Debug FindICU" OFF)
option(Boost_DEBUG      "Debug FindBoost" OFF)

option(BUILD_CLASSIC_MEMORIA_LIB "Build core library without Reactor support, using threads for concurrency" OFF)
option(BUILD_REACTOR_MEMORIA_LIB "Build core library WITH Reactor support, using fibers for concurrency" ON)


set (LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

set (BUILD_POSIX  FALSE)
set (BUILD_MSVC   FALSE)
set (BUILD_MACOSX FALSE)

if(APPLE)
    set (BUILD_MACOSX TRUE)
    set (BUILD_POSIX  TRUE)
elseif (UNIX)
    set (BUILD_POSIX  TRUE)
elseif(MSVC)
    set (BUILD_MSVC   TRUE)
else()
    message(FATAL_ERROR "This platform is not explicitly supported")
endif()

# Global definitions

add_definitions(-DBOOST_NO_AUTO_PTR -DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE -DJEMALLOC_NO_PRIVATE_NAMESPACE)

if (BUILD_POSIX)

    if (BUILD_EMIT_LLVM)
        set (CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -flto -fuse-ld=lld -Wl,-save-temps")
        add_definitions(-Wl,-save-temps -flto)
    endif()

    add_definitions(-Wall -Wno-sign-compare -fPIC -DPIC -D_GNU_SOURCE -D_REENTRANT -Wno-unused-command-line-argument)
    add_definitions(-Wno-old-style-cast)

    if (BUILD_MACOSX)
        add_definitions(-DBOOST_STACKTRACE_USE_ADDR2LINE)
    else()
        if (BUILD_WITH_BACKTRACE)
            add_definitions(-DBOOST_STACKTRACE_USE_BACKTRACE)
            add_global_libs(backtrace)
        endif()

        #add_global_linker(-stdlib=libstdc++)
        #add_global_linker(-static-libstdc++)
    endif()

    add_definitions(-fstrict-aliasing -Wstrict-aliasing=2 -Wfatal-errors  -ftemplate-backtrace-limit=0)
    add_definitions(-Wno-comment -Wno-unknown-warning-option -Wunused-variable)
    add_definitions(-Wno-unused-function -Wno-unused-but-set-variable)
    add_definitions(-Wno-unused-private-field -Wno-pessimizing-move)
    add_definitions(-Wno-missing-braces)
    add_definitions(-Wno-variables)
    add_definitions(-Werror=unused-result)
    add_definitions(-faligned-new)
    add_definitions(-Wno-class-memaccess)

    add_definitions(-DMMA1_ICU_CXX_NS=icu_61)
    #add_definitions(-DU_HAVE_LIB_SUFFIX=1)
    #add_definitions(-DU_LIB_SUFFIX_C_NAME=__mariana)
    
    add_definitions(-pthread)
    
    if (USE_ASAN)
        add_definitions(-fsanitize=address -DASAN_ENABLED)
        add_global_linker(-fsanitize=address)

        if (ASAN_SANITIZE_STACKS)
            add_definitions(-DMMA1_SANITIZE_STACKS)
        endif() 
    endif()
    
    if (USE_USAN) 
        add_definitions(-fsanitize=undefined)
        add_global_linker(-fsanitize=undefined)
    endif()
    
    if (USE_TSAN) 
        add_definitions(-fsanitize=thread)
        add_global_linker(-fsanitize=thread)
    endif()
    
    add_global_libs(dl pthread)


    if (BUILD_STATIC)
        add_definitions(-static)
        add_global_linker(-static) #-static-libgcc -static-libstdc++
    endif()
else()
    add_definitions(/W3 /std:c++14 -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DJEMALLOC_NO_PRIVATE_NAMESPACE -D_REENTRANT)
    add_definitions(-DDLLEXPORT -DNOMINMAX  /bigobj)
endif()


set (Boost_USE_DEBUG_LIBS	 ON)
set (Boost_USE_RELEASE_LIBS	 ON)
set (Boost_USE_DEBUG_RUNTIME	 ON)
set (Boost_USE_MULTITHREADED     ON)

if (MSVC)
    set (Boost_USE_STATIC_RUNTIME    OFF)
    set (Boost_USE_STATIC_LIBS       ON)
    find_package(ICU 60.2 COMPONENTS dt i18n uc REQUIRED)
else()
    set (Boost_USE_STATIC_RUNTIME    ${BUILD_WITH_STATIC_BOOST})
    set (Boost_USE_STATIC_LIBS       ${BUILD_WITH_STATIC_BOOST})
    find_package(ICU 60.2 COMPONENTS i18n uc data REQUIRED)
endif()

find_package(Boost 1.66.0 COMPONENTS filesystem context program_options serialization REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})
add_global_libs(${Boost_LIBRARIES})


include_directories(${ICU_INCLUDE_DIRS})
link_directories(${ICU_LIBRARY_DIRS})
add_global_libs(${ICU_LIBRARIES})

find_package(absl CONFIG REQUIRED)
link_libraries(absl::base absl::strings absl::hash absl::meta absl::time)

if (BUILD_TESTS)
    find_package(yaml-cpp CONFIG REQUIRED)
endif()

find_package(Qt5 COMPONENTS Widgets QUIET)

set_property (GLOBAL PROPERTY GLOBAL_SOURCES)

add_subdirectory(src)
