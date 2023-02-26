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

#add_library(Stores STATIC)

if (NO_COW_PROFILE)
    find_package(lmdb REQUIRED)

    file (GLOB LMDB_SOURCES lmdb/*.cpp lmdb/*.c)
    file (GLOB_RECURSE LMDB_HEADERS lmdb/*.hpp lmdb/*.h)

    target_link_libraries(Stores PRIVATE lmdb)
    target_include_directories(Stores PRIVATE lmdb)
endif()

if (COW_PROFILE)
    file (GLOB_RECURSE MEMORY_COW_THREADS_SOURCES memory_cow_threads/*.cpp)
    file (GLOB_RECURSE MEMORY_COW_THREADS_HEADERS memory_cow_threads/*.hpp)
    target_include_directories(Stores PRIVATE memory_cow_threads)

    file (GLOB_RECURSE MEMORY_SWMR_MAPPED_SOURCES swmr_mapped/*.cpp)
    file (GLOB_RECURSE MEMORY_SWMR_MAPPED_HEADERS swmr_mapped/*.hpp)
    target_include_directories(Stores PRIVATE swmr_mapped)
endif()

if (COW_LITE_PROFILE)
    file (GLOB_RECURSE SWMR_FIBERS_SOURCES swmr_fibers/*.cpp)
    file (GLOB_RECURSE SWMR_FIBERS_HEADERS swmr_fibers/*.hpp)
    target_include_directories(Stores PRIVATE swmr_fibers)

    file (GLOB_RECURSE SWMR_LITE_FIBERS_SOURCES swmr_lite_fibers/*.cpp)
    file (GLOB_RECURSE SWMR_LITE_FIBERS_HEADERS swmr_lite_fibers/*.hpp)
    target_include_directories(Stores PRIVATE swmr_lite_fibers)

    file (GLOB_RECURSE SWMR_LITE_MAPPED_SOURCES swmr_lite_mapped/*.cpp)
    file (GLOB_RECURSE SWMR_LITE_MAPPED_HEADERS swmr_lite_mapped/*.hpp)
    target_include_directories(Stores PRIVATE swmr_lite_mapped)

    file (GLOB_RECURSE SWMR_LITE_RAW_SOURCES swmr_lite_raw/*.cpp)
    file (GLOB_RECURSE SWMR_LITE_RAW_HEADERS swmr_lite_raw/*.hpp)
    target_include_directories(Stores PRIVATE swmr_lite_raw)
endif()

file (GLOB_RECURSE COMMON_SOURCES common/*.cpp)
file (GLOB_RECURSE COMMON_HEADERS common/*.hpp)

SET_TARGET_PROPERTIES(Stores PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_SRC=${CMAKE_SOURCE_DIR}")
set_target_properties(Stores PROPERTIES CXX_STANDARD ${MEMORIA_INTERNAL_CXX_STANDARD})
set_target_properties(Stores PROPERTIES OUTPUT_NAME memoria_stores)
target_link_libraries(Stores PUBLIC Core)
target_include_directories(Stores PUBLIC ../include)

file (GLOB LIB_SOURCES *.cpp)
file (GLOB LIB_HEADERS *.hpp)

target_sources(
    Stores
    PRIVATE
        ${LIB_SOURCES} ${LIB_HEADERS}
        ${LMDB_SOURCES} ${LMDB_HEADERS}
        ${MEMORY_COW_THREADS_SOURCES} ${MEMORY_COW_THREADS_HEADERS}
        ${MEMORY_SWMR_MAPPED_SOURCES} ${MEMORY_SWMR_MAPPED_HEADERS}
        ${SWMR_FIBERS_SOURCES} ${SWMR_FIBERS_HEADERS}
        ${SWMR_LITE_FIBERS_SOURCES} ${SWMR_LITE_FIBERS_HEADERS}
        ${SWMR_LITE_MAPPED_SOURCES} ${SWMR_LITE_MAPPED_HEADERS}
        ${SWMR_LITE_RAW_SOURCES} ${SWMR_LITE_RAW_HEADERS}
        ${COMMON_SOURCES} ${COMMON_HEADERS}
)
