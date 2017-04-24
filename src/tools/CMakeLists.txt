
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

include_directories(include)

file(GLOB TOOLS_LIB_SRC lib/*.cpp)


add_library(MemoriaTools STATIC ${TOOLS_LIB_SRC})
SET_TARGET_PROPERTIES(MemoriaTools PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_SRC=${CMAKE_SOURCE_DIR}")
target_link_libraries(MemoriaTools Memoria)



if (NOT DEFINED BUILD_TESTS)
    set(BUILD_TESTS false)
endif()

if (NOT DEFINED BUILD_BENCHMARKS)
    set(BUILD_BENCHMARKS false)
endif()

if (NOT DEFINED BUILD_EXAMPLES)
    set(BUILD_EXAMPLES false)
endif()

if (BUILD_TESTS) 
    add_subdirectory(tests)     
#    add_subdirectory(other-tests/disruptor)
#    add_subdirectory(other-tests/fiber)
#    add_subdirectory(other-tests/reactor)
endif()

if (BUILD_BENCHMARKS) 
    add_subdirectory(benchmarks)
endif()

if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
