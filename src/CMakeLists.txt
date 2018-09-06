
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

include_directories(../include)
include_directories(core/include)
#include_directories(jemalloc/include/linux)


add_subdirectory(core)
add_subdirectory(allocators)
add_subdirectory(containers)
add_subdirectory(fiber)
add_subdirectory(reactor)
add_subdirectory(filesystem)
add_subdirectory(yaml-cpp)


if (BUILD_MACOSX)
#    add_subdirectory(jemalloc/src/linux)
#    add_subdirectory(jemalloc/src/macosx)
elseif (BUILD_MSVC)
#    add_subdirectory(jemalloc/src/linux)
else()
#    add_subdirectory(jemalloc/src/linux)
endif()


get_property(GLOBAL_SRCS GLOBAL PROPERTY GLOBAL_SOURCES)

add_library(Memoria STATIC ${GLOBAL_SRCS})

SET_TARGET_PROPERTIES(Memoria PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_SRC=${CMAKE_SOURCE_DIR}")
SET_TARGET_PROPERTIES(Memoria PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
target_link_libraries(Memoria ${MEMORIA_LIBS})


if (BUILD_MACOSX)
    target_include_directories(Memoria PRIVATE BEFORE jemalloc/include/macosx)
elseif (BUILD_MSVC)
    target_include_directories(Memoria PRIVATE BEFORE jemalloc/include/msvc/msvc_compat)
    target_include_directories(Memoria PRIVATE BEFORE jemalloc/include/msvc)
else()
    target_include_directories(Memoria PRIVATE BEFORE jemalloc/include/linux)
endif()

export(TARGETS Memoria FILE ${LIBRARY_OUTPUT_PATH}/Memoria.cmake)



if (BUILD_CLASSIC_MEMORIA_LIB)

    add_library(MemoriaLMDB STATIC lmdb/mdb.c lmdb/midl.c)

    target_include_directories(MemoriaLMDB PUBLIC lmdb/include)
    set_target_properties(MemoriaLMDB PROPERTIES COMPILE_FLAGS "-pthread -fPIC")
    set_target_properties(MemoriaLMDB PROPERTIES LINK_FLAGS "")
    target_link_libraries(MemoriaLMDB pthread)

    set(LMDB_TOOLS mdb_copy mdb_load mdb_dump mdb_stat mtest mtest2 mtest3 mtest4 mtest5)

    FOREACH(LMDB_TOOLS_TARGET_BASE ${LMDB_TOOLS})
        set(LMDB_TOOLS_TARGET mma1_${LMDB_TOOLS_TARGET_BASE})
        add_executable(${LMDB_TOOLS_TARGET} lmdb/${LMDB_TOOLS_TARGET_BASE}.c)
        set_target_properties(${LMDB_TOOLS_TARGET} PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
        set_target_properties(${LMDB_TOOLS_TARGET} PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
        target_link_libraries(${LMDB_TOOLS_TARGET} MemoriaLMDB)
    ENDFOREACH()


    get_property(GLOBAL_SRCS_CLASSIC GLOBAL PROPERTY GLOBAL_SOURCES_CLASSIC)
    add_library(MemoriaClassic STATIC ${GLOBAL_SRCS_CLASSIC})

    SET_TARGET_PROPERTIES(MemoriaClassic PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_SRC=${CMAKE_SOURCE_DIR} -DMMA1_NO_REACTOR")
    SET_TARGET_PROPERTIES(MemoriaClassic PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
    target_link_libraries(MemoriaClassic MemoriaLMDB ${MEMORIA_LIBS})

    if (BUILD_MACOSX)
        target_include_directories(MemoriaClassic PRIVATE BEFORE jemalloc/include/macosx)
    elseif (BUILD_MSVC)
        target_include_directories(MemoriaClassic PRIVATE BEFORE jemalloc/include/msvc/msvc_compat)
        target_include_directories(MemoriaClassic PRIVATE BEFORE jemalloc/include/msvc)
    else()
        target_include_directories(MemoriaClassic PRIVATE BEFORE jemalloc/include/linux)
    endif()

    export(TARGETS MemoriaClassic MemoriaLMDB FILE ${LIBRARY_OUTPUT_PATH}/MemoriaClassic.cmake)
endif()





if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if (BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()



if (BUILD_TESTS) 
    add_subdirectory(tests-core)
#    add_subdirectory(tests-other/disruptor)
#    add_subdirectory(tests-other/fiber)
#    add_subdirectory(tests-other/reactor)
endif()

if (BUILD_SANDBOX)
    add_subdirectory(sandbox)
    if (BUILD_CLASSIC_MEMORIA_LIB)
        add_subdirectory(sandbox-classic)
    endif()
endif()


#SET(MEMORIA_APPS tcp_server tcp_client disruptor_bm spsc_queue_bm timers alloc shared_ptrs yaml)
#SET(MEMORIA_APPS ${MEMORIA_APPS} ustrings icu_regexp fmt_u16 sockets anon_pipes processes crash print_env)

SET(MEMORIA_APPS)

if(BUILD_MSVC)
#    SET(MEMORIA_APPS ${MEMORIA_APPS} named_pipes)
endif()


FOREACH(MEMORIA_TARGET ${MEMORIA_APPS})
    add_executable(${MEMORIA_TARGET} ${MEMORIA_TARGET}.cpp)
    SET_TARGET_PROPERTIES(${MEMORIA_TARGET} PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
    set_target_properties(${MEMORIA_TARGET} PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
    target_link_libraries(${MEMORIA_TARGET} Memoria ${MEMORIA_LIBS})
ENDFOREACH()

if (Qt5_FOUND)
    add_subdirectory(datascope)

    set(CMAKE_AUTOMOC ON)

    SET(MEMORIA_QT_APPS)

    FOREACH(MEMORIA_QT_TARGET ${MEMORIA_QT_APPS})
        add_executable(${MEMORIA_QT_TARGET} ${MEMORIA_QT_TARGET}.cpp)
        SET_TARGET_PROPERTIES(${MEMORIA_QT_TARGET} PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
        set_target_properties(${MEMORIA_QT_TARGET} PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
        target_link_libraries(${MEMORIA_QT_TARGET} Memoria ${MEMORIA_LIBS} ${Qt5_LIBRARIES})
        qt5_use_modules(${MEMORIA_QT_TARGET} Widgets Core Gui)
    ENDFOREACH()
endif()
