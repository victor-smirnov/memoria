
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

add_subdirectory(app_init)
add_subdirectory(core)
add_subdirectory(stores)

add_subdirectory(containers)

add_subdirectory(context)
add_subdirectory(fiber)
add_subdirectory(reactor)
add_subdirectory(filesystem_reactor)
add_subdirectory(filesystem_common)

if (BUILD_REACTOR_MEMORIA_LIB)
    get_property(GLOBAL_CORE_SRCS_REACTOR GLOBAL PROPERTY GLOBAL_CORE_SOURCES_REACTOR)
    get_property(GLOBAL_INIT_SRCS_REACTOR GLOBAL PROPERTY GLOBAL_INIT_SOURCES_REACTOR)
    get_property(GLOBAL_CONTAINERS_SRCS_REACTOR GLOBAL PROPERTY GLOBAL_CONTAINERS_SOURCES_REACTOR)
    get_property(GLOBAL_SRORES_SRCS_REACTOR GLOBAL PROPERTY GLOBAL_STORES_SOURCES_REACTOR)

    if (MSVC)
        enable_language(ASM_MASM)
    else()
        enable_language(ASM)
    endif()

    add_library(MemoriaCoreReactor STATIC ${GLOBAL_CORE_SRCS_REACTOR})
    add_library(MemoriaInitReactor STATIC ${GLOBAL_INIT_SRCS_REACTOR})
    add_library(MemoriaCtrsReactor STATIC ${GLOBAL_CONTAINERS_SRCS_REACTOR})
    add_library(MemoriaStoresReactor STATIC ${GLOBAL_SRORES_SRCS_REACTOR})

    SET_TARGET_PROPERTIES(MemoriaInitReactor PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_SRC=${CMAKE_SOURCE_DIR}")
    SET_TARGET_PROPERTIES(MemoriaInitReactor PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")

    SET_TARGET_PROPERTIES(MemoriaCoreReactor PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_SRC=${CMAKE_SOURCE_DIR}")
    SET_TARGET_PROPERTIES(MemoriaCoreReactor PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")

    SET_TARGET_PROPERTIES(MemoriaCtrsReactor PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_SRC=${CMAKE_SOURCE_DIR}")
    SET_TARGET_PROPERTIES(MemoriaCtrsReactor PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")

    SET_TARGET_PROPERTIES(MemoriaStoresReactor PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_SRC=${CMAKE_SOURCE_DIR}")
    SET_TARGET_PROPERTIES(MemoriaStoresReactor PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")

    target_link_libraries(MemoriaCoreReactor ${MEMORIA_LIBS})
    target_link_libraries(MemoriaCtrsReactor MemoriaCoreReactor)
    target_link_libraries(MemoriaStoresReactor MemoriaCoreReactor)
    target_link_libraries(MemoriaInitReactor MemoriaCoreReactor MemoriaCtrsReactor MemoriaStoresReactor)

    set_property(TARGET MemoriaCoreReactor MemoriaInitReactor MemoriaCtrsReactor MemoriaStoresReactor PROPERTY CXX_STANDARD ${MEMORIA_INTERNAL_CXX_STANDARD})
    export(TARGETS MemoriaCoreReactor MemoriaInitReactor MemoriaCtrsReactor MemoriaStoresReactor FILE ${LIBRARY_OUTPUT_PATH}/MemoriaReactor.cmake)
endif()


if (BUILD_CLASSIC_MEMORIA_LIB)
    add_subdirectory(filesystem_classic)

    get_property(GLOBAL_CORE_SRCS GLOBAL PROPERTY GLOBAL_CORE_SOURCES)
    get_property(GLOBAL_INIT_SRCS GLOBAL PROPERTY GLOBAL_INIT_SOURCES)
    get_property(GLOBAL_CONTAINERS_SRCS GLOBAL PROPERTY GLOBAL_CONTAINERS_SOURCES)
    get_property(GLOBAL_SRORES_SRCS GLOBAL PROPERTY GLOBAL_STORES_SOURCES)

    add_library(MemoriaCore STATIC ${GLOBAL_CORE_SRCS})
    add_library(MemoriaInit STATIC ${GLOBAL_INIT_SRCS})
    add_library(MemoriaCtrs STATIC ${GLOBAL_CONTAINERS_SRCS})
    add_library(MemoriaStores STATIC ${GLOBAL_SRORES_SRCS})

    SET_TARGET_PROPERTIES(MemoriaInit PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_SRC=${CMAKE_SOURCE_DIR} -DMMA1_NO_REACTOR")
    SET_TARGET_PROPERTIES(MemoriaInit PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")

    SET_TARGET_PROPERTIES(MemoriaCore PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_SRC=${CMAKE_SOURCE_DIR} -DMMA1_NO_REACTOR")
    SET_TARGET_PROPERTIES(MemoriaCore PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")

    SET_TARGET_PROPERTIES(MemoriaCtrs PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_SRC=${CMAKE_SOURCE_DIR}")
    SET_TARGET_PROPERTIES(MemoriaCtrs PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")

    SET_TARGET_PROPERTIES(MemoriaStores PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_SRC=${CMAKE_SOURCE_DIR}")
    SET_TARGET_PROPERTIES(MemoriaStores PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")

    target_link_libraries(MemoriaCore ${MEMORIA_LIBS})
    target_link_libraries(MemoriaCtrs MemoriaCore)
    target_link_libraries(MemoriaStores MemoriaCore)
    target_link_libraries(MemoriaInit MemoriaCore MemoriaCtrs MemoriaStores)

    set_property(TARGET MemoriaCore MemoriaInit MemoriaCtrs MemoriaStores PROPERTY CXX_STANDARD ${MEMORIA_INTERNAL_CXX_STANDARD})
    export(TARGETS MemoriaCore MemoriaInit MemoriaCtrs MemoriaStores FILE ${LIBRARY_OUTPUT_PATH}/Memoria.cmake)
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

        if (BUILD_TESTS_FIBER)
            add_subdirectory(tests-other/fiber)
        endif()

        if (BUILD_TESTS_REACTOR)
            add_subdirectory(tests-other/reactor)
        endif()

        if (BUILD_TESTS_CONTEXT)
            add_subdirectory(tests-other/context)
        endif()
    endif()

    if (BUILD_TESTS_DISRUPTOR)
        add_subdirectory(tests-other/disruptor)
    endif()
endif()

if (BUILD_SANDBOX)    
    add_subdirectory(sandbox)
endif()

SET(MEMORIA_APPS)

if(BUILD_MSVC)
#    SET(MEMORIA_APPS ${MEMORIA_APPS} named_pipes)
endif()

if (BUILD_PYTHON_BINDINGS)
    add_subdirectory(python)
endif()

if (BUILD_REACTOR_MEMORIA_LIB AND BUILD_MEMORY_STORE)
    FOREACH(MEMORIA_TARGET ${MEMORIA_APPS})
        add_executable(${MEMORIA_TARGET} ${MEMORIA_TARGET}.cpp)
        SET_TARGET_PROPERTIES(${MEMORIA_TARGET} PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
        set_target_properties(${MEMORIA_TARGET} PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
        target_link_libraries(${MEMORIA_TARGET} MemoriaCoreReactor MemoriaCtrsReactor MemoriaStoresReactor ${MEMORIA_LIBS})
    ENDFOREACH()

    if (Qt5_FOUND)
        add_subdirectory(datascope)

        set(CMAKE_AUTOMOC ON)

        SET(MEMORIA_QT_APPS)

        FOREACH(MEMORIA_QT_TARGET ${MEMORIA_QT_APPS})
            add_executable(${MEMORIA_QT_TARGET} ${MEMORIA_QT_TARGET}.cpp)
            SET_TARGET_PROPERTIES(${MEMORIA_QT_TARGET} PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
            set_target_properties(${MEMORIA_QT_TARGET} PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
            target_link_libraries(${MEMORIA_QT_TARGET} MemoriaCoreReactor MemoriaCtrsReactor MemoriaStoresReactor ${MEMORIA_LIBS} ${Qt5_LIBRARIES})
            qt5_use_modules(${MEMORIA_QT_TARGET} Widgets Core Gui)
        ENDFOREACH()
    endif()
endif()



