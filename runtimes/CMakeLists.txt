
# Copyright 2023 Victor Smirnov
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

set_target_properties(RuntimeApi PROPERTIES CXX_STANDARD ${MEMORIA_API_CXX_STANDARD})
set_target_properties(RuntimeApi PROPERTIES OUTPUT_NAME memoria_runtime_api)

target_link_libraries(RuntimeApi PUBLIC Core)
target_include_directories(RuntimeApi PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/${MEMORIA_INCLUDES_INSTALL_DIR}/runtimes/include>
)

file(GLOB RUNTIME_API_SOURCES *.cpp)
file(GLOB_RECURSE RUNTIME_API_HEADERS include/*.hpp)

target_sources(
  RuntimeApi
  PRIVATE ${RUNTIME_API_HEADERS}
  PRIVATE ${RUNTIME_API_SOURCES}
)

install(
  DIRECTORY include DESTINATION ${CMAKE_INSTALL_PREFIX}/${MEMORIA_INCLUDES_INSTALL_DIR}/runtimes
)

install(TARGETS RuntimeApi EXPORT MemoriaTargets)

if (BUILD_SEASTAR)
    add_subdirectory(seastar)
endif()

add_subdirectory(asio)
add_subdirectory(reactor)


