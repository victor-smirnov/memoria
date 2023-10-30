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
    find_package(unofficial-lmdb REQUIRED)

    file (GLOB LMDB_SOURCES lmdb/*.cpp lmdb/*.c)
    file (GLOB_RECURSE LMDB_HEADERS lmdb/*.hpp lmdb/*.h)

    target_link_libraries(Stores PRIVATE unofficial::lmdb::lmdb)
    target_include_directories(Stores PRIVATE unofficial::lmdb::lmdb)
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

  if (BUILD_SEASTAR)
    file (GLOB_RECURSE SWMR_SEASTAR_SOURCES oltp_seastar/*.cpp)
    file (GLOB_RECURSE SWMR_SEASTAR_HEADERS oltp_seastar/*.hpp)
    target_include_directories(StoresSeastar PRIVATE oltp_seastar)
    target_sources(StoresSeastar PRIVATE ${SWMR_SEASTAR_SOURCES} ${SWMR_SEASTAR_HEADERS})
  endif()
endif()

file (GLOB_RECURSE COMMON_SOURCES common/*.cpp)
file (GLOB_RECURSE COMMON_HEADERS common/*.hpp)

file (GLOB_RECURSE STORE_HEADERS include/*.hpp)

set_target_properties(Stores PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_SRC=${CMAKE_SOURCE_DIR}")
set_target_properties(Stores PROPERTIES CXX_STANDARD ${MEMORIA_INTERNAL_CXX_STANDARD})
set_target_properties(Stores PROPERTIES OUTPUT_NAME memoria_stores)
target_link_libraries(Stores PUBLIC StoresApi Containers)
target_include_directories(Stores PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/stores/include>
)



file (GLOB LIB_SOURCES *.cpp)
file (GLOB LIB_HEADERS *.hpp)

target_sources(
    Stores
    PRIVATE FILE_SET HEADERS FILES ${STORE_HEADERS}
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

install(
  DIRECTORY include DESTINATION ${CMAKE_INSTALL_PREFIX}/stores
)

install(TARGETS
  Stores EXPORT MemoriaTargets
)


if (BUILD_SEASTAR)
  set_target_properties(StoresSeastar PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_SRC=${CMAKE_SOURCE_DIR}")
  set_target_properties(StoresSeastar PROPERTIES CXX_STANDARD ${MEMORIA_INTERNAL_CXX_STANDARD})
  set_target_properties(StoresSeastar PROPERTIES OUTPUT_NAME memoria_stores_seastar)
  target_link_libraries(StoresSeastar PUBLIC StoresApi Containers Stores SeastarLib)
  target_include_directories(StoresSeastar PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/stores-seastar/include>
  )

  target_sources(
    StoresSeastar
    PRIVATE FILE_SET HEADERS FILES ${STORE_HEADERS}
    PRIVATE
        ${LIB_HEADERS}
        ${LMDB_SOURCES} ${LMDB_HEADERS}
        ${MEMORY_COW_THREADS_SOURCES} ${MEMORY_COW_THREADS_HEADERS}
        ${MEMORY_SWMR_MAPPED_SOURCES} ${MEMORY_SWMR_MAPPED_HEADERS}
        ${SWMR_FIBERS_SOURCES} ${SWMR_FIBERS_HEADERS}
        ${SWMR_LITE_FIBERS_SOURCES} ${SWMR_LITE_FIBERS_HEADERS}
        ${SWMR_LITE_MAPPED_SOURCES} ${SWMR_LITE_MAPPED_HEADERS}
        ${SWMR_LITE_RAW_SOURCES} ${SWMR_LITE_RAW_HEADERS}
        ${COMMON_SOURCES} ${COMMON_HEADERS}
  )

  install(
    DIRECTORY include DESTINATION ${CMAKE_INSTALL_PREFIX}/stores-seastar
  )

  install(TARGETS
    StoresSeastar EXPORT MemoriaTargets
  )
endif()
