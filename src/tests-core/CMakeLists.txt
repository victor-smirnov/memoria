
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

add_library(MemoriaTests2 STATIC ${TOOLS_LIB_SRC})
SET_TARGET_PROPERTIES(MemoriaTests2 PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS} -DMEMORIA_SRC=${CMAKE_SOURCE_DIR}")

target_link_libraries(MemoriaTests2 Memoria ${MEMORIA_LIBS})

add_subdirectory(tests)


