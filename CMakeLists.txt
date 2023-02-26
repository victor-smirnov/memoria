
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

SET(MEMORIA_HAS_TOP_LEVEL ON)

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
add_subdirectory(mbt)


message(INFO "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Configuring MBT %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%")
execute_process(COMMAND ${CMAKE_COMMAND}
    -Wno-dev
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_TOOLCHAIN_FILE:INTERNAL=${CMAKE_TOOLCHAIN_FILE}
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -S ${CMAKE_SOURCE_DIR}/mbt
    -B ${CMAKE_BINARY_DIR}/mbt-explicit
    -G ${CMAKE_GENERATOR}

    COMMAND_ERROR_IS_FATAL ANY
)

execute_process(
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/mbt-explicit
    COMMAND_ERROR_IS_FATAL ANY
)

find_package(Clang CONFIG REQUIRED)

SET (MMA_CLANG_INCLUDES_DIR ${CLANG_INCLUDE_DIRS})

if (BUILD_EXTRA_CONTAINERS)
    set(BUILD_EXTRA_CONTAINERS_OPT "MEMORIA_BUILD_EXTRA_CONTAINERS")
else()
    set(BUILD_EXTRA_CONTAINERS_OPT "MEMORIA_NONE")
endif()

if (BUILD_COW_PROFILE)
    set (BUILD_COW_PROFILE_OPT "--enable")
else()
    set (BUILD_COW_PROFILE_OPT "--disable")
endif()

if (BUILD_COW_LITE_PROFILE)
    set (BUILD_COW_LITE_PROFILE_OPT "--enable")
else()
    set (BUILD_COW_LITE_PROFILE_OPT "--disable")
endif()

if (BUILD_NO_COW_PROFILE)
    set (BUILD_NO_COW_PROFILE_OPT "--enable")
else()
    set (BUILD_NO_COW_PROFILE_OPT "--disable")
endif()

execute_process(
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND
        ${CMAKE_BINARY_DIR}/mbt-explicit/mbt
        --dry-run
        -I ${CMAKE_SOURCE_DIR}/include
        -I ${CMAKE_SOURCE_DIR}/core/lib/include
        -I ${CMAKE_SOURCE_DIR}/codegen/include
        -I ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux/include
        -I ${MMA_CLANG_INCLUDES_DIR}/c++/v1
        -D MMA_ICU_CXX_NS=icu
        -D ${BUILD_EXTRA_CONTAINERS_OPT}
        ${BUILD_COW_LITE_PROFILE_OPT} CowLiteProfile
        ${BUILD_COW_PROFILE_OPT} CowProfile
        ${BUILD_NO_COW_PROFILE_OPT} NoCowProfile
        --config codegen_memoria.hpp
        --project-output ${CMAKE_BINARY_DIR}/generated
        --components-output-base ${CMAKE_BINARY_DIR}

    OUTPUT_VARIABLE GENERATED_SOURCES
    COMMAND_ERROR_IS_FATAL ANY
)

message ("========================  MBT GENERATED SOURCES OUTPUT: |${GENERATED_SOURCES}|============================")


SET (GENERATED_MAIN_SOURCES)
SET (GENERATED_STORE_SOURCES)

SET (GENERATED_MAIN_BYPRODUCTS)
SET (GENERATED_STORE_BYPRODUCTS)
SET (GENERATED_OTHER_BYPRODUCTS)
SET (ACTIVE_PROFILES)

foreach(FNAME ${GENERATED_SOURCES})
    if (FNAME MATCHES "^BYPRODUCT:")
        string(SUBSTRING ${FNAME} 10 -1 FNAME_OUT)
        if (FNAME_OUT MATCHES "src/containers")
            list(APPEND GENERATED_MAIN_BYPRODUCTS ${FNAME_OUT})
        elseif (FNAME_OUT MATCHES "src/stores")
            list(APPEND GENERATED_STORE_BYPRODUCTS ${FNAME_OUT})
        else()
            list(APPEND GENERATED_OTHER_BYPRODUCTS ${FNAME_OUT})
        endif()
    endif()

    if (FNAME MATCHES "^SOURCE:")
        string(SUBSTRING ${FNAME} 7 -1 FNAME_OUT)
        list(APPEND GENERATED_CTR_SOURCES ${FNAME_OUT})

        if (FNAME_OUT MATCHES "src/containers")
            list(APPEND GENERATED_MAIN_SOURCES ${FNAME_OUT})
        elseif (FNAME_OUT MATCHES "src/stores")
            list(APPEND GENERATED_STORE_SOURCES ${FNAME_OUT})
        else()
            message(FATAL_ERROR "Unknown destination for generated source: ${FNAME_OUT}")
        endif()
    endif()

    if (FNAME MATCHES "^PROFILE:")
        string(SUBSTRING ${FNAME} 8 -1 FNAME_OUT)
        list(APPEND ACTIVE_PROFILES ${FNAME_OUT})
    endif()
