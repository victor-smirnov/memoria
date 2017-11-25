
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
add_subdirectory(allocators)
add_subdirectory(containers)
add_subdirectory(fiber)
add_subdirectory(reactor)
add_subdirectory(filesystem)


get_property(GLOBAL_SRCS GLOBAL PROPERTY GLOBAL_SOURCES)


add_library(Memoria STATIC ${GLOBAL_SRCS})

SET_TARGET_PROPERTIES(Memoria PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_SRC=${CMAKE_SOURCE_DIR}")
SET_TARGET_PROPERTIES(Memoria PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
target_link_libraries(Memoria ${MEMORIA_LIBS})

    
export(TARGETS Memoria FILE ${LIBRARY_OUTPUT_PATH}/Memoria.cmake)


if (BUILD_EXAMPLES) 
    add_subdirectory(examples)
endif()


if (BUILD_TESTS) 
    add_subdirectory(tests-core)     
#    add_subdirectory(tests-other/disruptor)
#    add_subdirectory(tests-other/fiber)
#    add_subdirectory(tests-other/reactor)
endif()

if (BUILD_SANDBOX) 
    add_subdirectory(sandbox)
endif()


SET(MEMORIA_APPS intellectio tcp_client disruptor_bm spsc_queue_bm files timers)

FOREACH(MEMORIA_TARGET ${MEMORIA_APPS})
    add_executable(${MEMORIA_TARGET} ${MEMORIA_TARGET}.cpp)
    SET_TARGET_PROPERTIES(${MEMORIA_TARGET} PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
    set_target_properties(${MEMORIA_TARGET} PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
    target_link_libraries(${MEMORIA_TARGET} Memoria ${MEMORIA_LIBS})
ENDFOREACH()

