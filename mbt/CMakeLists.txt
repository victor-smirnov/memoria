# Copyright 2022-2023 Victor Smirnov
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

SET(CMAKE_PREFIX_PATH /home/victor/clang/lib/cmake;/home/victor/clang/lib/llvm)

if (NOT MEMORIA_HAS_TOP_LEVEL)
    cmake_minimum_required (VERSION 3.22)
    project (MBT VERSION 0.1.0)

    find_package(absl CONFIG REQUIRED)
    find_package(fmt CONFIG REQUIRED)
    find_package(unofficial-hash-library CONFIG REQUIRED)
    find_package(ICU 60.2 COMPONENTS i18n uc data REQUIRED)

    find_package(Boost 1.74.0 COMPONENTS filesystem program_options REQUIRED)

    SET(CORE_LIB ../core/lib)
    SET(MEMORIA_API_CXX_STANDARD 17)
    SET(MEMORIA_INTERNAL_CXX_STANDARD 20)

    file(GLOB_RECURSE LIB_SOURCES ${CORE_LIB}/*.cpp)

    add_library(Core STATIC )
    target_include_directories(Core PUBLIC ../core/include)
    target_include_directories(Core PUBLIC ${CORE_LIB}/include)

    set_property(TARGET Core PROPERTY CXX_STANDARD ${MEMORIA_API_CXX_STANDARD})

    target_link_libraries(Core PUBLIC absl::base absl::strings absl::hash absl::meta absl::time)
    target_link_libraries(Core PUBLIC fmt::fmt)
    target_link_libraries(Core PUBLIC Boost::filesystem)
    target_link_libraries(Core PUBLIC ICU::i18n ICU::uc ICU::data)

    target_link_libraries(Core PRIVATE unofficial::hash-library)

    target_compile_definitions(Core PUBLIC  -DMMA_ICU_CXX_NS=icu)
    target_compile_definitions(Core PRIVATE -DMEMORIA_SRC=${CMAKE_SOURCE_DIR})
    target_compile_definitions(Core PUBLIC  -DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE)

    target_compile_options(Core PUBLIC -pthread)

    target_sources(
        Core
        PRIVATE ${LIB_SOURCES}
    )
endif()

find_package(inja CONFIG REQUIRED)
find_package(Clang CONFIG REQUIRED)


file (GLOB MBT_SOURCES *.cpp *.hpp)
file (GLOB_RECURSE MBT_HEDERS include/*.hpp)

add_executable(mbt)

target_include_directories(mbt PRIVATE include)
target_include_directories(mbt PRIVATE ${CLANG_INCLUDE_DIRS})
target_include_directories(mbt PRIVATE ${CMAKE_BINARY_DIR})

target_sources(mbt PRIVATE ${MBT_SOURCES} PRIVATE ${MBT_HEADERS})

target_link_libraries(mbt PRIVATE Core)
target_link_libraries(mbt PRIVATE Boost::program_options Boost::filesystem)
target_link_libraries(mbt PRIVATE clangBasic clangLex clangParse clangAST clangFrontend clangTooling)
target_link_libraries(mbt PRIVATE pantor::inja)


#execute_process(
#    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
#    COMMAND
#        ${CMAKE_CXX_COMPILER}
#        -E
#        -Wp,-v
#        -xc++
#        /dev/null
#    ERROR_VARIABLE COMPILER_INCLUDE_DIRS
#    OUTPUT_VARIABLE NO_NEEDED
#    COMMAND_ERROR_IS_FATAL ANY
#)

#message("&&&&&&&&&&&&&&&&&&&&&&&&&&&&& ${COMPILER_INCLUDE_DIRS} &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&")
#string(REPLACE " /" "/" COMPILER_INCLUDE_DIRS_LIST  "${COMPILER_INCLUDE_DIRS}")
#string(REPLACE "\n" ";" COMPILER_INCLUDE_DIRS_LIST  "${COMPILER_INCLUDE_DIRS_LIST}")

#list(FILTER COMPILER_INCLUDE_DIRS_LIST INCLUDE REGEX "^/")
#list(FILTER COMPILER_INCLUDE_DIRS_LIST EXCLUDE REGEX "gcc")

##message("&&&&&&&&&&&&&&&&&&&&&&&&&&&&& ${COMPILER_INCLUDE_DIRS_LIST} &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&")

#string(JOIN ":" COMPILER_INCLUDE_DIRS_STR ${COMPILER_INCLUDE_DIRS_LIST})
#string(PREPEND COMPILER_INCLUDE_DIRS_STR "const char* MBT_INCLUDES=\"")
#string(APPEND COMPILER_INCLUDE_DIRS_STR "\"\\;")

##message("+++++&&&&&&&&&&&&&&&&&&&&&&&&&&&&& ${COMPILER_INCLUDE_DIRS_STR} &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&")
#message("+++++&&&&&&&&&&&&&&&&&&&&&&&&&&&&& ${CXX_STANDARD} &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&")

#file(WRITE ${CMAKE_BINARY_DIR}/mbt_config.hpp ${COMPILER_INCLUDE_DIRS_STR})

file(GLOB_RECURSE LIB_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "include/*.*" )

target_sources(
  mbt
  PRIVATE ${CMAKE_BINARY_DIR}/mbt_config.hpp
  PRIVATE ${LIB_HEADERS}
)


install(TARGETS
  mbt
    EXPORT MemoriaTargets
    RUNTIME DESTINATION ${MEMORIA_TOOLS_INSTALL_DIR}
)
