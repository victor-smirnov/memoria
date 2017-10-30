
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


add_executable(intellectio intellectio.cpp)
SET_TARGET_PROPERTIES(intellectio PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
set_target_properties(intellectio PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
target_link_libraries(intellectio Memoria ${MEMORIA_LIBS})

add_executable(disruptor_bm disruptor_bm.cpp)
SET_TARGET_PROPERTIES(disruptor_bm PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
set_target_properties(disruptor_bm PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
target_link_libraries(disruptor_bm Memoria ${MEMORIA_LIBS})

add_executable(spsc_queue_bm spsc_queue_bm.cpp)
SET_TARGET_PROPERTIES(spsc_queue_bm PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
set_target_properties(spsc_queue_bm PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
target_link_libraries(spsc_queue_bm Memoria ${MEMORIA_LIBS})
