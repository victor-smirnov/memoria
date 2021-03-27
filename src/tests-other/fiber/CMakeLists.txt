
# Copyright 2017 Victor Smirnov
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

file(GLOB SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)

FOREACH(TEST_SOURCE ${SRCS})
    get_filename_component(TEST_TARGET ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_TARGET} ${TEST_SOURCE})
    SET_TARGET_PROPERTIES(${TEST_TARGET} PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
    SET_TARGET_PROPERTIES(${TEST_TARGET} PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")
    
    target_link_libraries(${TEST_TARGET} MemoriaCore MemoriaCoreCtrs MemoriaCoreStores ${MEMORIA_LIBS})
ENDFOREACH()
