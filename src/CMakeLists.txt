
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

add_subdirectory(core)
add_subdirectory(stores)
add_subdirectory(containers)
add_subdirectory(fiber)
add_subdirectory(reactor)
add_subdirectory(filesystem)
add_subdirectory(yaml-cpp)

get_property(GLOBAL_SRCS GLOBAL PROPERTY GLOBAL_SOURCES)

if (BUILD_REACTOR_MEMORIA_LIB)
    add_library(Memoria STATIC ${GLOBAL_SRCS})

    SET_TARGET_PROPERTIES(Memoria PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_SRC=${CMAKE_SOURCE_DIR}")
    SET_TARGET_PROPERTIES(Memoria PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
    target_link_libraries(Memoria ${MEMORIA_LIBS})
    set_property(TARGET Memoria PROPERTY CXX_STANDARD ${MEMORIA_INTERNAL_CXX_STANDARD})

    export(TARGETS Memoria FILE ${LIBRARY_OUTPUT_PATH}/Memoria.cmake)
endif()

if (BUILD_CLASSIC_MEMORIA_LIB)
    get_property(GLOBAL_SRCS_CLASSIC GLOBAL PROPERTY GLOBAL_SOURCES_CLASSIC)
    add_library(MemoriaClassic STATIC ${GLOBAL_SRCS_CLASSIC})
    set_property(TARGET MemoriaClassic PROPERTY CXX_STANDARD ${MEMORIA_INTERNAL_CXX_STANDARD})

    SET_TARGET_PROPERTIES(MemoriaClassic PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_SRC=${CMAKE_SOURCE_DIR} -DMMA1_NO_REACTOR")
    SET_TARGET_PROPERTIES(MemoriaClassic PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")

    export(TARGETS MemoriaClassic FILE ${LIBRARY_OUTPUT_PATH}/MemoriaClassic.cmake)
endif()





if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if (BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()



if (BUILD_TESTS) 
    if (BUILD_REACTOR_MEMORIA_LIB)
        add_subdirectory(tests-core)
    endif()
#    add_subdirectory(tests-other/fiber)
#    add_subdirectory(tests-other/reactor)
endif()

if (BUILD_SANDBOX)    
    add_subdirectory(sandbox)
endif()


#SET(MEMORIA_APPS tcp_server tcp_client disruptor_bm spsc_queue_bm timers alloc shared_ptrs yaml)
#SET(MEMORIA_APPS ${MEMORIA_APPS} ustrings icu_regexp fmt_u16 sockets anon_pipes processes crash print_env)

SET(MEMORIA_APPS)

if(BUILD_MSVC)
#    SET(MEMORIA_APPS ${MEMORIA_APPS} named_pipes)
endif()

if (BUILD_REACTOR_MEMORIA_LIB)
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
endif()