endforeach()

list(FIND ACTIVE_PROFILES "CowLiteProfile" COW_LITE_PROFILE_IDX)
if (COW_LITE_PROFILE_IDX EQUAL -1)
    set(COW_LITE_PROFILE OFF)
else()
    set(COW_LITE_PROFILE ON)
    add_definitions(-DMEMORIA_COW_LITE_PROFILE)
endif()

list(FIND ACTIVE_PROFILES "CowProfile" COW_PROFILE_IDX)
if (COW_PROFILE_IDX EQUAL -1)
    set(COW_PROFILE OFF)
else()
    set(COW_PROFILE ON)
    add_definitions(-DMEMORIA_COW_PROFILE)
endif()

list(FIND ACTIVE_PROFILES "NoCowProfile" NO_COW_PROFILE_IDX)
if (NO_COW_PROFILE_IDX EQUAL -1)
    set(NO_COW_PROFILE OFF)
else()
    set(NO_COW_PROFILE ON)
    add_definitions(-DMEMORIA_NO_COW_PROFILE)
endif()

message ("ActiveProfiles:")
message ("\tCowLiteProfile: ${COW_LITE_PROFILE}")
message ("\tCowProfile: ${COW_PROFILE}")
message ("\tNoCowProfile: ${NO_COW_PROFILE}")
message ("")

# For debugging
message ("======================== MBT  MAIN SRC: |${GENERATED_MAIN_SOURCES}|============================")
message ("======================== MBT STORE SRC: |${GENERATED_STORE_SOURCES}|===========================")

#message ("======================== MBT  MAIN BP: |${GENERATED_MAIN_BYPRODUCTS}|============================")
#message ("======================== MBT STORE BP: |${GENERATED_STORE_BYPRODUCTS}|===========================")
#message ("======================== MBT OTHER BP: |${GENERATED_OTHER_BYPRODUCTS}|===========================")

message ("======================== MBT ACTIVE PROFILES: |${ACTIVE_PROFILES} |===========================")

if (BUILD_REUSE_CODEGEN_CONFIG)
    SET(BUILD_REUSE_CODEGEN_CONFIG_OPT "--reuse-config")
endif()

message ("Reusing codegen config is ${REUSE_GENERATED_SOURCES}")

add_custom_command(
    IMPLICIT_DEPENDS C++ codegen/include/codegen_memoria.hpp
    OUTPUT ${GENERATED_MAIN_SOURCES} ${GENERATED_STORE_SOURCES}
    DEPENDS ${CODEGEN_DEPS}
    BYPRODUCTS ${GENERATED_MAIN_BYPRODUCTS} ${GENERATED_STORE_BYPRODUCTS} ${GENERATED_OTHER_BYPRODUCTS}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND
        ${CMAKE_BINARY_DIR}/mbt-explicit/mbt
        -I ${CMAKE_SOURCE_DIR}/include
        -I ${CMAKE_SOURCE_DIR}/core/lib/include
        -I ${CMAKE_SOURCE_DIR}/codegen/include
        -I ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux/include
        -I ${MMA_CLANG_INCLUDES_DIR}/c++/v1
        -D MMA_ICU_CXX_NS=icu
        -D ${BUILD_EXTRA_CONTAINERS_OPT}
        ${BUILD_COW_LITE_PROFILE_OPT} CowLiteProfile
        ${BUILD_COW_PROFILE_OPT} CowProfile
        ${BUILD_NO_COW_PROFILE_OPT} NoCowProfile
        --config codegen_memoria.hpp
        --project-output ${CMAKE_BINARY_DIR}/generated
        --components-output-base ${CMAKE_BINARY_DIR}
        ${BUILD_REUSE_CODEGEN_CONFIG_OPT}

    COMMAND_ERROR_IS_FATAL ANY
)

add_library(MemoriaCtrs STATIC)
target_sources(MemoriaCtrs PRIVATE ${GENERATED_MAIN_SOURCES})
target_link_libraries(MemoriaCtrs MemoriaCore)
set_target_properties(MemoriaCtrs PROPERTIES CXX_STANDARD ${MEMORIA_INTERNAL_CXX_STANDARD})

add_subdirectory(stores)

add_subdirectory(seastar)
add_subdirectory(app_init)

if (BUILD_SANDBOX)
    add_subdirectory(sandbox)
endif()

if (BUILD_TESTS)
    add_subdirectory(tests-core)

    if (BUILD_TESTS_HRPC)
        add_subdirectory(tests-other/hrpc)
    endif()
endif()


if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if (BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

export(TARGETS MemoriaCore MemoriaInit MemoriaCtrs MemoriaStores MemoriaSeastar FILE ${LIBRARY_OUTPUT_PATH}/Memoria.cmake)

