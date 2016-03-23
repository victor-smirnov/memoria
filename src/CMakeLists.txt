
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

get_property(GLOBAL_SRCS GLOBAL PROPERTY GLOBAL_SOURCES)

if (EMIT_LLVM)
    add_library(Memoria OBJECT ${GLOBAL_SRCS})
    SET_TARGET_PROPERTIES(Memoria PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_MAIN ")
    SET_TARGET_PROPERTIES(Memoria PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_SRC=${CMAKE_SOURCE_DIR}")
else()
    add_library(Memoria STATIC ${GLOBAL_SRCS})
    SET_TARGET_PROPERTIES(Memoria PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_MAIN ")
    SET_TARGET_PROPERTIES(Memoria PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_SRC=${CMAKE_SOURCE_DIR}")
    SET_TARGET_PROPERTIES(Memoria PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
    
    export(TARGETS Memoria FILE ${LIBRARY_OUTPUT_PATH}/Memoria.cmake)
endif()

if (NOT DEFINED BUILD_TOOLS)
    set(BUILD_TOOLS true)
endif()

if (NOT DEFINED BUILD_SANDBOX)
    set(BUILD_SANDBOX false)
endif()

if (BUILD_TOOLS) 
    add_subdirectory(tools)
endif()

if (BUILD_SANDBOX) 
    add_subdirectory(sandbox)
endif()