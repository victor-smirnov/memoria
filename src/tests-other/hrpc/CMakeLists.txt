
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

file(GLOB SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *_test.cpp)


add_executable(hrpc_tests_server hrpc_tests_server.cpp ${SRCS})
SET_TARGET_PROPERTIES(hrpc_tests_server PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
SET_TARGET_PROPERTIES(hrpc_tests_server PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")

target_link_libraries(hrpc_tests_server MemoriaCtrsReactor MemoriaStoresReactor MemoriaCoreReactor MemoriaInitReactor ${MEMORIA_LIBS})

add_executable(hrpc_tests_client hrpc_tests_client.cpp ${SRCS})
SET_TARGET_PROPERTIES(hrpc_tests_client PROPERTIES COMPILE_FLAGS "${MEMORIA_COMPILE_FLAGS}")
SET_TARGET_PROPERTIES(hrpc_tests_client PROPERTIES LINK_FLAGS "${MEMORIA_LINK_FLAGS}")

target_link_libraries(hrpc_tests_client MemoriaCtrsReactor MemoriaStoresReactor MemoriaCoreReactor MemoriaInitReactor ${MEMORIA_LIBS})
