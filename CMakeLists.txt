
# Copyright 2016-2023 Victor Smirnov
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

cmake_minimum_required (VERSION 3.16)
project (Memoria)

set(VERSION "0.1.0")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Memoria.cmake)

set (MEMORIA_API_CXX_STANDARD 17)
set (MEMORIA_INTERNAL_CXX_STANDARD 20)

set(CMAKE_CXX_STANDARD ${MEMORIA_API_CXX_STANDARD})

if (NOT CMAKE_BUILD_TYPE)
     set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()


IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
    option(USE_ASAN "Compile with Address Sanitizer ON (Default with Debug build)" ON)
else()
    option(USE_ASAN "Compile with Address Sanitizer ON (Default with Debug build)" OFF)
ENDIF()

option(USE_TSAN "Compile with Thread Sanitizer ON" OFF)
option(USE_USAN "Compile with UB Sanitizer ON (Default with Debug build)" OFF)

option(BUILD_SANDBOX    "Build selected programs in the project's sandbox" OFF)
option(BUILD_EXAMPLES   "Build project examples" OFF)
option(BUILD_BENCHMARKS "Build project benchmarks" OFF)

option(BUILD_WITH_STATIC_BOOST     "Use static boost libraries" OFF)
option(BUILD_REUSE_CODEGEN_CONFIG  "Reuse previously generated codegen configuration (faster iteration)" OFF)

option(BUILD_COW_PROFILE        "Build CowProfile" ON)
option(BUILD_COW_LITE_PROFILE   "Build CowLiteProfile" ON)
option(BUILD_NO_COW_PROFILE     "Build NoCowProfile" ON)

option(BUILD_TESTS              "Build Unit/Functional/Integration/Randomized tests" ON)
option(BUILD_TESTS_PACKED       "Build Packed Structires tests" ON)
option(BUILD_TESTS_CONTAINERS   "Build Containers tests" ON)
option(BUILD_TESTS_SDN          "Build Hermes tests" ON)
option(BUILD_TESTS_DATATYPES    "Build Datatype tests" ON)
option(BUILD_TESTS_STORES       "Build Stores tests" ON)
option(BUILD_TESTS_HRPC         "Build HRPC Protocol tests" ON)

option(ICU_DEBUG   "Debug FindICU" OFF)
option(Boost_DEBUG "Debug FindBoost" OFF)

set (LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

add_definitions(-Wno-unknown-warning-option)

# FIXME: Must be target-specific (containers, mostly)!
add_definitions(-fstrict-aliasing)



find_package(Boost 1.74.0 COMPONENTS filesystem program_options serialization unit_test_framework REQUIRED)

if (BUILD_TESTS)
    find_package(Catch2 CONFIG REQUIRED)
endif()

find_package(Qt5 COMPONENTS Widgets QUIET)

set_property (GLOBAL PROPERTY GLOBAL_SOURCES)
file(GLOB_RECURSE MAIN_INCLUDES include/*.hpp)
set_property(GLOBAL APPEND PROPERTY GLOBAL_CORE_SOURCES ${MAIN_INCLUDES})
set_property(GLOBAL APPEND PROPERTY GLOBAL_CORE_SOURCES_SEASTAR ${MAIN_INCLUDES})

get_property(GLOBAL_CORE_SRCS GLOBAL PROPERTY GLOBAL_CORE_SOURCES)
get_property(GLOBAL_SEASTAR_SRCS GLOBAL PROPERTY GLOBAL_SEASTAR_SOURCES)

add_subdirectory(core)
add_subdirectory(seastar)
add_subdirectory(src)
