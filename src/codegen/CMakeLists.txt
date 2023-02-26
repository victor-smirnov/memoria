
# Copyright 2021-2022 Victor Smirnov
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


add_definitions("-Wno-ambiguous-reversed-operator")

if (BUILD_CODEGENERATOR_EXPLICITLY)
    find_package(Clang CONFIG REQUIRED)
    find_package(inja CONFIG REQUIRED)

    file(GLOB CTRGEN_SRCS *.cpp)
    add_executable(MemoriaCodeGen ${CTRGEN_SRCS} ${GLOBAL_CORE_SRCS})
    set_property(TARGET MemoriaCodeGen PROPERTY CXX_STANDARD ${MEMORIA_INTERNAL_CXX_STANDARD})

    target_include_directories(MemoriaCodeGen PRIVATE ../codegen/include)
    target_include_directories(MemoriaCodeGen PRIVATE include)
    target_include_directories(MemoriaCodeGen PRIVATE ${CLANG_INCLUDE_DIRS})

    target_link_libraries(MemoriaCodeGen PRIVATE Boost::program_options util)
    target_link_libraries(MemoriaCodeGen PRIVATE clangBasic clangLex clangParse clangAST clangFrontend clangTooling)
    target_link_libraries(MemoriaCodeGen PRIVATE pantor::inja hash-library)
else()

project(MemoriaCodeGen)
cmake_minimum_required(VERSION 3.19.0)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/Memoria.cmake)

set(CMAKE_CXX_STANDARD 17)

include_directories(../../include)
include_directories(../../core/include)

add_definitions(-DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE)
add_definitions(-DMMA_ICU_CXX_NS=icu)

if (MSVC)
    set (Boost_USE_STATIC_RUNTIME    ON)
    set (Boost_USE_STATIC_LIBS       ON)
    find_package(ICU 60.2 COMPONENTS dt i18n uc REQUIRED)
    link_libraries(ICU::dt ICU::uc ICU::i18n)
else()
    set (Boost_USE_STATIC_RUNTIME    ${BUILD_WITH_STATIC_BOOST})
    set (Boost_USE_STATIC_LIBS       ${BUILD_WITH_STATIC_BOOST})
    find_package(ICU 60.2 COMPONENTS i18n uc data REQUIRED)
    link_libraries(ICU::i18n ICU::uc ICU::data)
endif()

find_package(Boost 1.75.0 COMPONENTS context program_options serialization unit_test_framework filesystem REQUIRED)

find_package(absl CONFIG REQUIRED)
link_libraries(absl::base absl::strings absl::hash absl::meta absl::time util)

find_package(fmt CONFIG REQUIRED)
link_libraries(fmt::fmt fmt::fmt-header-only)

find_package(inja CONFIG REQUIRED)
find_package(Clang CONFIG REQUIRED)

find_package(unofficial-hash-library CONFIG REQUIRED)

STRING( REPLACE "^^" ";" MEMORIA_CORE_SRCS "${MEMORIA_CORE_SRCS_STR}" )

set(CORE_PATH ../../core)

file(GLOB_RECURSE  MEMORIA_CORE_LIB_SRCS ${CORE_PATH}/lib/*.* include)


file(GLOB CTRGEN_SRCS *.cpp)
add_executable(MemoriaCodeGen ${CTRGEN_SRCS} ${MEMORIA_CORE_SRCS} ${MEMORIA_CORE_LIB_SRCS})

target_include_directories(MemoriaCodeGen PRIVATE ${CORE_PATH}/lib/include)
target_include_directories(MemoriaCodeGen PRIVATE ../../include)
target_include_directories(MemoriaCodeGen PRIVATE ../codegen/include)
target_include_directories(MemoriaCodeGen PRIVATE include)
target_include_directories(MemoriaCodeGen PRIVATE ${CLANG_INCLUDE_DIRS})

target_link_libraries(MemoriaCodeGen PRIVATE Boost::program_options Boost::filesystem)
target_link_libraries(MemoriaCodeGen PRIVATE clangBasic clangLex clangParse clangAST clangFrontend clangTooling c++)
target_link_libraries(MemoriaCodeGen PRIVATE pantor::inja unofficial::hash-library)

endif()

